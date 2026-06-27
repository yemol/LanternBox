from typing import Any, Dict, List


def match_triggers_for_message(
    *,
    message: str,
    mode: str = "emergency",
    trigger_items: List[Dict[str, Any]] | None = None,
) -> List[Dict[str, Any]]:
    """匹配用户输入对应的本地触发规则。

    当前阶段：
    - 先由 routes.py 传入 trigger_items
    - 后续再由 service 自己读取数据源
    """
    if not trigger_items:
        return []

    text = str(message or "").lower()
    results: List[Dict[str, Any]] = []

    for item in trigger_items:
        keywords = item.get("keywords", []) or []
        if isinstance(keywords, str):
            keywords = [keywords]

        if any(str(keyword).lower() in text for keyword in keywords):
            results.append(item)

    return results


def find_guides_for_triggers(
    *,
    triggers: List[Dict[str, Any]],
    guide_items: List[Dict[str, Any]] | None = None,
) -> List[Dict[str, Any]]:
    """根据触发规则查找关联指南。

    当前阶段仍然使用传入的 guide_items。
    后续再接入正式数据源。
    """
    if not triggers or not guide_items:
        return []

    guide_ids = set()

    for trigger in triggers:
        for guide_id in trigger.get("guide_ids", []) or []:
            guide_ids.add(str(guide_id))

    if not guide_ids:
        return []

    return [
        guide
        for guide in guide_items
        if str(guide.get("id", "")) in guide_ids
    ]