// main/game_monkey_sprites.h —— 《尖叫猴子》精灵描述符声明。
// 实体由 tools/gen_sprites.mjs 生成在 assets/images/sm_sprites.c(勿手改)。
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 猴子 24x24 六帧:跑两帧(10fps 交替)/跳升/跳降/受伤/死亡。
extern const lv_image_dsc_t sm_img_monkey_run1;
extern const lv_image_dsc_t sm_img_monkey_run2;
extern const lv_image_dsc_t sm_img_monkey_jump_up;
extern const lv_image_dsc_t sm_img_monkey_jump_down;
extern const lv_image_dsc_t sm_img_monkey_hurt;
extern const lv_image_dsc_t sm_img_monkey_death;

// 障碍与配件。
extern const lv_image_dsc_t sm_img_stone;    // 16x16
extern const lv_image_dsc_t sm_img_cactus;   // 12x24
extern const lv_image_dsc_t sm_img_heart;    // 10x10 血量
extern const lv_image_dsc_t sm_img_tree;     // 16x16 中景树
extern const lv_image_dsc_t sm_img_cloud;    // 16x6 云

#ifdef __cplusplus
}
#endif
