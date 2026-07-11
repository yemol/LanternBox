"""Retrieval v2 orchestrator."""

from typing import Dict, List, Optional, Tuple

from pydantic import Field

from .fetchers import fetch_candidates_from_plan
from .policy import policy_int, policy_map, policy_set, policy_str_list, policy_string
from .planner import build_retrieval_plan
from .schemas import EvidenceCandidate, EvidenceSelection, RetrievalDebug, RetrievalPlan, RetrievalV2Result, SelectedEvidence, SourcePlanItem
from .selector import select_evidence_with_ai


class RetrievalV2ResultWithKiwix(RetrievalV2Result):
    kiwix_explanations: List[Dict[str, object]] = Field(default_factory=list)


def _candidate_key(candidate: EvidenceCandidate) -> Tuple[str, str]:
    return (candidate.source_type, candidate.id)


def _validate_selected(
    candidates: List[EvidenceCandidate],
    selected_items,
) -> List[EvidenceCandidate]:
    by_key: Dict[Tuple[str, str], EvidenceCandidate] = {
        _candidate_key(candidate): candidate
        for candidate in candidates
    }

    selected: List[EvidenceCandidate] = []
    seen = set()

    for item in selected_items:
        key = (item.source_type, item.id)
        candidate = by_key.get(key)

        if not candidate:
            continue

        if key in seen:
            continue

        selected.append(candidate)
        seen.add(key)

    return selected


def _candidate_type_counts(candidates: List[EvidenceCandidate]) -> Dict[str, int]:
    counts: Dict[str, int] = {}
    for item in candidates:
        source_type = getattr(item, "source_type", "unknown")
        counts[source_type] = counts.get(source_type, 0) + 1
    return counts


def _has_source_plan(plan: RetrievalPlan, source_type: str) -> bool:
    return any(item.source_type == source_type for item in plan.source_plan or [])


def _should_trigger_kiwix(user_message: str, plan: RetrievalPlan) -> bool:
    if _has_source_plan(plan, "kiwix"):
        return True

    text = " ".join([
        str(user_message or ""),
        str(plan.scenario_summary or ""),
        " ".join(plan.needs or []),
        " ".join(plan.core_terms or []),
    ])
    if any(term in text for term in policy_str_list("kiwix", "trigger_terms")):
        return True

    raw = plan.raw if isinstance(plan.raw, dict) else {}
    mode_hint = " ".join(str(raw.get(key, "") or "") for key in ("intent", "mode", "purpose"))
    return any(term in mode_hint for term in policy_str_list("kiwix", "intent_hints"))


def _is_explicit_kiwix_request(user_message: str, plan: RetrievalPlan) -> bool:
    text = " ".join([
        str(user_message or ""),
        str(plan.scenario_summary or ""),
    ]).lower()
    return any(term.lower() in text for term in policy_str_list("kiwix", "explicit_aliases"))


def _kiwix_plan_from_context(user_message: str, plan: RetrievalPlan, *, reason: str = "background") -> SourcePlanItem:
    keyword_limit = policy_int(("retrieval", "kiwix_plan_keyword_limit"), 8)
    query_chars = policy_int(("retrieval", "kiwix_plan_query_chars"), 80)
    plan_limit = policy_int(("retrieval", "kiwix_plan_limit"), 4)
    keywords = list(plan.core_terms or [])[:keyword_limit]
    query = " ".join([*keywords, str(user_message or "").strip()[:query_chars]]).strip()
    return SourcePlanItem(
        source_type="kiwix",
        purpose=f"Kiwix / ZIM 背景补充：{reason}",
        query=query or str(user_message or "").strip(),
        categories=[],
        keywords=keywords,
        limit=plan_limit,
    )


def _primary_candidate_count(candidates: List[EvidenceCandidate]) -> int:
    primary_types = policy_set("primary_source_types")
    return sum(1 for item in candidates if item.source_type in primary_types)


def _dedupe_candidates(candidates: List[EvidenceCandidate]) -> List[EvidenceCandidate]:
    results: List[EvidenceCandidate] = []
    seen = set()
    for candidate in candidates or []:
        key = _candidate_key(candidate)
        if key in seen:
            continue
        seen.add(key)
        results.append(candidate)
    return results


def _fallback_selection_from_candidates(
    candidates: List[EvidenceCandidate],
    *,
    error: str = "",
    stage: str = "selector",
) -> EvidenceSelection:
    selected = []

    for candidate in candidates:
        if candidate.source_type == "guide":
            selected.append(candidate)
            break

    if not selected and candidates:
        selected.append(candidates[0])

    return EvidenceSelection(
        selected=[
            SelectedEvidence(
                source_type=item.source_type,
                id=item.id,
                reason="AI 选择器不可用，保留本地最高相关候选作为兜底。",
            )
            for item in selected
        ],
        excluded=[],
        answer_focus=["优先参考本地最高相关行动指南"],
        raw={"fallback": True, "error": error, "stage": stage},
    )


def _debug_payload(
    *,
    plan: RetrievalPlan,
    candidates: Optional[List[EvidenceCandidate]] = None,
    selected_evidence: Optional[List[EvidenceCandidate]] = None,
    ok: bool = True,
    error: str = "",
    stage: str = "",
) -> RetrievalDebug:
    candidates = candidates or []
    selected_evidence = selected_evidence or []

    return RetrievalDebug(
        engine="retrieval_v2_ai_orchestrated",
        candidate_types=_candidate_type_counts(candidates),
        candidate_count=len(candidates),
        selected_count=len(selected_evidence),
        source_plan_count=len(plan.source_plan or []),
        core_terms=list(plan.core_terms or []),
        ok=ok,
        error=error,
        stage=stage,
    )


def _result_payload(
    *,
    plan: RetrievalPlan,
    candidates: Optional[List[EvidenceCandidate]] = None,
    selection: EvidenceSelection,
    selected_evidence: Optional[List[EvidenceCandidate]] = None,
    debug: RetrievalDebug,
    kiwix_explanations: Optional[List[Dict[str, object]]] = None,
    selected_sources: Optional[List[Dict[str, object]]] = None,
    excluded_sources: Optional[List[Dict[str, object]]] = None,
    retrieval_decision: Optional[Dict[str, object]] = None,
) -> RetrievalV2ResultWithKiwix:
    return RetrievalV2ResultWithKiwix(
        plan=plan,
        candidates=candidates or [],
        selection=selection,
        selected_evidence=selected_evidence or [],
        debug=debug,
        kiwix_explanations=kiwix_explanations or [],
        selected_sources=selected_sources or [],
        excluded_sources=excluded_sources or [],
        retrieval_decision=retrieval_decision or {},
    )


def _empty_result(user_message: str, *, error: str = "", stage: str = "") -> RetrievalV2Result:
    plan = RetrievalPlan(
        scenario_summary=str(user_message or "").strip()[:80],
        urgency="unknown",
        needs=[],
        core_terms=[],
        source_plan=[],
        raw={
            "fallback": True,
            "error": error,
            "stage": stage,
        },
    )

    return _result_payload(
        plan=plan,
        candidates=[],
        selection=EvidenceSelection(
            selected=[],
            excluded=[],
            answer_focus=[],
            raw={
                "fallback": True,
                "error": error,
                "stage": stage,
            },
        ),
        selected_evidence=[],
        debug=_debug_payload(
            plan=plan,
            ok=False,
            error=error,
            stage=stage,
        ),
        kiwix_explanations=[],
    )


def _selected_by_type(selected_evidence: List[EvidenceCandidate], source_type: str) -> List[EvidenceCandidate]:
    return [
        item
        for item in selected_evidence or []
        if item.source_type == source_type
    ]


def _source_card(
    candidate: EvidenceCandidate,
    *,
    reason: str = "",
    excluded_reason: str = "",
) -> Dict[str, object]:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    source_type = candidate.source_type
    card: Dict[str, object] = {
        "source_type": source_type,
        "source_id": raw.get("source_id") or candidate.id,
        "id": candidate.id,
        "title": candidate.title,
        "label": raw.get("label") or policy_map("source_labels").get(source_type, source_type),
        "summary": candidate.summary,
        "snippet": candidate.snippet,
        "reason": reason,
        "selection_reason": reason,
    }

    if excluded_reason:
        card["excluded_reason"] = excluded_reason

    if source_type == "kiwix":
        usage_policy = _kiwix_usage_policy(candidate)
        card.update({
            "zim_filename": raw.get("zim_filename", ""),
            "article_path": raw.get("article_path", ""),
            "usage_policy": usage_policy,
            "source_role": raw.get("source_role") or raw.get("role") or "",
            "language": raw.get("language", ""),
            "relevance_score": raw.get("relevance_score", 0.0),
            "matched_terms": raw.get("matched_terms", []),
            "open_url": raw.get("open_url", ""),
            "metadata": raw.get("metadata", {}),
        })

    return card


def _selection_reason_map(selection_items: List[SelectedEvidence]) -> Dict[Tuple[str, str], str]:
    return {
        (item.source_type, item.id): item.reason
        for item in selection_items or []
    }


def _kiwix_usage_policy(candidate: EvidenceCandidate) -> str:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    usage_policy = str(raw.get("usage_policy") or "").strip()
    role = str(raw.get("source_role") or raw.get("role") or "").strip()
    if usage_policy:
        return usage_policy
    return str(policy_map("kiwix", "usage_policy", "role_to_policy").get(role, ""))


def _candidate_priority(candidate: EvidenceCandidate) -> tuple[int, float]:
    source_priorities = policy_map("source_priority")
    default_rank = int(source_priorities.get("default", 9))
    if candidate.source_type != "kiwix":
        return (int(source_priorities.get(candidate.source_type, default_rank)), 0.0)
    if candidate.source_type == "kiwix":
        raw = candidate.raw if isinstance(candidate.raw, dict) else {}
        usage_policy = _kiwix_usage_policy(candidate)
        language = str(raw.get("language") or "").strip()
        score = float(raw.get("relevance_score") or raw.get("score") or 0.0)
        policy_rank = policy_map("kiwix", "usage_policy", "source_priority_rank")
        base = int(policy_rank.get(usage_policy, policy_rank.get("default", source_priorities.get("kiwix", default_rank))))

        preferred_language = policy_string(("language", "preferred"), "")
        if preferred_language and language and language != preferred_language:
            base += policy_int(("language", "non_preferred_penalty"), 0)

        return (base, -score)

    return (6, 0.0)


def _best_selectable_kiwix(candidates: List[EvidenceCandidate]) -> Optional[EvidenceCandidate]:
    kiwix_candidates = [
        item
        for item in candidates or []
        if item.source_type == "kiwix"
    ]
    preferred = [
        item
        for item in kiwix_candidates
        if _kiwix_usage_policy(item) not in policy_set("kiwix", "usage_policy", "not_selectable")
        and _kiwix_usage_policy(item) not in policy_set("kiwix", "usage_policy", "fallback_excluded_when_primary_selected")
    ]
    if preferred:
        return preferred[0]
    background = [
        item
        for item in kiwix_candidates
        if _kiwix_usage_policy(item) in policy_set("kiwix", "usage_policy", "background_selectable")
    ]
    if background:
        return background[0]
    return None


def _ensure_explicit_kiwix_selected(
    *,
    user_message: str,
    plan: RetrievalPlan,
    candidates: List[EvidenceCandidate],
    selected_evidence: List[EvidenceCandidate],
    selection: EvidenceSelection,
) -> tuple[List[EvidenceCandidate], EvidenceSelection]:
    if not _is_explicit_kiwix_request(user_message, plan):
        return selected_evidence, selection
    candidate = _best_selectable_kiwix(candidates)
    if not candidate:
        return selected_evidence, selection
    if selected_evidence and any(item.source_type == "kiwix" and item.id == candidate.id for item in selected_evidence):
        return selected_evidence, selection

    selected_evidence = [
        item
        for item in selected_evidence
        if item.source_type != "kiwix"
    ]
    selected_evidence.append(candidate)
    selection.selected = [
        item
        for item in selection.selected
        if item.source_type != "kiwix"
    ]
    selection.selected.append(
        SelectedEvidence(
            source_type="kiwix",
            id=candidate.id,
            reason=policy_string(("kiwix", "selection_reason_explicit"), ""),
        )
    )
    answer_focus = policy_string(("kiwix", "answer_focus_explicit"), "")
    if answer_focus and answer_focus not in selection.answer_focus:
        selection.answer_focus.append(answer_focus)
    return selected_evidence, selection


def _apply_usage_policy(
    *,
    selection: EvidenceSelection,
    candidates: List[EvidenceCandidate],
    selected_evidence: List[EvidenceCandidate],
) -> tuple[List[EvidenceCandidate], EvidenceSelection]:
    primary_types = policy_set("primary_source_types")
    non_selectable = policy_map("kiwix", "usage_policy", "not_selectable")
    fallback_exclusions = policy_map("kiwix", "usage_policy", "fallback_excluded_when_primary_selected")
    has_selected_primary = any(item.source_type in primary_types for item in selected_evidence)
    filtered: List[EvidenceCandidate] = []
    policy_excluded: List[SelectedEvidence] = []
    selected_keys = {_candidate_key(item) for item in selected_evidence}

    for item in selected_evidence:
        if item.source_type != "kiwix":
            filtered.append(item)
            continue

        usage_policy = _kiwix_usage_policy(item)
        if usage_policy in non_selectable:
            policy_excluded.append(SelectedEvidence(source_type="kiwix", id=item.id, reason=str(non_selectable.get(usage_policy, ""))))
            continue
        if usage_policy in fallback_exclusions and has_selected_primary:
            policy_excluded.append(SelectedEvidence(source_type="kiwix", id=item.id, reason=str(fallback_exclusions.get(usage_policy, ""))))
            continue

        filtered.append(item)

    for candidate in candidates:
        if candidate.source_type != "kiwix":
            continue
        if _candidate_key(candidate) in selected_keys:
            continue
        usage_policy = _kiwix_usage_policy(candidate)
        if usage_policy in non_selectable:
            policy_excluded.append(SelectedEvidence(source_type="kiwix", id=candidate.id, reason=str(non_selectable.get(usage_policy, ""))))
        elif usage_policy in fallback_exclusions and has_selected_primary:
            policy_excluded.append(SelectedEvidence(source_type="kiwix", id=candidate.id, reason=str(fallback_exclusions.get(usage_policy, ""))))

    existing = selection.excluded or []
    seen = {(item.source_type, item.id, item.reason) for item in existing}
    for item in policy_excluded:
        key = (item.source_type, item.id, item.reason)
        if key not in seen:
            existing.append(item)
            seen.add(key)
    selection.excluded = existing
    filtered = sorted(filtered, key=_candidate_priority)
    selection.selected = [
        SelectedEvidence(
            source_type=item.source_type,
            id=item.id,
            reason=_selection_reason_map(selection.selected).get(_candidate_key(item), ""),
        )
        for item in filtered
    ]

    return filtered, selection


def _knowledge_gap_note(
    *,
    selected_evidence: List[EvidenceCandidate],
) -> str:
    selected_types = {item.source_type for item in selected_evidence or []}
    if "kiwix" in selected_types and "guide" not in selected_types and "wiki" not in selected_types:
        return "当前回答主要依赖 Kiwix/ZIM 背景资料，说明本地 Guide/Wiki 还不够丰富，建议继续补充应急手册和 Wiki。"
    if "kiwix" in selected_types and "guide" not in selected_types:
        return "当前回答使用了 Kiwix/ZIM 作为背景补充，建议补强本地 Guide 以提高行动可执行性。"
    if "kiwix" in selected_types and "wiki" not in selected_types:
        return "当前回答使用了 Kiwix/ZIM 作为背景补充，建议补强本地 Wiki 以提高判断解释能力。"
    return ""


def _build_source_outputs(
    *,
    candidates: List[EvidenceCandidate],
    selected_evidence: List[EvidenceCandidate],
    selection: EvidenceSelection,
) -> tuple[List[Dict[str, object]], List[Dict[str, object]], Dict[str, object]]:
    candidates_by_key = {_candidate_key(item): item for item in candidates}
    selected_reasons = _selection_reason_map(selection.selected)
    excluded_reasons = _selection_reason_map(selection.excluded)

    selected_sources = [
        _source_card(
            item,
            reason=selected_reasons.get(_candidate_key(item), ""),
        )
        for item in selected_evidence
    ]

    excluded_sources: List[Dict[str, object]] = []
    seen = set()
    for item in selection.excluded or []:
        candidate = candidates_by_key.get((item.source_type, item.id))
        if not candidate:
            continue
        key = (item.source_type, item.id, item.reason)
        if key in seen:
            continue
        seen.add(key)
        excluded_sources.append(
            _source_card(
                candidate,
                reason="",
                excluded_reason=excluded_reasons.get((item.source_type, item.id), item.reason),
            )
        )

    selected_types = _candidate_type_counts(selected_evidence)
    candidate_types = _candidate_type_counts(candidates)
    retrieval_decision = {
        "selected_count": len(selected_sources),
        "excluded_count": len(excluded_sources),
        "selected_types": selected_types,
        "candidate_types": candidate_types,
        "knowledge_gap_note": _knowledge_gap_note(selected_evidence=selected_evidence),
        **policy_map("kiwix", "decision"),
    }
    return selected_sources, excluded_sources, retrieval_decision


def _router_is_high(plan: RetrievalPlan) -> bool:
    raw = plan.raw if isinstance(plan.raw, dict) else {}
    values = [
        raw.get("router"),
        raw.get("router_level"),
        raw.get("route_level"),
        raw.get("routing_level"),
        raw.get("urgency"),
        plan.urgency,
    ]

    return any(str(value or "").strip().upper() == "HIGH" for value in values)


def _kiwix_context_for_item(
    item: EvidenceCandidate,
    plan: RetrievalPlan,
) -> Dict[str, object]:
    raw = item.raw if isinstance(item.raw, dict) else {}
    keywords = raw.get("keywords") or raw.get("tags") or item.tags

    return {
        "core_terms": list(plan.core_terms or []),
        "keywords": keywords,
        "topics": [
            item.title,
            item.category,
            *list(item.tags or []),
        ],
    }


def _build_kiwix_explanations(
    *,
    plan: RetrievalPlan,
    selected_evidence: List[EvidenceCandidate],
    limit_per_item: int = 1,
    max_total: int = 6,
) -> List[Dict[str, object]]:
    selected_guides = _selected_by_type(selected_evidence, "guide")
    selected_wikis = _selected_by_type(selected_evidence, "wiki")

    if not _router_is_high(plan):
        return []
    if not selected_guides or not selected_wikis:
        return []

    try:
        from api.kiwix.fetcher import query_for_ai
    except Exception:
        return []

    explanations: List[Dict[str, object]] = []
    seen = set()

    for item in [*selected_guides, *selected_wikis]:
        query = " ".join([
            item.title,
            item.category,
            " ".join(list(item.tags or [])[:6]),
        ]).strip()

        try:
            results = query_for_ai(query, context=_kiwix_context_for_item(item, plan), limit=limit_per_item)
        except Exception:
            results = []

        added_for_item = 0
        for result in results or []:
            title = str(getattr(result, "title", "") or "").strip()
            snippet = str(getattr(result, "snippet", "") or "").strip()
            source = str(getattr(result, "source", "") or "").strip()
            confidence = float(getattr(result, "relevance_score", 0.0) or 0.0)

            if not title or not snippet:
                continue

            key = (item.id, title, source)
            if key in seen:
                continue
            seen.add(key)

            explanations.append(
                {
                    "title": title,
                    "snippet": snippet,
                    "source": source,
                    "related_to": item.id,
                    "confidence": max(0.0, min(confidence, 1.0)),
                }
            )
            added_for_item += 1

            if added_for_item >= limit_per_item:
                break
            if len(explanations) >= max_total:
                return explanations

    return explanations[:max_total]


def run_retrieval_v2(user_message: str) -> RetrievalV2Result:
    try:
        plan = build_retrieval_plan(user_message)
    except Exception as exc:
        return _empty_result(user_message, error=str(exc), stage="planner")

    # 只做结构合并，不做语义理解：core_terms 由 AI 产生，代码只是传给 fetcher。
    # needs 是回答关注点，不是检索词；把它混入 keywords 会让“资料/方案”等泛词污染排序。
    try:
        plan.core_terms = list(plan.core_terms or [])

        for item in plan.source_plan:
            merged_keywords = list(item.keywords or [])

            for core_term in plan.core_terms:
                if core_term not in merged_keywords:
                    merged_keywords.append(core_term)

            item.keywords = merged_keywords

        if _should_trigger_kiwix(user_message, plan) and not _has_source_plan(plan, "kiwix"):
            plan.source_plan.append(
                _kiwix_plan_from_context(user_message, plan, reason="explicit_or_background_query")
            )
    except Exception as exc:
        return _empty_result(user_message, error=str(exc), stage="plan_normalization")

    try:
        candidates = fetch_candidates_from_plan(
            plan.source_plan,
            user_message=user_message,
            core_terms=plan.core_terms,
        )
        threshold = policy_int(("retrieval", "kiwix_fallback_primary_candidate_threshold"), 0)
        if not _has_source_plan(plan, "kiwix") and _primary_candidate_count(candidates) < threshold:
            kiwix_plan = _kiwix_plan_from_context(user_message, plan, reason="guide_wiki_insufficient")
            extra_candidates = fetch_candidates_from_plan(
                [kiwix_plan],
                user_message=user_message,
                core_terms=plan.core_terms,
            )
            if extra_candidates:
                plan.source_plan.append(kiwix_plan)
                candidates = _dedupe_candidates([*candidates, *extra_candidates])
    except Exception as exc:
        return _result_payload(
            plan=plan,
            candidates=[],
            selection=EvidenceSelection(
                selected=[],
                excluded=[],
                answer_focus=[],
                raw={"fallback": True, "error": str(exc), "stage": "fetch"},
            ),
            selected_evidence=[],
            debug=_debug_payload(plan=plan, ok=False, error=str(exc), stage="fetch"),
            kiwix_explanations=[],
        )

    try:
        selection = select_evidence_with_ai(
            user_message=user_message,
            plan=plan.model_dump(),
            candidates=candidates,
        )
    except Exception as exc:
        selection = _fallback_selection_from_candidates(
            candidates,
            error=str(exc),
            stage="selector",
        )

    selected_evidence = _validate_selected(candidates, selection.selected)
    selected_evidence, selection = _ensure_explicit_kiwix_selected(
        user_message=user_message,
        plan=plan,
        candidates=candidates,
        selected_evidence=selected_evidence,
        selection=selection,
    )
    selected_evidence, selection = _apply_usage_policy(
        selection=selection,
        candidates=candidates,
        selected_evidence=selected_evidence,
    )
    kiwix_explanations = _build_kiwix_explanations(
        plan=plan,
        selected_evidence=selected_evidence,
    )
    selected_sources, excluded_sources, retrieval_decision = _build_source_outputs(
        candidates=candidates,
        selected_evidence=selected_evidence,
        selection=selection,
    )

    return _result_payload(
        plan=plan,
        candidates=candidates,
        selection=selection,
        selected_evidence=selected_evidence,
        debug=_debug_payload(
            plan=plan,
            candidates=candidates,
            selected_evidence=selected_evidence,
            ok=not bool(selection.raw.get("fallback") if isinstance(selection.raw, dict) else False),
            error=str(selection.raw.get("error", "")) if isinstance(selection.raw, dict) else "",
            stage=str(selection.raw.get("stage", "")) if isinstance(selection.raw, dict) else "",
        ),
        kiwix_explanations=kiwix_explanations,
        selected_sources=selected_sources,
        excluded_sources=excluded_sources,
        retrieval_decision=retrieval_decision,
    )
