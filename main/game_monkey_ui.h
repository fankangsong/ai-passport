// main/game_monkey_ui.h —— 《尖叫猴子》UI 层(对象化渲染)。
// 只依赖 LVGL 与素材;世界状态经 sm_ui_state_t 快照输入,渲染缓存纪律:
// 标签/颜色/坐标变化才刷,位移量化到像素后比较。
#pragma once

#include "game_monkey_config.h"
#include "game_monkey_board.h"

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 每 tick 交给 UI 的状态快照(由 game_monkey_app 填写)。
typedef struct {
    sm_phase_t phase;
    uint32_t meters;
    uint8_t hp;                 // SM_HP_MAX..0
    int32_t monkey_y;           // 脚底 y(像素)
    bool airborne;
    bool jump_down;             // 空中下落段(切下落帧)
    bool falling;
    bool blink;                 // 受击无敌闪烁相位(true=本 tick 隐藏猴子)
    bool slow_active;           // 头顶「晕」标签
    uint8_t level;              // 0..100(校准完成后有效)
    uint8_t calib_percent;      // CALIB 期进度 0..100
    uint32_t ready_left_ms;
    struct {
        sm_obst_kind_t kind;
        int32_t x, w;
    } obs[SM_OBST_MAX];
    uint8_t obs_count;
    sm_death_t death_cause;
    bool record;                // 结算面板:新纪录
    uint8_t rank;               // 结算面板:名次 1..5,0=未上榜
} sm_ui_state_t;

// ---- 标题屏 ----
void sm_ui_title_build(void);                                   // 需已持 LVGL 锁
void sm_ui_title_set_board(const sm_score_t *board, uint8_t count);
void sm_ui_title_set_battery(int soc);                          // -1 = 不可用
void sm_ui_title_tick(void);                                    // 猴子跑步动画(10fps)

// ---- 游戏屏 ----
void sm_ui_game_build(void);                                    // 需已持 LVGL 锁
void sm_ui_game_sync(const sm_ui_state_t *st);                  // 每 tick 同步
void sm_ui_game_flash(void);                                    // 受击红闪(80ms)
void sm_ui_game_dust(void);                                     // 落地尘土
void sm_ui_game_over_show(const sm_ui_state_t *st);             // 结算面板

// 开机字形覆盖自检:逐码点检查 sm_font_16,缺失打警告(不阻塞)。
void sm_ui_font_covers(void);

#ifdef __cplusplus
}
#endif
