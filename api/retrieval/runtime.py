"""Retrieval Runtime。负责候选池、硬排除、来源选择和检索决策。"""

from typing import Any, Dict, List, Tuple

from ..utils import unique_list
from .guide import build_guide_query
from .candidate_source import build_candidate_source, get_candidate_raw_item


HYBRID_RAG_VERSION = "v0.6"


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
    strategy: Dict[str, Any] | None = None,
    guide_candidates: List[Dict[str, Any]],
    wiki_candidates: List[Dict[str, Any]] = None,
    inventory_candidates: List[Dict[str, Any]] = None,
    record_candidates: List[Dict[str, Any]] = None,
    include_kiwix: bool = False,
) -> List[Dict[str, Any]]:
    pool: List[Dict[str, Any]] = []

    query_profile = build_guide_query(strategy or {})

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
    strategy: Dict[str, Any],
    candidates: List[Dict[str, Any]],
    selected: List[Dict[str, Any]],
    excluded: List[Dict[str, Any]],
    mode: str = "rule",
) -> Dict[str, Any]:
    focus = []

    query_profile = build_guide_query(strategy)

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
