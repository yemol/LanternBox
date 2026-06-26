from .schema import LanternContext
from .rules import apply_rule_signals


def dedupe(items: list[str]) -> list[str]:
    seen = set()
    result = []
    for item in items:
        if item not in seen:
            seen.add(item)
            result.append(item)
    return result


def analyze_context(text: str, source: str = "text") -> LanternContext:
    context = LanternContext(
        source=source,
        input_text=text,
    )

    context = apply_rule_signals(text, context)

    context.domains = dedupe(context.domains)
    context.signals = dedupe(context.signals)
    context.observations = dedupe(context.observations)
    context.risks = dedupe(context.risks)
    context.inferred_needs = dedupe(context.inferred_needs)
    context.retrieval_plan = dedupe(context.retrieval_plan)

    if not context.signals:
        context.input_nature = "general_input"
        context.risk_level = "low"

    return context