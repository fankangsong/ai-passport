idf_component_register(
    SRCS "main.c"
         "game_monkey_world.c"
         "game_monkey_detector.c"
         "game_monkey_sfx.c"
         "game_monkey_board.c"
         "game_monkey_store.c"
         "game_monkey_ui.c"
         "game_monkey_app.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_hw_support esp_timer nvs_flash
)

# 《尖叫猴子》素材:中文字库(lv_font_conv 子集,生成命令见 assets/README.md)
# 与精灵(lv_image RGB565A8,由 tools/gen_sprites.mjs 从 assets/art 矩阵源展开)。
target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/sm_font_16.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/sm_font_24.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/images/sm_sprites.c"
)
