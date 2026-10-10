// 《尖叫猴子》精灵矩阵唯一数据源 (source of truth)
// 字符 = 调色板索引，'.' = 透明。后续 tools/gen_sprites.mjs 直接消费本文件的
// 组装逻辑，展开为 LVGL9 RGB565A8 const 数组。预览页 HTML 使用同一套数据。
// 运行 `node sm_art_matrices.mjs print` 可打印全部矩阵。

function rep(s, n) { let r = ''; for (let i = 0; i < n; i++) r += s; return r; }

// ---- 角色调色板索引 ----
// o 描边深棕 #4A2F14 | b 体毛 #8B5A2B | l 手脚耳尾 #C89058
// f 脸肚 #F4E3C2 | e 眼白 #FFFFFF | p 瞳/墨 #1B2733 | r 嘴/伤红 #E43B2F

const D = rep('.', 24);
const H = [ // 头部（行 0-13，含耳、脸、眼、嘴基础版）
  D,
  '....oo' + rep('.', 10) + 'oo' + rep('.', 6),
  '...ollo' + rep('.', 10) + 'ollo...',
  rep('.', 5) + rep('o', 14) + rep('.', 5),
  '....o' + rep('b', 14) + 'o....',
  '....ob' + rep('f', 12) + 'bo....',
  '....ob' + rep('f', 12) + 'bo....',
  '....obf' + 'ee' + rep('f', 4) + 'ee' + 'fff' + 'bo....',
  '....obf' + 'ep' + rep('f', 4) + 'ep' + 'fff' + 'bo....',
  '....ob' + rep('f', 12) + 'bo....',
  '....ob' + rep('f', 12) + 'bo....',
  '....ob' + rep('f', 4) + 'rrrr' + rep('f', 4) + 'bo....',
  '....ob' + rep('f', 12) + 'bo....',
  rep('.', 5) + rep('o', 14) + rep('.', 5),
];
const B = [ // 躯干+手臂+尾根（行 14-20）
  rep('.', 6) + rep('o', 12) + rep('.', 6),
  '..olloob' + rep('f', 8) + 'boollo..',
  '..olloob' + rep('f', 8) + 'boollo..',
  '..olloob' + rep('f', 8) + 'boollo..',
  '.llll.ob' + rep('f', 8) + 'bo......',
  '.llll.o' + rep('b', 10) + 'o......',
  rep('.', 6) + rep('o', 12) + rep('.', 6),
];
const L_RUN1 = [rep('.', 5) + 'lll' + rep('.', 4) + 'llll' + rep('.', 8),
                '....ll' + rep('.', 8) + 'lll' + rep('.', 7), D];
const L_RUN2 = [rep('.', 6) + 'lll' + rep('.', 4) + 'llll' + rep('.', 7),
                rep('.', 6) + 'll' + rep('.', 8) + 'lll' + rep('.', 5), D];
const L_JUMP1 = [rep('.', 7) + 'lll' + rep('.', 2) + 'lll' + rep('.', 9), D, D];
const L_JUMP2 = [rep('.', 6) + 'lll' + rep('.', 4) + 'lll' + rep('.', 8),
                 rep('.', 6) + 'll' + rep('.', 7) + 'lll' + rep('.', 6), D];
const L_DEATH = [rep('.', 4) + 'lll' + rep('.', 10) + 'lll' + rep('.', 4),
                 '...ll' + rep('.', 14) + 'll...', D];

function frame(legs, over) {
  const r = H.concat(B, legs);
  if (over) for (const k in over) r[+k] = over[k];
  return r;
}
const HURT_OVER = { // 受伤：额头淤青 + X 眼 + 大张嘴
  4: '....o' + 'bbbb' + 'rr' + rep('b', 8) + 'o....',
  7: '....obf' + 'pfp' + 'ff' + 'pfp' + 'fff' + 'bo....',
  8: '....obf' + 'fp' + rep('f', 4) + 'p' + 'ffff' + 'bo....',
  11: '....ob' + rep('f', 3) + rep('r', 7) + 'ff' + 'bo....',
  12: '....ob' + rep('f', 4) + 'rrr' + rep('f', 5) + 'bo....',
};
const DEATH_OVER = Object.assign({}, HURT_OVER, { // 死亡：+ 吐舌 + 尾巴摊平 + 四肢摊开
  18: 'lllll.ob' + rep('f', 8) + 'bo......',
  19: 'lllll.o' + rep('b', 10) + 'o......',
});

// ---- 场景 / 障碍调色板索引 ----
// 石: o 描边 #3E4750 | S 亮 #C2CBD0 | s 体 #9AA5AB | d 暗/裂纹 #6E787E
// 掌: g 描边 #1E5B25 | c 体 #3E8E41 | C 亮 #5FAF62 | w 刺 #E8F5E9
// 树: t 冠亮 #4E9B47 | T 冠 #2E7D32 | k 干 #6B4226
// 云: w #FFFFFF | 心: o 描边 #7A1E14 | R 亮 #FF6F5E | r 红 #E43B2F

function blank(w, h) { const a = []; for (let y = 0; y < h; y++) a.push(rep('.', w)); return a; }
function put(rows, y, x, s) { rows[y] = rows[y].substring(0, x) + s + rows[y].substring(x + s.length); }

const STONE = blank(16, 16);
put(STONE, 1, 5, rep('o', 6)); put(STONE, 2, 4, 'oSSSSSso');
put(STONE, 3, 3, 'oSSSSsssso'); put(STONE, 4, 2, 'oSSSssssssdo');
put(STONE, 5, 2, 'oSSssssssddo'); put(STONE, 6, 1, 'oSSsssssssddo');
put(STONE, 7, 1, 'oSSsssssssdddo'); put(STONE, 8, 1, 'oSsssssddsssdo');
put(STONE, 9, 1, 'osssssddssssdo'); put(STONE, 10, 1, 'ossssddsssssdo');
put(STONE, 11, 1, 'osssssssssdddo'); put(STONE, 12, 2, 'ossssssssddo');
put(STONE, 13, 3, 'osssssssdo'); put(STONE, 14, 4, 'oodddoo');

const CACTUS = blank(12, 24);
put(CACTUS, 0, 4, 'gggg'); put(CACTUS, 1, 4, 'gCCg'); put(CACTUS, 2, 4, 'gCCg');
put(CACTUS, 3, 4, 'gccg'); put(CACTUS, 4, 0, 'gg'); put(CACTUS, 4, 4, 'gccg');
put(CACTUS, 5, 0, 'gcg'); put(CACTUS, 5, 4, 'gccg'); put(CACTUS, 6, 0, 'gcgg');
put(CACTUS, 6, 4, 'gccg'); put(CACTUS, 7, 0, 'gccc'); put(CACTUS, 7, 4, 'gccg');
put(CACTUS, 8, 0, 'ggggg'); put(CACTUS, 8, 5, 'ccg'); put(CACTUS, 9, 4, 'gccg');
put(CACTUS, 10, 4, 'gccg'); put(CACTUS, 10, 8, 'gg'); put(CACTUS, 11, 4, 'gccg');
put(CACTUS, 11, 8, 'gcg'); put(CACTUS, 12, 4, 'gccg'); put(CACTUS, 12, 8, 'gcg');
put(CACTUS, 13, 4, 'gccg'); put(CACTUS, 13, 8, 'gcg'); put(CACTUS, 14, 4, 'gccg');
put(CACTUS, 14, 8, 'gggg'); put(CACTUS, 15, 4, 'gccg'); put(CACTUS, 16, 4, 'gccg');
put(CACTUS, 17, 1, 'w'); put(CACTUS, 17, 4, 'gccg'); put(CACTUS, 17, 10, 'w');
put(CACTUS, 18, 4, 'gccg'); put(CACTUS, 19, 1, 'w'); put(CACTUS, 19, 4, 'gccg');
put(CACTUS, 19, 10, 'w'); put(CACTUS, 20, 4, 'gccg'); put(CACTUS, 21, 3, 'ggccgg');
put(CACTUS, 22, 2, rep('g', 8));

const TREE = blank(16, 16);
put(TREE, 0, 4, rep('t', 6)); put(TREE, 1, 2, 'ttTTTTTTtt');
put(TREE, 2, 1, 'ttTTTTTTTTtt'); put(TREE, 3, 1, 'tTTTTTTTTTTt');
put(TREE, 4, 0, 'tTTTTTTTTTTTTt'); put(TREE, 5, 0, 'tTTTTTTTTTTTTt');
put(TREE, 6, 1, 'tTTTTTTTTTTt'); put(TREE, 7, 1, 'ttTTTTTTTTtt');
put(TREE, 8, 2, 'tttTTTTttt'); put(TREE, 9, 4, rep('t', 7));
put(TREE, 10, 6, 'kkk'); put(TREE, 11, 6, 'kkk'); put(TREE, 12, 6, 'kkk');
put(TREE, 13, 6, 'kkk'); put(TREE, 14, 5, 'kkkkk');

const CLOUD = blank(16, 6);
put(CLOUD, 0, 4, rep('w', 4)); put(CLOUD, 1, 2, rep('w', 9));
put(CLOUD, 2, 1, rep('w', 12)); put(CLOUD, 3, 0, rep('w', 14));
put(CLOUD, 4, 1, rep('w', 12)); put(CLOUD, 5, 2, rep('w', 7));

const HEART = blank(10, 10);
put(HEART, 1, 1, 'oo'); put(HEART, 1, 7, 'oo'); put(HEART, 2, 0, 'oRRoooorro');
put(HEART, 3, 0, 'oRrrrrrrro'); put(HEART, 4, 0, 'orrrrrrrro');
put(HEART, 5, 1, 'orrrrrro'); put(HEART, 6, 2, 'orrrro');
put(HEART, 7, 3, 'orro'); put(HEART, 8, 4, 'oo');

export const PALETTES = {
  monkey: { o: '#4A2F14', b: '#8B5A2B', l: '#C89058', f: '#F4E3C2', e: '#FFFFFF', p: '#1B2733', r: '#E43B2F' },
  stone: { o: '#3E4750', S: '#C2CBD0', s: '#9AA5AB', d: '#6E787E' },
  cactus: { g: '#1E5B25', c: '#3E8E41', C: '#5FAF62', w: '#E8F5E9' },
  tree: { t: '#4E9B47', T: '#2E7D32', k: '#6B4226' },
  cloud: { w: '#FFFFFF' },
  heart: { o: '#7A1E14', R: '#FF6F5E', r: '#E43B2F' },
};

export const SPRITES = {
  monkey_run1: frame(L_RUN1),
  monkey_run2: frame(L_RUN2, { 17: 'llolloob' + rep('f', 8) + 'boollo..' }),
  monkey_jump_up: frame(L_JUMP1),
  monkey_jump_down: frame(L_JUMP2),
  monkey_hurt: frame(L_RUN1, HURT_OVER),
  monkey_death: frame(L_DEATH, DEATH_OVER),
  stone: STONE,
  cactus: CACTUS,
  tree: TREE,
  cloud: CLOUD,
  heart: HEART,
};

export const COLORS = { // 场景 / HUD 直配色
  sky: '#9CE0FF', grass: '#82BE2D', grassDark: '#55951D',
  dirt: '#75452E', dirtDark: '#4A2C1B', pit: '#2B1B0E', pitEdge: '#6B4A2E',
  hudPaper: '#F4F4EA', ink: '#17202A',
  barGreen: '#4CAF50', barOrange: '#FF9800', barRed: '#F44336',
  groundY: 64, // 逻辑坐标：地面顶（实景区 240x90 内）
};

if (process.argv[2] === 'print') {
  for (const [k, v] of Object.entries(SPRITES)) {
    console.log('### ' + k + ' (' + v[0].length + 'x' + v.length + ')');
    console.log(v.join('\n'));
  }
}
