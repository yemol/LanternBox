"""Pipeline 调度器。根据 mode 分发到对应 Pipeline。"""

from .schema import PipelineRequest, PipelineResult
from .companion import run_companion_pipeline
from .emergency import run_emergency_pipeline

from .companion import run_companion_pipeline, run_companion_stream_pipeline
from .emergency import run_emergency_pipeline, run_emergency_stream_pipeline
from .hooks import run_hooks

def normalize_mode(mode: str) -> str:
    mode = (mode or "").strip().lower()

    if mode in {"companion", "陪伴", "companion_mode"}:
        return "companion"

    if mode in {"emergency", "应急", "emergency_mode"}:
        return "emergency"

    return "emergency"


def run_ai_pipeline(request: PipelineRequest) -> PipelineResult:
    hook_data = run_hooks("before_dispatch", {
        "mode": normalize_mode(request.mode),
        "request": request,
    })

    mode = hook_data.get("mode", normalize_mode(request.mode))
    request = hook_data.get("request", request)

    if mode == "companion":
        result = run_companion_pipeline(request)
    else:
        result = run_emergency_pipeline(request)

    hook_result = run_hooks("after_dispatch", {
        "mode": mode,
        "request": request,
        "result": result,
    })

    return hook_result.get("result", result)

def run_ai_stream_pipeline(request: PipelineRequest):
    hook_data = run_hooks("before_dispatch", {
        "mode": normalize_mode(request.mode),
        "request": request,
        "stream": True,
    })

    mode = hook_data.get("mode", normalize_mode(request.mode))
    request = hook_data.get("request", request)

    if mode == "companion":
        result = run_companion_stream_pipeline(request)
    else:
        result = run_emergency_stream_pipeline(request)

    hook_result = run_hooks("after_dispatch", {
        "mode": mode,
        "request": request,
        "result": result,
        "stream": True,
    })

    return hook_result.get("result", result)
