
from typing import Any, Dict, List, Optional, Tuple

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

REFERENCE_MAX_GUIDES = 4
REFERENCE_MAX_WIKIS = 4

AI_RERANK_VERSION = "v0.6.1-ai-reranker"
AI_RERANK_ENV_NAME = "LANTERNBOX_AI_RERANK"
AI_RERANK_MODEL_ENV_NAME = "LANTERNBOX_AI_RERANK_MODEL"
AI_RERANK_MAX_CANDIDATES = 30
AI_RERANK_MAX_SELECTED = 6
AI_RERANK_MAX_EXCLUDED = 10