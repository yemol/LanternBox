import pytest

from scripts.ft01_usb_serial_helper import (
    END_MARKER,
    BEGIN_MARKER,
    ManifestCaptureError,
    RECORDS_BEGIN_MARKER,
    RECORDS_END_MARKER,
    RecordsCaptureError,
    capture_manifest_from_lines,
    capture_records_from_lines,
    manifest_url,
    normalize_manifest,
    parse_jsonl_records,
    parse_record_types,
    record_request_command,
    upload_records_url,
)


def test_capture_manifest_between_markers_ignores_serial_noise():
    lines = [
        "",
        "FT01_SYNC_HELLO",
        "DEVICE_ID FT01-0001",
        "GNSS NOFIX",
        BEGIN_MARKER,
        "{",
        '  "device_id":"FT01-0001",',
        '  "firmware_version":"0.4.0f",',
        '  "sync_session_id":"FT01-0001-20260718-001",',
        '  "transport":"usb_serial",',
        '  "items":{',
        '    "path_points":{"count":3},',
        '    "field_events":{"count":1},',
        '    "boot_logs":{"count":2},',
        '    "audio_index":{"count":0},',
        '    "audio_files":[]',
        "  }",
        "}",
        END_MARKER,
        "GNSS 39.1,116.3",
    ]

    result = capture_manifest_from_lines(lines)

    assert result.manifest == {
        "device_id": "FT01-0001",
        "firmware_version": "0.4.0f",
        "sync_session_id": "FT01-0001-20260718-001",
        "transport": "usb_serial",
        "items": {
            "path_points": {"count": 3},
            "field_events": {"count": 1},
            "boot_logs": {"count": 2},
            "audio_index": {"count": 0},
            "audio_files": [],
        },
    }


def test_capture_manifest_rejects_missing_end_marker():
    with pytest.raises(ManifestCaptureError, match="end marker"):
        capture_manifest_from_lines([BEGIN_MARKER, '{"device_id":"FT01-0001"}'])


def test_capture_manifest_rejects_invalid_json():
    with pytest.raises(ManifestCaptureError, match="JSON parse error"):
        capture_manifest_from_lines([BEGIN_MARKER, "{not-json", END_MARKER])


def test_normalize_manifest_defaults_transport_and_firmware_version():
    manifest = normalize_manifest(
        {
            "device_id": " FT01-0001 ",
            "sync_session_id": " session-1 ",
            "items": {"path_points": {"count": 0}},
        }
    )

    assert manifest == {
        "device_id": "FT01-0001",
        "firmware_version": "",
        "sync_session_id": "session-1",
        "transport": "usb_serial",
        "items": {"path_points": {"count": 0}},
    }


def test_normalize_manifest_requires_core_fields():
    with pytest.raises(ManifestCaptureError, match="device_id"):
        normalize_manifest({"sync_session_id": "session-1", "items": {}})

    with pytest.raises(ManifestCaptureError, match="sync_session_id"):
        normalize_manifest({"device_id": "FT01-0001", "items": {}})

    with pytest.raises(ManifestCaptureError, match="items"):
        normalize_manifest(
            {
                "device_id": "FT01-0001",
                "sync_session_id": "session-1",
                "items": [],
            }
        )


def test_manifest_url_joins_api_base():
    assert manifest_url("http://127.0.0.1:8000") == "http://127.0.0.1:8000/api/terminal-sync/manifest"
    assert manifest_url("http://127.0.0.1:8000/") == "http://127.0.0.1:8000/api/terminal-sync/manifest"


def test_capture_records_between_markers_ignores_other_serial_lines():
    lines = [
        "FT01_SYNC_HELLO",
        f"{RECORDS_BEGIN_MARKER} path_points",
        '{"device_id":"FT01-0001","session_id":"s1","seq":1,"lat":39.1,"lon":116.3}',
        "",
        '{"device_id":"FT01-0001","session_id":"s1","seq":2,"lat":39.2,"lon":116.4}',
        f"{RECORDS_END_MARKER} path_points",
        "GNSS NOFIX",
    ]

    result = capture_records_from_lines(lines, "path_points")

    assert result.record_type == "path_points"
    assert result.records == [
        {"device_id": "FT01-0001", "session_id": "s1", "seq": 1, "lat": 39.1, "lon": 116.3},
        {"device_id": "FT01-0001", "session_id": "s1", "seq": 2, "lat": 39.2, "lon": 116.4},
    ]


def test_capture_records_waits_for_expected_record_type():
    lines = [
        f"{RECORDS_BEGIN_MARKER} field_events",
        '{"event_type":"note"}',
        f"{RECORDS_END_MARKER} field_events",
        f"{RECORDS_BEGIN_MARKER}:boot_logs",
        '{"firmware_version":"0.4.0f"}',
        f"{RECORDS_END_MARKER}:boot_logs",
    ]

    result = capture_records_from_lines(lines, "boot_logs")

    assert result.records == [{"firmware_version": "0.4.0f"}]


def test_capture_records_allows_empty_jsonl_stream():
    result = capture_records_from_lines(
        [
            f"{RECORDS_BEGIN_MARKER} audio_index",
            "",
            f"{RECORDS_END_MARKER} audio_index",
        ],
        "audio_index",
    )

    assert result.records == []


def test_capture_records_rejects_invalid_jsonl():
    with pytest.raises(RecordsCaptureError, match="JSONL parse error"):
        capture_records_from_lines(
            [
                f"{RECORDS_BEGIN_MARKER} field_events",
                "{bad-json",
                f"{RECORDS_END_MARKER} field_events",
            ],
            "field_events",
        )

    with pytest.raises(RecordsCaptureError, match="JSON object"):
        parse_jsonl_records(["[]"], "field_events")


def test_record_type_parsing_and_validation():
    assert parse_record_types("path_points, field_events，boot_logs、audio_index") == [
        "path_points",
        "field_events",
        "boot_logs",
        "audio_index",
    ]

    with pytest.raises(RecordsCaptureError, match="unsupported record_type"):
        parse_record_types("path_points,clear")


def test_record_request_command_is_read_only():
    assert record_request_command("GET_RECORDS {record_type}", "boot_logs") == "GET_RECORDS boot_logs"

    with pytest.raises(RecordsCaptureError, match="refusing unsafe"):
        record_request_command("CLEAR_RECORDS {record_type}", "path_points")

    with pytest.raises(RecordsCaptureError, match="refusing unsafe"):
        record_request_command("terminal_may_clear {record_type}", "field_events")


def test_upload_records_url_joins_api_base():
    assert upload_records_url("http://127.0.0.1:8000") == "http://127.0.0.1:8000/api/terminal-sync/upload-records"
    assert upload_records_url("http://127.0.0.1:8000/") == "http://127.0.0.1:8000/api/terminal-sync/upload-records"
