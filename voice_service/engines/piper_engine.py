from pathlib import Path
import os
import subprocess
import uuid


BASE_DIR = Path(__file__).resolve().parents[1]
OUTPUT_DIR = BASE_DIR / "output"
MODEL_DIR = BASE_DIR / "models" / "piper"

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

DEFAULT_PIPER_MODEL = MODEL_DIR / "zh_CN-huayan-medium.onnx"

PIPER_MODEL = Path(
    os.getenv("LANTERNBOX_PIPER_MODEL", str(DEFAULT_PIPER_MODEL))
)


def synthesize_with_piper(text: str, mode: str = "default") -> dict:
    clean_text = (text or "").strip()

    if not clean_text:
        raise ValueError("TTS 文本不能为空")

    if not PIPER_MODEL.exists():
        raise FileNotFoundError(f"Piper 模型文件不存在：{PIPER_MODEL}")

    filename = f"tts_{mode}_{uuid.uuid4().hex}.wav"
    output_path = OUTPUT_DIR / filename

    cmd = [
        "python",
        "-m",
        "piper",
        "-m",
        str(PIPER_MODEL),
        "-f",
        str(output_path),
    ]

    try:
        result = subprocess.run(
            cmd,
            input=clean_text,
            text=True,
            capture_output=True,
            check=False,
            timeout=60,
        )
    except Exception as exc:
        raise RuntimeError(f"Piper 调用失败：{exc}") from exc

    if result.returncode != 0:
        raise RuntimeError(
            "Piper 生成失败："
            + (result.stderr or result.stdout or "未知错误")
        )

    if not output_path.exists():
        raise RuntimeError("Piper 未生成音频文件")

    return {
        "ok": True,
        "engine": "piper",
        "mode": mode,
        "model": str(PIPER_MODEL),
        "filename": filename,
        "audio_path": str(output_path),
        "audio_url": f"/voice/audio/{filename}",
    }