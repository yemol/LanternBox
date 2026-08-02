#!/usr/bin/env python3
"""Compile editable YAML cards into the compact FT-02 SD-card runtime pack."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import sys
import zlib
from pathlib import Path
from typing import Any, Iterable

try:
    import yaml
except ImportError as exc:
    raise SystemExit("缺少 PyYAML。请先运行：python3 -m pip install pyyaml") from exc

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent
SOURCE = ROOT / "source"
MAGIC = b"FT02FM1\0"
SCHEMA_VERSION = 1
MANIFEST_STRUCT = struct.Struct("<8sHHHH16sIIIII")
CATEGORY_STRUCT = struct.Struct("<4s48s144sBHH3x")
CARD_INDEX_STRUCT = struct.Struct("<12s4s96s192sIIBB2x")
PRIORITY_MAP = {"critical": 0, "high": 1, "normal": 2}
ICON_MAP = {
    "MED": 0,
    "WAT": 1,
    "FOD": 2,
    "FIR": 3,
    "HYG": 0,
    "REP": 4,
    "ENR": 5,
    "NAV": 6,
    "SHL": 7,
    "ANM": 7,
    "ORG": 8,
}


def load_yaml(path: Path) -> Any:
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def fixed_utf8(value: str, size: int, field: str) -> bytes:
    raw = value.encode("utf-8")
    if len(raw) >= size:
        raise ValueError(f"{field} exceeds {size - 1} UTF-8 bytes: {value}")
    return raw + b"\0" * (size - len(raw))


def list_text(lines: Iterable[str]) -> str:
    return "\n".join(f"{index}. {line.strip()}" for index, line in enumerate(lines, 1))


def summary_text(card: dict[str, Any]) -> str:
    trigger = str(card["trigger"]).strip()
    raw = trigger.encode("utf-8")
    if len(raw) < 190:
        return trigger
    # Keep a valid UTF-8 prefix and reserve space for an ellipsis.
    clipped = raw[:186]
    while True:
        try:
            return clipped.decode("utf-8") + "…"
        except UnicodeDecodeError:
            clipped = clipped[:-1]


def card_sections(card: dict[str, Any]) -> list[tuple[str, str]]:
    sections: list[tuple[str, str]] = [
        ("什么时候使用", str(card["trigger"]).strip()),
        ("立即做", list_text(card["immediate_actions"])),
        ("操作步骤", list_text(card["steps"])),
    ]
    if card.get("alternatives"):
        sections.append(("没有标准工具时", list_text(card["alternatives"])))
    sections.extend(
        [
            ("停止或升级处置", list_text(card["stop_or_escalate"])),
            ("禁止", list_text(card["do_not"])),
            ("完成检查", list_text(card["completion_check"])),
        ]
    )
    return sections


def encode_card_record(card: dict[str, Any]) -> bytes:
    sections = card_sections(card)
    if len(sections) > 8:
        raise ValueError(f"{card['id']}: too many sections")
    output = bytearray([len(sections)])
    for title, text in sections:
        title_bytes = title.encode("utf-8") + b"\0"
        text_bytes = text.encode("utf-8") + b"\0"
        if len(title_bytes) > 65535 or len(text_bytes) > 65535:
            raise ValueError(f"{card['id']}: section is too large")
        output.extend(struct.pack("<HH", len(title_bytes), len(text_bytes)))
        output.extend(title_bytes)
        output.extend(text_bytes)
    return bytes(output)


def crc32(path: Path) -> int:
    value = 0
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            value = zlib.crc32(chunk, value)
    return value & 0xFFFFFFFF


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            digest.update(chunk)
    return digest.hexdigest()


def validate_source() -> tuple[dict[str, Any], list[dict[str, Any]], list[dict[str, Any]]]:
    # Reuse the validator without requiring package imports.
    sys.path.insert(0, str(SCRIPT_DIR))
    from validate_cards import validate

    return validate()


def build(output: Path) -> None:
    manifest, categories, cards = validate_source()
    output.mkdir(parents=True, exist_ok=True)
    for child in output.iterdir():
        if child.is_dir():
            shutil.rmtree(child)
        else:
            child.unlink()

    categories = sorted(categories, key=lambda item: item["order"])
    category_order = {category["id"]: index for index, category in enumerate(categories)}
    cards = sorted(cards, key=lambda card: (category_order[card["category"]], card["id"]))

    cards_by_category: dict[str, list[int]] = {category["id"]: [] for category in categories}
    quick_indices: list[int] = []
    card_records: list[bytes] = []
    card_offsets: list[int] = []
    offset = 0

    for global_index, card in enumerate(cards):
        cards_by_category[card["category"]].append(global_index)
        if card.get("quick_access", False):
            quick_indices.append(global_index)
        record = encode_card_record(card)
        card_offsets.append(offset)
        card_records.append(record)
        offset += len(record)

    categories_path = output / "categories.bin"
    with categories_path.open("wb") as handle:
        for category in categories:
            indices = cards_by_category[category["id"]]
            first_card = indices[0] if indices else 0
            handle.write(
                CATEGORY_STRUCT.pack(
                    fixed_utf8(category["id"], 4, "category.id"),
                    fixed_utf8(category["name"], 48, "category.name"),
                    fixed_utf8(category["summary"], 144, "category.summary"),
                    ICON_MAP.get(category["id"], 8),
                    first_card,
                    len(indices),
                )
            )

    data_path = output / "cards.dat"
    with data_path.open("wb") as handle:
        for record in card_records:
            handle.write(record)

    index_path = output / "cards.idx"
    with index_path.open("wb") as handle:
        for global_index, card in enumerate(cards):
            handle.write(
                CARD_INDEX_STRUCT.pack(
                    fixed_utf8(card["id"], 12, "card.id"),
                    fixed_utf8(card["category"], 4, "card.category"),
                    fixed_utf8(card["title"], 96, "card.title"),
                    fixed_utf8(summary_text(card), 192, "card.summary"),
                    card_offsets[global_index],
                    len(card_records[global_index]),
                    1 if card.get("quick_access", False) else 0,
                    PRIORITY_MAP[card["priority"]],
                )
            )

    quick_path = output / "quick.idx"
    with quick_path.open("wb") as handle:
        for index in quick_indices:
            handle.write(struct.pack("<H", index))

    search_path = output / "search.idx"
    with search_path.open("w", encoding="utf-8", newline="\n") as handle:
        for global_index, card in enumerate(cards):
            terms = [card["id"], card["title"], card["category"]]
            terms.extend(card.get("keywords", []))
            handle.write(f"{global_index}\t{'|'.join(terms)}\n")

    sources_path = output / "sources.json"
    sources_path.write_text(
        json.dumps(manifest.get("source_registry", {}), ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    runtime_manifest = {
        "pack_id": manifest["pack_id"],
        "title": manifest["title"],
        "language": manifest["language"],
        "schema_version": SCHEMA_VERSION,
        "content_version": manifest["content_version"],
        "category_count": len(categories),
        "card_count": len(cards),
        "quick_count": len(quick_indices),
        "runtime_files": [
            "manifest.bin",
            "categories.bin",
            "cards.idx",
            "cards.dat",
            "quick.idx",
        ],
        "optional_files": ["search.idx", "sources.json", "checksums.sha256"],
    }
    (output / "manifest.json").write_text(
        json.dumps(runtime_manifest, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    version = fixed_utf8(str(manifest["content_version"]), 16, "content_version")
    manifest_bin = MANIFEST_STRUCT.pack(
        MAGIC,
        SCHEMA_VERSION,
        len(categories),
        len(cards),
        len(quick_indices),
        version,
        crc32(categories_path),
        crc32(index_path),
        crc32(data_path),
        crc32(quick_path),
        1,
    )
    (output / "manifest.bin").write_bytes(manifest_bin)

    checksum_files = [
        "manifest.bin",
        "manifest.json",
        "categories.bin",
        "cards.idx",
        "cards.dat",
        "quick.idx",
        "search.idx",
        "sources.json",
    ]
    (output / "checksums.sha256").write_text(
        "".join(f"{sha256(output / name)}  {name}\n" for name in checksum_files),
        encoding="utf-8",
    )

    print(
        f"BUILT: {len(cards)} cards, {len(categories)} categories, "
        f"{len(quick_indices)} quick cards -> {output}"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT.parents[1] / "SD_CARD_COPY" / "knowledge" / "field_manual",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    build(args.output.resolve())
