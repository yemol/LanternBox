from .schema import LanternContext


WATER_KEYWORDS = ["水", "没水", "缺水", "只剩", "一桶", "喝水", "饮水", "净水"]
HEAT_KEYWORDS = ["热", "很热", "高温", "暴晒", "太阳", "中暑", "闷热"]


def apply_rule_signals(text: str, context: LanternContext) -> LanternContext:
    has_water = any(k in text for k in WATER_KEYWORDS)
    has_heat = any(k in text for k in HEAT_KEYWORDS)

    if has_water:
        context.domains.extend(["water", "resource"])
        context.signals.append("water_shortage")
        context.observations.append("水资源可能不足")
        context.risks.extend(["dehydration", "water_depletion"])
        context.inferred_needs.extend(["节水策略", "饮水优先级", "寻找或净化水源"])
        context.retrieval_plan.extend(["节水", "饮水优先级", "净水", "水源寻找"])
        context.confidence["water_shortage"] = 0.9

    if has_heat:
        context.domains.extend(["weather", "health"])
        context.signals.append("heat_exposure")
        context.observations.append("环境可能处于高温状态")
        context.risks.extend(["heatstroke", "dehydration"])
        context.inferred_needs.extend(["高温防护", "中暑预防"])
        context.retrieval_plan.extend(["中暑预防", "高温应对", "降温"])
        context.confidence["heat_exposure"] = 0.85

    if has_water and has_heat:
        context.input_nature = "situation_report"
        context.risk_level = "high"
        context.confidence["combined_water_heat_risk"] = 0.88
    elif has_water or has_heat:
        context.input_nature = "situation_report"
        context.risk_level = "medium"

    return context