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
# LanternBox resources v0.5
# 结构化领域召回：召回阶段即使用 guide.domains / guide.keywords / guide.negative_keywords。
# 目标：从源头减少错召回，而不是在 AI 回答阶段补锅。
# -----------------------------------------------------------------------------

SAFE_DOMAIN_KEYWORDS = {
    "security": [
        "安全", "风险", "威胁", "危险", "异响", "响动", "脚步", "脚步声",
        "门外", "窗外", "外面", "门口", "院外", "夜间", "晚上", "黑夜",
        "陌生人", "可疑", "担心", "害怕", "守夜", "警戒", "巡查", "观察",
        "低暴露", "不要外出", "入侵", "暴露", "门窗",
    ],
    "medical": [
        "发热", "发烧", "咳嗽", "腹泻", "呕吐", "伤口", "出血", "感染",
        "疼痛", "昏迷", "抽搐", "冻伤", "烧伤", "烫伤", "脱水", "药品",
        "病人", "老人", "儿童", "孕妇", "照护", "医疗",
    ],
    "water": [
        "缺水", "停水", "饮用水", "储水", "净水", "水质", "水源", "取水",
        "用水", "补水", "桶装水", "过滤", "煮沸", "消毒",
    ],
    "food": [
        "食物", "主食", "粮食", "罐头", "剩饭", "剩食", "配给", "口粮",
        "营养", "冰箱", "做饭", "烹饪", "热食", "冷食", "蛋白",
        "库存天数", "留种", "种子", "种菜",
    ],
    "power": [
        "停电", "断电", "电量", "低电量", "移动电源", "充电", "充电宝",
        "太阳能", "照明", "手电", "电池", "发电机", "插线板",
    ],
    "hygiene": [
        "卫生", "清洁", "洗手", "厕所", "桶厕", "排泄", "垃圾", "消毒",
        "污染", "污水", "呕吐物", "腹泻污染", "苍蝇", "蚊虫",
    ],
    "tools": [
        "工具", "维修", "修理", "固定", "加固", "胶带", "绳子", "螺丝刀",
        "扳手", "基础工具箱", "破损", "替代制作",
    ],
    "comms": [
        "通讯", "通信", "电台", "无线电", "对讲机", "短波", "lora",
        "信号", "地图", "导航", "定位", "离线地图",
    ],
    "shelter": [
        "避难", "庇护", "居住", "睡眠区", "房间", "帐篷", "保暖", "降温",
        "通风", "门窗", "漏水", "住所",
    ],
    "evacuation": [
        "撤离", "转移", "路线", "集合点", "离开", "返回", "撤高",
        "背包", "撤离包", "伤病员撤离",
    ],
    "disaster": [
        "地震", "洪水", "台风", "强风", "暴雨", "山洪", "火场", "火灾",
        "化学污染", "空气污染", "灾后", "余震",
    ],
    "records": [
        "记录", "日志", "备份", "档案", "清单", "登记", "巡检", "复查",
    ],
}

DOMAIN_COMPATIBILITY = {
    "security": {"security", "shelter", "tools", "comms", "power", "evacuation", "disaster"},
    "medical": {"medical", "water", "hygiene", "food", "shelter"},
    "water": {"water", "hygiene", "medical", "food", "disaster"},
    "food": {"food", "water", "hygiene", "medical"},
    "power": {"power", "tools", "comms", "shelter", "security"},
    "hygiene": {"hygiene", "water", "medical", "shelter", "food"},
    "tools": {"tools", "power", "shelter", "security", "comms"},
    "comms": {"comms", "power", "security", "records", "evacuation"},
    "shelter": {"shelter", "security", "power", "hygiene", "tools", "disaster"},
    "evacuation": {"evacuation", "security", "medical", "shelter", "comms", "disaster"},
    "disaster": {"disaster", "security", "shelter", "evacuation", "medical", "water", "power"},
    "records": {"records", "comms"},
}


def _safe_text(value: Any) -> str:
    if isinstance(value, list):
        return " ".join(_safe_text(item) for item in value)
    if isinstance(value, dict):
        return " ".join(_safe_text(v) for v in value.values())
    return str(value or "").lower().replace("\n", " ")


def _safe_split_terms(text: str) -> List[str]:
    terms = [
        item.strip()
        for item in re.split(r"[\s，。！？、；：,.!?;:（）()【】\[\]《》<>\"']+", str(text or ""))
        if len(item.strip()) >= 2
    ]

    raw = _safe_text(text)
    for words in SAFE_DOMAIN_KEYWORDS.values():
        for word in words:
            word = str(word).lower().strip()
            if len(word) >= 2 and word in raw and word not in terms:
                terms.append(word)

    return terms[:30]


def detect_domains(text: str) -> List[str]:
    """结构化领域识别 v0.5：只看用户原话，不使用单字关键词。"""
    clean_text = _safe_text(text)
    if not clean_text:
        return []

    scores = {}
    for domain, keywords in SAFE_DOMAIN_KEYWORDS.items():
        score = 0
        for keyword in keywords:
            keyword = str(keyword).lower().strip()
            if len(keyword) < 2:
                continue
            if keyword and keyword in clean_text:
                score += 1
        if score:
            scores[domain] = score

    return [
        domain
        for domain, _score in sorted(scores.items(), key=lambda pair: pair[1], reverse=True)
    ]


def guide_domains(guide: Dict[str, Any]) -> List[str]:
    domains = guide.get("domains")
    if isinstance(domains, list) and domains:
        return [str(item).lower().strip() for item in domains if item]

    text = build_guide_search_text(guide)
    result = []
    for domain, keywords in SAFE_DOMAIN_KEYWORDS.items():
        if any(len(keyword) >= 2 and keyword in text for keyword in keywords):
            result.append(domain)
    return result


def guide_compatible_with_domains(guide: Dict[str, Any], detected_domains: List[str]) -> bool:
    if not detected_domains:
        return True

    domains = set(guide_domains(guide))
    if not domains:
        return False

    for domain in detected_domains:
        allowed = DOMAIN_COMPATIBILITY.get(domain, {domain})
        if domains & allowed:
            return True

    return False


def build_guide_search_text(guide: Dict[str, Any]) -> str:
    fields = [
        "id", "title", "category", "category_original", "scenario", "goal",
        "fallback", "notes", "observe", "summary", "source",
        "domains", "keywords", "negative_keywords", "situations",
    ]

    parts = [_safe_text(guide.get(key)) for key in fields]

    for key in [
        "tools", "steps", "check", "common_mistakes", "stop_or_escalate",
        "do_first", "avoid", "items",
    ]:
        parts.append(_safe_text(guide.get(key)))

    return " ".join(part for part in parts if part)


def guide_has_negative_query_keywords(guide: Dict[str, Any], message: str) -> bool:
    negative_keywords = guide.get("negative_keywords") or []
    if not isinstance(negative_keywords, list):
        return False

    message_text = _safe_text(message)
    return any(str(keyword).lower().strip() in message_text for keyword in negative_keywords if len(str(keyword).strip()) >= 2)


def score_guide_for_message(
    guide: Dict[str, Any],
    message: str,
    detected_domains: List[str],
) -> int:
    if detected_domains and not guide_compatible_with_domains(guide, detected_domains):
        return -100

    if guide_has_negative_query_keywords(guide, message):
        return -100

    score = 0
    terms = _safe_split_terms(message)
    search_text = build_guide_search_text(guide)

    title = _safe_text(guide.get("title"))
    category = _safe_text([guide.get("category"), guide.get("category_original")])
    scenario_goal = _safe_text([guide.get("scenario"), guide.get("goal"), guide.get("summary")])
    keywords_text = _safe_text(guide.get("keywords"))
    guide_domain_text = _safe_text(guide.get("domains"))

    for term in terms:
        if term in title:
            score += 10
        elif term in keywords_text:
            score += 8
        elif term in category:
            score += 5
        elif term in scenario_goal:
            score += 4
        elif term in search_text:
            score += 1

    for domain in detected_domains:
        if domain in guide_domain_text:
            score += 6

    return score


def find_guides_by_message_and_domains(
    message: str,
    detected_domains: List[str],
    guides: List[Dict[str, Any]],
    min_score: int = 4,
) -> List[Dict[str, Any]]:
    scored_guides = []

    for guide in guides:
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


def find_domain_fallback_guides(
    domains: List[str],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    """只用结构化 domains 召回，不再用标题相似度猜测。"""
    if not domains:
        return []

    result = []
    seen_ids = set()

    for guide in guides:
        guide_id = guide.get("id") or guide.get("title")
        if not guide_id or guide_id in seen_ids:
            continue

        if guide_compatible_with_domains(guide, domains):
            result.append(guide)
            seen_ids.add(guide_id)

    return result


def find_guides_by_domain_keywords(
    domains: List[str],
    guides: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    """结构化 domains 优先的轻量召回，避免全文宽匹配。"""
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

    detected_domains = detect_domains(user_message)
    matched_triggers = match_triggers(user_message, triggers)[:10]

    trigger_guides = [
        guide for guide in find_related_guides(matched_triggers, guides)
        if guide_compatible_with_domains(guide, detected_domains)
    ]

    scored_guides = find_guides_by_message_and_domains(
        message=user_message,
        detected_domains=detected_domains,
        guides=guides,
        min_score=4,
    )

    # 只有没有词面高相关结果时，才启用同领域兜底；兜底也只取少量。
    domain_guides = []
    if not scored_guides and detected_domains:
        domain_guides = find_domain_fallback_guides(detected_domains, guides)[:6]

    related_guides = merge_guides(
        trigger_guides,
        scored_guides,
        domain_guides,
    )[:10]

    # print("RESOURCE DEBUG detected_domains:", detected_domains)
    # print("RESOURCE DEBUG scored_guides:", [(g.get("title"), g.get("_match_score")) for g in scored_guides[:10]])
    # print("RESOURCE DEBUG related_guides:", [g.get("title") for g in related_guides])

    return {
        "detected_domains": detected_domains,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
    }
