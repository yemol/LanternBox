"""Context Boost 过渡模块。将 Context Engine 输出转为检索加权信号。"""

from typing import Any, Dict, List, Optional, Tuple
from .query import normalize_reference_text
from ..context.engine import analyze_context

def lantern_context_terms(context: Dict[str, Any]) -> List[str]:
    """把 Context 转成检索可用的关键词。

    这里不做硬分类，只把 Context 里的观察、需求和检索计划转成召回信号。
    """
    terms: List[str] = []
    for key in [
        "retrieval_plan",
        "inferred_needs",
        "observations",
        "domains",
        "signals",
        "risks",
    ]:
        value = context.get(key)
        if isinstance(value, list):
            terms.extend(str(item).strip() for item in value if str(item or "").strip())
        elif value:
            terms.append(str(value).strip())

    # 英文内部信号补充中文检索词，避免资料库里没有英文标签时失效。
    signal_map = {
        "water_shortage": ["缺水", "节水", "饮水", "净水", "水源"],
        "heat_exposure": ["高温", "中暑", "降温", "补水"],
        "dehydration": ["脱水", "补水", "饮水"],
        "heatstroke": ["中暑", "高温", "降温"],
        "water_depletion": ["缺水", "储水", "用水"],
        "water": ["水", "饮水", "净水", "节水"],
        "weather": ["天气", "高温", "降温"],
        "health": ["健康", "中暑", "脱水"],
        "resource": ["资源", "配给", "节约"],
    }
    expanded: List[str] = []
    for term in terms:
        expanded.append(term)
        expanded.extend(signal_map.get(term, []))

    seen = set()
    result: List[str] = []
    for term in expanded:
        term = str(term or "").strip()
        if len(term) < 2:
            continue
        if term not in seen:
            seen.add(term)
            result.append(term)
    return result[:32]


def merge_lantern_context_into_query_profile(
    query_profile: Optional[Dict[str, Any]],
    context: Dict[str, Any],
) -> Dict[str, Any]:
    merged = dict(query_profile or {})
    if context:
        merged["lantern_context"] = context
        merged["context_terms"] = lantern_context_terms(context)
        merged["context_risk_level"] = context.get("risk_level")
        merged["context_input_nature"] = context.get("input_nature")
    return merged


def candidate_text_for_context_boost(candidate: Dict[str, Any]) -> str:
    raw = candidate.get("raw") if isinstance(candidate.get("raw"), dict) else {}
    fields = [
        candidate.get("title"),
        candidate.get("summary"),
        candidate.get("reason"),
        candidate.get("source_label"),
        candidate.get("source_type"),
        candidate.get("matched_terms"),
        raw.get("title"),
        raw.get("category"),
        raw.get("category_original"),
        raw.get("summary"),
        raw.get("scenario"),
        raw.get("goal"),
        raw.get("tags"),
        raw.get("content"),
        raw.get("steps"),
        raw.get("check"),
        raw.get("stop_or_escalate"),
    ]
    return normalize_reference_text(fields)


def apply_lantern_context_boost_to_candidates(
    candidates: List[Dict[str, Any]],
    context: Dict[str, Any],
) -> List[Dict[str, Any]]:
    terms = lantern_context_terms(context)
    if not terms:
        return candidates

    boosted: List[Dict[str, Any]] = []
    for candidate in candidates:
        item = dict(candidate)
        if item.get("hard_excluded"):
            boosted.append(item)
            continue

        text = candidate_text_for_context_boost(item)
        matched: List[str] = []
        boost = 0
        for term in terms:
            term_norm = normalize_reference_text(term)
            if term_norm and term_norm in text:
                matched.append(term)
                boost += 8

        if matched:
            boost = min(boost, 48)
            item["score"] = item.get("score", 0) + boost
            old_terms = item.get("matched_terms") or []
            if not isinstance(old_terms, list):
                old_terms = [str(old_terms)]
            item["matched_terms"] = list(dict.fromkeys(old_terms + matched))[:12]
            item["context_boost"] = boost
            item["context_boost_terms"] = matched[:8]
            reason = str(item.get("reason") or "").strip()
            boost_reason = f"Context Boost：{'、'.join(matched[:4])}"
            item["reason"] = f"{reason}；{boost_reason}" if reason else boost_reason

        boosted.append(item)

    boosted.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    return boosted


def build_lantern_context_for_retrieval(user_message: str) -> Dict[str, Any]:
    """为 Retrieval 层生成 Lantern Context。

    注意：
    - 只能依赖 context.engine
    - 不能依赖 response.context_blocks
    - Context 失败不能影响原有检索流程
    """
    try:
        ctx = analyze_context(user_message or "")

        if hasattr(ctx, "model_dump"):
            return ctx.model_dump()

        if hasattr(ctx, "dict"):
            return ctx.dict()

        return dict(ctx or {})

    except Exception:
        return {}
