from typing import Any, Dict, List, Tuple
from ..services.guide_service import (
    guide_domains,
)

SOURCE_PRIORITY = {
    "guide": 100,
    "wiki": 80,
    "kiwix": 60,
}
SOURCE_TYPE_LABELS = {
    "guide": "本地应急指南",
    "wiki": "本地精选 Wiki",
    "kiwix": "Kiwix 离线知识库",
}

def _candidate_id(source_type: str, item: Dict[str, Any], fallback_index: int = 0) -> str:
    raw_id = item.get("id") or item.get("title") or f"item-{fallback_index}"
    return f"{source_type}:{raw_id}"


def _candidate_title(item: Dict[str, Any]) -> str:
    return str(item.get("title") or item.get("name") or item.get("id") or "未命名来源")


def _candidate_summary(item: Dict[str, Any]) -> str:
    for key in ["summary", "goal", "scenario", "content", "notes"]:
        value = item.get(key)
        if isinstance(value, str) and value.strip():
            return value.strip()[:260]
    steps = item.get("steps")
    if isinstance(steps, list) and steps:
        return "；".join(str(step) for step in steps[:3])[:260]
    return ""


def build_candidate_source(
    source_type: str,
    item: Dict[str, Any],
    *,
    rank: int = 0,
    query_profile: Dict[str, Any] = None,
    base_score: int = 0,
    reason: str = "",
) -> Dict[str, Any]:
    """统一候选来源结构。

    后续 guide/wiki/kiwix/inventory/record 都先转成这个结构，再交给 AI 重排或规则回退。
    为兼容现有前端，原始 item 保留在 raw 字段，不改变 related_guides 的旧输出。
    """
    query_profile = query_profile or {}
    raw_score = int(base_score or item.get("_match_score") or item.get("_domain_score") or 0)
    priority = SOURCE_PRIORITY.get(source_type, 0)

    candidate = {
        "candidate_id": _candidate_id(source_type, item, rank),
        "source_type": source_type,
        "source_label": SOURCE_TYPE_LABELS.get(source_type, source_type),
        "source_id": item.get("id") or item.get("title"),
        "title": _candidate_title(item),
        "summary": _candidate_summary(item),
        "rank": rank,
        "score": raw_score + priority,
        "base_score": raw_score,
        "priority": priority,
        "confidence": min(1.0, max(0.05, (raw_score + priority) / 180.0)),
        "reason": reason or item.get("_match_reason") or "规则召回候选来源",
        "matched_terms": item.get("_reference_matched_terms") or item.get("_matched_terms") or [],
        "domains": item.get("domains") or guide_domains(item) if source_type == "guide" else item.get("domains", []),
        "intents": item.get("intents") or [],
        "situations": item.get("situations") or [],
        "objects": item.get("objects") or [],
        "hard_excluded": False,
        "excluded_reason": "",
        "raw": item,
    }

    # 当前问题的强意图如果与候选重叠，给 AI 和前端一个更清楚的解释口径。
    overlaps = []
    for label, key in [("意图", "intents"), ("场景", "situations"), ("对象", "objects")]:
        value = sorted(set(query_profile.get(key, [])) & set(candidate.get(key, [])))
        if value:
            overlaps.append(f"{label}匹配：{'、'.join(value[:2])}")
    if overlaps:
        candidate["reason"] = "；".join(overlaps)

    return candidate


def get_candidate_raw_item(candidate: Dict[str, Any]) -> Dict[str, Any]:
    raw = candidate.get("raw")
    return raw if isinstance(raw, dict) else candidate


