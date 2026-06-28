"""引用来源排序模块。负责 Guide / Wiki 等资料的过滤与排序。"""

from typing import Any, Dict, List, Optional, Tuple

from .constants import (
    CATEGORY_DOMAIN_HINTS,
    REFERENCE_MAX_GUIDES,
    REFERENCE_MAX_WIKIS,
)
from .domains import (
    detect_domains_from_text,
    detect_reference_domains,
    domains_compatible,
)
from .query import normalize_reference_text, tokenize_reference_query

def reference_item_text(item: Dict[str, Any]) -> str:
    fields = [
        "id", "title", "category", "category_original", "summary", "scenario", "goal",
        "tags", "source", "notes", "fallback", "content",
        "tools", "steps", "check", "common_mistakes", "stop_or_escalate",
    ]
    return " ".join(normalize_reference_text(item.get(field)) for field in fields)


def infer_item_domains(item: Dict[str, Any]) -> List[str]:
    category = normalize_reference_text([item.get("category"), item.get("category_original")])
    full_text = reference_item_text(item)

    domains: List[str] = []

    for hint, domain in CATEGORY_DOMAIN_HINTS.items():
        if hint in category and domain not in domains:
            domains.append(domain)

    for domain in detect_domains_from_text(full_text, for_query=False):
        if domain not in domains:
            domains.append(domain)

    return domains


def lexical_reference_score(user_message: str, item: Dict[str, Any]) -> Tuple[int, List[str]]:
    tokens = tokenize_reference_query(user_message)

    title = normalize_reference_text(item.get("title"))
    category = normalize_reference_text([item.get("category"), item.get("category_original")])
    tags = normalize_reference_text(item.get("tags"))
    summary = normalize_reference_text([item.get("summary"), item.get("scenario"), item.get("goal")])
    full = reference_item_text(item)

    score = 0
    matched_tokens: List[str] = []

    for token in tokens:
        token_score = 0
        if token in title:
            token_score += 10
        elif token in tags:
            token_score += 7
        elif token in category:
            token_score += 5
        elif token in summary:
            token_score += 4
        elif token in full:
            token_score += 1

        if token_score:
            score += token_score
            if token not in matched_tokens:
                matched_tokens.append(token)

    return score, matched_tokens[:6]

def validate_reference_for_query(
    user_message: str,
    item: Dict[str, Any],
    item_type: str = "guide",
    detected_domains: Optional[List[str]] = None,
) -> Dict[str, Any]:
    query_domains = detect_reference_domains(user_message, detected_domains)
    item_domains = infer_item_domains(item)
    lexical_score, matched_tokens = lexical_reference_score(user_message, item)
    compatible = domains_compatible(query_domains, item_domains)

    # 硬门禁：用户问题已经有明确领域，而候选资料领域不兼容时，
    # 必须有非常强的词面命中才放行。否则一律过滤。
    if query_domains and not compatible and lexical_score < 10:
        return {
            "accepted": False,
            "score": -100,
            "reason": f"领域不匹配：问题属于 {','.join(query_domains)}，资料属于 {','.join(item_domains) or '未知'}。",
            "query_domains": query_domains,
            "item_domains": item_domains,
            "matched_tokens": matched_tokens,
        }

    score = lexical_score

    if compatible:
        score += 6
    else:
        score -= 8

    risk_level = str(item.get("risk_level") or "").lower()
    if risk_level == "high":
        score += 2
    elif risk_level == "caution":
        score += 1

    threshold = 4 if query_domains else 3
    accepted = score >= threshold

    if matched_tokens:
        reason = f"命中关键词：{'、'.join(matched_tokens[:4])}"
    elif compatible and item_domains:
        reason = f"结构化召回：同属相关领域：{'、'.join(item_domains[:2])}"
    else:
        reason = "相关度不足，已过滤"

    return {
        "accepted": accepted,
        "score": score,
        "reason": reason,
        "query_domains": query_domains,
        "item_domains": item_domains,
        "matched_tokens": matched_tokens,
    }


def filter_and_rank_ai_references(
    user_message: str,
    related_guides: Optional[List[Dict[str, Any]]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    detected_domains: Optional[List[str]] = None,
    max_guides: int = REFERENCE_MAX_GUIDES,
    max_wikis: int = REFERENCE_MAX_WIKIS,
    min_score: Optional[int] = None,
) -> Dict[str, List[Dict[str, Any]]]:
    """过滤并收敛 AI 来源。

    v0.5.1 调整重点：
    1. resources.py 已经完成结构化召回和排序，因此这里不再大幅重排指南。
    2. 明确领域问题只保留领域兼容来源。
    3. 对同领域但弱相关的来源，优先保留上游排序靠前的少量条目。
    4. security 场景默认只展示 3 条指南，避免“泛安全资料”挤占页面。
    """

    query_domains = detect_reference_domains(user_message, detected_domains)

    # 明确安全场景下，来源越少越稳。宁可 3 条精准，不要 6 条泛化。
    guide_limit = max_guides
    if "security" in query_domains:
        guide_limit = min(max_guides, 3)

    def process(
        items: Optional[List[Dict[str, Any]]],
        item_type: str,
        limit: int,
        preserve_upstream_order: bool = True,
    ) -> List[Dict[str, Any]]:
        accepted_items = []

        for index, item in enumerate(items or []):
            if not isinstance(item, dict):
                continue

            result = validate_reference_for_query(
                user_message=user_message,
                item=item,
                item_type=item_type,
                detected_domains=detected_domains,
            )

            if not result["accepted"]:
                continue

            item_copy = dict(item)

            # 让前端能展示后端验收理由；同时保留原始顺序，便于后续排查。
            item_copy["_reference_score"] = result["score"]
            item_copy["_reference_reason"] = result["reason"]
            item_copy["_reference_domains"] = result["item_domains"]
            item_copy["_reference_matched_terms"] = result["matched_tokens"]
            item_copy["_reference_original_index"] = index
            accepted_items.append(item_copy)

        if preserve_upstream_order:
            # 结构化召回阶段已经按相关度排序，这里只做验收和截断。
            # 不再让“词面误加分”把泛化资料顶到前面。
            return accepted_items[:limit]

        accepted_items.sort(
            key=lambda item: (
                item.get("_reference_score", 0),
                -item.get("_reference_original_index", 0),
            ),
            reverse=True,
        )
        return accepted_items[:limit]

    return {
        "guides": process(related_guides, "guide", guide_limit, preserve_upstream_order=True),
        "wikis": process(related_wikis, "wiki", max_wikis, preserve_upstream_order=False),
    }
