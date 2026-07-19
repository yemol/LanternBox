#!/usr/bin/env python3
"""FT-01 USB serial sync helper.

Read-only FT-01 side. Core upload side supports manifest, JSONL records, and WAV audio files. Verbose mode hides raw audio payload chunks by default.
"""

from __future__ import annotations

import argparse
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


class SyncError(RuntimeError):
    pass


@dataclass
class CapturedAudioFile:
    filename: str
    size: int
    content: bytes


def eprint(*args: Any) -> None:
    print(*args, file=sys.stderr)


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
                    uploaded += 1
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

        return 0
    finally:
        client.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SyncError as exc:
        eprint(f"ERROR: {exc}")
        raise SystemExit(1)
