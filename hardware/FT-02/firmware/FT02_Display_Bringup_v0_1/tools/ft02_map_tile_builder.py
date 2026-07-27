#!/usr/bin/env python3
"""
Convert a pre-rendered map image into FT-02 FTM1 monochrome tiles.

Input requirements:
- Any image format supported by Pillow.
- The image is treated as an already-rendered map canvas.
- The script does not download map data and does not perform geographic
  projection. Generate the source canvas on the Core/Mac side first.

Output:
  <output>/map.cfg
  <output>/<zoom>/<x>/<y>.ftm

Install:
  python3 -m pip install pillow
"""

from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit(
        "Pillow is required. Install with: python3 -m pip install pillow"
    ) from exc

TILE_SIZE = 256
STRIDE = TILE_SIZE // 8
HEADER = struct.Struct("<4sHHHBBI")


def write_ftm(path: Path, image: Image.Image, threshold: int) -> None:
    gray = image.convert("L")
    payload = bytearray(TILE_SIZE * STRIDE)
    pixels = gray.load()

    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            if pixels[x, y] < threshold:
                payload[y * STRIDE + x // 8] |= 0x80 >> (x % 8)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        HEADER.pack(
            b"FTM1",
            TILE_SIZE,
            TILE_SIZE,
            STRIDE,
            1,
            0,
            len(payload),
        )
        + payload
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--zoom", type=int, default=0)
    parser.add_argument("--origin-x", type=int, default=0)
    parser.add_argument("--origin-y", type=int, default=0)
    parser.add_argument("--start-x", type=int)
    parser.add_argument("--start-y", type=int)
    parser.add_argument("--threshold", type=int, default=160)
    parser.add_argument("--name", default="FT-02 Offline Map")
    parser.add_argument(
        "--attribution",
        default="(c) OpenStreetMap contributors",
    )
    args = parser.parse_args()

    if not 1 <= args.threshold <= 254:
        raise SystemExit("--threshold must be between 1 and 254")

    image = Image.open(args.input).convert("L")

    tile_columns = math.ceil(image.width / TILE_SIZE)
    tile_rows = math.ceil(image.height / TILE_SIZE)

    padded = Image.new(
        "L",
        (tile_columns * TILE_SIZE, tile_rows * TILE_SIZE),
        255,
    )
    padded.paste(image, (0, 0))

    min_x = args.origin_x
    min_y = args.origin_y
    max_x = min_x + tile_columns - 1
    max_y = min_y + tile_rows - 1

    start_x = args.start_x if args.start_x is not None else min_x
    start_y = args.start_y if args.start_y is not None else min_y

    if not min_x <= start_x <= max_x:
        raise SystemExit("--start-x is outside the generated tile bounds")
    if not min_y <= start_y <= max_y:
        raise SystemExit("--start-y is outside the generated tile bounds")

    output = args.output
    output.mkdir(parents=True, exist_ok=True)

    for row in range(tile_rows):
        for column in range(tile_columns):
            tile = padded.crop(
                (
                    column * TILE_SIZE,
                    row * TILE_SIZE,
                    (column + 1) * TILE_SIZE,
                    (row + 1) * TILE_SIZE,
                )
            )

            x = min_x + column
            y = min_y + row

            write_ftm(
                output / str(args.zoom) / str(x) / f"{y}.ftm",
                tile,
                args.threshold,
            )

    config = "\n".join(
        [
            "FTMAP1",
            f"name={args.name}",
            f"attribution={args.attribution}",
            f"zoom={args.zoom}",
            f"min_x={min_x}",
            f"max_x={max_x}",
            f"min_y={min_y}",
            f"max_y={max_y}",
            f"start_x={start_x}",
            f"start_y={start_y}",
            "",
        ]
    )

    (output / "map.cfg").write_text(config, encoding="utf-8")

    print(
        f"Generated {tile_columns * tile_rows} tiles "
        f"({tile_columns} x {tile_rows}) in {output}"
    )


if __name__ == "__main__":
    main()
