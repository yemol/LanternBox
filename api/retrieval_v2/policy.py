"""Retrieval policy loader.

The retrieval pipeline should make decisions from data-backed policy instead
of source-specific literals spread across fetchers and orchestrators.
"""

import json
from functools import lru_cache
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Sequence


ROOT = Path(__file__).resolve().parents[2]
POLICY_FILE = ROOT / "data" / "retrieval_policy.json"


@lru_cache(maxsize=1)
def load_retrieval_policy() -> Dict[str, Any]:
    if not POLICY_FILE.exists():
        return {}
    payload = json.loads(POLICY_FILE.read_text(encoding="utf-8"))
    return payload if isinstance(payload, dict) else {}


def policy_section(*path: str) -> Any:
    value: Any = load_retrieval_policy()
    for key in path:
        if not isinstance(value, Mapping):
            return {}
        value = value.get(key, {})
    return value


def policy_list(*path: str) -> List[Any]:
    value = policy_section(*path)
    return list(value) if isinstance(value, list) else []


def policy_str_list(*path: str) -> List[str]:
    return [str(item) for item in policy_list(*path) if str(item)]


def policy_set(*path: str) -> set[str]:
    return {str(item) for item in policy_list(*path) if str(item)}


def policy_map(*path: str) -> Dict[str, Any]:
    value = policy_section(*path)
    return dict(value) if isinstance(value, Mapping) else {}


def policy_float(path: Sequence[str], default: float = 0.0) -> float:
    value: Any = load_retrieval_policy()
    for key in path:
        if not isinstance(value, Mapping):
            return default
        value = value.get(key)
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def policy_int(path: Sequence[str], default: int = 0) -> int:
    value: Any = load_retrieval_policy()
    for key in path:
        if not isinstance(value, Mapping):
            return default
        value = value.get(key)
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def policy_string(path: Sequence[str], default: str = "") -> str:
    value: Any = load_retrieval_policy()
    for key in path:
        if not isinstance(value, Mapping):
            return default
        value = value.get(key)
    return str(value) if value is not None else default


def first_policy_value(paths: Iterable[Sequence[str]], default: Any = None) -> Any:
    for path in paths:
        value: Any = load_retrieval_policy()
        found = True
        for key in path:
            if not isinstance(value, Mapping) or key not in value:
                found = False
                break
            value = value.get(key)
        if found:
            return value
    return default
