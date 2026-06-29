"""本地资源协调层。负责加载资源、合并指南并准备应急模式 AI 上下文。"""

from typing import Any, Dict, List


from .config import (
    DATA_DIR,
    GUIDES_CACHE,
    RESOURCE_CACHE_INFO,
    TRIGGERS_CACHE,
    CONTEXT_PROFILES_CACHE,
    GUIDE_TAXONOMY_CACHE,
)
from .utils import read_json_file, safe_text, contains_any, unique_list

from .services.guide_service import (
    match_triggers,
)

from .retrieval.guide import (
    build_guide_query,
)

from .retrieval.runtime import (
    build_candidate_pool,
    build_retrieval_decision,
    HYBRID_RAG_VERSION,
)

from .retrieval.exclusions import (
    detect_explicit_exclusions,
    apply_hard_exclusions_to_candidates,
    split_selected_and_excluded_candidates,
)

from .retrieval.candidate_source import (
    get_candidate_raw_item,
)

from .retrieval.strategy import build_retrieval_strategy
from .retrieval.candidates import build_guide_candidates

from .context.engine import analyze_context


def load_local_resources() -> None:
    guides_path = DATA_DIR / "emergency_guides.json"
    triggers_path = DATA_DIR / "ai_triggers.json"
    context_profiles_path = DATA_DIR / "context_profiles.json"
    guide_taxonomy_path = DATA_DIR / "guide_taxonomy.json"

    GUIDES_CACHE.clear()
    GUIDES_CACHE.extend(read_json_file(guides_path, []))
    TRIGGERS_CACHE.clear()
    TRIGGERS_CACHE.extend(read_json_file(triggers_path, []))
    CONTEXT_PROFILES_CACHE.clear()
    CONTEXT_PROFILES_CACHE.extend(read_json_file(context_profiles_path, []))
    GUIDE_TAXONOMY_CACHE.clear()
    GUIDE_TAXONOMY_CACHE.update(read_json_file(guide_taxonomy_path,{},)
    )

    RESOURCE_CACHE_INFO.update({
        "emergency_guides_count": len(GUIDES_CACHE),
        "ai_triggers_count": len(TRIGGERS_CACHE),
        "loaded": True,
        "guides_path": str(guides_path),
        "triggers_path": str(triggers_path),
        "context_profiles_count": len(CONTEXT_PROFILES_CACHE),
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

    query_profile = build_guide_query(strategy)

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
