from typing import Any, Dict, List

from ..retrieval.domains import detect_domains_from_text
from .hooks import run_hooks

def prepare_pipeline_inputs(
    *,
    user_message: str,
    mode: str,
    matched_triggers: List[Dict[str, Any]] | None = None,
    related_guides: List[Dict[str, Any]] | None = None,
    related_wikis: List[Dict[str, Any]] | None = None,
    detected_domains: List[str] | None = None,
    context_data: Dict[str, Any] | None = None,
    rerank_state: Dict[str, Any] | None = None,
) -> Dict[str, Any]:
    """准备 PipelineRequest 所需的业务输入。

    当前阶段 routes.py 仍然负责实际查询本地触发规则、指南和 Wiki。
    preload 先负责统一兜底、规整和补全。
    后续再逐步把实际查询逻辑搬进这里或专门的 service。
    """

    hook_input = run_hooks("before_preload", {
        "user_message": user_message,
        "mode": mode,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
        "related_wikis": related_wikis,
        "detected_domains": detected_domains,
        "context_data": context_data,
        "rerank_state": rerank_state,
    })

    user_message = hook_input.get("user_message", user_message)
    mode = hook_input.get("mode", mode)
    matched_triggers = hook_input.get("matched_triggers", matched_triggers)
    related_guides = hook_input.get("related_guides", related_guides)
    related_wikis = hook_input.get("related_wikis", related_wikis)
    detected_domains = hook_input.get("detected_domains", detected_domains)
    context_data = hook_input.get("context_data", context_data)
    rerank_state = hook_input.get("rerank_state", rerank_state)

    normalized_message = str(user_message or "").strip()
    normalized_mode = str(mode or "emergency").strip() or "emergency"

    final_detected_domains = detected_domains or detect_domains_from_text(normalized_message)

    prepared = {
        "message": normalized_message,
        "mode": normalized_mode,
        "matched_triggers": matched_triggers or [],
        "related_guides": related_guides or [],
        "related_wikis": related_wikis or [],
        "detected_domains": final_detected_domains or [],
        "context_data": context_data or {},
        "rerank_state": rerank_state or {},
    }

    return run_hooks("after_preload", prepared)