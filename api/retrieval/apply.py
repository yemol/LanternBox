
from typing import Any, Dict, List, Optional, Tuple
from .constants import AI_RERANK_VERSION

def get_candidate_raw_item_from_ai(candidate: Dict[str, Any]) -> Dict[str, Any]:
    raw = candidate.get("raw")
    return raw if isinstance(raw, dict) else candidate


def apply_rerank_result_to_context(
    context: Dict[str, Any],
    rerank_result: Dict[str, Any],
) -> Dict[str, Any]:
    """把 AI/规则重排结果回写到 prepare_ai_context 的上下文结构中。

    这个函数供后端路由调用。resources.py 不直接调用 ai.py，避免循环依赖。
    """
    updated = dict(context or {})
    selected_sources = rerank_result.get("selected_sources", []) or []
    excluded_sources = rerank_result.get("excluded_sources", []) or []

    updated["selected_sources"] = selected_sources
    updated["excluded_sources"] = excluded_sources
    if rerank_result.get("lantern_context"):
        updated["lantern_context"] = rerank_result.get("lantern_context")
    updated["related_guides"] = [
        get_candidate_raw_item_from_ai(item)
        for item in selected_sources
        if item.get("source_type") == "guide"
    ][:10]

    decision = dict(updated.get("retrieval_decision") or {})
    decision.update({
        "version": AI_RERANK_VERSION,
        "mode": rerank_result.get("mode"),
        "used_ai": rerank_result.get("used_ai", False),
        "intent_summary": rerank_result.get("intent_summary") or decision.get("intent_summary"),
        "answer_focus": rerank_result.get("answer_focus", []),
        "lantern_context": rerank_result.get("lantern_context"),
        "selected_count": len(selected_sources),
        "excluded_count": len(excluded_sources),
        "rejected_candidate_ids": rerank_result.get("rejected_candidate_ids", []),
    })
    updated["retrieval_decision"] = decision
    return updated

def build_selected_sources_text(selected_sources: List[Dict[str, Any]]) -> str:
    blocks = []
    for item in selected_sources[:8]:
        blocks.append("\n".join([
            f"来源ID：{item.get('candidate_id', '')}",
            f"来源类型：{item.get('source_label') or item.get('source_type', '')}",
            f"标题：{item.get('title', '')}",
            f"选择原因：{item.get('reason', '')}",
            f"摘要：{item.get('summary', '')}",
        ]))
    return "\n\n".join(blocks)
