#!/usr/bin/env node
// tools/gen_fonts.mjs —— 从 main/game_monkey_text.h 提取上屏汉字码点,
// 输出 lv_font_conv 的 --range 参数,保证"文案↔字体子集"不漂移。
// 用法: node tools/gen_fonts.mjs 16   → 全部文案码点(含 ASCII 0x20-0x7E)
//       node tools/gen_fonts.mjs 24   → 仅 SM_TITLE_TEXT 的码点
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const raw = readFileSync(join(root, 'main', 'game_monkey_text.h'), 'utf8');
// 剥离块注释与行注释,只统计字符串字面量里的字,避免把注释文案编进字库。
const header = raw.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
const size = process.argv[2] || '16';

const hanzi = [...new Set(header.match(/[\u3400-\u9fff]/g) || [])].sort();
if (hanzi.length === 0) throw new Error('no CJK codepoint found in game_monkey_text.h');

let text;
if (size === '24') {
    const m = /SM_TITLE_TEXT\s+"([^"]+)"/.exec(header);
    if (!m) throw new Error('SM_TITLE_TEXT not found');
    text = m[1];
} else {
    text = hanzi.join('');
}

const cps = [...new Set(text)]
    .map((c) => '0x' + c.charCodeAt(0).toString(16).toUpperCase());
if (size !== '24') cps.unshift('0x20-0x7E');
console.log(cps.join(','));
