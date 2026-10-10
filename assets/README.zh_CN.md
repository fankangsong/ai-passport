<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

### 《尖叫猴子》字库（sm_font_16 / sm_font_24）

`feature/scream-monkey` 游戏应用的中文子集字库，用 `lv_font_conv` 从
Noto Sans SC（SIL OFL 1.1）生成。

- 字体源：Noto Sans SC SubsetOTF，下载自
  `https://github.com/notofonts/noto-cjk`（Regular SHA-256
  `faa6c9df652116dde789d351359f3d7e5d2285a2b2a1f04a2d7244df706d5ea9`，Bold
  `c6cb5a93abaa9edc8ee7463b7ebb7f42d618d40e6ed2f7a5371c97b0b64767c0`）。
  OTF 源文件不入库；重新生成时先下载。
- 码点由 `node tools/gen_fonts.mjs 16|24` 从 `main/game_monkey_text.h`
  （上屏文案唯一来源）提取；宿主契约测试：`tests/test_font_coverage.py`。
- 再生成命令（仓库根目录执行）：

```bash
npx -y lv_font_conv@1.5.3 --font <NotoSansSC-Regular.otf> --size 16 --bpp 4 \
  --format lvgl --no-compress --lv-font-name sm_font_16 --lv-include lvgl.h \
  --range "$(node tools/gen_fonts.mjs 16)" --output assets/fonts/sm_font_16.c

npx -y lv_font_conv@1.5.3 --font <NotoSansSC-Bold.otf> --size 24 --bpp 4 \
  --format lvgl --no-compress --lv-font-name sm_font_24 --lv-include lvgl.h \
  --range "$(node tools/gen_fonts.mjs 24)" --output assets/fonts/sm_font_24.c
```

- 覆盖：16px 为 56 汉字 + ASCII 0x20–0x7E（源码 ≈94KB）；24px Bold 仅标题字
  （≈10KB）。经 `main/CMakeLists.txt` 的 `target_sources` 编入应用。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |
| [`images/sm_sprites.c`](images/sm_sprites.c) | 生成的 C 源码 | 《尖叫猴子》像素精灵，LVGL9 `RGB565A8` 格式 `lv_image_dsc_t` 常量（猴子 24×24 ×6 帧、石头 16×16、仙人掌 12×24、心 10×10、树 16×16、云 16×6；数据 ≈13.4KB）。**生成文件——勿手改。** 唯一数据源：[`art/sm_art_matrices.mjs`](art/sm_art_matrices.mjs)（字符矩阵 + 调色板）；用 `node tools/gen_sprites.mjs` 展开。同一矩阵源同时驱动美术评审预览页。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

### 美术矩阵源（art/）

`art/sm_art_matrices.mjs` 是《尖叫猴子》精灵的唯一数据源：每个精灵是一个
字符矩阵（调色板索引 + `.` 透明），单精灵 ≤8 色，由 `tools/gen_sprites.mjs`
离线展开为 `images/sm_sprites.c`（RGB565A8：前 w*h*2 字节色平面，后 w*h 字节
alpha 平面）。改美术 → 改矩阵 → 重跑生成器；勿改生成的 C 文件。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
