"""本地资源协调层。负责加载资源、合并指南并准备应急模式 AI 上下文。"""

from typing import Any, Dict, List


from .config import (
    DATA_DIR,
    GUIDES_CACHE,
    RESOURCE_CACHE_INFO,
    TRIGGERS_CACHE,
)
from .utils import read_json_file, safe_text, contains_any, unique_list

from .services.guide_service import (
    match_triggers,
    find_related_guides
)

from .retrieval.guide import (
    analyze_query,
    score_guide_for_message,
    find_guides_by_message_and_domains,
    find_domain_fallback_guides,
    build_match_reason,
    build_query_profile_from_strategy,
)

from .retrieval.runtime import (
    build_candidate_pool,
    apply_hard_exclusions_to_candidates,
    split_selected_and_excluded_candidates,
    build_retrieval_decision,
    detect_explicit_exclusions,
    get_candidate_raw_item,
    HYBRID_RAG_VERSION,
)
from .retrieval.strategy import build_retrieval_strategy
from .retrieval.candidates import build_guide_candidates

from .context.engine import analyze_context


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


SOURCE_PRIORITY = {
    "guide": 100,
    "wiki": 80,
    "inventory": 70,
    "record": 65,
    "kiwix": 45,
    "model": 10,
}

SOURCE_TYPE_LABELS = {
    "guide": "应急指南",
    "wiki": "本地 Wiki",
    "inventory": "库存/物资",
    "record": "本地记录",
    "kiwix": "Kiwix 大型资料库",
    "model": "模型自身知识",
}


def prepare_ai_context(user_message: str, mode: str) -> Dict[str, Any]:
    context = analyze_context(user_message)
    strategy = build_retrieval_strategy(context)
    analysis_domains = context.domains
    analysis_signals = context.signals
    analysis_intents = context.intents
    analysis_risks = context.risks
    analysis_plan = context.retrieval_plan


    if mode == "companion":
        return {
            "detected_domains": [],
            "matched_triggers": [],
            "related_guides": [],
            "query_profile": {},
            "candidate_sources": [],
            "selected_sources": [],
            "excluded_sources": [],
            "retrieval_decision": {"version": HYBRID_RAG_VERSION, "mode": "companion"},
        }

    if not RESOURCE_CACHE_INFO.get("loaded"):
        load_local_resources()

    guides = GUIDES_CACHE
    triggers = TRIGGERS_CACHE

    strategy = build_retrieval_strategy(context)

    query_profile = build_query_profile_from_strategy(strategy)

    detected_domains = analysis_domains or query_profile.get("domains", [])
    matched_triggers = match_triggers(user_message, triggers)[:10]

    guide_pool = build_guide_candidates(
        strategy=strategy,
        user_message=user_message,
        guides=guides,
        matched_triggers=matched_triggers,
    )

    candidates = build_candidate_pool(
        user_message=user_message,
        strategy=strategy,
        guide_candidates=guide_pool,
        wiki_candidates=[],
        include_kiwix=False,
    )

    exclusions = detect_explicit_exclusions(user_message)
    candidates = apply_hard_exclusions_to_candidates(candidates, exclusions)
    split = split_selected_and_excluded_candidates(candidates, max_selected=12)

    selected_sources = split["selected"]
    excluded_sources = split["excluded"]

    # 兼容旧逻辑：related_guides 仍然返回 guide 的 raw，并保持最多 10 条。
    related_guides = [
        get_candidate_raw_item(item)
        for item in selected_sources
        if item.get("source_type") == "guide"
    ][:10]

    retrieval_decision = build_retrieval_decision(
        user_message=user_message,
        strategy=strategy,
        candidates=candidates,
        selected=selected_sources,
        excluded=excluded_sources,
        mode="rule_candidate_pool",
    )
    retrieval_decision["explicit_exclusions"] = exclusions

    return {
        "detected_domains": detected_domains,
        "matched_triggers": matched_triggers,
        "related_guides": related_guides,
        "query_profile": query_profile,
        "candidate_sources": candidates,
        "selected_sources": selected_sources,
        "excluded_sources": excluded_sources,
        "retrieval_decision": retrieval_decision,
    }
