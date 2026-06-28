import json
from pathlib import Path
from typing import Any, Iterable, List

from .config import DEFAULT_MODELS, OLLAMA_MODEL


def get_default_model_for_mode(mode: str) -> str:
    return DEFAULT_MODELS.get(mode, OLLAMA_MODEL)


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


def safe_text(value: Any) -> str:
    if isinstance(value, list):
        return " ".join(safe_text(item) for item in value)
    if isinstance(value, dict):
        return " ".join(safe_text(v) for v in value.values())
    return str(value or "").lower()


def contains_any(text: str, words: Iterable[str]) -> bool:
    normalized = str(text or "").lower()
    return any(str(word).lower().strip() in normalized for word in words if word)


def unique_list(items: Iterable[Any]) -> List[str]:
    result: List[str] = []
    seen = set()

    for item in items or []:
        value = str(item or "").strip()
        if value and value not in seen:
            seen.add(value)
            result.append(value)

    return result

