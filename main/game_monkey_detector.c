// main/game_monkey_detector.c —— 喊叫检测实现(纯C,确定性,可宿主测试)。
#include "game_monkey_detector.h"

#include <string.h>

static uint32_t isqrt32(uint32_t v) {
    uint32_t res = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit != 0) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

void sm_detector_init(sm_detector_t *d) {
    memset(d, 0, sizeof(*d));
    d->calib_left = SM_CALIB_CHUNKS;
}

void sm_detector_reset_session(sm_detector_t *d) {
    d->level_q8 = 0;
    d->floor_rms = 0;
    d->calib_left = SM_CALIB_CHUNKS;
    d->primed = false;
}

// 供 push/hold 共用:更新直流并返回该块 RMS。
static uint32_t feed(sm_detector_t *d, const int16_t *pcm, size_t n) {
    int64_t sum = 0;
    int32_t dc = d->dc_q8;
    for (size_t i = 0; i < n; i++) {
        dc += (((int32_t)pcm[i]) * 256 - dc) >> SM_DC_SHIFT;   // 乘 256 而非 <<8:避免负数左移 UB
        const int32_t c = (int32_t)pcm[i] - (dc >> 8);
        sum += (int64_t)c * c;
    }
    d->dc_q8 = dc;
    return (n > 0) ? isqrt32((uint32_t)(sum / (int64_t)n)) : 0;
}

static void floor_follow_down(sm_detector_t *d, uint32_t rms) {
    if ((uint32_t)d->floor_rms > rms) {
        d->floor_rms -= (int32_t)((uint32_t)(d->floor_rms - (int32_t)rms) >>
                                  SM_FLOOR_DOWN_SHIFT);
        if (d->floor_rms < 1) d->floor_rms = 1;
    }
}

uint8_t sm_detector_push(sm_detector_t *d, const int16_t *pcm, size_t n) {
    const uint32_t rms = feed(d, pcm, n);

    if (d->calib_left > 0) {
        // 校准期:地板取各块 RMS 的最大值(环境噪声上包络)。
        if ((int32_t)rms > d->floor_rms) d->floor_rms = (int32_t)rms;
        d->calib_left--;
        if (d->calib_left == 0) {
            if (d->floor_rms < 1) d->floor_rms = 1;
            d->primed = true;
        }
        return (uint8_t)(d->level_q8 >> 8);
    }

    // 地板跟随:快速下探、极慢上爬(每块 +1)。
    if ((uint32_t)d->floor_rms > rms) {
        floor_follow_down(d, rms);
    } else {
        d->floor_rms += SM_FLOOR_UP_STEP;
    }

    int32_t excess = (int32_t)rms - d->floor_rms;
    if (excess < 0) excess = 0;
    int32_t target = excess * 100 / SM_LEVEL_SPAN;
    if (target > 100) target = 100;

    const int32_t tq = target << 8;
    if (tq > d->level_q8) {
        d->level_q8 += (int32_t)(((int64_t)(tq - d->level_q8) * SM_EMA_ATTACK_Q8) >> 8);
    } else {
        d->level_q8 -= (int32_t)(((int64_t)(d->level_q8 - tq) * SM_EMA_RELEASE_Q8) >> 8);
    }
    return (uint8_t)(d->level_q8 >> 8);
}

void sm_detector_hold(sm_detector_t *d, const int16_t *pcm, size_t n) {
    const uint32_t rms = feed(d, pcm, n);
    // SFX 期间:只让地板缓慢下探,绝不上爬 —— 扬声器残响不得抬高噪声基准。
    floor_follow_down(d, rms);
}
