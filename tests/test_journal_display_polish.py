from api.services.journal_service import (
    TERMINAL_AUDIO_ENTRY_TYPE,
    TERMINAL_EVENT_ENTRY_TYPE,
    create_journal_entry_from_terminal_audio_index,
    create_journal_entry_from_terminal_event,
    extract_filename,
    format_duration,
    format_file_size,
    format_journal_created_at,
    format_sync_session,
    format_terminal_journal_entry_for_display,
)


def test_terminal_audio_display_hides_raw_sync_fields_for_legacy_entries():
    entry = {
        "id": 1,
        "entry_type": TERMINAL_AUDIO_ENTRY_TYPE,
        "title": "终端录音日志｜FT01-0001｜/lanternbox/audio/audio_002.wav",
        "content": "\n".join(
            [
                "这是终端录音索引日志。",
                "录音文件可能尚未上传到 Core。",
                "来源终端：FT01-0001",
                "同步会话：FT01-0001-20260719-001735",
                "记录 ID：audio-index-002",
                "录音 ID：FT01-0001:audio:FT01-161833:audio_002.wav:196652",
                "文件名：/lanternbox/audio/audio_002.wav",
                "终端时间：2026-07-11T16:18:33.000000",
                "时长：dropped_chunks",
                "文件大小：196652",
                "Core 接收时间：2026-07-19T00:18:20.000000+08:00",
            ]
        ),
        "created_at": "2026-07-19 00:18:20",
    }

    result = format_terminal_journal_entry_for_display(entry)

    assert result["title"] == "终端录音｜FT01-0001｜audio_002.wav"
    assert result["created_at"] == "2026-07-11 16:18:33"
    assert "文件名：audio_002.wav" in result["content"]
    assert "记录时间：2026-07-11 16:18" in result["content"]
    assert "文件大小：192 KB" in result["content"]
    assert "时长：未知" in result["content"]
    assert "同步批次" not in result["content"]
    assert "记录 ID" not in result["content"]
    assert "录音 ID" not in result["content"]
    assert "Core 接收时间" not in result["content"]
    assert "/lanternbox/audio" not in result["content"]
    assert "dropped_chunks" not in result["content"]


def test_terminal_field_event_display_prefers_human_fields():
    entry = {
        "id": 2,
        "entry_type": TERMINAL_EVENT_ENTRY_TYPE,
        "title": "终端现场日志｜FT01-0001｜base position saved with a long note",
        "content": "\n".join(
            [
                "来源终端：FT01-0001",
                "现场类型：base_mark",
                "终端时间：2026-07-08T20:34:10",
                "坐标：31.217260, 121.381508",
                "卫星数：33",
                "现场记录：base position saved with a long note",
                "同步会话：FT01-0001-20260719-001735",
                "记录 ID：field-event-001",
            ]
        ),
        "created_at": "2026-07-19 00:18:20",
    }

    result = format_terminal_journal_entry_for_display(entry)

    assert result["title"] == "终端现场｜FT01-0001｜base position saved wi..."
    assert result["created_at"] == "2026-07-08 20:34:10"
    assert "事件类型：base_mark" in result["content"]
    assert "记录时间：2026-07-08 20:34" in result["content"]
    assert "位置：31.217260, 121.381508" in result["content"]
    assert "卫星数：33" in result["content"]
    assert "备注：base position saved with a long note" in result["content"]
    assert "同步会话" not in result["content"]
    assert "记录 ID" not in result["content"]


def test_terminal_event_no_fix_displays_no_valid_location():
    entry = {
        "entry_type": TERMINAL_EVENT_ENTRY_TYPE,
        "title": "终端现场｜FT01-0001｜GNSS NOFIX",
        "content": "\n".join(
            [
                "来源终端：FT01-0001",
                "事件类型：path_point",
                "记录时间：2026-07-08 20:34",
                "位置：无有效定位",
                "备注：GNSS NOFIX",
            ]
        ),
    }

    result = format_terminal_journal_entry_for_display(entry)

    assert "位置：无有效定位" in result["content"]


def test_legacy_time_only_terminal_event_uses_record_clock_time_for_created_at():
    entry = {
        "id": 3,
        "entry_type": TERMINAL_EVENT_ENTRY_TYPE,
        "title": "终端现场日志｜FT01-0001｜path_point｜14:05:56",
        "content": "\n".join(
            [
                "来源终端：FT01-0001",
                "终端时间：14:05:56",
                "现场类型：path_point",
                "现场记录：auto",
                "坐标：31.213531, 121.377881",
            ]
        ),
        "created_at": "2026-07-18 23:37:53",
    }

    result = format_terminal_journal_entry_for_display(entry)

    assert result["created_at"] == "2026-07-18 14:05:56"
    assert "记录时间：2026-07-18 14:05" in result["content"]


def test_already_polished_audio_entry_keeps_friendly_sync_batch_and_size():
    entry = {
        "entry_type": TERMINAL_AUDIO_ENTRY_TYPE,
        "title": "终端录音｜FT01-0001｜audio_002.wav",
        "content": "\n".join(
            [
                "这是终端录音索引日志。",
                "录音文件可能尚未上传到 Core。",
                "来源终端：FT01-0001",
                "文件名：audio_002.wav",
                "记录时间：2026-07-11 16:18",
                "文件大小：192 KB",
                "时长：未知",
                "同步批次：2026-07-19 00:17",
            ]
        ),
    }

    result = format_terminal_journal_entry_for_display(entry)

    assert "文件大小：192 KB" in result["content"]
    assert "同步批次" not in result["content"]


def test_friendly_terminal_format_helpers():
    assert extract_filename("/lanternbox/audio/audio_002.wav") == "audio_002.wav"
    assert extract_filename("FT01-0001:audio:FT01-161833:audio_002.wav:196652") == "audio_002.wav"
    assert format_file_size(196652) == "192 KB"
    assert format_file_size("192 KB") == "192 KB"
    assert format_file_size("8B") == "8 B"
    assert format_duration("6.14") == "6.1秒"
    assert format_duration("72") == "1分12秒"
    assert format_duration("dropped_chunks") == "未知"
    assert format_journal_created_at("2026-07-11T16:18:33.000000") == "2026-07-11 16:18:33"
    assert format_sync_session("FT01-0001-20260719-001735") == "2026-07-19 00:17"


def test_manual_journal_entry_is_not_reformatted():
    entry = {
        "entry_type": "日常记录",
        "title": "手写记录",
        "content": "今天整理了物资。",
        "created_at": "2026-07-19 10:00:00",
    }

    assert format_terminal_journal_entry_for_display(entry) == entry


def test_new_terminal_entries_store_record_time_as_created_at(monkeypatch):
    calls = []

    def fake_create_journal_entry(**kwargs):
        calls.append(kwargs)
        return kwargs

    monkeypatch.setattr(
        "api.services.journal_service.create_journal_entry",
        fake_create_journal_entry,
    )

    create_journal_entry_from_terminal_event(
        device_id="FT01-0001",
        sync_session_id="FT01-0001-20260719-001735",
        record_id="event-1",
        record={
            "event_type": "base_mark",
            "device_date": "2026-07-08",
            "device_time": "20:34:10",
            "note": "base position saved",
        },
        received_at="2026-07-19T00:17:35+08:00",
    )
    create_journal_entry_from_terminal_audio_index(
        device_id="FT01-0001",
        sync_session_id="FT01-0001-20260719-001735",
        record_id="audio-1",
        record={
            "filename": "/lanternbox/audio/audio_002.wav",
            "timestamp": "2026-07-11T16:18:33.000000",
            "duration": "6.14",
        },
        received_at="2026-07-19T00:17:35+08:00",
    )

    assert calls[0]["created_at"] == "2026-07-08 20:34:10"
    assert calls[1]["created_at"] == "2026-07-11 16:18:33"
