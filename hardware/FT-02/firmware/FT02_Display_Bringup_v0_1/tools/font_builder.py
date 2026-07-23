#!/usr/bin/env python3
"""
FT-02 Font Builder v1.0

用途：
  从 TTF/OTF 离线生成 FT-02 可读取的 .idx + .bin 字库包。
  ESP32 不解析 TTF，只读取预生成 bitmap fontpack。

依赖：
  pip install pillow

示例：
  python tools/font_builder.py \
    --font HarmonyOS_Sans_SC_Regular.ttf \
    --size 22 \
    --chars tools/chars_ui.txt \
    --name ft02_ui_22r \
    --threshold 132 \
    --out fontpacks
"""

from pathlib import Path
from PIL import Image, ImageFont, ImageDraw
import argparse
import math
import struct

def render_glyph(ch, size, font_path, threshold=128, scale=4):
    font = ImageFont.truetype(str(font_path), size * scale)

    if ch == " ":
        return {
            "code": ord(ch), "width": 0, "height": 0, "xOffset": 0, "yOffset": 0,
            "xAdvance": max(4, int(round(font.getlength(ch) / scale))),
            "bytesPerRow": 0, "data": b""
        }

    ascent, descent = font.getmetrics()
    pad = 12 * scale
    baseline_y = pad + ascent

    canvas_w = max(size * scale * 3, int(font.getlength(ch)) + pad * 3)
    canvas_h = ascent + descent + pad * 2

    img = Image.new("L", (canvas_w, canvas_h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((pad, baseline_y), ch, font=font, fill=255, anchor="ls")

    bbox = img.getbbox()
    if bbox is None:
        return {
            "code": ord(ch), "width": 0, "height": 0, "xOffset": 0, "yOffset": 0,
            "xAdvance": max(4, int(round(font.getlength(ch) / scale))),
            "bytesPerRow": 0, "data": b""
        }

    left, top, right, bottom = bbox
    left_a = (left // scale) * scale
    top_a = (top // scale) * scale
    right_a = math.ceil(right / scale) * scale
    bottom_a = math.ceil(bottom / scale) * scale

    crop = img.crop((left_a, top_a, right_a, bottom_a))
    out_w = max(1, (right_a - left_a) // scale)
    out_h = max(1, (bottom_a - top_a) // scale)

    small = crop.resize((out_w, out_h), Image.Resampling.LANCZOS)
    bw = small.point(lambda p: 255 if p >= threshold else 0, mode="1")

    bpr = (out_w + 7) // 8
    data = bytearray()

    for y in range(out_h):
        for block in range(bpr):
            value = 0
            for bit in range(8):
                x = block * 8 + bit
                if x < out_w and bw.getpixel((x, y)):
                    value |= (0x80 >> bit)
            data.append(value)

    return {
        "code": ord(ch),
        "width": out_w,
        "height": out_h,
        "xOffset": int(round((left_a - pad) / scale)),
        "yOffset": int(round((top_a - baseline_y) / scale)),
        "xAdvance": max(1, int(round(font.getlength(ch) / scale))),
        "bytesPerRow": bpr,
        "data": bytes(data)
    }

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", required=True)
    ap.add_argument("--size", required=True, type=int)
    ap.add_argument("--chars", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--threshold", type=int, default=128)
    ap.add_argument("--out", default="fontpacks")
    args = ap.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    char_text = Path(args.chars).read_text(encoding="utf-8")
    chars = sorted(set(char_text))

    glyphs = [
        render_glyph(ch, args.size, args.font, threshold=args.threshold)
        for ch in chars
        if ch not in "\r\n\t"
    ]

    glyphs = sorted(glyphs, key=lambda g: g["code"])

    bin_blob = bytearray()
    idx_blob = bytearray()

    for g in glyphs:
        offset = len(bin_blob)
        bitmap = g["data"]

        bin_blob.extend(struct.pack(
            "<BBBbbBH",
            g["width"],
            g["height"],
            g["bytesPerRow"],
            g["xOffset"],
            g["yOffset"],
            g["xAdvance"],
            len(bitmap)
        ))
        bin_blob.extend(bitmap)

        idx_blob.extend(struct.pack("<II", g["code"], offset))

    (out / f"{args.name}.idx").write_bytes(idx_blob)
    (out / f"{args.name}.bin").write_bytes(bin_blob)

    print(f"Wrote {out / (args.name + '.idx')}")
    print(f"Wrote {out / (args.name + '.bin')}")
    print(f"Glyphs: {len(glyphs)}")
    print(f"BIN bytes: {len(bin_blob)}")

if __name__ == "__main__":
    main()
