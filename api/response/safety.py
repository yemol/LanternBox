

FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS = [
    "联系物业",
    "联系供水公司",
    "联系供电公司",
    "联系电力公司",
    "联系客服",
    "拨打供电",
    "拨打供水",
    "叫外卖",
    "点外卖",
    "快递",
    "网上搜索",
    "上网查询",
]


def sanitize_ai_answer(answer: str, mode: str = "emergency") -> str:
    """最终回答安全清洗。

    这是提示词之外的确定性兜底：应急模式下如果模型仍把城市服务或
    外部稳定系统写成行动步骤，就移除包含这些词的整句/整行。
    """
    text = str(answer or "")
    if mode != "emergency" or not text.strip():
        return text

    lines = text.splitlines()
    cleaned = []
    removed = False

    for line in lines:
        if any(term in line for term in FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS):
            removed = True
            continue
        cleaned.append(line)

    result = "\n".join(cleaned).strip()
    if removed:
        note = "\n\n补充约束：当前按无外部支援场景处理，优先执行本地可控措施，并持续记录与复查。"
        if "无外部支援" not in result:
            result = (result + note).strip()

    return result or "当前按无外部支援场景处理。请先执行本地可控安全动作，并记录变化。"
