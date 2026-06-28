from typing import Any, Dict, List

from ..config import DOMAIN_KEYWORDS
from ..retrieval.constants import DOMAIN_COMPATIBILITY
from ..utils import get_severity_weight, get_severity_weight, unique_list, safe_text, contains_any

# 指南基础能力：触发规则、指南关联、指南文本、领域兼容

def match_triggers_for_message(
    *,
    message: str,
    mode: str = "emergency",
    trigger_items: List[Dict[str, Any]] | None = None,
) -> List[Dict[str, Any]]:
    """匹配用户输入对应的本地触发规则。

    当前阶段：
    - 先由 routes.py 传入 trigger_items
    - 后续再由 service 自己读取数据源
    """
    if not trigger_items:
        return []

    text = str(message or "").lower()
    results: List[Dict[str, Any]] = []

    for item in trigger_items:
        keywords = item.get("keywords", []) or []
        if isinstance(keywords, str):
            keywords = [keywords]

        if any(str(keyword).lower() in text for keyword in keywords):
            results.append(item)

    return results

def find_guides_for_triggers(
    *,
    triggers: List[Dict[str, Any]],
    guide_items: List[Dict[str, Any]] | None = None,
) -> List[Dict[str, Any]]:
    """根据触发规则查找关联指南。

    当前阶段仍然使用传入的 guide_items。
    后续再接入正式数据源。
    """
    if not triggers or not guide_items:
        return []

    guide_ids = set()

    for trigger in triggers:
        for guide_id in trigger.get("guide_ids", []) or []:
            guide_ids.add(str(guide_id))

    if not guide_ids:
        return []

    return [
        guide
        for guide in guide_items
        if str(guide.get("id", "")) in guide_ids
    ]

def match_triggers(input_text: str, triggers: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    text = (input_text or "").strip()
    if not text:
        return []

    matched = []

    for trigger in triggers:
        score = 0

        for keyword in trigger.get("trigger_keywords", []):
            if keyword and keyword in text:
                score += 3

        title = trigger.get("title", "")
        if title and title in text:
            score += 5

        if score > 0:
            item = dict(trigger)
            item["matchScore"] = score
            matched.append(item)

    matched.sort(
        key=lambda x: (
            get_severity_weight(x.get("severity", "")),
            x.get("matchScore", 0),
        ),
        reverse=True,
    )

    return matched

def find_related_guides(
    matched_triggers: List[Dict[str, Any]],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    guide_by_title = {g.get("title"): g for g in guides}
    result = []
    seen_ids = set()

    for trigger in matched_triggers:
        for title in trigger.get("related_guides", []):
            guide = guide_by_title.get(title)
            if not guide:
                continue

            guide_id = guide.get("id") or guide.get("title")
            if guide_id in seen_ids:
                continue

            seen_ids.add(guide_id)
            result.append(guide)

    return result

def serialize_related_guides(related_guides: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    return [
        {
            "id": g.get("id"),
            "title": g.get("title"),
            "category": g.get("category"),
            "category_original": g.get("category_original"),
            "scenario": g.get("scenario"),
            "goal": g.get("goal"),
            "tools": g.get("tools", []),
            "steps": g.get("steps", []),
            "check": g.get("check", []),
            "common_mistakes": g.get("common_mistakes", []),
            "fallback": g.get("fallback"),
            "stop_or_escalate": g.get("stop_or_escalate", []),
            "notes": g.get("notes"),
            "_ai_rerank_reason": g.get("_ai_rerank_reason"),
            "_ai_rerank_mode": g.get("_ai_rerank_mode"),
        }
        for g in related_guides
    ]

def guide_core_text(guide: Dict[str, Any]) -> str:
    # 核心文本不包含 negative_keywords，避免负词反向污染正向召回。
    fields = [
        "id", "title", "category", "category_original", "scenario", "goal",
        "fallback", "notes", "summary", "source", "domains", "keywords",
        "situations", "objects", "intents",
    ]
    return " ".join(safe_text(guide.get(key)) for key in fields)

def guide_full_text(guide: Dict[str, Any]) -> str:
    parts = [guide_core_text(guide)]
    for key in ["tools", "steps", "check", "common_mistakes", "stop_or_escalate", "do_first", "avoid", "items"]:
        parts.append(safe_text(guide.get(key)))
    return " ".join(parts)

def guide_domains(guide: Dict[str, Any]) -> List[str]:
    domains = guide.get("domains")
    if isinstance(domains, list) and domains:
        return unique_list([str(item) for item in domains])

    text = guide_core_text(guide)
    result = []
    for domain, words in DOMAIN_KEYWORDS.items():
        if contains_any(text, words):
            result.append(domain)
    return result or ["general"]

def guide_compatible_with_domains(guide: Dict[str, Any], detected_domains: List[str]) -> bool:
    if not detected_domains:
        return True
    guide_set = set(guide_domains(guide))
    for domain in detected_domains:
        allowed = DOMAIN_COMPATIBILITY.get(domain, {domain})
        if guide_set & allowed:
            return True
    return False
