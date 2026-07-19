#!/usr/bin/env python3
"""Reset LanternBox Core terminal sync data for a clean FT-01 sync test.

This script intentionally clears only Core-side terminal sync artifacts and
terminal-generated journal entries. It preserves inventory, wiki data, tasks,
runtime settings, and terminal device registration/trust records.

Default mode is dry-run. Use --apply to make changes. This test reset script does not create backups.
"""

from __future__ import annotations

import argparse
import shutil
import sqlite3
from datetime import datetime
from pathlib import Path
from typing import Any

TERMINAL_ENTRY_TYPES = (
    "终端现场日志",
    "终端录音日志",
    "终端轨迹记录",
)


def _connect(db_path: Path) -> sqlite3.Connection:
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    return conn


def _table_exists(conn: sqlite3.Connection, name: str) -> bool:
    row = conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name=?",
        (name,),
    ).fetchone()
    return row is not None


def _column_exists(conn: sqlite3.Connection, table: str, column: str) -> bool:
    rows = conn.execute(f"PRAGMA table_info({table})").fetchall()
    return any(row["name"] == column for row in rows)


def _build_journal_where(conn: sqlite3.Connection, device_id: str) -> tuple[str, list[Any]]:
    clauses = [
        "entry_type IN (?, ?, ?)",
        "title LIKE '终端现场%'",
        "title LIKE '终端录音%'",
        "title LIKE '终端轨迹%'",
    ]
    params: list[Any] = list(TERMINAL_ENTRY_TYPES)

    if device_id:
        device_clauses = [
            "content LIKE ?",
            "title LIKE ?",
        ]
        params.extend([f"%{device_id}%", f"%{device_id}%"])
        if _column_exists(conn, "journal", "metadata_json"):
            device_clauses.append("metadata_json LIKE ?")
            params.append(f"%{device_id}%")
        clauses.append("(" + " OR ".join(device_clauses) + ")")

    # Keep this broad enough to clean older journal rows created before
    # metadata_json existed, but narrow enough to avoid normal handwritten notes.
    return "(" + " OR ".join(clauses[:4]) + ")" + (" AND " + clauses[4] if len(clauses) > 4 else ""), params


def _preview_journal(conn: sqlite3.Connection, device_id: str, limit: int) -> tuple[int, list[sqlite3.Row]]:
    where, params = _build_journal_where(conn, device_id)
    total = conn.execute(f"SELECT COUNT(*) AS count FROM journal WHERE {where}", params).fetchone()["count"]
    rows = conn.execute(
        f"""
        SELECT id, entry_type, title, created_at
        FROM journal
        WHERE {where}
        ORDER BY id DESC
        LIMIT ?
        """,
        [*params, limit],
    ).fetchall()
    return int(total), rows


def run(args: argparse.Namespace) -> int:
    project_root = Path(args.project_root).resolve()
    db_path = project_root / "data" / "lanternbox.db"
    sync_root = project_root / "data" / "terminal_sync"

    if not db_path.exists():
        raise SystemExit(f"database not found: {db_path}")

    conn = _connect(db_path)
    try:
        if not _table_exists(conn, "journal"):
            raise SystemExit("journal table not found in data/lanternbox.db")
        journal_count, preview_rows = _preview_journal(conn, args.device_id, args.preview_limit)
    finally:
        conn.close()

    print("LanternBox terminal sync reset")
    print(f"project_root: {project_root}")
    print(f"device_id: {args.device_id or '(all terminal devices)'}")
    print(f"mode: {'APPLY' if args.apply else 'DRY-RUN'}")
    print()
    print("Will clear Core-side terminal sync archive:")
    print(f"  {sync_root}")
    print()
    print(f"Will delete terminal-generated journal rows: {journal_count}")
    for row in preview_rows:
        print(f"  #{row['id']} [{row['entry_type']}] {row['title']} | {row['created_at']}")

    print()
    print("Will preserve:")
    print("  terminal device registration and trust status")
    print("  inventory / wiki / tasks / AI settings / user-created normal journal rows")

    if not args.apply:
        print()
        print("Dry-run only. Re-run with --apply to reset.")
        return 0

    if not args.yes:
        confirmation = input("Type RESET to continue: ").strip()
        if confirmation != "RESET":
            print("Aborted.")
            return 1

    conn = _connect(db_path)
    try:
        where, params = _build_journal_where(conn, args.device_id)
        cursor = conn.execute(f"DELETE FROM journal WHERE {where}", params)
        deleted = int(cursor.rowcount or 0)
        conn.commit()
    finally:
        conn.close()

    if sync_root.exists():
        shutil.rmtree(sync_root)
    (sync_root / "manifests").mkdir(parents=True, exist_ok=True)
    (sync_root / "archive").mkdir(parents=True, exist_ok=True)

    print()
    print("Reset complete.")
    print(f"Deleted terminal journal rows: {deleted}")
    print("Core terminal sync archive is now empty and ready for a clean FT-01 sync.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Reset Core-side terminal sync data for clean testing")
    parser.add_argument("--project-root", default=".", help="LanternBox project root, default current directory")
    parser.add_argument("--device-id", default="FT01-0001", help="device id to reset journal rows for; empty string means all terminal devices")
    parser.add_argument("--preview-limit", type=int, default=30)
    parser.add_argument("--apply", action="store_true", help="actually reset data; without this it only previews")
    parser.add_argument("--yes", action="store_true", help="skip interactive RESET confirmation when using --apply")
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
