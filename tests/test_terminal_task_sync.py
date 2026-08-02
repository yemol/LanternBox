import json

from api import db
from api.models import TaskCreateRequest, TaskReportRequest, TerminalRegisterRequest
from api.services import task_service
from api.services.terminal_service import register_terminal_device
from api.services.terminal_usb_sync_job_service import extract_sync_summary
from scripts import ft01_usb_serial_helper as helper


def setup_task_db(monkeypatch, tmp_path):
    monkeypatch.setattr(db, "DB_PATH", tmp_path / "lanternbox-task-sync.db")
    db.init_db()
    register_terminal_device(
        TerminalRegisterRequest(
            device_id="FT01-0001",
            name="FT-01",
            firmware_version="v0.4.3-task-inbox-local",
        )
    )
    return task_service.create_task(
        TaskCreateRequest(
            title="检查水桶",
            description="检查桶盖和余量",
            assigned_to=["FT01-0001"],
        )
    )


def test_task_report_request_accepts_ft01_fields():
    payload = TaskReportRequest(
        report_id="FT01-0001:task_report:TASK-001:20260719-141233:completed",
        device_id="FT01-0001",
        task_id="TASK-001",
        status="completed",
        note="",
        device_date="2026-07-19",
        device_time="14:12:33",
        lat=None,
        lon=None,
        source="ft01",
    )

    assert payload.report_id.startswith("FT01-0001:task_report")
    assert payload.source == "ft01"


def test_task_report_is_idempotent_and_updates_status(monkeypatch, tmp_path):
    task = setup_task_db(monkeypatch, tmp_path)
    payload = TaskReportRequest(
        report_id=f"FT01-0001:task_report:{task.task_id}:20260719-141233:completed",
        device_id="FT01-0001",
        task_id=task.task_id,
        status="completed",
        note="done",
        device_date="2026-07-19",
        device_time="14:12:33",
        source="ft01",
    )

    first = task_service.record_task_report(payload)
    second = task_service.record_task_report(payload)
    updated = task_service.get_task(task.task_id)

    conn = db.get_db_connection()
    try:
        report_count = conn.execute("SELECT COUNT(*) AS count FROM task_reports").fetchone()["count"]
        event_count = conn.execute("SELECT COUNT(*) AS count FROM task_events WHERE event_type = 'terminal_report'").fetchone()["count"]
    finally:
        conn.close()

    assert first["duplicate"] is False
    assert second["duplicate"] is True
    assert first["ack"] is True
    assert second["ack"] is True
    assert updated.status == "completed"
    assert report_count == 1
    assert event_count == 1


def test_helper_upload_task_reports_counts_duplicates_and_failures(monkeypatch):
    class FakeClient:
        def get_records(self, record_type):
            assert record_type == "task_reports"
            return [
                {"report_id": "r1", "task_id": "TASK-1", "status": "completed"},
                {"report_id": "r2", "task_id": "TASK-2", "status": "blocked"},
                {"report_id": "r3", "task_id": "TASK-3", "status": "completed"},
            ]

    def fake_post(api_base, report, timeout):
        if report["report_id"] == "r2":
            return {"ok": True, "duplicate": True}
        if report["report_id"] == "r3":
            raise helper.SyncError("boom")
        return {"ok": True, "duplicate": False}

    monkeypatch.setattr(helper, "post_task_report", fake_post)

    summary = helper.upload_task_reports(FakeClient(), "http://core", "FT01-0001", 5)

    assert summary["received"] == 3
    assert summary["uploaded"] == 1
    assert summary["duplicates"] == 1
    assert summary["failed"] == 1
    assert summary["ack"] is False
    assert summary["errors"] == ["boom"]


def test_helper_pull_core_tasks_accepts_dict_and_normalizes(monkeypatch):
    monkeypatch.setattr(
        helper,
        "get_json",
        lambda api_base, path, timeout: {
            "tasks": [
                {
                    "task_id": "TASK-1",
                    "title": "巡查",
                    "description": "检查东侧",
                    "status": "assigned",
                    "priority": "high",
                    "revision": 2,
                    "updated_at": "2026-07-19T14:00:00Z",
                    "assigned_to": ["FT01-0001"],
                }
            ]
        },
    )

    tasks = helper.pull_core_tasks("http://core", "FT01-0001", 5)

    assert tasks == [
        {
            "task_id": "TASK-1",
            "title": "巡查",
            "description": "检查东侧",
            "status": "assigned",
            "priority": "high",
            "revision": 2,
            "updated_at": "2026-07-19T14:00:00Z",
            "assigned_to": ["FT01-0001"],
            "target": {},
            "tags": [],
        }
    ]


def test_helper_put_tasks_sends_jsonl_and_parses_ack():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=1 ok=true",
        "FT01_SYNC_TASKS_ACK received=1 stored=1 ok=true",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [{"task_id": "TASK-1", "title": "巡查", "assigned_to": ["FT01-0001"]}],
    )

    assert sent[0] == "PUT_TASKS_BEGIN 1"
    assert json.loads(sent[1])["task_id"] == "TASK-1"
    assert sent[2] == "PUT_TASKS_END"
    assert result["sent"] == 1
    assert result["expected"] == 1
    assert result["received"] == 1
    assert result["stored"] == 1
    assert result["ack"] is True


def test_helper_put_tasks_parses_stage_field():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=3 ok=true",
        "FT01_SYNC_TASKS_ACK received=3 stored=3 ok=true stage=buffered",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [
            {"task_id": "T1", "title": "t", "assigned_to": ["FT01-0001"]},
            {"task_id": "T2", "title": "t", "assigned_to": ["FT01-0001"]},
            {"task_id": "T3", "title": "t", "assigned_to": ["FT01-0001"]},
        ],
    )

    assert result["sent"] == 3
    assert result.get("stage") == "buffered"
    assert result["ack"] is True


def test_helper_put_tasks_parses_save_done():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=1 ok=true",
        "FT01_SYNC_TASKS_ACK received=1 stored=1 ok=true",
        "FT01_SYNC_TASKS_SAVE_DONE received=1 stored=1 ok=true",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [{"task_id": "TASK-1", "title": "巡查", "assigned_to": ["FT01-0001"]}],
    )

    assert result["ack"] is True
    assert result.get("save_done") is True
    assert result.get("save_stored") == 1


def test_terminal_usb_sync_job_summary_parses_task_lines():
    summary = extract_sync_summary(
        [
            "captured manifest device_id=FT01-0001 sync_session_id=s1",
            "task reports: received=2 uploaded=1 duplicates=1 failed=0 ack=True",
            "tasks download: pulled=3 sent=3 stored=3 ack=True",
        ]
    )

    assert summary["task_reports"] == {
        "received": 2,
        "uploaded": 1,
        "duplicates": 1,
        "failed": 0,
        "ack": True,
    }
    assert summary["tasks_download"] == {
        "pulled": 3,
        "sent": 3,
        "stored": 3,
        "ack": True,
    }


def test_terminal_usb_sync_job_summary_parses_extended_tasks_download():
    summary = extract_sync_summary(
        [
            "tasks download: pulled=3 sent=3 stored=3 ack=True save_done=True save_stored=3",
        ]
    )

    assert summary["tasks_download"]["pulled"] == 3
    assert summary["tasks_download"]["sent"] == 3
    assert summary["tasks_download"]["stored"] == 3
    assert summary["tasks_download"]["ack"] is True
    assert summary["tasks_download"]["save_done"] is True
    assert summary["tasks_download"]["save_stored"] == 3


def test_terminal_usb_sync_job_summary_parses_line_ack_fields():
    summary = extract_sync_summary(
        [
            "tasks download: pulled=3 sent=3 stored=3 ack=True line_ack=True line_acked=3 save_done=True save_stored=3",
        ]
    )

    assert summary["tasks_download"]["ack"] is True
    assert summary["tasks_download"]["line_ack"] is True
    assert summary["tasks_download"]["line_acked"] == 3
    assert summary["tasks_download"]["save_done"] is True
    assert summary["tasks_download"]["save_stored"] == 3


def test_terminal_usb_sync_job_summary_parses_cleanup_line():
    summary = extract_sync_summary(
        [
            "cleanup: cleared=path_points,field_events,task_reports ack=True",
        ]
    )

    assert summary["cleanup"] == {
        "requested": 3,
        "cleared": ["path_points", "field_events", "task_reports"],
        "failed": 0,
        "ack": True,
        "error": None,
    }


def test_terminal_usb_sync_job_summary_parses_cleanup_timeout_error():
    summary = extract_sync_summary(
        [
            "cleanup: requested=3 cleared=0 failed=3 ack=False error=cleanup_ack_timeout",
        ]
    )

    assert summary["cleanup"] == {
        "requested": 3,
        "cleared": 0,
        "failed": 3,
        "ack": False,
        "error": "cleanup_ack_timeout",
    }


def test_helper_clear_synced_records_timeout(monkeypatch):
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = []
    client.timeout = 1
    client.send_line = sent.append

    def reader(deadline):
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.clear_synced_records(client, ["path_points", "field_events", "task_reports"])

    assert sent[0] == "CLEAR_SYNCED_RECORDS path_points field_events task_reports"
    assert result["ack"] is False
    assert result["error"] == "cleanup_ack_timeout"


def test_terminal_usb_sync_job_summary_parses_cleanup_requested_line():
    summary = extract_sync_summary(
        [
            "cleanup: requested=5 cleared=5 failed=0 ack=True",
        ]
    )

    assert summary["cleanup"] == {
        "requested": 5,
        "cleared": 5,
        "failed": 0,
        "ack": True,
        "error": None,
    }


def test_helper_clear_synced_records_parses_ack():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_CLEANUP_ACK cleared=path_points,field_events,task_reports ack=True",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.clear_synced_records(client, ["path_points", "field_events", "task_reports"])

    assert sent[0] == "CLEAR_SYNCED_RECORDS path_points field_events task_reports"
    assert result["ack"] is True
    assert result["cleared"] == ["path_points", "field_events", "task_reports"]
    assert result["error"] is None


def test_helper_clear_synced_records_does_not_include_audio_index(monkeypatch):
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_CLEANUP_ITEM name=path_points cleared=true ok=true",
        "FT01_SYNC_CLEANUP_ITEM name=field_events cleared=true ok=true",
        "FT01_SYNC_CLEANUP_ITEM name=task_reports cleared=true ok=true",
        "FT01_SYNC_CLEANUP_ITEM name=boot_logs cleared=true ok=true retained=20",
        "FT01_SYNC_CLEANUP_ACK ok=true cleared=4 failed=0 requested=4",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.clear_synced_records(
        client,
        ["path_points", "field_events", "task_reports", "boot_logs"],
    )

    assert sent[0] == "CLEAR_SYNCED_RECORDS path_points field_events task_reports boot_logs"
    assert "audio/" not in sent[0]
    assert "tasks.jsonl" not in sent[0]
    assert result["ack"] is True
    assert result["cleared"] == 4
    assert result["failed"] == 0
    assert result["error"] is None


def test_delete_uploaded_audio_parses_ack(monkeypatch):
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_AUDIO_DELETE_ACK file=audio_001.wav wav_deleted=true index_removed=true ok=true",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.delete_uploaded_audio(client, "audio_001.wav")

    assert sent[0] == "DELETE_UPLOADED_AUDIO audio_001.wav"
    assert result["ok"] is True
    assert result["wav_deleted"] is True
    assert result["index_removed"] is True


def test_terminal_usb_sync_job_summary_parses_audio_cleanup():
    summary = extract_sync_summary(
        [
            "audio cleanup: requested=3 deleted=3 index_removed=3 failed=0 ack=True",
        ]
    )

    assert summary["audio_cleanup"] == {
        "requested": 3,
        "deleted": 3,
        "index_removed": 3,
        "failed": 0,
        "ack": True,
        "error": None,
    }


def test_helper_put_tasks_line_ack_success():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=3 ok=true protocol=line_ack",
        "FT01_SYNC_TASK_LINE_ACK index=1 ok=true",
        "FT01_SYNC_TASK_LINE_ACK index=2 ok=true",
        "FT01_SYNC_TASK_LINE_ACK index=3 ok=true",
        "FT01_SYNC_TASKS_ACK received=3 stored=3 ok=true stage=saved",
        "FT01_SYNC_TASKS_SAVE_DONE received=3 stored=3 ok=true",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [
            {"task_id": "T1", "title": "t1", "assigned_to": ["FT01-0001"]},
            {"task_id": "T2", "title": "t2", "assigned_to": ["FT01-0001"]},
            {"task_id": "T3", "title": "t3", "assigned_to": ["FT01-0001"]},
        ],
    )

    assert sent[0] == "PUT_TASKS_BEGIN 3"
    # three JSON lines then PUT_TASKS_END
    assert json.loads(sent[1])["task_id"] == "T1"
    assert json.loads(sent[2])["task_id"] == "T2"
    assert json.loads(sent[3])["task_id"] == "T3"
    assert sent[4] == "PUT_TASKS_END"
    assert result["sent"] == 3
    assert result["line_ack"] is True
    assert result["line_acked"] == 3
    assert result["ack"] is True
    assert result["stored"] == 3
    assert result.get("save_done") is True


def test_helper_put_tasks_line_ack_timeout():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=3 ok=true protocol=line_ack",
        "FT01_SYNC_TASK_LINE_ACK index=1 ok=true",
        # missing ack for index=2 -> will timeout
    ]
    client.timeout = 1
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [
            {"task_id": "T1", "title": "t1", "assigned_to": ["FT01-0001"]},
            {"task_id": "T2", "title": "t2", "assigned_to": ["FT01-0001"]},
            {"task_id": "T3", "title": "t3", "assigned_to": ["FT01-0001"]},
        ],
    )

    assert result["ack"] is False
    assert result["line_ack"] is True
    assert result["line_acked"] == 1
    assert result.get("error") == "task_line_ack_timeout"


def test_helper_put_tasks_compatible_with_old_protocol():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=2 ok=true",
        "FT01_SYNC_TASKS_ACK received=2 stored=2 ok=true",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [
            {"task_id": "A", "title": "a", "assigned_to": ["FT01-0001"]},
            {"task_id": "B", "title": "b", "assigned_to": ["FT01-0001"]},
        ],
    )

    assert result["ack"] is True
    assert result["line_ack"] is False


def test_helper_put_tasks_compact_task_and_status_mapping():
    task = {
        "task_id": "T1",
        "title": "巡查",
        "description": "检查东侧",
        "status": "assigned",
        "priority": "high",
        "revision": 2,
        "updated_at": "2026-07-19T14:00:00Z",
        "assigned_to": ["FT01-0001"],
        "target": {"foo": "bar"},
        "tags": ["a"],
    }
    compact = helper.compact_task_for_terminal(task)

    assert compact == {
        "task_id": "T1",
        "title": "巡查",
        "description": "检查东侧",
        "status": "pending",
        "priority": "high",
        "revision": 2,
    }


def test_helper_normalize_task_for_terminal_keeps_description_and_drops_extras():
    task = {
        "task_id": "T2",
        "title": "巡查",
        "description": "检查东侧",
        "status": "assigned",
        "priority": "normal",
        "revision": 3,
        "updated_at": "2026-07-19T14:00:00Z",
        "assigned_to": ["FT01-0001"],
        "target": {"foo": "bar"},
        "tags": ["a"],
    }
    normalized = helper.normalize_task_for_terminal(task)

    assert normalized == {
        "task_id": "T2",
        "title": "巡查",
        "description": "检查东侧",
        "status": "pending",
        "priority": "normal",
        "revision": 3,
    }


def test_helper_put_tasks_truncates_long_task_title_for_payload():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    long_title = "巡查" * 100
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=1 ok=true protocol=line_ack",
        "FT01_SYNC_TASK_LINE_ACK index=1 ok=true",
        "FT01_SYNC_TASKS_ACK received=1 stored=1 ok=true stage=saved",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [{"task_id": "T1", "title": long_title, "description": "长描述" * 50, "assigned_to": ["FT01-0001"]}],
    )

    assert result["ack"] is True
    assert result["line_ack"] is True
    assert result["line_acked"] == 1
    assert len(sent[1].encode("utf-8")) <= 520
    actual_task = json.loads(sent[1])
    assert actual_task["task_id"] == "T1"
    assert actual_task["title"]
    assert actual_task["description"]
    assert actual_task["description"].endswith("...")


def test_helper_put_tasks_returns_error_for_line_too_long():
    client = object.__new__(helper.SerialSyncClient)
    sent = []
    responses = [
        "FT01_SYNC_TASKS_BEGIN_ACK expected=1 ok=true protocol=line_ack",
    ]
    client.timeout = 5
    client.send_line = sent.append

    def reader(deadline):
        if responses:
            return responses.pop(0)
        raise helper.SyncError("Timed out waiting for FT-01 serial response")

    client.readline = reader

    result = helper.SerialSyncClient.put_tasks(
        client,
        [{"task_id": "X" * 600, "title": "T" * 240, "description": "D" * 240, "assigned_to": ["FT01-0001"]}],
    )

    assert result["ack"] is False
    assert result["line_ack"] is True
    assert result["error"] == "task_line_too_long"
