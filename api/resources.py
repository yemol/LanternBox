import json
from typing import Any, Dict, List
import re

from .config import (
    DATA_DIR,
    EMERGENCY_GUIDES_PATH,
    EMERGENCY_GUIDES_FILE,
    DOMAIN_FALLBACK_GUIDES,
    DOMAIN_KEYWORDS,
    GUIDES_CACHE,
    RESOURCE_CACHE_INFO,
    TRIGGERS_CACHE,
    DOMAIN_NEGATIVE_KEYWORDS,
)
from .utils import get_severity_weight, read_json_file


def load_local_resources() -> None:
    guides_path = DATA_DIR / "emergency_guides.json"
    triggers_path = DATA_DIR / "ai_triggers.json"

    GUIDES_CACHE.clear()
    GUIDES_CACHE.extend(read_json_file(guides_path, []))
    TRIGGERS_CACHE.clear()
    TRIGGERS_CACHE.extend(read_json_file(triggers_path, []))

    RESOURCE_CACHE_INFO.update({
        "emergency_guides_count": len(GUIDES_CACHE),
        "ai_triggers_count": len(TRIGGERS_CACHE),
        "loaded": True,
        "guides_path": str(guides_path),
        "triggers_path": str(triggers_path),
    })

    # print("本地资源缓存已加载：", RESOURCE_CACHE_INFO)


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
                if title == expected:
                    add_guide(guide)
                    break

                if expected in title or title in expected:
                    add_guide(guide)
                    break

                expected_chars = set(expected)
                title_chars = set(title)

                if expected_chars and title_chars:
                    overlap = len(expected_chars & title_chars)
                    ratio = overlap / max(len(expected_chars), 1)

                    if ratio >= 0.45:
                        add_guide(guide)
                        break

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
                f"建议动作：{'；'.join(suggested_actions[:10])}",
                f"后续追踪：{'；'.join(follow_up[:3])}",
            ])
        )

    guide_blocks = []
    for guide in related_guides[:10]:
        steps = guide.get("steps", [])
        stop_or_escalate = guide.get("stop_or_escalate", [])

        guide_blocks.append(
            "\n".join([
                f"指南标题：{guide.get('title', '')}",
                f"分类：{guide.get('category', '')}",
                f"适用场景：{guide.get('scenario', '')}",
                f"目标：{guide.get('goal', '')}",
                f"关键步骤：{'；'.join(steps[:10])}",
                f"停止或升级：{'；'.join(stop_or_escalate[:3])}",
            ])
        )

    return {
        "trigger_text": "\n\n".join(trigger_blocks),
        "guide_text": "\n\n".join(guide_blocks),
    }


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
    matched_triggers = match_triggers(user_message, triggers)[:10]

    trigger_guides = find_related_guides(matched_triggers, guides)
    domain_guides = find_domain_fallback_guides(detected_domains, guides)
    keyword_guides = find_guides_by_domain_keywords(detected_domains, guides)
    scored_guides = find_guides_by_message_and_domains(
        message=user_message,
        detected_domains=detected_domains,
        guides=guides,
    )

    related_guides = merge_guides(
        trigger_guides,
        domain_guides,
        keyword_guides,
    )[:10]

    # print("detected_domains:", detected_domains)
    # print("trigger_guides:", [g.get("title") for g in trigger_guides])
    # print("domain_guides:", [g.get("title") for g in domain_guides])
    # print("scored_guides:", [(g.get("title"), g.get("_match_score")) for g in scored_guides[:10]])

    return {
        "detected_domains": detected_domains,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
    }

def build_guide_search_text(guide: Dict[str, Any]) -> str:
    parts = [
        guide.get("id", ""),
        guide.get("title", ""),
        guide.get("category", ""),
        guide.get("category_original", ""),
        guide.get("scenario", ""),
        guide.get("goal", ""),
        guide.get("fallback", ""),
        guide.get("notes", ""),
        guide.get("observe", ""),
    ]

    for key in [
        "tools",
        "steps",
        "check",
        "common_mistakes",
        "stop_or_escalate",
        "do_first",
        "avoid",
        "items",
    ]:
        value = guide.get(key)
        if isinstance(value, list):
            parts.extend(str(item) for item in value if item)
        elif isinstance(value, str):
            parts.append(value)

    return " ".join(str(part) for part in parts if part)


def score_guide_for_message(
    guide: Dict[str, Any],
    message: str,
    detected_domains: List[str],
) -> int:
    score = 0

    message = message.strip()
    title = str(guide.get("title", ""))
    category = str(guide.get("category", ""))
    scenario = str(guide.get("scenario", ""))
    goal = str(guide.get("goal", ""))
    search_text = build_guide_search_text(guide)

    # 用户原文直接命中标题，最高价值
    if message and message in title:
        score += 8

    # 标题中的关键词出现在用户描述中
    for token in extract_simple_keywords(title):
        if token and token in message:
            score += 5

    # 用户描述里的关键词出现在指南标题、场景或目标中
    for token in extract_simple_keywords(message):
        if not token:
            continue

        if token in title:
            score += 5
        elif token in category:
            score += 4
        elif token in scenario or token in goal:
            score += 3
        elif token in search_text:
            score += 1

    # 领域关键词命中
    for domain in detected_domains:
        domain_keywords = DOMAIN_KEYWORDS.get(domain, [])

        for keyword in domain_keywords:
            if not keyword:
                continue

            if keyword in title:
                score += 4
            elif keyword in category:
                score += 3
            elif keyword in scenario or keyword in goal:
                score += 2
            elif keyword in search_text:
                score += 1

    return score


def extract_simple_keywords(text: str) -> List[str]:
    text = str(text or "").strip()

    stop_words = {
        "我", "你", "他", "她", "它", "我们", "你们", "他们",
        "现在", "今天", "感觉", "好像", "有点", "一个", "一下",
        "怎么", "怎么办", "需要", "可以", "应该", "是不是",
        "了", "的", "吗", "呢", "吧", "啊",
        "使用", "检查", "处理", "情况", "问题", "不足", "不多", "还有",
        "当前", "相关", "指南", "操作", "步骤", "工具", "注意"
    }

    # 先按常见标点和空格切
    raw_parts = re.split(r"[\s，。！？、；：,.!?;:（）()【】\[\]《》<>\"']+", text)

    keywords = []

    for part in raw_parts:
        part = part.strip()
        if not part or part in stop_words:
            continue

        # 太短的泛词跳过，但保留“水”“电”“药”这种关键字
        if len(part) == 1 and part not in {"水", "电", "药", "火", "冷", "热"}:
            continue

        keywords.append(part)

    return keywords[:20]

def find_guides_by_message_and_domains(
    message: str,
    detected_domains: List[str],
    guides: List[Dict[str, Any]],
    min_score: int = 5,
) -> List[Dict[str, Any]]:
    scored_guides = []

    for guide in guides:
        if detected_domains and not guide_matches_any_domain(guide, detected_domains):
            continue
        if detected_domains and guide_has_negative_domain_keywords(guide, detected_domains):
            continue

        score = score_guide_for_message(
            guide=guide,
            message=message,
            detected_domains=detected_domains,
        )

        if score >= min_score:
            item = dict(guide)
            item["_match_score"] = score
            scored_guides.append(item)

        scored_guides.sort(
            key=lambda item: item.get("_match_score", 0),
            reverse=True,
        )

    return scored_guides

def guide_matches_any_domain(
    guide: Dict[str, Any],
    detected_domains: List[str],
) -> bool:
    if not detected_domains:
        return True

    search_text = build_guide_search_text(guide)

    for domain in detected_domains:
        keywords = DOMAIN_KEYWORDS.get(domain, [])

        if any(keyword and keyword in search_text for keyword in keywords):
            return True

    return False


def guide_has_negative_domain_keywords(
    guide: Dict[str, Any],
    detected_domains: List[str],
) -> bool:
    search_text = build_guide_search_text(guide)

    for domain in detected_domains:
        negative_keywords = DOMAIN_NEGATIVE_KEYWORDS.get(domain, [])
        if any(keyword and keyword in search_text for keyword in negative_keywords):
            return True

    return False

# -----------------------------------------------------------------------------
# LanternBox resources v0.6.2
# 意图路由收敛版：强意图优先、标题白名单加权、弱相关降权。
# -----------------------------------------------------------------------------

DOMAIN_COMPATIBILITY = {k: set(v) for k, v in {
    "security": [
        "security",
        "shelter",
        "tools",
        "comms",
        "power",
        "evacuation",
        "disaster",
        "records"
    ],
    "medical": [
        "medical",
        "hygiene",
        "water",
        "food",
        "tools",
        "records"
    ],
    "water": [
        "water",
        "hygiene",
        "medical",
        "food",
        "disaster",
        "evacuation"
    ],
    "food": [
        "food",
        "water",
        "hygiene",
        "medical",
        "power",
        "planting"
    ],
    "power": [
        "power",
        "tools",
        "comms",
        "shelter",
        "security",
        "food",
        "disaster"
    ],
    "hygiene": [
        "hygiene",
        "water",
        "medical",
        "shelter",
        "food",
        "tools"
    ],
    "tools": [
        "tools",
        "power",
        "shelter",
        "security",
        "comms",
        "medical"
    ],
    "comms": [
        "comms",
        "power",
        "security",
        "records",
        "evacuation"
    ],
    "shelter": [
        "shelter",
        "security",
        "power",
        "hygiene",
        "tools",
        "disaster",
        "evacuation"
    ],
    "evacuation": [
        "evacuation",
        "security",
        "medical",
        "shelter",
        "comms",
        "disaster",
        "water",
        "records"
    ],
    "disaster": [
        "disaster",
        "security",
        "shelter",
        "evacuation",
        "medical",
        "water",
        "power",
        "hygiene"
    ],
    "records": [
        "records",
        "comms",
        "security",
        "medical",
        "evacuation"
    ],
    "planting": [
        "planting",
        "food",
        "water",
        "hygiene"
    ]
}.items()}

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
        "intent": "food_safety_judgment",
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
    ]
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
    ]
}

DOMAIN_KEYWORDS = {
    "security": ["安全", "风险", "异响", "脚步", "门外", "窗外", "外面", "夜间", "可疑", "陌生人", "资源暴露", "门窗"],
    "food": ["食物", "冰箱", "剩饭", "剩菜", "肉", "罐头", "主食", "配给", "变质", "能不能吃"],
    "water": ["停水", "缺水", "饮用水", "雨水", "净水", "水质", "过滤", "煮沸", "消毒"],
    "medical": ["发热", "发烧", "咳嗽", "腹泻", "呕吐", "伤口", "出血", "止血", "老人", "意识"],
    "power": ["停电", "断电", "电量", "充电", "充电宝", "照明", "手电", "太阳能", "发电机"],
    "hygiene": ["卫生", "清洁", "洗手", "厕所", "污染", "呕吐物", "衣物", "垃圾", "消毒"],
    "tools": ["工具", "维修", "加固", "固定", "门锁", "门窗", "胶带", "螺丝刀"],
    "comms": ["通信", "通讯", "信号", "集合点", "留言", "口令", "求救"],
    "evacuation": ["撤离", "路线", "集合点", "撤高", "走散", "离开"],
    "disaster": ["地震", "洪水", "暴雨", "水位", "山洪", "火灾", "余震"],
    "records": ["记录", "日志", "清单", "登记", "物资", "人员"],
    "planting": ["种子", "留种", "发芽", "种植"],
    "shelter": ["住所", "睡眠", "房间", "保暖", "降温", "门窗"],
}


def _safe_text(value: Any) -> str:
    if isinstance(value, list):
        return " ".join(_safe_text(item) for item in value)
    if isinstance(value, dict):
        return " ".join(_safe_text(v) for v in value.values())
    return str(value or "").lower().replace("\n", " ")


def _contains_any(text: str, words: List[str]) -> bool:
    return any(str(word).lower().strip() in text for word in words if len(str(word).strip()) >= 2)


def _unique(items: List[str]) -> List[str]:
    result = []
    for item in items:
        item = str(item or "").lower().strip()
        if item and item not in result:
            result.append(item)
    return result


def _match_concepts(text: str, rules: Dict[str, List[str]]) -> List[str]:
    return [key for key, words in rules.items() if _contains_any(text, words)]


def _score_intent(text: str, rule: Dict[str, Any]) -> int:
    if rule.get("must_any") and not _contains_any(text, rule.get("must_any", [])):
        return 0

    score = 0
    for word in rule.get("must_any", []):
        if str(word).lower().strip() in text:
            score += 4
    for word in rule.get("keywords", []):
        if str(word).lower().strip() in text:
            score += 2
    for sit in rule.get("situations", []):
        if _contains_any(text, SITUATION_WORDS.get(sit, [])):
            score += 2
    for obj in rule.get("objects", []):
        if _contains_any(text, OBJECT_WORDS.get(obj, [])):
            score += 2
    return score


def analyze_query(user_message: str) -> Dict[str, Any]:
    text = _safe_text(user_message)
    matched = []

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

    return {
        "domains": _unique(domains)[:5],
        "intents": [item.get("intent") for item in matched[:4]],
        "situations": _match_concepts(text, SITUATION_WORDS),
        "objects": _match_concepts(text, OBJECT_WORDS),
        "intent_rules": matched[:4],
    }


def detect_domains(text: str) -> List[str]:
    return analyze_query(text).get("domains", [])


def guide_core_text(guide: Dict[str, Any]) -> str:
    # 核心文本不包含 negative_keywords，避免负词反向污染正向召回。
    fields = [
        "id", "title", "category", "category_original", "scenario", "goal",
        "fallback", "notes", "summary", "source", "domains", "keywords",
        "situations", "objects", "intents",
    ]
    return " ".join(_safe_text(guide.get(key)) for key in fields)


def guide_full_text(guide: Dict[str, Any]) -> str:
    parts = [guide_core_text(guide)]
    for key in ["tools", "steps", "check", "common_mistakes", "stop_or_escalate", "do_first", "avoid", "items"]:
        parts.append(_safe_text(guide.get(key)))
    return " ".join(parts)


def guide_domains(guide: Dict[str, Any]) -> List[str]:
    domains = guide.get("domains")
    if isinstance(domains, list) and domains:
        return _unique([str(item) for item in domains])

    text = guide_core_text(guide)
    result = []
    for domain, words in DOMAIN_KEYWORDS.items():
        if _contains_any(text, words):
            result.append(domain)
    return result or ["general"]


def guide_compatible_with_domains(guide: Dict[str, Any], detected_domains: List[str]) -> bool:
    if not detected_domains:
        return True
    guide_set = set(guide_domains(guide))
    for domain in detected_domains:
        allowed = DOMAIN_COMPATIBILITY.get(domain, {domain})
        if guide_set & allowed:
            return True
    return False


def build_guide_search_text(guide: Dict[str, Any]) -> str:
    return guide_core_text(guide)


def _title_priority_score(guide: Dict[str, Any], query_profile: Dict[str, Any]) -> int:
    title = _safe_text(guide.get("title"))
    score = 0
    for rule in query_profile.get("intent_rules", []):
        for good in rule.get("preferred_titles", []):
            if _safe_text(good) in title:
                score += 60
        for bad in rule.get("bad_title_words", []):
            if _safe_text(bad) in title:
                score -= 80
    return score


def _overlap_score(query_items: List[str], guide_items: Any, weight: int) -> int:
    if not isinstance(guide_items, list):
        return 0
    guide_set = set(str(item).lower().strip() for item in guide_items if item)
    return sum(weight for item in query_items if str(item).lower().strip() in guide_set)


def _term_score(message: str, guide: Dict[str, Any]) -> int:
    raw = _safe_text(message)
    terms = [
        item.strip()
        for item in re.split(r"[\s，。！？、；：,.!?;:（）()【】\[\]《》<>\"']+", str(message or ""))
        if len(item.strip()) >= 2
    ]
    for source in list(DOMAIN_KEYWORDS.values()) + list(SITUATION_WORDS.values()) + list(OBJECT_WORDS.values()):
        for word in source:
            word = str(word).lower().strip()
            if len(word) >= 2 and word in raw and word not in terms:
                terms.append(word)

    title = _safe_text(guide.get("title"))
    scenario_goal = _safe_text([guide.get("scenario"), guide.get("goal"), guide.get("summary")])
    keywords = _safe_text(guide.get("keywords"))
    category = _safe_text([guide.get("category"), guide.get("category_original")])
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


def score_guide_for_message(
    guide: Dict[str, Any],
    message: str,
    detected_domains: List[str],
    query_profile: Dict[str, Any] = None,
) -> int:
    query_profile = query_profile or analyze_query(message)
    query_domains = query_profile.get("domains", detected_domains)

    if query_domains and not guide_compatible_with_domains(guide, query_domains):
        return -100

    score = 0
    score += _title_priority_score(guide, query_profile)
    score += _overlap_score(query_profile.get("intents", []), guide.get("intents"), 34)
    score += _overlap_score(query_profile.get("situations", []), guide.get("situations"), 18)
    score += _overlap_score(query_profile.get("objects", []), guide.get("objects"), 14)

    guide_domain_set = set(guide_domains(guide))
    for index, domain in enumerate(query_domains):
        if domain in guide_domain_set:
            score += 14 if index == 0 else 8
        elif guide_compatible_with_domains(guide, [domain]):
            score += 2

    score += _term_score(message, guide)

    # 强意图下，必须达到足够分数；这会清掉“同领域但题不对”的杂项。
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
    detected_domains: List[str],
    guides: List[Dict[str, Any]],
    min_score: int = 8,
    query_profile: Dict[str, Any] = None,
) -> List[Dict[str, Any]]:
    query_profile = query_profile or analyze_query(message)
    effective_min_score = max(min_score, 22 if query_profile.get("intents") else 8)
    scored = []
    for guide in guides:
        score = score_guide_for_message(guide, message, detected_domains, query_profile=query_profile)
        if score >= effective_min_score:
            item = dict(guide)
            item["_match_score"] = score
            item["_match_reason"] = build_match_reason(item, query_profile)
            scored.append(item)
    scored.sort(key=lambda item: item.get("_match_score", 0), reverse=True)
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


def find_guides_by_domain_keywords(domains: List[str], guides: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    if not domains:
        return []
    result = []
    for guide in guides:
        if guide_compatible_with_domains(guide, domains):
            item = dict(guide)
            item["_domain_score"] = len(set(guide_domains(guide)) & set(domains))
            result.append(item)
    result.sort(key=lambda item: item.get("_domain_score", 0), reverse=True)
    return result


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

    query_profile = analyze_query(user_message)
    detected_domains = query_profile.get("domains", [])
    matched_triggers = match_triggers(user_message, triggers)[:10]

    trigger_guides = []
    for guide in find_related_guides(matched_triggers, guides):
        score = score_guide_for_message(guide, user_message, detected_domains, query_profile=query_profile)
        if score >= 24:
            item = dict(guide)
            item["_match_score"] = score
            item["_match_reason"] = build_match_reason(item, query_profile)
            trigger_guides.append(item)

    scored_guides = find_guides_by_message_and_domains(
        message=user_message,
        detected_domains=detected_domains,
        guides=guides,
        min_score=8,
        query_profile=query_profile,
    )

    domain_guides = []
    if not scored_guides and detected_domains:
        domain_guides = find_domain_fallback_guides(detected_domains, guides)[:4]

    # v0.6.3：不要再让 trigger_guides 天然排在 scored_guides 前面。
    # 旧逻辑 merge_guides(trigger_guides, scored_guides, ...) 会让“停电”触发的电力指南
    # 压过“停电 + 冰箱 + 剩饭”的食物安全指南。
    # 现在统一合并后按 _match_score 排序，再截断。
    related_guides = merge_guides(scored_guides, trigger_guides, domain_guides)

    related_guides.sort(
        key=lambda item: item.get("_match_score", item.get("_domain_score", 0)),
        reverse=True,
    )

    related_guides = related_guides[:10]

    return {
        "detected_domains": detected_domains,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
        "query_profile": query_profile,
    }
