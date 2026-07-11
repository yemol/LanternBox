"""AI Source Planner.

只负责让 AI 根据用户问题生成检索计划。
"""

import json
import re
from pathlib import Path
from typing import Any, Dict

from .policy import policy_set, policy_string
from .schemas import RetrievalPlan
from ..config import OLLAMA_MODEL, load_runtime_settings
from ..llm.client import call_ollama


ROOT = Path(__file__).resolve().parents[2]
EMERGENCY_GUIDES_FILE = ROOT / "data" / "emergency_guides.json"

FALLBACK_STOP_TERMS = policy_set("term_filter", "query_stop_terms")
FALLBACK_STOP_SUBSTRINGS = policy_set("term_filter", "weak_user_term_substrings")
FALLBACK_WEAK_CHARS = set(policy_string(("term_filter", "weak_user_term_chars"), ""))


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
- kiwix：大型离线百科和 ZIM 资料，适合扩展知识、百科查阅、术语解释、医学/技术背景补充。
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
6. Kiwix 已接入，但不要滥用。只有在用户明确询问定义、百科、背景、解释、医学/技术细节，或 Guide/Wiki 可能不足时规划 kiwix。
7. Kiwix 是背景资料层，不是应急行动卡的替代品。应急场景中必须仍然优先规划 guide；需要解释判断边界时规划 wiki；需要百科背景时才补充 kiwix。

核心词要求：
1. core_terms 是用户问题中的核心对象、核心风险、核心动作词。
2. core_terms 用于后续候选排序，因此必须精确。
3. core_terms 必须是中文具体词，不要输出英文词，除非用户原文就是英文。
4. core_terms 不要超过 8 个。
5. core_terms 必须使用独立关键词数组，每个元素只能是一个独立词或一个不可拆的固定名词。
6. 不要输出抽象词，例如：方案、方法、措施、管理、评估、标准、判断、风险、原因、处理、步骤、知识、安全、检查。
7. 不要输出“安全检查”“风险判断”“应急响应”这类泛词。
8. 应优先输出资料标题、标签、正文中可能直接出现的词。
9. 如果用户问题包含地点、对象、状态、动作，应尽量拆开保留。
10. 如果一个词包含多个含义，应拆成多个独立词。
11. core_terms 的每个元素不得包含空格，例如不要输出“饼干 分配”“米 分配”，应拆为“饼干”“米”“分配”。
12. core_terms 不要输出“使用”“安全”“检查”“判断”这类过泛抽象词，除非和具体对象组合后仍是资料库常见固定词；优先保留具体对象和直接风险词。
13. 对“能不能”“可不可以”“还能用吗”类问题，必须保留具体对象和风险词，例如“插线板”“进水”“漏电”“禁止通电”，不要只输出“使用”“安全”。

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
11. keywords 数组中的每个元素不得包含空格；如果想到“对象 + 动作”，拆成两个元素，例如“饼干”“分配”。
12. 如果用户问“还能用吗/能不能用/可不可以”，keywords 应同时包含对象、异常状态、禁止动作或风险词，例如“延长线”“雨水”“漏电”“禁止通电”。

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
    keywords = _fallback_terms_from_message(user_message)
    query = " ".join(keywords[:12]) or user_message.strip()

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


def _load_fallback_guides() -> list[dict[str, Any]]:
    try:
        if not EMERGENCY_GUIDES_FILE.exists():
            return []
        data = json.loads(EMERGENCY_GUIDES_FILE.read_text(encoding="utf-8"))
        return data if isinstance(data, list) else []
    except Exception:
        return []


def _fallback_candidate_terms(value: Any) -> list[str]:
    terms: list[str] = []

    values = value if isinstance(value, list) else [value]
    for item in values:
        for term in _split_terms(item):
            if len(term) < 2:
                continue
            if len(term) > 14:
                continue
            if _is_fallback_stop_term(term, include_substrings=True):
                continue
            if term not in terms:
                terms.append(term)

    return terms


def _is_fallback_stop_term(term: str, *, include_substrings: bool = False) -> bool:
    term = str(term or "").strip()
    if not term:
        return True
    if term in FALLBACK_STOP_TERMS:
        return True
    if include_substrings and any(stop in term for stop in FALLBACK_STOP_SUBSTRINGS):
        return True
    return False


def _fallback_ngram_score(term: str) -> int:
    term = str(term or "").strip()
    if _is_fallback_stop_term(term, include_substrings=True):
        return -100

    score = min(len(term), 4)
    weak_count = sum(1 for char in term if char in FALLBACK_WEAK_CHARS)
    score -= weak_count

    if term and term[0] in FALLBACK_WEAK_CHARS:
        score -= 3
    if term and term[-1] in FALLBACK_WEAK_CHARS:
        score -= 3
    if weak_count == 0:
        score += 4
    if len(term) >= 3:
        score += 1

    return score


def _fallback_ngrams(text: str, limit: int = 12) -> list[str]:
    chunks = [
        item
        for item in re.split(r"[\s，。！？、,.!?；;：:\-_/]+", text.strip())
        if item
    ]
    terms: list[str] = []

    for chunk in chunks:
        if 2 <= len(chunk) <= 8 and not _is_fallback_stop_term(chunk, include_substrings=True):
            terms.append(chunk)

        for size in (2, 3, 4):
            for index in range(0, max(len(chunk) - size + 1, 0)):
                term = chunk[index:index + size]
                if _is_fallback_stop_term(term, include_substrings=True):
                    continue
                if re.search(r"[\u4e00-\u9fff]", term) and term not in terms:
                    terms.append(term)

    ranked = [
        (score, index, term)
        for index, term in enumerate(terms)
        if (score := _fallback_ngram_score(term)) > 0
    ]
    ranked.sort(key=lambda item: (-item[0], item[1]))

    return [term for _, _, term in ranked[:limit]]


def _char_ngrams(text: str, sizes: tuple[int, ...] = (2, 3, 4)) -> set[str]:
    clean_text = re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", str(text or ""))
    grams: set[str] = set()

    for size in sizes:
        if len(clean_text) < size:
            continue
        for index in range(0, len(clean_text) - size + 1):
            gram = clean_text[index:index + size]
            if re.search(r"[\u4e00-\u9fff]", gram):
                grams.add(gram)

    return grams


def _fallback_term_score(term: str, message: str, message_grams: set[str]) -> int:
    if _is_fallback_stop_term(term, include_substrings=True):
        return 0

    score = 0
    if term in message:
        score += 40 + min(len(term), 12)

    term_grams = _char_ngrams(term)
    overlap = term_grams & message_grams
    if overlap:
        score += len(overlap) * 4
        score += min(len(term), 12)

    return score


def _guide_fallback_score_fields(guide: dict[str, Any]) -> list[Any]:
    return [
        guide.get("title"),
        guide.get("category"),
        guide.get("scenario"),
        guide.get("goal"),
        guide.get("keywords"),
        guide.get("top1_aliases"),
        guide.get("objects"),
        guide.get("signals"),
        guide.get("domains"),
    ]


def _guide_fallback_term_fields(guide: dict[str, Any]) -> list[Any]:
    return [
        guide.get("title"),
        guide.get("category"),
        guide.get("keywords"),
        guide.get("top1_aliases"),
        guide.get("objects"),
        guide.get("signals"),
    ]


def _score_fallback_guide(
    guide: dict[str, Any],
    message: str,
    message_grams: set[str],
) -> tuple[int, list[str]]:
    terms: list[str] = []
    score_terms: list[str] = []
    scored_output_terms: list[tuple[int, str]] = []

    for field in _guide_fallback_score_fields(guide):
        for term in _fallback_candidate_terms(field):
            if term not in score_terms:
                score_terms.append(term)

    for field in _guide_fallback_term_fields(guide):
        for term in _fallback_candidate_terms(field):
            if term not in terms:
                terms.append(term)

    scored_terms: list[tuple[int, str]] = []
    for term in score_terms:
        score = _fallback_term_score(term, message, message_grams)
        if score:
            scored_terms.append((score, term))

    for term in terms:
        score = _fallback_term_score(term, message, message_grams)
        if score >= 30:
            scored_output_terms.append((score, term))

    scored_terms.sort(key=lambda item: (-item[0], item[1]))
    scored_output_terms.sort(key=lambda item: (-item[0], item[1]))
    guide_score = sum(score for score, _ in scored_terms[:8])

    title = str(guide.get("title") or "")
    category = str(guide.get("category") or "")
    guide_score += _fallback_term_score(title, message, message_grams) * 2
    guide_score += _fallback_term_score(category, message, message_grams)

    return guide_score, [term for _, term in scored_output_terms[:8]]


def _fallback_terms_from_message(user_message: str, limit: int = 12) -> list[str]:
    message = str(user_message or "").strip()
    if not message:
        return ["本地资料"]

    message_grams = _char_ngrams(message)
    scored_guides: list[tuple[int, list[str]]] = []

    for guide in _load_fallback_guides():
        score, guide_terms = _score_fallback_guide(guide, message, message_grams)
        if score:
            scored_guides.append((score, guide_terms))

    message_terms = _fallback_ngrams(message)
    terms: list[str] = message_terms[:4]
    for _, guide_terms in sorted(scored_guides, key=lambda item: -item[0])[:5]:
        for term in guide_terms:
            if term not in terms:
                terms.append(term)
            if len(terms) >= limit:
                break
        if len(terms) >= limit:
            break

    for term in message_terms:
        if term not in terms:
            terms.append(term)

    return terms[:limit] or [message[:20]]


def _has_source_type(source_plan: list[dict[str, Any]], source_type: str) -> bool:
    return any(item.get("source_type") == source_type for item in source_plan)


def _merge_missing_fallback_plan(data: Dict[str, Any], fallback: Dict[str, Any]) -> Dict[str, Any]:
    if not data.get("core_terms"):
        data["core_terms"] = list(fallback.get("core_terms") or [])

    source_plan = data.get("source_plan") if isinstance(data.get("source_plan"), list) else []
    fallback_source_plan = fallback.get("source_plan") or []

    if not source_plan:
        data["source_plan"] = fallback_source_plan
        return data

    for item in source_plan:
        if not isinstance(item, dict):
            continue
        if not item.get("query"):
            item["query"] = fallback_source_plan[0].get("query", "") if fallback_source_plan else ""
        if not item.get("keywords"):
            item["keywords"] = list(data.get("core_terms") or [])

    for source_type in ("guide", "wiki"):
        if not _has_source_type(source_plan, source_type):
            for fallback_item in fallback_source_plan:
                if fallback_item.get("source_type") == source_type:
                    source_plan.append(fallback_item)
                    break

    data["source_plan"] = source_plan
    return data


def _split_terms(value: Any) -> list[str]:
    parts = re.split(r"[\s，。！？、,.!?；;：:\-_/]+", str(value or "").strip())
    return [part.strip() for part in parts if part.strip()]


def _normalize_terms(value: Any, limit: int | None = None) -> list[str]:
    values = value if isinstance(value, list) else [value]
    result: list[str] = []

    for item in values:
        for term in _split_terms(item):
            if term not in result:
                result.append(term)

    return result[:limit] if limit else result


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
                "categories": _normalize_terms(categories),
                "keywords": _normalize_terms(keywords),
                "limit": int(item.get("limit") or 8),
            }
        )

    data["source_plan"] = normalized_source_plan
    data["core_terms"] = _normalize_terms(data.get("core_terms") or [], limit=8)
    data["needs"] = [str(x).strip() for x in data.get("needs") or [] if str(x).strip()]

    return data


def build_retrieval_plan(user_message: str) -> RetrievalPlan:
    settings = load_runtime_settings()
    model = settings.get("retrieval_v2_model") or OLLAMA_MODEL

    messages = [
        {"role": "system", "content": PLANNER_SYSTEM_PROMPT},
        {"role": "user", "content": user_message},
    ]

    try:
        content = call_ollama(
            messages=messages,
            model=model,
            force_json=True,
            temperature=0.1,
            num_predict=1200,
            timeout=(5, 120),
        )
    except Exception as exc:
        content = ""
        data = _fallback_plan_payload(
            user_message=user_message,
            raw_output="",
            error=str(exc),
        )
    else:
        try:
            data = _extract_json(content)
        except Exception as exc:
            data = _fallback_plan_payload(
                user_message=user_message,
                raw_output=content,
                error=str(exc),
            )

    fallback_reason = ""

    existing_raw = data.get("raw") if isinstance(data.get("raw"), dict) else {}
    data = _normalize_plan_payload(data)

    if not data.get("core_terms") or not data.get("source_plan"):
        fallback_reason = "empty_planner_plan"
        fallback = _fallback_plan_payload(
            user_message=user_message,
            raw_output=content,
            error=fallback_reason,
        )
        data = _merge_missing_fallback_plan(data, fallback)

    data["raw"] = {
        **existing_raw,
        **({"fallback_reason": fallback_reason} if fallback_reason else {}),
        **{key: value for key, value in data.items() if key != "raw"},
    }

    return RetrievalPlan.model_validate(data)
