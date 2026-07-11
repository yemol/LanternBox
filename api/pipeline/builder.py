"""Pipeline 请求构建器。将 API payload 规整为 PipelineRequest。"""

from typing import Any, Dict, List, Optional

from .schema import PipelineRequest
from .preload import prepare_pipeline_inputs



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
    related_guides: Optional[List[Dict[str, Any]]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    related_kiwix: Optional[List[Dict[str, Any]]] = None,
    detected_domains: Optional[List[str]] = None,
    context_data: Optional[Dict[str, Any]] = None,
    retrieval_v2: Optional[Dict[str, Any]] = None,
) -> PipelineRequest:
    prepared = prepare_pipeline_inputs(
        user_message=getattr(payload, "message", ""),
        mode=getattr(payload, "mode", "emergency"),
        related_guides=related_guides,
        related_wikis=related_wikis,
        related_kiwix=related_kiwix,
        detected_domains=detected_domains,
        context_data=context_data,
        retrieval_v2=retrieval_v2,
    )

    return PipelineRequest(
        message=prepared["message"],
        mode=prepared["mode"],
        history=normalize_history_for_pipeline(getattr(payload, "history", [])),
        conversation_summary=str(getattr(payload, "conversation_summary", "") or "").strip()[:1800],
        related_guides=prepared["related_guides"],
        related_wikis=prepared["related_wikis"],
        related_kiwix=prepared["related_kiwix"],
        detected_domains=prepared["detected_domains"],
        retrieval_v2=prepared["retrieval_v2"],
        stream=stream,
        metadata={
            "context_data": prepared["context_data"],
            "retrieval_v2": prepared["retrieval_v2"],
        },
    )
