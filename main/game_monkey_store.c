// main/game_monkey_store.c —— NVS 序列化实现。
// 布局:namespace "sm_game";键 "top5" = SM_BOARD_N*8 字节小端 blob,
//       键 "top5_n" = u8 有效条数。每条:4B 里程 + 2B 年 + 1B 月 + 1B 日。
#include "game_monkey_store.h"

#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

esp_err_t sm_store_init(void) {
    return nvs_flash_init();
}

static void pack_entry(uint8_t *p, const sm_score_t *e) {
    p[0] = (uint8_t)(e->meters & 0xff);
    p[1] = (uint8_t)((e->meters >> 8) & 0xff);
    p[2] = (uint8_t)((e->meters >> 16) & 0xff);
    p[3] = (uint8_t)((e->meters >> 24) & 0xff);
    p[4] = (uint8_t)(e->year & 0xff);
    p[5] = (uint8_t)((e->year >> 8) & 0xff);
    p[6] = e->month;
    p[7] = e->day;
}

static void unpack_entry(const uint8_t *p, sm_score_t *e) {
    e->meters = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    e->year = (uint16_t)((uint16_t)p[4] | ((uint16_t)p[5] << 8));
    e->month = p[6];
    e->day = p[7];
}

esp_err_t sm_store_load(sm_score_t board[SM_BOARD_N], uint8_t *count) {
    if (board == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    *count = 0;
    memset(board, 0, sizeof(sm_score_t) * SM_BOARD_N);

    nvs_handle_t h;
    esp_err_t err = nvs_open(SM_GAME_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    uint8_t blob[SM_BOARD_N * 8];
    size_t len = sizeof(blob);
    err = nvs_get_blob(h, SM_KEY_TOP5, blob, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(h);
        return ESP_OK;   // 首次使用:空榜
    }
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    uint8_t n = 0;
    err = nvs_get_u8(h, SM_KEY_COUNT, &n);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        n = 0;           // 兼容:缺计数键视为空榜
    } else if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }
    if (n > SM_BOARD_N || len < (size_t)n * 8u) n = 0;   // 数据异常按空榜

    for (uint8_t i = 0; i < n; i++) unpack_entry(&blob[i * 8u], &board[i]);
    *count = n;
    nvs_close(h);
    return ESP_OK;
}

esp_err_t sm_store_save(const sm_score_t *board, uint8_t count) {
    if (board == NULL || count > SM_BOARD_N) return ESP_ERR_INVALID_ARG;

    uint8_t blob[SM_BOARD_N * 8];
    memset(blob, 0, sizeof(blob));
    for (uint8_t i = 0; i < count; i++) pack_entry(&blob[i * 8u], &board[i]);

    nvs_handle_t h;
    esp_err_t err = nvs_open(SM_GAME_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(h, SM_KEY_TOP5, blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_set_u8(h, SM_KEY_COUNT, count);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}
