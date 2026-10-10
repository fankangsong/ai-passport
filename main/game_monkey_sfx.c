// main/game_monkey_sfx.c —— 芯片音效合成实现(方波/三角/噪声 + 全局包络)。
#include "game_monkey_sfx.h"

#include <string.h>

#define W_SQ  0u
#define W_TRI 1u
#define W_NS  2u

typedef struct {
    uint8_t wave;
    uint16_t f0, f1;   // Hz(线性扫频)
    uint16_t ms;
} sm_seg_t;

// 每条音效最多 4 段;ms==0 为结束标记。
static const sm_seg_t k_fx[SM_SFX_COUNT][4] = {
    [SM_SFX_JUMP]   = { { W_SQ, 300, 700, 90 } },
    [SM_SFX_LAND]   = { { W_TRI, 220, 220, 60 } },
    [SM_SFX_HURT]   = { { W_NS, 0, 0, 60 }, { W_SQ, 200, 120, 140 } },
    [SM_SFX_DEATH]  = { { W_SQ, 392, 392, 130 },
                        { W_SQ, 311, 311, 130 },
                        { W_SQ, 262, 262, 130 } },
    [SM_SFX_RECORD] = { { W_TRI, 523, 523, 90 },
                        { W_TRI, 659, 659, 90 },
                        { W_TRI, 784, 784, 90 },
                        { W_TRI, 1047, 1047, 90 } },
};

size_t sm_sfx_render(sm_sfx_t fx, int16_t *buf, size_t max_samples, uint32_t rate_hz) {
    if ((uint32_t)fx >= (uint32_t)SM_SFX_COUNT || buf == NULL || rate_hz == 0) return 0;

    // 先算总时长,容量不足直接拒绝(调用方缓冲按 SM_SFX_MAX_SAMPLES 配)。
    size_t need = 0;
    for (size_t s = 0; s < 4; s++) {
        if (k_fx[fx][s].ms == 0) break;
        need += (size_t)(((uint64_t)k_fx[fx][s].ms * rate_hz) / 1000u);
    }
    if (need == 0 || need > max_samples) return 0;

    const size_t fade = (SM_SFX_FADE < need / 2u) ? (size_t)SM_SFX_FADE : need / 2u;
    uint32_t phase = 0;
    uint32_t rng = 0x1234567u;
    size_t produced = 0;

    for (size_t s = 0; s < 4; s++) {
        const sm_seg_t *seg = &k_fx[fx][s];
        if (seg->ms == 0) break;
        const size_t count = (size_t)(((uint64_t)seg->ms * rate_hz) / 1000u);
        for (size_t j = 0; j < count; j++) {
            const uint16_t f = (uint16_t)(seg->f0 +
                ((int32_t)(seg->f1 - seg->f0) * (int32_t)j) / (int32_t)count);
            phase += (uint32_t)(((uint64_t)f << 32) / rate_hz);

            int32_t v;
            if (seg->wave == W_SQ) {
                v = (phase & 0x80000000u) ? SM_SFX_AMP : -SM_SFX_AMP;
            } else if (seg->wave == W_TRI) {
                if (phase < 0x80000000u) {
                    v = -SM_SFX_AMP + (int32_t)(((int64_t)phase * 2 * SM_SFX_AMP) >> 31);
                } else {
                    v = SM_SFX_AMP -
                        (int32_t)((((int64_t)phase - 0x80000000u) * 2 * SM_SFX_AMP) >> 31);
                }
            } else {
                rng = rng * 1664525u + 1013904223u;
                v = ((int32_t)(rng >> 16) - 32768) * SM_SFX_AMP / 32768;
            }

            // 全局包络:整条音效首尾 5ms 线性淡入淡出。
            int32_t env = 256;
            if (produced < fade) env = (int32_t)(produced * 256u / fade);
            const size_t tail = need - produced;
            if (tail < fade && env > (int32_t)(tail * 256u / fade)) {
                env = (int32_t)(tail * 256u / fade);
            }
            v = v * env / 256;
            buf[produced++] = (int16_t)v;
        }
    }
    return produced;
}
