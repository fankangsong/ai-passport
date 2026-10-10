// 《尖叫猴子》喊叫检测 host 测试:校准/映射单调性/EMA 不对称/地板跟随/hold 冻结。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game_monkey_detector.h"

static void fill_square(int16_t *buf, size_t n, int16_t amp, int *toggle) {
    for (size_t i = 0; i < n; i++) {
        *toggle = !*toggle;
        buf[i] = *toggle ? amp : (int16_t)-amp;
    }
}
static void fill_zero(int16_t *buf, size_t n) {
    memset(buf, 0, n * sizeof(int16_t));
}

int main(void) {
    int16_t chunk[SM_CHUNK_SAMPLES];
    int toggle = 0;

    // ---- 校准期:20 块静音 → primed 且 level=0 ----
    sm_detector_t d;
    sm_detector_init(&d);
    assert(d.calib_left == SM_CALIB_CHUNKS);
    for (int i = 0; i < SM_CALIB_CHUNKS; i++) {
        fill_zero(chunk, SM_CHUNK_SAMPLES);
        assert(sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES) == 0);
    }
    assert(d.primed);
    assert(d.calib_left == 0);
    assert(d.floor_rms >= 1);

    // ---- 静音 → level 0 ----
    fill_zero(chunk, SM_CHUNK_SAMPLES);
    assert(sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES) == 0);

    // ---- 大声(±6000 方波,rms≈6000)→ level 单调上升且映射正确 ----
    // excess ≈ 6000-floor,level_target ≈ excess*100/9000 ≈ 66
    uint8_t prev = 0;
    for (int i = 0; i < 40; i++) {
        fill_square(chunk, SM_CHUNK_SAMPLES, 6000, &toggle);
        const uint8_t lv = sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES);
        assert(lv >= prev);                    // 攻击期单调不降
        prev = lv;
    }
    assert(prev >= 60 && prev <= 70);          // (6000-小地板)*100/9000
    const uint8_t loud_level = prev;

    // ---- EMA 非对称:释放一步的降幅远小于攻击一步的增幅 ----
    fill_zero(chunk, SM_CHUNK_SAMPLES);
    const uint8_t after_quiet1 = sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES);
    assert(loud_level - after_quiet1 < 10);    // 释放慢

    // ---- hold:level 冻结,地板不上爬 ----
    const int32_t floor_before = d.floor_rms;
    const uint8_t level_before = d.level_q8 >> 8;
    for (int i = 0; i < 30; i++) {
        fill_square(chunk, SM_CHUNK_SAMPLES, 6000, &toggle);   // 模拟扬声器音效
        sm_detector_hold(&d, chunk, SM_CHUNK_SAMPLES);
    }
    assert((d.level_q8 >> 8) == level_before);          // level 不变
    assert(d.floor_rms <= floor_before + SM_FLOOR_UP_STEP);  // 地板几乎不动

    // ---- 释放与地板下探:长时间安静后 level 衰减、地板回落 ----
    for (int i = 0; i < 300; i++) {
        fill_zero(chunk, SM_CHUNK_SAMPLES);
        sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES);
    }
    assert((d.level_q8 >> 8) < 10);
    assert(d.floor_rms <= 60);                          // 已跟随到静音域

    // ---- 重新校准 ----
    sm_detector_reset_session(&d);
    assert(d.calib_left == SM_CALIB_CHUNKS && !d.primed);
    for (int i = 0; i < SM_CALIB_CHUNKS; i++) {
        fill_square(chunk, SM_CHUNK_SAMPLES, 300, &toggle);
        sm_detector_push(&d, chunk, SM_CHUNK_SAMPLES);
    }
    assert(d.primed);
    assert(d.floor_rms >= 250);                         // 地板=新环境噪声

    puts("scream monkey detector tests: PASS");
    return 0;
}
