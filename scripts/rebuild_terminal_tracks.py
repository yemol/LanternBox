#!/usr/bin/env python3
"""Rebuild clean terminal track journal entries from Terminal Sync archive.

This script only rebuilds derived Journal terminal_track entries. It never
modifies terminal_sync/archive, seen_ids.json, sync_log.jsonl, audio files, or
FT-01 data.

Usage:
  python3 scripts/rebuild_terminal_tracks.py --dry-run
  python3 scripts/rebuild_terminal_tracks.py --apply
  python3 scripts/rebuild_terminal_tracks.py --apply --device-id FT01-0001
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from api.services.terminal_sync_service import rebuild_terminal_tracks_from_archive


def main() -> int:
    parser = argparse.ArgumentParser(description="Rebuild terminal track journal entries from archive")
    parser.add_argument("--device-id", default=None, help="Limit rebuild to one terminal device id")
    parser.add_argument("--apply", action="store_true", help="Actually rebuild journal entries")
    parser.add_argument("--dry-run", action="store_true", help="Preview mode, does not write to journal")
    args = parser.parse_args()

    if args.apply and args.dry_run:
        parser.error("--apply and --dry-run cannot be used together")

    apply_changes = bool(args.apply)
    if not apply_changes:
        print("Terminal Track Rebuild DRY RUN")
        print("No journal rows will be changed. Use --apply to rebuild.")
        replace_existing = False
    else:
        print("Terminal Track Rebuild APPLY")
        replace_existing = True

    result = rebuild_terminal_tracks_from_archive(
        device_id=args.device_id,
        replace_existing=replace_existing,
        apply_changes=apply_changes,
    )

    print(json.dumps(result, ensure_ascii=False, indent=2))
    if not apply_changes:
        print("\nDry run note: existing journal entries were not deleted or upserted.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
