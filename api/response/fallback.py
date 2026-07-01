"""本地兜底回答构造。用于模型失败或 metadata_only 等降级场景。"""

from typing import Any, Dict, List, Optional, Tuple
from .safety import sanitize_ai_answer

def build_fallback_answer(
    mode: str,
    related_guides: List[Dict[str, Any]],
) -> str:
    if mode == "companion":
        return "本地 AI 模型暂时没有响应。先别急，我们可以从最小的一步开始：把你现在最在意的一件事写下来，然后只处理它。"

    answer = "\n".join([
        "本地 AI 模型暂时没有响应。以下是根据 Retrieval v2 已选本地资料生成的基础建议：",
        "",
        "请先确保人员安全，并记录当前资源、环境、人员状态和时间。",
        "",
        "请补充更具体的情况，例如人员状态、物资数量、地点和发生时间。",
        "",
        f"建议查看指南：{'、'.join([g.get('title', '') for g in related_guides])}" if related_guides else "",
    ])

    return sanitize_ai_answer(answer, mode=mode)
