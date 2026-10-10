// main/game_monkey_world.h —— 《尖叫猴子》纯C世界模型(状态机+物理+障碍+计分)。
// 不依赖 ESP-IDF/LVGL,宿主测试直接编译本模块。
#pragma once

#include "game_monkey_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    sm_obst_kind_t kind;
    int32_t x;           // 屏幕坐标左缘,随滚动左移(像素)
    int32_t w, h;        // 碰撞盒;GAP 时 h=0、w=缺口宽
    bool hit;            // 石/仙人掌已被撞过(一次性)
} sm_obst_t;

typedef struct {
    sm_phase_t phase;
    int32_t y_q;         // 猴子脚底 y(Q16.16),地面 = SM_GROUND_Y << SM_Q
    int32_t vy_q;        // 垂直速度(Q16.16),向下为正
    bool airborne;
    bool falling;        // 坠崖下落中(不再着陆)
    int hp;              // SM_HP_MAX..0
    uint32_t dist_px;    // 累计滚动像素(里程 = dist_px / SM_PX_PER_METER)
    uint32_t slow_left_ms;
    uint32_t invuln_ms;
    uint32_t ready_left_ms;
    uint32_t run_ms;     // RUN 阶段累计(难度爬坡时基)
    sm_obst_t obs[SM_OBST_MAX];
    uint8_t obs_count;
    int32_t spawn_left_px;   // 距下一次生成的剩余滚动像素
    uint32_t rng;            // LCG 状态(种子注入,宿主测试确定性)
    uint32_t events;         // 本 tick 事件位图(SM_EVT_*,每 tick 清零重填)
    sm_death_t death_cause;
} sm_world_t;

// 初始化到 CALIB 空转态;seed=0 时取固定种子(宿主测试可注入任意种子)。
void sm_world_init(sm_world_t *w, uint32_t seed);

// 校准完成(CALIB → READY 倒计时)。
void sm_world_begin_run(sm_world_t *w);

// 推进一个 tick(dt 毫秒)。返回本 tick 事件位图,同时写入 w->events。
uint32_t sm_world_tick(sm_world_t *w, uint32_t dt_ms);

// 喊叫起跳:仅 RUN 且着地时有效;level(0..100)决定起跳初速(连续映射)。
// 事件不进 tick 位图,直接用返回值驱动音效/动画。
bool sm_world_try_jump(sm_world_t *w, uint8_t level);

// 喊叫上升沿判定(滞回):armed 初始为 true。
// level >= SHOUT_ON 且 armed → 触发并解除武装;level 回落到 SHOUT_OFF 之下才重新武装。
bool sm_shout_edge(uint8_t level, bool *armed);

// 当前里程(米)。
uint32_t sm_world_meters(const sm_world_t *w);

// 猴子碰撞盒(屏幕坐标,供 UI 高亮调试;逻辑内部使用同一组常量)。
void sm_world_monkey_box(const sm_world_t *w, int32_t *x0, int32_t *y0,
                         int32_t *x1, int32_t *y1);

#ifdef __cplusplus
}
#endif
