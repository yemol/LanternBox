import json
import requests
from typing import Any, Dict, List, Optional, Tuple

from .config import SCENARIO_PROFILE
from .resources import build_local_context
from .utils import get_default_model_for_mode
from .wiki import build_wiki_context_for_ai


def call_ollama(messages: List[Dict[str, str]], model: str = "qwen2.5:3b") -> str:
    try:
        response = requests.post(
            "http://127.0.0.1:11434/api/chat",
            json={
                "model": model,
                "messages": messages,
                "stream": False,
                "options": {
                    "temperature": 0.2,
                    "num_predict": 700,
                },
            },
            timeout=120,
        )
        response.raise_for_status()
        data = response.json()
        return data.get("message", {}).get("content", "")
    except Exception as e:
        raise RuntimeError(f"Ollama 调用失败：{e}")


def stream_ollama(messages: List[Dict[str, str]], model: str = "qwen2.5:3b"):
    try:
        response = requests.post(
            "http://127.0.0.1:11434/api/chat",
            json={
                "model": model,
                "messages": messages,
                "stream": True,
                "options": {
                    "temperature": 0.2,
                    "num_predict": 700,
                },
            },
            timeout=120,
            stream=True,
        )

        response.raise_for_status()

        for line in response.iter_lines():
            if not line:
                continue

            try:
                data = json.loads(line.decode("utf-8"))
                content = data.get("message", {}).get("content", "")
                if content:
                    yield content
                if data.get("done"):
                    break
            except Exception:
                continue
    except Exception as e:
        yield f"\n\n[本地 AI 流式输出失败：{e}]"


def build_safe_history(
    history: Optional[List[Any]] = None,
    max_messages: int = 6,
    max_chars_per_message: int = 600,
) -> List[Dict[str, str]]:
    safe_history = []

    for item in (history or [])[-max_messages:]:
        if isinstance(item, dict):
            role = item.get("role")
            content = item.get("content")
        else:
            role = getattr(item, "role", None)
            content = getattr(item, "content", None)

        if role not in ["user", "assistant"]:
            continue

        if not content or not str(content).strip():
            continue

        safe_history.append({
            "role": role,
            "content": str(content).strip()[:max_chars_per_message],
        })

    return safe_history



# -----------------------------------------------------------------------------
# AI 来源验收器 v0.5.1
# 目标：召回可以宽，但展示和进入 AI 上下文的来源必须通过验收。
# 重点：不依赖大模型，不让“外面”的“面”把夜间安全问题误判成食物问题。
# -----------------------------------------------------------------------------

REFERENCE_MAX_GUIDES = 4
REFERENCE_MAX_WIKIS = 4

REFERENCE_DOMAIN_KEYWORDS: Dict[str, List[str]] = {
    "security": [
        "安全", "风险", "危险", "威胁", "异响", "响动", "声音", "脚步", "脚步声",
        "外面", "门外", "窗外", "门口", "院外", "夜间", "晚上", "黑夜", "巡查",
        "观察", "躲避", "庇护", "避难", "入侵", "可疑", "担心", "害怕", "惊动",
        "暴露", "门窗", "守夜", "警戒", "低暴露", "不要外出",
    ],
    "food": [
        "食物", "主食", "粮食", "罐头", "剩饭", "剩食", "配给", "口粮", "营养",
        "冰箱", "做饭", "烹饪", "热食", "冷食", "蛋白", "库存天数",
    ],
    "water": [
        "缺水", "饮用水", "水源", "净水", "储水", "水质", "停水", "取水",
        "过滤", "消毒水", "脱水", "补水", "桶装水", "用水",
    ],
    "medical": [
        "发烧", "发热", "咳嗽", "腹泻", "呕吐", "伤口", "出血", "烫伤", "烧伤",
        "冻伤", "疼痛", "头晕", "昏迷", "感染", "隔离", "药", "医疗", "照护",
        "病人", "儿童", "老人", "孕妇",
    ],
    "power": [
        "停电", "电量", "低电量", "移动电源", "电池", "充电", "太阳能", "照明",
        "手电", "灯", "发电", "断电",
    ],
    "hygiene": [
        "卫生", "清洁", "洗手", "厕所", "粪便", "垃圾", "消毒", "污水", "异味",
        "霉菌", "污染",
    ],
    "tools": [
        "工具", "维修", "修理", "扳手", "螺丝刀", "胶带", "绳索", "固定", "加固",
        "破损", "替代制作", "基础工具箱",
    ],
    "comms": [
        "通信", "通讯", "电台", "无线电", "短波", "对讲机", "lora", "信号",
        "地图", "导航", "定位", "离线地图",
    ],
    "shelter": [
        "居住", "帐篷", "床铺", "保暖", "降温", "通风", "漏水",
        "房间", "庇护点",
    ],
    "records": [
        "记录", "日志", "备份", "档案", "登记", "清单", "巡检", "复查", "表格",
    ],
}

CATEGORY_DOMAIN_HINTS: Dict[str, str] = {
    "安全": "security",
    "风险": "security",
    "医疗": "medical",
    "照护": "medical",
    "水": "water",
    "食物": "food",
    "营养": "food",
    "卫生": "hygiene",
    "清洁": "hygiene",
    "电力": "power",
    "设备": "power",
    "工具": "tools",
    "维修": "tools",
    "通讯": "comms",
    "通信": "comms",
    "地图": "comms",
    "居住": "shelter",
    "避难": "shelter",
    "记录": "records",
    "数据": "records",
}

DOMAIN_COMPATIBILITY: Dict[str, set] = {
    "security": {"security", "shelter", "tools", "comms", "power"},
    "medical": {"medical", "water", "hygiene", "food"},
    "water": {"water", "hygiene", "medical", "food"},
    "food": {"food", "water", "hygiene", "medical"},
    "power": {"power", "tools", "comms", "shelter"},
    "hygiene": {"hygiene", "water", "medical", "shelter"},
    "tools": {"tools", "power", "shelter", "security", "comms"},
    "comms": {"comms", "power", "security", "records"},
    "shelter": {"shelter", "security", "power", "hygiene", "tools"},
    "records": {"records", "comms"},
}


def normalize_reference_text(value: Any) -> str:
    if isinstance(value, list):
        return " ".join(normalize_reference_text(item) for item in value)
    if isinstance(value, dict):
        return " ".join(normalize_reference_text(v) for v in value.values())
    return str(value or "").lower().replace("\n", " ").strip()


def re_split_reference(text: str) -> List[str]:
    import re
    return re.split(r"[\s,，、。；;：:！!？?（）()【】\[\]「」\"'“”‘’/\\]+", text)


def detect_domains_from_text(text: str, *, for_query: bool = False) -> List[str]:
    normalized = normalize_reference_text(text)
    hits: Dict[str, int] = {}

    for domain, keywords in REFERENCE_DOMAIN_KEYWORDS.items():
        score = 0
        for word in keywords:
            keyword = str(word).lower().strip()
            if not keyword:
                continue

            # 用户问题领域识别不吃单字，避免“外面”的“面”之类误判。
            if for_query and len(keyword) <= 1:
                continue

            if keyword in normalized:
                score += 1

        if score:
            hits[domain] = score

    return [
        domain
        for domain, _score in sorted(hits.items(), key=lambda item: item[1], reverse=True)
    ]


def detect_reference_domains(user_message: str, detected_domains: Optional[List[str]] = None) -> List[str]:
    """用户原话优先，上游 detected_domains 只做兜底。"""
    explicit_domains = detect_domains_from_text(user_message, for_query=True)

    if explicit_domains:
        return explicit_domains

    domains: List[str] = []
    for domain in detected_domains or []:
        domain = str(domain or "").lower().strip()
        if domain in REFERENCE_DOMAIN_KEYWORDS and domain not in domains:
            domains.append(domain)

    return domains


def reference_item_text(item: Dict[str, Any]) -> str:
    fields = [
        "id", "title", "category", "category_original", "summary", "scenario", "goal",
        "tags", "source", "notes", "fallback", "content",
        "tools", "steps", "check", "common_mistakes", "stop_or_escalate",
    ]
    return " ".join(normalize_reference_text(item.get(field)) for field in fields)


def infer_item_domains(item: Dict[str, Any]) -> List[str]:
    category = normalize_reference_text([item.get("category"), item.get("category_original")])
    full_text = reference_item_text(item)

    domains: List[str] = []

    for hint, domain in CATEGORY_DOMAIN_HINTS.items():
        if hint in category and domain not in domains:
            domains.append(domain)

    for domain in detect_domains_from_text(full_text, for_query=False):
        if domain not in domains:
            domains.append(domain)

    return domains


def tokenize_reference_query(user_message: str) -> List[str]:
    text = normalize_reference_text(user_message)
    tokens: List[str] = []

    for token in re_split_reference(text):
        token = token.strip()
        if len(token) >= 2:
            tokens.append(token)

    for words in REFERENCE_DOMAIN_KEYWORDS.values():
        for word in words:
            keyword = str(word).lower().strip()
            if len(keyword) >= 2 and keyword in text:
                tokens.append(keyword)

    seen = set()
    unique_tokens = []
    for token in tokens:
        if token not in seen:
            unique_tokens.append(token)
            seen.add(token)

    return unique_tokens[:24]


def lexical_reference_score(user_message: str, item: Dict[str, Any]) -> Tuple[int, List[str]]:
    tokens = tokenize_reference_query(user_message)

    title = normalize_reference_text(item.get("title"))
    category = normalize_reference_text([item.get("category"), item.get("category_original")])
    tags = normalize_reference_text(item.get("tags"))
    summary = normalize_reference_text([item.get("summary"), item.get("scenario"), item.get("goal")])
    full = reference_item_text(item)

    score = 0
    matched_tokens: List[str] = []

    for token in tokens:
        token_score = 0
        if token in title:
            token_score += 10
        elif token in tags:
            token_score += 7
        elif token in category:
            token_score += 5
        elif token in summary:
            token_score += 4
        elif token in full:
            token_score += 1

        if token_score:
            score += token_score
            if token not in matched_tokens:
                matched_tokens.append(token)

    return score, matched_tokens[:6]


def domains_compatible(query_domains: List[str], item_domains: List[str]) -> bool:
    if not query_domains:
        return True

    if not item_domains:
        return False

    for query_domain in query_domains:
        allowed = DOMAIN_COMPATIBILITY.get(query_domain, {query_domain})
        if any(item_domain in allowed for item_domain in item_domains):
            return True

    return False


def validate_reference_for_query(
    user_message: str,
    item: Dict[str, Any],
    item_type: str = "guide",
    detected_domains: Optional[List[str]] = None,
) -> Dict[str, Any]:
    query_domains = detect_reference_domains(user_message, detected_domains)
    item_domains = infer_item_domains(item)
    lexical_score, matched_tokens = lexical_reference_score(user_message, item)
    compatible = domains_compatible(query_domains, item_domains)

    # 硬门禁：用户问题已经有明确领域，而候选资料领域不兼容时，
    # 必须有非常强的词面命中才放行。否则一律过滤。
    if query_domains and not compatible and lexical_score < 10:
        return {
            "accepted": False,
            "score": -100,
            "reason": f"领域不匹配：问题属于 {','.join(query_domains)}，资料属于 {','.join(item_domains) or '未知'}。",
            "query_domains": query_domains,
            "item_domains": item_domains,
            "matched_tokens": matched_tokens,
        }

    score = lexical_score

    if compatible:
        score += 6
    else:
        score -= 8

    risk_level = str(item.get("risk_level") or "").lower()
    if risk_level == "high":
        score += 2
    elif risk_level == "caution":
        score += 1

    threshold = 4 if query_domains else 3
    accepted = score >= threshold

    if matched_tokens:
        reason = f"命中关键词：{'、'.join(matched_tokens[:4])}"
    elif compatible and item_domains:
        reason = f"结构化召回：同属相关领域：{'、'.join(item_domains[:2])}"
    else:
        reason = "相关度不足，已过滤"

    return {
        "accepted": accepted,
        "score": score,
        "reason": reason,
        "query_domains": query_domains,
        "item_domains": item_domains,
        "matched_tokens": matched_tokens,
    }


def filter_and_rank_ai_references(
    user_message: str,
    related_guides: Optional[List[Dict[str, Any]]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    detected_domains: Optional[List[str]] = None,
    max_guides: int = REFERENCE_MAX_GUIDES,
    max_wikis: int = REFERENCE_MAX_WIKIS,
    min_score: Optional[int] = None,
) -> Dict[str, List[Dict[str, Any]]]:
    """过滤并收敛 AI 来源。

    v0.5.1 调整重点：
    1. resources.py 已经完成结构化召回和排序，因此这里不再大幅重排指南。
    2. 明确领域问题只保留领域兼容来源。
    3. 对同领域但弱相关的来源，优先保留上游排序靠前的少量条目。
    4. security 场景默认只展示 3 条指南，避免“泛安全资料”挤占页面。
    """

    query_domains = detect_reference_domains(user_message, detected_domains)

    # 明确安全场景下，来源越少越稳。宁可 3 条精准，不要 6 条泛化。
    guide_limit = max_guides
    if "security" in query_domains:
        guide_limit = min(max_guides, 3)

    def process(
        items: Optional[List[Dict[str, Any]]],
        item_type: str,
        limit: int,
        preserve_upstream_order: bool = True,
    ) -> List[Dict[str, Any]]:
        accepted_items = []

        for index, item in enumerate(items or []):
            if not isinstance(item, dict):
                continue

            result = validate_reference_for_query(
                user_message=user_message,
                item=item,
                item_type=item_type,
                detected_domains=detected_domains,
            )

            if not result["accepted"]:
                continue

            item_copy = dict(item)

            # 让前端能展示后端验收理由；同时保留原始顺序，便于后续排查。
            item_copy["_reference_score"] = result["score"]
            item_copy["_reference_reason"] = result["reason"]
            item_copy["_reference_domains"] = result["item_domains"]
            item_copy["_reference_matched_terms"] = result["matched_tokens"]
            item_copy["_reference_original_index"] = index
            accepted_items.append(item_copy)

        if preserve_upstream_order:
            # 结构化召回阶段已经按相关度排序，这里只做验收和截断。
            # 不再让“词面误加分”把泛化资料顶到前面。
            return accepted_items[:limit]

        accepted_items.sort(
            key=lambda item: (
                item.get("_reference_score", 0),
                -item.get("_reference_original_index", 0),
            ),
            reverse=True,
        )
        return accepted_items[:limit]

    return {
        "guides": process(related_guides, "guide", guide_limit, preserve_upstream_order=True),
        "wikis": process(related_wikis, "wiki", max_wikis, preserve_upstream_order=False),
    }

def build_companion_messages(
    user_message: str,
    safe_history: List[Dict[str, str]],
) -> List[Dict[str, str]]:
    system_prompt = """
你是“壳中灯 LanternBox”的陪伴模式 AI。

你的角色：
你像一个低压力的日常朋友，陪使用者聊天、整理情绪、复盘一天、缓解焦虑、帮助理清思路。

回答要求：
1. 语气自然、温和、像朋友，不要像指挥官。
2. 不要默认把用户的话转成任务、风险或应急流程。
3. 可以帮助用户把混乱的想法整理成几步。
4. 可以提醒休息、喝水、吃饭、睡觉，但不要说教。
5. 如果用户只是表达累、烦、卡住了，先接住情绪，再给一个很小的下一步。
6. 除非用户提到明显危险、医疗急症、火灾、暴力、自伤、严重资源危机，否则不要切换成应急模式。
7. 如果发现真实危险，温和提醒：这个部分可能需要切回应急模式处理。
8. 回答不要太长，适合语音播报。
9. 不要使用“一、当前判断 / 二、马上做什么”这种应急格式。
10. 可以参考对话历史，但以用户最后一句话为主。
"""

    user_prompt = f"""
/no_think

用户说：
{user_message}

当前模式：陪伴模式

请像日常朋友一样回应。先陪伴和整理，不要默认拉响警报。
"""

    return [
        {"role": "system", "content": system_prompt},
        *safe_history,
        {"role": "user", "content": user_prompt},
    ]


def build_emergency_messages(
    user_message: str,
    matched_triggers: List[Dict[str, Any]],
    related_guides: List[Dict[str, Any]],
    detected_domains: List[str],
    safe_history: List[Dict[str, str]],
    related_wikis: Optional[List[Dict[str, Any]]] = None,
) -> List[Dict[str, str]]:
    context = build_local_context(matched_triggers, related_guides)
    wiki_context = build_wiki_context_for_ai(related_wikis or [])

    system_prompt = f"""
你是“壳中灯 LanternBox”的应急模式本地离线 AI 助手。

系统默认使用场景：
1. 使用者可能处在断网、断电、断水、断供、灾害、野外、避难、长期居住或低资源环境中。
2. 不要默认存在稳定城市服务、供水公司、物业、医院、快递、外卖、互联网、云端服务或即时救援。
3. 除非用户明确说明自己处在正常城市生活场景，否则优先按“本地资源有限、需要自救和组织管理”的场景回答。
4. 你的核心任务不是让使用者自己查资料，而是主动调用本地资料，判断风险，提出可执行建议，并提醒后续观察和记录。

回答原则：
1. 优先基于本地资料回答，包括触发规则、应急指南、库存、成员档案、地图、日志和百科。
2. 当前阶段主要可用资料是 ai_triggers.json、emergency_guides.json 和本地精选 Wiki。
3. 不要编造本地资料中没有的事实。
4. 如果本地资料不足，可以给出通用安全建议，但必须说明这是基于一般原则。
5. 回答要适合语音播报，句子不要太长。
6. 回答应控制在 600 字以内，优先给行动步骤。
7. 不要在回答正文最后重复列出本地触发场景和指南标题，页面会单独展示这些资料。
8. 可以参考对话历史，但当前用户最后一句话优先级最高。
9. 如果对话历史和当前风险冲突，以当前风险为准。

场景边界：
1. 不要把问题自动理解成普通家庭维修或城市客服问题。
2. 不要优先建议“联系供水公司、物业、客服、快递、上网搜索”等依赖外部稳定系统的方案。
3. 可以在最后补充：“如果确认外部服务仍可用，再联系相关部门或专业人员。”
4. 遇到医疗、火灾、燃气、电池、洪水、地震、结构损坏等高风险问题时，可以提醒停止危险操作并升级处理。
5. 如果需要外部专业救援，必须先给出当前立刻能做的本地安全措施，再说明外部救援作为补充选项。

低资源场景特别规则：
1. 当用户提到水不够、食物不够、燃料不够、电量不足、药品不足时，必须优先考虑剩余量估算、人数、特殊成员、配给优先级、暂停非必要消耗、备用来源、记录和复查。
2. 缺水时，不要建议完全停止洗手。正确做法是保留关键洗手，例如如厕后、做饭前、取饮用水前、处理伤口前后。
3. 缺水时，应暂停洗衣、拖地、洗车、浇灌植物、长时间冲洗等非必要耗水。
4. 缺水时，饮用、用药、病人照护、儿童老人照护和最低限度烹饪优先于一般清洁。

回答格式：
请按下面结构回答：

一、当前判断
用 1 到 3 句话判断风险等级和问题性质。

二、马上做什么
列出最先执行的 3 到 6 个动作。

三、不要做什么
列出容易造成风险的错误做法。

四、接下来观察什么
列出需要记录和复查的指标。

五、需要追问的信息
如果信息不足，最多追问 1 到 3 个关键问题。

不要在回答正文里重复列出“参考的本地资料”。
页面下方会单独显示触发场景和关联指南。
"""

    user_prompt = f"""
用户描述：
{user_message}

当前模式：应急模式

系统场景设定：
{SCENARIO_PROFILE}

检测到的问题领域：
{', '.join(detected_domains) if detected_domains else '未识别到明确领域'}

匹配到的本地触发场景：
{context['trigger_text'] or '未命中精确触发规则。'}

关联的本地指南：
{context['guide_text'] or '未找到明确关联指南。'}

关联的本地精选 Wiki：
{wiki_context or '未找到明确关联 Wiki。'}

资料说明：
如果没有命中精确触发规则，但存在关联的本地指南，请仍然优先使用这些本地指南回答。
不要因为触发场景为空就转向普通城市客服式回答。
不要优先建议供水公司、物业、客服、互联网搜索等依赖外部稳定系统的方案。

请基于以上本地资料，给出壳中灯 AI 的建议。
不要在正文末尾重复列出参考资料，触发场景和关联指南会由页面单独显示。
"""

    return [
        {"role": "system", "content": system_prompt},
        *safe_history,
        {"role": "user", "content": user_prompt},
    ]


def build_ai_messages(
    user_message: str,
    mode: str,
    matched_triggers: List[Dict[str, Any]],
    related_guides: List[Dict[str, Any]],
    detected_domains: List[str],
    history: Optional[List[Any]] = None,
    related_wikis: Optional[List[Dict[str, Any]]] = None,
) -> List[Dict[str, str]]:
    safe_history = build_safe_history(history)

    if mode == "companion":
        return build_companion_messages(
            user_message=user_message,
            safe_history=safe_history,
        )

    return build_emergency_messages(
        user_message=user_message,
        matched_triggers=matched_triggers,
        related_guides=related_guides,
        detected_domains=detected_domains,
        safe_history=safe_history,
        related_wikis=related_wikis,
    )


def build_fallback_answer(
    mode: str,
    matched_triggers: List[Dict[str, Any]],
    related_guides: List[Dict[str, Any]],
) -> str:
    if mode == "companion":
        return "本地 AI 模型暂时没有响应。先别急，我们可以从最小的一步开始：把你现在最在意的一件事写下来，然后只处理它。"

    fallback_actions = []
    for trigger in matched_triggers:
        fallback_actions.extend(trigger.get("suggested_actions", []))

    return "\n".join([
        "本地 AI 模型暂时没有响应。以下是根据本地触发规则生成的基础建议：",
        "",
        f"检测到：{'、'.join([t.get('title', '') + '（' + t.get('severity', '') + '）' for t in matched_triggers])}" if matched_triggers else "暂未匹配到明确触发场景。",
        "",
        "\n".join([f"{i + 1}. {item}" for i, item in enumerate(fallback_actions[:8])]) or "请补充更具体的情况，例如人员状态、物资数量、地点和发生时间。",
        "",
        f"建议查看指南：{'、'.join([g.get('title', '') for g in related_guides])}" if related_guides else "",
    ])
