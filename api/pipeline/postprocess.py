from typing import Any, Dict, List, Optional

from .schema import PipelineResult



def build_ai_advice_response(
    *,
    result: PipelineResult,
    mode: str,
    matched_triggers: Optional[List[Dict[str, Any]]] = None,
    related_guides: Optional[List[Dict[str, Any]]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    detected_domains: Optional[List[str]] = None,
    context_data: Optional[Dict[str, Any]] = None,
    rerank_state: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    """把 PipelineResult 转换成 /api/ai/advice 的 JSON 响应。"""

    return {
        "answer": result.answer,
        "mode": mode,
        "matched_triggers": matched_triggers or [],
        "related_guides": related_guides or [],
        "related_wikis": related_wikis or [],
        "detected_domains": detected_domains or [],
        "context_data": context_data or {},
        "rerank_state": rerank_state or {},
        "pipeline": result.debug or {},
    }

def build_fallback_pipeline_result(
    *,
    mode: str,
    answer: str,
    reason: str = "pipeline_error",
) -> PipelineResult:
    return PipelineResult(
        mode=mode,
        answer=answer,
        debug={
            "pipeline": "fallback",
            "reason": reason,
        },
    )