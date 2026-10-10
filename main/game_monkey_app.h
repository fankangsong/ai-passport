// main/game_monkey_app.h —— 《尖叫猴子》应用层:两屏生命周期 + 音频任务 +
// 世界 tick。main.c 只调用 sm_app_enter() 与 sm_app_key()。
#pragma once

#include "bsp_button.h"

#ifdef __cplusplus
extern "C" {
#endif

// 构建标题屏并启动音频 worker(幂等)。调用方必须已持 LVGL 锁。
void sm_app_enter(void);

// 按键分发(输入任务上下文调用;内部自行持 LVGL 锁)。
void sm_app_key(bsp_btn_t btn, bsp_btn_ev_t ev);

#ifdef __cplusplus
}
#endif
