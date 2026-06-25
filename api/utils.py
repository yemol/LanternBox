import json
from pathlib import Path
from typing import Any, Dict, List, Optional

from .config import DEFAULT_MODELS


def get_tts_engine(mode: str, engine: Optional[str] = None) -> str:
    """
    v0.7.2 起，主系统不再按模式切换 Piper / MeloTTS。
    统一走独立 Voice Service，由 voice_service 内部管理实际语音引擎。
    保留此函数仅用于兼容旧路由调用。
    """
    return "voice_service"


def get_default_model_for_mode(mode: str) -> str:
    return DEFAULT_MODELS.get(mode, "qwen2.5:3b")


def read_json_file(file_path: Path, fallback: Any):
    try:
        if not file_path.exists():
            print(f"文件不存在：{file_path}")
            return fallback

        with file_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"读取或解析 JSON 失败：{file_path}", e)
        return fallback


def get_severity_weight(severity: str) -> int:
    return {
        "紧急": 4,
        "高": 3,
        "中": 2,
        "低": 1,
    }.get(severity, 0)


def make_item_code(item_id: int) -> str:
    return f"LB-{item_id:05d}"
