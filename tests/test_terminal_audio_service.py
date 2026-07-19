import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from api.services import terminal_audio_service as svc


def setup_audio_archive(tmp_path, monkeypatch):
    data_dir = tmp_path / "data"
    archive_dir = data_dir / "terminal_sync" / "archive"
    monkeypatch.setattr(svc, "DATA_DIR", data_dir)
    monkeypatch.setattr(svc, "SYNC_ROOT", data_dir / "terminal_sync")
    monkeypatch.setattr(svc, "ARCHIVE_DIR", archive_dir)
    device_dir = archive_dir / "FT01-0001"
    audio_dir = device_dir / "audio"
    audio_dir.mkdir(parents=True)
    wav = audio_dir / "sample.wav"
    wav.write_bytes(b"RIFFtestWAVE")
    record = {
        "audio_id": "FT01-0001:audio:test:audio_001.wav:12",
        "device_id": "FT01-0001",
        "filename": "audio_001.wav",
        "size": 12,
        "stored_path": str(wav),
        "sha256": "abc",
    }
    with (device_dir / "audio_files.jsonl").open("w", encoding="utf-8") as f:
        f.write(json.dumps(record, ensure_ascii=False) + "\n")
    return record, wav


def test_audio_lookup_by_audio_id(tmp_path, monkeypatch):
    record, wav = setup_audio_archive(tmp_path, monkeypatch)
    path, found = svc.get_audio_file_for_playback(record["audio_id"])
    assert path == wav.resolve(strict=False)
    assert found["filename"] == "audio_001.wav"


def test_audio_metadata_contains_controlled_play_url(tmp_path, monkeypatch):
    record, _ = setup_audio_archive(tmp_path, monkeypatch)
    metadata = svc.build_audio_playback_metadata(audio_id=record["audio_id"])
    assert metadata["audio_available"] is True
    assert metadata["play_url"].startswith("/api/terminal-sync/audio?audio_id=")
    assert "stored_path" not in metadata


def test_audio_path_escape_is_rejected(tmp_path, monkeypatch):
    data_dir = tmp_path / "data"
    archive_dir = data_dir / "terminal_sync" / "archive"
    monkeypatch.setattr(svc, "DATA_DIR", data_dir)
    monkeypatch.setattr(svc, "SYNC_ROOT", data_dir / "terminal_sync")
    monkeypatch.setattr(svc, "ARCHIVE_DIR", archive_dir)
    device_dir = archive_dir / "FT01-0001"
    device_dir.mkdir(parents=True)
    outside = tmp_path / "outside.wav"
    outside.write_bytes(b"bad")
    with (device_dir / "audio_files.jsonl").open("w", encoding="utf-8") as f:
        f.write(json.dumps({
            "audio_id": "bad",
            "device_id": "FT01-0001",
            "filename": "bad.wav",
            "stored_path": str(outside),
        }) + "\n")
    try:
        svc.get_audio_file_for_playback("bad")
    except svc.TerminalAudioPathError:
        pass
    else:
        raise AssertionError("expected TerminalAudioPathError")
