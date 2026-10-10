// main/game_monkey_world.c —— 纯C世界模型实现。
// 定点 Q16.16 半隐式欧拉;所有随机走注入种子的 LCG,宿主测试可复现。
#include "game_monkey_world.h"

#include <string.h>

static uint32_t lcg_next(uint32_t *rng) {
    *rng = *rng * 1664525u + 1013904223u;
    return *rng >> 8;   // 取高 24 位
}

void sm_world_init(sm_world_t *w, uint32_t seed) {
    memset(w, 0, sizeof(*w));
    w->rng = seed ? seed : 0x1234u;
    w->phase = SM_PHASE_CALIB;
    w->hp = SM_HP_MAX;
    w->y_q = (int32_t)SM_GROUND_Y << SM_Q;
    w->spawn_left_px = -1;   // 未开跑,不生成
}

void sm_world_begin_run(sm_world_t *w) {
    if (w->phase != SM_PHASE_CALIB) return;
    w->phase = SM_PHASE_READY;
    w->ready_left_ms = SM_READY_MS;
}

uint32_t sm_world_meters(const sm_world_t *w) {
    return w->dist_px / SM_PX_PER_METER;
}

void sm_world_monkey_box(const sm_world_t *w, int32_t *x0, int32_t *y0,
                         int32_t *x1, int32_t *y1) {
    const int32_t feet = w->y_q >> SM_Q;
    *x0 = SM_MONKEY_X + SM_BOX_OFF_X;
    *x1 = *x0 + SM_BOX_W;
    *y0 = feet - SM_MONKEY_H + SM_BOX_OFF_Y;
    *y1 = *y0 + SM_BOX_H;
}

bool sm_shout_edge(uint8_t level, bool *armed) {
    if (level >= SM_SHOUT_ON) {
        if (*armed) {
            *armed = false;
            return true;
        }
        return false;
    }
    if (level <= SM_SHOUT_OFF) *armed = true;
    return false;
}

bool sm_world_try_jump(sm_world_t *w, uint8_t level) {
    if (w->phase != SM_PHASE_RUN || w->airborne) return false;
    if (level < SM_SHOUT_ON) return false;
    if (level > SM_SHOUT_HI) level = SM_SHOUT_HI;
    const int32_t v0 = SM_V_JUMP_MIN_Q +
        (int32_t)(((int64_t)(SM_V_JUMP_MAX_Q - SM_V_JUMP_MIN_Q) *
                   (level - SM_SHOUT_ON)) / (SM_SHOUT_HI - SM_SHOUT_ON));
    w->vy_q = -v0;
    w->airborne = true;
    w->falling = false;
    return true;
}

// 当前滚动速度(Q16.16):难度爬坡 + 减速系数。
static uint32_t speed_q(const sm_world_t *w) {
    uint32_t s = SM_SPEED_BASE_Q;
    if (w->run_ms >= SM_RAMP_MS) {
        s = SM_SPEED_MAX_Q;
    } else {
        s = SM_SPEED_BASE_Q +
            (uint32_t)(((uint64_t)(SM_SPEED_MAX_Q - SM_SPEED_BASE_Q) * w->run_ms) /
                       SM_RAMP_MS);
    }
    if (w->slow_left_ms > 0) s = s * SM_SLOW_PCT / 100u;
    return s;
}

// 猴子(中心 x)是否悬在缺口上。
static bool over_gap(const sm_world_t *w) {
    const int32_t cx = SM_MONKEY_X + SM_MONKEY_W / 2;
    for (uint8_t i = 0; i < w->obs_count; i++) {
        const sm_obst_t *ob = &w->obs[i];
        if (ob->kind == SM_OBST_GAP && ob->x < cx && ob->x + ob->w > cx) return true;
    }
    return false;
}

static void sm_spawn(sm_world_t *w, uint32_t spd) {
    sm_obst_t *ob = &w->obs[w->obs_count];
    memset(ob, 0, sizeof(*ob));
    const uint32_t r = lcg_next(&w->rng) % 100u;
    if (r < 40u) {                       // 权重 石 0.40
        ob->kind = SM_OBST_STONE;
        ob->w = SM_STONE_W;
        ob->h = SM_STONE_H;
    } else if (r < 75u) {                // 权重 仙人掌 0.35
        ob->kind = SM_OBST_CACTUS;
        ob->w = SM_CACTUS_W;
        ob->h = SM_CACTUS_H;
    } else {                             // 权重 悬崖 0.25
        ob->kind = SM_OBST_GAP;
        ob->w = SM_GAP_MIN_PX +
                (int32_t)(lcg_next(&w->rng) % (SM_GAP_MAX_PX - SM_GAP_MIN_PX + 1u));
        ob->h = 0;
    }
    ob->x = SM_LCD_W + 8;
    w->obs_count++;

    // 生成节奏:1.1~2.0s 的滚动距离,且不小于最小反应间距。
    const uint32_t interval_ms = 1100u + lcg_next(&w->rng) % 900u;
    uint32_t px = (uint32_t)(((uint64_t)spd * interval_ms) / 1000u >> SM_Q);
    if (px < SM_MIN_SPACING_PX) px = SM_MIN_SPACING_PX;
    w->spawn_left_px = (int32_t)px;
}

uint32_t sm_world_tick(sm_world_t *w, uint32_t dt_ms) {
    w->events = SM_EVT_NONE;
    if (dt_ms == 0) return 0;

    if (w->phase == SM_PHASE_CALIB) return 0;

    if (w->phase == SM_PHASE_READY) {
        w->ready_left_ms = (w->ready_left_ms > dt_ms) ? (w->ready_left_ms - dt_ms) : 0;
        if (w->ready_left_ms == 0) {
            w->phase = SM_PHASE_RUN;
            w->run_ms = 0;
            w->spawn_left_px = (int32_t)
                (((uint64_t)speed_q(w) * SM_WARMUP_MS) / 1000u >> SM_Q);
        }
        return w->events;
    }

    if (w->phase != SM_PHASE_RUN) return w->events;

    w->run_ms += dt_ms;
    if (w->invuln_ms > dt_ms) w->invuln_ms -= dt_ms; else w->invuln_ms = 0;
    if (w->slow_left_ms > dt_ms) w->slow_left_ms -= dt_ms; else w->slow_left_ms = 0;

    const uint32_t spd = speed_q(w);
    const int32_t dx = (int32_t)(((uint64_t)spd * dt_ms) / 1000u >> SM_Q);
    w->dist_px += (uint32_t)dx;
    w->spawn_left_px -= dx;

    // 障碍滚动 + 出屏回收(交换删除)。
    for (uint8_t i = 0; i < w->obs_count; ) {
        w->obs[i].x -= dx;
        if (w->obs[i].x + w->obs[i].w < -4) {
            w->obs[i] = w->obs[w->obs_count - 1u];
            w->obs_count--;
        } else {
            i++;
        }
    }

    if (w->spawn_left_px <= 0 && w->obs_count < SM_OBST_MAX) sm_spawn(w, spd);

    // 纵向物理。
    if (w->airborne) {
        w->vy_q += (int32_t)(((int64_t)SM_GRAVITY_Q * (int64_t)dt_ms) / 1000);
        if (w->vy_q > SM_V_FALL_MAX_Q) w->vy_q = SM_V_FALL_MAX_Q;
        w->y_q += (int32_t)(((int64_t)w->vy_q * (int64_t)dt_ms) / 1000);
    }

    // 地面交互:缺口中心判定(着地悬空在缺口上→开始坠落;坠崖超过限深→终局)。
    const int32_t ground_q = (int32_t)SM_GROUND_Y << SM_Q;
    const bool gap_below = over_gap(w);
    if (!w->airborne) {
        if (gap_below) {
            w->airborne = true;
            w->falling = true;
            w->vy_q = 0;
            w->events |= SM_EVT_FALL_START;
        }
    } else if (w->y_q >= ground_q && w->vy_q > 0) {
        if (gap_below) {
            w->falling = true;      // 缺口上方不落地,继续下坠
        } else if (!w->falling) {
            w->y_q = ground_q;
            w->vy_q = 0;
            w->airborne = false;
            w->events |= SM_EVT_LAND;
        }
    }
    if (w->falling && w->y_q > ground_q + (SM_FALL_LIMIT_PX << SM_Q)) {
        w->phase = SM_PHASE_OVER;
        w->death_cause = SM_DEATH_FALL;
        w->events |= SM_EVT_DEATH;
        return w->events;
    }

    // 石/仙人掌 AABB 碰撞(坠落中不再判碰撞)。
    if (!w->falling) {
        int32_t bx0, by0, bx1, by1;
        sm_world_monkey_box(w, &bx0, &by0, &bx1, &by1);
        for (uint8_t i = 0; i < w->obs_count; i++) {
            sm_obst_t *ob = &w->obs[i];
            if (ob->kind != SM_OBST_STONE && ob->kind != SM_OBST_CACTUS) continue;
            const int32_t oy0 = SM_GROUND_Y - ob->h;
            if (bx1 > ob->x && bx0 < ob->x + ob->w && by1 > oy0 && by0 < SM_GROUND_Y) {
                if (!ob->hit) {
                    ob->hit = true;
                    if (w->invuln_ms == 0) {
                        w->hp -= 1;
                        w->invuln_ms = SM_INVULN_MS;
                        if (ob->kind == SM_OBST_STONE) w->slow_left_ms = SM_SLOW_MS;
                        w->events |= SM_EVT_HURT;
                        if (w->hp == 0) {
                            w->phase = SM_PHASE_OVER;
                            w->death_cause = SM_DEATH_HP;
                            w->events |= SM_EVT_DEATH;
                            return w->events;
                        }
                    }
                }
            }
        }
    }

    return w->events;
}
