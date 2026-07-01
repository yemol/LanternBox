"""Retrieval 应用层。根据运行时设置应用本地 AI 重排结果。"""


from typing import Any, Dict, List, Optional, Tuple
from .constants import AI_RERANK_VERSION
from ..config import OLLAMA_MODEL, load_runtime_settings
from .reranker import rerank_candidates_with_local_ai


def apply_ai_rerank_if_enabled(user_message, context_data, related_guides):
    """根据运行时设置决定是否让本地 AI 重排候选来源。"""

    runtime_settings = load_runtime_settings()
    enable_ai_rerank = bool(runtime_settings.get("ai_rerank_enabled"))
    rerank_model = runtime_settings.get("ai_rerank_model") or OLLAMA_MODEL

    candidates = context_data.get("candidate_sources") or []
    strategy = context_data.get("retrieval_strategy", {})

    rerank_result = rerank_candidates_with_local_ai(
        user_message=user_message,
        candidates=candidates,
        strategy=strategy,
        model=rerank_model,
        enable_ai_rerank=enable_ai_rerank,
        max_selected=6,
    )

    selected_sources = (
        rerank_result.get("selected_sources")
        or context_data.get("selected_sources")
        or []
    )

    excluded_sources = (
        rerank_result.get("excluded_sources")
        or context_data.get("excluded_sources")
        or []
    )

    def source_raw_key(source):
        if not isinstance(source, dict):
            return None

        return (
            source.get("raw_id")
            or source.get("source_id")
            or str(source.get("candidate_id") or "").split(":", 1)[-1]
            or source.get("title")
        )

    selected_guide_ids = {
        source_raw_key(item)
        for item in selected_sources
        if item.get("source_type") == "guide" and source_raw_key(item)
    }

    if selected_guide_ids:
        guide_by_id = {
            (guide.get("id") or guide.get("title")): guide
            for guide in related_guides
        }

        ordered_guides = []
        seen = set()

        for source in selected_sources:
            if source.get("source_type") != "guide":
                continue

            raw_id = source_raw_key(source)
            guide = guide_by_id.get(raw_id)

            if guide and raw_id not in seen:
                if source.get("rerank_reason") or source.get("reason"):
                    guide = dict(guide)
                    guide["_ai_rerank_reason"] = (
                        source.get("rerank_reason")
                        or source.get("reason")
                    )
                    guide["_ai_rerank_mode"] = (
                        source.get("rerank_mode")
                        or rerank_result.get("mode")
                    )

                ordered_guides.append(guide)
                seen.add(raw_id)

        for guide in related_guides:
            raw_id = guide.get("id") or guide.get("title")
            if raw_id not in seen:
                ordered_guides.append(guide)
                seen.add(raw_id)

        related_guides = ordered_guides[:10]

    retrieval_decision = dict(
        context_data.get("retrieval_decision") or {}
    )

    retrieval_decision.update({
        "runtime_settings": runtime_settings,
        "rerank_result": {
            "version": rerank_result.get("version"),
            "mode": rerank_result.get("mode"),
            "used_ai": rerank_result.get("used_ai", False),
            "fallback_reason": rerank_result.get("fallback_reason"),
            "intent_summary": rerank_result.get("intent_summary"),
            "answer_focus": rerank_result.get("answer_focus", []),
            "rejected_candidate_ids": rerank_result.get("rejected_candidate_ids", []),
            "selected_candidate_ids": rerank_result.get("selected_candidate_ids", []),
            "error": rerank_result.get("error"),
        },
    })

    return {
        "related_guides": related_guides,
        "selected_sources": selected_sources,
        "excluded_sources": excluded_sources,
        "retrieval_decision": retrieval_decision,
        "runtime_settings": runtime_settings,
    }