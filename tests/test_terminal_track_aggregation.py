from api.models import TerminalSyncUploadRecordsRequest
from api import db
from api.services.journal_service import (
    TERMINAL_TRACK_ENTRY_TYPE,
    list_journal_entries,
    upsert_journal_entry_from_terminal_track,
)
from api.services import terminal_sync_service


def _configure_tmp_sync(monkeypatch, tmp_path):
    sync_root = tmp_path / "terminal_sync"
    monkeypatch.setattr(terminal_sync_service, "SYNC_ROOT", sync_root)
    monkeypatch.setattr(terminal_sync_service, "MANIFEST_DIR", sync_root / "manifests")
    monkeypatch.setattr(terminal_sync_service, "ARCHIVE_DIR", sync_root / "archive")
    monkeypatch.setattr(terminal_sync_service, "SYNC_LOG_PATH", sync_root / "sync_log.jsonl")
    monkeypatch.setattr(terminal_sync_service, "_check_trusted_device", lambda device_id: None)
    monkeypatch.setattr(terminal_sync_service, "update_terminal_last_seen", lambda device_id: None)


def test_path_related_field_events_aggregate_to_one_track_journal(monkeypatch, tmp_path):
    _configure_tmp_sync(monkeypatch, tmp_path)
    terminal_events = []
    terminal_tracks = []

    monkeypatch.setattr(
        terminal_sync_service,
        "create_journal_entry_from_terminal_event",
        lambda **kwargs: terminal_events.append(kwargs),
    )
    monkeypatch.setattr(
        terminal_sync_service,
        "upsert_journal_entry_from_terminal_track",
        lambda **kwargs: terminal_tracks.append(kwargs),
    )

    payload = TerminalSyncUploadRecordsRequest(
        device_id="FT01-0001",
        sync_session_id="FT01-0001-20260719-001735",
        record_type="field_events",
        records=[
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "event_type": "session_start",
                "device_date": "2026-07-19",
                "device_time": "00:17:00",
                "lat": 31.1,
                "lon": 121.1,
                "note": "session started",
            },
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "event_type": "path_point",
                "device_date": "2026-07-19",
                "device_time": "00:18:00",
                "lat": 31.101,
                "lon": 121.101,
                "note": "auto",
            },
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "event_type": "session_stop",
                "device_date": "2026-07-19",
                "device_time": "00:19:00",
                "lat": 31.102,
                "lon": 121.102,
                "note": "leave recorder",
            },
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "event_type": "note",
                "device_date": "2026-07-19",
                "device_time": "00:20:00",
                "note": "checked the gate",
            },
        ],
    )

    result = terminal_sync_service.upload_records(payload)

    assert result["imported"] == 4
    assert len(terminal_events) == 1
    assert terminal_events[0]["record"]["event_type"] == "note"
    assert len(terminal_tracks) == 1
    assert terminal_tracks[0]["session_id"] == "FT01-105318"
    track = terminal_tracks[0]["track"]
    assert track["entry_type"] == "terminal_track"
    assert track["summary"]["point_count"] == 1
    assert track["summary"]["start_time"] == "2026-07-19 00:17:00"
    assert track["summary"]["end_time"] == "2026-07-19 00:19:00"
    assert track["start_point"] == {"lat": 31.1, "lon": 121.1}
    assert track["end_point"] == {"lat": 31.102, "lon": 121.102}
    assert "path_points.jsonl" in track["track_source"]

    duplicate = terminal_sync_service.upload_records(payload)

    assert duplicate["imported"] == 0
    assert duplicate["skipped_duplicate"] == 4
    assert len(terminal_events) == 1
    assert len(terminal_tracks) == 2
    assert terminal_tracks[1]["session_id"] == "FT01-105318"


def test_path_points_upload_aggregates_without_terminal_event_journals(monkeypatch, tmp_path):
    _configure_tmp_sync(monkeypatch, tmp_path)
    terminal_events = []
    terminal_tracks = []

    monkeypatch.setattr(
        terminal_sync_service,
        "create_journal_entry_from_terminal_event",
        lambda **kwargs: terminal_events.append(kwargs),
    )
    monkeypatch.setattr(
        terminal_sync_service,
        "upsert_journal_entry_from_terminal_track",
        lambda **kwargs: terminal_tracks.append(kwargs),
    )

    payload = TerminalSyncUploadRecordsRequest(
        device_id="FT01-0001",
        sync_session_id="FT01-0001-20260719-001735",
        record_type="path_points",
        records=[
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "seq": 1,
                "device_date": "2026-07-19",
                "device_time": "00:17:00",
                "lat": 31.1,
                "lon": 121.1,
            },
            {
                "device_id": "FT01-0001",
                "session_id": "FT01-105318",
                "seq": 2,
                "device_date": "2026-07-19",
                "device_time": "00:18:00",
                "lat": 31.101,
                "lon": 121.101,
            },
        ],
    )

    result = terminal_sync_service.upload_records(payload)

    assert result["imported"] == 2
    assert terminal_events == []
    assert len(terminal_tracks) == 1
    assert terminal_tracks[0]["track"]["summary"]["point_count"] == 2


def test_terminal_track_journal_upsert_updates_existing_session(monkeypatch, tmp_path):
    monkeypatch.setattr(db, "DB_PATH", tmp_path / "lanternbox.db")
    db.init_db()

    track = {
        "entry_type": "terminal_track",
        "device_id": "FT01-0001",
        "session_id": "FT01-105318",
        "summary": {
            "point_count": 2,
            "start_time": "2026-07-19 00:17:00",
            "end_time": "2026-07-19 00:18:00",
            "duration_seconds": 60,
            "distance_meters": 142,
        },
        "start_point": {"lat": 31.1, "lon": 121.1},
        "base_points": [],
        "end_point": {"lat": 31.101, "lon": 121.101},
        "track_source": "terminal_sync/archive/FT01-0001/path_points.jsonl",
    }

    first = upsert_journal_entry_from_terminal_track(
        device_id="FT01-0001",
        session_id="FT01-105318",
        track=track,
    )
    track["summary"]["point_count"] = 3
    second = upsert_journal_entry_from_terminal_track(
        device_id="FT01-0001",
        session_id="FT01-105318",
        track=track,
    )

    entries = [
        entry for entry in list_journal_entries()
        if entry.get("entry_type") == TERMINAL_TRACK_ENTRY_TYPE
    ]
    assert first["id"] == second["id"]
    assert len(entries) == 1
    assert entries[0]["title"] == "外出轨迹｜FT01-0001｜2026-07-19 00:17"
    assert "轨迹点：3" in entries[0]["content"]
    assert entries[0]["session_id"] == "FT01-105318"
    assert entries[0]["track_source"].endswith("path_points.jsonl")
