"""
Candidate Builder

负责根据 Retrieval Strategy 构建候选集合。

职责：
- Guide Candidate
- Wiki Candidate
- Inventory Candidate
- Record Candidate

不负责排序。
不负责 AI。
不负责 Response。
"""

from typing import Any


from typing import Any

from .guide import (
    build_match_reason,
    find_domain_fallback_guides,
    find_guides_by_message_and_domains,
    score_guide_for_message,
)

from ..services.guide_service import find_related_guides

def merge_guides(*guide_lists):
    merged = []
    seen = set()

    for guide_list in guide_lists:
        for guide in guide_list:
            guide_id = guide.get("id") or guide.get("title")

            if guide_id in seen:
                continue

            seen.add(guide_id)
            merged.append(guide)

    return merged

def build_guide_candidates(
    *,
    strategy: dict[str, Any],
    user_message: str,
    guides: list[dict[str, Any]],
    matched_triggers: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    trigger_guides = []

    guide_filters = strategy.get("guide_filters", {})

    detected_domains = guide_filters.get("domains", [])

    query_profile = {
        "domains": detected_domains,
        "intents": guide_filters.get("tasks", []),
    }

    for guide in find_related_guides(matched_triggers, guides):
        score = score_guide_for_message(
            guide,
            user_message,
            detected_domains,
            strategy=strategy,
        )

        if score >= 24:
            item = dict(guide)
            item["_match_score"] = score
            item["_match_reason"] = build_match_reason(item, query_profile)
            trigger_guides.append(item)

    scored_guides = find_guides_by_message_and_domains(
        message=user_message,
        strategy=strategy,
        guides=guides,
        min_score=8,
    )

    domain_guides = []

    if not scored_guides and detected_domains:
        domain_guides = find_domain_fallback_guides(
            detected_domains,
            guides,
        )[:4]

    guide_pool = merge_guides(
        scored_guides,
        trigger_guides,
        domain_guides,
    )

    guide_pool.sort(
        key=lambda item: item.get(
            "_match_score",
            item.get("_domain_score", 0),
        ),
        reverse=True,
    )

    return guide_pool[:16]