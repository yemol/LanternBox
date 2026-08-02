#!/usr/bin/env python3
"""Validate the FT-02 field-manual source before building an SD-card pack."""

from __future__ import annotations

import re
import sys
import unicodedata
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as exc:
    raise SystemExit("缺少 PyYAML。请先运行：python3 -m pip install pyyaml") from exc

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source"
ID_RE = re.compile(r"^[A-Z]{3}-[0-9]{3}$")
PRIORITIES = {"critical", "high", "normal"}
ACTION_LIST_FIELDS = (
    "immediate_actions",
    "steps",
    "stop_or_escalate",
    "do_not",
    "completion_check",
)


def load_yaml(path: Path) -> Any:
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def nonempty_text(value: Any) -> bool:
    return isinstance(value, str) and len(value.strip()) >= 2


def display_cells(value: str) -> int:
    """Conservative width estimate: CJK/full-width=2, ASCII/narrow=1."""
    total = 0
    for char in value.strip():
        total += 2 if unicodedata.east_asian_width(char) in {"W", "F", "A"} else 1
    return total


def validate() -> tuple[list[dict[str, Any]], list[dict[str, Any]], dict[str, Any]]:
    manifest = load_yaml(SOURCE / "manifest.yaml")
    categories = load_yaml(SOURCE / "categories.yaml").get("categories", [])
    cards = load_yaml(SOURCE / "phase1_cards.yaml").get("cards", [])
    errors: list[str] = []

    category_ids: set[str] = set()
    category_orders: set[int] = set()
    for index, category in enumerate(categories, 1):
        cid = category.get("id")
        if not isinstance(cid, str) or not re.fullmatch(r"[A-Z]{3}", cid):
            errors.append(f"category {index}: invalid id {cid!r}")
        elif cid in category_ids:
            errors.append(f"duplicate category id: {cid}")
        else:
            category_ids.add(cid)

        order = category.get("order")
        if not isinstance(order, int) or order < 1:
            errors.append(f"{cid}: invalid order {order!r}")
        elif order in category_orders:
            errors.append(f"duplicate category order: {order}")
        else:
            category_orders.add(order)

        if not nonempty_text(category.get("name")):
            errors.append(f"{cid}: missing category name")
        if not nonempty_text(category.get("summary")):
            errors.append(f"{cid}: missing category summary")
        elif "\n" in category["summary"]:
            errors.append(f"{cid}: category summary must be one line")
        elif display_cells(category["summary"]) > 24:
            errors.append(
                f"{cid}: category summary exceeds one-line Card limit "
                f"({display_cells(category['summary'])}/24 display cells): "
                f"{category['summary']}"
            )

    source_registry = manifest.get("source_registry", {})
    seen_ids: set[str] = set()
    quick_count = 0

    for index, card in enumerate(cards, 1):
        card_id = card.get("id")
        label = card_id or f"card {index}"

        if not isinstance(card_id, str) or not ID_RE.fullmatch(card_id):
            errors.append(f"card {index}: invalid id {card_id!r}")
        elif card_id in seen_ids:
            errors.append(f"duplicate card id: {card_id}")
        else:
            seen_ids.add(card_id)

        if card.get("category") not in category_ids:
            errors.append(f"{label}: unknown category {card.get('category')!r}")
        if card.get("priority") not in PRIORITIES:
            errors.append(f"{label}: invalid priority {card.get('priority')!r}")
        if not nonempty_text(card.get("title")):
            errors.append(f"{label}: missing title")
        if not nonempty_text(card.get("trigger")):
            errors.append(f"{label}: missing trigger")

        if card.get("quick_access") is True:
            quick_count += 1
        elif not isinstance(card.get("quick_access", False), bool):
            errors.append(f"{label}: quick_access must be boolean")

        for field in ACTION_LIST_FIELDS:
            value = card.get(field)
            if not isinstance(value, list) or not value:
                errors.append(f"{label}: {field} must be a non-empty list")
                continue
            for line_number, line in enumerate(value, 1):
                if not nonempty_text(line):
                    errors.append(f"{label}: {field}[{line_number}] is too short")

        alternatives = card.get("alternatives", [])
        if alternatives is not None and not isinstance(alternatives, list):
            errors.append(f"{label}: alternatives must be a list")

        refs = card.get("source_refs")
        if not isinstance(refs, list) or not refs:
            refs = []
        else:
            for ref in refs:
                if ref not in source_registry:
                    errors.append(f"{label}: unknown source ref {ref}")

        review = card.get("review")
        if not isinstance(review, dict):
            errors.append(f"{label}: missing review block")
        else:
            status = review.get("status")
            if status not in {"draft", "engineering_draft", "field_test_required", "source_reviewed", "approved"}:
                errors.append(f"{label}: invalid review status {status!r}")
            if status in {"source_reviewed", "approved"} and not refs:
                errors.append(f"{label}: reviewed card must have source_refs")
            if not nonempty_text(review.get("last_reviewed")):
                errors.append(f"{label}: missing last_reviewed")

    expected_count = manifest.get("phase1_card_count")
    if expected_count != len(cards):
        errors.append(
            f"manifest phase1_card_count={expected_count!r}, actual={len(cards)}"
        )

    if errors:
        print("VALIDATION FAILED")
        for error in errors:
            print(f"- {error}")
        raise SystemExit(1)

    print(
        f"PASS: {len(cards)} cards, {len(categories)} categories, "
        f"{quick_count} quick-access cards, IDs and action sections valid."
    )
    return manifest, categories, cards


if __name__ == "__main__":
    validate()
