from typing import Any, Dict, List

from ..config import DOMAIN_KEYWORDS
from ..retrieval.constants import DOMAIN_COMPATIBILITY
from ..utils import get_severity_weight, get_severity_weight, unique_list, safe_text, contains_any

# 指南基础能力：触发规则、指南关联、指南文本、领域兼容

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
