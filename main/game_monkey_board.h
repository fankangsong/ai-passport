// main/game_monkey_board.h —— Top5 榜单插入逻辑(纯C,可宿主测试)。
#pragma once

#include "game_monkey_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 8 字节(自然对齐无填充):4B 里程 + 2B 年 + 1B 月 + 1B 日。
// 年月日全 0 表示"未知日期"(设备无 RTC/未对时),UI 显示 "--"。
typedef struct {
    uint32_t meters;
    uint16_t year;
    uint8_t month, day;
} sm_score_t;

// 尝试把 entry 插入 board(按里程降序,最多 SM_BOARD_N 条)。
// 返回名次 1..5;未上榜返回 0(满榜且不比垫底好 / 参数非法)。
// 并列里程时新纪录排在并列者之后(不挤掉同分旧纪录)。
// count 为当前有效条数,成功插入后更新。
uint8_t sm_board_insert(sm_score_t board[SM_BOARD_N], uint8_t *count,
                        sm_score_t entry, uint8_t *rank_out);

#ifdef __cplusplus
}
#endif
