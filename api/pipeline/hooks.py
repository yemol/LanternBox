from typing import Any, Callable, Dict, List


PipelineHook = Callable[[Dict[str, Any]], Dict[str, Any]]


HOOKS: Dict[str, List[PipelineHook]] = {
    "before_preload": [],
    "after_preload": [],
    "before_dispatch": [],
    "after_dispatch": [],
    "before_response": [],
    "after_response": [],
}


def register_hook(stage: str, hook: PipelineHook) -> None:
    if stage not in HOOKS:
        HOOKS[stage] = []
    HOOKS[stage].append(hook)


def run_hooks(stage: str, data: Dict[str, Any]) -> Dict[str, Any]:
    result = data

    for hook in HOOKS.get(stage, []):
        try:
            updated = hook(result)
            if isinstance(updated, dict):
                result = updated
        except Exception as exc:
            result.setdefault("_hook_errors", []).append({
                "stage": stage,
                "error": str(exc)[:200],
            })

    return result