"""Pipeline 预加载。

Retrieval v2 是当前 AI 助手主检索入口：
用户问题 -> AI Planner -> Source Fetchers -> AI Selector -> selected_evidence。

本文件只组织 Retrieval v2 的上下文预处理，不承载旧检索链路。
"""

from typing import Any, Dict, List, Tuple

from .hooks import run_hooks
from ..retrieval_v2.orchestrator import run_retrieval_v2


def _model_dump(value: Any) -> Any:
    """Return a plain JSON-serializable dict/list when possible."""
    if hasattr(value, "model_dump"):
        return value.model_dump()
    return value


def _evidence_raw(item: Any, reason: str = "") -> Dict[str, Any]:
    """Convert v2 EvidenceCandidate to the legacy raw shape expected by prompts/pages.

    这里不做语义判断，只把 v2 统一证据对象展开成旧前端/Prompt 能读取的 dict。
    """
    raw = dict(getattr(item, "raw", None) or {})

    raw.setdefault("id", getattr(item, "id", ""))
    raw.setdefault("title", getattr(item, "title", ""))
    raw.setdefault("summary", getattr(item, "summary", ""))
    raw.setdefault("category", getattr(item, "category", ""))
    raw.setdefault("tags", getattr(item, "tags", []) or [])
    raw.setdefault("snippet", getattr(item, "snippet", ""))

    raw["_retrieval_v2"] = {
        "source_type": getattr(item, "source_type", ""),
        "id": getattr(item, "id", ""),
        "title": getattr(item, "title", ""),
        "reason": reason,
    }

    return raw


def _split_selected_evidence(retrieval_v2_result: Any) -> Tuple[List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]]]:
    """Split selected v2 evidence into related_guides, related_wikis and related_kiwix."""
    selection = getattr(retrieval_v2_result, "selection", None)
    selected = getattr(selection, "selected", []) or []

    reason_by_key = {
        (getattr(item, "source_type", ""), getattr(item, "id", "")): getattr(item, "reason", "")
        for item in selected
    }

    related_guides: List[Dict[str, Any]] = []
    related_wikis: List[Dict[str, Any]] = []
    related_kiwix: List[Dict[str, Any]] = []

    for item in getattr(retrieval_v2_result, "selected_evidence", []) or []:
        source_type = getattr(item, "source_type", "")
        item_id = getattr(item, "id", "")
        reason = reason_by_key.get((source_type, item_id), "")

        raw = _evidence_raw(item, reason=reason)

        if source_type == "guide":
            related_guides.append(raw)
        elif source_type == "wiki":
            related_wikis.append(raw)
        elif source_type == "kiwix":
            related_kiwix.append(raw)

    return related_guides, related_wikis, related_kiwix


def _history_context_for_retrieval(history: List[Any] | None, limit: int = 4) -> str:
    user_messages: List[str] = []

    for item in history or []:
        if isinstance(item, dict):
            role = item.get("role")
            content = item.get("content")
        else:
            role = getattr(item, "role", None)
            content = getattr(item, "content", None)

        if role != "user":
            continue

        text = str(content or "").strip()
        if text:
            user_messages.append(text[:220])

    recent = user_messages[-limit:]
    if not recent:
        return ""

    return "\n".join(f"- {text}" for text in recent)


def _build_retrieval_query(
    *,
    user_message: str,
    conversation_summary: str = "",
    history: List[Any] | None = None,
) -> str:
    summary = str(conversation_summary or "").strip()[:900]
    recent_user_context = _history_context_for_retrieval(history)

    if not summary and not recent_user_context:
        return user_message

    return "\n".join([
        "当前问题：",
        user_message,
        "",
        "会话摘要：",
        summary or "暂无",
        "",
        "最近用户提到：",
        recent_user_context or "暂无",
    ])


def prepare_pipeline_inputs(
    *,
    user_message: str,
    mode: str,
    related_guides: List[Dict[str, Any]] | None = None,
    related_wikis: List[Dict[str, Any]] | None = None,
    related_kiwix: List[Dict[str, Any]] | None = None,
    detected_domains: List[str] | None = None,
    context_data: Dict[str, Any] | None = None,
    retrieval_v2: Dict[str, Any] | None = None,
) -> Dict[str, Any]:
    """Prepare normalized PipelineRequest inputs.

    This function is now a generic normalization hook layer. It does not run old retrieval.
    """

    hook_input = run_hooks("before_preload", {
        "user_message": user_message,
        "mode": mode,
        "related_guides": related_guides,
        "related_wikis": related_wikis,
        "related_kiwix": related_kiwix,
        "detected_domains": detected_domains,
        "context_data": context_data,
        "retrieval_v2": retrieval_v2,
    })

    normalized_message = str(hook_input.get("user_message", user_message) or "").strip()
    normalized_mode = str(hook_input.get("mode", mode) or "emergency").strip() or "emergency"

    prepared = {
        "message": normalized_message,
        "mode": normalized_mode,
        "related_guides": hook_input.get("related_guides", related_guides) or [],
        "related_wikis": hook_input.get("related_wikis", related_wikis) or [],
        "related_kiwix": hook_input.get("related_kiwix", related_kiwix) or [],
        "detected_domains": hook_input.get("detected_domains", detected_domains) or [],
        "context_data": hook_input.get("context_data", context_data) or {},
        "retrieval_v2": hook_input.get("retrieval_v2", retrieval_v2) or {},
    }

    return run_hooks("after_preload", prepared)


def prepare_ai_pipeline_context(
    *,
    user_message: str,
    mode: str,
    history: List[Any] | None = None,
    conversation_summary: str = "",
) -> Dict[str, Any]:
    """Build AI pipeline context using Retrieval v2 only."""

    normalized_message = str(user_message or "").strip()
    normalized_mode = str(mode or "emergency").strip() or "emergency"
    retrieval_query = _build_retrieval_query(
        user_message=normalized_message,
        conversation_summary=conversation_summary,
        history=history,
    )

    retrieval_v2_result = run_retrieval_v2(retrieval_query)
    retrieval_v2 = retrieval_v2_result.model_dump()

    related_guides, related_wikis, related_kiwix = _split_selected_evidence(retrieval_v2_result)

    plan = retrieval_v2_result.plan
    core_terms = list(getattr(plan, "core_terms", []) or [])

    context_data: Dict[str, Any] = {
        "engine": "retrieval_v2_ai_orchestrated",
        "mode": normalized_mode,
        "related_guides": related_guides,
        "related_wikis": related_wikis,
        "related_kiwix": related_kiwix,
        "core_terms": core_terms,
        "conversation_summary": str(conversation_summary or "").strip()[:1800],
        "retrieval_query": retrieval_query,
        "source_plan": [
            item.model_dump() if hasattr(item, "model_dump") else item
            for item in getattr(plan, "source_plan", []) or []
        ],
        "retrieval_v2": retrieval_v2,
    }

    # v2 不再使用旧 domain detector。这里保留字段，避免下游结构断裂。
    detected_domains: List[str] = []

    prepared = prepare_pipeline_inputs(
        user_message=normalized_message,
        mode=normalized_mode,
        related_guides=related_guides,
        related_wikis=related_wikis,
        related_kiwix=related_kiwix,
        detected_domains=detected_domains,
        context_data=context_data,
        retrieval_v2=retrieval_v2,
    )

    return {
        "context_data": prepared["context_data"],
        "detected_domains": prepared["detected_domains"],
        "related_guides": prepared["related_guides"],
        "related_wikis": prepared["related_wikis"],
        "related_kiwix": prepared["related_kiwix"],
        "retrieval_v2": prepared["retrieval_v2"],
    }
