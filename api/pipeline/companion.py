"""陪伴模式 Pipeline。保持轻量对话，不走完整应急检索管线。"""

from .schema import PipelineRequest, PipelineResult
from ..response.prompts import build_safe_history, build_companion_messages
from ..llm.client import call_ollama, stream_ollama


def run_companion_pipeline(request: PipelineRequest) -> PipelineResult:
    safe_history = build_safe_history(request.history)

    messages = build_companion_messages(
        user_message=request.message,
        safe_history=safe_history,
    )

    answer = call_ollama(messages)

    return PipelineResult(
        mode="companion",
        answer=answer,
        messages=messages,
        debug={
            "pipeline": "companion",
            "modules": ["response", "llm"],
        },
    )

def build_companion_pipeline_messages(request: PipelineRequest) -> list[dict[str, str]]:
    safe_history = build_safe_history(request.history)

    return build_companion_messages(
        user_message=request.message,
        safe_history=safe_history,
    )


def run_companion_stream_pipeline(request: PipelineRequest):
    messages = build_companion_pipeline_messages(request)

    return {
        "mode": "companion",
        "messages": messages,
        "debug": {
            "pipeline": "companion",
            "modules": ["response", "llm"],
        },
    }
