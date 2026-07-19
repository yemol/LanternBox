"""Terminal audio lookup and safe playback helpers.

Core stores uploaded terminal WAV files as ordinary files under
``data/terminal_sync/archive`` and keeps a JSONL index beside the raw sync
archive. This service is the only place that turns an audio_id into a readable
file path. Frontend code must never build file paths directly.

Journal display note:
Audio-to-Journal linking is persisted when WAV files are imported. Journal
reads should not scan the archive or rebuild an audio index. They only validate
the stored path already recorded in journal.metadata_json.
"""

from __future__ import annotations

import json
import urllib.parse
from pathlib import Path
from typing import Any

from ..config import DATA_DIR


SYNC_ROOT = DATA_DIR / "terminal_sync"
ARCHIVE_DIR = SYNC_ROOT / "archive"
AUDIO_INDEX_JSONL_FILE = "audio_files.jsonl"
LEGACY_AUDIO_INDEX_FILE = "audio_files.json"


class TerminalAudioNotFoundError(Exception):
    """Raised when an audio id / filename is not known or has no stored file."""


class TerminalAudioPathError(Exception):
    """Raised when an indexed audio path would escape the archive boundary."""


class AudioPlaybackIndex:
    """Request-scoped terminal audio lookup index.

    It is intentionally lightweight and rebuilt from JSONL when /api/journal is
    requested. With N audio archive rows and M journal cards this changes the
    page cost from roughly O(N*M) repeated file scans to O(N+M).
    """

    def __init__(self, records: list[dict[str, Any]]):
        self.by_audio_id: dict[str, dict[str, Any]] = {}
        self.by_device_filename: dict[tuple[str, str], dict[str, Any]] = {}
        self.by_filename: dict[str, dict[str, Any]] = {}
        self.count = 0

        for record in records:
            normalized = dict(record)
            audio_id = _record_audio_id(normalized)
            device_id = _text(normalized.get("device_id"))
            filename = _record_filename(normalized)

            if audio_id:
                self.by_audio_id[audio_id] = normalized
            if device_id and filename:
                self.by_device_filename[(device_id, filename)] = normalized
            if filename:
                self.by_filename[filename] = normalized
            self.count += 1

    def find(
        self,
        *,
        audio_id: str | None = None,
        device_id: str | None = None,
        filename: str | None = None,
    ) -> dict[str, Any] | None:
        audio_id_text = _text(audio_id)
        device_id_text = _text(device_id)
        filename_text = Path(_text(filename)).name if filename else ""

        if audio_id_text and audio_id_text in self.by_audio_id:
            return dict(self.by_audio_id[audio_id_text])

        if device_id_text and filename_text:
            record = self.by_device_filename.get((device_id_text, filename_text))
            if record:
                return dict(record)

        if filename_text:
            record = self.by_filename.get(filename_text)
            if record:
                return dict(record)

        return None


def _text(value: Any) -> str:
    return str(value or "").strip()


def _read_jsonl_records(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    records: list[dict[str, Any]] = []
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


def _load_legacy_audio_records(device_archive_dir: Path) -> list[dict[str, Any]]:
    path = device_archive_dir / LEGACY_AUDIO_INDEX_FILE
    if not path.exists():
        return []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return []
    if not isinstance(data, dict):
        return []
    audio_files = data.get("audio_files")
    if not isinstance(audio_files, dict):
        return []

    records: list[dict[str, Any]] = []
    for audio_id, record in audio_files.items():
        if not isinstance(record, dict):
            continue
        normalized = dict(record)
        normalized.setdefault("audio_id", str(audio_id))
        if normalized.get("path") and not normalized.get("stored_path"):
            normalized["stored_path"] = str(device_archive_dir / str(normalized["path"]))
        records.append(normalized)
    return records


def _device_dirs(device_id: str | None = None) -> list[Path]:
    if device_id:
        return [ARCHIVE_DIR / device_id]
    if not ARCHIVE_DIR.exists():
        return []
    return sorted(path for path in ARCHIVE_DIR.iterdir() if path.is_dir())


def _resolve_stored_path(stored_path: str) -> Path:
    path = Path(stored_path)
    if path.is_absolute():
        return path
    return DATA_DIR.parent / path


def _ensure_archive_path(path: Path) -> Path:
    archive_root = ARCHIVE_DIR.resolve(strict=False)
    resolved = path.resolve(strict=False)
    try:
        resolved.relative_to(archive_root)
    except ValueError as exc:
        raise TerminalAudioPathError(f"audio path escapes archive: {path}") from exc
    return resolved


def _record_filename(record: dict[str, Any]) -> str:
    return Path(_text(record.get("filename") or record.get("file") or record.get("path"))).name


def _record_audio_id(record: dict[str, Any]) -> str:
    return _text(record.get("audio_id"))


def _record_stored_path(record: dict[str, Any]) -> str:
    return _text(record.get("stored_path") or record.get("path"))


def _resolve_audio_record_path(record: dict[str, Any], device_id: str | None = None) -> Path | None:
    """Resolve an indexed terminal audio record to a safe existing file path."""
    stored_path = _record_stored_path(record)
    candidate_paths: list[Path] = []

    if stored_path:
        candidate_paths.append(_resolve_stored_path(stored_path))

    stored_filename = _text(record.get("stored_filename"))
    record_device_id = _text(record.get("device_id") or device_id)
    if stored_filename and record_device_id:
        candidate_paths.append(ARCHIVE_DIR / record_device_id / "audio" / stored_filename)

    # Compatibility for early archive rows that stored only original filename.
    filename = _record_filename(record)
    if filename and record_device_id:
        candidate_paths.append(ARCHIVE_DIR / record_device_id / "audio" / filename)

    for path in candidate_paths:
        resolved = _ensure_archive_path(path)
        if resolved.exists():
            return resolved
    return None


def iter_audio_records(device_id: str | None = None, *, include_legacy: bool = True) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for device_archive_dir in _device_dirs(device_id):
        if not device_archive_dir.exists():
            continue
        current_device_id = device_archive_dir.name
        for record in _read_jsonl_records(device_archive_dir / AUDIO_INDEX_JSONL_FILE):
            normalized = dict(record)
            normalized.setdefault("device_id", current_device_id)
            records.append(normalized)
        if include_legacy:
            for record in _load_legacy_audio_records(device_archive_dir):
                normalized = dict(record)
                normalized.setdefault("device_id", current_device_id)
                records.append(normalized)
    return records


def _normalize_audio_record_for_index(
    record: dict[str, Any],
    *,
    require_existing_file: bool = True,
    suppress_path_error: bool = False,
) -> dict[str, Any] | None:
    normalized = dict(record)
    try:
        resolved = _resolve_audio_record_path(normalized, device_id=_text(normalized.get("device_id")))
    except TerminalAudioPathError:
        # A bad archive row must not break the whole Journal page when building
        # the request-scoped index, but direct playback lookup should still
        # report the unsafe archive entry clearly.
        if suppress_path_error:
            return None
        raise

    if require_existing_file and resolved is None:
        return None

    if resolved is not None:
        normalized["resolved_path"] = str(resolved)
    normalized.setdefault("filename", _record_filename(normalized))
    return normalized


def build_audio_playback_index(
    device_id: str | None = None,
    *,
    include_legacy: bool = True,
    require_existing_file: bool = True,
) -> AudioPlaybackIndex:
    """Build a request-scoped lookup index for terminal audio files."""
    normalized_records: list[dict[str, Any]] = []
    for record in iter_audio_records(device_id=device_id, include_legacy=include_legacy):
        normalized = _normalize_audio_record_for_index(
            record,
            require_existing_file=require_existing_file,
            suppress_path_error=True,
        )
        if normalized is not None:
            normalized_records.append(normalized)
    return AudioPlaybackIndex(normalized_records)


def _find_audio_record_once(
    *,
    audio_id: str | None = None,
    device_id: str | None = None,
    filename: str | None = None,
    require_existing_file: bool = True,
) -> dict[str, Any] | None:
    audio_id_text = _text(audio_id)
    filename_text = Path(_text(filename)).name if filename else ""
    device_id_text = _text(device_id)

    matches: list[dict[str, Any]] = []
    for record in iter_audio_records(device_id_text or None, include_legacy=True):
        if audio_id_text:
            if _record_audio_id(record) != audio_id_text:
                continue
        elif filename_text:
            if _record_filename(record) != filename_text:
                continue
        else:
            continue

        normalized = _normalize_audio_record_for_index(
            record,
            require_existing_file=require_existing_file,
            suppress_path_error=False,
        )
        if normalized is not None:
            matches.append(normalized)

    if not matches:
        return None
    return matches[-1]


def find_audio_record(
    *,
    audio_id: str | None = None,
    device_id: str | None = None,
    filename: str | None = None,
    require_existing_file: bool = True,
    audio_index: AudioPlaybackIndex | None = None,
) -> dict[str, Any] | None:
    """Find the newest matching audio archive record.

    Exact audio_id matching is preferred. If exact audio_id lookup misses, this
    intentionally falls back to filename+device because old journal rows can be
    created before the final session-aware audio_id format was introduced.
    """
    audio_id_text = _text(audio_id)
    filename_text = Path(_text(filename)).name if filename else ""

    if audio_index is not None:
        return audio_index.find(
            audio_id=audio_id_text,
            device_id=device_id,
            filename=filename_text,
        )

    if audio_id_text:
        exact = _find_audio_record_once(
            audio_id=audio_id_text,
            device_id=device_id,
            require_existing_file=require_existing_file,
        )
        if exact:
            return exact

    if filename_text:
        return _find_audio_record_once(
            filename=filename_text,
            device_id=device_id,
            require_existing_file=require_existing_file,
        )

    return None


def get_audio_file_for_playback(audio_id: str) -> tuple[Path, dict[str, Any]]:
    audio_id_text = _text(audio_id)
    if not audio_id_text:
        raise TerminalAudioNotFoundError("audio_id is required")
    record = find_audio_record(audio_id=audio_id_text, require_existing_file=True)
    if not record:
        raise TerminalAudioNotFoundError(f"audio not found: {audio_id_text}")
    path = Path(str(record["resolved_path"]))
    if not path.exists():
        raise TerminalAudioNotFoundError(f"audio file missing: {audio_id_text}")
    return path, record




def build_journal_audio_playback_metadata(metadata: dict[str, Any]) -> dict[str, Any]:
    """Validate persisted Journal audio metadata without scanning the archive.

    audio_index import creates a Journal row with audio_available=false.
    upload_audio_file later writes stored_path/sha256 into that same row. When
    /api/journal is read, this function only checks the single persisted
    stored_path. It never walks audio_files.jsonl or the audio directory.
    """
    base = dict(metadata or {})
    audio_id = _text(base.get("audio_id"))
    device_id = _text(base.get("device_id"))
    filename = _record_filename(base)
    if not filename:
        filename = Path(_text(base.get("filename"))).name if base.get("filename") else ""

    stored_path = _record_stored_path(base)
    if not audio_id or not stored_path:
        return {
            **base,
            "audio_available": False,
            "stored_audio_exists": False,
            "play_url": "",
            "device_id": device_id,
            "filename": filename,
        }

    try:
        resolved = _resolve_audio_record_path(base, device_id=device_id)
    except TerminalAudioPathError:
        return {
            **base,
            "audio_available": False,
            "stored_audio_exists": False,
            "play_url": "",
            "device_id": device_id,
            "filename": filename,
        }

    if resolved is None or not resolved.exists() or not resolved.is_file():
        return {
            **base,
            "audio_available": False,
            "stored_audio_exists": False,
            "play_url": "",
            "device_id": device_id,
            "filename": filename,
        }

    return {
        **base,
        "audio_available": True,
        "stored_audio_exists": True,
        "play_url": "/api/terminal-sync/audio?" + urllib.parse.urlencode({"audio_id": audio_id}),
        "device_id": device_id,
        "filename": filename,
        "size": base.get("size") or resolved.stat().st_size,
    }

def build_audio_playback_metadata(
    *,
    audio_id: str | None = None,
    device_id: str | None = None,
    filename: str | None = None,
    audio_index: AudioPlaybackIndex | None = None,
) -> dict[str, Any]:
    record = find_audio_record(
        audio_id=audio_id,
        device_id=device_id,
        filename=filename,
        require_existing_file=True,
        audio_index=audio_index,
    )
    if not record:
        return {
            "audio_available": False,
            "audio_id": _text(audio_id),
            "device_id": _text(device_id),
            "filename": Path(_text(filename)).name if filename else "",
            "play_url": "",
        }

    resolved_audio_id = _record_audio_id(record) or _text(audio_id)
    play_url = ""
    if resolved_audio_id:
        play_url = "/api/terminal-sync/audio?" + urllib.parse.urlencode({"audio_id": resolved_audio_id})

    return {
        "audio_available": bool(play_url),
        "audio_id": resolved_audio_id,
        "device_id": _text(record.get("device_id") or device_id),
        "filename": _record_filename(record) or Path(_text(filename)).name,
        "size": record.get("size"),
        "received_at": _text(record.get("received_at")),
        "sha256": _text(record.get("sha256")),
        "play_url": play_url,
    }
