// main/game_monkey_board.c —— Top5 插入实现(纯C)。
#include "game_monkey_board.h"

uint8_t sm_board_insert(sm_score_t board[SM_BOARD_N], uint8_t *count,
                        sm_score_t entry, uint8_t *rank_out) {
    if (rank_out != NULL) *rank_out = 0;
    if (board == NULL || count == NULL) return 0;

    uint8_t n = *count;
    if (n > SM_BOARD_N) n = SM_BOARD_N;

    // 找插入位:第一个比 entry 小的槽(并列者保持在前)。
    uint8_t pos = n;
    for (uint8_t i = 0; i < n; i++) {
        if (entry.meters > board[i].meters) {
            pos = i;
            break;
        }
    }
    if (pos >= SM_BOARD_N) return 0;              // 满榜且不比垫底好
    if (pos == n && n == SM_BOARD_N) return 0;

    if (rank_out != NULL) *rank_out = (uint8_t)(pos + 1);

    // 后移腾位:满榜时挤掉垫底,未满时直接扩一位。
    const uint8_t last = (n < SM_BOARD_N) ? n : (uint8_t)(SM_BOARD_N - 1u);
    for (uint8_t i = last; i > pos; i--) board[i] = board[i - 1u];
    board[pos] = entry;

    *count = (n < SM_BOARD_N) ? (uint8_t)(n + 1u) : n;
    return (uint8_t)(pos + 1);
}
