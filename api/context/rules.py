"""Context Engine 规则库。负责输入性质、风险与需求的规则识别。"""

from .schema import LanternContext


WATER_KEYWORDS = ["水", "没水", "缺水", "只剩", "一桶", "喝水", "饮水", "净水"]
HEAT_KEYWORDS = ["热", "很热", "高温", "暴晒", "太阳", "中暑", "闷热"]
FOOD_KEYWORDS = [
    "食物",
    "粮食",
    "粮",
    "断粮",
    "没吃的",
    "口粮",
    "食材",
]
FOOD_SHORTAGE_KEYWORDS = [
    "只剩",
    "剩一点",
    "快没",
    "不足",
    "告急",
    "断粮",
    "最后一点",
]

def apply_rule_signals(text: str, context: LanternContext) -> LanternContext:
    has_water = any(k in text for k in WATER_KEYWORDS)
    has_heat = any(k in text for k in HEAT_KEYWORDS)
    has_food = any(k in text for k in FOOD_KEYWORDS)
    has_food_shortage = any(k in text for k in FOOD_SHORTAGE_KEYWORDS)

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

    if has_food:

        context.domains.extend(["food", "resource"])

        if has_food_shortage:
            context.signals.append("resource_low")
            context.intents.append("food_shortage")
            context.risks.append("starvation")
            context.observations.append("食物资源可能不足")
            context.inferred_needs.extend([
                "口粮管理",
                "寻找替代食物",
                "降低体力消耗",
            ])
            context.retrieval_plan.extend([
                "guide.food_shortage",
                "guide.rationing",
                "guide.food_acquisition"
            ])

            context.risk_level = "high"

            context.confidence["food_shortage"] = 0.92

    if has_water and has_heat:
        context.input_nature = "situation_report"
        context.risk_level = "high"
        context.confidence["combined_water_heat_risk"] = 0.88
    elif has_water or has_heat:
        context.input_nature = "situation_report"
        context.risk_level = "medium"

    return context
