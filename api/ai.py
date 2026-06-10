import json
import requests
from typing import Any, Dict, List, Optional

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
