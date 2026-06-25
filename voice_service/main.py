from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
from pydantic import BaseModel

from voice_service.engines.piper_engine import synthesize_with_piper


BASE_DIR = Path(__file__).resolve().parent
OUTPUT_DIR = BASE_DIR / "output"

app = FastAPI(title="LanternBox Voice Service")


class TTSRequest(BaseModel):
    text: str
    mode: str = "default"


@app.get("/api/voice/health")
def voice_health():
    return {
        "ok": True,
        "service": "LanternBox Voice Service",
        "engine": "piper",
        "status": "ready",
    }


@app.post("/api/voice/tts")
def voice_tts(payload: TTSRequest):
    try:
        return synthesize_with_piper(
            text=payload.text,
            mode=payload.mode,
        )
    except Exception as exc:
        raise HTTPException(
            status_code=500,
            detail=f"TTS 生成语音失败：{exc}",
        )


@app.get("/voice/audio/{filename}")
def get_audio(filename: str):
    safe_name = Path(filename).name
    audio_path = OUTPUT_DIR / safe_name

    if not audio_path.exists():
        raise HTTPException(status_code=404, detail="音频文件不存在")

    return FileResponse(
        path=audio_path,
        media_type="audio/wav",
        filename=safe_name,
    )