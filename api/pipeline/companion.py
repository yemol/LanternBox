from .schema import PipelineRequest, PipelineResult
from ..response.prompts import build_safe_history, build_companion_messages
from ..llm.client import call_ollama


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