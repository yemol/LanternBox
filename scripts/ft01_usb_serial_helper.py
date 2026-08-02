#!/usr/bin/env python3
"""FT-01 USB serial sync helper.

Read-only FT-01 side. Core upload side supports manifest, JSONL records, and WAV audio files. Verbose mode hides raw audio payload chunks by default.
"""

from __future__ import annotations

import argparse
import traceback
import base64
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

try:
    import serial
    from serial.tools import list_ports
except Exception:  # pragma: no cover - import error is reported at runtime
    serial = None
    list_ports = None

DEFAULT_RECORD_TYPES = ["path_points", "field_events", "boot_logs", "audio_index"]
BEGIN_MARKER = "FT01_SYNC_MANIFEST_BEGIN"
END_MARKER = "FT01_SYNC_MANIFEST_END"
RECORDS_BEGIN_MARKER = "FT01_SYNC_RECORDS_BEGIN"
RECORDS_END_MARKER = "FT01_SYNC_RECORDS_END"

_VERBOSE_MODE = False


class SyncError(RuntimeError):
    pass


class ManifestCaptureError(SyncError):
    pass


class RecordsCaptureError(SyncError):
    pass


@dataclass
class CapturedAudioFile:
    filename: str
    size: int
    content: bytes


@dataclass
class CapturedManifest:
    manifest: dict[str, Any]


@dataclass
class CapturedRecords:
    record_type: str
    records: list[dict[str, Any]]


def eprint(*args: Any) -> None:
    print(*args, file=sys.stderr)


def manifest_url(api_base: str) -> str:
    return api_base.rstrip("/") + "/api/terminal-sync/manifest"


def upload_records_url(api_base: str) -> str:
    return api_base.rstrip("/") + "/api/terminal-sync/upload-records"


def post_json(api_base: str, path: str, payload: dict[str, Any], timeout: float) -> dict[str, Any]:
    url = api_base.rstrip("/") + path
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            data = response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise SyncError(f"Core HTTP {exc.code} for {path}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise SyncError(f"Core request failed for {path}: {exc}") from exc

    try:
        return json.loads(data)
    except json.JSONDecodeError as exc:
        raise SyncError(f"Core returned non-JSON for {path}: {data[:200]}") from exc


def get_json(api_base: str, path: str, timeout: float) -> dict[str, Any] | list[Any]:
    url = api_base.rstrip("/") + path
    request = urllib.request.Request(url, method="GET")
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            data = response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise SyncError(f"Core HTTP {exc.code} for {path}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise SyncError(f"Core request failed for {path}: {exc}") from exc

    try:
        return json.loads(data)
    except json.JSONDecodeError as exc:
        raise SyncError(f"Core returned non-JSON for {path}: {data[:200]}") from exc


def post_task_report(api_base: str, report: dict[str, Any], timeout: float) -> dict[str, Any]:
    return post_json(api_base, "/api/tasks/report", report, timeout)


def normalize_manifest(manifest: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(manifest, dict):
        raise ManifestCaptureError("manifest must be a JSON object")
    device_id = str(manifest.get("device_id") or "").strip()
    sync_session_id = str(manifest.get("sync_session_id") or "").strip()
    items = manifest.get("items")
    if not device_id:
        raise ManifestCaptureError("manifest missing device_id")
    if not sync_session_id:
        raise ManifestCaptureError("manifest missing sync_session_id")
    if not isinstance(items, dict):
        raise ManifestCaptureError("manifest items must be a JSON object")
    return {
        **manifest,
        "device_id": device_id,
        "firmware_version": str(manifest.get("firmware_version") or ""),
        "sync_session_id": sync_session_id,
        "transport": str(manifest.get("transport") or "usb_serial"),
        "items": items,
    }


def capture_manifest_from_lines(lines: Iterable[str]) -> CapturedManifest:
    in_block = False
    payload_lines: list[str] = []
    for raw in lines:
        line = str(raw or "").strip()
        if not in_block:
            if line == BEGIN_MARKER:
                in_block = True
            continue
        if line == END_MARKER:
            text = "\n".join(payload_lines)
            try:
                value = json.loads(text)
            except json.JSONDecodeError as exc:
                raise ManifestCaptureError(f"JSON parse error: {exc}") from exc
            return CapturedManifest(manifest=normalize_manifest(value))
        payload_lines.append(line)
    raise ManifestCaptureError("missing manifest end marker")


def parse_jsonl_records(lines: Iterable[str], record_type: str) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for raw in lines:
        line = str(raw or "").strip()
        if not line:
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise RecordsCaptureError(f"JSONL parse error for {record_type}: {exc}") from exc
        if not isinstance(value, dict):
            raise RecordsCaptureError(f"JSONL line for {record_type} must be a JSON object")
        records.append(value)
    return records


def _records_marker_matches(line: str, marker: str, record_type: str) -> bool:
    return line == f"{marker} {record_type}" or line == f"{marker}:{record_type}"


def capture_records_from_lines(lines: Iterable[str], record_type: str) -> CapturedRecords:
    in_block = False
    payload_lines: list[str] = []
    for raw in lines:
        line = str(raw or "").strip()
        if not in_block:
            if _records_marker_matches(line, RECORDS_BEGIN_MARKER, record_type):
                in_block = True
            continue
        if _records_marker_matches(line, RECORDS_END_MARKER, record_type):
            return CapturedRecords(record_type=record_type, records=parse_jsonl_records(payload_lines, record_type))
        payload_lines.append(line)
    raise RecordsCaptureError(f"missing records end marker for {record_type}")


def parse_record_types(raw: str | None) -> list[str]:
    if not raw:
        return list(DEFAULT_RECORD_TYPES)
    normalized = raw.replace("，", ",").replace("、", ",")
    values = [item.strip() for item in normalized.split(",") if item.strip()]
    allowed = set(DEFAULT_RECORD_TYPES)
    for item in values:
        if item not in allowed:
            raise RecordsCaptureError(f"unsupported record_type: {item}")
    return values or list(DEFAULT_RECORD_TYPES)


def record_request_command(template: str, record_type: str) -> str:
    command = str(template or "").format(record_type=record_type).strip()
    lowered = command.lower()
    unsafe_tokens = ("clear", "delete", "truncate", "rotate", "terminal_may_clear")
    if any(token in lowered for token in unsafe_tokens):
        raise RecordsCaptureError(f"refusing unsafe record request command: {command}")
    if not lowered.startswith("get_records "):
        raise RecordsCaptureError(f"refusing unsafe record request command: {command}")
    return command


def post_audio(
    api_base: str,
    *,
    device_id: str,
    sync_session_id: str,
    audio_id: str,
    filename: str,
    content: bytes,
    timeout: float,
) -> dict[str, Any]:
    query = urllib.parse.urlencode(
        {
            "device_id": device_id,
            "sync_session_id": sync_session_id,
            "audio_id": audio_id,
            "filename": filename,
            "size": len(content),
        }
    )
    url = api_base.rstrip("/") + "/api/terminal-sync/upload-audio?" + query
    request = urllib.request.Request(
        url,
        data=content,
        headers={"Content-Type": "application/octet-stream"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            data = response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise SyncError(f"Core HTTP {exc.code} for upload-audio {filename}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise SyncError(f"Core request failed for upload-audio {filename}: {exc}") from exc

    try:
        return json.loads(data)
    except json.JSONDecodeError as exc:
        raise SyncError(f"Core returned non-JSON for upload-audio {filename}: {data[:200]}") from exc


class SerialSyncClient:
    def __init__(self, port: str, baud: int, timeout: float, verbose: bool = False, verbose_audio_chunks: bool = False) -> None:
        if serial is None:
            raise SyncError("pyserial is required. Install with: pip install pyserial")
        self.verbose = verbose
        self.verbose_audio_chunks = verbose_audio_chunks
        self.timeout = timeout
        self.serial = serial.Serial(port=port, baudrate=baud, timeout=0.15, write_timeout=timeout)
        # Give USB CDC a moment to settle, then clear boot chatter already buffered.
        time.sleep(1.0)
        self.serial.reset_input_buffer()

    def close(self) -> None:
        self.serial.close()

    def send_line(self, line: str) -> None:
        if self.verbose:
            eprint(">", line)
        self.serial.write((line.strip() + "\n").encode("utf-8"))
        self.serial.flush()

    def readline(self, deadline: float, *, log: bool = True) -> str:
        while time.monotonic() < deadline:
            raw = self.serial.readline()
            if not raw:
                continue
            text = raw.decode("utf-8", errors="replace").strip()
            if self.verbose and log and text:
                eprint("<", text[:180])
            return text
        raise SyncError("Timed out waiting for FT-01 serial response")

    def capture_block(self, begin_marker: str, end_marker: str, deadline_seconds: float) -> list[str]:
        deadline = time.monotonic() + deadline_seconds
        lines: list[str] = []
        in_block = False

        while time.monotonic() < deadline:
            line = self.readline(deadline)
            if not in_block:
                if line == begin_marker:
                    in_block = True
                continue

            if line == end_marker:
                return lines
            lines.append(line)

        raise SyncError(f"Timed out waiting for {end_marker}")

    def get_manifest(self) -> dict[str, Any]:
        self.send_line("GET_MANIFEST")
        lines = self.capture_block("FT01_SYNC_MANIFEST_BEGIN", "FT01_SYNC_MANIFEST_END", self.timeout)
        text = "\n".join(lines)
        try:
            manifest = json.loads(text)
        except json.JSONDecodeError as exc:
            raise SyncError(f"Could not parse FT-01 manifest JSON: {exc}\n{text[:500]}") from exc
        if not isinstance(manifest, dict):
            raise SyncError("FT-01 manifest was not a JSON object")
        return manifest

    def get_records(self, record_type: str, request_records: bool = True) -> list[dict[str, Any]]:
        if request_records:
            self.send_line(f"GET_RECORDS {record_type}")

        begin = f"FT01_SYNC_RECORDS_BEGIN {record_type}"
        end = f"FT01_SYNC_RECORDS_END {record_type}"
        lines = self.capture_block(begin, end, self.timeout)
        records: list[dict[str, Any]] = []
        for line in lines:
            if not line or not line.startswith("{"):
                continue
            try:
                value = json.loads(line)
            except json.JSONDecodeError as exc:
                raise SyncError(f"Bad JSONL line for {record_type}: {line[:240]}") from exc
            if isinstance(value, dict):
                records.append(value)
        return records

    def clear_synced_records(self, record_types: list[str], timeout: float | None = None) -> dict[str, Any]:
        if not record_types:
            return {"requested": 0, "cleared": 0, "failed": 0, "ack": True, "error": None}

        send_timeout = timeout if timeout is not None else self.timeout
        self.send_line("CLEAR_SYNCED_RECORDS " + " ".join(record_types))
        if getattr(self, "verbose", False):
            eprint("cleanup command send:", "CLEAR_SYNCED_RECORDS " + " ".join(record_types))

        deadline = time.monotonic() + send_timeout
        error: str | None = None

        while time.monotonic() < deadline:
            try:
                line = self.readline(deadline)
            except SyncError as exc:
                error = str(exc)
                if "timed out waiting for ft-01 serial response" in error.lower():
                    error = "cleanup_ack_timeout"
                break
            if not line:
                continue
            if line.startswith("FT01_SYNC_CLEANUP_ITEM"):
                continue
            if line.startswith("FT01_SYNC_CLEANUP_ACK"):
                tail = parse_key_value_tail(line)
                ack = parse_bool(tail.get("ack") or tail.get("ok"), default=False)
                requested = 0
                cleared_value: int | list[str] = 0
                failed_count = 0

                raw_cleared = str(tail.get("cleared") or "").strip()
                if raw_cleared.isdigit():
                    cleared_value = int(raw_cleared)
                    requested = cleared_value
                else:
                    items = [item.strip() for item in raw_cleared.split(",") if item.strip()]
                    cleared_value = items
                    requested = len(items)

                if "requested" in tail and str(tail.get("requested") or "").isdigit():
                    requested = int(tail.get("requested") or 0)
                if "failed" in tail and str(tail.get("failed") or "").isdigit():
                    failed_count = int(tail.get("failed") or 0)

                return {
                    "requested": requested,
                    "cleared": cleared_value,
                    "failed": failed_count,
                    "ack": ack,
                    "error": error,
                }
        if getattr(self, "verbose", False):
            eprint("cleanup ack timeout")
        return {
            "requested": 0,
            "cleared": 0,
            "failed": 0,
            "ack": False,
            "error": error or "cleanup_ack_timeout",
        }

    def delete_uploaded_audio(self, filename: str, timeout: float | None = None) -> dict[str, Any]:
        """Request FT-01 to delete a single uploaded WAV and remove index entry.

        Returns a dict with keys: file, wav_deleted (bool), index_removed (bool), wav_missing (bool), ok (bool), error
        """
        # Validate filename: only basename allowed, must match audio_*.wav
        import re

        safe_pattern = re.compile(r'^audio_[A-Za-z0-9_.-]+\.wav$')
        if not isinstance(filename, str) or not safe_pattern.match(filename):
            return {"file": filename, "wav_deleted": False, "index_removed": False, "wav_missing": False, "ok": False, "error": "invalid_filename"}

        send_timeout = timeout if timeout is not None else self.timeout
        self.send_line(f"DELETE_UPLOADED_AUDIO {filename}")
        if getattr(self, "verbose", False):
            eprint("audio delete command send:", filename)

        deadline = time.monotonic() + send_timeout
        error: str | None = None

        while time.monotonic() < deadline:
            try:
                line = self.readline(deadline)
            except SyncError as exc:
                error = str(exc)
                break
            if not line:
                continue
            if line.startswith("FT01_SYNC_AUDIO_DELETE_ACK"):
                tail = parse_key_value_tail(line)
                file = tail.get("file") or filename
                wav_deleted = parse_bool(tail.get("wav_deleted"), default=False)
                index_removed = parse_bool(tail.get("index_removed"), default=False)
                wav_missing = parse_bool(tail.get("wav_missing"), default=False)
                ok = parse_bool(tail.get("ok"), default=False)
                return {"file": file, "wav_deleted": wav_deleted, "index_removed": index_removed, "wav_missing": wav_missing, "ok": ok, "error": None}

        return {"file": filename, "wav_deleted": False, "index_removed": False, "wav_missing": False, "ok": False, "error": error or "audio_delete_timeout"}

    def put_tasks(self, tasks: list[dict[str, Any]]) -> dict[str, Any]:
        send_tasks = [compact_task_for_terminal(task) for task in tasks]
        expected = len(send_tasks)
        # Protocol: send BEGIN, wait for BEGIN_ACK. If protocol=line_ack,
        # send tasks one-by-one and wait per-line ACKs. Otherwise send burst.
        self.send_line(f"PUT_TASKS_BEGIN {expected}")

        deadline = time.monotonic() + self.timeout
        begin_ack: dict[str, str] = {}
        final_ack: dict[str, str] = {}
        save_done = False
        save_stored: int | None = None
        stage: str | None = None
        noise_count = 0

        # Wait for BEGIN_ACK
        while time.monotonic() < deadline:
            line = self.readline(deadline)
            if not line:
                continue
            if line.startswith("FT01_SYNC_TASKS_BEGIN_ACK"):
                begin_ack = parse_key_value_tail(line)
                break
            noise_count += 1

        if not begin_ack:
            return {
                "sent": expected,
                "expected": expected,
                "received": 0,
                "stored": 0,
                "ack": False,
                "line_ack": False,
                "line_acked": 0,
                "error": "begin_ack_timeout",
                "noise_count": noise_count,
            }

        protocol = begin_ack.get("protocol")
        is_line_ack = str(protocol or "").strip().lower() == "line_ack"

        line_acked = 0

        if is_line_ack:
            # Send tasks one-by-one, waiting for per-line ACKs
            for idx, task in enumerate(send_tasks, start=1):
                task = dict(task)
                task["title"] = _truncate_task_title_for_payload(task.get("title", ""))
                task["description"] = _truncate_task_description_for_payload(task.get("description", ""))
                json_line = json.dumps(task, ensure_ascii=False, separators=(",", ":"))
                byte_length = len(json_line.encode("utf-8"))
                if byte_length > 520:
                    while task["description"] and byte_length > 520:
                        if task["description"].endswith("..."):
                            task["description"] = task["description"][:-4].rstrip()
                        else:
                            task["description"] = task["description"][:-1]
                        if task["description"]:
                            task["description"] = task["description"].rstrip(".") + "..."
                        json_line = json.dumps(task, ensure_ascii=False, separators=(",", ":"))
                        byte_length = len(json_line.encode("utf-8"))
                if byte_length > 520:
                    return {
                        "sent": expected,
                        "expected": expected,
                        "received": 0,
                        "stored": 0,
                        "ack": False,
                        "line_ack": True,
                        "line_acked": line_acked,
                        "error": "task_line_too_long",
                        "noise_count": noise_count,
                    }
                if getattr(self, "verbose", False):
                    eprint(f"tasks line send: index={idx} bytes={byte_length}")
                self.send_line(json_line)
                time.sleep(0.03)
                # wait for FT01_SYNC_TASK_LINE_ACK index=<idx> ok=true
                got_ack = False
                while time.monotonic() < deadline:
                    try:
                        line = self.readline(deadline)
                    except SyncError:
                        break
                    if not line:
                        continue
                    if line.startswith("FT01_SYNC_TASK_LINE_ACK"):
                        tail = parse_key_value_tail(line)
                        try:
                            ack_index = int(tail.get("index") or 0)
                        except Exception:
                            ack_index = 0
                        ok = parse_bool(tail.get("ok"), default=False)
                        if ack_index == idx and ok:
                            line_acked += 1
                            got_ack = True
                            if getattr(self, "verbose", False):
                                eprint(f"tasks line ack: index={ack_index} ok=True")
                            break
                        continue
                    noise_count += 1
                if not got_ack:
                    return {
                        "sent": expected,
                        "expected": expected,
                        "received": 0,
                        "stored": 0,
                        "ack": False,
                        "line_ack": True,
                        "line_acked": line_acked,
                        "error": "task_line_ack_timeout",
                        "noise_count": noise_count,
                    }
            self.send_line("PUT_TASKS_END")

        else:
            # legacy burst mode: send all tasks then end
            for task in send_tasks:
                self.send_line(json.dumps(task, ensure_ascii=False, separators=(",", ":")))
            self.send_line("PUT_TASKS_END")

        # Wait for final FT01_SYNC_TASKS_ACK
        while time.monotonic() < deadline:
            try:
                line = self.readline(deadline)
            except SyncError:
                break
            if not line:
                continue
            if line.startswith("FT01_SYNC_TASKS_ACK"):
                final_ack = parse_key_value_tail(line)
                if "stage" in final_ack:
                    stage = final_ack.get("stage")
                break
            noise_count += 1

        if not final_ack:
            return {
                "sent": expected,
                "expected": expected,
                "received": 0,
                "stored": 0,
                "ack": False,
                "line_ack": bool(is_line_ack),
                "line_acked": line_acked,
                "error": "final_ack_timeout",
                "noise_count": noise_count,
            }

        # optional SAVE_DONE
        follow_deadline = min(time.monotonic() + 0.5, deadline)
        while time.monotonic() < follow_deadline:
            try:
                line = self.readline(follow_deadline)
            except SyncError:
                break
            if not line:
                continue
            if line.startswith("FT01_SYNC_TASKS_SAVE_DONE"):
                tail = parse_key_value_tail(line)
                save_done = parse_bool(tail.get("ok"), default=False)
                try:
                    save_stored = int(tail.get("stored") or 0)
                except Exception:
                    save_stored = None
                break
            noise_count += 1

        received = int(final_ack.get("received") or 0)
        stored = int(final_ack.get("stored") or 0)
        final_ok = parse_bool(final_ack.get("ok"), default=False)

        return {
            "sent": expected,
            "expected": expected,
            "received": received,
            "stored": stored,
            "ack": bool(final_ok),
            "line_ack": bool(is_line_ack),
            "line_acked": line_acked,
            "stage": stage,
            "save_done": save_done,
            "save_stored": save_stored,
            "noise_count": noise_count,
        }

    def get_audio_file(self, filename: str) -> CapturedAudioFile:
        self.send_line(f"GET_AUDIO_FILE {filename}")
        deadline = time.monotonic() + self.timeout
        begin_prefix = "FT01_SYNC_AUDIO_BEGIN "
        error_prefix = "FT01_SYNC_AUDIO_ERROR "

        while time.monotonic() < deadline:
            line = self.readline(deadline)
            if line.startswith(error_prefix):
                raise SyncError(f"FT-01 audio error: {line}")
            if line.startswith(begin_prefix):
                parts = line.split()
                if len(parts) < 3:
                    raise SyncError(f"Malformed audio begin marker: {line}")
                begin_filename = parts[1]
                try:
                    expected_size = int(parts[2])
                except ValueError as exc:
                    raise SyncError(f"Malformed audio size in begin marker: {line}") from exc
                return self._read_audio_chunks(begin_filename, expected_size, deadline)

        raise SyncError(f"Timed out waiting for FT01_SYNC_AUDIO_BEGIN for {filename}")

    def _read_audio_chunks(self, filename: str, expected_size: int, deadline: float) -> CapturedAudioFile:
        end_prefix = "FT01_SYNC_AUDIO_END "
        chunks: list[bytes] = []

        while time.monotonic() < deadline:
            line = self.readline(deadline, log=self.verbose_audio_chunks)
            if line.startswith(end_prefix):
                if self.verbose and not self.verbose_audio_chunks:
                    eprint("<", line[:180])
                parts = line.split()
                end_filename = parts[1] if len(parts) >= 2 else filename
                end_size = expected_size
                if len(parts) >= 3:
                    try:
                        end_size = int(parts[2])
                    except ValueError:
                        end_size = expected_size
                if end_filename != filename:
                    raise SyncError(f"Audio end filename mismatch: {end_filename} != {filename}")
                content = b"".join(chunks)
                if len(content) != expected_size or len(content) != end_size:
                    raise SyncError(
                        f"Audio size mismatch for {filename}: got {len(content)}, "
                        f"begin={expected_size}, end={end_size}"
                    )
                return CapturedAudioFile(filename=filename, size=expected_size, content=content)

            if not line:
                continue
            try:
                chunks.append(base64.b64decode(line.encode("ascii"), validate=True))
            except Exception as exc:
                raise SyncError(f"Invalid base64 audio chunk for {filename}: {line[:120]}") from exc

        raise SyncError(f"Timed out waiting for FT01_SYNC_AUDIO_END for {filename}")


def list_serial_ports() -> int:
    if list_ports is None:
        raise SyncError("pyserial is required. Install with: pip install pyserial")
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 0
    for port in ports:
        print(f"{port.device}\t{port.description}")
    return 0


def normalize_record_types(raw: str | None) -> list[str]:
    if not raw:
        return list(DEFAULT_RECORD_TYPES)
    values = [item.strip() for item in raw.split(",") if item.strip()]
    return values or list(DEFAULT_RECORD_TYPES)


def parse_bool(value: Any, default: bool = False) -> bool:
    if value is None:
        return default
    return str(value).strip().lower() in {"1", "true", "yes", "ok"}


def parse_key_value_tail(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for part in str(line or "").split()[1:]:
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        result[key.strip()] = value.strip()
    return result


def normalize_task_for_terminal(task: dict[str, Any]) -> dict[str, Any]:
    return {
        "task_id": str(task.get("task_id") or "").strip(),
        "title": str(task.get("title") or "").strip(),
        "description": str(task.get("description") or ""),
        "status": "pending" if str(task.get("status") or "assigned") == "assigned" else str(task.get("status") or "pending"),
        "priority": str(task.get("priority") or "normal"),
        "revision": int(task.get("revision") or 1),
    }


def _truncate_text_to_utf8_bytes(text: str, max_bytes: int) -> str:
    if len(text.encode("utf-8")) <= max_bytes:
        return text
    truncated = text
    while truncated and len(truncated.encode("utf-8")) > max_bytes:
        truncated = truncated[:-1]
    return truncated


def _truncate_task_title_for_payload(title: str) -> str:
    title = str(title or "").strip()
    if not title:
        return ""
    is_ascii = all(ord(char) < 128 for char in title)
    max_chars = 48 if is_ascii else 32
    if len(title) > max_chars:
        title = title[:max_chars]
    title = _truncate_text_to_utf8_bytes(title, 124)
    return title


def _truncate_task_description_for_payload(description: str) -> str:
    description = str(description or "").strip()
    if not description:
        return ""
    is_ascii = all(ord(char) < 128 for char in description)
    max_chars = 128 if is_ascii else 96
    if len(description) > max_chars:
        description = description[:max_chars].rstrip()
        description = description + "..."
    return description


def compact_task_for_terminal(task: dict[str, Any]) -> dict[str, Any]:
    normalized = normalize_task_for_terminal(task)
    status = normalized.get("status")
    if status == "assigned":
        status = "pending"
    return {
        "task_id": normalized.get("task_id", ""),
        "title": normalized.get("title", ""),
        "description": normalized.get("description", ""),
        "status": status,
        "priority": normalized.get("priority", "normal"),
        "revision": normalized.get("revision", 1),
    }


def pull_core_tasks(api_base: str, device_id: str, timeout: float) -> list[dict[str, Any]]:
    payload = get_json(api_base, f"/api/tasks/pull/{urllib.parse.quote(device_id)}", timeout)
    if isinstance(payload, list):
        source = payload
    elif isinstance(payload, dict) and isinstance(payload.get("tasks"), list):
        source = payload["tasks"]
    elif isinstance(payload, dict) and isinstance(payload.get("items"), list):
        source = payload["items"]
    else:
        source = []

    normalized_tasks: list[dict[str, Any]] = []
    for item in source:
        if not isinstance(item, dict):
            continue
        task = dict(item)
        task["task_id"] = str(task.get("task_id") or "").strip()
        task["title"] = str(task.get("title") or "").strip()
        task["description"] = str(task.get("description") or "")
        task["status"] = str(task.get("status") or "pending")
        task["priority"] = str(task.get("priority") or "normal")
        task["revision"] = int(task.get("revision") or 1)
        task["target"] = task.get("target") if isinstance(task.get("target"), dict) else {}
        task["tags"] = task.get("tags") if isinstance(task.get("tags"), list) else []
        normalized_tasks.append(task)
    return normalized_tasks


def upload_task_reports(
    client: SerialSyncClient,
    api_base: str,
    device_id: str,
    timeout: float,
) -> dict[str, Any]:
    reports = client.get_records("task_reports")
    summary = {
        "received": len(reports),
        "uploaded": 0,
        "duplicates": 0,
        "failed": 0,
        "ack": True,
        "errors": [],
    }
    for report in reports:
        payload = {**report}
        payload["device_id"] = str(payload.get("device_id") or device_id)
        try:
            result = post_task_report(api_base, payload, timeout)
            if result.get("duplicate"):
                summary["duplicates"] += 1
            else:
                summary["uploaded"] += 1
        except SyncError as exc:
            summary["failed"] += 1
            summary["ack"] = False
            summary["errors"].append(str(exc))
    return summary


def iter_manifest_audio_files(manifest: dict[str, Any], requested: str | None = None) -> Iterable[dict[str, Any]]:
    items = manifest.get("items") if isinstance(manifest.get("items"), dict) else {}
    audio_files = items.get("audio_files") if isinstance(items.get("audio_files"), list) else []
    requested_names = {item.strip() for item in requested.split(",")} if requested else set()

    for item in audio_files:
        if not isinstance(item, dict):
            continue
        filename = str(item.get("filename") or "").strip()
        if not filename:
            continue
        if requested_names and filename not in requested_names:
            continue
        yield item


def iter_needed_audio_files(
    manifest: dict[str, Any],
    manifest_upload_result: dict[str, Any] | None,
    requested: str | None = None,
) -> Iterable[dict[str, Any]]:
    source = manifest
    if isinstance(manifest_upload_result, dict):
        need = manifest_upload_result.get("need")
        if isinstance(need, dict) and isinstance(need.get("audio_files"), list):
            source = {"items": {"audio_files": need.get("audio_files")}}
    yield from iter_manifest_audio_files(source, requested)


def capture_audio_with_retries(
    client: SerialSyncClient,
    filename: str,
    *,
    attempts: int,
    retry_delay_seconds: float = 1.0,
) -> CapturedAudioFile:
    safe_attempts = max(1, int(attempts or 1))
    last_error: SyncError | None = None

    for attempt in range(1, safe_attempts + 1):
        try:
            return client.get_audio_file(filename)
        except SyncError as exc:
            last_error = exc
            if attempt >= safe_attempts:
                break
            eprint(
                f"audio {filename}: retry {attempt}/{safe_attempts - 1} after transfer error: {exc}"
            )
            time.sleep(retry_delay_seconds)

    assert last_error is not None
    raise last_error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Sync FT-01 over USB serial into LanternBox Core")
    parser.add_argument("--list-ports", action="store_true", help="list serial ports and exit")
    parser.add_argument("--port", help="serial port, e.g. /dev/cu.usbmodem2201")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--api-base", default="http://127.0.0.1:8787")
    parser.add_argument("--upload-records", action="store_true", help="upload JSONL record sets after manifest")
    parser.add_argument("--record-types", help="comma-separated record types; default path_points,field_events,boot_logs,audio_index")
    parser.add_argument("--no-record-request", action="store_true", help="do not send GET_RECORDS before capturing records")
    parser.add_argument("--upload-audio-files", action="store_true", help="download WAV files from FT-01 and upload them to Core")
    parser.add_argument("--audio-filenames", help="optional comma-separated audio filenames to upload")
    parser.add_argument("--audio-retries", type=int, default=2, help="retry each WAV transfer this many times after the first failed attempt")
    parser.add_argument("--continue-on-audio-error", action="store_true", help="continue syncing other files when one WAV transfer fails")
    parser.add_argument("--sync-tasks", action="store_true", help="upload FT-01 task reports and download Core tasks to FT-01")
    parser.add_argument("--skip-manifest-post", action="store_true", help="capture manifest but do not POST it to Core")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--verbose-audio-chunks", action="store_true", help="also print raw base64 WAV chunks while using --verbose")
    args = parser.parse_args(argv)

    if args.list_ports:
        return list_serial_ports()

    if not args.port:
        parser.error("--port is required unless --list-ports is used")

    client = SerialSyncClient(args.port, args.baud, args.timeout, args.verbose, args.verbose_audio_chunks)
    try:
        manifest = client.get_manifest()
        device_id = str(manifest.get("device_id") or "").strip()
        sync_session_id = str(manifest.get("sync_session_id") or "").strip()
        if not device_id or not sync_session_id:
            raise SyncError("Manifest missing device_id or sync_session_id")

        print(f"captured manifest device_id={device_id} sync_session_id={sync_session_id}")

        manifest_upload_result: dict[str, Any] | None = None
        if not args.skip_manifest_post:
            manifest_upload_result = post_json(args.api_base, "/api/terminal-sync/manifest", manifest, args.timeout)
            print("manifest upload:", json.dumps(manifest_upload_result, ensure_ascii=False))

        if args.upload_records:
            for record_type in normalize_record_types(args.record_types):
                records = client.get_records(record_type, request_records=not args.no_record_request)
                payload = {
                    "device_id": device_id,
                    "sync_session_id": sync_session_id,
                    "record_type": record_type,
                    "records": records,
                }
                result = post_json(args.api_base, "/api/terminal-sync/upload-records", payload, args.timeout)
                print(
                    f"records {record_type}: received={result.get('received')} "
                    f"imported={result.get('imported')} "
                    f"skipped_duplicate={result.get('skipped_duplicate')} ack={result.get('ack')}"
                )

        if args.upload_audio_files:
            uploaded = 0
            failed = 0
            successful_audio_files: list[str] = []
            for item in iter_needed_audio_files(manifest, manifest_upload_result, args.audio_filenames):
                filename = str(item.get("filename") or "").strip()
                audio_id = str(item.get("audio_id") or "").strip()
                if not audio_id:
                    print(f"audio {filename}: skipped missing audio_id")
                    continue

                try:
                    captured = capture_audio_with_retries(
                        client,
                        filename,
                        attempts=max(1, int(args.audio_retries or 0) + 1),
                    )
                    result = post_audio(
                        args.api_base,
                        device_id=device_id,
                        sync_session_id=sync_session_id,
                        audio_id=audio_id,
                        filename=filename,
                        content=captured.content,
                        timeout=args.timeout,
                    )
                    # Determine whether Core confirmed the audio as saved/imported
                    status_ok = bool(result.get('imported')) or parse_bool(result.get('ack'))
                    if status_ok:
                        uploaded += 1
                        successful_audio_files.append(filename)
                    else:
                        failed += 1
                    print(
                        f"audio {filename}: size={captured.size} status={result.get('status')} "
                        f"imported={result.get('imported')} duplicate={result.get('duplicate')} ack={result.get('ack')}"
                    )
                except SyncError as exc:
                    failed += 1
                    print(f"audio {filename}: failed error={exc}")
                    if not args.continue_on_audio_error:
                        raise
            print(f"audio upload complete: files={uploaded} failed={failed}")

        cleanup_targets: list[str] = []
        cleanup_summary: dict[str, Any] | None = None

        if args.upload_records:
            requested_record_types = normalize_record_types(args.record_types)
            if "path_points" in requested_record_types:
                cleanup_targets.append("path_points")
            if "field_events" in requested_record_types:
                cleanup_targets.append("field_events")
            # audio_index is handled by a separate DELETE_UPLOADED_AUDIO flow; do not include here
            if "boot_logs" in requested_record_types:
                cleanup_targets.append("boot_logs")
        if args.sync_tasks:
            cleanup_targets.append("task_reports")

        if args.sync_tasks:
            task_report_summary = upload_task_reports(client, args.api_base, device_id, args.timeout)
            print(
                "task reports: "
                f"received={task_report_summary.get('received')} "
                f"uploaded={task_report_summary.get('uploaded')} "
                f"duplicates={task_report_summary.get('duplicates')} "
                f"failed={task_report_summary.get('failed')} "
                f"ack={task_report_summary.get('ack')}"
            )
            for error in task_report_summary.get("errors", []):
                print(f"task report error: {error}")

            tasks = pull_core_tasks(args.api_base, device_id, args.timeout)
            download_summary = client.put_tasks(tasks)
            parts = [
                f"pulled={len(tasks)}",
                f"sent={download_summary.get('sent')}",
                f"stored={download_summary.get('stored')}",
                f"ack={download_summary.get('ack')}",
            ]
            if download_summary.get('line_ack') is not None:
                parts.append(f"line_ack={str(bool(download_summary.get('line_ack')))}")
                parts.append(f"line_acked={int(download_summary.get('line_acked') or 0)}")
            if download_summary.get('save_done') is not None:
                parts.append(f"save_done={str(bool(download_summary.get('save_done')))}")
            if download_summary.get('error'):
                parts.append(f"error={download_summary.get('error')}")
            print("tasks download: " + " ".join(parts))

        if cleanup_targets:
            if args.sync_tasks:
                time.sleep(0.5)
            cleanup_summary = client.clear_synced_records(cleanup_targets, timeout=8.0)
            cleanup_requested = cleanup_summary.get("requested") or 0
            cleanup_cleared = cleanup_summary.get("cleared") or 0
            cleanup_cleared_text = ",".join(cleanup_cleared) if isinstance(cleanup_cleared, list) else str(cleanup_cleared)
            cleanup_failed = cleanup_summary.get("failed") or 0
            cleanup_ack = cleanup_summary.get("ack")
            cleanup_error = cleanup_summary.get("error")
            print(
                "cleanup: "
                f"requested={cleanup_requested} "
                f"cleared={cleanup_cleared_text} "
                f"failed={cleanup_failed} "
                f"ack={cleanup_ack}"
                + (f" error={cleanup_error}" if cleanup_error else "")
            )

        # Audio cleanup: after successful audio uploads, issue DELETE_UPLOADED_AUDIO
        if args.upload_audio_files and uploaded and failed == 0:
            # Only delete files that were successfully uploaded and look safe
            audio_cleanup_requested = 0
            audio_deleted = 0
            audio_index_removed = 0
            audio_failed = 0
            audio_ok_all = True
            for fname in locals().get('successful_audio_files', []) or []:
                # ensure basename and pattern
                # call the client method to request deletion on the device
                if hasattr(client, 'delete_uploaded_audio'):
                    res = client.delete_uploaded_audio(fname)
                else:
                    # client doesn't implement delete; record failure
                    res = {"ok": False}
                audio_cleanup_requested += 1
                if res.get('ok'):
                    if res.get('wav_deleted'):
                        audio_deleted += 1
                    if res.get('index_removed'):
                        audio_index_removed += 1
                else:
                    audio_failed += 1
                    audio_ok_all = False
            print(
                "audio cleanup: "
                f"requested={audio_cleanup_requested} "
                f"deleted={audio_deleted} "
                f"index_removed={audio_index_removed} "
                f"failed={audio_failed} "
                f"ack={audio_ok_all}"
            )

        return 0
    finally:
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SyncError as exc:
        # 用户友好的同步错误（中文）
        eprint(f"同步错误：{exc}")
        raise SystemExit(1)
    except Exception as exc:  # pragma: no cover - unexpected errors
        eprint(f"未处理异常：{exc}")
        if getattr(client, "verbose", False):
            eprint("追踪信息：")
            for line in traceback.format_exception(type(exc), exc, exc.__traceback__):
                eprint(line.rstrip())
        raise SystemExit(1)
