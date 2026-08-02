#!/usr/bin/env python3
"""Generate FT-02's global embedded bold CJK bitmap font.

The firmware stores carefully hinted monochrome 1-bit glyphs, not the source font file. The
character set covers printable Latin, common technical symbols, CJK
punctuation, full-width forms, and the complete Unicode CJK Unified Ideographs
Basic Block (U+4E00..U+9FFF). All glyphs are generated from the Simplified
Chinese face of a TTC collection.
"""
from __future__ import annotations

import argparse
import unicodedata
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTCollection, TTFont

RANGES = [
    (0x0020, 0x007E),  # Printable ASCII
    (0x00A0, 0x00FF),  # Latin-1 supplement
    (0x2000, 0x206F),  # General punctuation
    (0x2100, 0x214F),  # Letterlike symbols
    (0x2150, 0x218F),  # Number forms
    (0x2190, 0x21FF),  # Arrows
    (0x2200, 0x22FF),  # Mathematical operators
    (0x2460, 0x24FF),  # Enclosed alphanumerics
    (0x2500, 0x257F),  # Box drawing
    (0x25A0, 0x25FF),  # Geometric shapes
    (0x2600, 0x26FF),  # Miscellaneous symbols
    (0x3000, 0x303F),  # CJK punctuation
    (0x4E00, 0x9FFF),  # Complete CJK Unified Ideographs basic block
    (0xF900, 0xFAFF),  # CJK compatibility ideographs
    (0xFF01, 0xFFEF),  # Full-width and half-width forms
]

ALLOWED_BLANK = {0x0020, 0x00A0, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004,
                 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x202F,
                 0x205F, 0x3000, 0xFFA0}


def font_cmap(path: Path, face_index: int) -> set[int]:
    if path.suffix.lower() == ".ttc":
        coll = TTCollection(str(path), lazy=True)
        font = coll.fonts[face_index]
    else:
        font = TTFont(str(path), lazy=True)
    cps: set[int] = set()
    for table in font["cmap"].tables:
        if table.isUnicode():
            cps.update(table.cmap.keys())
    return cps


def charset(cmap: set[int]) -> list[int]:
    result: set[int] = set()
    for start, end in RANGES:
        for cp in range(start, end + 1):
            if cp not in cmap:
                continue
            cat = unicodedata.category(chr(cp))
            # Combining and format controls do not work as stand-alone glyphs
            # in this compact baseline renderer, so omit them deliberately.
            if cat in {"Cc", "Cf", "Cs", "Mn", "Me"}:
                continue
            result.add(cp)
    return sorted(result)


def pack_rows(image: Image.Image) -> tuple[int, int, int, list[int], int]:
    width, height = image.size
    bytes_per_row = (width + 7) // 8 if width else 0
    output: list[int] = []
    pixels = image.load()
    on_pixels = 0
    for y in range(height):
        for byte_index in range(bytes_per_row):
            value = 0
            for bit in range(8):
                x = byte_index * 8 + bit
                if x < width and bool(pixels[x, y]):
                    value |= 0x80 >> bit
                    on_pixels += 1
            output.append(value)
    return width, height, bytes_per_row, output, on_pixels


def generate(font_path: Path, face_index: int, size: int, header: Path, source: Path) -> None:
    cmap = font_cmap(font_path, face_index)
    codepoints = charset(cmap)
    font = ImageFont.truetype(str(font_path), size, index=face_index)

    records: list[tuple[int, int]] = []
    data: list[int] = []
    bad: list[int] = []

    for cp in codepoints:
        ch = chr(cp)
        records.append((cp, len(data)))
        bbox = font.getbbox(ch, anchor="ls")
        x0, y0, x1, y1 = bbox
        width = max(0, x1 - x0)
        height = max(0, y1 - y0)
        if width and height:
            image = Image.new("1", (width, height), 0)
            draw = ImageDraw.Draw(image)
            # Render through FreeType's monochrome path. This keeps 1-bit strokes
            # cleaner than thresholding a low-resolution anti-aliased image.
            draw.text((-x0, -y0), ch, font=font, fill=1, anchor="ls")
        else:
            image = Image.new("1", (0, 0), 0)
        width, height, bpr, bitmap, on_pixels = pack_rows(image)
        if cp not in ALLOWED_BLANK and on_pixels == 0:
            bad.append(cp)
        advance = max(1, round(font.getlength(ch)))
        if any(not -128 <= v <= 127 for v in (x0, y0)):
            raise RuntimeError(f"glyph offset outside int8: U+{cp:04X} {bbox}")
        if any(v > 255 for v in (width, height, bpr, advance)):
            raise RuntimeError(f"glyph metric outside uint8: U+{cp:04X} {bbox}")
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

    if bad:
        preview = ", ".join(f"U+{cp:04X}" for cp in bad[:20])
        raise RuntimeError(f"non-space empty glyphs detected ({len(bad)}): {preview}")

    header.write_text(
        "#pragma once\n"
        "#include <Arduino.h>\n"
        "#include \"FT02_FontPackTypes.h\"\n\n"
        "// Global embedded bold font: full CJK Basic block + UI punctuation/symbols.\n"
        "// Source font files are intentionally not distributed with this project.\n"
        "extern const FT02FontPack ft02_cjk_24b;\n"
        "extern const uint32_t FT02_CJK_24B_GLYPH_COUNT;\n",
        encoding="utf-8",
    )

    with source.open("w", encoding="utf-8") as f:
        f.write('#include "FT02_GlobalCJKBoldFontData.h"\n\n')
        f.write("static const FT02FontIndexRecord ft02_cjk_24b_idx[] PROGMEM = {\n")
        for cp, offset in records:
            f.write(f"    {{0x{cp:04X}, {offset}}},\n")
        f.write("};\n\n")
        f.write("static const uint8_t ft02_cjk_24b_bin[] PROGMEM = {\n")
        for start in range(0, len(data), 20):
            f.write("    " + ", ".join(f"0x{v:02X}" for v in data[start:start+20]) + ",\n")
        f.write("};\n\n")
        f.write(f"const uint32_t FT02_CJK_24B_GLYPH_COUNT = {len(records)};\n")
        f.write("const FT02FontPack ft02_cjk_24b = {\n")
        f.write("    ft02_cjk_24b_idx,\n")
        f.write(f"    {len(records)},\n")
        f.write("    ft02_cjk_24b_bin,\n")
        f.write(f"    {len(data)}\n")
        f.write("};\n")

    print(f"glyphs={len(records)} data_bytes={len(data)}")
    for sample in "壳中灯知识库急救导航测试":
        cp = ord(sample)
        print(f"sample {sample} U+{cp:04X}: {'yes' if cp in codepoints else 'NO'}")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--font", type=Path, default=Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"))
    p.add_argument("--face-index", type=int, default=2, help="Noto Sans CJK SC face in the TTC")
    p.add_argument("--size", type=int, default=24)
    p.add_argument("--header", type=Path, default=Path("src/FT02_GlobalCJKBoldFontData.h"))
    p.add_argument("--source", type=Path, default=Path("src/FT02_GlobalCJKBoldFontData.cpp"))
    args = p.parse_args()
    generate(args.font, args.face_index, args.size, args.header, args.source)


if __name__ == "__main__":
    main()
