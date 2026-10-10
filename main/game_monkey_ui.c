// main/game_monkey_ui.c —— 《尖叫猴子》UI 实现(对象化渲染,全新设计,
// 不复用 demo 测试菜单外壳;配色/精灵规格见美术设计文档)。
#include "game_monkey_ui.h"
#include "game_monkey_text.h"
#include "game_monkey_sprites.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(sm_font_16);
LV_FONT_DECLARE(sm_font_24);

static const char *TAG = "sm_ui";

// ---- 调色板(美术设计 §2) ----
#define SM_C_SKY      0x9CE0FF
#define SM_C_GRASS    0x82BE2D
#define SM_C_GRASS_D  0x55951D
#define SM_C_DIRT     0x75452E
#define SM_C_DIRT_D   0x4A2C1B
#define SM_C_PIT      0x2B1B0E
#define SM_C_PIT_EDGE 0x6B4A2E
#define SM_C_INK      0x17202A
#define SM_C_PAPER    0xF4F4EA
#define SM_C_RED      0xE43B2F
#define SM_C_MUTED    0x3E5A6B
#define SM_C_BAR_G    0x4CAF50
#define SM_C_BAR_O    0xFF9800
#define SM_C_BAR_R    0xF44336

// ---- dB 柱几何(右缘竖直 12 段,阈值 50 刻度线) ----
#define SM_SEG_W      22
#define SM_SEG_H      6
#define SM_SEG_GAP    2
#define SM_SEG_BOTTOM 118
#define SM_THRESH_Y   69    // level=50 顶到第 6 段之间的刻度线

// ---- 模块内小工具 ----
static lv_obj_t *make_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                            uint32_t color) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_pad_all(l, 0, 0);
    return l;
}

static void set_text_if_changed(lv_obj_t *label, const char *text) {
    const char *cur = lv_label_get_text(label);
    if (cur == NULL || strcmp(cur, text) != 0) lv_label_set_text(label, text);
}

static lv_obj_t *make_rect(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t color) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static void set_pos_if_changed(lv_obj_t *o, int32_t x, int32_t y) {
    if (lv_obj_get_x(o) != x) lv_obj_set_x(o, x);
    if (lv_obj_get_y(o) != y) lv_obj_set_y(o, y);
}

// lv_anim 执行回调:style opa 的三参函数适配成两参动画回调。
static void opa_anim_cb(void *var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void set_hidden(lv_obj_t *o, bool hidden) {
    if (hidden) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

// ===========================================================================
// 标题屏
// ===========================================================================
static lv_obj_t *s_scr_title;
static lv_obj_t *s_title_monkey;
static lv_obj_t *s_title_board;
static lv_obj_t *s_title_hint;
static lv_obj_t *s_title_battery;
static bool s_title_frame;

void sm_ui_title_build(void) {
    if (s_scr_title) { lv_obj_delete(s_scr_title); s_scr_title = NULL; }
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(SM_C_SKY), 0);

    lv_obj_t *title = make_label(scr, "尖叫猴子", &sm_font_24, SM_C_INK);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    s_title_monkey = lv_image_create(scr);
    lv_image_set_src(s_title_monkey, &sm_img_monkey_run1);
    lv_image_set_scale(s_title_monkey, 512);   // 2x 立绘
    lv_image_set_antialias(s_title_monkey, false);
    lv_obj_align(s_title_monkey, LV_ALIGN_TOP_MID, 0, 92);
    s_title_frame = false;

    lv_obj_t *board_cap = make_label(scr, "排行榜", &sm_font_16, SM_C_INK);
    lv_obj_align(board_cap, LV_ALIGN_TOP_MID, 0, 172);

    s_title_board = make_label(scr, "--", &sm_font_16, SM_C_MUTED);
    lv_obj_set_style_text_align(s_title_board, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_title_board, 200);
    lv_obj_align(s_title_board, LV_ALIGN_TOP_MID, 0, 194);

    s_title_hint = make_label(scr, "按OK开始", &sm_font_16, SM_C_RED);
    lv_obj_align(s_title_hint, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_title_hint);
    lv_anim_set_exec_cb(&a, opa_anim_cb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 700);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    s_title_battery = make_label(scr, "", &sm_font_16, SM_C_MUTED);
    lv_obj_align(s_title_battery, LV_ALIGN_BOTTOM_RIGHT, -8, -6);

    s_scr_title = scr;
    lv_screen_load(scr);
}

void sm_ui_title_set_board(const sm_score_t *board, uint8_t count) {
    if (!s_title_board) return;
    char line[32];
    char text[SM_BOARD_N * 34 + 8];
    text[0] = '\0';
    if (count == 0) {
        strncat(text, "--", sizeof(text) - strlen(text) - 1);
    }
    for (uint8_t i = 0; i < count && i < SM_BOARD_N; i++) {
        if (board[i].year >= 2024) {
            snprintf(line, sizeof(line), "%u. %lu 米  %02u-%02u\n",
                     (unsigned)(i + 1), (unsigned long)board[i].meters,
                     board[i].month, board[i].day);
        } else {
            snprintf(line, sizeof(line), "%u. %lu 米\n",
                     (unsigned)(i + 1), (unsigned long)board[i].meters);
        }
        strncat(text, line, sizeof(text) - strlen(text) - 1);
    }
    set_text_if_changed(s_title_board, text);
}

void sm_ui_title_set_battery(int soc) {
    if (!s_title_battery) return;
    char text[24];
    if (soc < 0) {
        text[0] = '\0';
    } else {
        snprintf(text, sizeof(text), "电量 %d%%", soc);
    }
    set_text_if_changed(s_title_battery, text);
}

void sm_ui_title_tick(void) {
    if (!s_title_monkey) return;
    s_title_frame = !s_title_frame;
    lv_image_set_src(s_title_monkey,
                     s_title_frame ? &sm_img_monkey_run2 : &sm_img_monkey_run1);
}

// ===========================================================================
// 游戏屏
// ===========================================================================
static lv_obj_t *s_scr_game;
static lv_obj_t *s_g_mileage;
static lv_obj_t *s_g_hearts[SM_HP_MAX];
static lv_obj_t *s_g_segs[SM_BAR_SEGS];
static lv_obj_t *s_g_clouds[2];
static lv_obj_t *s_g_trees[4];
static lv_obj_t *s_g_grass_dashes[6];
static lv_obj_t *s_g_dirt_dots[4];
static lv_obj_t *s_g_gap_pit[2];
static lv_obj_t *s_g_gap_wall[2][2];    // [坑][左/右]
static lv_obj_t *s_g_stone[2];
static lv_obj_t *s_g_cactus[2];
static lv_obj_t *s_g_monkey;
static lv_obj_t *s_g_dizzy;
static lv_obj_t *s_g_dust[3];
static lv_obj_t *s_g_flash;
static lv_obj_t *s_g_over_panel;
static lv_obj_t *s_g_over_reason;
static lv_obj_t *s_g_over_miles;
static lv_obj_t *s_g_over_rank;
static lv_obj_t *s_g_calib_box;     // 校准覆盖层:提示 + 进度条
static lv_obj_t *s_g_calib_fill;
static lv_obj_t *s_g_ready;         // 「预备」倒计时标签
// 渲染缓存
static int32_t  s_c_monkey_y = -1;
static uint8_t  s_c_hp = 0xff;
static uint8_t  s_c_level = 0xff;
static uint32_t s_c_meters = 0xffffffffu;

void sm_ui_game_build(void) {
    if (s_scr_game) { lv_obj_delete(s_scr_game); s_scr_game = NULL; }
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(SM_C_SKY), 0);

    // ---- HUD:里程(左) + 心形血量(中) ----
    s_g_mileage = make_label(scr, "里程 0 米", &sm_font_16, SM_C_INK);
    lv_obj_align(s_g_mileage, LV_ALIGN_TOP_LEFT, 8, 5);
    for (int i = 0; i < SM_HP_MAX; i++) {
        s_g_hearts[i] = lv_image_create(scr);
        lv_image_set_src(s_g_hearts[i], &sm_img_heart);
        lv_obj_set_pos(s_g_hearts[i], 128 + i * 24, 4);
    }

    // ---- dB 柱状图(右缘竖直 12 段) ----
    for (int i = 0; i < SM_BAR_SEGS; i++) {
        const uint32_t color = (i < 6) ? SM_C_BAR_G : ((i < 9) ? SM_C_BAR_O : SM_C_BAR_R);
        lv_obj_t *seg = make_rect(scr, SM_BAR_X, SM_SEG_BOTTOM - 8 * i, SM_SEG_W, SM_SEG_H,
                                  color);
        lv_obj_set_style_bg_opa(seg, LV_OPA_20, 0);
        s_g_segs[i] = seg;
    }
    make_rect(scr, SM_BAR_X - 4, SM_THRESH_Y, SM_SEG_W + 8, 2, SM_C_INK);
    lv_obj_t *db = make_label(scr, "分贝", &sm_font_16, SM_C_INK);
    lv_obj_align(db, LV_ALIGN_TOP_RIGHT, -30, 8);
    lv_obj_t *th = make_label(scr, "50", &sm_font_16, SM_C_INK);
    lv_obj_align(th, LV_ALIGN_TOP_RIGHT, -30, SM_THRESH_Y - 4);

    // ---- 视差:云(0.25x) 与 中景树(0.5x,立于地面顶) ----
    for (int i = 0; i < 2; i++) {
        s_g_clouds[i] = lv_image_create(scr);
        lv_image_set_src(s_g_clouds[i], &sm_img_cloud);
        lv_obj_set_pos(s_g_clouds[i], 30 + i * 130, 40 + i * 26);
    }
    for (int i = 0; i < 4; i++) {
        s_g_trees[i] = lv_image_create(scr);
        lv_image_set_src(s_g_trees[i], &sm_img_tree);
        lv_obj_set_pos(s_g_trees[i], i * 48, SM_GROUND_Y - 16);
    }

    // ---- 地面:草顶 + 土层 + 纹理(1x 滚动) ----
    make_rect(scr, 0, SM_GROUND_Y, SM_LCD_W, 4, SM_C_GRASS);
    make_rect(scr, 0, SM_GROUND_Y + 4, SM_LCD_W, SM_LCD_H - SM_GROUND_Y - 4, SM_C_DIRT);
    for (int i = 0; i < 6; i++) {
        s_g_grass_dashes[i] = make_rect(scr, i * 40, SM_GROUND_Y + 1, 10, 2, SM_C_GRASS_D);
    }
    for (int i = 0; i < 4; i++) {
        s_g_dirt_dots[i] = make_rect(scr, 18 + i * 56, SM_GROUND_Y + 12 + (i % 2) * 9,
                                     3, 3, SM_C_DIRT_D);
    }

    // ---- 悬崖池(2 个坑:坑体 + 左右崖壁) ----
    for (int g = 0; g < 2; g++) {
        s_g_gap_pit[g] = make_rect(scr, 0, SM_GROUND_Y, 1, SM_LCD_H - SM_GROUND_Y, SM_C_PIT);
        s_g_gap_wall[g][0] = make_rect(scr, 0, SM_GROUND_Y, 2, SM_LCD_H - SM_GROUND_Y,
                                       SM_C_DIRT_D);
        s_g_gap_wall[g][1] = make_rect(scr, 0, SM_GROUND_Y, 2, SM_LCD_H - SM_GROUND_Y,
                                       SM_C_DIRT_D);
        set_hidden(s_g_gap_pit[g], true);
        set_hidden(s_g_gap_wall[g][0], true);
        set_hidden(s_g_gap_wall[g][1], true);
    }

    // ---- 障碍池 ----
    for (int i = 0; i < 2; i++) {
        s_g_stone[i] = lv_image_create(scr);
        lv_image_set_src(s_g_stone[i], &sm_img_stone);
        set_hidden(s_g_stone[i], true);
        s_g_cactus[i] = lv_image_create(scr);
        lv_image_set_src(s_g_cactus[i], &sm_img_cactus);
        set_hidden(s_g_cactus[i], true);
    }

    // ---- 猴子 + 「晕」标签 ----
    s_g_monkey = lv_image_create(scr);
    lv_image_set_src(s_g_monkey, &sm_img_monkey_run1);
    lv_obj_set_pos(s_g_monkey, SM_MONKEY_X, SM_GROUND_Y - SM_MONKEY_H);
    s_g_dizzy = make_label(scr, "晕", &sm_font_16, SM_C_RED);
    set_hidden(s_g_dizzy, true);

    // ---- 校准/预备覆盖层 ----
    s_g_calib_box = make_rect(scr, 40, 120, 160, 64, SM_C_PAPER);
    lv_obj_set_style_radius(s_g_calib_box, 8, 0);
    lv_obj_set_style_border_width(s_g_calib_box, 2, 0);
    lv_obj_set_style_border_color(s_g_calib_box, lv_color_hex(SM_C_INK), 0);
    lv_obj_set_style_border_opa(s_g_calib_box, LV_OPA_COVER, 0);
    lv_obj_t *calib_tip = make_label(s_g_calib_box, "请保持安静", &sm_font_16, SM_C_INK);
    lv_obj_align(calib_tip, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_t *calib_tip2 = make_label(s_g_calib_box, "校准中", &sm_font_16, SM_C_MUTED);
    lv_obj_align(calib_tip2, LV_ALIGN_TOP_MID, 0, 26);
    make_rect(s_g_calib_box, 20, 50, 120, 8, SM_C_SKY);
    s_g_calib_fill = make_rect(s_g_calib_box, 20, 50, 1, 8, SM_C_BAR_G);
    s_g_ready = make_label(scr, "预备", &sm_font_16, SM_C_RED);
    lv_obj_align(s_g_ready, LV_ALIGN_TOP_MID, 0, 130);
    set_hidden(s_g_calib_box, true);
    set_hidden(s_g_ready, true);

    // ---- 尘土与红闪 ----
    for (int i = 0; i < 3; i++) {
        s_g_dust[i] = make_rect(scr, 0, 0, 4, 4, SM_C_DIRT);
        lv_obj_set_style_opa(s_g_dust[i], LV_OPA_TRANSP, 0);
    }
    s_g_flash = make_rect(scr, 0, 0, SM_LCD_W, SM_LCD_H, SM_C_RED);
    lv_obj_set_style_opa(s_g_flash, LV_OPA_TRANSP, 0);

    // ---- 结算面板(隐藏) ----
    s_g_over_panel = make_rect(scr, 30, 104, 180, 112, SM_C_PAPER);
    lv_obj_set_style_radius(s_g_over_panel, 8, 0);
    lv_obj_set_style_border_width(s_g_over_panel, 2, 0);
    lv_obj_set_style_border_color(s_g_over_panel, lv_color_hex(SM_C_INK), 0);
    lv_obj_set_style_border_opa(s_g_over_panel, LV_OPA_COVER, 0);
    lv_obj_t *over_title = make_label(s_g_over_panel, "游戏结束", &sm_font_16, SM_C_INK);
    lv_obj_align(over_title, LV_ALIGN_TOP_MID, 0, 8);
    s_g_over_reason = make_label(s_g_over_panel, "", &sm_font_16, SM_C_RED);
    lv_obj_align(s_g_over_reason, LV_ALIGN_TOP_MID, 0, 30);
    s_g_over_miles = make_label(s_g_over_panel, "", &sm_font_16, SM_C_INK);
    lv_obj_align(s_g_over_miles, LV_ALIGN_TOP_MID, 0, 52);
    s_g_over_rank = make_label(s_g_over_panel, "", &sm_font_16, SM_C_RED);
    lv_obj_align(s_g_over_rank, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_t *over_hint = make_label(s_g_over_panel, "OK再来一局 UP返回标题",
                                     &sm_font_16, SM_C_MUTED);
    lv_obj_set_style_text_align(over_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(over_hint, 172);
    lv_obj_align(over_hint, LV_ALIGN_BOTTOM_MID, 0, -6);
    set_hidden(s_g_over_panel, true);

    // 渲染缓存复位
    s_c_monkey_y = -1;
    s_c_hp = 0xff;
    s_c_level = 0xff;
    s_c_meters = 0xffffffffu;

    s_scr_game = scr;
    lv_screen_load(scr);
}

static const lv_image_dsc_t *monkey_frame(const sm_ui_state_t *st) {
    if (st->phase == SM_PHASE_OVER && !st->falling) return &sm_img_monkey_hurt;
    if (st->falling) return &sm_img_monkey_jump_down;
    if (st->airborne) return st->jump_down ? &sm_img_monkey_jump_down
                                           : &sm_img_monkey_jump_up;
    return &sm_img_monkey_run1;
}

void sm_ui_game_sync(const sm_ui_state_t *st) {
    if (!s_scr_game) return;
    char text[48];

    // 阶段覆盖层:校准(进度) / 预备 / 运行(全部隐藏)
    if (st->phase == SM_PHASE_CALIB) {
        set_hidden(s_g_calib_box, false);
        set_hidden(s_g_ready, true);
        lv_obj_set_width(s_g_calib_fill, 1 + (int32_t)st->calib_percent * 118 / 100);
    } else if (st->phase == SM_PHASE_READY) {
        set_hidden(s_g_calib_box, true);
        set_hidden(s_g_ready, false);
        snprintf(text, sizeof(text), "预备 %lu", (unsigned long)(st->ready_left_ms / 1000u + 1u));
        set_text_if_changed(s_g_ready, text);
    } else {
        set_hidden(s_g_calib_box, true);
        set_hidden(s_g_ready, true);
    }

    // 里程
    if (st->meters != s_c_meters) {
        snprintf(text, sizeof(text), "里程 %lu 米", (unsigned long)st->meters);
        set_text_if_changed(s_g_mileage, text);
        s_c_meters = st->meters;
    }
    // 血量
    if (st->hp != s_c_hp) {
        for (int i = 0; i < SM_HP_MAX; i++) {
            const bool lost = (unsigned)i >= st->hp;
            lv_obj_set_style_opa(s_g_hearts[i], lost ? LV_OPA_20 : LV_OPA_COVER, 0);
        }
        s_c_hp = st->hp;
    }
    // dB 柱
    if (st->level != s_c_level) {
        const int lit = (int)((uint32_t)st->level * SM_BAR_SEGS / 100u);
        for (int i = 0; i < SM_BAR_SEGS; i++) {
            lv_obj_set_style_bg_opa(s_g_segs[i], i < lit ? LV_OPA_COVER : LV_OPA_20, 0);
        }
        s_c_level = st->level;
    }
    // 猴子:帧与位置
    lv_image_set_src(s_g_monkey, monkey_frame(st));
    const int32_t my = st->monkey_y - SM_MONKEY_H;
    if (my != s_c_monkey_y) {
        set_pos_if_changed(s_g_monkey, SM_MONKEY_X, my);
        s_c_monkey_y = my;
    }
    // 受击无敌期:app 按 8Hz 翻转 blink,true=本 tick 隐藏猴子。
    lv_obj_set_style_opa(s_g_monkey, st->blink ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    set_hidden(s_g_dizzy, !st->slow_active);
    if (st->slow_active) set_pos_if_changed(s_g_dizzy, SM_MONKEY_X, my - 20);

    // 障碍与悬崖(渲染缓存:先全部隐藏,再按快照摆位)
    int stone_i = 0, cactus_i = 0, gap_i = 0;
    for (uint8_t i = 0; i < st->obs_count && i < SM_OBST_MAX; i++) {
        const int32_t x = st->obs[i].x;
        switch (st->obs[i].kind) {
        case SM_OBST_STONE:
            if (stone_i < 2) {
                set_hidden(s_g_stone[stone_i], false);
                set_pos_if_changed(s_g_stone[stone_i], x, SM_GROUND_Y - SM_STONE_H);
                stone_i++;
            }
            break;
        case SM_OBST_CACTUS:
            if (cactus_i < 2) {
                set_hidden(s_g_cactus[cactus_i], false);
                set_pos_if_changed(s_g_cactus[cactus_i], x, SM_GROUND_Y - SM_CACTUS_H);
                cactus_i++;
            }
            break;
        case SM_OBST_GAP:
            if (gap_i < 2) {
                set_hidden(s_g_gap_pit[gap_i], false);
                set_pos_if_changed(s_g_gap_pit[gap_i], x, SM_GROUND_Y);
                lv_obj_set_width(s_g_gap_pit[gap_i], st->obs[i].w);
                set_hidden(s_g_gap_wall[gap_i][0], false);
                set_pos_if_changed(s_g_gap_wall[gap_i][0], x - 2, SM_GROUND_Y);
                set_hidden(s_g_gap_wall[gap_i][1], false);
                set_pos_if_changed(s_g_gap_wall[gap_i][1], x + st->obs[i].w, SM_GROUND_Y);
                gap_i++;
            }
            break;
        default:
            break;
        }
    }
    for (int i = stone_i; i < 2; i++) set_hidden(s_g_stone[i], true);
    for (int i = cactus_i; i < 2; i++) set_hidden(s_g_cactus[i], true);
    for (int g = gap_i; g < 2; g++) {
        set_hidden(s_g_gap_pit[g], true);
        set_hidden(s_g_gap_wall[g][0], true);
        set_hidden(s_g_gap_wall[g][1], true);
    }
}

void sm_ui_game_flash(void) {
    if (!s_g_flash) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_g_flash);
    lv_anim_set_exec_cb(&a, opa_anim_cb);
    lv_anim_set_values(&a, LV_OPA_60, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 80);
    lv_anim_set_repeat_count(&a, 1);
    lv_anim_start(&a);
}

void sm_ui_game_dust(void) {
    if (!s_scr_game) return;
    const int32_t feet = SM_GROUND_Y;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *p = s_g_dust[i];
        const int32_t x0 = SM_MONKEY_X + 4 + i * 7;
        lv_obj_set_style_opa(p, LV_OPA_COVER, 0);
        set_pos_if_changed(p, x0, feet - 4);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, p);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_set_values(&a, x0, x0 + (i - 1) * 12);
        lv_anim_set_time(&a, 380);
        lv_anim_set_repeat_count(&a, 1);
        lv_anim_start(&a);
        lv_anim_t b;
        lv_anim_init(&b);
        lv_anim_set_var(&b, p);
        lv_anim_set_exec_cb(&b, opa_anim_cb);
        lv_anim_set_values(&b, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_time(&b, 380);
        lv_anim_set_delay(&b, 60);
        lv_anim_set_repeat_count(&b, 1);
        lv_anim_start(&b);
    }
}

void sm_ui_game_over_show(const sm_ui_state_t *st) {
    if (!s_g_over_panel) return;
    char text[48];
    snprintf(text, sizeof(text), "里程 %lu 米", (unsigned long)st->meters);
    set_text_if_changed(s_g_over_miles, text);
    set_text_if_changed(s_g_over_reason,
                        st->death_cause == SM_DEATH_FALL ? "掉下去了" : "被砸晕了");
    if (st->record) {
        set_text_if_changed(s_g_over_rank, "新纪录! 恭喜");
    } else if (st->rank > 0) {
        snprintf(text, sizeof(text), "第 %u 名", st->rank);
        set_text_if_changed(s_g_over_rank, text);
    } else {
        set_text_if_changed(s_g_over_rank, "--");
    }
    set_hidden(s_g_over_panel, false);
}

// ===========================================================================
// 字形覆盖自检(设备端):文案与字体子集同源于 game_monkey_text.h
// ===========================================================================
void sm_ui_font_covers(void) {
    const lv_font_t *font = &sm_font_16;
    for (size_t i = 0; i < SM_TEXT_COUNT; i++) {
        const char *s = k_sm_text[i];
        while (*s) {
            uint32_t cp = 0;
            const uint8_t b0 = (uint8_t)*s;
            int len = 0;
            if (b0 < 0x80) { cp = b0; len = 1; }
            else if ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; len = 2; }
            else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; len = 3; }
            else { cp = b0 & 0x07; len = 4; }
            s += 1;
            for (int k = 1; k < len && *s; k++, s++) {
                cp = (cp << 6) | ((uint8_t)*s & 0x3F);
            }
            if (len == 1) continue;   // ASCII 由 Montserrat/内置兜底
            lv_font_glyph_dsc_t dsc;
            /* letter 参数是完整码点(非 UTF-8 首字节),与 lv_label 渲染路径一致 */
            if (!lv_font_get_glyph_dsc(font, &dsc, cp, cp)) {
                ESP_LOGW(TAG, "sm_font_16 缺字形 U+%04lX (%s)", (unsigned long)cp,
                         k_sm_text[i]);
            }
        }
    }
    ESP_LOGI(TAG, "字形覆盖自检完成");
}
