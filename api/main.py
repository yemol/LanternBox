from fastapi import FastAPI
from fastapi.responses import FileResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from pathlib import Path
from datetime import datetime
import sqlite3
import json
import requests
from typing import Any, Dict, List, Optional
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import subprocess
import uuid
import sys
import time



BASE_DIR = Path(__file__).resolve().parent.parent
APP_DIR = BASE_DIR / "app"
DATA_DIR = BASE_DIR / "data"
BACKUP_DIR = BASE_DIR / "backups"
DB_PATH = DATA_DIR / "lanternbox.db"
EMERGENCY_GUIDES_PATH = DATA_DIR / "emergency_guides.json"
EMERGENCY_GUIDES_FILE = DATA_DIR / "emergency_guides.json"

GUIDES_CACHE: List[Dict[str, Any]] = []
TRIGGERS_CACHE: List[Dict[str, Any]] = []
RESOURCE_CACHE_INFO: Dict[str, Any] = {
    "emergency_guides_count": 0,
    "ai_triggers_count": 0,
    "loaded": False,
}

DEFAULT_MODELS = {
    "emergency": "qwen2.5:3b",
    "companion": "qwen3:8b",
}

# =========================================================
# Conversation Memory Settings
# 对话短期记忆配置 
# 如果觉得会话记忆力不行的话，就可以通过调整这2个参数来进行扩充，但是会给系统带来负担，所以不要轻易增加
# =========================================================
CHAT_HISTORY_MAX_MESSAGES = 6
CHAT_HISTORY_MAX_CHARS_PER_MESSAGE = 600


DATA_DIR.mkdir(exist_ok=True)
BACKUP_DIR.mkdir(exist_ok=True)

SCENARIO_PROFILE = """
当前系统默认场景是“壳中灯 LanternBox 离线生存与长期自给环境”。

默认假设：
1. 可能断网、断电、断水、断供。
2. 外部服务可能不可用，包括供水公司、物业、医院、快递、外卖、互联网搜索。
3. 使用者可能处于灾害、避难、野外、长期居住、资源紧张或低功耗设备环境。
4. 回答必须优先使用本地资源、本地库存、本地人员、本地地图和本地指南。
5. 不要默认用户能马上获得城市服务。
6. 如果外部服务仍可用，只能作为最后补充，不作为第一建议。

回答目标：
先保证人员安全，再保护饮水、食物、医疗、能源、通信、数据和路线。
"""

DOMAIN_KEYWORDS = {
    "water": [
        "水", "饮用水", "储水", "缺水", "没水", "不够水", "水不够",
        "水源", "净水", "取水", "雨水", "井水", "桶", "配水",
        "脱水", "水快没了", "水快用完", "水只够", "撑不到", "不够几天"
    ],
    "food": [
        "食物", "粮食", "米", "面", "罐头", "吃", "发霉", "变质",
        "蛋白", "油", "配给", "主食", "剩饭", "剩菜", "饿", "粮"
    ],
    "medical": [
        "发烧", "发热", "腹泻", "呕吐", "出血", "伤口", "疼",
        "咳嗽", "呼吸", "昏迷", "感染", "药", "老人", "儿童",
        "头晕", "意识", "抽搐", "红肿", "化脓"
    ],
    "disaster": [
        "火灾", "着火", "烟", "洪水", "地震", "余震", "台风",
        "暴雨", "雷电", "倒灌", "坍塌", "裂缝", "进水", "内涝"
    ],
    "security": [
        "陌生人", "敲门", "冲突", "争吵", "失联", "不见了",
        "脚步声", "夜间", "可疑", "交易", "跟踪", "威胁"
    ],
    "power": [
        "停电", "电池", "充电", "移动电源", "燃料", "煤气",
        "发电", "灯", "低电量", "没电", "断电", "电池发热", "设备发热", "充电发热", "电源发热", "鼓包"
    ],
    "data": [
        "备份", "数据", "系统", "更新", "档案", "口令", "证件",
        "照片", "加密", "恢复", "文件"
    ],
    "map": [
        "路线", "地图", "导航", "集合点", "水源点", "风险点",
        "怎么去", "最近", "位置", "路", "方向"
    ],
}

DOMAIN_FALLBACK_GUIDES = {
    "water": [
        "缺水时用途排序",
        "紧急配水记录",
        "备用水封存",
        "高温缺水观察",
        "饮用水分区储存",
        "水质异常初筛",
        "取水路线演练"
    ],
    "food": [
        "食物库存天数估算",
        "基础配给线",
        "主食轮换规则",
        "蛋白来源分配",
        "剩食安全处理"
    ],
    "medical": [
        "疑似传染症状分区",
        "症状时间线记录",
        "咳嗽发热照护",
        "腹泻呕吐隔离流程",
        "伤员转运信息卡"
    ],
    "disaster": [
        "火灾后返回检查",
        "洪水后进入检查",
        "地震后室内快速检查",
        "余震准备点",
        "烟尘进入室内处理"
    ],
    "security": [
        "陌生人接近记录",
        "冲突降级沟通",
        "失联人员最后位置记录",
        "夜间值守异常上报"
    ],
    "power": [
        "停电后第一小时",
        "低电量工作模式",
        "移动电源保管",
        "燃料节约使用"
    ],
    "data": [
        "指南版本号规则",
        "数据恢复演练",
        "成员档案权限分级",
        "成员档案应急读取"
    ],
    "map": [
        "集合点三层规则",
        "留言点安全规则",
        "取水路线演练",
        "路线安全重新评估"
    ],
}

# 设定与TTS有关的常量信息
# 这里我们使用的引擎是 Piper
TTS_DIR = BASE_DIR / "tts"
TTS_VOICES_DIR = TTS_DIR / "voices"
TTS_OUTPUT_DIR = TTS_DIR / "output"
PIPER_DEFAULT_VOICE = "zh_CN-huayan-medium.onnx"
PIPER_MODEL_PATH = TTS_VOICES_DIR / PIPER_DEFAULT_VOICE
TTS_MAX_OUTPUT_FILES = 30
TTS_MAX_FILE_AGE_SECONDS = 60 * 60 * 24

# 这里是Melotts的常量设置
TTS_ENGINE = "melotts"  # piper / melotts
MELOTTS_PYTHON_PATH = BASE_DIR / "tts_engines" / "melotts" / "venv" / "bin" / "python"
MELOTTS_SCRIPT_PATH = BASE_DIR / "tts_engines" / "melotts" / "melotts_speak.py"
MELOTTS_SPEED = 1.0

DEFAULT_TTS_ENGINE_BY_MODE = {
    "emergency": "piper",
    "companion": "melotts",
}



app = FastAPI(title="LanternBox", version="0.4.0")
app.mount("/static", StaticFiles(directory=APP_DIR), name="static")


TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
app.mount(
    "/tts_output",
    StaticFiles(directory=TTS_OUTPUT_DIR),
    name="tts_output"
)

class ChatHistoryItem(BaseModel):
    role: str
    content: str

class AiAdviceRequest(BaseModel):
    message: str
    mode: Optional[str] = "emergency"
    model: Optional[str] = None
    metadata_only: Optional[bool] = False
    history: Optional[List[ChatHistoryItem]] = None

class InventoryItem(BaseModel):
    name: str
    category: str = ""
    quantity: float = 0
    unit: str = ""
    expire_date: str = ""
    note: str = ""

class JournalEntry(BaseModel):
    entry_type: str = "日常记录"
    title: str = ""
    content: str

class TtsSpeakRequest(BaseModel):
    text: str
    mode: Optional[str] = "assistant"
    engine: Optional[str] = None


def get_tts_engine(mode: str, engine: Optional[str] = None) -> str:
    if engine in ["piper", "melotts"]:
        return engine

    return DEFAULT_TTS_ENGINE_BY_MODE.get(mode, "piper")

def get_default_model_for_mode(mode: str) -> str:
    return DEFAULT_MODELS.get(mode, "qwen2.5:3b")

def load_local_resources() -> None:
    global GUIDES_CACHE, TRIGGERS_CACHE, RESOURCE_CACHE_INFO

    guides_path = DATA_DIR / "emergency_guides.json"
    triggers_path = DATA_DIR / "ai_triggers.json"

    GUIDES_CACHE = read_json_file(guides_path, [])
    TRIGGERS_CACHE = read_json_file(triggers_path, [])

    RESOURCE_CACHE_INFO = {
        "emergency_guides_count": len(GUIDES_CACHE),
        "ai_triggers_count": len(TRIGGERS_CACHE),
        "loaded": True,
        "guides_path": str(guides_path),
        "triggers_path": str(triggers_path),
    }

    print("本地资源缓存已加载：", RESOURCE_CACHE_INFO)


def detect_domains(text: str) -> List[str]:
    result = []
    clean_text = (text or "").strip()

    if not clean_text:
        return result

    for domain, keywords in DOMAIN_KEYWORDS.items():
        if any(keyword in clean_text for keyword in keywords):
            result.append(domain)

    return result

def find_domain_fallback_guides(
    domains: List[str],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    result = []
    seen_ids = set()

    def add_guide(guide: Dict[str, Any]):
        guide_id = guide.get("id") or guide.get("title")
        if not guide_id or guide_id in seen_ids:
            return
        seen_ids.add(guide_id)
        result.append(guide)

    for domain in domains:
        expected_titles = DOMAIN_FALLBACK_GUIDES.get(domain, [])

        for guide in guides:
            title = guide.get("title", "") or ""
            category = guide.get("category", "") or ""
            category_original = guide.get("category_original", "") or ""
            scenario = guide.get("scenario", "") or ""
            goal = guide.get("goal", "") or ""

            haystack = " ".join([
                title,
                category,
                category_original,
                scenario,
                goal,
            ])

            for expected in expected_titles:
                # 完全一致
                if title == expected:
                    add_guide(guide)
                    break

                # 互相包含
                if expected in title or title in expected:
                    add_guide(guide)
                    break

                # 标题关键词部分重合
                expected_chars = set(expected)
                title_chars = set(title)

                if expected_chars and title_chars:
                    overlap = len(expected_chars & title_chars)
                    ratio = overlap / max(len(expected_chars), 1)

                    if ratio >= 0.45:
                        add_guide(guide)
                        break

                # 指南内容里包含兜底关键词
                if expected and expected in haystack:
                    add_guide(guide)
                    break

    return result

def find_guides_by_domain_keywords(
    domains: List[str],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    result = []
    seen_ids = set()

    for domain in domains:
        keywords = DOMAIN_KEYWORDS.get(domain, [])

        for guide in guides:
            guide_id = guide.get("id") or guide.get("title")

            if not guide_id or guide_id in seen_ids:
                continue

            searchable_text = " ".join([
                str(guide.get("title", "")),
                str(guide.get("category", "")),
                str(guide.get("category_original", "")),
                str(guide.get("scenario", "")),
                str(guide.get("goal", "")),
                " ".join(guide.get("steps", []) or []),
                " ".join(guide.get("check", []) or []),
                " ".join(guide.get("common_mistakes", []) or []),
            ])

            score = 0

            for keyword in keywords:
                if keyword and keyword in searchable_text:
                    score += 1

            if score > 0:
                item = dict(guide)
                item["_domain_score"] = score
                result.append(item)
                seen_ids.add(guide_id)

    result.sort(key=lambda item: item.get("_domain_score", 0), reverse=True)
    return result


def merge_guides(*guide_lists: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    result = []
    seen_ids = set()

    for guide_list in guide_lists:
        for guide in guide_list:
            guide_id = guide.get("id") or guide.get("title")

            if not guide_id or guide_id in seen_ids:
                continue

            seen_ids.add(guide_id)
            result.append(guide)

    return result


def read_json_file(file_path: Path, fallback: Any):
    try:
        if not file_path.exists():
            print(f"文件不存在：{file_path}")
            return fallback

        with file_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"读取或解析 JSON 失败：{file_path}", e)
        return fallback


def get_severity_weight(severity: str) -> int:
    return {
        "紧急": 4,
        "高": 3,
        "中": 2,
        "低": 1,
    }.get(severity, 0)


def match_triggers(input_text: str, triggers: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    text = (input_text or "").strip()
    if not text:
        return []

    matched = []

    for trigger in triggers:
        score = 0

        for keyword in trigger.get("trigger_keywords", []):
            if keyword and keyword in text:
                score += 3

        title = trigger.get("title", "")
        if title and title in text:
            score += 5

        if score > 0:
            item = dict(trigger)
            item["matchScore"] = score
            matched.append(item)

    matched.sort(
        key=lambda x: (
            get_severity_weight(x.get("severity", "")),
            x.get("matchScore", 0),
        ),
        reverse=True,
    )

    return matched


def find_related_guides(
    matched_triggers: List[Dict[str, Any]],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    guide_by_title = {g.get("title"): g for g in guides}
    result = []
    seen_ids = set()

    for trigger in matched_triggers:
        for title in trigger.get("related_guides", []):
            guide = guide_by_title.get(title)

            if not guide:
                continue

            guide_id = guide.get("id") or guide.get("title")

            if guide_id in seen_ids:
                continue

            seen_ids.add(guide_id)
            result.append(guide)

    return result

def build_local_context(
    matched_triggers: List[Dict[str, Any]],
    related_guides: List[Dict[str, Any]],
) -> Dict[str, str]:
    trigger_blocks = []

    for trigger in matched_triggers[:3]:
        suggested_actions = trigger.get("suggested_actions", [])
        follow_up = trigger.get("follow_up", [])

        trigger_blocks.append(
            "\n".join([
                f"触发场景：{trigger.get('title', '')}",
                f"严重等级：{trigger.get('severity', '')}",
                f"AI提示：{trigger.get('ai_prompt', '')}",
                f"建议动作：{'；'.join(suggested_actions[:5])}",
                f"后续追踪：{'；'.join(follow_up[:3])}",
            ])
        )

    guide_blocks = []

    for guide in related_guides[:5]:
        steps = guide.get("steps", [])
        stop_or_escalate = guide.get("stop_or_escalate", [])

        guide_blocks.append(
            "\n".join([
                f"指南标题：{guide.get('title', '')}",
                f"分类：{guide.get('category', '')}",
                f"适用场景：{guide.get('scenario', '')}",
                f"目标：{guide.get('goal', '')}",
                f"关键步骤：{'；'.join(steps[:5])}",
                f"停止或升级：{'；'.join(stop_or_escalate[:3])}",
            ])
        )

    return {
        "trigger_text": "\n\n".join(trigger_blocks),
        "guide_text": "\n\n".join(guide_blocks),
    }


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
                    "num_predict": 700
                }
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
                    "num_predict": 700
                }
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


def prepare_ai_context(user_message: str, mode: str) -> Dict[str, Any]:
    if mode == "companion":
        return {
            "detected_domains": [],
            "matched_triggers": [],
            "related_guides": [],
        }

    if not RESOURCE_CACHE_INFO.get("loaded"):
        load_local_resources()

    guides = GUIDES_CACHE
    triggers = TRIGGERS_CACHE

    detected_domains = detect_domains(user_message)
    matched_triggers = match_triggers(user_message, triggers)[:5]

    trigger_guides = find_related_guides(matched_triggers, guides)
    domain_guides = find_domain_fallback_guides(detected_domains, guides)
    keyword_guides = find_guides_by_domain_keywords(detected_domains, guides)

    related_guides = merge_guides(
        trigger_guides,
        domain_guides,
        keyword_guides,
    )[:5]

    return {
        "detected_domains": detected_domains,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
    }

def build_safe_history(
    history: Optional[List[Any]] = None,
    max_messages: int = CHAT_HISTORY_MAX_MESSAGES,
    max_chars_per_message: int = CHAT_HISTORY_MAX_CHARS_PER_MESSAGE,
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
) -> List[Dict[str, str]]:
    context = build_local_context(matched_triggers, related_guides)

    system_prompt = """
你是“壳中灯 LanternBox”的应急模式本地离线 AI 助手。

系统默认使用场景：
1. 使用者可能处在断网、断电、断水、断供、灾害、野外、避难、长期居住或低资源环境中。
2. 不要默认存在稳定城市服务、供水公司、外卖、快递、医院、互联网、云端服务或即时救援。
3. 除非用户明确说明自己处在正常城市生活场景，否则优先按“本地资源有限、需要自救和组织管理”的场景回答。
4. 你的核心任务不是让使用者自己查资料，而是主动调用本地资料，判断风险，提出可执行建议，并提醒后续观察和记录。

回答原则：
1. 优先基于本地资料回答，包括触发规则、应急指南、库存、成员档案、地图、日志和百科。
2. 当前阶段主要可用资料是 ai_triggers.json 和 emergency_guides.json。
3. 不要编造本地资料中没有的事实。
4. 如果本地资料不足，可以给出通用安全建议，但必须说明这是基于一般原则。
5. 回答要适合语音播报，句子不要太长。
6. 回答应控制在 600 字以内，优先给行动步骤。
7. 不要在回答正文最后重复列出本地触发场景和指南标题，页面会单独展示这些资料。
8. 可以参考对话历史，但当前用户最后一句话优先级最高。
9. 如果对话历史和当前风险冲突，以当前风险为准。

场景边界：
1. 不要把问题自动理解成普通家庭维修或城市客服问题。
2. 不要优先建议“联系供水公司、物业、客服、快递、外卖、上网搜索”等依赖外部稳定系统的方案。
3. 可以在最后补充：“如果确认外部服务仍可用，再联系相关部门或专业人员。”
4. 遇到医疗、火灾、燃气、电池、洪水、地震、结构损坏等高风险问题时，可以提醒停止危险操作并升级处理。
5. 如果需要外部专业救援，必须先给出当前立刻能做的本地安全措施，再说明外部救援作为补充选项。

低资源场景特别规则：
1. 当用户提到水不够、食物不够、燃料不够、电量不足、药品不足时，必须优先考虑剩余量估算、人数、特殊成员、配给优先级、暂停非必要消耗、备用来源、记录和复查。
2. 缺水时，不要建议完全停止洗手。正确做法是保留关键洗手，例如如厕后、做饭前、取饮用水前、处理伤口前后。
3. 缺水时，应暂停洗衣、拖地、洗车、浇灌植物、长时间冲洗等非必要耗水。
4. 缺水时，饮用、用药、病人照护、儿童老人照护和最低限度烹饪优先于一般清洁。
5. 不要建议饮用来源不明、气味异常、油膜、污水接触或未处理的水。

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
{", ".join(detected_domains) if detected_domains else "未识别到明确领域"}

匹配到的本地触发场景：
{context["trigger_text"] or "未命中精确触发规则。"}

关联的本地指南：
{context["guide_text"] or "未找到明确关联指南。"}

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


def serialize_related_guides(related_guides: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    return [
        {
            "id": g.get("id"),
            "title": g.get("title"),
            "category": g.get("category"),
            "category_original": g.get("category_original"),
            "scenario": g.get("scenario"),
            "goal": g.get("goal"),
            "tools": g.get("tools", []),
            "steps": g.get("steps", []),
            "check": g.get("check", []),
            "common_mistakes": g.get("common_mistakes", []),
            "fallback": g.get("fallback"),
            "stop_or_escalate": g.get("stop_or_escalate", []),
            "notes": g.get("notes"),
        }
        for g in related_guides
    ]



def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def column_exists(conn, table_name: str, column_name: str) -> bool:
    rows = conn.execute(f"PRAGMA table_info({table_name})").fetchall()
    return any(row["name"] == column_name for row in rows)


def make_item_code(item_id: int) -> str:
    return f"LB-{item_id:05d}"


def backfill_item_codes(conn):
    rows = conn.execute(
        """
        SELECT id FROM inventory
        WHERE item_code IS NULL OR item_code = ''
        ORDER BY id ASC
        """
    ).fetchall()

    for row in rows:
        item_code = make_item_code(row["id"])
        conn.execute(
            "UPDATE inventory SET item_code = ? WHERE id = ?",
            (item_code, row["id"])
        )


def init_db():
    conn = get_db_connection()

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS inventory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_code TEXT UNIQUE,
            name TEXT NOT NULL,
            category TEXT,
            quantity REAL DEFAULT 0,
            unit TEXT,
            expire_date TEXT,
            note TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS journal (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            entry_type TEXT DEFAULT '日常记录',
            title TEXT,
            content TEXT NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    if not column_exists(conn, "inventory", "item_code"):
        conn.execute("ALTER TABLE inventory ADD COLUMN item_code TEXT")

    backfill_item_codes(conn)

    conn.commit()
    conn.close()

def cleanup_tts_output() -> None:
    if not TTS_OUTPUT_DIR.exists():
        return

    wav_files = sorted(
        TTS_OUTPUT_DIR.glob("*.wav"),
        key=lambda item: item.stat().st_mtime,
        reverse=True,
    )

    now = time.time()

    for index, wav_file in enumerate(wav_files):
        try:
            file_age = now - wav_file.stat().st_mtime

            should_delete_by_count = index >= TTS_MAX_OUTPUT_FILES
            should_delete_by_age = file_age > TTS_MAX_FILE_AGE_SECONDS

            if should_delete_by_count or should_delete_by_age:
                wav_file.unlink(missing_ok=True)

        except Exception as error:
            print("清理 TTS 输出文件失败：", wav_file, error)

def run_piper_tts(text: str, output_path: Path) -> None:
    if not PIPER_MODEL_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"Piper 语音模型不存在：{PIPER_MODEL_PATH}"
        )

    subprocess.run(
        [
            sys.executable,
            "-m",
            "piper",
            "--model",
            str(PIPER_MODEL_PATH),
            "--output_file",
            str(output_path),
        ],
        input=text,
        text=True,
        check=True,
        capture_output=True,
    )


def run_melotts_tts(text: str, output_path: Path) -> None:
    if not MELOTTS_PYTHON_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"MeloTTS Python 不存在：{MELOTTS_PYTHON_PATH}"
        )

    if not MELOTTS_SCRIPT_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"MeloTTS 脚本不存在：{MELOTTS_SCRIPT_PATH}"
        )

    subprocess.run(
        [
            str(MELOTTS_PYTHON_PATH),
            str(MELOTTS_SCRIPT_PATH),
            "--text",
            text,
            "--output",
            str(output_path),
            "--speed",
            str(MELOTTS_SPEED),
        ],
        text=True,
        check=True,
        capture_output=True,
    )

@app.on_event("startup")
def on_startup():
    init_db()
    load_local_resources()


@app.get("/")
def home():
    return FileResponse(APP_DIR / "index.html")


@app.get("/inventory")
def inventory_page():
    return FileResponse(APP_DIR / "inventory.html")


@app.get("/api/status")
def status():
    return {
        "name": "LanternBox",
        "version": "0.4.0",
        "mode": "offline-ready",
        "message": "壳中灯已启动。"
    }


@app.get("/api/inventory")
def list_inventory():
    conn = get_db_connection()
    rows = conn.execute(
        "SELECT * FROM inventory ORDER BY id DESC"
    ).fetchall()
    conn.close()
    return [dict(row) for row in rows]


@app.post("/api/inventory")
def add_inventory(item: InventoryItem):
    conn = get_db_connection()

    cursor = conn.execute(
        """
        INSERT INTO inventory
        (name, category, quantity, unit, expire_date, note)
        VALUES (?, ?, ?, ?, ?, ?)
        """,
        (
            item.name,
            item.category,
            item.quantity,
            item.unit,
            item.expire_date,
            item.note,
        ),
    )

    new_id = cursor.lastrowid
    item_code = make_item_code(new_id)

    conn.execute(
        "UPDATE inventory SET item_code = ? WHERE id = ?",
        (item_code, new_id)
    )

    conn.commit()
    conn.close()

    return {
        "ok": True,
        "id": new_id,
        "item_code": item_code,
        "message": "物资已记录。"
    }


@app.delete("/api/inventory/{item_id}")
def delete_inventory(item_id: int):
    conn = get_db_connection()
    cursor = conn.execute(
        "DELETE FROM inventory WHERE id = ?",
        (item_id,)
    )
    conn.commit()
    deleted_count = cursor.rowcount
    conn.close()

    if deleted_count == 0:
        return {
            "ok": False,
            "message": "没有找到这条物资记录。"
        }

    return {
        "ok": True,
        "message": "物资已删除。"
    }


@app.post("/api/backup")
def backup_database():
    if not DB_PATH.exists():
        return {
            "ok": False,
            "message": "数据库文件不存在，无法备份。"
        }

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = BACKUP_DIR / f"lanternbox_{timestamp}.db"

    source = sqlite3.connect(DB_PATH)
    target = sqlite3.connect(backup_path)

    try:
        source.backup(target)
    finally:
        target.close()
        source.close()

    return {
        "ok": True,
        "message": "数据库备份成功。",
        "file": backup_path.name
    }

@app.get("/journal")
def journal_page():
    return FileResponse(APP_DIR / "journal.html")


@app.get("/api/journal")
def list_journal():
    conn = get_db_connection()
    rows = conn.execute(
        "SELECT * FROM journal ORDER BY id DESC"
    ).fetchall()
    conn.close()
    return [dict(row) for row in rows]


@app.post("/api/journal")
def add_journal(entry: JournalEntry):
    created_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    conn = get_db_connection()
    cursor = conn.execute(
        """
        INSERT INTO journal
        (entry_type, title, content, created_at)
        VALUES (?, ?, ?, ?)
        """,
        (
            entry.entry_type,
            entry.title,
            entry.content,
            created_at,
        ),
    )
    conn.commit()
    new_id = cursor.lastrowid
    conn.close()

    return {
        "ok": True,
        "id": new_id,
        "created_at": created_at,
        "message": "日记已记录。"
    }


@app.delete("/api/journal/{entry_id}")
def delete_journal(entry_id: int):
    conn = get_db_connection()
    cursor = conn.execute(
        "DELETE FROM journal WHERE id = ?",
        (entry_id,)
    )
    conn.commit()
    deleted_count = cursor.rowcount
    conn.close()

    if deleted_count == 0:
        return {
            "ok": False,
            "message": "没有找到这条日记记录。"
        }

    return {
        "ok": True,
        "message": "日记已删除。"
    }

@app.get("/emergency")
def emergency_page():
    return FileResponse(APP_DIR / "emergency.html")


@app.get("/api/emergency-guides")
def list_emergency_guides():
    if not EMERGENCY_GUIDES_PATH.exists():
        return []

    with open(EMERGENCY_GUIDES_PATH, "r", encoding="utf-8") as file:
        return json.load(file)
    


@app.get("/api/emergency")
def get_emergency_guides():
    if not EMERGENCY_GUIDES_FILE.exists():
        raise HTTPException(status_code=404, detail="emergency_guides.json not found")

    try:
        with open(EMERGENCY_GUIDES_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        raise HTTPException(status_code=500, detail=f"JSON 格式错误：{e}")
    

@app.post("/api/ai/advice")
def ai_advice(payload: AiAdviceRequest):
    user_message = payload.message.strip()
    mode = payload.mode or "emergency"
    model = payload.model or get_default_model_for_mode(mode)

    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    context_data = prepare_ai_context(user_message, mode)
    detected_domains = context_data["detected_domains"]
    matched_triggers = context_data["matched_triggers"]
    related_guides = context_data["related_guides"]

    if payload.metadata_only:
        answer = ""
    else:
        messages = build_ai_messages(
            user_message=user_message,
            mode=mode,
            matched_triggers=matched_triggers,
            related_guides=related_guides,
            detected_domains=detected_domains,
            history=payload.history,
        )

        try:
            answer = call_ollama(messages=messages, model=model)
        except Exception as e:
            print("Ollama 调用失败，返回本地规则建议：", e)
            answer = build_fallback_answer(mode, matched_triggers, related_guides)

    return {
        "ok": True,
        "mode": mode,
        "message": user_message,
        "detected_domains": detected_domains,
        "answer": answer,
        "matched_triggers": [
            {
                "id": t.get("id"),
                "title": t.get("title"),
                "category": t.get("category"),
                "severity": t.get("severity"),
                "reminder_type": t.get("reminder_type"),
                "matchScore": t.get("matchScore"),
                "should_log": t.get("should_log"),
                "should_create_task": t.get("should_create_task"),
            }
            for t in matched_triggers
        ],
        "related_guides": serialize_related_guides(related_guides),
    }


@app.post("/api/ai/advice/stream")
def ai_advice_stream(payload: AiAdviceRequest):
    user_message = payload.message.strip()
    mode = payload.mode or "emergency"
    model = payload.model or get_default_model_for_mode(mode)

    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    context_data = prepare_ai_context(user_message, mode)

    messages = build_ai_messages(
        user_message=user_message,
        mode=mode,
        matched_triggers=context_data["matched_triggers"],
        related_guides=context_data["related_guides"],
        detected_domains=context_data["detected_domains"],
        history=payload.history,
    )

    def event_generator():
        for chunk in stream_ollama(messages, model=model):
            yield chunk

    return StreamingResponse(event_generator(), media_type="text/plain; charset=utf-8")


@app.get("/assistant.html")
def assistant_page():
    return FileResponse(APP_DIR / "assistant.html")

@app.post("/api/resources/reload")
def reload_resources():
    load_local_resources()

    return {
        "ok": True,
        "message": "本地资源缓存已刷新",
        "cache": RESOURCE_CACHE_INFO,
    }

@app.get("/api/resources/status")
def resources_status():
    return {
        "ok": True,
        "cache": RESOURCE_CACHE_INFO,
    }

@app.post("/api/tts/speak")
def tts_speak(payload: TtsSpeakRequest):
    text = payload.text.strip()

    if not text:
        raise HTTPException(status_code=400, detail="请提供需要朗读的文本。")

    if not PIPER_MODEL_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"Piper 语音模型不存在：{PIPER_MODEL_PATH}"
        )

    TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    cleanup_tts_output()

    output_filename = f"tts_{uuid.uuid4().hex}.wav"
    output_path = TTS_OUTPUT_DIR / output_filename

    try:
        selected_engine = get_tts_engine(payload.mode, payload.engine)

        if selected_engine == "melotts":
            run_melotts_tts(text, output_path)
        else:
            run_piper_tts(text, output_path)

    except subprocess.CalledProcessError as e:
        raise HTTPException(
            status_code=500,
            detail=f"TTS 生成语音失败：{e.stderr or e.stdout or str(e)}"
        )

    return {
        "ok": True,
        "engine": selected_engine,
        "audio_url": f"/tts_output/{output_filename}",
        "filename": output_filename,
    }