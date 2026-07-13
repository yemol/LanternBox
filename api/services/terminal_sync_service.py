"""Terminal sync manifest, JSONL archive, audio archive, and commit ACK service.

This module stores terminal sync metadata and records on Core. It does not
implement transport, encryption, or FT-01 firmware behavior.
"""

import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from ..config import DATA_DIR
from ..models import (
    TerminalSyncCommitRequest,
    TerminalSyncManifestRequest,
    TerminalSyncUploadRecordsRequest,
)
from .journal_service import (
    create_journal_entry_from_terminal_audio_index,
    create_journal_entry_from_terminal_event,
)
from .terminal_service import get_terminal_device, update_terminal_last_seen


SYNC_ROOT = DATA_DIR / "terminal_sync"
MANIFEST_DIR = SYNC_ROOT / "manifests"
ARCHIVE_DIR = SYNC_ROOT / "archive"
SYNC_LOG_PATH = SYNC_ROOT / "sync_log.jsonl"
AUDIO_INDEX_JSONL_FILE = "audio_files.jsonl"
LEGACY_AUDIO_INDEX_FILE = "audio_files.json"
AUDIO_DIR_NAME = "audio"
COMMITS_FILE = "commits.jsonl"

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


class TerminalSyncAudioConflictError(Exception):
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


def _stored_path_for_audio_path(audio_path: Path) -> str:
    try:
        return str(audio_path.relative_to(DATA_DIR.parent))
    except ValueError:
        return str(audio_path)


def _resolve_stored_path(stored_path: str) -> Path:
    path = Path(stored_path)
    if path.is_absolute():
        return path
    return DATA_DIR.parent / path


def _read_jsonl_records(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    records = []
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        try:
            record = json.loads(stripped)
        except json.JSONDecodeError:
            continue
        if isinstance(record, dict):
            records.append(record)
    return records


def _normalize_audio_size(value: Any) -> int | None:
    try:
        size = int(value)
    except (TypeError, ValueError):
        return None
    if size < 0:
        return None
    return size


def _load_legacy_audio_records(device_archive_dir: Path) -> dict[str, list[dict[str, Any]]]:
    path = device_archive_dir / LEGACY_AUDIO_INDEX_FILE
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}
    if not isinstance(data, dict):
        return {}
    audio_files = data.get("audio_files")
    if not isinstance(audio_files, dict):
        return {}

    records_by_audio_id: dict[str, list[dict[str, Any]]] = {}
    for audio_id, record in audio_files.items():
        if not isinstance(record, dict):
            continue
        audio_id_value = str(record.get("audio_id") or audio_id or "").strip()
        if not audio_id_value:
            continue
        normalized = dict(record)
        normalized["audio_id"] = audio_id_value
        normalized_size = _normalize_audio_size(normalized.get("size"))
        if normalized_size is not None:
            normalized["size"] = normalized_size
        legacy_path = str(normalized.get("path") or "").strip()
        if legacy_path and not normalized.get("stored_path"):
            normalized["stored_path"] = str(device_archive_dir / legacy_path)
        records_by_audio_id.setdefault(audio_id_value, []).append(normalized)
    return records_by_audio_id


def _load_audio_records(device_archive_dir: Path, *, include_legacy: bool = True) -> dict[str, list[dict[str, Any]]]:
    records_by_audio_id: dict[str, list[dict[str, Any]]] = {}

    for record in _read_jsonl_records(device_archive_dir / AUDIO_INDEX_JSONL_FILE):
        audio_id = str(record.get("audio_id") or "").strip()
        if not audio_id:
            continue
        normalized = dict(record)
        normalized_size = _normalize_audio_size(normalized.get("size"))
        if normalized_size is not None:
            normalized["size"] = normalized_size
        records_by_audio_id.setdefault(audio_id, []).append(normalized)

    if include_legacy:
        legacy_records = _load_legacy_audio_records(device_archive_dir)
        for audio_id, records in legacy_records.items():
            if audio_id not in records_by_audio_id:
                records_by_audio_id[audio_id] = records

    return records_by_audio_id


def _safe_audio_filename(audio_id: str, filename: str, size: int) -> str:
    source = filename or f"{audio_id}.wav"
    safe = _safe_name(source, f"{_safe_name(audio_id, 'audio')}.wav")
    if "." not in safe:
        safe = f"{safe}.wav"
    stem, suffix = safe.rsplit(".", 1)
    safe_audio_id = _safe_name(audio_id, "audio")
    return f"{safe_audio_id}__{stem}_{size}.{suffix}"


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

        if record_type == "field_events":
            create_journal_entry_from_terminal_event(
                device_id=device_id,
                sync_session_id=sync_session_id,
                record_id=record_id,
                record=archived_record,
                received_at=now,
            )
        elif record_type == "audio_index":
            create_journal_entry_from_terminal_audio_index(
                device_id=device_id,
                sync_session_id=sync_session_id,
                record_id=record_id,
                record=archived_record,
                received_at=now,
            )

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


def upload_audio_file(
    *,
    device_id: str,
    sync_session_id: str,
    audio_id: str,
    filename: str,
    expected_size: int | None,
    content: bytes,
) -> dict[str, Any]:
    device_id_value = device_id.strip()
    sync_session_id_value = sync_session_id.strip()
    audio_id_value = audio_id.strip()
    filename_value = filename.strip()

    if not sync_session_id_value:
        raise ValueError("sync_session_id is required")
    if not audio_id_value:
        raise ValueError("audio_id is required")
    if not content:
        raise ValueError("audio content is required")

    actual_size = len(content)
    if expected_size is not None and expected_size != actual_size:
        raise ValueError(f"audio size mismatch: expected {expected_size}, got {actual_size}")

    _check_trusted_device(device_id_value)
    _ensure_sync_dirs()

    device_archive_dir = _get_archive_dir(device_id_value)
    audio_dir = device_archive_dir / AUDIO_DIR_NAME
    device_archive_dir.mkdir(parents=True, exist_ok=True)
    audio_dir.mkdir(parents=True, exist_ok=True)

    audio_records = _load_audio_records(device_archive_dir, include_legacy=True)
    existing_records = audio_records.get(audio_id_value, [])
    existing = existing_records[0] if existing_records else None

    if isinstance(existing, dict):
        existing_size = _normalize_audio_size(existing.get("size"))
        if existing_size != actual_size:
            _write_sync_log({
                "event_type": "upload_audio_conflict",
                "device_id": device_id_value,
                "sync_session_id": sync_session_id_value,
                "audio_id": audio_id_value,
                "existing_size": existing_size,
                "incoming_size": actual_size,
            })
            raise TerminalSyncAudioConflictError(
                f"audio_id conflict: {audio_id_value} already exists with size {existing_size}"
            )

        _write_sync_log({
            "event_type": "upload_audio_duplicate",
            "device_id": device_id_value,
            "sync_session_id": sync_session_id_value,
            "audio_id": audio_id_value,
            "size": actual_size,
        })
        update_terminal_last_seen(device_id_value)
        return {
            "ok": True,
            "audio_id": audio_id_value,
            "size": actual_size,
            "status": "duplicate",
            "imported": False,
            "duplicate": True,
            "conflict": False,
            "path": existing.get("stored_path") or existing.get("path", ""),
            "ack": True,
        }

    now = _now_iso()
    file_name = _safe_audio_filename(audio_id_value, filename_value, actual_size)
    audio_path = audio_dir / file_name
    audio_path.write_bytes(content)
    sha256 = hashlib.sha256(content).hexdigest()
    stored_path = _stored_path_for_audio_path(audio_path)

    audio_file_record = {
        "audio_id": audio_id_value,
        "device_id": device_id_value,
        "sync_session_id": sync_session_id_value,
        "filename": filename_value,
        "size": actual_size,
        "stored_path": stored_path,
        "received_at": now,
        "transport": "unknown",
        "stored_filename": file_name,
        "sha256": sha256,
    }
    _append_jsonl(device_archive_dir / AUDIO_INDEX_JSONL_FILE, audio_file_record)
    _write_sync_log({
        "event_type": "upload_audio",
        "device_id": device_id_value,
        "sync_session_id": sync_session_id_value,
        "audio_id": audio_id_value,
        "filename": filename_value,
        "stored_path": stored_path,
        "size": actual_size,
        "sha256": sha256,
    })
    update_terminal_last_seen(device_id_value)

    return {
        "ok": True,
        "audio_id": audio_id_value,
        "size": actual_size,
        "status": "imported",
        "imported": True,
        "duplicate": False,
        "conflict": False,
        "path": stored_path,
        "ack": True,
    }


def _acked_audio_files(acked: dict[str, Any]) -> list[dict[str, Any]]:
    audio_files = acked.get("audio_files")
    if not isinstance(audio_files, list):
        return []
    return [item for item in audio_files if isinstance(item, dict)]


def _audio_file_may_clear(device_archive_dir: Path, audio_id: str, ack: bool) -> bool:
    if not ack:
        return False

    records = _load_audio_records(device_archive_dir, include_legacy=False).get(audio_id, [])
    for record in records:
        stored_path = str(record.get("stored_path") or "").strip()
        if stored_path and _resolve_stored_path(stored_path).exists():
            return True
    return False


def _build_terminal_may_clear(device_archive_dir: Path, acked: dict[str, Any]) -> dict[str, Any]:
    may_clear = {
        "path_points": bool(acked.get("path_points")),
        "field_events": bool(acked.get("field_events")),
        "boot_logs": bool(acked.get("boot_logs")),
        "audio_index": bool(acked.get("audio_index")),
        "audio_files": [],
    }

    audio_results = []
    for item in _acked_audio_files(acked):
        audio_id = str(item.get("audio_id") or "").strip()
        if not audio_id:
            continue
        audio_results.append({
            "audio_id": audio_id,
            "may_clear": _audio_file_may_clear(
                device_archive_dir,
                audio_id,
                bool(item.get("ack")),
            ),
        })
    may_clear["audio_files"] = audio_results
    return may_clear


def commit_sync(payload: TerminalSyncCommitRequest) -> dict[str, Any]:
    device_id = payload.device_id.strip()
    sync_session_id = payload.sync_session_id.strip()
    if not sync_session_id:
        raise ValueError("sync_session_id is required")

    _check_trusted_device(device_id)
    _ensure_sync_dirs()

    committed_at = _now_iso()
    device_archive_dir = _get_archive_dir(device_id)
    device_archive_dir.mkdir(parents=True, exist_ok=True)
    acked = payload.acked if isinstance(payload.acked, dict) else {}
    terminal_may_clear = _build_terminal_may_clear(device_archive_dir, acked)
    commit_payload = {
        "event_type": "commit",
        "device_id": device_id,
        "sync_session_id": sync_session_id,
        "clear_request": payload.clear_request,
        "acked": acked,
        "summary": payload.summary if isinstance(payload.summary, dict) else {},
        "commit": "ok",
        "terminal_may_clear": terminal_may_clear,
        "committed_at": committed_at,
    }
    _append_jsonl(device_archive_dir / COMMITS_FILE, commit_payload)
    _write_sync_log(commit_payload)
    update_terminal_last_seen(device_id)

    return {
        "ok": True,
        "sync_session_id": sync_session_id,
        "commit": "ok",
        "terminal_may_clear": terminal_may_clear,
        "committed_at": committed_at,
        "ack": True,
    }
