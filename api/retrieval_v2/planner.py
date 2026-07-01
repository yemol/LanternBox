"""AI Source Planner.

只负责让 AI 根据用户问题生成检索计划。
"""

import json
import re
from typing import Any, Dict

from .schemas import RetrievalPlan
from ..config import OLLAMA_MODEL, load_runtime_settings
from ..llm.client import call_ollama


PLANNER_SYSTEM_PROMPT = """
你是 LanternBox 的离线资料检索规划器。

你的任务：
根据用户问题，判断当前场景，并决定应该从哪些资料源检索信息。

重要原则：
1. 所有理解、归纳、场景判断、核心对象提取都由你完成。
2. 代码只会根据你的计划去取候选资料，不会替你理解用户意图。
3. 不要编造资料 ID。
4. 只输出严格合法 JSON，不要输出解释文字、Markdown、注释或多余内容。
5. 所有字符串必须使用双引号。

当前可用资料源：
- guide：本地应急指南，适合操作步骤、应急流程、优先级和立即行动。
- wiki：本地精选 Wiki，适合背景知识、判断标准、风险解释、物品用途、维护方法和概念说明。
- kiwix：大型离线百科，当前暂未接入，但可以在计划中预留。
- log：用户日志，当前暂未接入。
- inventory：物资库存，当前暂未接入。
- sensor：传感器数据，当前暂未接入。
- map：离线地图，当前暂未接入。

资料源规划要求：
1. 对于食物、水、医疗、电力、安全、灾害、居住、卫生等实际生存问题，默认至少规划 guide 和 wiki 两类资料源。
2. guide 用于查找可执行操作步骤、应急流程、优先级和立即行动。
3. wiki 用于查找背景知识、判断标准、风险解释、物品用途、维护方法和补充说明。
4. 除非用户问题明显只需要闲聊或不需要资料，否则不要只规划 guide。
5. 如果用户问题涉及多个风险，应为主要风险规划 guide，并为关键判断点规划 wiki。
6. 如果某个资料源当前暂未接入，可以只在确实必要时预留，不要滥用。

核心词要求：
1. core_terms 是用户问题中的核心对象、核心风险、核心动作词。
2. core_terms 用于后续候选排序，因此必须精确。
3. core_terms 必须是中文具体词，不要输出英文词，除非用户原文就是英文。
4. core_terms 不要超过 8 个。
5. core_terms 必须尽量使用独立短词，不要把多个概念粘成一个长词。
6. 不要输出抽象词，例如：方案、方法、措施、管理、评估、标准、判断、风险、原因、处理、步骤、知识、安全、检查。
7. 不要输出“安全检查”“风险判断”“应急响应”这类泛词。
8. 应优先输出资料标题、标签、正文中可能直接出现的词。
9. 如果用户问题包含地点、对象、状态、动作，应尽量拆开保留。
10. 如果一个词包含多个含义，应拆成多个独立词。

核心词补充规则：
1. core_terms 不要使用过泛的单字词，例如“水”“电”“火”“药”。应改为更具体的资料词，例如“饮用水”“缺水”“漏电”“火灾”“常用药”。
2. 如果用户表达某种资源“不够、快没了、只剩、紧张”，core_terms 必须包含对应的短缺状态词，例如“缺水”“缺粮”“库存不足”“配给”。
3. 如果用户表达环境压力，例如“很热、很冷、暴雨、水位上涨、停电”，core_terms 必须同时包含环境词和直接风险词，例如“高温”“脱水”“洪水”“断电”“漏电”。
4. query 必须包含核心对象 + 状态 + 行动方向，不要只写对象或只写泛动作。
5. 对资源短缺类问题，query 应包含“配给、优先级、用途、库存、消耗”等可检索词。

检索词要求：
1. source_plan.query 必须由具体中文检索词组成，用空格分隔。
2. source_plan.keywords 必须是独立词数组，不要把多个词塞进一个字符串。
3. keywords 不要输出带顿号、逗号或整句的字符串。
4. 不要输出口语化长短语。
5. 不要把抽象词作为主要检索词，例如：方案、方法、措施、管理、评估、标准、判断、风险、原因、处理、步骤、知识、安全、检查。
6. 如果抽象词确实有辅助价值，也不要单独放入 keywords，应替换为资料库中更可能出现的具体词。
7. 必须把用户口语转成资料库常见词。
8. query 和 keywords 必须使用中文检索词，不要输出英文词，除非用户原文就是英文。
9. query 里要包含核心对象词，不要只写处置类词。
10. 对空间、时间、异常状态类问题，要保留地点词、时间词、异常词和行动词。

检索词转换原则：
- 不要写：“资源管理”
  应写：“库存 配给 消耗 优先级”

- 不要写：“安全评估”
  应写：“异常 声响 观察 距离 暴露”

- 不要写：“设备处理”
  应写：“进水 漏电 短路 干燥 损坏”

- 不要写：“饮水安全判断”
  应写：“饮用水 储存 水质 异味 浑浊 煮沸”

- 不要写：“灾害应对措施”
  应写：“暴雨 洪水 水位 撤高 断电 转移”

- 不要写：“夜间异响”
  应写：“夜间 外部 异响 观察 暴露”

- 不要写：“异常声响”
  应写：“异常 声响 门外 观察”

- 不要写：“水 剩余 高温”
  应写：“饮用水 缺水 配水 用水 优先级 高温 补水 脱水”

- 不要写：“水 储存 高温”
  应写：“饮用水 储存 缺水 高温 补水 脱水 水质”

输出 JSON 格式：
{
  "scenario_summary": "一句话说明用户当前场景",
  "urgency": "low|medium|high|critical|unknown",
  "needs": ["需要关注的事项"],
  "core_terms": ["核心对象词", "核心风险词", "核心动作词"],
  "source_plan": [
    {
      "source_type": "guide",
      "purpose": "查找可执行操作指南",
      "query": "具体检索词 用空格分隔",
      "categories": [],
      "keywords": ["具体词1", "具体词2", "具体词3"],
      "limit": 8
    },
    {
      "source_type": "wiki",
      "purpose": "查找背景知识、判断标准和补充说明",
      "query": "具体检索词 用空格分隔",
      "categories": [],
      "keywords": ["具体词1", "具体词2", "具体词3"],
      "limit": 8
    }
  ]
}
"""


def _extract_json(text: str) -> Dict[str, Any]:
    text = (text or "").strip()

    if text.startswith("```"):
        text = text.strip("`")
        if text.lower().startswith("json"):
            text = text[4:].strip()

    start = text.find("{")
    end = text.rfind("}")

    if start >= 0 and end >= start:
        text = text[start:end + 1]

    return json.loads(text)


def _fallback_plan_payload(
    user_message: str,
    raw_output: str = "",
    error: str = "",
) -> Dict[str, Any]:
    terms = [
        item
        for item in re.split(r"[\s，。！？、,.!?；;：:\-_/]+", user_message.strip())
        if item
    ]
    query = " ".join(terms[:8]) or user_message.strip()
    keywords = terms[:8] or [user_message.strip()]

    return {
        "scenario_summary": user_message.strip()[:80] or "用户请求本地资料协助",
        "urgency": "unknown",
        "needs": ["检索本地应急指南和 Wiki 资料"],
        "core_terms": keywords,
        "source_plan": [
            {
                "source_type": "guide",
                "purpose": "查找可执行操作指南",
                "query": query,
                "categories": [],
                "keywords": keywords,
                "limit": 8,
            },
            {
                "source_type": "wiki",
                "purpose": "查找背景知识、判断标准和补充说明",
                "query": query,
                "categories": [],
                "keywords": keywords,
                "limit": 8,
            },
        ],
        "raw": {
            "fallback": True,
            "planner_error": error,
            "planner_raw_output": raw_output,
        },
    }


def _normalize_plan_payload(data: Dict[str, Any]) -> Dict[str, Any]:
    """Normalize planner JSON shape before Pydantic validation."""
    if not isinstance(data.get("needs"), list):
        data["needs"] = []

    if not isinstance(data.get("core_terms"), list):
        data["core_terms"] = []

    if not isinstance(data.get("source_plan"), list):
        data["source_plan"] = []

    normalized_source_plan = []
    for item in data.get("source_plan") or []:
        if not isinstance(item, dict):
            continue

        keywords = item.get("keywords") or []
        if isinstance(keywords, str):
            keywords = [keywords]
        if not isinstance(keywords, list):
            keywords = []

        categories = item.get("categories") or []
        if isinstance(categories, str):
            categories = [categories]
        if not isinstance(categories, list):
            categories = []

        normalized_source_plan.append(
            {
                "source_type": item.get("source_type") or "guide",
                "purpose": str(item.get("purpose") or ""),
                "query": str(item.get("query") or ""),
                "categories": [str(x) for x in categories],
                "keywords": [str(x) for x in keywords],
                "limit": int(item.get("limit") or 8),
            }
        )

    data["source_plan"] = normalized_source_plan
    data["core_terms"] = [str(x) for x in data.get("core_terms") or [] if str(x).strip()][:8]
    data["needs"] = [str(x) for x in data.get("needs") or [] if str(x).strip()]

    return data


def build_retrieval_plan(user_message: str) -> RetrievalPlan:
    settings = load_runtime_settings()
    model = settings.get("retrieval_v2_model") or OLLAMA_MODEL

    messages = [
        {"role": "system", "content": PLANNER_SYSTEM_PROMPT},
        {"role": "user", "content": user_message},
    ]

    content = call_ollama(
        messages=messages,
        model=model,
        force_json=True,
        temperature=0.1,
        num_predict=1200,
    )

    try:
        data = _extract_json(content)
    except Exception as exc:
        data = _fallback_plan_payload(
            user_message=user_message,
            raw_output=content,
            error=str(exc),
        )

    existing_raw = data.get("raw") if isinstance(data.get("raw"), dict) else {}
    data = _normalize_plan_payload(data)
    data["raw"] = {
        **existing_raw,
        **{key: value for key, value in data.items() if key != "raw"},
    }

    return RetrievalPlan.model_validate(data)
