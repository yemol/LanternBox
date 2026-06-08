import subprocess
import sys
import time
from pathlib import Path
from fastapi import HTTPException

from .config import (
    MELOTTS_PYTHON_PATH,
    MELOTTS_SCRIPT_PATH,
    MELOTTS_SPEED,
    PIPER_MODEL_PATH,
    TTS_OUTPUT_DIR,
)
from .utils import get_tts_engine


def cleanup_tts_output() -> None:
    if not TTS_OUTPUT_DIR.exists():
        return

    wav_files = sorted(
        TTS_OUTPUT_DIR.glob("*.wav"),
        key=lambda item: item.stat().st_mtime,
        reverse=True,
    )

    now = time.time()

    for index, wav_file in enumerate(wav_files):
        try:
            file_age = now - wav_file.stat().st_mtime
            should_delete_by_count = index >= 30
            should_delete_by_age = file_age > 60 * 60 * 24

            if should_delete_by_count or should_delete_by_age:
                wav_file.unlink(missing_ok=True)
        except Exception as error:
            print("清理 TTS 输出文件失败：", wav_file, error)


def run_piper_tts(text: str, output_path: Path) -> None:
    if not PIPER_MODEL_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"Piper 语音模型不存在：{PIPER_MODEL_PATH}"
        )

    subprocess.run(
        [
            sys.executable,
            "-m",
            "piper",
            "--model",
            str(PIPER_MODEL_PATH),
            "--output_file",
            str(output_path),
        ],
        input=text,
        text=True,
        check=True,
        capture_output=True,
    )


def run_melotts_tts(text: str, output_path: Path) -> None:
    if not MELOTTS_PYTHON_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"MeloTTS Python 不存在：{MELOTTS_PYTHON_PATH}"
        )

    if not MELOTTS_SCRIPT_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"MeloTTS 脚本不存在：{MELOTTS_SCRIPT_PATH}"
        )

    subprocess.run(
        [
            str(MELOTTS_PYTHON_PATH),
            str(MELOTTS_SCRIPT_PATH),
            "--text",
            text,
            "--output",
            str(output_path),
            "--speed",
            str(MELOTTS_SPEED),
        ],
        text=True,
        check=True,
        capture_output=True,
    )
