"""回答安全清洗。负责移除应急模式下不合适的外部依赖建议。"""

import re


FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS = [
    "等待救援",
    "等待专业救援",
    "联系医院",
    "联系相关部门",
    "拨打电话",
    "拨打急救电话",
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
    "等专业人员到场",
    "等维修人员到场",
    "联系供应商",
]

NEGATION_MARKERS = ["不要", "不应", "不能", "不可", "不得", "无需", "不必", "别", "避免"]
OPTIONAL_MARKERS = ["若仍可用", "如仍可用", "可作为补充", "仅作为补充", "条件允许时"]
LOCAL_FALLBACK_MARKERS = ["无法联络", "无法联系", "否则", "同时继续", "本地", "仍要", "仍需"]


def _term_is_negated(line: str, term: str) -> bool:
    for index in [match.start() for match in re.finditer(re.escape(term), line)]:
        prefix = line[max(0, index - 12):index]
        if any(marker in prefix for marker in NEGATION_MARKERS):
            return True
    return False


def _optional_with_local_fallback(line: str) -> bool:
    return any(marker in line for marker in OPTIONAL_MARKERS) and any(marker in line for marker in LOCAL_FALLBACK_MARKERS)


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
        matched = [term for term in FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS if term in line]
        actionable = [term for term in matched if not _term_is_negated(line, term)]
        if actionable and not _optional_with_local_fallback(line):
            removed = True
            prefix = ""
            stripped = line.lstrip()
            if stripped.startswith(("-", "*", "1.", "2.", "3.", "4.", "5.")):
                prefix = line[:len(line) - len(stripped)] + "- "
            cleaned.append(prefix + "立即执行现场可控的停止、隔离、撤离或停用动作；无法联络时继续按本地方案观察、记录和复查。")
            continue
        cleaned.append(line)

    result = "\n".join(cleaned).strip()
    if removed:
        note = "\n\n补充约束：不要等待外部支援才行动；当前优先执行本地可控措施，并持续记录与复查。"
        if "无外部支援" not in result:
            result = (result + note).strip()

    return result or "当前按无外部支援场景处理。请先执行本地可控安全动作，并记录变化。"
