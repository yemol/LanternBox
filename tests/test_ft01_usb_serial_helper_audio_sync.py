from pathlib import Path
import importlib.util
import sys

HELPER_PATH = Path(__file__).resolve().parents[1] / "scripts" / "ft01_usb_serial_helper.py"
if not HELPER_PATH.exists():
    HELPER_PATH = Path(__file__).resolve().parents[1] / "ft01_usb_serial_helper_audio_sync_v0_3.py"

spec = importlib.util.spec_from_file_location("ft01_usb_serial_helper", HELPER_PATH)
helper = importlib.util.module_from_spec(spec)
assert spec.loader is not None
sys.modules[spec.name] = helper
spec.loader.exec_module(helper)


def test_normalize_record_types_default():
    assert helper.normalize_record_types(None) == [
        "path_points",
        "field_events",
        "boot_logs",
        "audio_index",
    ]


def test_normalize_record_types_custom():
    assert helper.normalize_record_types("audio_index, field_events") == ["audio_index", "field_events"]


def test_iter_manifest_audio_files_filters_names():
    manifest = {
        "items": {
            "audio_files": [
                {"filename": "audio_001.wav", "audio_id": "a1"},
                {"filename": "audio_002.wav", "audio_id": "a2"},
                {"filename": "", "audio_id": "bad"},
            ]
        }
    }
    files = list(helper.iter_manifest_audio_files(manifest, "audio_002.wav"))
    assert files == [{"filename": "audio_002.wav", "audio_id": "a2"}]


def test_iter_manifest_audio_files_returns_all_when_filter_missing():
    manifest = {
        "items": {
            "audio_files": [
                {"filename": "audio_001.wav", "audio_id": "a1"},
                {"filename": "audio_002.wav", "audio_id": "a2"},
                {"filename": "", "audio_id": "bad"},
                "not-a-dict",
            ]
        }
    }
    files = list(helper.iter_manifest_audio_files(manifest, ""))
    assert files == [
        {"filename": "audio_001.wav", "audio_id": "a1"},
        {"filename": "audio_002.wav", "audio_id": "a2"},
    ]


def test_captured_audio_file_dataclass():
    captured = helper.CapturedAudioFile(filename="audio_001.wav", size=3, content=b"abc")
    assert captured.filename == "audio_001.wav"
    assert captured.size == 3
    assert captured.content == b"abc"


def test_client_init_accepts_verbose_audio_chunks_parameter():
    import inspect
    signature = inspect.signature(helper.SerialSyncClient.__init__)
    assert "verbose_audio_chunks" in signature.parameters
