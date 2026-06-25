import json
import os
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen

from fastapi import HTTPException

from .config import (
    TTS_MAX_FILE_AGE_SECONDS,
    TTS_MAX_OUTPUT_FILES,
    TTS_OUTPUT_DIR,
    VOICE_SERVICE_DEFAULT_URL,
    VOICE_SERVICE_TIMEOUT_SECONDS,
)


VOICE_SERVICE_URL = os.getenv(
    "LANTERNBOX_VOICE_SERVICE_URL",
    VOICE_SERVICE_DEFAULT_URL,
).rstrip("/")

VOICE_TTS_ENDPOINT = f"{VOICE_SERVICE_URL}/api/voice/tts"
VOICE_TIMEOUT_SECONDS = float(
    os.getenv("LANTERNBOX_VOICE_TIMEOUT", str(VOICE_SERVICE_TIMEOUT_SECONDS))
)


def cleanup_tts_output() -> None:
    """
    清理主系统本地 TTS 缓存文件。

    注意：
    实际语音生成已经交给 voice_service。
    这里保留缓存清理，是为了兼容主系统原有 /api/tts/speak 返回本地音频 URL 的流程。
    """
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
            should_delete_by_count = index >= TTS_MAX_OUTPUT_FILES
            should_delete_by_age = file_age > TTS_MAX_FILE_AGE_SECONDS

            if should_delete_by_count or should_delete_by_age:
                wav_file.unlink(missing_ok=True)
        except Exception as error:
            print("清理 TTS 输出文件失败：", wav_file, error)


def _read_error_body(error: HTTPError) -> str:
    try:
        return error.read().decode("utf-8", errors="replace")
    except Exception:
        return str(error)


def _normalize_audio_url(audio_url: str) -> str:
    if not audio_url:
        raise HTTPException(status_code=500, detail="语音服务未返回 audio_url")

    if audio_url.startswith("http://") or audio_url.startswith("https://"):
        return audio_url

    if audio_url.startswith("/"):
        return urljoin(f"{VOICE_SERVICE_URL}/", audio_url.lstrip("/"))

    return urljoin(f"{VOICE_SERVICE_URL}/", audio_url)


def _download_audio(audio_url: str, output_path: Path) -> None:
    resolved_url = _normalize_audio_url(audio_url)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        with urlopen(resolved_url, timeout=VOICE_TIMEOUT_SECONDS) as response:
            audio_bytes = response.read()
    except HTTPError as error:
        raise HTTPException(
            status_code=502,
            detail=f"语音服务音频读取失败：{_read_error_body(error)}",
        ) from error
    except URLError as error:
        raise HTTPException(
            status_code=503,
            detail=f"语音服务音频不可访问：{error}",
        ) from error
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"语音文件下载失败：{error}",
        ) from error

    if not audio_bytes:
        raise HTTPException(status_code=500, detail="语音服务返回了空音频文件")

    output_path.write_bytes(audio_bytes)


def synthesize_tts_to_file(text: str, output_path: Path, mode: str = "default") -> None:
    """
    调用独立 Voice Service 生成语音，并把音频缓存到主系统指定位置。

    主系统不再关心 Piper / MeloTTS / 模型路径。
    后续语音服务迁移到外部硬件时，只需要设置：
    LANTERNBOX_VOICE_SERVICE_URL=http://<voice-box-ip>:8790
    """
    clean_text = (text or "").strip()

    if not clean_text:
        raise HTTPException(status_code=400, detail="TTS 文本不能为空")

    payload = json.dumps({
        "text": clean_text,
        "mode": mode or "default",
    }).encode("utf-8")

    request = Request(
        VOICE_TTS_ENDPOINT,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    try:
        with urlopen(request, timeout=VOICE_TIMEOUT_SECONDS) as response:
            raw_body = response.read().decode("utf-8", errors="replace")
    except HTTPError as error:
        raise HTTPException(
            status_code=502,
            detail=f"语音服务生成失败：{_read_error_body(error)}",
        ) from error
    except URLError as error:
        raise HTTPException(
            status_code=503,
            detail=f"语音服务不可用：{error}",
        ) from error
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"语音服务调用失败：{error}",
        ) from error

    try:
        data = json.loads(raw_body)
    except json.JSONDecodeError as error:
        raise HTTPException(
            status_code=500,
            detail=f"语音服务返回内容不是有效 JSON：{raw_body}",
        ) from error

    if data.get("ok") is False:
        raise HTTPException(
            status_code=500,
            detail=f"语音服务返回失败：{data}",
        )

    _download_audio(data.get("audio_url", ""), output_path)


# 兼容旧代码调用名。
# 后续确认 routes.py 已改为 synthesize_tts_to_file 后，可以删除这两个别名。
def run_voice_service_tts(text: str, output_path: Path, mode: str = "default") -> None:
    synthesize_tts_to_file(text=text, output_path=output_path, mode=mode)


def run_piper_tts(text: str, output_path: Path) -> None:
    synthesize_tts_to_file(text=text, output_path=output_path, mode="emergency")


def run_melotts_tts(text: str, output_path: Path) -> None:
    synthesize_tts_to_file(text=text, output_path=output_path, mode="companion")
