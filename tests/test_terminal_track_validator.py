from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from api.services.terminal_track_validator import (
    build_valid_terminal_track,
    is_path_related_record,
    validate_track_records,
)


def point(seq, lat, lon, time):
    return {
        "session_id": "S1",
        "seq": seq,
        "lat": lat,
        "lon": lon,
        "device_date": "2026-07-19",
        "device_time": time,
    }


def test_single_point_invalid():
    result = validate_track_records(
        path_point_records=[point(1, 31.0, 121.0, "10:00:00")],
        field_event_records=[],
    )
    assert result["valid"] is False
    assert result["quality"]["reason"] == "not_enough_points"


def test_static_points_invalid():
    records = [
        point(1, 31.0, 121.0, "10:00:00"),
        point(2, 31.0, 121.0, "10:01:00"),
        point(3, 31.0, 121.0, "10:02:00"),
    ]
    result = validate_track_records(path_point_records=records, field_event_records=[])
    assert result["valid"] is False
    assert result["quality"]["reason"] == "no_meaningful_movement"


def test_short_duration_is_valid_when_cumulative_distance_is_useful():
    records = [
        point(1, 31.0, 121.0, "10:00:00"),
        point(2, 31.001, 121.001, "10:00:01"),
        point(3, 31.002, 121.002, "10:00:03"),
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="terminal_sync/archive/FT01-0001/path_points.jsonl",
    )
    assert track is not None
    assert track["valid"] is True
    assert track["summary"]["distance_meters"] >= 10
    assert track["summary"]["duration_seconds"] is None
    assert track["summary"]["raw_duration_seconds"] == 3
    assert track["summary"]["duration_reliable"] is False


def test_loop_track_valid_even_when_start_and_end_are_close():
    records = [
        point(1, 31.0000, 121.0000, "10:00:00"),
        point(2, 31.0010, 121.0000, "10:01:00"),
        point(3, 31.0010, 121.0010, "10:02:00"),
        point(4, 31.0000, 121.0010, "10:03:00"),
        point(5, 31.0000, 121.0000, "10:04:00"),
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="terminal_sync/archive/FT01-0001/path_points.jsonl",
    )
    assert track is not None
    assert track["valid"] is True
    assert track["summary"]["point_count"] == 5
    assert track["summary"]["distance_meters"] >= 300
    assert track["start_point"] == track["end_point"]


def test_valid_track_builds_clean_metadata():
    records = [
        point(1, 31.0000, 121.0000, "10:00:00"),
        point(2, 31.0005, 121.0005, "10:01:00"),
        point(3, 31.0010, 121.0010, "10:02:00"),
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="terminal_sync/archive/FT01-0001/path_points.jsonl",
    )
    assert track is not None
    assert track["valid"] is True
    assert track["summary"]["point_count"] == 3
    assert track["summary"]["duration_seconds"] == 120
    assert track["summary"]["distance_meters"] >= 10
    assert track["start_point"]
    assert track["end_point"]


def test_auto_track_toggle_is_path_related_control_noise():
    assert is_path_related_record({"event_type": "auto_track_on", "note": "toggle"}) is True
    assert is_path_related_record({"event_type": "auto_track_off", "note": "toggle"}) is True


def test_core_received_at_is_not_used_as_track_time():
    records = [
        {
            "session_id": "S1",
            "seq": 1,
            "lat": 31.0000,
            "lon": 121.0000,
            "device_time": "10:00:00",
            "core_received_at": "2026-07-18T23:59:00+00:00",
        },
        {
            "session_id": "S1",
            "seq": 2,
            "lat": 31.0010,
            "lon": 121.0010,
            "device_time": "10:01:00",
            "core_received_at": "2026-07-18T23:59:01+00:00",
        },
        {
            "session_id": "S1",
            "seq": 3,
            "lat": 31.0020,
            "lon": 121.0020,
            "device_time": "10:02:00",
            "core_received_at": "2026-07-18T23:59:02+00:00",
        },
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="validator",
    )
    assert track is not None
    assert track["summary"]["start_time"] == ""
    assert track["summary"]["time_quality"] == "missing"


def test_session_date_is_inferred_for_time_only_points():
    records = [
        point(1, 31.0000, 121.0000, "10:00:00"),
        {"session_id": "S1", "seq": 2, "lat": 31.0010, "lon": 121.0010, "device_time": "10:01:00"},
        {"session_id": "S1", "seq": 3, "lat": 31.0020, "lon": 121.0020, "device_time": "10:02:00"},
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="validator",
    )
    assert track is not None
    assert track["summary"]["start_time"] == "2026-07-19 10:00:00"
    assert track["summary"]["end_time"] == "2026-07-19 10:02:00"
    assert track["summary"]["time_quality"] == "session_date_inferred"


def test_gnss_utc_date_time_with_timezone_builds_terminal_time():
    records = [
        {
            "session_id": "S1",
            "seq": 1,
            "lat": 31.0000,
            "lon": 121.0000,
            "gnss_utc_date": "20260719",
            "gnss_utc_time": "020000",
            "timezone": "UTC+8",
        },
        {
            "session_id": "S1",
            "seq": 2,
            "lat": 31.0010,
            "lon": 121.0010,
            "gnss_utc_date": "20260719",
            "gnss_utc_time": "020100",
            "timezone": "UTC+8",
        },
        {
            "session_id": "S1",
            "seq": 3,
            "lat": 31.0020,
            "lon": 121.0020,
            "gnss_utc_date": "20260719",
            "gnss_utc_time": "020200",
            "timezone": "UTC+8",
        },
    ]
    track = build_valid_terminal_track(
        device_id="FT01-0001",
        session_id="S1",
        path_point_records=records,
        field_event_records=[],
        track_source="validator",
    )
    assert track is not None
    assert track["summary"]["start_time"] == "2026-07-19 10:00:00"
    assert track["summary"]["time_quality"] == "terminal_gnss_utc"


def test_base_mark_failed_is_suppressed_as_track_control_noise():
    assert is_path_related_record({"event_type": "base_mark_failed", "note": "no gnss fix"}) is True


def test_storage_test_is_suppressed_as_development_noise():
    assert is_path_related_record({"event_type": "storage_test", "note": "manual test write from recorder screen"}) is True
