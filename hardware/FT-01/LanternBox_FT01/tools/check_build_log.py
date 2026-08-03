#!/usr/bin/env python3
"""Validate the final Arduino 'Sketch uses ... Maximum is ...' output."""
from __future__ import annotations
import re
import sys
from pathlib import Path

DEFAULT_BUDGET_RATIO = 0.80
MIN_REQUIRED_MAX = 1_322_806


def main() -> int:
    text = Path(sys.argv[1]).read_text(errors="replace") if len(sys.argv) > 1 else sys.stdin.read()
    match = re.search(r"Sketch uses\s+(\d+)\s+bytes.*?Maximum is\s+(\d+)\s+bytes", text, re.S)
    if not match:
        print("FAIL: Arduino size summary not found.")
        return 2
    used, maximum = map(int, match.groups())
    remaining = maximum - used
    ratio = used / maximum if maximum else 1.0
    print(f"used={used}")
    print(f"maximum={maximum}")
    print(f"remaining={remaining}")
    print(f"occupancy={ratio:.2%}")
    if maximum <= 1_310_720:
        print("FAIL: default 1.25 MiB application profile is still selected.")
        return 3
    if maximum < MIN_REQUIRED_MAX:
        print("FAIL: selected application partition cannot hold this firmware.")
        return 4
    if used > maximum:
        print("FAIL: firmware exceeds application partition.")
        return 5
    if ratio > DEFAULT_BUDGET_RATIO:
        print("FAIL: firmware exceeds the 80% release budget.")
        return 6
    print("PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
