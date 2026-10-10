// main/game_monkey_sfx.h —— 芯片音效运行时合成(纯C,确定性)。
// 不引入任何音频素材文件;渲染结果为 16bit 单声道 PCM。
#pragma once

#include "game_monkey_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SM_SFX_JUMP = 0,     // 方波 300→700Hz 上滑 90ms
    SM_SFX_LAND,         // 三角 220Hz 60ms
    SM_SFX_HURT,         // 噪声前缀 60ms + 方波 200→120Hz 140ms
    SM_SFX_DEATH,        // 方波 392/311/262Hz 下行三连 各130ms
    SM_SFX_RECORD,       // 三角 523/659/784/1047Hz 上行琶音 各90ms
    SM_SFX_COUNT,
} sm_sfx_t;

// 渲染一条音效到 buf(16bit 单声道)。返回写入的采样数;
// 缓冲不足(max_samples < 所需)时返回 0,不写任何数据。
// 同参数重复调用输出逐字节一致(确定性)。
size_t sm_sfx_render(sm_sfx_t fx, int16_t *buf, size_t max_samples, uint32_t rate_hz);

#ifdef __cplusplus
}
#endif
