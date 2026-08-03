#!/usr/bin/env python3
"""FT-01 firmware size gate.

Usage:
  python3 tools/check_size.py compile.log
  echo 'Sketch uses 1322806 bytes ...' | python3 tools/check_size.py -
  python3 tools/check_size.py --bytes 1322806
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

APP_PARTITION_BYTES = 0x330000  # 3,342,336 bytes
RELEASE_BUDGET_PERCENT = 80.0
RELEASE_BUDGET_BYTES = int(APP_PARTITION_BYTES * RELEASE_BUDGET_PERCENT / 100.0)
PATTERN = re.compile(r"Sketch uses\s+([0-9,]+)\s+bytes", re.IGNORECASE)


def read_text(source: str) -> str:
    if source == "-":
        return sys.stdin.read()
    return Path(source).read_text(encoding="utf-8", errors="replace")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check LanternBox FT-01 application size")
    parser.add_argument("source", nargs="?", help="Arduino compile log path, or '-' for stdin")
    parser.add_argument("--bytes", type=int, dest="size_bytes", help="Application size in bytes")
    args = parser.parse_args()

    size = args.size_bytes
    if size is None:
        if not args.source:
            parser.error("provide a compile log, '-' for stdin, or --bytes")
        match = PATTERN.search(read_text(args.source))
        if not match:
            print("FAIL: no 'Sketch uses ... bytes' line found", file=sys.stderr)
            return 2
        size = int(match.group(1).replace(",", ""))

    usage = size * 100.0 / APP_PARTITION_BYTES
    remaining = APP_PARTITION_BYTES - size
    print(f"FT-01 app size      : {size:,} bytes")
    print(f"App partition       : {APP_PARTITION_BYTES:,} bytes (0x330000)")
    print(f"Usage               : {usage:.2f}%")
    print(f"Remaining           : {remaining:,} bytes")
    print(f"Release budget      : {RELEASE_BUDGET_BYTES:,} bytes ({RELEASE_BUDGET_PERCENT:.0f}%)")

    if size > APP_PARTITION_BYTES:
        print("FAIL: application exceeds the physical app partition", file=sys.stderr)
        return 1
    if size > RELEASE_BUDGET_BYTES:
        print("FAIL: application fits, but exceeds the FT-01 release budget", file=sys.stderr)
        return 1

    print("PASS: size gate satisfied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
