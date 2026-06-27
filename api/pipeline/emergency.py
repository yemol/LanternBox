from .schema import PipelineRequest, PipelineResult
from ..response.prompts import build_safe_history, build_emergency_messages
from ..llm.client import call_ollama


def run_emergency_pipeline(request: PipelineRequest) -> PipelineResult:
    safe_history = build_safe_history(request.history)

    messages = build_emergency_messages(
        user_message=request.message,
        matched_triggers=request.matched_triggers,
        related_guides=request.related_guides,
        detected_domains=request.detected_domains,
        safe_history=safe_history,
        related_wikis=request.related_wikis,
    )

    answer = call_ollama(messages)

    return PipelineResult(
        mode="emergency",
        answer=answer,
        messages=messages,
        debug={
            "pipeline": "emergency",
            "modules": [
                "context",
                "retrieval",
                "response",
                "llm",
            ],
            "planner": "not_enabled",
        },
    )