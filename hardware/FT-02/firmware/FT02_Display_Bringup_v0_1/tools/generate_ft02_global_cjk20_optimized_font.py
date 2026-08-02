#!/usr/bin/env python3
"""Generate FT-02's optimized global 20 px CJK bitmap font.

The glyphs use the same rendering pipeline chosen on the v2.57 hardware
comparison page: Noto Sans CJK SC at 20 px, anti-aliased grayscale rendering,
then a fixed threshold of 170 into a 1-bit bitmap. Source font files are never
copied into the firmware package.
"""
from __future__ import annotations

import argparse
import unicodedata
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTCollection, TTFont

RANGES = [
    (0x0020, 0x007E), (0x00A0, 0x00FF), (0x2000, 0x206F),
    (0x2100, 0x214F), (0x2150, 0x218F), (0x2190, 0x21FF),
    (0x2200, 0x22FF), (0x2460, 0x24FF), (0x2500, 0x257F),
    (0x25A0, 0x25FF), (0x2600, 0x26FF), (0x3000, 0x303F),
    (0x4E00, 0x9FFF), (0xF900, 0xFAFF), (0xFF01, 0xFFEF),
]

ALLOWED_BLANK = {
    0x0020, 0x00A0, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004,
    0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x202F,
    0x205F, 0x3000, 0xFFA0,
}


def font_cmap(path: Path, face_index: int) -> set[int]:
    if path.suffix.lower() == '.ttc':
        font = TTCollection(str(path), lazy=True).fonts[face_index]
    else:
        font = TTFont(str(path), lazy=True)
    cps: set[int] = set()
    for table in font['cmap'].tables:
        if table.isUnicode():
            cps.update(table.cmap.keys())
    return cps


def charset(cmap: set[int]) -> list[int]:
    result: set[int] = set()
    for start, end in RANGES:
        for cp in range(start, end + 1):
            if cp not in cmap:
                continue
            if unicodedata.category(chr(cp)) in {'Cc', 'Cf', 'Cs', 'Mn', 'Me'}:
                continue
            result.add(cp)
    return sorted(result)


def pack_rows(image: Image.Image) -> tuple[int, int, int, list[int], int]:
    width, height = image.size
    bpr = (width + 7) // 8 if width else 0
    output: list[int] = []
    px = image.load()
    on_pixels = 0
    for y in range(height):
        for byte_index in range(bpr):
            value = 0
            for bit in range(8):
                x = byte_index * 8 + bit
                if x < width and px[x, y]:
                    value |= 0x80 >> bit
                    on_pixels += 1
            output.append(value)
    return width, height, bpr, output, on_pixels


def generate(font_path: Path, face_index: int, size: int, threshold: int,
             header: Path, source: Path) -> None:
    cmap = font_cmap(font_path, face_index)
    codepoints = charset(cmap)
    font = ImageFont.truetype(str(font_path), size, index=face_index)
    records: list[tuple[int, int]] = []
    data: list[int] = []
    bad: list[int] = []

    for cp in codepoints:
        ch = chr(cp)
        records.append((cp, len(data)))
        x0, y0, x1, y1 = font.getbbox(ch, anchor='ls')
        width = max(0, x1 - x0)
        height = max(0, y1 - y0)
        if width and height:
            gray = Image.new('L', (width, height), 255)
            draw = ImageDraw.Draw(gray)
            draw.text((-x0, -y0), ch, font=font, fill=0, anchor='ls')
            mono = gray.point(lambda p: 1 if p < threshold else 0, mode='1')
        else:
            mono = Image.new('1', (0, 0), 0)
        width, height, bpr, bitmap, on_pixels = pack_rows(mono)
        if cp not in ALLOWED_BLANK and on_pixels == 0:
            bad.append(cp)
        advance = max(1, round(font.getlength(ch)))
        if any(not -128 <= v <= 127 for v in (x0, y0)):
            raise RuntimeError(f'glyph offset outside int8: U+{cp:04X}')
        if any(v > 255 for v in (width, height, bpr, advance)):
            raise RuntimeError(f'glyph metric outside uint8: U+{cp:04X}')
        data.extend([
            width & 0xFF, height & 0xFF, bpr & 0xFF,
            x0 & 0xFF, y0 & 0xFF, advance & 0xFF,
            0, 0, *bitmap,
        ])

    if bad:
        preview = ', '.join(f'U+{cp:04X}' for cp in bad[:20])
        raise RuntimeError(f'non-space empty glyphs ({len(bad)}): {preview}')

    header.write_text(
        '#pragma once\n#include <Arduino.h>\n#include "FT02_FontPackTypes.h"\n\n'
        '// Optimized global 20 px font: grayscale raster + threshold 170.\n'
        '// Source font files are intentionally not distributed with this project.\n'
        'extern const FT02FontPack ft02_cjk_20r;\n'
        'extern const uint32_t FT02_CJK_20R_GLYPH_COUNT;\n',
        encoding='utf-8',
    )

    with source.open('w', encoding='utf-8') as f:
        f.write('#include "FT02_GlobalCJK20FontData.h"\n\n')
        f.write('static const FT02FontIndexRecord ft02_cjk_20r_idx[] PROGMEM = {\n')
        for cp, offset in records:
            f.write(f'    {{0x{cp:04X}, {offset}}},\n')
        f.write('};\n\n')
        f.write('static const uint8_t ft02_cjk_20r_bin[] PROGMEM = {\n')
        for start in range(0, len(data), 20):
            f.write('    ' + ', '.join(f'0x{v:02X}' for v in data[start:start + 20]) + ',\n')
        f.write('};\n\n')
        f.write(f'const uint32_t FT02_CJK_20R_GLYPH_COUNT = {len(records)};\n')
        f.write('const FT02FontPack ft02_cjk_20r = {\n')
        f.write('    ft02_cjk_20r_idx,\n')
        f.write(f'    {len(records)},\n')
        f.write('    ft02_cjk_20r_bin,\n')
        f.write(f'    {len(data)}\n')
        f.write('};\n')

    print(f'glyphs={len(records)} data_bytes={len(data)} threshold={threshold}')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--font', type=Path, default=Path('/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'))
    parser.add_argument('--face-index', type=int, default=2)
    parser.add_argument('--size', type=int, default=20)
    parser.add_argument('--threshold', type=int, default=170)
    parser.add_argument('--header', type=Path, default=Path('src/FT02_GlobalCJK20FontData.h'))
    parser.add_argument('--source', type=Path, default=Path('src/FT02_GlobalCJK20FontData.cpp'))
    args = parser.parse_args()
    generate(args.font, args.face_index, args.size, args.threshold, args.header, args.source)


if __name__ == '__main__':
    main()
