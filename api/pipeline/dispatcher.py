from .schema import PipelineRequest, PipelineResult
from .companion import run_companion_pipeline
from .emergency import run_emergency_pipeline

from .companion import run_companion_pipeline, run_companion_stream_pipeline
from .emergency import run_emergency_pipeline, run_emergency_stream_pipeline


def normalize_mode(mode: str) -> str:
    mode = (mode or "").strip().lower()

    if mode in {"companion", "陪伴", "companion_mode"}:
        return "companion"

    if mode in {"emergency", "应急", "emergency_mode"}:
        return "emergency"

    return "emergency"


def run_ai_pipeline(request: PipelineRequest) -> PipelineResult:
    mode = normalize_mode(request.mode)

    if mode == "companion":
        return run_companion_pipeline(request)

    return run_emergency_pipeline(request)


def run_ai_stream_pipeline(request: PipelineRequest):
    mode = normalize_mode(request.mode)

    if mode == "companion":
        return run_companion_stream_pipeline(request)

    return run_emergency_stream_pipeline(request)