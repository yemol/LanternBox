"""Prompt 构造模块。负责应急和陪伴模式的消息组织。

应急模式回答以 Retrieval v2 selected_evidence 映射出的 related_guides / related_wikis / related_kiwix 为证据底座。
"""

from typing import Any, Dict, List, Optional

from ..config import SCENARIO_PROFILE


def build_safe_history(
    history: Optional[List[Any]] = None,
    max_messages: int = 10,
    max_chars_per_message: int = 900,
) -> List[Dict[str, str]]:
    safe_history: List[Dict[str, str]] = []

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


def _format_conversation_summary(summary: str) -> str:
    text = str(summary or "").strip()
    if not text:
        return ""
    return text[:1800]


def _clean_text(value: Any, limit: int = 900) -> str:
    text = str(value or "").strip()
    if not text:
        return ""
    return text[:limit]


def _format_field(value: Any, limit: int = 1200) -> str:
    if isinstance(value, list):
        text = "\n".join(f"- {str(item).strip()}" for item in value if str(item).strip())
    else:
        text = str(value or "").strip()
    return text[:limit]


def _format_guide_evidence(related_guides: List[Dict[str, Any]], limit: int = 6) -> str:
    """把 Retrieval v2 选中的 guide 转成回答层证据文本。

    不在这里重新排序、不重新理解语义，只做格式化。
    """
    blocks: List[str] = []

    for index, guide in enumerate((related_guides or [])[:limit], start=1):
        guide_id = _clean_text(guide.get("id"), 40)
        title = _clean_text(guide.get("title") or guide.get("name"), 120)
        risk_level = _clean_text(guide.get("risk_level"), 20) or "normal"
        lines = [f"【指南 {index}】{guide_id} {title or '未命名指南'}"]
        fields = [
            ("分类", guide.get("category"), 100),
            ("领域", guide.get("domains"), 180),
            ("优先级", guide.get("priority"), 20),
            ("风险等级", risk_level, 20),
            ("场景", guide.get("scenario"), 500),
            ("目标", guide.get("goal"), 500),
            ("工具材料", guide.get("tools"), 800),
        ]
        # High-risk boundaries are placed before actions so the model sees them first.
        if risk_level in {"high", "critical"}:
            fields.extend([
                ("停止与升级边界（优先输出）", guide.get("stop_or_escalate"), 1200),
                ("判断标准", guide.get("check"), 1200),
            ])
        fields.extend([
            ("操作步骤", guide.get("steps"), 1600),
            ("判断标准", guide.get("check") if risk_level not in {"high", "critical"} else None, 1200),
            ("常见错误", guide.get("common_mistakes"), 1000),
            ("本地降级方案", guide.get("fallback"), 1200),
            ("停止条件", guide.get("stop_or_escalate") if risk_level not in {"high", "critical"} else None, 1200),
            ("对应 Wiki slug", guide.get("related_wiki"), 800),
        ])
        for label, value, field_limit in fields:
            rendered = _format_field(value, field_limit)
            if rendered:
                lines.append(f"{label}：\n{rendered}")

        blocks.append("\n".join(lines))

    return "\n\n".join(blocks)


def _format_wiki_evidence(related_wikis: List[Dict[str, Any]], limit: int = 4) -> str:
    """把 Retrieval v2 选中的 wiki 转成回答层证据文本。"""
    blocks: List[str] = []

    for index, wiki in enumerate((related_wikis or [])[:limit], start=1):
        title = _clean_text(wiki.get("title") or wiki.get("name"), 120)
        category = _clean_text(wiki.get("category") or wiki.get("domain"), 80)
        summary = _clean_text(wiki.get("summary") or wiki.get("description"), 600)
        content = _clean_text(
            wiki.get("content")
            or wiki.get("body")
            or wiki.get("text")
            or wiki.get("snippet"),
            900,
        )
        tags = wiki.get("tags") or []
        if isinstance(tags, list):
            tag_text = "、".join(str(tag) for tag in tags[:8] if str(tag).strip())
        else:
            tag_text = str(tags or "").strip()

        slug = _clean_text(wiki.get("slug") or wiki.get("id"), 160)
        source_reason = _clean_text(wiki.get("source_reason"), 80)
        linked_guide_id = _clean_text(wiki.get("linked_guide_id"), 40)
        lines = [f"【Wiki {index}】{slug} {title or '未命名条目'}"]
        if source_reason:
            lines.append(f"证据来源：{source_reason}")
        if linked_guide_id:
            lines.append(f"对应 Guide：{linked_guide_id}")
        if category:
            lines.append(f"分类：{category}")
        if tag_text:
            lines.append(f"标签：{tag_text}")
        if summary:
            lines.append(f"摘要：{summary}")
        if content:
            lines.append(f"内容：{content}")

        blocks.append("\n".join(lines))

    return "\n\n".join(blocks)


def _format_kiwix_evidence(related_kiwix: List[Dict[str, Any]], limit: int = 3) -> str:
    """把 Retrieval v2 选中的 Kiwix/ZIM 转成回答层背景证据文本。"""
    blocks: List[str] = []

    for index, item in enumerate((related_kiwix or [])[:limit], start=1):
        title = _clean_text(item.get("title") or item.get("name"), 120)
        zim_filename = _clean_text(item.get("zim_filename") or item.get("source"), 160)
        article_path = _clean_text(item.get("article_path") or item.get("path"), 180)
        usage_policy = _clean_text(item.get("usage_policy"), 80)
        source_role = _clean_text(item.get("source_role") or item.get("role"), 80)
        snippet = _clean_text(
            item.get("snippet")
            or item.get("summary")
            or item.get("content")
            or item.get("text"),
            900,
        )

        lines = [f"【Kiwix/ZIM 背景 {index}】{title or '未命名条目'}"]
        if zim_filename:
            lines.append(f"ZIM：{zim_filename}")
        if article_path:
            lines.append(f"路径：{article_path}")
        if usage_policy:
            lines.append(f"使用策略：{usage_policy}")
        if source_role:
            lines.append(f"资料角色：{source_role}")
        if snippet:
            lines.append(f"内容：{snippet}")

        blocks.append("\n".join(lines))

    return "\n\n".join(blocks)


def build_companion_messages(
    user_message: str,
    safe_history: List[Dict[str, str]],
    conversation_summary: str = "",
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

    summary_text = _format_conversation_summary(conversation_summary)

    user_prompt = f"""
/no_think

会话摘要：
{summary_text or '暂无。'}

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
    related_guides: List[Dict[str, Any]],
    detected_domains: List[str],
    safe_history: List[Dict[str, str]],
    conversation_summary: str = "",
    related_wikis: Optional[List[Dict[str, Any]]] = None,
    related_kiwix: Optional[List[Dict[str, Any]]] = None,
) -> List[Dict[str, str]]:
    """构造应急模式消息。

    Retrieval v2 之后：
    - detected_domains 只作为弱提示。
    - related_guides / related_wikis / related_kiwix 应来自 Retrieval v2 selected_evidence。
    """
    guide_evidence = _format_guide_evidence(related_guides or [])
    wiki_evidence = _format_wiki_evidence(related_wikis or [])
    kiwix_evidence = _format_kiwix_evidence(related_kiwix or [])
    domain_text = "、".join(detected_domains or []) if detected_domains else "未提供"
    summary_text = _format_conversation_summary(conversation_summary)

    has_local_evidence = bool(guide_evidence or wiki_evidence or kiwix_evidence)
    risk_order = {"normal": 0, "caution": 1, "high": 2, "critical": 3}
    highest_risk = max(
        (str(item.get("risk_level") or "normal") for item in related_guides or []),
        key=lambda value: risk_order.get(value, 0),
        default="normal",
    )
    if highest_risk in {"high", "critical"}:
        risk_directive = (
            f"最高 Guide 风险等级为 {highest_risk}。回答开头必须先写立即停止/禁止动作，"
            "随后写隔离、撤离、停用、不入口、不通电或不可继续等适用边界；"
            "再使用 check 给出可观察判断标准，使用 steps 给出本地行动，使用 fallback 给出缺资源降级方案，"
            "最后写记录与复查。不得先解释背景。"
        )
    else:
        risk_directive = (
            f"最高 Guide 风险等级为 {highest_risk}。使用 check 作为判断标准，fallback 作为本地降级方案，"
            "stop_or_escalate 作为停止条件。"
        )

    system_prompt = """
你是“壳中灯 LanternBox”的应急模式本地离线 AI 助手。

系统默认使用场景：
1. 使用者可能处在断网、断电、断水、断供、灾害、野外、避难、长期居住或低资源环境中。
2. 不要默认存在稳定城市服务、供水公司、物业、医院、快递、外卖、互联网、云端服务或即时救援。
3. 除非用户明确说明自己处在正常城市生活场景，否则优先按“本地资源有限、需要自救和组织管理”的场景回答。
4. 你的核心任务是基于本地证据判断风险，提出可执行建议，并提醒后续观察和记录。

证据使用规则：
1. 本次回答的主要依据是 Retrieval v2 已选中的本地指南和 Wiki 证据。
2. 指南优先用于行动步骤，Wiki 用于背景知识、判断标准和注意事项。
3. Kiwix / ZIM 是离线百科和背景资料，只能用于术语解释、原理说明和补充背景，不能替代指南行动步骤。
4. 如果 Guide 与 Kiwix 内容冲突，行动建议以 Guide 为先；Kiwix 不得压过高置信 Guide。
5. lookup_only 或 background_support_only 的 Kiwix 资料不得作为决策主依据。
6. 不要编造证据中没有的具体数字、药物剂量、设备参数、地点、库存或成员状态。
7. 如果本地证据不足，可以给出通用安全原则，但必须说明“本地资料不足，以下按一般安全原则处理”。
8. 不要让用户自己再去查资料。你要把已选证据转成可执行动作。
9. Guide 的 risk_level、check、fallback、stop_or_escalate 是回答约束，不得遗漏或让模型自行猜测。
10. related_wiki 只补充判断依据和边界，不得替代 Guide 行动步骤。

回答边界：
1. 不要把问题自动理解成普通家庭维修或城市客服问题。
2. 回答正文中禁止把“联系供水公司、联系供电公司、联系电力公司、联系物业、联系客服、上网搜索、叫外卖、快递”作为行动步骤。
3. 如果确实需要外部专业救援，只能用笼统表述“外部救援若仍可用可作为补充”。
4. 遇到医疗、火灾、燃气、电池、洪水、地震、结构损坏等高风险问题时，先给本地立即可做措施，再提示升级处理。
5. 如果用户明确说“先不管/不是/不考虑某主题”，不要把该主题扩展成重点，只可简短确认已暂不处理。

低资源场景特别规则：
1. 当用户提到水、食物、燃料、电量、药品不足时，优先考虑剩余量估算、人数、特殊成员、配给优先级、暂停非必要消耗、备用来源、记录和复查。
2. 缺水时，不要建议完全停止洗手。应保留关键洗手，例如如厕后、做饭前、取饮用水前、处理伤口前后。
3. 缺水时，应暂停洗衣、拖地、洗车、浇灌植物、长时间冲洗等非必要耗水。
4. 缺水时，饮用、用药、病人照护、儿童老人照护和最低限度烹饪优先于一般清洁。

回答格式：
请按下面结构回答，控制在 700 字以内，适合语音播报。每个部分都必须出现：

一、当前判断
用 1 到 3 句话判断风险等级和问题性质。

二、马上做什么
列出最先执行的 3 到 6 个动作。

三、不要做什么
列出容易造成风险的错误做法。

四、本地降级方案
必须使用 Guide fallback，说明缺工具、缺电、缺水、缺人手或无法执行首选方案时怎么做。

五、接下来观察和记录什么
必须使用 Guide check，列出可观察指标、记录项和复查时机。

六、需要追问的信息
如果信息不足，最多追问 1 到 3 个关键问题。

不要在回答正文里重复列出“参考资料标题”。页面下方会单独展示关联指南、Wiki 和 Kiwix 来源。
"""

    user_prompt = f"""
/no_think

用户描述：
{user_message}

当前模式：应急模式

会话摘要：
{summary_text or '暂无。'}

系统场景设定：
{SCENARIO_PROFILE}

弱提示领域：
{domain_text}

Retrieval v2 已选中的本地指南证据：
{guide_evidence or '未选中明确指南证据。'}

Retrieval v2 已选中的本地 Wiki 证据：
{wiki_evidence or '未选中明确 Wiki 证据。'}

Retrieval v2 已选中的 Kiwix / ZIM 背景资料：
{kiwix_evidence or '未选中 Kiwix / ZIM 背景资料。'}

证据状态：
{'已获得本地证据，请优先基于上述证据回答。' if has_local_evidence else '本地资料不足，请明确说明资料不足，并按一般安全原则给出保守建议。'}

风险结构硬约束：
{risk_directive}

请基于以上 Retrieval v2 证据，给出壳中灯 AI 的应急建议。Kiwix / ZIM 只能用于背景解释，不能替代 Guide 的行动建议。
不要在正文末尾重复列出参考资料。
"""

    return [
        {"role": "system", "content": system_prompt},
        *safe_history,
        {"role": "user", "content": user_prompt},
    ]
