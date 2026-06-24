import json
import requests
from typing import Any, Dict, List, Optional, Tuple

from .config import SCENARIO_PROFILE
from .resources import build_local_context
from .utils import get_default_model_for_mode
from .wiki import build_wiki_context_for_ai


def call_ollama(
    messages: List[Dict[str, str]],
    model: str = "qwen2.5:3b",
    *,
    force_json: bool = False,
    temperature: float = 0.2,
    num_predict: int = 700,
) -> str:
    try:
        payload = {
            "model": model,
            "messages": messages,
            "stream": False,
            "options": {
                "temperature": temperature,
                "num_predict": num_predict,
            },
        }

        # Ollama 支持 format=json，可以显著降低“模型返回了说明文字而非 JSON”的概率。
        # 只在重排这类结构化任务中开启，普通聊天不受影响。
        if force_json:
            payload["format"] = "json"

        response = requests.post(
            "http://127.0.0.1:11434/api/chat",
            json=payload,
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


# -----------------------------------------------------------------------------
# LanternBox AI v0.6.1 Hybrid RAG 重排层
# 目标：让本地 AI 参与候选来源过滤和重排，但由代码负责边界、校验和失败回退。
# 特点：默认仍可纯规则运行；开启后要求 Ollama 输出结构化 JSON；任何异常都回退规则排序。
# -----------------------------------------------------------------------------

AI_RERANK_VERSION = "v0.6.1-ai-reranker"
AI_RERANK_ENV_NAME = "LANTERNBOX_AI_RERANK"
AI_RERANK_MODEL_ENV_NAME = "LANTERNBOX_AI_RERANK_MODEL"
AI_RERANK_MAX_CANDIDATES = 30
AI_RERANK_MAX_SELECTED = 6
AI_RERANK_MAX_EXCLUDED = 10


def _env_truthy(value: Any) -> bool:
    return str(value or "").strip().lower() in {"1", "true", "yes", "on", "y", "启用", "开启"}


def should_enable_ai_rerank(enable_ai_rerank: Optional[bool] = None) -> bool:
    """解析 AI 重排开关。

    - 函数参数显式传 True/False 时优先；
    - 未传时读取环境变量 LANTERNBOX_AI_RERANK；
    - 默认关闭，保证低功耗和测试稳定。
    """
    if enable_ai_rerank is not None:
        return bool(enable_ai_rerank)
    try:
        import os
        return _env_truthy(os.getenv(AI_RERANK_ENV_NAME))
    except Exception:
        return False


def get_ai_rerank_model(default: str = "qwen2.5:3b") -> str:
    try:
        import os
        return os.getenv(AI_RERANK_MODEL_ENV_NAME, default) or default
    except Exception:
        return default


def _safe_json_loads_from_text(text: str) -> Optional[Dict[str, Any]]:
    """从模型输出中提取 JSON。失败返回 None，调用方必须回退规则排序。"""
    if not text:
        return None

    content = str(text).strip()

    # 兼容 ```json ... ``` 包裹。
    if content.startswith("```"):
        content = content.strip("`").strip()
        if content.lower().startswith("json"):
            content = content[4:].strip()

    candidates = [content]

    start = content.find("{")
    end = content.rfind("}")
    if start >= 0 and end > start:
        candidates.append(content[start:end + 1])

    for item in candidates:
        try:
            data = json.loads(item)
            if isinstance(data, dict):
                return data
        except Exception:
            continue
    return None


def _candidate_for_prompt(candidate: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "candidate_id": candidate.get("candidate_id"),
        "source_type": candidate.get("source_type"),
        "source_label": candidate.get("source_label") or candidate.get("source_type"),
        "title": candidate.get("title"),
        "summary": str(candidate.get("summary", ""))[:260],
        "score": candidate.get("score", 0),
        "reason": candidate.get("reason", ""),
        "matched_terms": candidate.get("matched_terms", [])[:8],
        "hard_excluded": bool(candidate.get("hard_excluded")),
        "excluded_reason": candidate.get("excluded_reason", ""),
    }


def build_ai_rerank_messages(
    user_message: str,
    candidates: List[Dict[str, Any]],
    query_profile: Optional[Dict[str, Any]] = None,
) -> List[Dict[str, str]]:
    """要求本地模型只在候选池里选择来源，并输出结构化 JSON。"""
    query_profile = query_profile or {}
    compact_candidates = [_candidate_for_prompt(item) for item in candidates[:AI_RERANK_MAX_CANDIDATES]]

    system_prompt = """
你是 LanternBox 本地离线知识重排器，不是最终回答助手。
你的任务是从候选来源中选择最适合当前问题的资料。

必须遵守：
1. 只能选择候选列表里存在的 candidate_id，不能编造来源。
2. hard_excluded=true 的来源绝对不能选入 selected_candidate_ids。
3. 用户明确说“先不管、暂时不考虑、不要处理”的主题，必须放入 excluded，并说明原因。
4. 应急指南 guide 通常优先于 wiki；wiki 优先于 kiwix；Kiwix 只作为背景资料。
5. 如果两个来源都相关，优先选择更贴近“当前优先事项”的来源，而不是只看大场景关键词。
6. 输出必须是一个合法 JSON object。
7. 不要输出解释性正文，不要使用 Markdown 代码块，不要在 JSON 前后添加任何文字。
8. JSON 中所有字符串必须使用英文双引号，不能使用单引号。
"""

    user_prompt = f"""
用户问题：
{user_message}

规则解析结果：
{json.dumps(query_profile, ensure_ascii=False)[:1800]}

候选来源：
{json.dumps(compact_candidates, ensure_ascii=False, indent=2)}

请输出 JSON，格式如下：
{{
  "intent_summary": "一句话概括用户当前真正要处理的事",
  "selected_candidate_ids": ["最多 6 个 candidate_id"],
  "selected": [
    {{"candidate_id": "被选中的 candidate_id", "reason": "为什么当前要用它"}}
  ],
  "excluded": [
    {{"candidate_id": "被排除的 candidate_id", "reason": "为什么当前不选"}}
  ],
  "answer_focus": ["回答应该聚焦的关键词"]
}}

注意：selected_candidate_ids 和 selected 二选一即可；如果都输出，代码会合并校验。
"""

    return [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt},
    ]


def _extract_selected_entries(ai_result: Dict[str, Any]) -> List[Dict[str, str]]:
    """兼容多种 JSON 字段名，降低本地小模型输出漂移影响。"""
    entries: List[Dict[str, str]] = []

    raw_ids = ai_result.get("selected_candidate_ids")
    if raw_ids is None:
        raw_ids = ai_result.get("selected_source_ids")
    if raw_ids is None:
        raw_ids = ai_result.get("selected_ids")

    for item in raw_ids or []:
        entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 选择"})

    raw_selected = ai_result.get("selected") or ai_result.get("selected_sources") or []
    for item in raw_selected:
        if isinstance(item, dict):
            candidate_id = str(item.get("candidate_id") or item.get("id") or item.get("source_id") or "").strip()
            reason = str(item.get("reason") or item.get("why") or "AI 选择")[:180]
            entries.append({"candidate_id": candidate_id, "reason": reason})
        else:
            entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 选择"})

    return entries


def _extract_excluded_entries(ai_result: Dict[str, Any]) -> List[Dict[str, str]]:
    entries: List[Dict[str, str]] = []
    raw_excluded = ai_result.get("excluded") or ai_result.get("excluded_sources") or ai_result.get("excluded_candidate_ids") or []
    for item in raw_excluded:
        if isinstance(item, dict):
            candidate_id = str(item.get("candidate_id") or item.get("id") or item.get("source_id") or "").strip()
            reason = str(item.get("reason") or item.get("why") or "AI 排除")[:180]
            entries.append({"candidate_id": candidate_id, "reason": reason})
        else:
            entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 排除"})
    return entries


def validate_ai_rerank_result(
    ai_result: Dict[str, Any],
    candidates: List[Dict[str, Any]],
    *,
    max_selected: int = AI_RERANK_MAX_SELECTED,
) -> Dict[str, Any]:
    by_id = {str(item.get("candidate_id")): item for item in candidates if item.get("candidate_id")}
    selected_ids: List[str] = []
    selected_reasons: Dict[str, str] = {}
    rejected_ids: List[str] = []

    for entry in _extract_selected_entries(ai_result):
        candidate_id = str(entry.get("candidate_id") or "").strip()
        item = by_id.get(candidate_id)
        if not candidate_id or not item:
            if candidate_id:
                rejected_ids.append(candidate_id)
            continue
        if item.get("hard_excluded"):
            rejected_ids.append(candidate_id)
            continue
        if candidate_id not in selected_ids:
            selected_ids.append(candidate_id)
            selected_reasons[candidate_id] = str(entry.get("reason") or "AI 选择")[:180]
        if len(selected_ids) >= max_selected:
            break

    selected: List[Dict[str, Any]] = []
    for candidate_id in selected_ids:
        item = dict(by_id[candidate_id])
        item["rerank_reason"] = selected_reasons.get(candidate_id, "AI 选择")
        item["rerank_mode"] = "ai"
        selected.append(item)

    # 硬排除候选永远进入 excluded_sources，AI 额外排除理由可覆盖/补充。
    excluded_by_id: Dict[str, Dict[str, Any]] = {}
    for candidate in candidates:
        if candidate.get("hard_excluded") and candidate.get("candidate_id"):
            cid = str(candidate.get("candidate_id"))
            excluded_by_id[cid] = {
                "candidate_id": cid,
                "source_type": candidate.get("source_type"),
                "title": candidate.get("title"),
                "reason": candidate.get("excluded_reason") or "硬排除",
            }

    for entry in _extract_excluded_entries(ai_result):
        candidate_id = str(entry.get("candidate_id") or "").strip()
        if candidate_id in by_id:
            excluded_by_id[candidate_id] = {
                "candidate_id": candidate_id,
                "source_type": by_id[candidate_id].get("source_type"),
                "title": by_id[candidate_id].get("title"),
                "reason": str(entry.get("reason") or by_id[candidate_id].get("excluded_reason") or "AI 排除")[:180],
            }

    # 如果模型没选出任何合法来源，调用方应回退规则排序。
    valid = bool(selected)

    return {
        "valid": valid,
        "version": AI_RERANK_VERSION,
        "mode": "ai_rerank_validated" if valid else "ai_rerank_failed",
        "intent_summary": str(ai_result.get("intent_summary") or "")[:240],
        "answer_focus": [str(x)[:50] for x in (ai_result.get("answer_focus") or [])[:8]],
        "selected_sources": selected,
        "excluded_sources": list(excluded_by_id.values())[:AI_RERANK_MAX_EXCLUDED],
        "rejected_candidate_ids": rejected_ids,
        "selected_candidate_ids": [item.get("candidate_id") for item in selected if item.get("candidate_id")],
        "fallback_reason": None,
        "used_ai": valid,
    }


def rule_rerank_candidates(candidates: List[Dict[str, Any]], *, max_selected: int = AI_RERANK_MAX_SELECTED) -> Dict[str, Any]:
    selected = [item for item in candidates if not item.get("hard_excluded")]
    excluded = [item for item in candidates if item.get("hard_excluded")]
    selected.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    excluded.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    return {
        "valid": True,
        "version": AI_RERANK_VERSION,
        "mode": "rule_fallback",
        "intent_summary": "规则回退排序",
        "answer_focus": [],
        "selected_sources": selected[:max_selected],
        "excluded_sources": [
            {
                "candidate_id": item.get("candidate_id"),
                "source_type": item.get("source_type"),
                "title": item.get("title"),
                "reason": item.get("excluded_reason", "硬排除"),
            }
            for item in excluded[:AI_RERANK_MAX_EXCLUDED]
        ],
        "rejected_candidate_ids": [],
        "selected_candidate_ids": [item.get("candidate_id") for item in selected[:max_selected] if item.get("candidate_id")],
        "fallback_reason": "规则排序回退",
        "used_ai": False,
    }


def rerank_candidates_with_local_ai(
    user_message: str,
    candidates: List[Dict[str, Any]],
    *,
    query_profile: Optional[Dict[str, Any]] = None,
    model: Optional[str] = None,
    enable_ai_rerank: Optional[bool] = None,
    max_selected: int = AI_RERANK_MAX_SELECTED,
) -> Dict[str, Any]:
    """AI 重排入口。

    默认读取环境变量 LANTERNBOX_AI_RERANK；未开启时使用纯规则回退。
    开启后：规则候选池 → 本地 AI JSON 重排 → 代码校验 → 失败回退规则排序。
    """
    if not candidates:
        result = rule_rerank_candidates([], max_selected=max_selected)
        result["mode"] = "rule_fallback_no_candidates"
        result["fallback_reason"] = "没有可供 AI 重排的候选来源，使用空规则结果。"
        return result

    if not should_enable_ai_rerank(enable_ai_rerank):
        result = rule_rerank_candidates(candidates, max_selected=max_selected)
        result["mode"] = "rule_only_ai_rerank_disabled"
        result["fallback_reason"] = "AI 检索增强未开启，使用规则排序。"
        return result

    model = model or get_ai_rerank_model()

    try:
        messages = build_ai_rerank_messages(user_message, candidates, query_profile=query_profile)
        raw = call_ollama(
            messages,
            model=model,
            force_json=True,
            temperature=0.0,
            num_predict=900,
        )
        parsed = _safe_json_loads_from_text(raw)
        if not parsed:
            # 少数本地模型即使 format=json 也可能吐出残缺 JSON。
            # 此处只做一次低成本修复请求：把原始输出整理成指定 JSON，不让它重新判断来源。
            repair_messages = [
                {
                    "role": "system",
                    "content": "你是 JSON 修复器。只输出一个合法 JSON object，不要解释，不要 Markdown。",
                },
                {
                    "role": "user",
                    "content": (
                        "把下面内容修复为合法 JSON object。字段尽量保留："
                        "intent_summary, selected_candidate_ids, selected, excluded, answer_focus。\n\n"
                        f"原始内容：\n{str(raw or '')[:1600]}"
                    ),
                },
            ]
            try:
                repaired_raw = call_ollama(
                    repair_messages,
                    model=model,
                    force_json=True,
                    temperature=0.0,
                    num_predict=700,
                )
                parsed = _safe_json_loads_from_text(repaired_raw)
                if parsed:
                    raw = repaired_raw
            except Exception:
                parsed = None

        if not parsed:
            fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
            fallback["mode"] = "rule_fallback_json_parse_failed"
            fallback["fallback_reason"] = "本地 AI 已返回内容，但不是可解析的 JSON，已回退到规则排序。"
            fallback["raw_ai_output"] = str(raw or "")[:500]
            return fallback

        validated = validate_ai_rerank_result(parsed, candidates, max_selected=max_selected)
        if not validated.get("valid"):
            fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
            fallback["mode"] = "rule_fallback_no_valid_ai_selection"
            rejected = validated.get("rejected_candidate_ids") or []
            fallback["fallback_reason"] = "本地 AI 没有选出合法来源，或选择了不存在/被硬排除的 candidate_id，已回退到规则排序。"
            if rejected:
                fallback["fallback_reason"] += f" 被拒绝的候选：{', '.join(map(str, rejected[:6]))}"
            fallback["ai_validation"] = validated
            return fallback
        return validated
    except Exception as exc:
        fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
        fallback["mode"] = "rule_fallback_ai_error"
        fallback["error"] = str(exc)[:300]
        fallback["fallback_reason"] = f"本地 AI 重排调用失败，已回退到规则排序：{str(exc)[:240]}"
        return fallback


def get_candidate_raw_item_from_ai(candidate: Dict[str, Any]) -> Dict[str, Any]:
    raw = candidate.get("raw")
    return raw if isinstance(raw, dict) else candidate


def apply_rerank_result_to_context(
    context: Dict[str, Any],
    rerank_result: Dict[str, Any],
) -> Dict[str, Any]:
    """把 AI/规则重排结果回写到 prepare_ai_context 的上下文结构中。

    这个函数供后端路由调用。resources.py 不直接调用 ai.py，避免循环依赖。
    """
    updated = dict(context or {})
    selected_sources = rerank_result.get("selected_sources", []) or []
    excluded_sources = rerank_result.get("excluded_sources", []) or []

    updated["selected_sources"] = selected_sources
    updated["excluded_sources"] = excluded_sources
    updated["related_guides"] = [
        get_candidate_raw_item_from_ai(item)
        for item in selected_sources
        if item.get("source_type") == "guide"
    ][:10]

    decision = dict(updated.get("retrieval_decision") or {})
    decision.update({
        "version": AI_RERANK_VERSION,
        "mode": rerank_result.get("mode"),
        "used_ai": rerank_result.get("used_ai", False),
        "intent_summary": rerank_result.get("intent_summary") or decision.get("intent_summary"),
        "answer_focus": rerank_result.get("answer_focus", []),
        "selected_count": len(selected_sources),
        "excluded_count": len(excluded_sources),
        "rejected_candidate_ids": rerank_result.get("rejected_candidate_ids", []),
    })
    updated["retrieval_decision"] = decision
    return updated

def build_selected_sources_text(selected_sources: List[Dict[str, Any]]) -> str:
    blocks = []
    for item in selected_sources[:8]:
        blocks.append("\n".join([
            f"来源ID：{item.get('candidate_id', '')}",
            f"来源类型：{item.get('source_label') or item.get('source_type', '')}",
            f"标题：{item.get('title', '')}",
            f"选择原因：{item.get('reason', '')}",
            f"摘要：{item.get('summary', '')}",
        ]))
    return "\n\n".join(blocks)
