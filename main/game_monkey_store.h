// main/game_monkey_store.h —— NVS 榜单读写薄层(固件专用,依赖 nvs_flash)。
// 逻辑部分(Top5 插入)在 game_monkey_board.c,本层只做序列化与持久化。
#pragma once

#include "game_monkey_board.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 NVS。按仓库保守原则:分区校验失败不自动擦除,显式返回错误由上层报告。
esp_err_t sm_store_init(void);

// 读取榜单。首次使用(键不存在)返回 ESP_OK 且 *count=0;
// 数据异常(长度不符)同样按空榜处理,不让坏数据卡死游戏。
esp_err_t sm_store_load(sm_score_t board[SM_BOARD_N], uint8_t *count);

// 保存榜单(整块覆盖写 + commit)。仅在结算时调用,不在 tick 热路径。
esp_err_t sm_store_save(const sm_score_t *board, uint8_t count);

#ifdef __cplusplus
}
#endif
