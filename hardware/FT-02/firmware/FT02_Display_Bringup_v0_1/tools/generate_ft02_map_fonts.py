#!/usr/bin/env python3
"""Generate FT-02 map label and map-status bitmap headers.

The generated headers are committed to the firmware package. The source font
file is intentionally not included. Pillow and a locally installed CJK font are
only needed when regenerating the headers.
"""
from __future__ import annotations

import argparse
import math
import zlib
from pathlib import Path
from typing import Iterator

from PIL import Image, ImageDraw, ImageFont


def read_varint(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while pos < len(data) and shift <= 63:
        byte = data[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, pos
        shift += 7
    raise ValueError("invalid varint")


def fields(data: bytes) -> Iterator[tuple[int, int, int | bytes]]:
    pos = 0
    while pos < len(data):
        key, pos = read_varint(data, pos)
        field = key >> 3
        wire = key & 7
        if wire == 0:
            value, pos = read_varint(data, pos)
            yield field, wire, value
        elif wire == 1:
            value = data[pos:pos + 8]
            if len(value) != 8:
                raise ValueError("truncated fixed64")
            pos += 8
            yield field, wire, value
        elif wire == 2:
            length, pos = read_varint(data, pos)
            value = data[pos:pos + length]
            if len(value) != length:
                raise ValueError("truncated bytes")
            pos += length
            yield field, wire, value
        elif wire == 5:
            value = data[pos:pos + 4]
            if len(value) != 4:
                raise ValueError("truncated fixed32")
            pos += 4
            yield field, wire, value
        else:
            raise ValueError(f"unsupported wire type {wire}")


def packed(data: bytes) -> Iterator[int]:
    pos = 0
    while pos < len(data):
        value, pos = read_varint(data, pos)
        yield value


def blob_header(data: bytes) -> tuple[str, int]:
    block_type = None
    data_size = None
    for field, wire, value in fields(data):
        if field == 1 and wire == 2:
            block_type = bytes(value).decode("ascii")
        elif field == 3 and wire == 0:
            data_size = int(value)
    if block_type is None or data_size is None:
        raise ValueError("invalid blob header")
    return block_type, data_size


def blob_payload(data: bytes) -> bytes:
    raw = None
    raw_size = 0
    compressed = None
    for field, wire, value in fields(data):
        if field == 1 and wire == 2:
            raw = bytes(value)
        elif field == 2 and wire == 0:
            raw_size = int(value)
        elif field == 3 and wire == 2:
            compressed = bytes(value)
    if raw is not None:
        return raw
    if compressed is None:
        raise ValueError("blob has no supported payload")
    result = zlib.decompress(compressed)
    if len(result) != raw_size:
        raise ValueError("raw_size mismatch")
    return result


def iter_osm_data(path: Path) -> Iterator[bytes]:
    with path.open("rb") as stream:
        while True:
            prefix = stream.read(4)
            if not prefix:
                return
            header_bytes = int.from_bytes(prefix, "big")
            header = stream.read(header_bytes)
            block_type, data_size = blob_header(header)
            blob = stream.read(data_size)
            if block_type == "OSMData":
                yield blob_payload(blob)


def tags_from_arrays(strings: list[str], keys: list[int], values: list[int]) -> dict[str, str]:
    return {
        strings[key]: strings[value]
        for key, value in zip(keys, values)
        if key < len(strings) and value < len(strings)
    }


def collect_name_codepoints(source: Path) -> set[int]:
    names: set[str] = set()
    for raw in iter_osm_data(source):
        string_message = None
        groups: list[bytes] = []
        for field, wire, value in fields(raw):
            if field == 1 and wire == 2:
                string_message = bytes(value)
            elif field == 2 and wire == 2:
                groups.append(bytes(value))
        strings: list[str] = []
        if string_message is not None:
            for field, wire, value in fields(string_message):
                if field == 1 and wire == 2:
                    strings.append(bytes(value).decode("utf-8", "replace"))

        for group in groups:
            for field, wire, value in fields(group):
                if wire != 2:
                    continue
                message = bytes(value)
                if field in (1, 3):
                    keys: list[int] = []
                    values: list[int] = []
                    for subfield, subwire, subvalue in fields(message):
                        if subfield == 2:
                            if subwire == 2:
                                keys.extend(packed(bytes(subvalue)))
                            elif subwire == 0:
                                keys.append(int(subvalue))
                        elif subfield == 3:
                            if subwire == 2:
                                values.extend(packed(bytes(subvalue)))
                            elif subwire == 0:
                                values.append(int(subvalue))
                    tags = tags_from_arrays(strings, keys, values)
                    name = tags.get("name:zh") or tags.get("name")
                    if name:
                        names.add(name)
                elif field == 2:
                    key_values: list[int] = []
                    for subfield, subwire, subvalue in fields(message):
                        if subfield == 10:
                            if subwire == 2:
                                key_values.extend(packed(bytes(subvalue)))
                            elif subwire == 0:
                                key_values.append(int(subvalue))
                    cursor = 0
                    node_tags: dict[str, str] = {}
                    while cursor < len(key_values):
                        key = key_values[cursor]
                        cursor += 1
                        if key == 0:
                            name = node_tags.get("name:zh") or node_tags.get("name")
                            if name:
                                names.add(name)
                            node_tags = {}
                            continue
                        if cursor >= len(key_values):
                            break
                        tag_value = key_values[cursor]
                        cursor += 1
                        if key < len(strings) and tag_value < len(strings):
                            node_tags[strings[key]] = strings[tag_value]

    characters = set(" ?-+./:()（）·—0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
    for name in names:
        characters.update(name)
    return {ord(char) for char in characters}


def fixed_16_bitmap(font: ImageFont.FreeTypeFont, character: str) -> list[int]:
    working = Image.new("L", (48, 48), 0)
    draw = ImageDraw.Draw(working)
    bbox = draw.textbbox((24, 24), character, font=font, anchor="mm")
    draw.text((24, 24), character, font=font, fill=255, anchor="mm")
    actual = working.getbbox()
    if actual is None:
        return [0] * 32
    glyph = working.crop(actual)
    width, height = glyph.size
    scale = min(15 / max(1, width), 15 / max(1, height), 1.0)
    if scale < 1.0:
        glyph = glyph.resize(
            (max(1, round(width * scale)), max(1, round(height * scale))),
            Image.Resampling.LANCZOS,
        )
    canvas = Image.new("L", (16, 16), 0)
    canvas.paste(glyph, ((16 - glyph.width) // 2, (16 - glyph.height) // 2))
    pixels = canvas.load()
    result: list[int] = []
    for y in range(16):
        bits = 0
        for x in range(16):
            if pixels[x, y] >= 96:
                bits |= 1 << (15 - x)
        result.extend(((bits >> 8) & 0xFF, bits & 0xFF))
    return result


def write_map_label_header(output: Path, codepoints: set[int], font_path: Path, font_index: int) -> None:
    font = ImageFont.truetype(str(font_path), 18, index=font_index)
    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "#include <stdint.h>",
        "",
        "// Generated bitmap subset for names found in the Shanghai OSM PBF.",
        "// The source font file is not distributed with this project.",
        "struct FT02RoadGlyph16 { uint32_t codepoint; uint8_t rows[32]; };",
        "",
        "static const FT02RoadGlyph16 FT02_ROAD_GLYPHS[] PROGMEM = {",
    ]
    for codepoint in sorted(codepoints):
        bitmap = fixed_16_bitmap(font, chr(codepoint))
        data = ",".join(f"0x{value:02X}" for value in bitmap)
        lines.append(f"    {{0x{codepoint:04X}, {{{data}}}}},")
    lines += [
        "};",
        "",
        f"static const size_t FT02_ROAD_GLYPH_COUNT = {len(codepoints)};",
        "",
    ]
    output.write_text("\n".join(lines), encoding="utf-8")


def pack_rows(image: Image.Image) -> tuple[int, int, int, list[int]]:
    width, height = image.size
    bytes_per_row = (width + 7) // 8 if width else 0
    output: list[int] = []
    pixels = image.load()
    for y in range(height):
        for byte_index in range(bytes_per_row):
            value = 0
            for bit in range(8):
                x = byte_index * 8 + bit
                if x < width and pixels[x, y] >= 96:
                    value |= 0x80 >> bit
            output.append(value)
    return width, height, bytes_per_row, output


def write_status_header(output: Path, font_path: Path, font_index: int) -> None:
    characters = sorted(set(" 0123456789-./:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz速度方向海拔距离比例尺未定位米"), key=ord)
    font = ImageFont.truetype(str(font_path), 18, index=font_index)
    records: list[tuple[int, int]] = []
    data: list[int] = []
    for character in characters:
        codepoint = ord(character)
        records.append((codepoint, len(data)))
        bbox = font.getbbox(character, anchor="ls")
        x0, y0, x1, y1 = bbox
        width = max(0, x1 - x0)
        height = max(0, y1 - y0)
        image = Image.new("L", (max(1, width), max(1, height)), 0)
        if width and height:
            draw = ImageDraw.Draw(image)
            draw.text((-x0, -y0), character, font=font, fill=255)
            image = image.crop((0, 0, width, height))
        else:
            image = Image.new("L", (0, 0), 0)
        width, height, bytes_per_row, bitmap = pack_rows(image)
        advance = max(1, round(font.getlength(character)))
        data.extend([
            width & 0xFF,
            height & 0xFF,
            bytes_per_row & 0xFF,
            x0 & 0xFF,
            y0 & 0xFF,
            advance & 0xFF,
            0,
            0,
            *bitmap,
        ])

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        '#include "FT02_FontPackTypes.h"',
        "",
        "// Generated 18 px map footer font. Source font file is not included.",
        "static const FT02FontIndexRecord ft02_map_status_18m_idx[] = {",
    ]
    lines.extend(f"    {{0x{cp:04X}, {offset}}}," for cp, offset in records)
    lines += [
        "};",
        "",
        "static const uint8_t ft02_map_status_18m_bin[] = {",
    ]
    for start in range(0, len(data), 16):
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in data[start:start + 16]) + ",")
    lines += [
        "};",
        "",
        "static const FT02FontPack ft02_map_status_18m = {",
        "    ft02_map_status_18m_idx,",
        f"    {len(records)},",
        "    ft02_map_status_18m_bin,",
        f"    {len(data)}",
        "};",
        "",
    ]
    output.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_pbf", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--font", type=Path, default=Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"))
    parser.add_argument("--font-index", type=int, default=2)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    codepoints = collect_name_codepoints(args.source_pbf)
    write_map_label_header(args.output_dir / "FT02_RoadNameFont.h", codepoints, args.font, args.font_index)
    write_status_header(args.output_dir / "FT02_MapStatusFontData.h", args.font, args.font_index)
    print(f"generated {len(codepoints)} map-label glyphs")


if __name__ == "__main__":
    main()
