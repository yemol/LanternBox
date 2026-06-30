"""指南检索模块。负责应急指南的意图分析、打分与召回。"""

from typing import Any, Dict, List
import re

from ..config import DOMAIN_KEYWORDS,GUIDE_TAXONOMY_CACHE
from ..utils import safe_text, contains_any, unique_list,read_json_file
from ..services.guide_service import (
    guide_core_text,
    guide_full_text,
    guide_domains,
    guide_compatible_with_domains,
)


def _term_score(message: str, guide: Dict[str, Any]) -> int:
    raw = safe_text(message)

    terms = [
        item.strip()
        for item in re.split(
            r"[\s，。！？、；：,.!?;:（）()【】\[\]《》<>\"']+",
            str(message or ""),
        )
        if len(item.strip()) >= 2
    ]

    taxonomy_sources = [DOMAIN_KEYWORDS]

    taxonomy_sources.extend(GUIDE_TAXONOMY_CACHE.values())

    for taxonomy in taxonomy_sources:
        for words in taxonomy.values():
            for word in words:
                word = str(word).lower().strip()
                if len(word) >= 2 and word in raw and word not in terms:
                    terms.append(word)

    title = safe_text(guide.get("title"))
    scenario_goal = safe_text([
        guide.get("scenario"),
        guide.get("goal"),
        guide.get("summary"),
    ])
    keywords = safe_text(guide.get("keywords"))
    category = safe_text([
        guide.get("category"),
        guide.get("category_original"),
    ])
    core = guide_core_text(guide)
    full = guide_full_text(guide)

    score = 0

    for term in terms[:40]:
        if term in title:
            score += 14
        elif term in keywords:
            score += 9
        elif term in scenario_goal:
            score += 6
        elif term in category:
            score += 4
        elif term in core:
            score += 2
        elif term in full:
            score += 1

    return score


def _list_overlap_score(left: Any, right: Any, weight: int = 1) -> int:
    left_items = set(unique_list(left if isinstance(left, list) else [left]))
    right_items = set(unique_list(right if isinstance(right, list) else [right]))

    if not left_items or not right_items:
        return 0

    return len(left_items & right_items) * weight


def build_guide_query(
    strategy: Dict[str, Any],
    context=None,
) -> Dict[str, Any]:

    guide_filters = strategy.get("guide_filters", {})

    return {
        "domains": guide_filters.get("domains", []),
        "intents": guide_filters.get("tasks", []),
        "situations": guide_filters.get("situations", []),
        "objects": guide_filters.get("objects", []),
        "signals": strategy.get("signals", []),
        "risks": strategy.get("risks", []),
        "retrieval_plan": strategy.get("retrieval_plan", []),
        "analysis": context,
    }


def score_guide_metadata(
    guide: Dict[str, Any],
    query_profile: Dict[str, Any],
) -> int:
    """
    根据 Guide 结构化元数据计算通用评分。

    只在主意图、信号或风险明确命中时加较高分。
    不因为同领域就把 primary guide 顶到前面，避免领域级通用指南压过具体指南。
    """
    score = 0

    guide_signals = set(guide.get("signals") or [])
    guide_risks = set(guide.get("risks") or [])
    guide_intents = set(guide.get("intents") or [])
    guide_domains = set(guide.get("domains") or [])

    query_signals = set(query_profile.get("signals") or [])
    query_risks = set(query_profile.get("risks") or [])
    query_intents = set(query_profile.get("intents") or [])
    query_domains = set(query_profile.get("domains") or [])

    primary_intent = guide.get("primary_intent")
    primary_domain = guide.get("primary_domain")
    ranking_role = guide.get("ranking_role")

    primary_intent_matched = bool(
        primary_intent and primary_intent in query_intents
    )

    matched_signals = guide_signals & query_signals
    matched_risks = guide_risks & query_risks
    matched_intents = guide_intents & query_intents

    if primary_intent_matched:
        score += 18

    if matched_signals:
        score += 10 * len(matched_signals)

    if matched_risks:
        score += 5 * len(matched_risks)

    # 主领域只能作为轻微辅助，不能单独把指南顶上去
    if (
        primary_domain
        and primary_domain in query_domains
        and (primary_intent_matched or matched_signals or matched_risks or matched_intents)
    ):
        score += 3

    # primary 只能在已有明确匹配时加权，不能只靠同领域加权
    if ranking_role == "primary" and (
        primary_intent_matched or matched_signals or matched_risks
    ):
        score += 8

    return score


def score_guide_for_message(
    guide: Dict[str, Any],
    message: str,
    strategy: Dict[str, Any],
) -> int:
    query_profile = build_guide_query(strategy)
    query_domains = query_profile.get("domains", [])

    if query_domains and not guide_compatible_with_domains(guide, query_domains):
        return -100

    title_score = 0

    intent_score = _list_overlap_score(
        query_profile.get("intents", []),
        guide.get("intents"),
        34,
    )

    situation_score = _list_overlap_score(
        query_profile.get("situations", []),
        guide.get("situations"),
        18,
    )

    object_score = _list_overlap_score(
        query_profile.get("objects", []),
        guide.get("objects"),
        14,
    )

    term_score = _term_score(message, guide)

    score = (
        title_score
        + intent_score
        + situation_score
        + object_score
        + term_score
    )

    score += score_guide_metadata(guide, query_profile)

    guide_domain_set = set(guide_domains(guide))
    for index, domain in enumerate(query_domains):
        if domain in guide_domain_set:
            score += 14 if index == 0 else 8
        elif guide_compatible_with_domains(guide, [domain]):
            score += 2

    if query_profile.get("intents") and score < 22:
        return -100

    return score


def build_match_reason(guide: Dict[str, Any], query_profile: Dict[str, Any]) -> str:
    parts = []
    for label, q_key, g_key in [
        ("意图匹配", "intents", "intents"),
        ("场景匹配", "situations", "situations"),
        ("对象匹配", "objects", "objects"),
    ]:
        overlap = set(query_profile.get(q_key, [])) & set(guide.get(g_key) or [])
        if overlap:
            parts.append(label + "：" + "、".join(sorted(overlap)[:2]))
    if not parts:
        overlap = set(query_profile.get("domains", [])) & set(guide_domains(guide))
        if overlap:
            parts.append("领域匹配：" + "、".join(sorted(overlap)[:2]))
    return "；".join(parts) if parts else "结构化召回匹配"


def find_guides_by_message_and_domains(
    message: str,
    strategy: Dict[str, Any],
    guides: List[Dict[str, Any]],
    min_score: int = 8,
) -> List[Dict[str, Any]]:
    query_profile = build_guide_query(strategy)
    detected_domains = query_profile["domains"]

    effective_min_score = max(
        min_score,
        22 if query_profile.get("intents") else 8,
    )

    scored = []

    for guide in guides:
        score = score_guide_for_message(
            guide,
            message,
            strategy=strategy,
        )

        if score >= effective_min_score:
            item = dict(guide)
            item["_match_score"] = score
            item["_match_reason"] = build_match_reason(item, query_profile)
            scored.append(item)

    scored.sort(
        key=lambda item: item.get("_match_score", 0),
        reverse=True,
    )

    return scored


def find_domain_fallback_guides(domains: List[str], guides: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    if not domains:
        return []
    result = []
    seen = set()
    for guide in guides:
        guide_id = guide.get("id") or guide.get("title")
        if guide_id in seen:
            continue
        if guide_compatible_with_domains(guide, domains):
            result.append(guide)
            seen.add(guide_id)
    return result


def score_strategy_overlap(
    guide: Dict[str, Any],
    strategy: Dict[str, Any],
) -> int:

    filters = strategy.get("guide_filters", {})

    score = 0

    score += _list_overlap_score(
        filters.get("tasks", []),
        guide.get("tasks", []),
        100,
    )

    score += _list_overlap_score(
        filters.get("risks", []),
        guide.get("risks", []),
        60,
    )

    score += _list_overlap_score(
        filters.get("signals", []),
        guide.get("signals", []),
        40,
    )

    score += _list_overlap_score(
        filters.get("domains", []),
        guide_domains(guide),
        15,
    )

    return score