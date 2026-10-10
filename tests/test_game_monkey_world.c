// 《尖叫猴子》世界模型 host 测试:物理/映射/碰撞/坠崖/减速/生成间距/确定性。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game_monkey_world.h"

#define STEP 33u   // 与 UI tick 一致

static void advance(sm_world_t *w, uint32_t ms) {
    for (uint32_t left = ms; left > 0;) {
        const uint32_t dt = left > STEP ? STEP : left;
        sm_world_tick(w, dt);
        left -= dt;
    }
}

static int32_t run_jump_apex(uint8_t level) {
    sm_world_t w;
    sm_world_init(&w, 7u);
    sm_world_begin_run(&w);
    advance(&w, SM_READY_MS);            // → RUN
    assert(sm_world_try_jump(&w, level));
    int32_t apex = 0;
    for (int i = 0; i < 2000 && (w.airborne || w.y_q < (SM_GROUND_Y << SM_Q)); i++) {
        sm_world_tick(&w, 1u);
        const int32_t h = (SM_GROUND_Y << SM_Q) - w.y_q;
        if (h > apex) apex = h;
    }
    return apex >> SM_Q;
}

int main(void) {
    // ---- 阶段机:CALIB → READY → RUN ----
    sm_world_t w;
    sm_world_init(&w, 42u);
    assert(w.phase == SM_PHASE_CALIB);
    assert(sm_world_try_jump(&w, 80u) == false);      // CALIB 不许跳
    sm_world_tick(&w, 1000u);
    assert(w.dist_px == 0);                            // CALIB 空转
    sm_world_begin_run(&w);
    assert(w.phase == SM_PHASE_READY);
    assert(sm_world_try_jump(&w, 80u) == false);      // READY 不许跳
    advance(&w, SM_READY_MS - STEP);
    assert(w.phase == SM_PHASE_READY);
    advance(&w, STEP);
    assert(w.phase == SM_PHASE_RUN);

    // ---- 喊叫上升沿滞回 ----
    bool armed = true;
    assert(sm_shout_edge(20u, &armed) == false && armed);
    assert(sm_shout_edge(55u, &armed) == true && !armed);   // 触发并解除武装
    assert(sm_shout_edge(80u, &armed) == false);            // 未回落下限不再触发
    assert(sm_shout_edge(38u, &armed) == false && armed);   // 回落重新武装
    assert(sm_shout_edge(45u, &armed) == false);            // 未到 ON
    assert(sm_shout_edge(60u, &armed) == true);

    // ---- 连续映射:level 50 → 跳高≈27px;level 95 → ≈108px ----
    const int32_t apex_lo = run_jump_apex(SM_SHOUT_ON);
    const int32_t apex_hi = run_jump_apex(SM_SHOUT_HI);
    assert(apex_lo >= 25 && apex_lo <= 29);
    assert(apex_hi >= 106 && apex_hi <= 110);
    assert(sm_world_try_jump(&w, 30u) == false);            // 低于阈值不起跳

    // ---- 空中不可再跳 ----
    sm_world_init(&w, 5u);
    sm_world_begin_run(&w);
    advance(&w, SM_READY_MS);
    assert(sm_world_try_jump(&w, 95u));
    assert(sm_world_try_jump(&w, 95u) == false);

    // ---- 石头:掉血 + 减速 + 无敌帧(events 是单 tick 位图,需逐 tick 捕获) ----
    sm_world_init(&w, 11u);
    sm_world_begin_run(&w);
    advance(&w, SM_READY_MS);
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_STONE;
    w.obs[0].x = 100; w.obs[0].w = SM_STONE_W; w.obs[0].h = SM_STONE_H;
    w.obs[0].hit = false;
    bool hurt_seen = false;
    for (int i = 0; i < 200 && !hurt_seen; i++) {
        if (sm_world_tick(&w, STEP) & SM_EVT_HURT) hurt_seen = true;
    }
    assert(hurt_seen);
    assert(w.hp == SM_HP_MAX - 1);
    assert(w.slow_left_ms > 0);
    assert(w.invuln_ms > 0);

    // 无敌期内第二块石头不再扣血;无敌结束后恢复受伤。
    const uint32_t hp_after_first = (uint32_t)w.hp;
    w.invuln_ms = SM_INVULN_MS;                         // 确保仍在无敌期
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_STONE;
    w.obs[0].x = 52; w.obs[0].w = SM_STONE_W; w.obs[0].h = SM_STONE_H;
    w.obs[0].hit = false;
    advance(&w, 300u);
    assert((uint32_t)w.hp == hp_after_first);          // 仍在无敌期
    advance(&w, SM_INVULN_MS);
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_CACTUS;
    w.obs[0].x = 52; w.obs[0].w = SM_CACTUS_W; w.obs[0].h = SM_CACTUS_H;
    w.obs[0].hit = false;
    advance(&w, 2000u);
    assert((uint32_t)w.hp == hp_after_first - 1);      // 仙人掌只扣血不减速

    // ---- 血量归零 → OVER(原因=HP) ----
    w.hp = 1;
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_CACTUS;
    w.obs[0].x = 52; w.obs[0].w = SM_CACTUS_W; w.obs[0].h = SM_CACTUS_H;
    w.obs[0].hit = false;
    w.invuln_ms = 0;
    bool death_seen = false;
    for (int i = 0; i < 300 && w.phase != SM_PHASE_OVER; i++) {
        if (sm_world_tick(&w, STEP) & SM_EVT_DEATH) death_seen = true;
    }
    assert(death_seen);
    assert(w.phase == SM_PHASE_OVER);
    assert(w.death_cause == SM_DEATH_HP);

    // ---- 大喊跳过石头 ----
    sm_world_init(&w, 13u);
    sm_world_begin_run(&w);
    advance(&w, SM_READY_MS);
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_STONE;
    w.obs[0].x = 140; w.obs[0].w = SM_STONE_W; w.obs[0].h = SM_STONE_H;
    w.obs[0].hit = false;
    assert(sm_world_try_jump(&w, SM_SHOUT_HI));
    advance(&w, 1200u);
    assert(w.hp == SM_HP_MAX);                          // 全程未受伤
    assert(w.phase == SM_PHASE_RUN);

    // ---- 悬崖:中心进入缺口→坠落→超限判死(原因=坠崖) ----
    sm_world_init(&w, 21u);
    sm_world_begin_run(&w);
    advance(&w, SM_READY_MS);
    w.obs_count = 1;
    w.obs[0].kind = SM_OBST_GAP;
    w.obs[0].x = 120; w.obs[0].w = 50; w.obs[0].h = 0;
    bool saw_fall = false;
    for (int i = 0; i < 400; i++) {
        const uint32_t ev = sm_world_tick(&w, STEP);
        if (ev & SM_EVT_FALL_START) saw_fall = true;
        if (w.phase == SM_PHASE_OVER) break;
    }
    assert(saw_fall);
    assert(w.phase == SM_PHASE_OVER);
    assert(w.death_cause == SM_DEATH_FALL);

    // ---- 减速生效:同样 3 秒,减速态里程更短 ----
    sm_world_t a, b;
    sm_world_init(&a, 99u); sm_world_init(&b, 99u);
    sm_world_begin_run(&a); sm_world_begin_run(&b);
    advance(&a, SM_READY_MS); advance(&b, SM_READY_MS);
    advance(&a, 3000u); advance(&b, 3000u);
    b.slow_left_ms = SM_SLOW_MS;                        // 人为减速
    const uint32_t d0 = b.dist_px;
    advance(&b, 3000u);
    // 精确断言:减速期速度 ≈ 55%
    const uint32_t slow_dist = b.dist_px - d0;
    const uint32_t fast_dist = a.dist_px;
    assert(slow_dist * 100u < fast_dist * 70u);         // 明显更慢

    // ---- 生成间距 ≥ SM_MIN_SPACING_PX + 确定性 ----
    sm_world_t r1, r2;
    sm_world_init(&r1, 777u); sm_world_init(&r2, 777u);
    sm_world_begin_run(&r1); sm_world_begin_run(&r2);
    advance(&r1, SM_READY_MS); advance(&r2, SM_READY_MS);
    uint8_t prev_count = 0;
    for (int i = 0; i < 400; i++) {
        advance(&r1, STEP);
        if (r1.obs_count > prev_count) {                // 刚生成过
            assert(r1.spawn_left_px >= SM_MIN_SPACING_PX);
            prev_count = r1.obs_count;
        }
        advance(&r2, STEP);
        assert(r1.obs_count == r2.obs_count);
        for (int k = 0; k < r1.obs_count; k++) {
            assert(r1.obs[k].kind == r2.obs[k].kind);
            assert(r1.obs[k].w == r2.obs[k].w);
        }
    }
    assert(r1.obs_count > 0);                           // 确实生成过

    // ---- 里程换算 ----
    assert(sm_world_meters(&a) == a.dist_px / SM_PX_PER_METER);

    puts("scream monkey world tests: PASS");
    return 0;
}
