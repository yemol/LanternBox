#!/usr/bin/env python3
"""Validate an FT-02 FTPBC5 / A3.3 regional cache copied from the SD card."""
from __future__ import annotations

import argparse
import json
import struct
import zlib
from pathlib import Path

HEADER = struct.Struct("<8sHHHHQIIIiiiiiiIII56s")
SEGMENT = struct.Struct("<iiiiB3s")
LABEL = struct.Struct("<iiBB48s6s")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("cache", type=Path)
    args = parser.parse_args()

    data = args.cache.read_bytes()
    if len(data) < HEADER.size:
        raise SystemExit("cache too small")

    header = HEADER.unpack_from(data, 0)
    (
        magic,
        version,
        header_bytes,
        segment_bytes,
        label_bytes,
        source_bytes,
        source_signature,
        segment_count,
        label_count,
        min_lat,
        min_lon,
        max_lat,
        max_lon,
        center_lat,
        center_lon,
        build_ms,
        payload_crc,
        file_bytes,
        _,
    ) = header

    errors: list[str] = []
    if magic[:6] != b"FTPBC5":
        errors.append("bad magic")
    if version != 4:
        errors.append("bad version")
    if header_bytes != HEADER.size:
        errors.append("bad header size")
    if segment_bytes != SEGMENT.size:
        errors.append("bad segment size")
    if label_bytes != LABEL.size:
        errors.append("bad label size")

    expected = HEADER.size + segment_count * SEGMENT.size + label_count * LABEL.size
    if expected != len(data):
        errors.append(f"length mismatch expected={expected} actual={len(data)}")
    if file_bytes != len(data):
        errors.append(f"header file_bytes mismatch {file_bytes}")

    payload = data[HEADER.size:]
    crc = zlib.crc32(payload) & 0xFFFFFFFF
    if crc != payload_crc:
        errors.append(f"crc mismatch expected=0x{payload_crc:08X} actual=0x{crc:08X}")

    label_offset = HEADER.size + segment_count * SEGMENT.size
    labels = []
    priorities = {1: 0, 2: 0, 3: 0, 4: 0}
    for index in range(label_count):
        lat, lon, priority, text_bytes, text, _ = LABEL.unpack_from(
            data, label_offset + index * LABEL.size
        )
        priorities[priority] = priorities.get(priority, 0) + 1
        labels.append(
            {
                "lat": lat / 1e7,
                "lon": lon / 1e7,
                "priority": priority,
                "text": text[:text_bytes].decode("utf-8", "replace"),
            }
        )

    report = {
        "ok": not errors,
        "errors": errors,
        "cache_bytes": len(data),
        "source_bytes": source_bytes,
        "source_signature": f"0x{source_signature:08X}",
        "segments": segment_count,
        "labels": label_count,
        "priorities": priorities,
        "bounds": {
            "min_lat": min_lat / 1e7,
            "min_lon": min_lon / 1e7,
            "max_lat": max_lat / 1e7,
            "max_lon": max_lon / 1e7,
        },
        "center": {"lat": center_lat / 1e7, "lon": center_lon / 1e7},
        "device_build_ms": build_ms,
        "sample_labels": labels[:20],
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())
