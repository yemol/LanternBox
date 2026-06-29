"""全局配置与运行时设置。只放路径、模型、端口、集合名和开关类配置。"""

import json
import os
from pathlib import Path
from typing import Any, Dict, List

BASE_DIR = Path(__file__).resolve().parent.parent
APP_DIR = BASE_DIR / "app"
DATA_DIR = BASE_DIR / "data"
BACKUP_DIR = BASE_DIR / "backups"
DB_PATH = DATA_DIR / "lanternbox.db"
EMERGENCY_GUIDES_PATH = DATA_DIR / "emergency_guides.json"
EMERGENCY_GUIDES_FILE = DATA_DIR / "emergency_guides.json"
ENV_PATH = BASE_DIR / ".env"


def load_env_file(path: Path = ENV_PATH) -> None:
    if not path.exists():
        return

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except Exception:
        return

    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue

        key, value = stripped.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        if key:
            os.environ.setdefault(key, value)


load_env_file()

CONTEXT_PROFILES_CACHE: List[Dict[str, Any]] = []
GUIDES_CACHE: List[Dict[str, Any]] = []
TRIGGERS_CACHE: List[Dict[str, Any]] = []
RESOURCE_CACHE_INFO: Dict[str, Any] = {
    "emergency_guides_count": 0,
    "ai_triggers_count": 0,
    "loaded": False,
}
GUIDE_TAXONOMY_CACHE = {}


OLLAMA_BASE_URL = os.environ["OLLAMA_URL"].rstrip("/")
OLLAMA_MODEL = os.environ["OLLAMA_MODEL"].strip()

DEFAULT_MODELS = {
    "emergency": OLLAMA_MODEL,
    "companion": OLLAMA_MODEL,
}



# -----------------------------
# Runtime AI retrieval settings
# -----------------------------
RUNTIME_SETTINGS_PATH = DATA_DIR / "runtime_settings.json"

DEFAULT_RUNTIME_SETTINGS: Dict[str, Any] = {
    "ai_rerank_enabled": False,
    "ai_rerank_model": OLLAMA_MODEL,
    "retrieval_mode": "rule",
    "show_retrieval_debug": True,
}


def _normalize_runtime_settings(raw: Dict[str, Any]) -> Dict[str, Any]:
    settings = dict(DEFAULT_RUNTIME_SETTINGS)
    if isinstance(raw, dict):
        settings.update({k: v for k, v in raw.items() if k in DEFAULT_RUNTIME_SETTINGS})

    settings["ai_rerank_enabled"] = bool(settings.get("ai_rerank_enabled"))
    settings["show_retrieval_debug"] = bool(settings.get("show_retrieval_debug"))

    settings["ai_rerank_model"] = OLLAMA_MODEL

    mode = str(settings.get("retrieval_mode") or "").strip()
    if mode not in {"rule", "hybrid", "enhanced"}:
        mode = "hybrid" if settings["ai_rerank_enabled"] else "rule"
    settings["retrieval_mode"] = mode

    return settings


def load_runtime_settings() -> Dict[str, Any]:
    if not RUNTIME_SETTINGS_PATH.exists():
        return dict(DEFAULT_RUNTIME_SETTINGS)

    try:
        with open(RUNTIME_SETTINGS_PATH, "r", encoding="utf-8") as file:
            raw = json.load(file)
    except Exception:
        return dict(DEFAULT_RUNTIME_SETTINGS)

    return _normalize_runtime_settings(raw)


def save_runtime_settings(settings: Dict[str, Any]) -> Dict[str, Any]:
    normalized = _normalize_runtime_settings(settings)
    DATA_DIR.mkdir(exist_ok=True)
    with open(RUNTIME_SETTINGS_PATH, "w", encoding="utf-8") as file:
        json.dump(normalized, file, ensure_ascii=False, indent=2)
    return normalized


def update_runtime_settings(patch: Dict[str, Any]) -> Dict[str, Any]:
    current = load_runtime_settings()
    if isinstance(patch, dict):
        current.update({k: v for k, v in patch.items() if v is not None})
    return save_runtime_settings(current)


# Conversation memory settings
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


DOMAIN_NEGATIVE_KEYWORDS = {
    "power": ["燃气", "煤气", "燃气罐", "食物", "饮水", "伤口", "咳嗽", "发烧"],
    "medical": ["燃气罐", "充电", "移动电源", "电池"],
    "water": ["燃气罐", "电池", "充电"],
}


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
        "电", "电量", "没电", "低电量", "断电",
        "充电", "电池", "移动电源", "充电宝",
        "手机", "照明", "手电", "通讯", "设备",
        "太阳能", "发电", "节电", "省电"
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
        "低电量设备优先级",
        "移动电源使用优先级",
        "照明设备省电",
        "通讯设备节电",
        "太阳能充电安排",
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



# -----------------------------
# Voice service settings
# -----------------------------
TTS_DIR = BASE_DIR / "tts"
TTS_OUTPUT_DIR = TTS_DIR / "output"
TTS_MAX_OUTPUT_FILES = 30
TTS_MAX_FILE_AGE_SECONDS = 60 * 60 * 24

VOICE_SERVICE_DEFAULT_URL = "http://127.0.0.1:8790"
VOICE_SERVICE_URL = os.getenv("LANTERNBOX_VOICE_SERVICE_URL", VOICE_SERVICE_DEFAULT_URL).rstrip("/")
VOICE_SERVICE_TIMEOUT_SECONDS = int(os.getenv("LANTERNBOX_VOICE_TIMEOUT", "90"))

TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


# -----------------------------
# PocketBase / Wiki 配置
# -----------------------------
POCKETBASE_URL = "http://127.0.0.1:8090"
POCKETBASE_WIKI_ARTICLES = "wiki_articles"
POCKETBASE_WIKI_MEDIA = "wiki_media"
POCKETBASE_WIKI_CATEGORIES = "wiki_categories"
