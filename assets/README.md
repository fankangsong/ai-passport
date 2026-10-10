<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

### Scream Monkey (`sm_font_16`, `sm_font_24`)

Chinese subset fonts for the `feature/scream-monkey` game application, generated
with `lv_font_conv` from Noto Sans SC (SIL OFL 1.1).

- Source font: Noto Sans SC SubsetOTF, downloaded from
  `https://github.com/notofonts/noto-cjk` (Regular SHA-256
  `faa6c9df652116dde789d351359f3d7e5d2285a2b2a1f04a2d7244df706d5ea9`, Bold
  `c6cb5a93abaa9edc8ee7463b7ebb7f42d618d40e6ed2f7a5371c97b0b64767c0`).
  The OTF is NOT committed; re-download when regenerating.
- Codepoints are extracted from `main/game_monkey_text.h` (single source for UI
  strings) via `node tools/gen_fonts.mjs 16|24`. Host contract test:
  `tests/test_font_coverage.py`.
- Regeneration commands (run from the repository root):

```bash
npx -y lv_font_conv@1.5.3 --font <NotoSansSC-Regular.otf> --size 16 --bpp 4 \
  --format lvgl --no-compress --lv-font-name sm_font_16 --lv-include lvgl.h \
  --range "$(node tools/gen_fonts.mjs 16)" --output assets/fonts/sm_font_16.c

npx -y lv_font_conv@1.5.3 --font <NotoSansSC-Bold.otf> --size 24 --bpp 4 \
  --format lvgl --no-compress --lv-font-name sm_font_24 --lv-include lvgl.h \
  --range "$(node tools/gen_fonts.mjs 24)" --output assets/fonts/sm_font_24.c
```

- Coverage: 56 CJK chars + ASCII 0x20–0x7E at 16 px (≈94 KB source), title-only
  subset at 24 px Bold (≈10 KB). Compiled into the application via
  `main/CMakeLists.txt` `target_sources`.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |
| [`images/sm_sprites.c`](images/sm_sprites.c) | Generated C source | Scream Monkey pixel sprites as LVGL9 `RGB565A8` `lv_image_dsc_t` constants (monkey 24×24 ×6 frames, stone 16×16, cactus 12×24, heart 10×10, tree 16×16, cloud 16×6; ≈13.4 KB data). **Generated file — do not edit.** Source of truth: [`art/sm_art_matrices.mjs`](art/sm_art_matrices.mjs) (character-matrix sprites + palettes); expand with `node tools/gen_sprites.mjs`. The same matrix source drives the design review preview. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

### Art matrix source (`art/`)

`art/sm_art_matrices.mjs` is the single source of truth for the Scream Monkey
sprites: each sprite is a character-matrix (palette indices + `.` for
transparency) with ≤8 colors per sprite, expanded offline by
`tools/gen_sprites.mjs` into `images/sm_sprites.c` (RGB565A8: color plane
`w*h*2` bytes followed by alpha plane `w*h` bytes). Edit the matrices, rerun
the generator; never edit the generated C file.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
