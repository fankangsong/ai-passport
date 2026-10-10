// main/game_monkey_detector.h —— 麦克风流式喊叫检测(纯C)。
// 管线:去直流 → 分块 RMS → 噪声地板跟随 → 线性映射 0..100 → 非对称 EMA。
#pragma once

#include "game_monkey_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t dc_q8;       // 直流估计(Q8)
    int32_t floor_rms;   // 噪声地板(RMS 域)
    int32_t level_q8;    // 平滑输出 0..100(Q8)
    uint8_t calib_left;  // 剩余校准块数
    bool primed;         // 校准完成
} sm_detector_t;

// 复位并进入校准(采 SM_CALIB_CHUNKS 块环境噪声定地板)。
void sm_detector_init(sm_detector_t *d);

// 重新校准(保留 DC 估计,清 level/地板,回到 CALIB)。
void sm_detector_reset_session(sm_detector_t *d);

// 喂一块 PCM,返回平滑 level(0..100)。校准期返回 0。
uint8_t sm_detector_push(sm_detector_t *d, const int16_t *pcm, size_t n);

// SFX 播放期间喂入:只跟踪直流与地板下探,不更新 level(防扬声器回授)。
void sm_detector_hold(sm_detector_t *d, const int16_t *pcm, size_t n);

#ifdef __cplusplus
}
#endif
