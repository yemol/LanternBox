# 负责候选池、硬排除、Kiwix 占位、选择/排除来源、检索决策。

from typing import Any, Dict, List, Tuple

from ..utils import safe_text, contains_any, unique_list
from ..services.guide_service import (
    guide_core_text,
    guide_domains,
)

HYBRID_RAG_VERSION = "v0.6"
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


def retrieve_kiwix_candidates(user_message: str, query_profile: Dict[str, Any], limit: int = 8) -> List[Dict[str, Any]]:
    """Kiwix 预留接口。

    v0.6 先不实际检索 ZIM。后续接入时，这里返回统一 CandidateSource：
    - source_type: kiwix
    - title: 页面标题
    - summary/snippet: 摘要或命中片段
    - source_id/path: zim 内路径
    - raw: 原始搜索结果
    """
    return []


def build_candidate_pool(
    *,
    user_message: str,
    query_profile: Dict[str, Any],
    guide_candidates: List[Dict[str, Any]],
    wiki_candidates: List[Dict[str, Any]] = None,
    inventory_candidates: List[Dict[str, Any]] = None,
    record_candidates: List[Dict[str, Any]] = None,
    include_kiwix: bool = False,
) -> List[Dict[str, Any]]:
    pool: List[Dict[str, Any]] = []

    for index, guide in enumerate(guide_candidates or []):
        pool.append(build_candidate_source(
            "guide",
            guide,
            rank=index,
            query_profile=query_profile,
            base_score=guide.get("_match_score", 0),
            reason=guide.get("_match_reason", "应急指南规则召回"),
        ))

    for index, wiki in enumerate(wiki_candidates or []):
        pool.append(build_candidate_source(
            "wiki",
            wiki,
            rank=index,
            query_profile=query_profile,
            base_score=wiki.get("_match_score", wiki.get("score", 0)),
            reason=wiki.get("_match_reason", "本地 Wiki 候选"),
        ))

    for index, item in enumerate(inventory_candidates or []):
        pool.append(build_candidate_source("inventory", item, rank=index, query_profile=query_profile, reason="库存/物资候选"))

    for index, item in enumerate(record_candidates or []):
        pool.append(build_candidate_source("record", item, rank=index, query_profile=query_profile, reason="本地记录候选"))

    if include_kiwix:
        pool.extend(retrieve_kiwix_candidates(user_message, query_profile, limit=8))

    # 候选池去重：同 source_type + source_id 只保留最高分。
    best: Dict[str, Dict[str, Any]] = {}
    for item in pool:
        key = item.get("candidate_id")
        if key not in best or item.get("score", 0) > best[key].get("score", 0):
            best[key] = item

    return sorted(best.values(), key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)


def build_retrieval_decision(
    *,
    user_message: str,
    query_profile: Dict[str, Any],
    candidates: List[Dict[str, Any]],
    selected: List[Dict[str, Any]],
    excluded: List[Dict[str, Any]],
    mode: str = "rule",
) -> Dict[str, Any]:
    focus = []
    if query_profile.get("intents"):
        focus.extend(query_profile.get("intents", [])[:3])
    if query_profile.get("objects"):
        focus.extend(query_profile.get("objects", [])[:3])

    return {
        "version": HYBRID_RAG_VERSION,
        "mode": mode,
        "intent_summary": "、".join(query_profile.get("intents") or query_profile.get("domains") or ["未识别明确意图"]),
        "domains": query_profile.get("domains", []),
        "focus": unique_list(focus)[:6],
        "candidate_count": len(candidates),
        "selected_count": len(selected),
        "excluded_count": len(excluded),
        "selected_sources": [
            {
                "candidate_id": item.get("candidate_id"),
                "source_type": item.get("source_type"),
                "title": item.get("title"),
                "reason": item.get("reason"),
                "confidence": item.get("confidence"),
            }
            for item in selected[:8]
        ],
        "excluded_sources": [
            {
                "candidate_id": item.get("candidate_id"),
                "source_type": item.get("source_type"),
                "title": item.get("title"),
                "reason": item.get("excluded_reason"),
            }
            for item in excluded[:8]
        ],
    }

