#!/usr/bin/env python3
"""Validate FT-02 map.cfg and every FTM1 tile in a map package."""
from __future__ import annotations
import argparse
import struct
from pathlib import Path

HEADER = struct.Struct('<4sHHHBBI')
EXPECTED_SIZE = 16 + 8192


def parse_cfg(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    lines = path.read_text(encoding='utf-8').splitlines()
    if not lines or lines[0].strip() != 'FTMAP1':
        raise ValueError('map.cfg signature must be FTMAP1')
    for line in lines[1:]:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if '=' not in line:
            raise ValueError(f'invalid config line: {line}')
        key, value = line.split('=', 1)
        values[key.strip()] = value.strip()
    return values


def validate_tile(path: Path) -> None:
    data = path.read_bytes()
    if len(data) != EXPECTED_SIZE:
        raise ValueError(f'{path}: expected {EXPECTED_SIZE} bytes, got {len(data)}')
    signature, width, height, stride, flags, reserved, payload = HEADER.unpack(data[:16])
    if signature != b'FTM1':
        raise ValueError(f'{path}: bad signature')
    if (width, height, stride, flags, reserved, payload) != (256, 256, 32, 1, 0, 8192):
        raise ValueError(f'{path}: invalid header values')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('package', type=Path)
    args = parser.parse_args()
    root = args.package.resolve()
    cfg = parse_cfg(root / 'map.cfg')
    zoom = int(cfg['zoom'])
    min_x, max_x = int(cfg['min_x']), int(cfg['max_x'])
    min_y, max_y = int(cfg['min_y']), int(cfg['max_y'])
    missing = []
    count = 0
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            path = root / str(zoom) / str(x) / f'{y}.ftm'
            if not path.exists():
                missing.append(path)
                continue
            validate_tile(path)
            count += 1
    if missing:
        sample = '\n'.join(str(path) for path in missing[:10])
        raise SystemExit(f'Missing {len(missing)} tiles:\n{sample}')
    print(f'OK: {count} FTM1 tiles, zoom={zoom}, x={min_x}..{max_x}, y={min_y}..{max_y}')


if __name__ == '__main__':
    main()
