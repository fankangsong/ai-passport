#!/usr/bin/env python3
"""字体子集覆盖契约测试。

契约:main/game_monkey_text.h 里每个非 ASCII 字符都必须出现在
assets/fonts/sm_font_16.c;标题字(SM_TITLE_TEXT)必须出现在
sm_font_24.c。另断言一个已知缺失的负例(U+9F98 龙),防止测试
无条件通过。

生成链路:tools/gen_fonts.mjs 提取码点 → npx lv_font_conv 生成字库。
改文案必须重跑生成命令(命令记录在 assets/README.md)。
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def font_codepoints(path: Path) -> set:
    """lv_font_conv 产物里每个字形前都有 '/* U+XXXX ... * /' 注释。"""
    glyphs = set()
    for m in re.finditer(r"/\* U\+([0-9A-Fa-f]{4,6})", path.read_text(encoding="utf-8")):
        glyphs.add(int(m.group(1), 16))
    return glyphs


def main() -> None:
    f16 = font_codepoints(ROOT / "assets" / "fonts" / "sm_font_16.c")
    f24 = font_codepoints(ROOT / "assets" / "fonts" / "sm_font_24.c")

    raw = (ROOT / "main" / "game_monkey_text.h").read_text(encoding="utf-8")
    chars = {ord(ch) for ch in strip_comments(raw) if ord(ch) > 0x7E}
    assert chars, "game_monkey_text.h 里没有找到非 ASCII 字符"

    missing16 = sorted(c for c in chars if c not in f16)
    assert not missing16, (
        "sm_font_16.c 缺少字形: " + ", ".join(f"U+{c:04X}" for c in missing16)
    )

    title_m = re.search(r'SM_TITLE_TEXT\s+"([^"]+)"', strip_comments(raw))
    assert title_m, "SM_TITLE_TEXT 未定义"
    missing24 = [ch for ch in title_m.group(1) if ord(ch) not in f24]
    assert not missing24, f"sm_font_24.c 缺少标题字形: {missing24!r}"

    # 负例:一个确定不在文案里的生僻字,防止字形集合意外膨胀后测试失去意义。
    assert 0x9F98 not in f16, "负例 U+9F98(龘族字)不应出现在正文字库"

    print(f"font coverage tests: PASS ({len(chars)} CJK chars covered)")


if __name__ == "__main__":
    main()
