"""Retrieval v2 orchestrator."""

from typing import Dict, List, Optional, Tuple

from pydantic import Field

from .fetchers import fetch_candidates_from_plan
from .planner import build_retrieval_plan
from .schemas import EvidenceCandidate, EvidenceSelection, RetrievalDebug, RetrievalPlan, RetrievalV2Result, SelectedEvidence
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
) -> RetrievalV2ResultWithKiwix:
    return RetrievalV2ResultWithKiwix(
        plan=plan,
        candidates=candidates or [],
        selection=selection,
        selected_evidence=selected_evidence or [],
        debug=debug,
        kiwix_explanations=kiwix_explanations or [],
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
    except Exception as exc:
        return _empty_result(user_message, error=str(exc), stage="plan_normalization")

    try:
        candidates = fetch_candidates_from_plan(
            plan.source_plan,
            user_message=user_message,
            core_terms=plan.core_terms,
        )
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
    kiwix_explanations = _build_kiwix_explanations(
        plan=plan,
        selected_evidence=selected_evidence,
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
    )
