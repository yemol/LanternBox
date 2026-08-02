"""USB terminal sync jobs for the Core web UI.

The command-line helper remains the source of truth for the serial protocol.
This service wraps it in a supervised background job so the browser can start a
read-only terminal sync and poll progress without copying shell commands.
"""

from __future__ import annotations

import json
import subprocess
import sys
import threading
import time
import uuid
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any

from ..config import DATA_DIR

try:
    from serial.tools import list_ports
except Exception:  # pragma: no cover - surfaced through list_sync_ports
    list_ports = None


SYNC_ROOT = DATA_DIR / "terminal_sync"
SYNC_JOBS_FILE = SYNC_ROOT / "sync_jobs.jsonl"
MAX_LOG_LINES = 300


class TerminalUsbSyncError(Exception):
    pass


@dataclass
class TerminalUsbSyncJob:
    job_id: str
    port: str
    baud: int
    api_base: str
    upload_records: bool
    upload_audio_files: bool
    sync_tasks: bool
    status: str = "queued"
    returncode: int | None = None
    created_at: str = field(default_factory=lambda: datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    updated_at: str = field(default_factory=lambda: datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    started_at: str = ""
    finished_at: str = ""
    stdout_lines: list[str] = field(default_factory=list)
    stderr_lines: list[str] = field(default_factory=list)
    error: str = ""


_JOBS: dict[str, TerminalUsbSyncJob] = {}
_LOCK = threading.Lock()


def _now() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def _project_root() -> Path:
    # api/services/terminal_usb_sync_job_service.py -> project root
    return Path(__file__).resolve().parents[2]


def _helper_path() -> Path:
    return _project_root() / "scripts" / "ft01_usb_serial_helper.py"


def _append_limited(lines: list[str], text: str) -> None:
    if not text:
        return
    lines.append(text.rstrip())
    if len(lines) > MAX_LOG_LINES:
        del lines[:-MAX_LOG_LINES]


def _parse_bool(value: str) -> bool:
    return str(value or "").strip().lower() in {"1", "true", "yes", "ok"}


def _parse_task_reports_line(line: str) -> dict[str, Any] | None:
    text = str(line or "")
    prefix = "task reports:"
    if not text.startswith(prefix):
        return None
    parts = {}
    for token in text[len(prefix):].strip().split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        parts[key] = value
    return {
        "received": int(parts.get("received") or 0),
        "uploaded": int(parts.get("uploaded") or 0),
        "duplicates": int(parts.get("duplicates") or 0),
        "failed": int(parts.get("failed") or 0),
        "ack": _parse_bool(parts.get("ack", "false")),
    }


def _parse_tasks_download_line(line: str) -> dict[str, Any] | None:
    text = str(line or "")
    prefix = "tasks download:"
    if not text.startswith(prefix):
        return None
    parts = {}
    for token in text[len(prefix):].strip().split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        parts[key] = value
    result: dict[str, Any] = {
        "pulled": int(parts.get("pulled") or 0),
        "sent": int(parts.get("sent") or 0),
        "stored": int(parts.get("stored") or 0),
        "ack": _parse_bool(parts.get("ack", "false")),
    }
    # Optional extended fields
    if "save_done" in parts:
        result["save_done"] = _parse_bool(parts.get("save_done"))
    if "save_stored" in parts:
        try:
            result["save_stored"] = int(parts.get("save_stored") or 0)
        except Exception:
            result["save_stored"] = None
    if "stage" in parts:
        result["stage"] = parts.get("stage")
    if "line_ack" in parts:
        result["line_ack"] = _parse_bool(parts.get("line_ack"))
    if "line_acked" in parts:
        try:
            result["line_acked"] = int(parts.get("line_acked") or 0)
        except Exception:
            result["line_acked"] = None
    if "error" in parts:
        result["error"] = parts.get("error")
    return result


def _parse_cleanup_line(line: str) -> dict[str, Any] | None:
    text = str(line or "")
    prefix = "cleanup:"
    if not text.startswith(prefix):
        return None
    parts = {}
    for token in text[len(prefix):].strip().split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        parts[key] = value

    cleared_value: int | list[str] = 0
    raw_cleared = parts.get("cleared")
    if raw_cleared is not None:
        if raw_cleared.isdigit():
            cleared_value = int(raw_cleared)
        else:
            cleared_value = [item.strip() for item in raw_cleared.split(",") if item.strip()]

    requested = int(parts.get("requested") or 0)
    if requested == 0 and isinstance(cleared_value, list):
        requested = len(cleared_value)

    result: dict[str, Any] = {
        "requested": requested,
        "cleared": cleared_value,
        "failed": int(parts.get("failed") or 0),
        "ack": _parse_bool(parts.get("ack", "false")),
        "error": parts.get("error"),
    }
    return result


def _parse_audio_cleanup_line(line: str) -> dict[str, Any] | None:
    text = str(line or "")
    prefix = "audio cleanup:"
    if not text.startswith(prefix):
        return None
    parts = {}
    for token in text[len(prefix):].strip().split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        parts[key] = value

    result: dict[str, Any] = {
        "requested": int(parts.get("requested") or 0),
        "deleted": int(parts.get("deleted") or 0),
        "index_removed": int(parts.get("index_removed") or 0),
        "failed": int(parts.get("failed") or 0),
        "ack": _parse_bool(parts.get("ack", "false")),
        "error": parts.get("error"),
    }
    return result


def extract_sync_summary(stdout_lines: list[str]) -> dict[str, Any]:
    summary: dict[str, Any] = {}
    for line in stdout_lines:
        task_reports = _parse_task_reports_line(line)
        if task_reports is not None:
            summary["task_reports"] = task_reports
            continue
        tasks_download = _parse_tasks_download_line(line)
        if tasks_download is not None:
            summary["tasks_download"] = tasks_download
            continue
        cleanup = _parse_cleanup_line(line)
        if cleanup is not None:
            summary["cleanup"] = cleanup
            continue
        audio_cleanup = _parse_audio_cleanup_line(line)
        if audio_cleanup is not None:
            summary["audio_cleanup"] = audio_cleanup
            continue
    return summary


def _job_to_dict(job: TerminalUsbSyncJob) -> dict[str, Any]:
    return {
        "job_id": job.job_id,
        "port": job.port,
        "baud": job.baud,
        "api_base": job.api_base,
        "upload_records": job.upload_records,
        "upload_audio_files": job.upload_audio_files,
        "sync_tasks": job.sync_tasks,
        "status": job.status,
        "returncode": job.returncode,
        "created_at": job.created_at,
        "updated_at": job.updated_at,
        "started_at": job.started_at,
        "finished_at": job.finished_at,
        "stdout": list(job.stdout_lines),
        "stderr": list(job.stderr_lines),
        "error": job.error,
        "summary": extract_sync_summary(job.stdout_lines),
    }


def _write_job_history(job: TerminalUsbSyncJob) -> None:
    SYNC_ROOT.mkdir(parents=True, exist_ok=True)
    payload = _job_to_dict(job)
    payload["event_type"] = "usb_sync_job"
    with SYNC_JOBS_FILE.open("a", encoding="utf-8") as file:
        file.write(json.dumps(payload, ensure_ascii=False, separators=(",", ":")))
        file.write("\n")


def list_sync_ports() -> dict[str, Any]:
    if list_ports is None:
        return {
            "ok": False,
            "ports": [],
            "message": "pyserial is required. Install with: pip install pyserial",
        }
    ports = []
    for port in list(list_ports.comports()):
        device = str(port.device or "")
        description = str(port.description or "")
        # macOS exposes pseudo ports such as /dev/cu.debug-console. They are
        # not FT-01 transports and should not become the default UI selection.
        lowered = f"{device} {description}".lower()
        if "debug-console" in lowered or "bluetooth-incoming-port" in lowered:
            continue
        ports.append({
            "device": device,
            "description": description,
            "hwid": getattr(port, "hwid", ""),
        })

    ports.sort(key=lambda item: (
        0 if ("usbmodem" in item["device"].lower() or "usbserial" in item["device"].lower()) else 1,
        item["device"],
    ))
    return {"ok": True, "ports": ports, "message": ""}


def start_usb_sync_job(
    *,
    port: str,
    baud: int = 115200,
    api_base: str = "http://127.0.0.1:8787",
    upload_records: bool = True,
    upload_audio_files: bool = True,
    sync_tasks: bool = True,
) -> dict[str, Any]:
    port_value = str(port or "").strip()
    if not port_value:
        raise TerminalUsbSyncError("port is required")

    helper = _helper_path()
    if not helper.exists():
        raise TerminalUsbSyncError(f"helper not found: {helper}")

    job = TerminalUsbSyncJob(
        job_id=uuid.uuid4().hex,
        port=port_value,
        baud=int(baud or 115200),
        api_base=str(api_base or "http://127.0.0.1:8787").strip(),
        upload_records=bool(upload_records),
        upload_audio_files=bool(upload_audio_files),
        sync_tasks=bool(sync_tasks),
    )
    with _LOCK:
        _JOBS[job.job_id] = job

    thread = threading.Thread(target=_run_job, args=(job.job_id,), daemon=True)
    thread.start()
    return _job_to_dict(job)


def _run_job(job_id: str) -> None:
    with _LOCK:
        job = _JOBS[job_id]
        job.status = "running"
        job.started_at = _now()
        job.updated_at = job.started_at

    command = [
        sys.executable,
        str(_helper_path()),
        "--port",
        job.port,
        "--baud",
        str(job.baud),
        "--api-base",
        job.api_base,
        "--verbose",
    ]
    if job.upload_records:
        command.append("--upload-records")
    if job.upload_audio_files:
        command.append("--upload-audio-files")
        command.extend(["--audio-retries", "2"])
        command.append("--continue-on-audio-error")
    if job.sync_tasks:
        command.append("--sync-tasks")

    try:
        process = subprocess.Popen(
            command,
            cwd=str(_project_root()),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
    except Exception as exc:
        with _LOCK:
            job.status = "failed"
            job.error = str(exc)
            job.finished_at = _now()
            job.updated_at = job.finished_at
            _write_job_history(job)
        return

    def pump(stream, target: list[str]) -> None:
        assert stream is not None
        for line in stream:
            with _LOCK:
                _append_limited(target, line)
                job.updated_at = _now()

    stdout_thread = threading.Thread(target=pump, args=(process.stdout, job.stdout_lines), daemon=True)
    stderr_thread = threading.Thread(target=pump, args=(process.stderr, job.stderr_lines), daemon=True)
    stdout_thread.start()
    stderr_thread.start()

    returncode = process.wait()
    stdout_thread.join(timeout=1.0)
    stderr_thread.join(timeout=1.0)

    with _LOCK:
        job.returncode = returncode
        job.status = "succeeded" if returncode == 0 else "failed"
        job.finished_at = _now()
        job.updated_at = job.finished_at
        if returncode != 0 and not job.error:
            job.error = "USB sync helper exited with non-zero status"
        _write_job_history(job)


def get_usb_sync_job(job_id: str) -> dict[str, Any] | None:
    with _LOCK:
        job = _JOBS.get(job_id)
        return _job_to_dict(job) if job else None


def list_usb_sync_jobs(limit: int = 20) -> list[dict[str, Any]]:
    with _LOCK:
        jobs = sorted(_JOBS.values(), key=lambda item: item.created_at, reverse=True)
        return [_job_to_dict(job) for job in jobs[:limit]]
