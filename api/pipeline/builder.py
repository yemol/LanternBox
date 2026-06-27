from typing import Any, Dict, List, Optional

from .schema import PipelineRequest



def normalize_history_for_pipeline(history: Optional[List[Any]]) -> List[Dict[str, str]]:
    """把前端历史消息统一转成 Pipeline 可用的 dict。

    支持：
    - dict
    - Pydantic model
    - 普通对象
    """
    result: List[Dict[str, str]] = []

    for item in history or []:
        if isinstance(item, dict):
            role = item.get("role", "")
            content = item.get("content", "")
        elif hasattr(item, "model_dump"):
            data = item.model_dump()
            role = data.get("role", "")
            content = data.get("content", "")
        elif hasattr(item, "dict"):
            data = item.dict()
            role = data.get("role", "")
            content = data.get("content", "")
        else:
            role = getattr(item, "role", "")
            content = getattr(item, "content", "")

        role = str(role or "").strip()
        content = str(content or "").strip()

        if role and content:
            result.append({
                "role": role,
                "content": content,
            })

    return result


def build_pipeline_request(
    payload: Any,
    *,
    stream: bool = False,
    matched_triggers: Optional[List[Dict[str, Any]]] = None,
    related_guides: Optional[List[Dict[str, Any]]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    detected_domains: Optional[List[str]] = None,
    context_data: Optional[Dict[str, Any]] = None,
    rerank_state: Optional[Dict[str, Any]] = None,
) -> PipelineRequest:
    user_message = str(getattr(payload, "message", "") or "").strip()
    mode = str(getattr(payload, "mode", "emergency") or "emergency").strip()

    return PipelineRequest(
        message=user_message,
        mode=mode,
        history=normalize_history_for_pipeline(getattr(payload, "history", [])),
        matched_triggers=matched_triggers or [],
        related_guides=related_guides or [],
        related_wikis=related_wikis or [],
        detected_domains=detected_domains or [],
        stream=stream,
        metadata={
            "context_data": context_data or {},
            "rerank_state": rerank_state or {},
        },
    )