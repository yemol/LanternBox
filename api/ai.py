import json
import requests
from typing import Any, Dict, List, Optional, Tuple

from .config import OLLAMA_BASE_URL, OLLAMA_MODEL, SCENARIO_PROFILE
from .resources import build_local_context
from .utils import get_default_model_for_mode
from .wiki import build_wiki_context_for_ai
from .llm.client import call_ollama, stream_ollama
from .response.prompts import (
    build_safe_history,
    build_companion_messages,
    build_emergency_messages,
    build_ai_messages,
)
from .response.context_blocks import build_lantern_context
from .retrieval.query import normalize_reference_text


from .retrieval.references import filter_and_rank_ai_references
from .retrieval.domains import (
    detect_domains_from_text,
    detect_reference_domains,
)
from .retrieval.query import tokenize_reference_query


try:
    from .context.engine import analyze_context
except Exception:
    analyze_context = None


# -----------------------------------------------------------------------------
# AI 来源验收器 v0.5.1
# 目标：召回可以宽，但展示和进入 AI 上下文的来源必须通过验收。
# 重点：不依赖大模型，不让“外面”的“面”把夜间安全问题误判成食物问题。
#
# query.py
# 负责把用户输入变成 tokens
# domains.py
# 负责识别领域、判断领域兼容
# references.py
# 负责对候选引用资料进行过滤、打分、排序
#
# -----------------------------------------------------------------------------



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




FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS = [
    "联系物业",
    "联系供水公司",
    "联系供电公司",
    "联系电力公司",
    "联系客服",
    "拨打供电",
    "拨打供水",
    "叫外卖",
    "点外卖",
    "快递",
    "网上搜索",
    "上网查询",
]


def sanitize_ai_answer(answer: str, mode: str = "emergency") -> str:
    """最终回答安全清洗。

    这是提示词之外的确定性兜底：应急模式下如果模型仍把城市服务或
    外部稳定系统写成行动步骤，就移除包含这些词的整句/整行。
    """
    text = str(answer or "")
    if mode != "emergency" or not text.strip():
        return text

    lines = text.splitlines()
    cleaned = []
    removed = False

    for line in lines:
        if any(term in line for term in FORBIDDEN_EXTERNAL_DEPENDENCY_TERMS):
            removed = True
            continue
        cleaned.append(line)

    result = "\n".join(cleaned).strip()
    if removed:
        note = "\n\n补充约束：当前按无外部支援场景处理，优先执行本地可控措施，并持续记录与复查。"
        if "无外部支援" not in result:
            result = (result + note).strip()

    return result or "当前按无外部支援场景处理。请先执行本地可控安全动作，并记录变化。"


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


# -----------------------------------------------------------------------------
# LanternBox AI v0.6.1 Hybrid RAG 重排层
# 目标：让本地 AI 参与候选来源过滤和重排，但由代码负责边界、校验和失败回退。
# 特点：默认仍可纯规则运行；开启后要求 Ollama 输出结构化 JSON；任何异常都回退规则排序。
# -----------------------------------------------------------------------------

AI_RERANK_VERSION = "v0.6.1-ai-reranker"
AI_RERANK_ENV_NAME = "LANTERNBOX_AI_RERANK"
AI_RERANK_MODEL_ENV_NAME = "LANTERNBOX_AI_RERANK_MODEL"
AI_RERANK_MAX_CANDIDATES = 30
AI_RERANK_MAX_SELECTED = 6
AI_RERANK_MAX_EXCLUDED = 10


def _env_truthy(value: Any) -> bool:
    return str(value or "").strip().lower() in {"1", "true", "yes", "on", "y", "启用", "开启"}


def should_enable_ai_rerank(enable_ai_rerank: Optional[bool] = None) -> bool:
    """解析 AI 重排开关。

    - 函数参数显式传 True/False 时优先；
    - 未传时读取环境变量 LANTERNBOX_AI_RERANK；
    - 默认关闭，保证低功耗和测试稳定。
    """
    if enable_ai_rerank is not None:
        return bool(enable_ai_rerank)
    try:
        import os
        return _env_truthy(os.getenv(AI_RERANK_ENV_NAME))
    except Exception:
        return False


def get_ai_rerank_model() -> str:
    return OLLAMA_MODEL


def _safe_json_loads_from_text(text: str) -> Optional[Dict[str, Any]]:
    """从模型输出中提取 JSON。失败返回 None，调用方必须回退规则排序。"""
    if not text:
        return None

    content = str(text).strip()

    # 兼容 ```json ... ``` 包裹。
    if content.startswith("```"):
        content = content.strip("`").strip()
        if content.lower().startswith("json"):
            content = content[4:].strip()

    candidates = [content]

    start = content.find("{")
    end = content.rfind("}")
    if start >= 0 and end > start:
        candidates.append(content[start:end + 1])

    for item in candidates:
        try:
            data = json.loads(item)
            if isinstance(data, dict):
                return data
        except Exception:
            continue
    return None


def _candidate_for_prompt(candidate: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "candidate_id": candidate.get("candidate_id"),
        "source_type": candidate.get("source_type"),
        "source_label": candidate.get("source_label") or candidate.get("source_type"),
        "title": candidate.get("title"),
        "summary": str(candidate.get("summary", ""))[:260],
        "score": candidate.get("score", 0),
        "reason": candidate.get("reason", ""),
        "matched_terms": candidate.get("matched_terms", [])[:8],
        "hard_excluded": bool(candidate.get("hard_excluded")),
        "excluded_reason": candidate.get("excluded_reason", ""),
    }


def build_ai_rerank_messages(
    user_message: str,
    candidates: List[Dict[str, Any]],
    query_profile: Optional[Dict[str, Any]] = None,
) -> List[Dict[str, str]]:
    """要求本地模型只在候选池里选择来源，并输出结构化 JSON。"""
    query_profile = query_profile or {}
    compact_candidates = [_candidate_for_prompt(item) for item in candidates[:AI_RERANK_MAX_CANDIDATES]]

    system_prompt = """
你是 LanternBox 本地离线知识重排器，不是最终回答助手。
你的任务是从候选来源中选择最适合当前问题的资料。

必须遵守：
1. 只能选择候选列表里存在的 candidate_id，不能编造来源。
2. hard_excluded=true 的来源绝对不能选入 selected_candidate_ids。
3. 用户明确说“先不管、暂时不考虑、不要处理”的主题，必须放入 excluded，并说明原因。
4. 应急指南 guide 通常优先于 wiki；wiki 优先于 kiwix；Kiwix 只作为背景资料。
5. 如果两个来源都相关，优先选择更贴近“当前优先事项”的来源，而不是只看大场景关键词。
6. 输出必须是一个合法 JSON object。
7. 不要输出解释性正文，不要使用 Markdown 代码块，不要在 JSON 前后添加任何文字。
8. JSON 中所有字符串必须使用英文双引号，不能使用单引号。
"""

    user_prompt = f"""
用户问题：
{user_message}

规则解析结果：
{json.dumps(query_profile, ensure_ascii=False)[:1800]}

候选来源：
{json.dumps(compact_candidates, ensure_ascii=False, indent=2)}

请输出 JSON，格式如下：
{{
  "intent_summary": "一句话概括用户当前真正要处理的事",
  "selected_candidate_ids": ["最多 6 个 candidate_id"],
  "selected": [
    {{"candidate_id": "被选中的 candidate_id", "reason": "为什么当前要用它"}}
  ],
  "excluded": [
    {{"candidate_id": "被排除的 candidate_id", "reason": "为什么当前不选"}}
  ],
  "answer_focus": ["回答应该聚焦的关键词"]
}}

注意：selected_candidate_ids 和 selected 二选一即可；如果都输出，代码会合并校验。
"""

    return [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt},
    ]


def _extract_selected_entries(ai_result: Dict[str, Any]) -> List[Dict[str, str]]:
    """兼容多种 JSON 字段名，降低本地小模型输出漂移影响。"""
    entries: List[Dict[str, str]] = []

    raw_ids = ai_result.get("selected_candidate_ids")
    if raw_ids is None:
        raw_ids = ai_result.get("selected_source_ids")
    if raw_ids is None:
        raw_ids = ai_result.get("selected_ids")

    for item in raw_ids or []:
        entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 选择"})

    raw_selected = ai_result.get("selected") or ai_result.get("selected_sources") or []
    for item in raw_selected:
        if isinstance(item, dict):
            candidate_id = str(item.get("candidate_id") or item.get("id") or item.get("source_id") or "").strip()
            reason = str(item.get("reason") or item.get("why") or "AI 选择")[:180]
            entries.append({"candidate_id": candidate_id, "reason": reason})
        else:
            entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 选择"})

    return entries


def _extract_excluded_entries(ai_result: Dict[str, Any]) -> List[Dict[str, str]]:
    entries: List[Dict[str, str]] = []
    raw_excluded = ai_result.get("excluded") or ai_result.get("excluded_sources") or ai_result.get("excluded_candidate_ids") or []
    for item in raw_excluded:
        if isinstance(item, dict):
            candidate_id = str(item.get("candidate_id") or item.get("id") or item.get("source_id") or "").strip()
            reason = str(item.get("reason") or item.get("why") or "AI 排除")[:180]
            entries.append({"candidate_id": candidate_id, "reason": reason})
        else:
            entries.append({"candidate_id": str(item or "").strip(), "reason": "AI 排除"})
    return entries


def validate_ai_rerank_result(
    ai_result: Dict[str, Any],
    candidates: List[Dict[str, Any]],
    *,
    max_selected: int = AI_RERANK_MAX_SELECTED,
) -> Dict[str, Any]:
    by_id = {str(item.get("candidate_id")): item for item in candidates if item.get("candidate_id")}
    selected_ids: List[str] = []
    selected_reasons: Dict[str, str] = {}
    rejected_ids: List[str] = []

    for entry in _extract_selected_entries(ai_result):
        candidate_id = str(entry.get("candidate_id") or "").strip()
        item = by_id.get(candidate_id)
        if not candidate_id or not item:
            if candidate_id:
                rejected_ids.append(candidate_id)
            continue
        if item.get("hard_excluded"):
            rejected_ids.append(candidate_id)
            continue
        if candidate_id not in selected_ids:
            selected_ids.append(candidate_id)
            selected_reasons[candidate_id] = str(entry.get("reason") or "AI 选择")[:180]
        if len(selected_ids) >= max_selected:
            break

    selected: List[Dict[str, Any]] = []
    for candidate_id in selected_ids:
        item = dict(by_id[candidate_id])
        item["rerank_reason"] = selected_reasons.get(candidate_id, "AI 选择")
        item["rerank_mode"] = "ai"
        selected.append(item)

    # 硬排除候选永远进入 excluded_sources，AI 额外排除理由可覆盖/补充。
    excluded_by_id: Dict[str, Dict[str, Any]] = {}
    for candidate in candidates:
        if candidate.get("hard_excluded") and candidate.get("candidate_id"):
            cid = str(candidate.get("candidate_id"))
            excluded_by_id[cid] = {
                "candidate_id": cid,
                "source_type": candidate.get("source_type"),
                "title": candidate.get("title"),
                "reason": candidate.get("excluded_reason") or "硬排除",
            }

    for entry in _extract_excluded_entries(ai_result):
        candidate_id = str(entry.get("candidate_id") or "").strip()
        if candidate_id in by_id:
            excluded_by_id[candidate_id] = {
                "candidate_id": candidate_id,
                "source_type": by_id[candidate_id].get("source_type"),
                "title": by_id[candidate_id].get("title"),
                "reason": str(entry.get("reason") or by_id[candidate_id].get("excluded_reason") or "AI 排除")[:180],
            }

    # 如果模型没选出任何合法来源，调用方应回退规则排序。
    valid = bool(selected)

    return {
        "valid": valid,
        "version": AI_RERANK_VERSION,
        "mode": "ai_rerank_validated" if valid else "ai_rerank_failed",
        "intent_summary": str(ai_result.get("intent_summary") or "")[:240],
        "answer_focus": [str(x)[:50] for x in (ai_result.get("answer_focus") or [])[:8]],
        "selected_sources": selected,
        "excluded_sources": list(excluded_by_id.values())[:AI_RERANK_MAX_EXCLUDED],
        "rejected_candidate_ids": rejected_ids,
        "selected_candidate_ids": [item.get("candidate_id") for item in selected if item.get("candidate_id")],
        "fallback_reason": None,
        "used_ai": valid,
    }


def rule_rerank_candidates(candidates: List[Dict[str, Any]], *, max_selected: int = AI_RERANK_MAX_SELECTED) -> Dict[str, Any]:
    selected = [item for item in candidates if not item.get("hard_excluded")]
    excluded = [item for item in candidates if item.get("hard_excluded")]
    selected.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    excluded.sort(key=lambda item: (item.get("score", 0), -item.get("rank", 0)), reverse=True)
    return {
        "valid": True,
        "version": AI_RERANK_VERSION,
        "mode": "rule_fallback",
        "intent_summary": "规则回退排序",
        "answer_focus": [],
        "selected_sources": selected[:max_selected],
        "excluded_sources": [
            {
                "candidate_id": item.get("candidate_id"),
                "source_type": item.get("source_type"),
                "title": item.get("title"),
                "reason": item.get("excluded_reason", "硬排除"),
            }
            for item in excluded[:AI_RERANK_MAX_EXCLUDED]
        ],
        "rejected_candidate_ids": [],
        "selected_candidate_ids": [item.get("candidate_id") for item in selected[:max_selected] if item.get("candidate_id")],
        "fallback_reason": "规则排序回退",
        "used_ai": False,
    }


def rerank_candidates_with_local_ai(
    user_message: str,
    candidates: List[Dict[str, Any]],
    *,
    query_profile: Optional[Dict[str, Any]] = None,
    model: Optional[str] = None,
    enable_ai_rerank: Optional[bool] = None,
    max_selected: int = AI_RERANK_MAX_SELECTED,
) -> Dict[str, Any]:
    """AI 重排入口。

    默认读取环境变量 LANTERNBOX_AI_RERANK；未开启时使用纯规则回退。
    开启后：规则候选池 → 本地 AI JSON 重排 → 代码校验 → 失败回退规则排序。
    """
    lantern_context = build_lantern_context(user_message)
    query_profile = merge_lantern_context_into_query_profile(query_profile, lantern_context)

    if not candidates:
        result = rule_rerank_candidates([], max_selected=max_selected)
        result["mode"] = "rule_fallback_no_candidates"
        result["fallback_reason"] = "没有可供 AI 重排的候选来源，使用空规则结果。"
        result["lantern_context"] = lantern_context
        return result

    candidates = apply_lantern_context_boost_to_candidates(candidates, lantern_context)

    if not should_enable_ai_rerank(enable_ai_rerank):
        result = rule_rerank_candidates(candidates, max_selected=max_selected)
        result["mode"] = "rule_only_ai_rerank_disabled"
        result["fallback_reason"] = "AI 检索增强未开启，使用规则排序。"
        result["lantern_context"] = lantern_context
        return result

    model = model or get_ai_rerank_model()

    try:
        messages = build_ai_rerank_messages(user_message, candidates, query_profile=query_profile)
        raw = call_ollama(
            messages,
            model=model,
            force_json=True,
            temperature=0.0,
            num_predict=900,
        )
        parsed = _safe_json_loads_from_text(raw)
        if not parsed:
            # 少数本地模型即使 format=json 也可能吐出残缺 JSON。
            # 此处只做一次低成本修复请求：把原始输出整理成指定 JSON，不让它重新判断来源。
            repair_messages = [
                {
                    "role": "system",
                    "content": "你是 JSON 修复器。只输出一个合法 JSON object，不要解释，不要 Markdown。",
                },
                {
                    "role": "user",
                    "content": (
                        "把下面内容修复为合法 JSON object。字段尽量保留："
                        "intent_summary, selected_candidate_ids, selected, excluded, answer_focus。\n\n"
                        f"原始内容：\n{str(raw or '')[:1600]}"
                    ),
                },
            ]
            try:
                repaired_raw = call_ollama(
                    repair_messages,
                    model=model,
                    force_json=True,
                    temperature=0.0,
                    num_predict=700,
                )
                parsed = _safe_json_loads_from_text(repaired_raw)
                if parsed:
                    raw = repaired_raw
            except Exception:
                parsed = None

        if not parsed:
            fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
            fallback["mode"] = "rule_fallback_json_parse_failed"
            fallback["fallback_reason"] = "本地 AI 已返回内容，但不是可解析的 JSON，已回退到规则排序。"
            fallback["raw_ai_output"] = str(raw or "")[:500]
            fallback["lantern_context"] = lantern_context
            return fallback

        validated = validate_ai_rerank_result(parsed, candidates, max_selected=max_selected)
        if not validated.get("valid"):
            fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
            fallback["mode"] = "rule_fallback_no_valid_ai_selection"
            rejected = validated.get("rejected_candidate_ids") or []
            fallback["fallback_reason"] = "本地 AI 没有选出合法来源，或选择了不存在/被硬排除的 candidate_id，已回退到规则排序。"
            if rejected:
                fallback["fallback_reason"] += f" 被拒绝的候选：{', '.join(map(str, rejected[:6]))}"
            fallback["ai_validation"] = validated
            fallback["lantern_context"] = lantern_context
            return fallback
        validated["lantern_context"] = lantern_context
        return validated
    except Exception as exc:
        fallback = rule_rerank_candidates(candidates, max_selected=max_selected)
        fallback["mode"] = "rule_fallback_ai_error"
        fallback["error"] = str(exc)[:300]
        fallback["fallback_reason"] = f"本地 AI 重排调用失败，已回退到规则排序：{str(exc)[:240]}"
        fallback["lantern_context"] = lantern_context
        return fallback


def get_candidate_raw_item_from_ai(candidate: Dict[str, Any]) -> Dict[str, Any]:
    raw = candidate.get("raw")
    return raw if isinstance(raw, dict) else candidate


def apply_rerank_result_to_context(
    context: Dict[str, Any],
    rerank_result: Dict[str, Any],
) -> Dict[str, Any]:
    """把 AI/规则重排结果回写到 prepare_ai_context 的上下文结构中。

    这个函数供后端路由调用。resources.py 不直接调用 ai.py，避免循环依赖。
    """
    updated = dict(context or {})
    selected_sources = rerank_result.get("selected_sources", []) or []
    excluded_sources = rerank_result.get("excluded_sources", []) or []

    updated["selected_sources"] = selected_sources
    updated["excluded_sources"] = excluded_sources
    if rerank_result.get("lantern_context"):
        updated["lantern_context"] = rerank_result.get("lantern_context")
    updated["related_guides"] = [
        get_candidate_raw_item_from_ai(item)
        for item in selected_sources
        if item.get("source_type") == "guide"
    ][:10]

    decision = dict(updated.get("retrieval_decision") or {})
    decision.update({
        "version": AI_RERANK_VERSION,
        "mode": rerank_result.get("mode"),
        "used_ai": rerank_result.get("used_ai", False),
        "intent_summary": rerank_result.get("intent_summary") or decision.get("intent_summary"),
        "answer_focus": rerank_result.get("answer_focus", []),
        "lantern_context": rerank_result.get("lantern_context"),
        "selected_count": len(selected_sources),
        "excluded_count": len(excluded_sources),
        "rejected_candidate_ids": rerank_result.get("rejected_candidate_ids", []),
    })
    updated["retrieval_decision"] = decision
    return updated

def build_selected_sources_text(selected_sources: List[Dict[str, Any]]) -> str:
    blocks = []
    for item in selected_sources[:8]:
        blocks.append("\n".join([
            f"来源ID：{item.get('candidate_id', '')}",
            f"来源类型：{item.get('source_label') or item.get('source_type', '')}",
            f"标题：{item.get('title', '')}",
            f"选择原因：{item.get('reason', '')}",
            f"摘要：{item.get('summary', '')}",
        ]))
    return "\n\n".join(blocks)
