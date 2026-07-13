"""Reusable journal entry helpers.

The journal table is the Core-facing daily record stream. Terminal sync keeps
its raw JSONL archive separately and calls this service only for records that
should become user-visible journal entries.
"""

from datetime import datetime
from typing import Any

from ..db import get_db_connection


TERMINAL_EVENT_ENTRY_TYPE = "终端现场日志"
TERMINAL_AUDIO_ENTRY_TYPE = "终端录音日志"


def _now_journal_time() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def _text(value: Any) -> str:
    return str(value or "").strip()


def _first_text(record: dict[str, Any], keys: list[str]) -> str:
    for key in keys:
        value = _text(record.get(key))
        if value:
            return value
    return ""


def _append_line(lines: list[str], label: str, value: Any) -> None:
    text = _text(value)
    if text:
        lines.append(f"{label}：{text}")


def _terminal_time(record: dict[str, Any]) -> str:
    device_date = _first_text(record, ["device_date", "date"])
    device_time = _first_text(record, ["device_time", "time"])
    timestamp = _first_text(record, ["device_timestamp", "timestamp", "created_at"])
    if device_date or device_time:
        return " ".join(part for part in [device_date, device_time] if part)
    return timestamp


def _location_text(record: dict[str, Any]) -> str:
    lat = record.get("lat", record.get("latitude"))
    lon = record.get("lon", record.get("longitude"))
    if lat is None or lon is None:
        return ""
    return f"{lat}, {lon}"


def list_journal_entries() -> list[dict[str, Any]]:
    conn = get_db_connection()
    rows = conn.execute("SELECT * FROM journal ORDER BY id DESC").fetchall()
    conn.close()
    return [dict(row) for row in rows]


def create_journal_entry(
    *,
    entry_type: str = "日常记录",
    title: str = "",
    content: str = "",
    created_at: str | None = None,
) -> dict[str, Any]:
    created_at_value = created_at or _now_journal_time()
    entry_type_value = entry_type or "日常记录"
    title_value = title or ""
    content_value = content or ""

    conn = get_db_connection()
    cursor = conn.execute(
        """
        INSERT INTO journal
        (entry_type, title, content, created_at)
        VALUES (?, ?, ?, ?)
        """,
        (
            entry_type_value,
            title_value,
            content_value,
            created_at_value,
        ),
    )
    conn.commit()
    new_id = cursor.lastrowid
    conn.close()

    return {
        "id": new_id,
        "entry_type": entry_type_value,
        "title": title_value,
        "content": content_value,
        "created_at": created_at_value,
    }


def delete_journal_entry(entry_id: int) -> bool:
    conn = get_db_connection()
    cursor = conn.execute(
        "DELETE FROM journal WHERE id = ?",
        (entry_id,),
    )
    conn.commit()
    deleted_count = cursor.rowcount
    conn.close()
    return deleted_count > 0


def create_journal_entry_from_terminal_event(
    *,
    device_id: str,
    sync_session_id: str,
    record_id: str,
    record: dict[str, Any],
    received_at: str | None = None,
) -> dict[str, Any]:
    terminal_time = _terminal_time(record)
    note = _first_text(record, ["note", "content", "message", "text", "description", "event"])
    event_type = _first_text(record, ["event_type", "type", "category"])

    title_parts = ["终端现场日志", device_id]
    if event_type:
        title_parts.append(event_type)
    if terminal_time:
        title_parts.append(terminal_time)

    lines = [
        "这是由终端同步导入的现场日志。",
        f"来源终端：{device_id}",
        f"同步会话：{sync_session_id}",
        f"记录 ID：{record_id}",
    ]
    _append_line(lines, "终端时间", terminal_time)
    _append_line(lines, "现场类型", event_type)
    _append_line(lines, "现场记录", note)
    _append_line(lines, "坐标", _location_text(record))
    _append_line(lines, "Core 接收时间", received_at or record.get("core_received_at"))

    return create_journal_entry(
        entry_type=TERMINAL_EVENT_ENTRY_TYPE,
        title="｜".join(title_parts),
        content="\n".join(lines),
    )


def create_journal_entry_from_terminal_audio_index(
    *,
    device_id: str,
    sync_session_id: str,
    record_id: str,
    record: dict[str, Any],
    received_at: str | None = None,
) -> dict[str, Any]:
    terminal_time = _terminal_time(record)
    audio_id = _first_text(record, ["audio_id", "id"])
    filename = _first_text(record, ["filename", "file", "path", "relative_path"])
    note = _first_text(record, ["note", "content", "message", "description"])

    title_target = filename or audio_id or record_id
    title = f"终端录音日志｜{device_id}｜{title_target}"

    lines = [
        "这是终端录音索引日志。",
        "WAV 文件可能尚未上传到 Core；当前仅记录录音索引。",
        f"来源终端：{device_id}",
        f"同步会话：{sync_session_id}",
        f"记录 ID：{record_id}",
    ]
    _append_line(lines, "录音 ID", audio_id)
    _append_line(lines, "文件名", filename)
    _append_line(lines, "终端时间", terminal_time)
    _append_line(lines, "时长", _first_text(record, ["duration_sec", "duration_seconds", "duration"]))
    _append_line(lines, "文件大小", _first_text(record, ["size", "bytes", "file_size"]))
    _append_line(lines, "备注", note)
    _append_line(lines, "Core 接收时间", received_at or record.get("core_received_at"))

    return create_journal_entry(
        entry_type=TERMINAL_AUDIO_ENTRY_TYPE,
        title=title,
        content="\n".join(lines),
    )
