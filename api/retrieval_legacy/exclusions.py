from typing import Any, Dict, List, Tuple
from ..utils import safe_text, contains_any, unique_list
from .candidate_source import get_candidate_raw_item

def detect_explicit_exclusions(user_message: str) -> Dict[str, Any]:
    text = safe_text(user_message)
    exclusions = []

    patterns = [
        ("fridge_food", ["冰箱", "冷藏", "冷冻"], ["先不管", "暂时不管", "不用管", "先别管", "不考虑", "暂不考虑", "先放一边", "先不用"]),
        ("food", ["食物", "吃的", "做饭", "剩饭", "剩菜", "肉", "罐头"], ["先不管", "暂时不管", "不用管", "先别管", "不考虑", "暂不考虑", "先放一边", "先不用"]),
        ("outside_action", ["外出", "出去", "门外", "巡查"], ["不出去", "不要出去", "先不出去", "暂时不出去", "不外出", "不要外出"]),
    ]

    for topic, objects, negations in patterns:
        found = False
        for obj in objects:
            for neg in negations:
                if f"{obj}{neg}" in text or f"{neg}{obj}" in text:
                    found = True
                if obj in text and neg in text:
                    oi = text.find(obj)
                    ni = text.find(neg)
                    if oi >= 0 and ni >= 0 and abs(oi - ni) <= 12:
                        found = True
        if found and topic not in exclusions:
            exclusions.append(topic)

    # 纯记录/库存管理语境：物资变化、库存盘点、每日记录，不等于“有人盯上物资”。
    records_terms = ["物资变化", "库存盘点", "每日记录", "做记录", "清单", "登记", "盘点"]
    security_terms = ["门外", "可疑", "陌生人", "盯上", "靠近", "脚步声", "异响", "冲着物资"]
    if contains_any(text, records_terms) and not contains_any(text, security_terms):
        exclusions.append("security_resource_exposure")

    return {
        "topics": unique_list(exclusions),
        "has_exclusion": bool(exclusions),
    }


def candidate_matches_exclusion(candidate: Dict[str, Any], exclusions: Dict[str, Any]) -> str:
    topics = set(exclusions.get("topics") or [])
    if not topics:
        return ""

    raw = get_candidate_raw_item(candidate)
    text = safe_text([
        candidate.get("title"), candidate.get("summary"), candidate.get("domains"),
        raw.get("category"), raw.get("category_original"), raw.get("scenario"), raw.get("goal"),
        raw.get("keywords"), raw.get("objects"), raw.get("situations"), raw.get("intents"),
    ])

    if "fridge_food" in topics and contains_any(text, ["冰箱", "冷藏", "冷冻", "refrigeration_failure", "fridge", "停电后冰箱食物处置"]):
        return "用户明确表示冰箱/冷藏相关内容暂不处理。"

    if "food" in topics and contains_any(text, ["食物", "剩饭", "剩菜", "肉", "罐头", "food", "food_spoilage"]):
        return "用户明确表示食物相关内容暂不处理。"

    if "outside_action" in topics and contains_any(text, ["外出", "巡查", "门外", "出去", "outside", "patrol"]):
        return "用户明确表示暂不外出或不处理外部行动。"

    if "security_resource_exposure" in topics and contains_any(text, ["可疑人员", "陌生人", "门外", "盯上", "资源暴露", "不对峙", "异响", "脚步声"]):
        return "当前是库存/记录管理问题，未出现可疑人员或资源暴露信号。"

    return ""


def apply_hard_exclusions_to_candidates(
    candidates: List[Dict[str, Any]],
    exclusions: Dict[str, Any],
) -> List[Dict[str, Any]]:
    result = []
    for candidate in candidates:
        item = dict(candidate)
        reason = candidate_matches_exclusion(item, exclusions)
        if reason:
            item["hard_excluded"] = True
            item["excluded_reason"] = reason
        result.append(item)
    return result


def split_selected_and_excluded_candidates(
    candidates: List[Dict[str, Any]],
    *,
    max_selected: int = 12,
) -> Dict[str, List[Dict[str, Any]]]:
    selected = [item for item in candidates if not item.get("hard_excluded")]
    excluded = [item for item in candidates if item.get("hard_excluded")]

    selected.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    excluded.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)

    return {
        "selected": selected[:max_selected],
        "excluded": excluded,
    }
