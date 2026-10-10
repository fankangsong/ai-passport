// 《尖叫猴子》Top5 榜单 host 测试。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game_monkey_board.h"

static sm_score_t mk(uint32_t m) {
    sm_score_t e = { .meters = m, .year = 2026, .month = 10, .day = 10 };
    return e;
}

int main(void) {
    sm_score_t b[SM_BOARD_N];
    uint8_t n = 0;
    uint8_t rank = 0;

    // 空榜插入 → 第 1 名
    assert(sm_board_insert(b, &n, mk(100), &rank) == 1);
    assert(n == 1 && rank == 1 && b[0].meters == 100);

    // 递增插入保持降序
    assert(sm_board_insert(b, &n, mk(300), &rank) == 1);
    assert(sm_board_insert(b, &n, mk(200), &rank) == 2);
    assert(n == 3);
    assert(b[0].meters == 300 && b[1].meters == 200 && b[2].meters == 100);

    // 插入中间位置
    assert(sm_board_insert(b, &n, mk(250), &rank) == 2);
    assert(b[1].meters == 250 && b[2].meters == 200);

    // 并列里程:新纪录排在并列者之后
    assert(sm_board_insert(b, &n, mk(300), &rank) == 2);
    assert(b[0].meters == 300 && b[1].meters == 300 && b[2].meters == 250);

    // 填满 5 条后再插入更优值 → 挤掉垫底
    assert(sm_board_insert(b, &n, mk(400), &rank) == 1);
    assert(n == SM_BOARD_N);

    // 满榜且比垫底差 → 不上榜
    assert(sm_board_insert(b, &n, mk(50), &rank) == 0);
    assert(n == SM_BOARD_N && rank == 0);
    assert(b[4].meters == 200);

    // 满榜挤掉垫底
    assert(sm_board_insert(b, &n, mk(220), &rank) == 5);
    assert(n == SM_BOARD_N && b[4].meters == 220 && rank == 5);

    // 参数非法
    assert(sm_board_insert(NULL, &n, mk(1), &rank) == 0);

    puts("scream monkey board tests: PASS");
    return 0;
}
