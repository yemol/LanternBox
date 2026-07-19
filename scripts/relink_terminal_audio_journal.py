#!/usr/bin/env python3
"""Backfill persistent Journal links for already uploaded terminal WAV files.

This script reads Core's terminal audio archive index and writes the matching
stored_path / sha256 / play_url metadata into existing 终端录音日志 rows. It does
not delete archive files, seen_ids, sync logs, or terminal SD data.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import json

from api.config import DATA_DIR
from api.services.journal_service import extract_filename, link_terminal_audio_to_journal
from api.services.terminal_audio_service import iter_audio_records


def _read_jsonl_records(path: Path) -> list[dict]:
    if not path.exists():
        return []
    records = []
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        try:
            value = json.loads(stripped)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict):
            records.append(value)
    return records


def _audio_id_from_index_record(record: dict, device_id: str) -> str:
    explicit = str(record.get("audio_id") or "").strip()
    if explicit:
        return explicit
    session_id = str(record.get("session_id") or record.get("session") or "").strip()
    filename = extract_filename(record.get("filename") or record.get("file") or record.get("path"))
    size = str(record.get("size") or record.get("bytes") or record.get("file_size") or "").strip()
    if device_id and session_id and filename and size:
        return f"{device_id}:audio:{session_id}:{filename}:{size}"
    return ""


def _load_audio_index_metadata(device_id: str) -> dict[str, dict]:
    archive_dir = DATA_DIR / "terminal_sync" / "archive" / device_id
    records = _read_jsonl_records(archive_dir / "audio_index.jsonl")
    by_audio_id: dict[str, dict] = {}
    by_filename: dict[str, dict] = {}
    for record in records:
        audio_id = _audio_id_from_index_record(record, device_id)
        filename = extract_filename(record.get("filename") or record.get("file") or record.get("path"))
        duration = record.get("duration")
        if duration in (None, ""):
            duration = record.get("duration_sec", record.get("duration_seconds", record.get("seconds", "")))
        normalized = dict(record)
        normalized["duration"] = duration
        normalized["filename"] = filename
        if audio_id:
            by_audio_id[audio_id] = normalized
        if filename:
            by_filename[filename] = normalized
    return {"by_audio_id": by_audio_id, "by_filename": by_filename}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Relink terminal WAV archives to Journal audio rows")
    parser.add_argument("--device-id", default="", help="optional device id, e.g. FT01-0001")
    parser.add_argument("--dry-run", action="store_true", help="show what would be processed without updating Journal")
    args = parser.parse_args(argv)

    records = iter_audio_records(device_id=args.device_id or None, include_legacy=True)
    metadata_cache: dict[str, dict[str, dict]] = {}
    processed = 0
    updated = 0
    matched = 0

    for record in records:
        audio_id = str(record.get("audio_id") or "").strip()
        device_id = str(record.get("device_id") or args.device_id or "").strip()
        filename = str(record.get("filename") or record.get("file") or record.get("path") or "").strip()
        stored_path = str(record.get("stored_path") or record.get("path") or "").strip()
        if not audio_id or not device_id or not stored_path:
            continue
        processed += 1
        if args.dry_run:
            print(f"would relink audio_id={audio_id} device_id={device_id} filename={filename}")
            continue
        if device_id not in metadata_cache:
            metadata_cache[device_id] = _load_audio_index_metadata(device_id)
        index_metadata = (
            metadata_cache[device_id]["by_audio_id"].get(audio_id)
            or metadata_cache[device_id]["by_filename"].get(extract_filename(filename))
            or {}
        )
        duration = index_metadata.get("duration")
        result = link_terminal_audio_to_journal(
            device_id=device_id,
            audio_id=audio_id,
            filename=filename,
            stored_path=stored_path,
            size=record.get("size"),
            duration=duration,
            sha256=str(record.get("sha256") or ""),
            received_at=str(record.get("received_at") or ""),
            sync_session_id=str(record.get("sync_session_id") or ""),
            stored_filename=str(record.get("stored_filename") or ""),
        )
        matched += int(result.get("matched") or 0)
        updated += int(result.get("updated") or 0)
        print(
            f"relinked audio_id={audio_id} matched={result.get('matched')} "
            f"updated={result.get('updated')}"
        )

    print({
        "ok": True,
        "dry_run": args.dry_run,
        "processed_audio_records": processed,
        "matched_journal_rows": matched,
        "updated_journal_rows": updated,
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
