"""指南检索模块。负责应急指南的意图分析、打分与召回。"""

from typing import Any, Dict, List
import re

from ..config import DOMAIN_KEYWORDS
from ..utils import safe_text, contains_any, unique_list
from ..services.guide_service import (
    guide_core_text,
    guide_full_text,
    guide_domains,
    guide_compatible_with_domains,
)

# 指南检索能力：意图分析、打分、召回

QUERY_INTENT_RULES = [
    {
        "intent": "security_night_anomaly",
        "domains": [
            "security"
        ],
        "situations": [
            "night",
            "external_anomaly"
        ],
        "objects": [
            "outside_noise",
            "door",
            "window"
        ],
        "must_any": [
            "异响",
            "响动",
            "脚步",
            "脚步声",
            "门外",
            "窗外",
            "外面"
        ],
        "keywords": [
            "晚上",
            "夜间",
            "担心",
            "害怕",
            "声音",
            "异响",
            "响动",
            "脚步"
        ],
        "preferred_titles": [
            "夜间外部异响",
            "夜间守夜",
            "门窗低暴露"
        ],
        "bad_title_words": [
            "主食",
            "剩饭",
            "晒干",
            "种子",
            "自行车",
            "宠物"
        ],
        "priority": 100
    },
    {
        "intent": "security_suspicious_person",
        "domains": [
            "security"
        ],
        "situations": [
            "suspicious_approach",
            "resource_exposure"
        ],
        "objects": [
            "person",
            "door",
            "supplies"
        ],
        "must_any": [
            "门外",
            "陌生人",
            "可疑",
            "靠近",
            "反复",
            "物资",
            "资源暴露"
        ],
        "keywords": [
            "门外",
            "反复",
            "靠近",
            "冲着物资",
            "物资",
            "不对峙",
            "收缩",
            "资源暴露"
        ],
        "preferred_titles": [
            "可疑人员接近",
            "资源暴露后的应对",
            "门窗低暴露",
            "夜间外部异响"
        ],
        "bad_title_words": [
            "宠物",
            "生活光痕",
            "食物",
            "主食",
            "种子"
        ],
        "priority": 98
    },
    {
        "intent": "night_toilet_safety",
        "domains": ["hygiene", "security", "power", "medical"],
        "situations": ["night", "toilet_route", "fall_risk"],
        "objects": ["toilet", "lighting", "route"],
        "must_any": ["厕所", "太黑", "摔倒", "晚上", "夜间"],
        "keywords": ["晚上", "夜间", "厕所", "太黑", "摔倒", "路线", "照明", "低亮", "安全"],
        "preferred_titles": ["夜间厕所路线", "低亮照明", "夜间行动安全", "老人夜间如厕"],
        "bad_title_words": ["呕吐物", "腹泻污染", "剩饭", "冰箱"],
        "priority": 95
    },
    {
        "intent": "thermal_care_cold",
        "domains": ["medical", "shelter", "power"],
        "situations": ["cold_exposure", "hypothermia_watch", "night"],
        "objects": ["elder", "clothes", "sleeping_area"],
        "must_any": ["冷", "手脚冰凉", "保暖", "失温"],
        "keywords": ["老人", "晚上", "冷", "手脚冰凉", "意识清楚", "保暖", "观察", "失温"],
        "preferred_titles": ["失温", "睡眠保温检查", "室内保温", "小空间集中保暖", "极寒断电过夜"],
        "bad_title_words": ["意识混乱", "药物提醒", "热夜睡眠", "可疑人员"],
        "priority": 94
    },
    {
        "intent": "medical_dehydration_diarrhea",
        "domains": ["medical", "water", "hygiene"],
        "situations": ["diarrhea", "dehydration"],
        "objects": ["diarrhea", "patient", "drinking_water"],
        "must_any": ["腹泻", "脱水", "喝不下", "水也喝不下"],
        "keywords": ["腹泻", "一天", "喝不下", "防脱水", "脱水", "补液", "呕吐"],
        "preferred_titles": ["腹泻脱水", "腹泻超过一天", "腹泻呕吐隔离", "补液"],
        "bad_title_words": ["停水后的用水优先级", "门窗", "主食", "低电量"],
        "priority": 93
    },
    {
        "intent": "medical_fracture_fix",
        "domains": ["medical", "tools"],
        "situations": ["fracture", "temporary_fix"],
        "objects": ["splint", "tools", "wound"],
        "must_any": ["骨折", "夹板"],
        "keywords": ["疑似骨折", "骨折", "夹板", "临时固定", "固定", "木板", "硬纸板"],
        "preferred_titles": ["疑似骨折", "骨折固定", "木板和木棍再利用", "固定不复位"],
        "bad_title_words": ["门窗临时加固", "门窗低暴露", "门窗静音"],
        "priority": 94
    },
    {
        "intent": "shelter_damp_mold",
        "domains": ["shelter", "hygiene"],
        "situations": ["damp", "mold"],
        "objects": ["building", "clothes", "sleeping_area"],
        "must_any": ["潮", "发霉", "霉", "墙角"],
        "keywords": ["暴雨", "屋里", "潮", "墙角", "发霉", "水位没有上涨", "没有上涨", "防潮", "排湿"],
        "preferred_titles": ["室内防潮防霉", "潮湿空气排出", "公共睡眠区卫生", "衣物污染隔离"],
        "bad_title_words": ["洪水来临前", "暴雨山洪", "撤高", "是否撤离"],
        "priority": 91
    },
    {
        "intent": "power_water_electrical_safety",
        "domains": ["power", "security", "tools"],
        "situations": ["water_contact_electric", "power_restore"],
        "objects": ["power_strip", "wire", "device"],
        "must_any": ["插线板", "进过水", "进水", "恢复供电", "漏电"],
        "keywords": ["插线板", "进过水", "进水", "恢复供电", "还能不能继续用", "电线", "漏电", "电器", "停用"],
        "preferred_titles": ["电线和插线板安全检查", "漏电检查", "火灾后电器停用", "维修前断电断火断水"],
        "bad_title_words": ["冰箱食物", "剩饭", "雨水收集", "主食"],
        "priority": 96
    },
    {
        "intents": [
            "food_shortage",
            "food_rationing",
            "resource_management"
        ],
        "domains": [
            "food",
            "power"
        ],
        "situations": [
            "power_outage",
            "refrigeration_failure",
            "food_spoilage"
        ],
        "objects": [
            "fridge",
            "meat",
            "leftovers",
            "canned_food",
            "cooked_food"
        ],
        "must_any": [
            "能不能吃",
            "还能不能吃",
            "变质",
            "鼓包",
            "冰箱",
            "冷藏",
            "冷冻",
            "肉",
            "剩饭",
            "剩菜",
            "罐头"
        ],
        "keywords": [
            "停电",
            "断电",
            "冰箱",
            "冷藏",
            "冷冻",
            "肉",
            "剩饭",
            "剩菜",
            "熟食",
            "罐头",
            "变质",
            "能不能吃"
        ],
        "preferred_titles": [
            "停电后冰箱食物处置",
            "剩饭处理",
            "剩食安全处理",
            "食物风险判断",
            "罐头检查"
        ],
        "bad_title_words": [
            "低电量",
            "低亮",
            "照明",
            "充电优先级",
            "工作模式"
        ],
        "priority": 96
    },
    {
        "intent": "shelter_disaster_evacuate",
        "domains": [
            "disaster",
            "evacuation",
            "shelter",
            "security"
        ],
        "situations": [
            "earthquake",
            "flood",
            "structural_risk",
            "evacuation_decision"
        ],
        "objects": [
            "building",
            "route",
            "go_bag"
        ],
        "must_any": [
            "地震",
            "裂缝",
            "暴雨",
            "水位",
            "撤到高处",
            "撤高",
            "撤离",
            "洪水",
            "山洪"
        ],
        "keywords": [
            "地震",
            "裂缝",
            "暴雨",
            "水位",
            "上涨",
            "撤到高处",
            "撤高",
            "撤离",
            "洪水",
            "山洪"
        ],
        "preferred_titles": [
            "洪水来临前",
            "暴雨山洪",
            "是否撤离判断",
            "撤离前",
            "住所不可继续停留"
        ],
        "bad_title_words": [
            "保温",
            "潮湿空气",
            "天气突变前兆"
        ],
        "priority": 88
    },
    {
        "intent": "water_rationing",
        "domains": [
            "water"
        ],
        "situations": [
            "water_outage",
            "scarcity"
        ],
        "objects": [
            "drinking_water",
            "bucket",
            "container"
        ],
        "must_any": [
            "停水",
            "缺水",
            "几桶水",
            "饮用水",
            "储水",
            "分配",
            "用水"
        ],
        "keywords": [
            "停水",
            "缺水",
            "饮用水",
            "水桶",
            "储水",
            "分配",
            "用水",
            "水源"
        ],
        "preferred_titles": [
            "大规模停水",
            "缺水时用途排序",
            "饮用水容器",
            "紧急配水"
        ],
        "bad_title_words": [],
        "priority": 90
    },
    {
        "intent": "water_treatment",
        "domains": [
            "water",
            "hygiene"
        ],
        "situations": [
            "unsafe_water",
            "rainwater",
            "turbid_water"
        ],
        "objects": [
            "rainwater",
            "filter",
            "boiling",
            "chlorine"
        ],
        "must_any": [
            "雨水",
            "浑浊水",
            "直接喝",
            "净水",
            "过滤",
            "煮沸",
            "消毒",
            "水质"
        ],
        "keywords": [
            "雨水",
            "浑浊水",
            "直接喝",
            "净水",
            "过滤",
            "煮沸",
            "消毒",
            "水质",
            "水源"
        ],
        "preferred_titles": [
            "雨水收集",
            "水源风险分级",
            "浑浊水处理",
            "煮沸净水"
        ],
        "bad_title_words": [],
        "priority": 88
    },
    {
        "intent": "medical_fever_respiratory",
        "domains": [
            "medical",
            "water"
        ],
        "situations": [
            "fever",
            "respiratory_symptom"
        ],
        "objects": [
            "patient",
            "temperature",
            "drinking_water"
        ],
        "must_any": [
            "发烧",
            "发热",
            "咳嗽",
            "体温",
            "病人"
        ],
        "keywords": [
            "发烧",
            "发热",
            "咳嗽",
            "体温",
            "喝水少",
            "补水",
            "病人"
        ],
        "preferred_titles": [
            "咳嗽发热",
            "发热",
            "症状时间线",
            "疑似传染"
        ],
        "bad_title_words": [],
        "priority": 92
    },
    {
        "intent": "medical_wound_bleeding",
        "domains": [
            "medical",
            "hygiene",
            "tools"
        ],
        "situations": [
            "wound",
            "bleeding"
        ],
        "objects": [
            "wound",
            "glass",
            "bandage"
        ],
        "must_any": [
            "伤口",
            "划破",
            "玻璃",
            "出血",
            "止血"
        ],
        "keywords": [
            "伤口",
            "划破",
            "玻璃",
            "出血",
            "止血",
            "血",
            "敷料",
            "压迫"
        ],
        "preferred_titles": [
            "直接压迫止血",
            "严重出血",
            "小伤口",
            "伤者初筛"
        ],
        "bad_title_words": [],
        "priority": 92
    },
    {
        "intent": "medical_elder_confusion",
        "domains": [
            "medical",
            "records"
        ],
        "situations": [
            "elderly",
            "consciousness_change"
        ],
        "objects": [
            "elder",
            "medicine",
            "temperature"
        ],
        "must_any": [
            "老人",
            "意识混乱",
            "走路不稳",
            "糊涂",
            "嗜睡"
        ],
        "keywords": [
            "老人",
            "意识混乱",
            "走路不稳",
            "糊涂",
            "嗜睡",
            "突然",
            "用药"
        ],
        "preferred_titles": [
            "老人意识混乱",
            "老人药物",
            "老人夜间如厕"
        ],
        "bad_title_words": [],
        "priority": 88
    },
    {
        "intent": "hygiene_contamination",
        "domains": [
            "hygiene",
            "medical",
            "water"
        ],
        "situations": [
            "contamination",
            "infection_control"
        ],
        "objects": [
            "vomit",
            "diarrhea",
            "clothes",
            "floor",
            "toilet"
        ],
        "must_any": [
            "呕吐",
            "腹泻",
            "弄脏",
            "污染",
            "厕所",
            "不能冲水",
            "排泄"
        ],
        "keywords": [
            "呕吐",
            "腹泻",
            "弄脏",
            "污染",
            "地面",
            "衣服",
            "衣物",
            "厕所",
            "洗手",
            "消毒"
        ],
        "preferred_titles": [
            "呕吐物和腹泻污染",
            "腹泻呕吐隔离",
            "厕所替代方案",
            "洗手关键"
        ],
        "bad_title_words": [],
        "priority": 86
    },
    {
        "intent": "comms_signal_meeting",
        "domains": [
            "comms",
            "security",
            "records",
            "evacuation"
        ],
        "situations": [
            "lost_contact",
            "signal_plan"
        ],
        "objects": [
            "signal",
            "meeting_point",
            "message"
        ],
        "must_any": [
            "通信",
            "通讯",
            "没回来",
            "约定信号",
            "走散",
            "留言",
            "集合点",
            "口令"
        ],
        "keywords": [
            "通信",
            "通讯",
            "没回来",
            "约定信号",
            "走散",
            "留言",
            "集合点",
            "口令",
            "求救",
            "失联"
        ],
        "preferred_titles": [
            "失联人员最后位置",
            "集合点三层规则",
            "留言点安全",
            "求救信号"
        ],
        "bad_title_words": [],
        "priority": 84
    },
    {
        "intent": "tools_security_repair",
        "domains": [
            "tools",
            "security",
            "shelter"
        ],
        "situations": [
            "temporary_repair",
            "door_window_security"
        ],
        "objects": [
            "door",
            "window",
            "lock",
            "tools"
        ],
        "must_any": [
            "门锁",
            "门窗",
            "加固",
            "固定",
            "工具"
        ],
        "keywords": [
            "门锁",
            "门窗",
            "松",
            "加固",
            "固定",
            "工具",
            "胶带",
            "绳子"
        ],
        "preferred_titles": [
            "门窗临时加固",
            "门窗低暴露",
            "门窗静音"
        ],
        "bad_title_words": [],
        "priority": 84
    },
    {
        "intent": "planting_seed_reserve",
        "domains": [
            "planting",
            "food",
            "water"
        ],
        "situations": [
            "seed_saving",
            "food_production"
        ],
        "objects": [
            "seed",
            "beans",
            "crop"
        ],
        "must_any": [
            "种子",
            "豆子",
            "留种",
            "发芽",
            "种植"
        ],
        "keywords": [
            "种子",
            "豆子",
            "留种",
            "发芽",
            "哪些可以吃",
            "种植",
            "播种"
        ],
        "preferred_titles": [
            "留种粮",
            "种子库存",
            "种子发芽",
            "种子保存"
        ],
        "bad_title_words": [],
        "priority": 82
    },
    {
        "intent": "records_inventory_people",
        "domains": [
            "records"
        ],
        "situations": [
            "inventory_change",
            "personnel_change"
        ],
        "objects": [
            "inventory",
            "people",
            "log"
        ],
        "must_any": [
            "记录",
            "物资",
            "人员",
            "变化",
            "清单",
            "登记",
            "日志"
        ],
        "keywords": [
            "记录",
            "物资",
            "人员",
            "变化",
            "清单",
            "登记",
            "日志",
            "档案"
        ],
        "preferred_titles": [
            "记录",
            "清单",
            "盘点",
            "登记",
            "共同决定记录"
        ],
        "bad_title_words": [],
        "priority": 78
    }
]

SITUATION_WORDS = {
    "power_outage": [
        "停电",
        "断电",
        "没电"
    ],
    "refrigeration_failure": [
        "冰箱",
        "冷藏",
        "冷冻"
    ],
    "food_spoilage": [
        "变质",
        "异味",
        "黏滑",
        "鼓包",
        "腐败"
    ],
    "water_outage": [
        "停水",
        "缺水"
    ],
    "scarcity": [
        "只剩",
        "不足",
        "配给"
    ],
    "unsafe_water": [
        "浑浊水",
        "雨水",
        "水质",
        "污染水",
        "化学味",
        "油膜"
    ],
    "fever": [
        "发热",
        "发烧",
        "体温"
    ],
    "respiratory_symptom": [
        "咳嗽",
        "呼吸困难"
    ],
    "wound": [
        "伤口",
        "划破",
        "割伤",
        "擦伤"
    ],
    "bleeding": [
        "出血",
        "止血",
        "血"
    ],
    "elderly": [
        "老人"
    ],
    "consciousness_change": [
        "意识",
        "糊涂",
        "嗜睡",
        "昏迷",
        "走路不稳"
    ],
    "contamination": [
        "污染",
        "污水",
        "呕吐物",
        "垃圾",
        "弄脏"
    ],
    "infection_control": [
        "洗手",
        "消毒",
        "隔离",
        "餐具"
    ],
    "earthquake": [
        "地震",
        "余震"
    ],
    "flood": [
        "洪水",
        "暴雨",
        "水位",
        "内涝",
        "山洪"
    ],
    "structural_risk": [
        "裂缝",
        "坍塌",
        "结构"
    ],
    "evacuation_decision": [
        "撤离",
        "离开",
        "撤高",
        "撤到高处"
    ],
    "lost_contact": [
        "没回来",
        "走散",
        "失联"
    ],
    "signal_plan": [
        "信号",
        "口令",
        "集合点",
        "留言"
    ],
    "temporary_repair": [
        "临时",
        "修理",
        "维修",
        "加固"
    ],
    "door_window_security": [
        "门锁",
        "门窗",
        "门外",
        "窗外"
    ],
    "seed_saving": [
        "留种",
        "种子"
    ],
    "food_production": [
        "种植",
        "播种",
        "阳台种菜",
        "堆肥"
    ],
    "inventory_change": [
        "库存",
        "物资",
        "清单"
    ],
    "personnel_change": [
        "人员",
        "成员"
    ],
    "night": [
        "夜间",
        "晚上",
        "黑夜"
    ],
    "external_anomaly": [
        "异响",
        "响动",
        "脚步",
        "外面",
        "门外",
        "窗外"
    ],
    "suspicious_approach": [
        "陌生人",
        "可疑",
        "靠近",
        "反复"
    ],
    "resource_exposure": [
        "资源暴露",
        "物资",
        "暴露"
    ],
    "toilet_route": ["夜间厕所", "厕所路线", "如厕", "太黑"],
    "fall_risk": ["摔倒", "跌倒", "绊倒"],
    "cold_exposure": ["冷", "手脚冰凉", "保暖", "寒冷"],
    "hypothermia_watch": ["失温", "发抖", "手脚冰凉"],
    "diarrhea": ["腹泻", "拉肚子"],
    "dehydration": ["脱水", "喝不下", "水也喝不下", "尿少"],
    "fracture": ["骨折", "夹板"],
    "temporary_fix": ["临时固定", "固定", "夹板"],
    "damp": ["潮", "潮湿", "屋里很潮"],
    "mold": ["发霉", "霉", "墙角发霉"],
    "water_contact_electric": ["进过水", "进水", "水泡", "漏电"],
    "power_restore": ["恢复供电", "通电", "继续用"],
}
OBJECT_WORDS = {
    "fridge": [
        "冰箱",
        "冷藏",
        "冷冻"
    ],
    "meat": [
        "肉",
        "肉类",
        "海鲜"
    ],
    "leftovers": [
        "剩饭",
        "剩菜",
        "剩食"
    ],
    "cooked_food": [
        "熟食",
        "热食",
        "汤"
    ],
    "canned_food": [
        "罐头",
        "鼓包"
    ],
    "drinking_water": [
        "饮用水",
        "水"
    ],
    "bucket": [
        "水桶",
        "桶"
    ],
    "rainwater": [
        "雨水"
    ],
    "filter": [
        "过滤",
        "滤布"
    ],
    "boiling": [
        "煮沸",
        "烧开"
    ],
    "patient": [
        "病人",
        "患者",
        "伤者"
    ],
    "temperature": [
        "体温",
        "体温计"
    ],
    "wound": [
        "伤口"
    ],
    "glass": [
        "玻璃"
    ],
    "bandage": [
        "绷带",
        "敷料",
        "纱布"
    ],
    "elder": [
        "老人"
    ],
    "medicine": [
        "药",
        "药品"
    ],
    "vomit": [
        "呕吐",
        "呕吐物"
    ],
    "diarrhea": [
        "腹泻"
    ],
    "clothes": [
        "衣服",
        "衣物"
    ],
    "floor": [
        "地面"
    ],
    "toilet": [
        "厕所",
        "桶厕"
    ],
    "building": [
        "房屋",
        "住所",
        "建筑",
        "墙"
    ],
    "route": [
        "路线",
        "道路"
    ],
    "signal": [
        "信号",
        "哨子",
        "电台"
    ],
    "meeting_point": [
        "集合点"
    ],
    "message": [
        "留言",
        "纸条"
    ],
    "door": [
        "门",
        "门锁",
        "门外"
    ],
    "window": [
        "窗",
        "窗外",
        "门窗"
    ],
    "lock": [
        "锁",
        "门锁"
    ],
    "tools": [
        "工具",
        "胶带",
        "绳子",
        "螺丝刀"
    ],
    "seed": [
        "种子",
        "留种"
    ],
    "beans": [
        "豆",
        "豆子",
        "豆类"
    ],
    "inventory": [
        "库存",
        "物资",
        "清单"
    ],
    "people": [
        "人员",
        "成员",
        "老人",
        "儿童"
    ],
    "log": [
        "记录",
        "日志"
    ],
    "outside_noise": [
        "异响",
        "响动",
        "声音",
        "脚步"
    ],
    "person": [
        "陌生人",
        "可疑人员",
        "人员"
    ],
    "supplies": [
        "物资",
        "资源"
    ],
    "flashlight": [
        "手电",
        "头灯",
        "低亮灯"
    ],
    "power_bank": [
        "充电宝",
        "移动电源"
    ],
    "battery": [
        "电池",
        "电量"
    ],
    "lighting": [
        "照明",
        "灯"
    ],
    "sleeping_area": ["睡眠区", "床", "房间"],
    "splint": ["夹板", "木板", "硬纸板"],
    "power_strip": ["插线板", "插座"],
    "wire": ["电线", "线缆"],
    "device": ["设备", "电器"],
}


def _term_score(message: str, guide: Dict[str, Any]) -> int:
    raw = safe_text(message)

    terms = [
        item.strip()
        for item in re.split(
            r"[\s，。！？、；：,.!?;:（）()【】\[\]《》<>\"']+",
            str(message or ""),
        )
        if len(item.strip()) >= 2
    ]

    for source in (
        list(DOMAIN_KEYWORDS.values())
        + list(SITUATION_WORDS.values())
        + list(OBJECT_WORDS.values())
    ):
        for word in source:
            word = str(word).lower().strip()
            if len(word) >= 2 and word in raw and word not in terms:
                terms.append(word)

    title = safe_text(guide.get("title"))
    scenario_goal = safe_text([
        guide.get("scenario"),
        guide.get("goal"),
        guide.get("summary"),
    ])
    keywords = safe_text(guide.get("keywords"))
    category = safe_text([
        guide.get("category"),
        guide.get("category_original"),
    ])
    core = guide_core_text(guide)
    full = guide_full_text(guide)

    score = 0

    for term in terms[:40]:
        if term in title:
            score += 14
        elif term in keywords:
            score += 9
        elif term in scenario_goal:
            score += 6
        elif term in category:
            score += 4
        elif term in core:
            score += 2
        elif term in full:
            score += 1

    return score


def _title_priority_score(guide: Dict[str, Any], query_profile: Dict[str, Any]) -> int:
    title = safe_text(guide.get("title"))
    score = 0
    for rule in query_profile.get("intent_rules", []):
        for good in rule.get("preferred_titles", []):
            if safe_text(good) in title:
                score += 60
        for bad in rule.get("bad_title_words", []):
            if safe_text(bad) in title:
                score -= 80
    return score


def _match_concepts(text: str, rules: Dict[str, List[str]]) -> List[str]:
    return [key for key, words in rules.items() if contains_any(text, words)]


def _score_intent(text: str, rule: Dict[str, Any]) -> int:
    intent = rule.get("intent")

    # 语义否定与场景互斥：避免“关键词撞车”。
    if intent == "food_safety_judgment":
        # v0.6.5：显式否定冰箱/食物线索时，不要让“停电 + 冰箱先不管”误入食物安全。
        if contains_any(text, ["冰箱先不管", "先不管冰箱", "冰箱暂时不管", "暂时不管冰箱"]):
            return 0
        if contains_any(text, ["手电", "充电宝", "照明", "充电"]) and not contains_any(text, ["肉", "剩饭", "剩菜", "熟食", "能不能吃", "变质", "鼓包", "冷藏", "冷冻"]):
            return 0
        if contains_any(text, ["冰箱先不管", "冰箱暂时不管"]) and contains_any(text, ["手电", "充电宝", "照明", "充电"]):
            return 0

    if intent == "power_lighting_energy":
        if contains_any(text, ["冰箱", "肉", "剩饭", "剩菜", "能不能吃"]) and not contains_any(text, ["手电", "充电宝", "照明", "充电"]):
            return 0

    if intent == "shelter_disaster_evacuate":
        if contains_any(text, ["水位没有上涨", "水位没上涨", "没有上涨"]) and contains_any(text, ["潮", "潮湿", "发霉", "墙角", "屋里"]):
            return 0

    if intent == "medical_elder_confusion":
        if contains_any(text, ["意识清楚"]) and not contains_any(text, ["糊涂", "说话不清", "说话异常", "走路不稳", "意识混乱"]):
            return 0

    if intent == "hygiene_contamination":
        if contains_any(text, ["厕所"]) and contains_any(text, ["晚上", "夜间", "太黑", "摔倒"]) and not contains_any(text, ["污染", "呕吐", "腹泻", "不能冲水", "气味", "排泄"]):
            return 0

    if intent == "tools_security_repair":
        if contains_any(text, ["骨折", "夹板"]):
            return 0

    if rule.get("must_any") and not contains_any(text, rule.get("must_any", [])):
        return 0

    score = 0
    for word in rule.get("must_any", []):
        if str(word).lower().strip() in text:
            score += 4
    for word in rule.get("keywords", []):
        if str(word).lower().strip() in text:
            score += 2
    for sit in rule.get("situations", []):
        if contains_any(text, SITUATION_WORDS.get(sit, [])):
            score += 2
    for obj in rule.get("objects", []):
        if contains_any(text, OBJECT_WORDS.get(obj, [])):
            score += 2
    return score


def _list_overlap_score(left: Any, right: Any, weight: int = 1) -> int:
    left_items = set(unique_list(left if isinstance(left, list) else [left]))
    right_items = set(unique_list(right if isinstance(right, list) else [right]))

    if not left_items or not right_items:
        return 0

    return len(left_items & right_items) * weight


def analyze_query(
    user_message: str,
    analysis_context=None,
) -> Dict[str, Any]:
    text = safe_text(user_message)
    matched = []
    analysis_intents = []

    if analysis_context:
        analysis_intents = list(getattr(analysis_context, "intents", []))

    for rule in QUERY_INTENT_RULES:
        score = _score_intent(text, rule)
        if score >= 8:
            item = dict(rule)
            item["_score"] = score
            matched.append(item)

    matched.sort(key=lambda item: (item.get("priority", 0), item.get("_score", 0)), reverse=True)

    domains = []
    for item in matched:
        domains.extend(item.get("domains", []))

    if not domains:
        domain_scores = {}
        for domain, words in DOMAIN_KEYWORDS.items():
            for word in words:
                if str(word).lower().strip() in text:
                    domain_scores[domain] = domain_scores.get(domain, 0) + 1
        domains.extend([d for d, _s in sorted(domain_scores.items(), key=lambda pair: pair[1], reverse=True)])

    query_intents = unique_list(
        analysis_intents +
        [item.get("intent") for item in matched[:6]]
    )    

    return {
        "domains": unique_list(domains)[:5],
        "intents": query_intents,
        "situations": _match_concepts(text, SITUATION_WORDS),
        "objects": _match_concepts(text, OBJECT_WORDS),
        "intent_rules": matched[:6],
    }

def build_query_profile_from_strategy(
    strategy: Dict[str, Any],
    context=None,
) -> Dict[str, Any]:

    guide_filters = strategy.get("guide_filters", {})

    return {
        "domains": guide_filters.get("domains", []),
        "intents": guide_filters.get("tasks", []),
        "situations": guide_filters.get("situations", []),
        "objects": guide_filters.get("objects", []),
        "signals": strategy.get("signals", []),
        "risks": strategy.get("risks", []),
        "retrieval_plan": strategy.get("retrieval_plan", []),
        "analysis": context,
        "intent_rules": [],
    }

def score_guide_for_message(
    guide: Dict[str, Any],
    message: str,
    detected_domains: List[str],
    strategy: Dict[str, Any],
) -> int:
    query_profile = build_query_profile_from_strategy(strategy)
    query_domains = query_profile.get("domains", detected_domains)

    if query_domains and not guide_compatible_with_domains(guide, query_domains):
        return -100

    title_score = _title_priority_score(guide, query_profile)

    intent_score = _list_overlap_score(
        query_profile.get("intents", []),
        guide.get("intents"),
        34,
    )

    situation_score = _list_overlap_score(
        query_profile.get("situations", []),
        guide.get("situations"),
        18,
    )

    object_score = _list_overlap_score(
        query_profile.get("objects", []),
        guide.get("objects"),
        14,
    )

    term_score = _term_score(message, guide)

    score = (
        title_score
        + intent_score
        + situation_score
        + object_score
        + term_score
    )

    guide_domain_set = set(guide_domains(guide))
    for index, domain in enumerate(query_domains):
        if domain in guide_domain_set:
            score += 14 if index == 0 else 8
        elif guide_compatible_with_domains(guide, [domain]):
            score += 2

    if query_profile.get("intents") and score < 22:
        return -100

    return score


def build_match_reason(guide: Dict[str, Any], query_profile: Dict[str, Any]) -> str:
    parts = []
    for label, q_key, g_key in [
        ("意图匹配", "intents", "intents"),
        ("场景匹配", "situations", "situations"),
        ("对象匹配", "objects", "objects"),
    ]:
        overlap = set(query_profile.get(q_key, [])) & set(guide.get(g_key) or [])
        if overlap:
            parts.append(label + "：" + "、".join(sorted(overlap)[:2]))
    if not parts:
        overlap = set(query_profile.get("domains", [])) & set(guide_domains(guide))
        if overlap:
            parts.append("领域匹配：" + "、".join(sorted(overlap)[:2]))
    return "；".join(parts) if parts else "结构化召回匹配"


def find_guides_by_message_and_domains(
    message: str,
    strategy: Dict[str, Any],
    guides: List[Dict[str, Any]],
    min_score: int = 8,
) -> List[Dict[str, Any]]:
    query_profile = build_query_profile_from_strategy(strategy)
    detected_domains = query_profile["domains"]

    effective_min_score = max(
        min_score,
        22 if query_profile.get("intents") else 8,
    )

    scored = []

    for guide in guides:
        score = score_guide_for_message(
            guide,
            message,
            detected_domains,
            strategy=strategy,
        )

        if score >= effective_min_score:
            item = dict(guide)
            item["_match_score"] = score
            item["_match_reason"] = build_match_reason(item, query_profile)
            scored.append(item)

    scored.sort(
        key=lambda item: item.get("_match_score", 0),
        reverse=True,
    )

    return scored

def find_domain_fallback_guides(domains: List[str], guides: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    if not domains:
        return []
    result = []
    seen = set()
    for guide in guides:
        guide_id = guide.get("id") or guide.get("title")
        if guide_id in seen:
            continue
        if guide_compatible_with_domains(guide, domains):
            result.append(guide)
            seen.add(guide_id)
    return result
