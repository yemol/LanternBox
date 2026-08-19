#!/usr/bin/env python3
"""Build one FT-02 FTPBI1 persistent PBF index on the host.

The generated .pbi is byte-compatible with firmware Persistent Index A1.
This keeps the expensive first-use index build off the ESP32.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from validate_a1_index_format import build


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source_pbf", type=Path)
    ap.add_argument("output_pbi", type=Path)
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    if not args.source_pbf.exists():
        raise SystemExit(f"ERROR: source not found: {args.source_pbf}")
    args.output_pbi.parent.mkdir(parents=True, exist_ok=True)
    report = build(args.source_pbf, args.output_pbi)
    text = json.dumps(report, ensure_ascii=False, indent=2)
    print(text)
    if args.report:
        args.report.write_text(text + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
