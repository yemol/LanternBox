"""应急模式 Pipeline。组织应急模式的 Response 与 LLM 调用。"""

from .schema import PipelineRequest, PipelineResult
from ..response.prompts import build_safe_history, build_emergency_messages
from ..llm.client import call_ollama


def run_emergency_pipeline(request: PipelineRequest) -> PipelineResult:
    messages = build_emergency_pipeline_messages(request)
    answer = call_ollama(messages)

    return PipelineResult(
        mode="emergency",
        answer=answer,
        messages=messages,
        debug={
            "pipeline": "emergency",
            "modules": ["retrieval_v2", "response", "llm"],
            "planner": "ai_orchestrated",
            "retrieval_v2": request.retrieval_v2,
        },
    )

def build_emergency_pipeline_messages(request: PipelineRequest) -> list[dict[str, str]]:
    safe_history = build_safe_history(request.history)

    return build_emergency_messages(
        user_message=request.message,
        matched_triggers=request.matched_triggers,
        related_guides=request.related_guides,
        detected_domains=request.detected_domains,
        safe_history=safe_history,
        conversation_summary=request.conversation_summary,
        related_wikis=request.related_wikis,
    )

def run_emergency_stream_pipeline(request: PipelineRequest):
    messages = build_emergency_pipeline_messages(request)

    return {
        "mode": "emergency",
        "messages": messages,
        "debug": {
            "pipeline": "emergency",
            "modules": ["retrieval_v2", "response", "llm"],
            "planner": "ai_orchestrated",
            "retrieval_v2": request.retrieval_v2,
        },
    }
