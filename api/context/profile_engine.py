from typing import Any

from .schema import LanternContext


def match_profile(
    profile: dict[str, Any],
    text: str,
) -> bool:

    match = profile["match"]

    all_words = match.get("all", [])
    any_words = match.get("any", [])
    none_words = match.get("none", [])

    if any(word in text for word in none_words):
        return False

    if any(word not in text for word in all_words):
        return False

    if not any_words:
        return True

    return any(word in text for word in any_words)

def apply_profile(
    profile: dict[str, Any],
    context: LanternContext,
) -> None:

    ctx = profile["context"]

    context.domains.extend(ctx.get("domains", []))
    context.signals.extend(ctx.get("signals", []))
    context.intents.extend(ctx.get("intents", []))
    context.risks.extend(ctx.get("risks", []))

    context.observations.extend(
        ctx.get("observations", [])
    )

    context.inferred_needs.extend(
        ctx.get("needs", [])
    )

    context.retrieval_plan.extend(
        ctx.get("retrieval_plan", [])
    )

    if ctx.get("risk_level"):
        context.risk_level = ctx["risk_level"]

    if "confidence" in ctx:
        context.confidence[profile["id"]] = ctx["confidence"]