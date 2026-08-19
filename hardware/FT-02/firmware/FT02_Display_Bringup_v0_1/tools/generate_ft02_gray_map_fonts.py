#!/usr/bin/env python3
"""Generate compact 2-bit anti-aliased font packs used by the native four-gray map.

The source font files are build-time inputs only and are never distributed in the firmware
package. Generated glyph coverage is quantized to four levels and stored in committed C++ data.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROAD_RE = re.compile(r"\{0x([0-9A-Fa-f]+),\s*\{")

UI_TEXT = (
    ''.join(chr(i) for i in range(0x20, 0x7F))
    + '已连接搜索中通讯超时数据异常无数据定位剩余电量速度方向海拔距离比例尺未米'
      '帮助返回地图加载错误重试当前位置跟随自由浏览卫星时间版本壳中灯智能随身终端'
      '记录中完成自动手动点失败白黑深灰浅灰导航道路建筑轨迹中心刷新'
)


def read_road_codepoints(path: Path) -> list[int]:
    text = path.read_text(encoding='utf-8')
    cps = {int(m.group(1), 16) for m in ROAD_RE.finditer(text)}
    cps.update(ord(ch) for ch in ' ?-+./:()（）·—0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz')
    return sorted(cps)


def quantize(value: int) -> int:
    # Coverage: 0 transparent, 1 light edge, 2 dark edge, 3 solid ink.
    if value < 24:
        return 0
    if value < 104:
        return 1
    if value < 200:
        return 2
    return 3


def pack_2bpp(image: Image.Image) -> tuple[int, int, int, list[int]]:
    width, height = image.size
    bytes_per_row = (width + 3) // 4 if width else 0
    px = image.load()
    output: list[int] = []
    for y in range(height):
        for byte_index in range(bytes_per_row):
            value = 0
            for slot in range(4):
                x = byte_index * 4 + slot
                coverage = quantize(px[x, y]) if x < width else 0
                value |= coverage << (6 - slot * 2)
            output.append(value)
    return width, height, bytes_per_row, output


def make_pack(font_path: Path, face_index: int, size: int, codepoints: list[int]):
    font = ImageFont.truetype(str(font_path), size, index=face_index)
    records: list[tuple[int, int]] = []
    data: list[int] = []
    for cp in codepoints:
        ch = chr(cp)
        records.append((cp, len(data)))
        bbox = font.getbbox(ch, anchor='ls')
        x0, y0, x1, y1 = bbox
        width = max(0, x1 - x0)
        height = max(0, y1 - y0)
        if width and height:
            image = Image.new('L', (width, height), 0)
            draw = ImageDraw.Draw(image)
            draw.text((-x0, -y0), ch, font=font, fill=255, anchor='ls')
        else:
            image = Image.new('L', (0, 0), 0)
        width, height, bpr, bitmap = pack_2bpp(image)
        advance = max(1, round(font.getlength(ch)))
        if any(not -128 <= v <= 127 for v in (x0, y0)):
            raise RuntimeError(f'offset out of int8 for U+{cp:04X}: {bbox}')
        if any(v > 255 for v in (width, height, bpr, advance)):
            raise RuntimeError(f'metric out of uint8 for U+{cp:04X}: {bbox}')
        data.extend([
            width & 0xFF,
            height & 0xFF,
            bpr & 0xFF,
            x0 & 0xFF,
            y0 & 0xFF,
            advance & 0xFF,
            0,
            0,
            *bitmap,
        ])
    return records, data


def write_pack(f, symbol: str, records: list[tuple[int, int]], data: list[int]) -> None:
    f.write(f'static const FT02GrayFontIndexRecord {symbol}_idx[] PROGMEM = {{\n')
    for cp, offset in records:
        f.write(f'    {{0x{cp:04X}, {offset}}},\n')
    f.write('};\n\n')
    f.write(f'static const uint8_t {symbol}_bin[] PROGMEM = {{\n')
    for start in range(0, len(data), 20):
        f.write('    ' + ', '.join(f'0x{v:02X}' for v in data[start:start+20]) + ',\n')
    f.write('};\n\n')
    f.write(f'const FT02GrayFontPack {symbol} = {{\n')
    f.write(f'    {symbol}_idx,\n')
    f.write(f'    {len(records)},\n')
    f.write(f'    {symbol}_bin,\n')
    f.write(f'    {len(data)}\n')
    f.write('};\n\n')


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument('--road-header', type=Path, default=Path('src/FT02_RoadNameFont.h'))
    p.add_argument('--regular-font', type=Path, default=Path('/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'))
    p.add_argument('--bold-font', type=Path, default=Path('/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc'))
    p.add_argument('--face-index', type=int, default=2)
    p.add_argument('--header', type=Path, default=Path('src/FT02_GrayMapFontData.h'))
    p.add_argument('--source', type=Path, default=Path('src/FT02_GrayMapFontData.cpp'))
    args = p.parse_args()

    road_cps = read_road_codepoints(args.road_header)
    ui_cps = sorted({ord(ch) for ch in UI_TEXT})
    time_cps = sorted({ord(ch) for ch in ''.join(chr(i) for i in range(0x20, 0x7F)) + '地图'})

    road_records, road_data = make_pack(args.regular_font, args.face_index, 18, road_cps)
    ui_records, ui_data = make_pack(args.regular_font, args.face_index, 20, ui_cps)
    bold_records, bold_data = make_pack(args.bold_font, args.face_index, 30, time_cps)

    args.header.write_text(
        '#pragma once\n'
        '#include <Arduino.h>\n'
        '#include "FT02_GrayFontPackTypes.h"\n\n'
        '// Generated 2-bit anti-aliased glyph data for the native four-gray map.\n'
        '// Source font files are intentionally not distributed with this project.\n'
        'extern const FT02GrayFontPack ft02_gray_road_18r;\n'
        'extern const FT02GrayFontPack ft02_gray_ui_20r;\n'
        'extern const FT02GrayFontPack ft02_gray_ui_30b;\n',
        encoding='utf-8',
    )

    with args.source.open('w', encoding='utf-8') as f:
        f.write('#include "FT02_GrayMapFontData.h"\n\n')
        write_pack(f, 'ft02_gray_road_18r', road_records, road_data)
        write_pack(f, 'ft02_gray_ui_20r', ui_records, ui_data)
        write_pack(f, 'ft02_gray_ui_30b', bold_records, bold_data)

    print(f'road glyphs={len(road_records)} data={len(road_data)}')
    print(f'ui glyphs={len(ui_records)} data={len(ui_data)}')
    print(f'bold glyphs={len(bold_records)} data={len(bold_data)}')


if __name__ == '__main__':
    main()
