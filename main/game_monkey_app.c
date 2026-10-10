// main/game_monkey_app.c —— 应用层组装。
// 音频 worker:单任务交错读麦克风/写音效(读 15ms + 写 ≤15ms,与 DMA 描述符
// 对齐),音效期间检测器进 hold,杜绝扬声器回授;格式仅启动时设置一次。
// 世界 tick:LVGL 任务内的 lv_timer,算和画同任务,无需加锁。
#include "game_monkey_app.h"

#include "game_monkey_config.h"
#include "game_monkey_world.h"
#include "game_monkey_detector.h"
#include "game_monkey_sfx.h"
#include "game_monkey_board.h"
#include "game_monkey_store.h"
#include "game_monkey_ui.h"
#include "game_monkey_text.h"

#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_display.h"

#include "esp_log.h"
#include "esp_random.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <time.h>
#include <string.h>

static const char *TAG = "sm_app";

// ---- 音频 worker 与检测共享态 ----
static TaskHandle_t s_audio_task;
static SemaphoreHandle_t s_audio_started;   // set_format 完成握手
static QueueHandle_t s_sfx_queue;           // sm_sfx_t 队列,深度 4
static volatile uint8_t s_level;            // 检测器平滑输出 0..100

// ---- 会话状态 ----
typedef enum { SM_MODE_TITLE = 0, SM_MODE_GAME } sm_mode_t;
static sm_mode_t s_mode;
static sm_world_t s_world;
static sm_detector_t s_detector;
static bool s_shout_armed;
static bool s_session_active;               // 一局是否在进行(含 OVER)
static sm_score_t s_board[SM_BOARD_N];
static uint8_t s_board_count;
static uint32_t s_title_ticks;              // 标题屏跑步动画分频
static lv_timer_t *s_tick_timer;

// 音效播放游标(rAM 双缓冲:一条音效渲染一次,分块喂 BSP)
static int16_t s_sfx_buf[SM_SFX_MAX_SAMPLES];
static size_t s_sfx_len, s_sfx_pos;

// ---- 前向 ----
static void game_tick(lv_timer_t *timer);

// ===========================================================================
// 音频 worker
// ===========================================================================
static void audio_task(void *arg) {
    (void)arg;
    // 格式仅此一次:bsp_audio_set_format 内部 close/reopen,运行中变更危险。
    if (bsp_audio_set_format(SM_SAMPLE_RATE_HZ, 16, 1) != ESP_OK) {
        ESP_LOGE(TAG, "audio format init failed; mic disabled");
        xSemaphoreGive(s_audio_started);
        vTaskSuspend(NULL);
    }
    bsp_audio_set_volume(65);   // 抑制回授
    xSemaphoreGive(s_audio_started);

    static int16_t chunk[SM_CHUNK_SAMPLES];
    for (;;) {
        if (bsp_audio_read(chunk, sizeof(chunk)) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        sm_sfx_t fx;
        if (s_sfx_pos >= s_sfx_len && xQueueReceive(s_sfx_queue, &fx, 0) == pdTRUE) {
            s_sfx_len = sm_sfx_render(fx, s_sfx_buf, SM_SFX_MAX_SAMPLES, SM_SAMPLE_RATE_HZ);
            s_sfx_pos = 0;
        }
        if (s_sfx_pos < s_sfx_len) {
            const size_t n = (s_sfx_len - s_sfx_pos) < SM_CHUNK_SAMPLES
                                 ? (s_sfx_len - s_sfx_pos) : SM_CHUNK_SAMPLES;
            bsp_audio_write(&s_sfx_buf[s_sfx_pos], n * sizeof(int16_t));
            s_sfx_pos += n;
            // 音效期间只跟踪直流/地板,level 冻结(防回授)。
            sm_detector_hold(&s_detector, chunk, SM_CHUNK_SAMPLES);
        } else {
            s_level = sm_detector_push(&s_detector, chunk, SM_CHUNK_SAMPLES);
        }
    }
}

static void sfx_play(sm_sfx_t fx) {
    if (s_sfx_queue) (void)xQueueSend(s_sfx_queue, &fx, 0);
}

static esp_err_t audio_start(void) {
    if (s_audio_task) return ESP_OK;
    s_audio_started = xSemaphoreCreateBinary();
    s_sfx_queue = xQueueCreate(4, sizeof(sm_sfx_t));
    s_sfx_len = s_sfx_pos = 0;
    if (!s_audio_started || !s_sfx_queue) return ESP_ERR_NO_MEM;
    if (xTaskCreate(audio_task, "sm_audio", 4096, NULL, 4, &s_audio_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    if (xSemaphoreTake(s_audio_started, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGW(TAG, "audio start handshake timeout");
    }
    return ESP_OK;
}

// ===========================================================================
// 一局生命周期
// ===========================================================================
static void fill_date(sm_score_t *entry) {
    entry->year = entry->month = entry->day = 0;
    const time_t now = time(NULL);
    struct tm tm_now;
    if (localtime_r(&now, &tm_now) && tm_now.tm_year + 1900 >= 2024) {
        entry->year = (uint16_t)(tm_now.tm_year + 1900);
        entry->month = (uint8_t)(tm_now.tm_mon + 1);
        entry->day = (uint8_t)tm_now.tm_mday;
    }
}

static void build_ui_state(sm_ui_state_t *st) {
    memset(st, 0, sizeof(*st));
    st->phase = s_world.phase;
    st->meters = sm_world_meters(&s_world);
    st->hp = (uint8_t)s_world.hp;
    st->monkey_y = s_world.y_q >> SM_Q;
    st->airborne = s_world.airborne;
    st->jump_down = s_world.airborne && (s_world.vy_q > 0) && !s_world.falling;
    st->falling = s_world.falling;
    st->slow_active = s_world.slow_left_ms > 0;
    st->level = s_level;
    st->ready_left_ms = s_world.ready_left_ms;
    if (s_world.invuln_ms > 0) {
        st->blink = ((s_world.invuln_ms / 62u) & 1u) != 0u;   // ≈8Hz 闪烁
    }
    st->obs_count = s_world.obs_count;
    for (uint8_t i = 0; i < s_world.obs_count && i < SM_OBST_MAX; i++) {
        st->obs[i].kind = s_world.obs[i].kind;
        st->obs[i].x = s_world.obs[i].x;
        st->obs[i].w = s_world.obs[i].w;
    }
    st->death_cause = s_world.death_cause;
}

static void start_session(void) {
    sm_world_init(&s_world, (uint32_t)esp_random());
    sm_detector_reset_session(&s_detector);
    s_shout_armed = true;
    s_session_active = true;
    sm_ui_game_build();
}

static void show_over_panel(void) {
    sm_ui_state_t st;
    build_ui_state(&st);
    st.record = (st.meters > 0) && (s_board_count > 0) &&
                (s_board[0].meters == st.meters);
    // 名次:插入后按里程在榜内的位置(等于 rank 返回值;未上榜为 0)
    uint8_t rank = 0;
    for (uint8_t i = 0; i < s_board_count; i++) {
        if (s_board[i].meters == st.meters) { rank = (uint8_t)(i + 1); break; }
    }
    st.rank = rank;
    sm_ui_game_over_show(&st);
}

static void finish_session(void) {
    const uint32_t meters = sm_world_meters(&s_world);
    sm_score_t entry = { .meters = meters };
    fill_date(&entry);
    uint8_t rank = sm_board_insert(s_board, &s_board_count, entry, NULL);
    if (rank > 0) {
        const esp_err_t err = sm_store_save(s_board, s_board_count);
        if (err != ESP_OK) ESP_LOGW(TAG, "榜单保存失败: %s", esp_err_to_name(err));
        sfx_play(SM_SFX_RECORD);
        ESP_LOGI(TAG, "新纪录! %lu 米,第 %u 名", (unsigned long)meters, rank);
    } else {
        sfx_play(SM_SFX_DEATH);
    }
    show_over_panel();
}

// ===========================================================================
// 世界 tick(LVGL 任务内)
// ===========================================================================
static void game_tick(lv_timer_t *timer) {
    (void)timer;
    if (s_mode == SM_MODE_TITLE) {
        if (++s_title_ticks >= 3u) {   // 33ms×3 ≈ 10fps 跑步动画
            s_title_ticks = 0;
            sm_ui_title_tick();
        }
        return;
    }
    if (!s_session_active) return;

    sm_ui_state_t st;
    build_ui_state(&st);

    if (s_world.phase == SM_PHASE_CALIB) {
        const uint8_t done = SM_CALIB_CHUNKS - s_detector.calib_left;
        st.calib_percent = (uint8_t)(done * 100u / SM_CALIB_CHUNKS);
        sm_ui_game_sync(&st);
        return;
    }

    if (s_world.phase == SM_PHASE_RUN) {
        if (sm_shout_edge(s_level, &s_shout_armed)) {
            if (sm_world_try_jump(&s_world, s_level)) sfx_play(SM_SFX_JUMP);
        }
    }

    const uint32_t events = sm_world_tick(&s_world, SM_UI_TICK_MS);
    if (events & SM_EVT_LAND) {
        sfx_play(SM_SFX_LAND);
        sm_ui_game_dust();
    }
    if (events & SM_EVT_HURT) {
        sfx_play(SM_SFX_HURT);
        sm_ui_game_flash();
    }
    if (events & SM_EVT_DEATH) {
        finish_session();
        return;
    }

    build_ui_state(&st);
    sm_ui_game_sync(&st);
}

// ===========================================================================
// 屏幕切换与按键
// ===========================================================================
static void show_title(void) {
    s_mode = SM_MODE_TITLE;
    s_session_active = false;
    sm_ui_title_build();
    sm_ui_title_set_board(s_board, s_board_count);
    sm_ui_title_set_battery(bsp_battery_soc());
}

static void start_game_locked(void) {
    s_mode = SM_MODE_GAME;
    start_session();
}

void sm_app_enter(void) {
    const esp_err_t store_err = sm_store_init();
    if (store_err != ESP_OK) {
        ESP_LOGW(TAG, "NVS 初始化失败(%s),榜单不可持久化", esp_err_to_name(store_err));
    }
    s_board_count = 0;
    memset(s_board, 0, sizeof(s_board));
    if (store_err == ESP_OK) {
        const esp_err_t load_err = sm_store_load(s_board, &s_board_count);
        if (load_err != ESP_OK) ESP_LOGW(TAG, "榜单读取失败: %s", esp_err_to_name(load_err));
    }

    const esp_err_t audio_err = audio_start();
    if (audio_err != ESP_OK) {
        ESP_LOGE(TAG, "音频 worker 启动失败: %s", esp_err_to_name(audio_err));
    }

    sm_detector_init(&s_detector);
    sm_ui_font_covers();   // 开机自检一次:文案↔字体子集逐码点核对
    s_tick_timer = lv_timer_create(game_tick, SM_UI_TICK_MS, NULL);
    show_title();

    ESP_LOGI(TAG, "就绪:音频=%s 榜单=%u 条",
             audio_err == ESP_OK ? "OK" : "FAIL", s_board_count);
}

void sm_app_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (!bsp_lvgl_lock(500)) return;

    if (s_mode == SM_MODE_TITLE) {
        if (ev == BSP_BTN_CLICK && btn == BSP_BTN_OK) start_game_locked();
    } else if (s_session_active && s_world.phase == SM_PHASE_OVER) {
        if (ev == BSP_BTN_CLICK && btn == BSP_BTN_OK) start_game_locked();
        else if (ev == BSP_BTN_CLICK && btn == BSP_BTN_UP) show_title();
    }
    bsp_lvgl_unlock();
}
