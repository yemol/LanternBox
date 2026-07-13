"""Terminal sync manifest and JSONL archive service.

This module stores terminal sync metadata and records on Core. It does not
implement transport, encryption, audio upload, or FT-01 firmware behavior.
"""

import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from ..config import DATA_DIR
from ..models import TerminalSyncManifestRequest, TerminalSyncUploadRecordsRequest
from .terminal_service import get_terminal_device, update_terminal_last_seen


SYNC_ROOT = DATA_DIR / "terminal_sync"
MANIFEST_DIR = SYNC_ROOT / "manifests"
ARCHIVE_DIR = SYNC_ROOT / "archive"
SYNC_LOG_PATH = SYNC_ROOT / "sync_log.jsonl"

ALLOWED_RECORD_TYPES = {"path_points", "field_events", "boot_logs", "audio_index"}
RECORD_ARCHIVE_FILES = {
    "path_points": "path_points.jsonl",
    "field_events": "field_events.jsonl",
    "boot_logs": "boot_logs.jsonl",
    "audio_index": "audio_index.jsonl",
}


class TerminalSyncDeviceNotFoundError(Exception):
    pass


class TerminalSyncDeviceNotTrustedError(Exception):
    pass


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _safe_name(value: str, fallback: str = "unknown") -> str:
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", str(value or "").strip())
    safe = safe.strip("._")
    return safe or fallback


def _ensure_sync_dirs() -> None:
    MANIFEST_DIR.mkdir(parents=True, exist_ok=True)
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    SYNC_ROOT.mkdir(parents=True, exist_ok=True)


def _json_dumps(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def _write_json_file_atomic(path: Path, payload: dict[str, Any]) -> None:
    tmp_path = path.with_suffix(f"{path.suffix}.tmp")
    tmp_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp_path.replace(path)


def _append_jsonl(path: Path, payload: dict[str, Any]) -> None:
    with path.open("a", encoding="utf-8") as file:
        file.write(_json_dumps(payload))
        file.write("\n")


def _load_seen_ids(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"seen_ids": {}}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {"seen_ids": {}}
    if not isinstance(data, dict):
        return {"seen_ids": {}}
    seen_ids = data.get("seen_ids")
    if not isinstance(seen_ids, dict):
        data["seen_ids"] = {}
    return data


def _get_archive_dir(device_id: str) -> Path:
    return ARCHIVE_DIR / _safe_name(device_id)


def _check_trusted_device(device_id: str) -> None:
    device = get_terminal_device(device_id)
    if device is None:
        raise TerminalSyncDeviceNotFoundError(device_id)
    if not device.trusted:
        raise TerminalSyncDeviceNotTrustedError(device_id)


def _record_text(record: dict[str, Any], key: str) -> str:
    return str(record.get(key) or "").strip()


def _generate_record_id(record_type: str, payload_device_id: str, record: dict[str, Any]) -> str:
    record_device_id = _record_text(record, "device_id") or payload_device_id

    if record_type == "path_points":
        session_id = _record_text(record, "session_id")
        seq = str(record.get("seq") if record.get("seq") is not None else "").strip()
        return f"{record_device_id}:path_point:{session_id}:{seq}"

    if record_type == "field_events":
        sync_id = _record_text(record, "sync_id")
        if sync_id:
            return sync_id
        session_id = _record_text(record, "session_id")
        device_date = _record_text(record, "device_date")
        device_time = _record_text(record, "device_time")
        note = str(record.get("note") or "")
        note_hash = hashlib.sha1(note.encode("utf-8")).hexdigest()[:12]
        return f"{record_device_id}:field_event:{session_id}:{device_date}:{device_time}:{note_hash}"

    if record_type == "boot_logs":
        sync_id = _record_text(record, "sync_id")
        if sync_id:
            return sync_id
        device_date = _record_text(record, "device_date")
        device_time = _record_text(record, "device_time")
        firmware_version = _record_text(record, "firmware_version") or _record_text(record, "version")
        return f"{record_device_id}:boot:{device_date}:{device_time}:{firmware_version}"

    if record_type == "audio_index":
        audio_id = _record_text(record, "audio_id")
        if audio_id:
            return audio_id
        session_id = _record_text(record, "session_id")
        filename = _record_text(record, "filename") or _record_text(record, "file")
        size = str(record.get("size") if record.get("size") is not None else "").strip()
        return f"{record_device_id}:audio:{session_id}:{filename}:{size}"

    raise ValueError(f"unsupported record_type: {record_type}")


def _write_sync_log(payload: dict[str, Any]) -> None:
    _ensure_sync_dirs()
    event = dict(payload)
    event.setdefault("created_at", _now_iso())
    _append_jsonl(SYNC_LOG_PATH, event)


def receive_manifest(payload: TerminalSyncManifestRequest) -> dict[str, Any]:
    device_id = payload.device_id.strip()
    sync_session_id = payload.sync_session_id.strip()
    if not sync_session_id:
        raise ValueError("sync_session_id is required")

    _check_trusted_device(device_id)
    _ensure_sync_dirs()

    manifest_path = MANIFEST_DIR / f"{_safe_name(sync_session_id, 'sync_session')}.json"
    manifest_payload = payload.dict()
    manifest_payload["core_received_at"] = _now_iso()
    _write_json_file_atomic(manifest_path, manifest_payload)

    _write_sync_log({
        "event_type": "manifest",
        "device_id": device_id,
        "sync_session_id": sync_session_id,
        "transport": payload.transport,
        "items": payload.items,
    })

    update_terminal_last_seen(device_id)

    audio_files = payload.items.get("audio_files", []) if isinstance(payload.items, dict) else []
    return {
        "ok": True,
        "sync_session_id": sync_session_id,
        "need": {
            "path_points": True,
            "field_events": True,
            "boot_logs": True,
            "audio_index": True,
            "audio_files": audio_files if isinstance(audio_files, list) else [],
        },
    }


def upload_records(payload: TerminalSyncUploadRecordsRequest) -> dict[str, Any]:
    device_id = payload.device_id.strip()
    sync_session_id = payload.sync_session_id.strip()
    record_type = payload.record_type.strip()

    if record_type not in ALLOWED_RECORD_TYPES:
        raise ValueError(f"unsupported record_type: {record_type}")

    _check_trusted_device(device_id)
    _ensure_sync_dirs()

    device_archive_dir = _get_archive_dir(device_id)
    device_archive_dir.mkdir(parents=True, exist_ok=True)
    archive_path = device_archive_dir / RECORD_ARCHIVE_FILES[record_type]
    seen_path = device_archive_dir / "seen_ids.json"
    seen_data = _load_seen_ids(seen_path)
    seen_ids = seen_data["seen_ids"]

    received = len(payload.records)
    imported = 0
    skipped_duplicate = 0
    now = _now_iso()

    for record in payload.records:
        record_id = _generate_record_id(record_type, device_id, record)
        if record_id in seen_ids:
            skipped_duplicate += 1
            continue

        archived_record = dict(record)
        archived_record["record_id"] = record_id
        archived_record["core_received_at"] = now
        _append_jsonl(archive_path, archived_record)

        seen_ids[record_id] = {
            "type": record_type,
            "first_seen_at": now,
        }
        imported += 1

    _write_json_file_atomic(seen_path, seen_data)

    _write_sync_log({
        "event_type": "upload_records",
        "device_id": device_id,
        "sync_session_id": sync_session_id,
        "record_type": record_type,
        "received": received,
        "imported": imported,
        "skipped_duplicate": skipped_duplicate,
    })

    update_terminal_last_seen(device_id)

    return {
        "ok": True,
        "record_type": record_type,
        "received": received,
        "imported": imported,
        "skipped_duplicate": skipped_duplicate,
        "ack": True,
    }
