// main/game_monkey_text.h —— 《尖叫猴子》全部上屏文案的唯一来源。
// 字体子集(tools/gen_fonts.mjs 提取码点)、设备端字形覆盖自检
// (game_monkey_ui.c 的 sm_ui_font_covers)与宿主覆盖测试
// (tests/test_font_coverage.py)都以本文件为准:改文案必须过此文件。
#pragma once

// 全量上屏文案。%u/%d/%% 为 ASCII 格式符,不参与 CJK 子集;实际显示时由
// lv_label_set_text_fmt 用同款格式串渲染,保证"文案即所测"。
static const char * const k_sm_text[] = {
    "尖叫猴子", "按OK开始", "排行榜", "最高", "里程 %u 米", "米",
    "请保持安静", "校准中", "预备", "喊!", "游戏结束", "被砸晕了", "掉下去了",
    "新纪录!", "再来一局", "返回标题", "第 %u 名", "恭喜", "电量 %d%%",
    "分贝", "晕", "--",
};

#define SM_TEXT_COUNT (sizeof(k_sm_text) / sizeof(k_sm_text[0]))

// 24px 标题字子集只覆盖这里出现过的字。
#define SM_TITLE_TEXT "尖叫猴子"
