"""Reusable journal entry helpers.

The journal table is the Core-facing daily record stream. Terminal sync keeps
its raw JSONL archive separately and calls this service only for records that
should become user-visible journal entries.
"""

import re
import json
from datetime import datetime
from pathlib import PurePosixPath
from typing import Any

from ..db import column_exists, get_db_connection


TERMINAL_EVENT_ENTRY_TYPE = "终端现场日志"
TERMINAL_AUDIO_ENTRY_TYPE = "终端录音日志"
TERMINAL_TRACK_ENTRY_TYPE = "终端轨迹记录"

PATH_RELATED_EVENT_TYPES = {
    "session_start",
    "session_stop",
    "session_end",
    "base_mark",
    "base",
    "base_mark_failed",
    "base_failed",
    "path_point",
    "endpoint",
    "end",
    "start",
    "auto_track_on",
    "auto_track_off",
    "auto_track_toggle",
    "auto_track",
    "track_toggle",
    "storage_test",
    "manual_test",
    "diagnostic_test",
    "debug_test",
}

PATH_RELATED_EVENT_NOTES = {
    "leave recorder",
    "enter recorder",
    "toggle",
    "no gnss fix",
    "manual test write from recorder screen",
}


def _now_journal_time() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def _ensure_journal_metadata_column(conn) -> None:
    if not column_exists(conn, "journal", "metadata_json"):
        conn.execute("ALTER TABLE journal ADD COLUMN metadata_json TEXT")


def _text(value: Any) -> str:
    if value is None:
        return ""
    return str(value).strip()


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


def _append_optional_line(lines: list[str], label: str, value: Any) -> None:
    text = _text(value)
    if text:
        lines.append(f"{label}：{text}")


def _json_dumps(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def _parse_datetime(value: Any) -> datetime | None:
    text = _text(value)
    if not text:
        return None

    text = text.replace("Z", "+00:00")
    candidates = [
        text,
        text.replace("T", " "),
        text.split(".")[0].replace("T", " "),
    ]

    for candidate in candidates:
        try:
            parsed = datetime.fromisoformat(candidate)
        except ValueError:
            parsed = None
        if parsed is None:
            continue
        if parsed.tzinfo is not None:
            parsed = parsed.astimezone()
        return parsed.replace(tzinfo=None)

    for fmt, length in (
        ("%Y-%m-%d %H:%M:%S", 19),
        ("%Y-%m-%d %H:%M", 16),
        ("%Y-%m-%d", 10),
    ):
        try:
            return datetime.strptime(text[:length], fmt)
        except ValueError:
            continue
    return None


def format_display_time(value: Any) -> str:
    parsed = _parse_datetime(value)
    if parsed is None:
        text = _text(value).replace("T", " ")
        if len(text) >= 16 and re.match(r"^\d{4}-\d{2}-\d{2} ", text):
            return text[:16]
        return text
    return parsed.strftime("%Y-%m-%d %H:%M")


def format_terminal_time(record: dict[str, Any]) -> str:
    raw = _terminal_time(record)
    return format_display_time(raw)


def format_journal_created_at(value: Any) -> str:
    parsed = _parse_datetime(value)
    if parsed is None:
        return ""
    return parsed.strftime("%Y-%m-%d %H:%M:%S")


def _terminal_created_at(record: dict[str, Any], fallback_date_source: Any = None) -> str | None:
    raw_time = _terminal_time(record)
    full_created_at = format_journal_created_at(raw_time)
    if full_created_at:
        return full_created_at

    time_only = re.match(r"^(\d{2}):(\d{2})(?::(\d{2}))?$", _text(raw_time))
    fallback_date = _parse_datetime(fallback_date_source)
    if time_only and fallback_date is not None:
        hour, minute, second = time_only.groups()
        return f"{fallback_date.strftime('%Y-%m-%d')} {hour}:{minute}:{second or '00'}"

    return None


def format_sync_session(sync_session_id: Any) -> str:
    text = _text(sync_session_id)
    if re.match(r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}$", text):
        return text
    match = re.search(r"(\d{8})-(\d{6})$", text)
    if not match:
        return ""
    date_text, time_text = match.groups()
    return (
        f"{date_text[0:4]}-{date_text[4:6]}-{date_text[6:8]} "
        f"{time_text[0:2]}:{time_text[2:4]}"
    )


def extract_filename(value: Any) -> str:
    text = _text(value).replace("\\", "/")
    if not text:
        return ""
    match = re.search(r"([^:/\\]+\.wav)\b", text, re.IGNORECASE)
    if match:
        return match.group(1)
    if "/" in text:
        return PurePosixPath(text).name
    return text


def format_file_size(value: Any) -> str:
    text = _text(value)
    if not text:
        return ""
    friendly_size = re.match(r"^(\d+(?:\.\d+)?)\s*(B|KB|MB)$", text, re.IGNORECASE)
    if friendly_size:
        number, unit = friendly_size.groups()
        return f"{number} {unit.upper()}"
    try:
        size = int(float(text))
    except ValueError:
        return ""
    if size < 0:
        return ""
    if size < 1024:
        return f"{size} B"
    if size < 1024 * 1024:
        return f"{round(size / 1024)} KB"
    return f"{size / (1024 * 1024):.1f} MB"


def format_duration(value: Any) -> str:
    text = _text(value)
    if not text:
        return "未知"
    try:
        seconds = float(text)
    except ValueError:
        return "未知"
    if seconds < 0:
        return "未知"
    if seconds < 60:
        if abs(seconds - round(seconds)) < 0.05:
            return f"{int(round(seconds))}秒"
        return f"{seconds:.1f}".rstrip("0").rstrip(".") + "秒"

    total = int(round(seconds))
    minutes = total // 60
    rest = total % 60
    return f"{minutes}分{rest:02d}秒"


def _short_text(value: Any, limit: int = 22) -> str:
    text = _text(value)
    if len(text) <= limit:
        return text
    return text[:limit].rstrip() + "..."


def _content_labels(content: Any) -> dict[str, str]:
    labels: dict[str, str] = {}
    for raw_line in str(content or "").splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if "：" in line:
            key, value = line.split("：", 1)
        elif ":" in line:
            key, value = line.split(":", 1)
        else:
            continue
        labels[key.strip()] = value.strip()
    return labels


def _title_parts(title: Any) -> list[str]:
    return [part.strip() for part in re.split(r"[｜|]", _text(title)) if part.strip()]


def _terminal_time(record: dict[str, Any]) -> str:
    device_date = _first_text(record, ["device_date", "date"])
    device_time = _first_text(record, ["device_time", "time"])
    timestamp = _first_text(record, ["device_timestamp", "timestamp", "created_at"])
    if device_date or device_time:
        return " ".join(part for part in [device_date, device_time] if part)
    return timestamp


def _bool_false(value: Any) -> bool:
    if value is False:
        return True
    if value == 0:
        return True
    text = _text(value).lower()
    return text in {"false", "0", "no", "nofix", "none", "null"}


def _location_text(record: dict[str, Any]) -> str:
    lat = record.get("lat", record.get("latitude"))
    lon = record.get("lon", record.get("longitude"))
    gnss_fix = record.get("gnss_fix", record.get("fix"))
    if _bool_false(gnss_fix):
        return "无有效定位"
    if lat is None or lon is None:
        return ""
    try:
        lat_num = float(lat)
        lon_num = float(lon)
    except (TypeError, ValueError):
        return ""
    if lat_num == 0 and lon_num == 0:
        return "无有效定位"
    return f"{lat}, {lon}"


def _terminal_event_title(device_id: str, record: dict[str, Any]) -> str:
    note = _first_text(record, ["note", "content", "message", "text", "description", "event"])
    event_type = _first_text(record, ["event_type", "type", "category"])
    suffix = _short_text(note or event_type)
    return "｜".join(part for part in ["终端现场", device_id, suffix] if part)


def _terminal_event_content(device_id: str, record: dict[str, Any]) -> str:
    terminal_time = format_terminal_time(record)
    event_type = _first_text(record, ["event_type", "type", "category"])
    note = _first_text(record, ["note", "content", "message", "text", "description", "event"])

    lines = [f"来源终端：{device_id}"]
    _append_optional_line(lines, "事件类型", event_type)
    _append_optional_line(lines, "记录时间", terminal_time)
    _append_optional_line(lines, "位置", _location_text(record))
    _append_optional_line(lines, "卫星数", _first_text(record, ["satellites", "sats", "gnss_satellites"]))
    _append_optional_line(lines, "模式", _first_text(record, ["mode"]))
    _append_optional_line(lines, "轨迹会话", _first_text(record, ["session_id", "track_session_id"]))
    _append_optional_line(lines, "备注", note)
    return "\n".join(lines)


def _terminal_audio_filename(record: dict[str, Any]) -> str:
    filename = _first_text(record, ["filename", "file", "path", "relative_path"])
    return extract_filename(filename)


def _terminal_audio_duration(record: dict[str, Any]) -> str:
    return format_duration(_first_text(record, ["duration", "duration_sec", "duration_seconds", "seconds"]))


def _terminal_audio_title(device_id: str, record: dict[str, Any], fallback: str = "") -> str:
    filename = _terminal_audio_filename(record) or extract_filename(fallback)
    return "｜".join(part for part in ["终端录音", device_id, filename] if part)


def _terminal_audio_content(
    *,
    device_id: str,
    sync_session_id: str,
    record: dict[str, Any],
) -> str:
    terminal_time = format_terminal_time(record)
    filename = _terminal_audio_filename(record)
    size = format_file_size(_first_text(record, ["size", "bytes", "file_size"]))
    duration = _terminal_audio_duration(record)

    lines = [
        "这是终端录音索引日志。",
        "录音文件可能尚未上传到 Core。",
        f"来源终端：{device_id}",
    ]
    _append_optional_line(lines, "文件名", filename)
    _append_optional_line(lines, "记录时间", terminal_time)
    _append_optional_line(lines, "文件大小", size)
    lines.append(f"时长：{duration}")
    return "\n".join(lines)


def _format_track_distance(value: Any) -> str:
    try:
        meters = float(value)
    except (TypeError, ValueError):
        return ""
    if meters < 0:
        return ""
    if meters < 1000:
        return f"{round(meters)} m"
    return f"{meters / 1000:.1f} km"


def _format_track_duration(value: Any) -> str:
    try:
        seconds = int(round(float(value)))
    except (TypeError, ValueError):
        return ""
    if seconds < 0:
        return ""
    hours = seconds // 3600
    minutes = (seconds % 3600) // 60
    rest = seconds % 60
    if hours:
        return f"{hours}小时{minutes:02d}分"
    if minutes:
        return f"{minutes}分{rest:02d}秒"
    return f"{rest}秒"


def _track_title(device_id: str, track: dict[str, Any]) -> str:
    summary = track.get("summary") if isinstance(track.get("summary"), dict) else {}
    start_time = format_display_time(summary.get("start_time")) or "终端时间未知"
    return "｜".join(part for part in ["外出轨迹", device_id, start_time] if part)


def _track_content(device_id: str, track: dict[str, Any]) -> str:
    summary = track.get("summary") if isinstance(track.get("summary"), dict) else {}
    session_id = _text(track.get("session_id"))
    point_count = summary.get("point_count")
    start_time = format_display_time(summary.get("start_time"))
    end_time = format_display_time(summary.get("end_time"))
    distance = _format_track_distance(summary.get("distance_meters"))
    duration = _format_track_duration(summary.get("duration_seconds"))
    raw_duration = _format_track_duration(summary.get("raw_duration_seconds"))
    time_quality = _text(summary.get("time_quality"))

    start_point = track.get("start_point") if isinstance(track.get("start_point"), dict) else {}
    end_point = track.get("end_point") if isinstance(track.get("end_point"), dict) else {}
    base_points = track.get("base_points") if isinstance(track.get("base_points"), list) else []

    node_lines = []
    if start_point:
        node_lines.append("✓ 起点")
    if base_points:
        node_lines.append("✓ 基地位置")
    if point_count:
        node_lines.append("✓ 移动轨迹")
    if end_point:
        node_lines.append("✓ 终点")

    lines = [
        f"来源终端：{device_id}",
        f"轨迹ID：{session_id}",
    ]
    _append_optional_line(lines, "开始", start_time)
    _append_optional_line(lines, "结束", end_time)
    _append_optional_line(lines, "轨迹点", point_count)
    _append_optional_line(lines, "距离", distance)
    _append_optional_line(lines, "用时", duration or raw_duration)
    # Keep time_quality in metadata_json for diagnostics, but do not show it in
    # the user-facing journal card. Values such as terminal_device_datetime and
    # terminal_gnss_utc are implementation details, not journal content.
    if node_lines:
        lines.append("")
        lines.append("包含节点：")
        lines.extend(node_lines)
    return "\n".join(lines)


def _find_terminal_track_entry_id(
    conn,
    *,
    device_id: str,
    session_id: str,
) -> int | None:
    rows = conn.execute(
        """
        SELECT id, content, metadata_json FROM journal
        WHERE entry_type = ?
        ORDER BY id DESC
        """,
        (TERMINAL_TRACK_ENTRY_TYPE,),
    ).fetchall()

    for row in rows:
        metadata = {}
        raw_metadata = _text(row["metadata_json"] if "metadata_json" in row.keys() else "")
        if raw_metadata:
            try:
                parsed = json.loads(raw_metadata)
            except json.JSONDecodeError:
                parsed = {}
            if isinstance(parsed, dict):
                metadata = parsed
        if metadata.get("device_id") == device_id and metadata.get("session_id") == session_id:
            return int(row["id"])

        labels = _content_labels(row["content"])
        if labels.get("来源终端") == device_id and labels.get("轨迹ID") == session_id:
            return int(row["id"])
    return None


def upsert_journal_entry_from_terminal_track(
    *,
    device_id: str,
    session_id: str,
    track: dict[str, Any],
) -> dict[str, Any]:
    metadata = dict(track)
    metadata["device_id"] = device_id
    metadata["session_id"] = session_id
    summary = metadata.get("summary") if isinstance(metadata.get("summary"), dict) else {}
    # Do not invent a terminal track date from Core rebuild/import time. If the
    # terminal did not provide or imply a trustworthy date, place the derived
    # track at the bottom of the Journal instead of giving it a false current day.
    created_at = format_journal_created_at(summary.get("start_time")) or "1970-01-01 00:00:00"
    title = _track_title(device_id, metadata)
    content = _track_content(device_id, metadata)
    metadata_json = _json_dumps(metadata)

    conn = get_db_connection()
    _ensure_journal_metadata_column(conn)
    existing_id = _find_terminal_track_entry_id(
        conn,
        device_id=device_id,
        session_id=session_id,
    )
    if existing_id is not None:
        conn.execute(
            """
            UPDATE journal
            SET title = ?, content = ?, created_at = ?, metadata_json = ?
            WHERE id = ?
            """,
            (title, content, created_at, metadata_json, existing_id),
        )
        conn.commit()
        conn.close()
        return {
            "id": existing_id,
            "entry_type": TERMINAL_TRACK_ENTRY_TYPE,
            "title": title,
            "content": content,
            "created_at": created_at,
            "metadata_json": metadata_json,
            "metadata": metadata,
        }

    conn.close()
    return create_journal_entry(
        entry_type=TERMINAL_TRACK_ENTRY_TYPE,
        title=title,
        content=content,
        created_at=created_at,
        metadata=metadata,
    )



def delete_journal_entry_from_terminal_track(
    *,
    device_id: str,
    session_id: str,
) -> bool:
    conn = get_db_connection()
    _ensure_journal_metadata_column(conn)
    existing_id = _find_terminal_track_entry_id(
        conn,
        device_id=device_id,
        session_id=session_id,
    )
    if existing_id is None:
        conn.close()
        return False

    cursor = conn.execute(
        "DELETE FROM journal WHERE id = ? AND entry_type = ?",
        (existing_id, TERMINAL_TRACK_ENTRY_TYPE),
    )
    conn.commit()
    deleted = cursor.rowcount > 0
    conn.close()
    return deleted


def delete_all_terminal_track_journal_entries(device_id: str | None = None) -> int:
    conn = get_db_connection()
    _ensure_journal_metadata_column(conn)

    if not device_id:
        cursor = conn.execute(
            "DELETE FROM journal WHERE entry_type = ?",
            (TERMINAL_TRACK_ENTRY_TYPE,),
        )
        conn.commit()
        count = cursor.rowcount
        conn.close()
        return count

    rows = conn.execute(
        """
        SELECT id, content, metadata_json FROM journal
        WHERE entry_type = ?
        """,
        (TERMINAL_TRACK_ENTRY_TYPE,),
    ).fetchall()

    delete_ids: list[int] = []
    for row in rows:
        matched = False
        raw_metadata = _text(row["metadata_json"] if "metadata_json" in row.keys() else "")
        if raw_metadata:
            try:
                parsed = json.loads(raw_metadata)
            except json.JSONDecodeError:
                parsed = {}
            if isinstance(parsed, dict) and parsed.get("device_id") == device_id:
                matched = True

        labels = _content_labels(row["content"])
        if labels.get("来源终端") == device_id:
            matched = True

        if matched:
            delete_ids.append(int(row["id"]))

    for entry_id in delete_ids:
        conn.execute("DELETE FROM journal WHERE id = ?", (entry_id,))

    conn.commit()
    conn.close()
    return len(delete_ids)


def _row_is_path_related_terminal_event(row) -> bool:
    """Return True for legacy terminal-event rows that should be folded into tracks.

    Earlier Core versions wrote path_point/session_start/session_stop/base_mark
    field_events as user-visible 终端现场日志. Those rows are derived, noisy
    track-source records. They should not remain in Journal once the clean
    terminal_track pipeline is active.
    """

    labels = _content_labels(row["content"] if "content" in row.keys() else "")
    parts = _title_parts(row["title"] if "title" in row.keys() else "")
    entry_type = _text(row["entry_type"] if "entry_type" in row.keys() else "")

    if entry_type != TERMINAL_EVENT_ENTRY_TYPE and not _text(row["title"] if "title" in row.keys() else "").startswith("终端现场"):
        return False

    event_type = (
        labels.get("事件类型")
        or labels.get("现场类型")
        or labels.get("event_type")
        or labels.get("type")
        or (parts[2] if len(parts) > 2 else "")
    )
    event_type = _text(event_type).lower()
    if event_type in PATH_RELATED_EVENT_TYPES:
        return True

    note = _text(labels.get("备注") or labels.get("现场记录") or "").lower()
    if note in PATH_RELATED_EVENT_NOTES:
        return True

    return False


def delete_path_related_terminal_event_journal_entries(device_id: str | None = None) -> int:
    """Delete legacy path-related terminal event rows from Journal only.

    This does not touch terminal_sync/archive, seen_ids, sync logs, audio rows,
    non-path terminal field notes, manual journal rows, or validated
    终端轨迹记录 rows. It removes old path/source rows that are now represented
    by terminal_track entries.
    """

    conn = get_db_connection()
    _ensure_journal_metadata_column(conn)
    rows = conn.execute(
        """
        SELECT id, entry_type, title, content, metadata_json FROM journal
        WHERE entry_type = ? OR title LIKE '终端现场%'
        """,
        (TERMINAL_EVENT_ENTRY_TYPE,),
    ).fetchall()

    delete_ids: list[int] = []
    for row in rows:
        labels = _content_labels(row["content"] if "content" in row.keys() else "")
        parts = _title_parts(row["title"] if "title" in row.keys() else "")
        row_device_id = labels.get("来源终端") or (parts[1] if len(parts) > 1 else "")
        if device_id and row_device_id != device_id:
            continue
        if _row_is_path_related_terminal_event(row):
            delete_ids.append(int(row["id"]))

    for entry_id in delete_ids:
        conn.execute("DELETE FROM journal WHERE id = ?", (entry_id,))

    conn.commit()
    conn.close()
    return len(delete_ids)

def _row_to_terminal_event_record(row: dict[str, Any]) -> tuple[str, dict[str, Any]]:
    labels = _content_labels(row.get("content"))
    parts = _title_parts(row.get("title"))
    device_id = labels.get("来源终端") or (parts[1] if len(parts) > 1 else "")
    record = {
        "event_type": labels.get("事件类型") or labels.get("现场类型") or (parts[2] if len(parts) > 2 else ""),
        "note": labels.get("备注") or labels.get("现场记录") or "",
        "device_timestamp": labels.get("记录时间") or labels.get("终端时间") or "",
        "session_id": labels.get("轨迹会话") or "",
        "mode": labels.get("模式") or "",
        "satellites": labels.get("卫星数") or "",
    }
    location = labels.get("位置") or labels.get("坐标") or ""
    if location and location != "无有效定位":
        pieces = [piece.strip() for piece in location.split(",")]
        if len(pieces) >= 2:
            record["lat"] = pieces[0]
            record["lon"] = pieces[1]
    elif location == "无有效定位":
        record["gnss_fix"] = False
    return device_id, record


def _row_to_terminal_audio_record(row: dict[str, Any]) -> tuple[str, str, dict[str, Any], str]:
    labels = _content_labels(row.get("content"))
    parts = _title_parts(row.get("title"))
    device_id = labels.get("来源终端") or (parts[1] if len(parts) > 1 else "")
    title_target = parts[2] if len(parts) > 2 else ""
    record = {
        "filename": labels.get("文件名") or title_target,
        "device_timestamp": labels.get("记录时间") or labels.get("终端时间") or "",
        "duration_sec": labels.get("时长") or "",
        "size": labels.get("文件大小") or "",
    }
    sync_session = labels.get("同步会话") or labels.get("同步批次") or ""
    return device_id, sync_session, record, title_target


def format_terminal_journal_entry_for_display(entry: dict[str, Any]) -> dict[str, Any]:
    entry_type = _text(entry.get("entry_type"))
    title = _text(entry.get("title"))

    if entry_type == TERMINAL_EVENT_ENTRY_TYPE or title.startswith("终端现场"):
        device_id, record = _row_to_terminal_event_record(entry)
        if not device_id:
            return entry
        created_at = _terminal_created_at(record, entry.get("created_at"))
        display_record = dict(record)
        if created_at and not format_journal_created_at(_terminal_time(display_record)):
            display_record["device_timestamp"] = created_at
        return {
            **entry,
            "title": _terminal_event_title(device_id, display_record),
            "content": _terminal_event_content(device_id, display_record),
            "created_at": created_at or entry.get("created_at"),
        }

    if entry_type == TERMINAL_AUDIO_ENTRY_TYPE or title.startswith("终端录音"):
        device_id, sync_session_id, record, fallback = _row_to_terminal_audio_record(entry)
        if not device_id:
            return entry
        created_at = _terminal_created_at(record, entry.get("created_at"))
        display_record = dict(record)
        if created_at and not format_journal_created_at(_terminal_time(display_record)):
            display_record["device_timestamp"] = created_at
        return {
            **entry,
            "title": _terminal_audio_title(device_id, display_record, fallback),
            "content": _terminal_audio_content(
                device_id=device_id,
                sync_session_id=sync_session_id,
                record=display_record,
            ),
            "created_at": created_at or entry.get("created_at"),
        }

    return entry


def _attach_journal_metadata(entry: dict[str, Any]) -> dict[str, Any]:
    raw = _text(entry.get("metadata_json"))
    if not raw:
        return entry
    try:
        metadata = json.loads(raw)
    except json.JSONDecodeError:
        return entry
    if not isinstance(metadata, dict):
        return entry

    result = dict(entry)
    result["metadata"] = metadata
    if result.get("entry_type") == TERMINAL_TRACK_ENTRY_TYPE:
        for key in (
            "device_id",
            "session_id",
            "summary",
            "start_point",
            "base_points",
            "end_point",
            "track_source",
            "quality",
        ):
            if key in metadata:
                result[key] = metadata[key]
    return result


def _journal_sort_key(entry: dict[str, Any]) -> tuple[datetime, int]:
    parsed = _parse_datetime(entry.get("created_at")) or datetime.min
    try:
        entry_id = int(entry.get("id") or 0)
    except (TypeError, ValueError):
        entry_id = 0
    return parsed, entry_id


def list_journal_entries() -> list[dict[str, Any]]:
    conn = get_db_connection()
    rows = conn.execute("SELECT * FROM journal ORDER BY id DESC").fetchall()
    conn.close()
    entries = [
        _attach_journal_metadata(format_terminal_journal_entry_for_display(dict(row)))
        for row in rows
    ]
    return sorted(entries, key=_journal_sort_key, reverse=True)


def create_journal_entry(
    *,
    entry_type: str = "日常记录",
    title: str = "",
    content: str = "",
    created_at: str | None = None,
    metadata: dict[str, Any] | None = None,
) -> dict[str, Any]:
    created_at_value = created_at or _now_journal_time()
    entry_type_value = entry_type or "日常记录"
    title_value = title or ""
    content_value = content or ""
    metadata_json = _json_dumps(metadata) if metadata else None

    conn = get_db_connection()
    _ensure_journal_metadata_column(conn)
    cursor = conn.execute(
        """
        INSERT INTO journal
        (entry_type, title, content, created_at, metadata_json)
        VALUES (?, ?, ?, ?, ?)
        """,
        (
            entry_type_value,
            title_value,
            content_value,
            created_at_value,
            metadata_json,
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
        "metadata_json": metadata_json,
        "metadata": metadata or {},
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
    return create_journal_entry(
        entry_type=TERMINAL_EVENT_ENTRY_TYPE,
        title=_terminal_event_title(device_id, record),
        content=_terminal_event_content(device_id, record),
        created_at=_terminal_created_at(record, received_at),
    )


def create_journal_entry_from_terminal_audio_index(
    *,
    device_id: str,
    sync_session_id: str,
    record_id: str,
    record: dict[str, Any],
    received_at: str | None = None,
) -> dict[str, Any]:
    return create_journal_entry(
        entry_type=TERMINAL_AUDIO_ENTRY_TYPE,
        title=_terminal_audio_title(device_id, record, record_id),
        content=_terminal_audio_content(
            device_id=device_id,
            sync_session_id=sync_session_id,
            record=record,
        ),
        created_at=_terminal_created_at(record, received_at),
    )
