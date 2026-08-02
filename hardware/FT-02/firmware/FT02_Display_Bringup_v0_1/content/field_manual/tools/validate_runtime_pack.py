#!/usr/bin/env python3
"""Validate the compiled FT-02 field-manual binary pack."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

MANIFEST = struct.Struct("<8sHHHH16sIIIII")
CATEGORY = struct.Struct("<4s48s144sBHH3x")
CARD = struct.Struct("<12s4s96s192sIIBB2x")


def crc32(path: Path) -> int:
    value = 0
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            value = zlib.crc32(chunk, value)
    return value & 0xFFFFFFFF


def text(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("utf-8")


def validate(root: Path) -> None:
    raw_manifest = (root / "manifest.bin").read_bytes()
    if len(raw_manifest) != MANIFEST.size:
        raise SystemExit(f"manifest.bin size mismatch: {len(raw_manifest)}")
    (
        magic,
        schema,
        category_count,
        card_count,
        quick_count,
        version,
        category_crc,
        card_crc,
        data_crc,
        quick_crc,
        file_format,
    ) = MANIFEST.unpack(raw_manifest)
    if magic != b"FT02FM1\0" or schema != 1 or file_format != 1:
        raise SystemExit("manifest identity/version mismatch")

    paths = {
        "categories": root / "categories.bin",
        "cards": root / "cards.idx",
        "data": root / "cards.dat",
        "quick": root / "quick.idx",
    }
    expected_crc = {
        "categories": category_crc,
        "cards": card_crc,
        "data": data_crc,
        "quick": quick_crc,
    }
    for key, path in paths.items():
        actual = crc32(path)
        if actual != expected_crc[key]:
            raise SystemExit(f"CRC mismatch: {path.name}")

    category_bytes = paths["categories"].read_bytes()
    card_bytes = paths["cards"].read_bytes()
    data = paths["data"].read_bytes()
    quick_bytes = paths["quick"].read_bytes()
    if len(category_bytes) != category_count * CATEGORY.size:
        raise SystemExit("category file size mismatch")
    if len(card_bytes) != card_count * CARD.size:
        raise SystemExit("card index file size mismatch")
    if len(quick_bytes) != quick_count * 2:
        raise SystemExit("quick index file size mismatch")

    categories = [CATEGORY.unpack_from(category_bytes, i * CATEGORY.size) for i in range(category_count)]
    cards = [CARD.unpack_from(card_bytes, i * CARD.size) for i in range(card_count)]
    quick = list(struct.unpack(f"<{quick_count}H", quick_bytes)) if quick_count else []

    for item in categories:
        cid, name, _, _, first, count = item
        if first + count > card_count:
            raise SystemExit(f"invalid category range: {text(cid)} {text(name)}")
    for index, item in enumerate(cards):
        card_id, _, title, _, offset, length, _, _, = item
        if length < 1 or offset + length > len(data):
            raise SystemExit(f"invalid card range: {index} {text(card_id)} {text(title)}")
        record = data[offset : offset + length]
        sections = record[0]
        cursor = 1
        if not 1 <= sections <= 8:
            raise SystemExit(f"invalid section count: {text(card_id)}")
        for _ in range(sections):
            if cursor + 4 > len(record):
                raise SystemExit(f"truncated section header: {text(card_id)}")
            title_len, text_len = struct.unpack_from("<HH", record, cursor)
            cursor += 4
            end = cursor + title_len + text_len
            if title_len < 1 or text_len < 1 or end > len(record):
                raise SystemExit(f"invalid section length: {text(card_id)}")
            if record[cursor + title_len - 1] != 0 or record[end - 1] != 0:
                raise SystemExit(f"missing section terminator: {text(card_id)}")
            cursor = end
        if cursor != len(record):
            raise SystemExit(f"record trailing bytes: {text(card_id)}")
    if any(index >= card_count for index in quick):
        raise SystemExit("quick index out of range")

    print(
        f"PASS: runtime pack version {text(version)}, {category_count} categories, "
        f"{card_count} cards, {quick_count} quick cards, all CRCs and records valid."
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("pack", type=Path)
    args = parser.parse_args()
    validate(args.pack.resolve())


if __name__ == "__main__":
    main()
