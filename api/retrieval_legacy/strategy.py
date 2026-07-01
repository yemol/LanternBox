"""
Retrieval Strategy

负责根据 Context 生成 Retrieval Strategy。

注意：
- 不认识 Guide
- 不认识 Wiki
- 不认识 Inventory
- 不做任何检索

这里只负责描述：
"接下来应该检索什么。"
"""

from typing import Any

def should_retrieve_guides(context) -> bool:
    return any(
        [
            getattr(context, "domains", None),
            getattr(context, "intents", None),
            getattr(context, "signals", None),
            getattr(context, "risks", None),
            getattr(context, "retrieval_plan", None),
        ]
    )


def build_retrieval_strategy(context) -> dict[str, Any]:
    strategy = {
        "domains": list(context.domains),
        "signals": list(context.signals),
        "risks": list(context.risks),
        "tasks": list(context.intents),
        "retrieval_plan": list(context.retrieval_plan),

        # 以后 Candidate Builder 使用
        "guide_filters": {},
        "wiki_filters": {},
        "inventory_filters": {},
        "guide_retrieval_enabled": should_retrieve_guides(context),
    }

    # ---------- Guide ----------
    strategy["guide_filters"] = {
        "domains": strategy["domains"],
        "signals": strategy["signals"],
        "risks": strategy["risks"],
        "tasks": strategy["tasks"],
    }

    # ---------- Wiki ----------
    strategy["wiki_filters"] = {
        "domains": strategy["domains"],
    }

    # ---------- Inventory ----------
    strategy["inventory_filters"] = {
        "domains": strategy["domains"],
    }

    return strategy