"""本地 AI 重排模块。负责候选来源的 LLM 辅助选择与排除。"""

from typing import Any, Dict, List, Optional, Tuple
import json
import os

from ..config import OLLAMA_MODEL

from .constants import (
    AI_RERANK_VERSION,
    AI_RERANK_ENV_NAME,
    AI_RERANK_MODEL_ENV_NAME,
    AI_RERANK_MAX_CANDIDATES,
    AI_RERANK_MAX_SELECTED,
    AI_RERANK_MAX_EXCLUDED,
)

from .context_boost import (
    build_lantern_context_for_retrieval,
    merge_lantern_context_into_query_profile,
    apply_lantern_context_boost_to_candidates,
)

from .guide import build_guide_query

from ..llm.client import call_ollama

def _resolve_candidate_id(
    candidate_id: str,
    by_id: Dict[str, Dict[str, Any]],
) -> Optional[str]:
    """
    解析 AI 返回的 candidate_id。

    本地小模型有时会把 guide:DG-0065 输出成 DG-0065。
    这里只做 ID 规范化，不按标题猜测，避免引入不稳定规则。
    """
    candidate_id = str(candidate_id or "").strip()

    if not candidate_id:
        return None

    if candidate_id in by_id:
        return candidate_id

    guide_prefixed = f"guide:{candidate_id}"
    if guide_prefixed in by_id:
        return guide_prefixed

    if candidate_id.startswith("guide:"):
        unprefixed = candidate_id.removeprefix("guide:")
        if unprefixed in by_id:
            return unprefixed

    return None


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
    return os.getenv(AI_RERANK_MODEL_ENV_NAME) or OLLAMA_MODEL


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
6. 选择时优先匹配用户当前真正要处理的风险和行动目标，不要被单个物体词带偏。
7. 如果用户描述的是资源余量不足、只剩少量水或食物，优先选择配给、用途排序、消耗控制、优先级分配类指南；不要优先选择容器清洁、工具维护、储存容器类指南。
8. 如果用户描述的是暴雨、水位上涨、洪水风险，优先选择洪水、撤离、断电、避险类指南；不要优先选择停水、储水、日常用水类指南。
9. 如果用户描述的是门外有人徘徊、接近、走来走去，优先选择可疑人员接近、观察、暴露控制类指南；不要仅因为发生在夜间就选择夜间异响类指南。
10. 输出必须是一个合法 JSON object。
11. 不要输出解释性正文，不要使用 Markdown 代码块，不要在 JSON 前后添加任何文字。
12. JSON 中所有字符串必须使用英文双引号，不能使用单引号。
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
        raw_candidate_id = str(entry.get("candidate_id") or "").strip()
        candidate_id = _resolve_candidate_id(raw_candidate_id, by_id)
        item = by_id.get(candidate_id) if candidate_id else None

        if not raw_candidate_id or not item:
            if raw_candidate_id:
                rejected_ids.append(raw_candidate_id)
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
    strategy: Optional[Dict[str, Any]] = None,
    model: Optional[str] = None,
    enable_ai_rerank: Optional[bool] = None,
    max_selected: int = AI_RERANK_MAX_SELECTED,
) -> Dict[str, Any]:
    """AI 重排入口。

    Retrieval v2:
        Context -> Strategy -> AI Reranker

    query_profile 不再由外部传入，而是在 Reranker 内部由 Strategy
    转换得到，作为兼容层存在。
    """

    lantern_context = build_lantern_context_for_retrieval(user_message)

    query_profile = build_guide_query(strategy or {})
    query_profile = merge_lantern_context_into_query_profile(
        query_profile,
        lantern_context,
    )

    if not candidates:
        result = rule_rerank_candidates([], max_selected=max_selected)
        result["mode"] = "rule_fallback_no_candidates"
        result["fallback_reason"] = "没有可供 AI 重排的候选来源，使用空规则结果。"
        result["lantern_context"] = lantern_context
        return result

    candidates = apply_lantern_context_boost_to_candidates(
        candidates,
        lantern_context,
    )

    if not should_enable_ai_rerank(enable_ai_rerank):
        result = rule_rerank_candidates(
            candidates,
            max_selected=max_selected,
        )
        result["mode"] = "rule_only_ai_rerank_disabled"
        result["fallback_reason"] = "AI 检索增强未开启，使用规则排序。"
        result["lantern_context"] = lantern_context
        return result

    model = model or get_ai_rerank_model()

    try:
        messages = build_ai_rerank_messages(
            user_message,
            candidates,
            query_profile=query_profile,
        )

        raw = call_ollama(
            messages,
            model=model,
            force_json=True,
            temperature=0.0,
            num_predict=900,
        )

        parsed = _safe_json_loads_from_text(raw)

        if not parsed:
            repair_messages = [
                {
                    "role": "system",
                    "content": "你是 JSON 修复器。只输出一个合法 JSON object，不要解释，不要 Markdown。",
                },
                {
                    "role": "user",
                    "content": (
                        "把下面内容修复为合法 JSON object。"
                        "字段尽量保留："
                        "intent_summary, selected_candidate_ids, "
                        "selected, excluded, answer_focus。\n\n"
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
            fallback = rule_rerank_candidates(
                candidates,
                max_selected=max_selected,
            )

            fallback["mode"] = "rule_fallback_json_parse_failed"
            fallback["fallback_reason"] = (
                "本地 AI 已返回内容，但不是可解析的 JSON，"
                "已回退到规则排序。"
            )
            fallback["raw_ai_output"] = str(raw or "")[:500]
            fallback["lantern_context"] = lantern_context
            return fallback

        validated = validate_ai_rerank_result(
            parsed,
            candidates,
            max_selected=max_selected,
        )

        if not validated.get("valid"):
            fallback = rule_rerank_candidates(
                candidates,
                max_selected=max_selected,
            )

            fallback["mode"] = "rule_fallback_no_valid_ai_selection"

            rejected = validated.get("rejected_candidate_ids") or []

            fallback["fallback_reason"] = (
                "本地 AI 没有选出合法来源，"
                "或选择了不存在/被硬排除的 candidate_id，"
                "已回退到规则排序。"
            )

            if rejected:
                fallback["fallback_reason"] += (
                    f" 被拒绝的候选：{', '.join(map(str, rejected[:6]))}"
                )

            fallback["ai_validation"] = validated
            fallback["lantern_context"] = lantern_context
            return fallback

        validated["lantern_context"] = lantern_context
        return validated

    except Exception as exc:
        fallback = rule_rerank_candidates(
            candidates,
            max_selected=max_selected,
        )

        fallback["mode"] = "rule_fallback_ai_error"
        fallback["error"] = str(exc)[:300]
        fallback["fallback_reason"] = (
            f"本地 AI 重排调用失败，"
            f"已回退到规则排序：{str(exc)[:240]}"
        )
        fallback["lantern_context"] = lantern_context
        return fallback
    

def _mark_guide_rule_rank(
    candidates: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    if not candidates:
        return candidates

    rule_top1_title = candidates[0].get("title")

    marked = []
    for index, candidate in enumerate(candidates, start=1):
        item = dict(candidate)
        item["_rule_rank"] = index
        item["_rule_top1_title"] = rule_top1_title
        marked.append(item)

    return marked


def _guide_candidate_id(candidate: Dict[str, Any], index: int) -> str:
    guide_id = candidate.get("id") or candidate.get("guide_id") or index
    return f"guide:{guide_id}"


def _guide_to_rerank_candidate(
    candidate: Dict[str, Any],
    index: int,
) -> Dict[str, Any]:
    candidate_id = _guide_candidate_id(candidate, index)

    summary_parts = [
        f"title={candidate.get('title')}",
        f"scenario={candidate.get('scenario')}",
        f"goal={candidate.get('goal')}",
        f"notes={candidate.get('notes')}",
        f"domains={candidate.get('domains') or []}",
        f"intents={candidate.get('intents') or []}",
        f"primary_domain={candidate.get('primary_domain')}",
        f"primary_intent={candidate.get('primary_intent')}",
        f"ranking_role={candidate.get('ranking_role')}",
        f"guide_type={candidate.get('guide_type')}",
        f"signals={candidate.get('signals') or []}",
        f"risks={candidate.get('risks') or []}",
    ]

    return {
        "candidate_id": candidate_id,
        "source_type": "guide",
        "source_label": "应急指南",
        "title": candidate.get("title"),
        "summary": "；".join(summary_parts),
        "score": candidate.get("_match_score", 0),
        "rank": index,
        "reason": candidate.get("_match_reason", ""),
        "matched_terms": (
            candidate.get("keywords")
            or candidate.get("top1_aliases")
            or []
        )[:8],
        "hard_excluded": False,
        "_guide_candidate": candidate,
    }


def rerank_guide_candidates(
    *,
    user_message: str,
    strategy: Dict[str, Any],
    query_profile: Dict[str, Any],
    candidates: List[Dict[str, Any]],
) -> List[Dict[str, Any]]:
    """
    Guide candidates rerank hook.

    v0.8.2:
    - 记录规则排序结果
    - enable_ai_rerank=False 时不改变顺序
    - enable_ai_rerank=True 时复用现有本地 AI reranker
    - AI 失败或输出无效时回退规则顺序
    """
    if not candidates:
        return candidates

    marked_candidates = _mark_guide_rule_rank(candidates)

    if not should_enable_ai_rerank(strategy.get("enable_ai_rerank")):
        return marked_candidates

    rerank_candidates = [
        _guide_to_rerank_candidate(candidate, index)
        for index, candidate in enumerate(marked_candidates, start=1)
    ]

    by_candidate_id = {
        item["candidate_id"]: item["_guide_candidate"]
        for item in rerank_candidates
    }

    result = rerank_candidates_with_local_ai(
        user_message,
        rerank_candidates,
        strategy=strategy,
        enable_ai_rerank=True,
        max_selected=len(rerank_candidates),
    )

    selected_ids = result.get("selected_candidate_ids") or []

    ranked_guides = []
    used_ids = set()

    for candidate_id in selected_ids:
        guide = by_candidate_id.get(candidate_id)
        if not guide:
            continue

        item = dict(guide)
        item["_guide_rerank_mode"] = result.get("mode")
        item["_guide_rerank_used_ai"] = result.get("used_ai", False)
        item["_guide_rerank_reason"] = "AI 重排选择"
        ranked_guides.append(item)
        used_ids.add(candidate_id)

    for rerank_candidate in rerank_candidates:
        candidate_id = rerank_candidate["candidate_id"]
        if candidate_id in used_ids:
            continue

        guide = by_candidate_id[candidate_id]
        item = dict(guide)
        item["_guide_rerank_mode"] = result.get("mode")
        item["_guide_rerank_used_ai"] = result.get("used_ai", False)
        ranked_guides.append(item)

    return ranked_guides