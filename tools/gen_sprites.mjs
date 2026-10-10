#!/usr/bin/env node
// tools/gen_sprites.mjs —— 把 assets/art/sm_art_matrices.mjs 的字符调色板矩阵
// 展开为 LVGL9 RGB565A8 C 数组(assets/images/sm_sprites.c)。
// 用法: 在仓库根目录执行  node tools/gen_sprites.mjs
// 生成物入库;改美术只改矩阵源后重跑本脚本。
import { writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { SPRITES, PALETTES } from '../assets/art/sm_art_matrices.mjs';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');

const PAL = Object.assign(
    {}, PALETTES.monkey, PALETTES.stone, PALETTES.cactus,
    PALETTES.tree, PALETTES.cloud, PALETTES.heart);

function rgb565(hex) {
    const v = parseInt(hex.slice(1), 16);
    const r = (v >> 16) & 0xff, g = (v >> 8) & 0xff, b = v & 0xff;
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
const PAL565 = {};
for (const [k, hex] of Object.entries(PAL)) PAL565[k] = rgb565(hex);

const OUT_NAMES = {
    monkey_run1: 'sm_img_monkey_run1',
    monkey_run2: 'sm_img_monkey_run2',
    monkey_jump_up: 'sm_img_monkey_jump_up',
    monkey_jump_down: 'sm_img_monkey_jump_down',
    monkey_hurt: 'sm_img_monkey_hurt',
    monkey_death: 'sm_img_monkey_death',
    stone: 'sm_img_stone',
    cactus: 'sm_img_cactus',
    tree: 'sm_img_tree',
    cloud: 'sm_img_cloud',
    heart: 'sm_img_heart',
};

function expand(rows) {
    const h = rows.length, w = rows[0].length;
    for (let y = 0; y < h; y++) {
        if (rows[y].length !== w) {
            throw new Error('row ' + y + ' width ' + rows[y].length + ' != ' + w);
        }
    }
    const bytes = [];
    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            const k = rows[y][x];
            const v = k === '.' ? 0 : PAL565[k];
            if (v === undefined) throw new Error('unknown palette char: ' + k);
            bytes.push(v & 0xff, (v >> 8) & 0xff);
        }
    }
    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            bytes.push(rows[y][x] === '.' ? 0 : 255);
        }
    }
    return { w, h, bytes };
}

let out =
    '// assets/images/sm_sprites.c —— 由 tools/gen_sprites.mjs 自动生成,请勿手改。\n' +
    '// 源: assets/art/sm_art_matrices.mjs (《尖叫猴子》美术矩阵唯一数据源)。\n' +
    '// 格式: LVGL9 RGB565A8 —— 色平面 w*h*2 字节在前,alpha 平面 w*h 字节紧随,\n' +
    '//       两平面连续存放,data_size = w*h*3。透明像素 alpha=0。\n' +
    '#include "lvgl.h"\n' +
    '#include <stdint.h>\n\n';

const decls = [];
for (const [key, name] of Object.entries(OUT_NAMES)) {
    const rows = SPRITES[key];
    if (!rows) throw new Error('missing sprite: ' + key);
    const e = expand(rows);
    const arr = 'sm_px_' + key;
    let body = 'static const uint8_t ' + arr + '[' + (e.w * e.h * 3) + '] = {\n';
    for (let i = 0; i < e.bytes.length; i += 12) {
        body += '    ' + e.bytes.slice(i, i + 12).join(',') + ',\n';
    }
    body += '};\n';
    out += body;
    out += 'const lv_image_dsc_t ' + name + ' = {\n' +
        '    .header = { .cf = LV_COLOR_FORMAT_RGB565A8, .w = ' + e.w + ', .h = ' + e.h + ' },\n' +
        '    .data_size = sizeof(' + arr + '),\n' +
        '    .data = ' + arr + ',\n' +
        '};\n\n';
    decls.push(name);
}

const outPath = join(root, 'assets', 'images', 'sm_sprites.c');
writeFileSync(outPath, out);
console.log('written:', outPath);
console.log('sprites:', decls.join(', '));
