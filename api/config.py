from pathlib import Path
from typing import Any, Dict, List

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

# TTS constants
TTS_DIR = BASE_DIR / "tts"
TTS_VOICES_DIR = TTS_DIR / "voices"
TTS_OUTPUT_DIR = TTS_DIR / "output"
PIPER_DEFAULT_VOICE = "zh_CN-huayan-medium.onnx"
PIPER_MODEL_PATH = TTS_VOICES_DIR / PIPER_DEFAULT_VOICE
TTS_MAX_OUTPUT_FILES = 30
TTS_MAX_FILE_AGE_SECONDS = 60 * 60 * 24

TTS_ENGINE = "melotts"
MELOTTS_PYTHON_PATH = BASE_DIR / "tts_engines" / "melotts" / "venv" / "bin" / "python"
MELOTTS_SCRIPT_PATH = BASE_DIR / "tts_engines" / "melotts" / "melotts_speak.py"
MELOTTS_SPEED = 1.0

DEFAULT_TTS_ENGINE_BY_MODE = {
    "emergency": "piper",
    "companion": "melotts",
}

TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
