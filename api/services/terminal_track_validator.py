"""Terminal track validation and normalization helpers.

This module turns raw terminal path records into one clean, Core-facing track
object. Raw path_points / field_events remain in terminal_sync/archive as the
black-box source. Journal and future map/history modules should only consume
validated terminal_track objects produced here.
"""

from __future__ import annotations

import math
import re
from collections import Counter
from dataclasses import dataclass
from datetime import date, datetime, time, timedelta, timezone
from typing import Any

TRACK_VALIDATOR_VERSION = "terminal_track_validator_v0.2e"

MIN_TRACK_POINTS = 3
MIN_TRACK_DISTANCE_METERS = 10
MIN_TRACK_DURATION_SECONDS = 30
MAX_REASONABLE_SPEED_MPS = 12.0
# Duration and speed are kept as quality metadata. They are not hard gates
# because older FT-01 path samples can collapse timestamps while still carrying
# useful cumulative movement.

PATH_EVENT_TYPES = {
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

TRACK_CONTROL_NOTES = {
    "toggle",
    "enter recorder",
    "leave recorder",
    "no gnss fix",
    "manual test write from recorder screen",
}


@dataclass(frozen=True)
class TrackPoint:
    lat: float
    lon: float
    record: dict[str, Any]
    timestamp: datetime | None
    time_quality: str
    seq: int


@dataclass(frozen=True)
class ParsedTime:
    timestamp: datetime | None
    quality: str
    has_full_date: bool


def text(value: Any) -> str:
    if value is None:
        return ""
    return str(value).strip()


def first_text(record: dict[str, Any], keys: list[str]) -> str:
    for key in keys:
        value = text(record.get(key))
        if value:
            return value
    return ""


def record_session_id(record: dict[str, Any]) -> str:
    return first_text(record, ["session_id", "track_session_id", "session"])


def record_event_type(record: dict[str, Any]) -> str:
    return first_text(record, ["point_type", "event_type", "type", "category"]).lower()


def is_path_related_record(record: dict[str, Any]) -> bool:
    event_type = record_event_type(record)
    if event_type in PATH_EVENT_TYPES:
        return True
    note = first_text(record, ["note", "content", "message", "text", "description", "event"]).lower()
    return note in TRACK_CONTROL_NOTES


def track_kind(record: dict[str, Any], default_kind: str = "path_point") -> str:
    event_type = record_event_type(record)
    if event_type in {"session_start", "start"}:
        return "start"
    if event_type in {"base_mark", "base"}:
        return "base"
    if event_type in {"endpoint", "end", "session_end", "session_stop"}:
        return "end"
    if event_type in {"auto_track_on", "auto_track_off", "auto_track_toggle", "auto_track", "track_toggle"}:
        return "control"
    if event_type == "path_point":
        return "path_point"
    return default_kind


def _parse_iso_datetime(value: str) -> datetime | None:
    raw = text(value)
    if not raw:
        return None
    normalized = raw.replace("Z", "+00:00")
    variants = [
        normalized,
        normalized.replace("T", " "),
        normalized.split(".")[0].replace("T", " "),
    ]
    for candidate in variants:
        try:
            parsed = datetime.fromisoformat(candidate)
        except ValueError:
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
            return datetime.strptime(raw[:length], fmt)
        except ValueError:
            continue
    return None


def _parse_date_value(value: Any) -> date | None:
    raw = text(value)
    if not raw:
        return None
    raw = raw.split("T", 1)[0].strip()
    for fmt in ("%Y-%m-%d", "%Y/%m/%d"):
        try:
            return datetime.strptime(raw[:10], fmt).date()
        except ValueError:
            pass

    digits = re.sub(r"\D", "", raw)
    if len(digits) == 8:
        if digits.startswith("20"):
            try:
                return datetime.strptime(digits, "%Y%m%d").date()
            except ValueError:
                return None
        try:
            return datetime.strptime(digits, "%d%m%Y").date()
        except ValueError:
            return None
    if len(digits) == 6:
        # GNSS/NMEA date is commonly DDMMYY.
        try:
            return datetime.strptime(digits, "%d%m%y").date()
        except ValueError:
            return None
    return None


def _parse_time_value(value: Any) -> time | None:
    raw = text(value)
    if not raw:
        return None
    raw = raw.split("T")[-1].strip()
    raw = raw.rstrip("Z")
    raw = raw.split("+", 1)[0].split("-", 1)[0].strip() if ":" in raw else raw

    for fmt in ("%H:%M:%S", "%H:%M"):
        try:
            return datetime.strptime(raw[:8 if fmt.endswith('%S') else 5], fmt).time()
        except ValueError:
            pass

    digits = re.sub(r"[^0-9.]", "", raw)
    if "." in digits:
        digits = digits.split(".", 1)[0]
    if len(digits) >= 6:
        try:
            return datetime.strptime(digits[:6], "%H%M%S").time()
        except ValueError:
            return None
    if len(digits) == 4:
        try:
            return datetime.strptime(digits, "%H%M").time()
        except ValueError:
            return None
    return None


def _timezone_offset_minutes(value: Any) -> int:
    raw = text(value).upper().replace("UTC", "").replace("GMT", "").strip()
    if not raw:
        return 0
    if raw in {"Z", "+00", "+0000", "+00:00", "0"}:
        return 0
    match = re.match(r"^([+-]?)(\d{1,2})(?::?(\d{2}))?$", raw)
    if not match:
        return 0
    sign_text, hour_text, minute_text = match.groups()
    sign = -1 if sign_text == "-" else 1
    try:
        hours = int(hour_text)
        minutes = int(minute_text or "0")
    except ValueError:
        return 0
    if hours > 14 or minutes >= 60:
        return 0
    return sign * (hours * 60 + minutes)


def _combine_date_time(day: date | None, clock: time | None) -> datetime | None:
    if day is None or clock is None:
        return None
    return datetime.combine(day, clock.replace(tzinfo=None))


def _parse_direct_terminal_time(record: dict[str, Any]) -> ParsedTime:
    # Full terminal timestamps first. These are trusted terminal-origin times.
    for key in ["device_timestamp", "timestamp", "created_at"]:
        raw = first_text(record, [key])
        parsed = _parse_iso_datetime(raw)
        if parsed is not None:
            return ParsedTime(parsed, f"terminal_{key}", True)

    device_date = _parse_date_value(first_text(record, ["device_date", "date"]))
    device_time = _parse_time_value(first_text(record, ["device_time", "time"]))
    parsed = _combine_date_time(device_date, device_time)
    if parsed is not None:
        return ParsedTime(parsed, "terminal_device_datetime", True)

    gnss_date = _parse_date_value(first_text(record, ["gnss_utc_date", "utc_date"]))
    gnss_time = _parse_time_value(first_text(record, ["gnss_utc_time", "utc_time"]))
    gnss_dt = _combine_date_time(gnss_date, gnss_time)
    if gnss_dt is not None:
        offset_minutes = _timezone_offset_minutes(record.get("timezone"))
        local_dt = gnss_dt.replace(tzinfo=timezone.utc) + timedelta(minutes=offset_minutes)
        return ParsedTime(local_dt.replace(tzinfo=None), "terminal_gnss_utc", True)

    return ParsedTime(None, "missing_date", False)


def _infer_session_date(records: list[dict[str, Any]]) -> date | None:
    dates: list[date] = []
    for record in records:
        parsed = _parse_direct_terminal_time(record)
        if parsed.timestamp is not None and parsed.has_full_date:
            dates.append(parsed.timestamp.date())
    if not dates:
        return None
    return Counter(dates).most_common(1)[0][0]


def _parse_with_session_date(record: dict[str, Any], session_date: date | None) -> ParsedTime:
    direct = _parse_direct_terminal_time(record)
    if direct.timestamp is not None:
        return direct

    if session_date is None:
        return direct

    clock = _parse_time_value(first_text(record, ["device_time", "time", "gnss_utc_time", "utc_time"]))
    inferred = _combine_date_time(session_date, clock)
    if inferred is not None:
        return ParsedTime(inferred, "session_date_inferred", True)

    return direct


def _attach_track_times(path_records: list[dict[str, Any]], event_records: list[dict[str, Any]]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    all_records = path_records + event_records
    session_date = _infer_session_date(all_records)

    def attach(record: dict[str, Any]) -> dict[str, Any]:
        parsed = _parse_with_session_date(record, session_date)
        item = dict(record)
        item["_track_timestamp"] = parsed.timestamp
        item["_time_quality"] = parsed.quality
        return item

    return [attach(record) for record in path_records], [attach(record) for record in event_records]


def parse_record_datetime(record: dict[str, Any]) -> datetime | None:
    timestamp = record.get("_track_timestamp")
    if isinstance(timestamp, datetime):
        return timestamp
    # Deliberately do not use core_received_at here. Core receive time is an
    # archive/import fact, not the time when the terminal collected the track.
    return _parse_direct_terminal_time(record).timestamp


def record_time_quality(record: dict[str, Any]) -> str:
    return text(record.get("_time_quality")) or _parse_direct_terminal_time(record).quality


def format_track_time(value: datetime | None) -> str:
    if value is None:
        return ""
    return value.strftime("%Y-%m-%d %H:%M:%S")


def coordinate_point(record: dict[str, Any]) -> dict[str, float] | None:
    lat = record.get("lat", record.get("latitude"))
    lon = record.get("lon", record.get("longitude"))
    try:
        lat_num = float(lat)
        lon_num = float(lon)
    except (TypeError, ValueError):
        return None
    if lat_num == 0 and lon_num == 0:
        return None
    if not (-90 <= lat_num <= 90 and -180 <= lon_num <= 180):
        return None
    return {"lat": lat_num, "lon": lon_num}


def record_seq(record: dict[str, Any]) -> int:
    for key in ("seq", "order", "index"):
        try:
            return int(record.get(key))
        except (TypeError, ValueError):
            continue
    return 0


def haversine_meters(a: dict[str, float], b: dict[str, float]) -> float:
    radius = 6371000
    lat1 = math.radians(a["lat"])
    lat2 = math.radians(b["lat"])
    delta_lat = math.radians(b["lat"] - a["lat"])
    delta_lon = math.radians(b["lon"] - a["lon"])
    h = (
        math.sin(delta_lat / 2) ** 2
        + math.cos(lat1) * math.cos(lat2) * math.sin(delta_lon / 2) ** 2
    )
    return 2 * radius * math.asin(math.sqrt(h))


def _dedupe_movement_points(points: list[TrackPoint]) -> list[TrackPoint]:
    deduped: list[TrackPoint] = []
    seen: set[tuple[int, str, str, str]] = set()
    for point in points:
        timestamp_key = point.timestamp.isoformat() if point.timestamp else ""
        key = (point.seq, f"{point.lat:.7f}", f"{point.lon:.7f}", timestamp_key)
        if key in seen:
            continue
        seen.add(key)
        deduped.append(point)
    return deduped


def _sort_records(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    def sort_key(record: dict[str, Any]) -> tuple[datetime, int, str]:
        parsed = parse_record_datetime(record) or datetime.min
        return parsed, record_seq(record), record_event_type(record)

    return sorted(records, key=sort_key)


def _movement_points(records: list[dict[str, Any]]) -> list[TrackPoint]:
    points: list[TrackPoint] = []
    for record in _sort_records(records):
        kind = track_kind(record, "path_point")
        if kind != "path_point":
            continue
        coord = coordinate_point(record)
        if coord is None:
            continue
        points.append(
            TrackPoint(
                lat=coord["lat"],
                lon=coord["lon"],
                record=record,
                timestamp=parse_record_datetime(record),
                time_quality=record_time_quality(record),
                seq=record_seq(record),
            )
        )
    return _dedupe_movement_points(points)


def _distance(points: list[TrackPoint]) -> float:
    total = 0.0
    for previous, current in zip(points, points[1:]):
        total += haversine_meters(
            {"lat": previous.lat, "lon": previous.lon},
            {"lat": current.lat, "lon": current.lon},
        )
    return total


def _node_points(records: list[dict[str, Any]], kind: str) -> list[dict[str, float]]:
    points: list[dict[str, float]] = []
    for record in _sort_records(records):
        if track_kind(record) != kind:
            continue
        coord = coordinate_point(record)
        if coord:
            points.append(coord)
    return points


def _time_quality_summary(points: list[TrackPoint]) -> dict[str, Any]:
    qualities = Counter(point.time_quality or "missing_date" for point in points)
    timed_count = sum(1 for point in points if point.timestamp is not None)
    inferred_count = qualities.get("session_date_inferred", 0)
    if timed_count == 0:
        quality = "missing"
    elif inferred_count:
        quality = "session_date_inferred"
    elif len(qualities) == 1:
        quality = next(iter(qualities.keys()))
    else:
        quality = "mixed_terminal_time"
    return {
        "time_quality": quality,
        "time_quality_counts": dict(qualities),
        "timed_point_count": timed_count,
        "time_inferred_point_count": inferred_count,
    }


def _invalid(reason: str, **extra: Any) -> dict[str, Any]:
    return {
        "entry_type": "terminal_track",
        "valid": False,
        "invalid_reason": reason,
        "quality": {
            "valid": False,
            "reason": reason,
            "validator_version": TRACK_VALIDATOR_VERSION,
            "min_point_count": MIN_TRACK_POINTS,
            "min_distance_meters": MIN_TRACK_DISTANCE_METERS,
            "min_duration_seconds": MIN_TRACK_DURATION_SECONDS,
            "max_reasonable_speed_mps": MAX_REASONABLE_SPEED_MPS,
            **extra,
        },
    }


def build_valid_terminal_track(
    *,
    device_id: str,
    session_id: str,
    path_point_records: list[dict[str, Any]],
    field_event_records: list[dict[str, Any]],
    track_source: str,
) -> dict[str, Any] | None:
    """Return a clean terminal_track dict, or None when the session is not useful.

    A valid Core-facing track must have enough real movement points and useful
    cumulative movement. Raw input records are intentionally not copied into
    metadata_json; only map-ready summaries and node references are stored.
    """

    normalized_path_records = []
    for record in path_point_records:
        item = dict(record)
        item["_track_kind"] = track_kind(item, "path_point")
        item["_source_record_type"] = "path_points"
        normalized_path_records.append(item)

    normalized_event_records = []
    for record in field_event_records:
        if not is_path_related_record(record):
            continue
        item = dict(record)
        item["_track_kind"] = track_kind(item)
        item["_source_record_type"] = "field_events"
        normalized_event_records.append(item)

    normalized_path_records, normalized_event_records = _attach_track_times(
        normalized_path_records,
        normalized_event_records,
    )

    all_records = normalized_path_records + normalized_event_records
    if not all_records:
        return None

    movement = _movement_points(normalized_path_records)
    point_count = len(movement)
    if point_count < MIN_TRACK_POINTS:
        return None

    timed_points = [point for point in movement if point.timestamp is not None]
    start_time = timed_points[0].timestamp if timed_points else None
    end_time = timed_points[-1].timestamp if timed_points else None

    raw_duration_seconds: int | None = None
    duration_seconds: int | None = None
    duration_reliable = False
    if start_time is not None and end_time is not None:
        raw_duration_seconds = max(0, int(round((end_time - start_time).total_seconds())))
        if raw_duration_seconds >= MIN_TRACK_DURATION_SECONDS:
            duration_seconds = raw_duration_seconds
            duration_reliable = True

    distance_meters = _distance(movement)
    if distance_meters < MIN_TRACK_DISTANCE_METERS:
        return None

    average_speed = None
    speed_reasonable = None
    if duration_reliable and duration_seconds is not None and duration_seconds > 0:
        average_speed = distance_meters / duration_seconds
        speed_reasonable = average_speed <= MAX_REASONABLE_SPEED_MPS

    explicit_starts = _node_points(all_records, "start")
    explicit_ends = _node_points(all_records, "end")
    base_points = _node_points(all_records, "base")

    start_point = explicit_starts[0] if explicit_starts else {"lat": movement[0].lat, "lon": movement[0].lon}
    end_point = explicit_ends[-1] if explicit_ends else {"lat": movement[-1].lat, "lon": movement[-1].lon}
    time_summary = _time_quality_summary(movement)

    return {
        "entry_type": "terminal_track",
        "valid": True,
        "device_id": device_id,
        "session_id": session_id,
        "summary": {
            "point_count": point_count,
            "start_time": format_track_time(start_time),
            "end_time": format_track_time(end_time),
            "duration_seconds": duration_seconds,
            "raw_duration_seconds": raw_duration_seconds,
            "duration_reliable": duration_reliable,
            "distance_meters": int(round(distance_meters)),
            "average_speed_mps": round(average_speed, 2) if average_speed is not None else None,
            **time_summary,
        },
        "start_point": start_point or {},
        "base_points": base_points,
        "end_point": end_point or {},
        "track_source": track_source,
        "quality": {
            "valid": True,
            "validator_version": TRACK_VALIDATOR_VERSION,
            "min_point_count": MIN_TRACK_POINTS,
            "min_distance_meters": MIN_TRACK_DISTANCE_METERS,
            "min_duration_seconds": MIN_TRACK_DURATION_SECONDS,
            "max_reasonable_speed_mps": MAX_REASONABLE_SPEED_MPS,
            "duration_is_hard_gate": False,
            "speed_is_hard_gate": False,
            "speed_reasonable": speed_reasonable,
            **time_summary,
        },
    }


def validate_track_records(
    *,
    path_point_records: list[dict[str, Any]],
    field_event_records: list[dict[str, Any]],
) -> dict[str, Any]:
    track = build_valid_terminal_track(
        device_id="validator",
        session_id="validator",
        path_point_records=path_point_records,
        field_event_records=field_event_records,
        track_source="validator",
    )
    if track is None:
        normalized_path_records, normalized_event_records = _attach_track_times(
            [dict(record) for record in path_point_records],
            [dict(record) for record in field_event_records],
        )
        movement = _movement_points(normalized_path_records)
        point_count = len(movement)
        timed_points = [point for point in movement if point.timestamp is not None]
        duration_seconds = 0
        if len(timed_points) >= 2 and timed_points[0].timestamp and timed_points[-1].timestamp:
            duration_seconds = int(round((timed_points[-1].timestamp - timed_points[0].timestamp).total_seconds()))
        distance_meters = int(round(_distance(movement))) if movement else 0

        if point_count < MIN_TRACK_POINTS:
            reason = "not_enough_points"
        elif distance_meters < MIN_TRACK_DISTANCE_METERS:
            reason = "no_meaningful_movement"
        else:
            reason = "invalid_track"

        return _invalid(
            reason,
            point_count=point_count,
            duration_seconds=duration_seconds,
            distance_meters=distance_meters,
            **_time_quality_summary(movement),
        )

    return {
        "valid": True,
        "reason": "",
        "point_count": track["summary"]["point_count"],
        "distance_meters": track["summary"]["distance_meters"],
        "duration_seconds": track["summary"]["duration_seconds"],
        "quality": track["quality"],
    }
