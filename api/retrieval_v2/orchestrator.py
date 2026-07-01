"""Retrieval v2 orchestrator."""

from typing import Dict, List, Tuple

from .fetchers import fetch_candidates_from_plan
from .planner import build_retrieval_plan
from .schemas import EvidenceCandidate, EvidenceSelection, RetrievalDebug, RetrievalPlan, RetrievalV2Result
from .selector import select_evidence_with_ai


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


def _debug_payload(
    *,
    plan: RetrievalPlan,
    candidates: List[EvidenceCandidate] | None = None,
    selected_evidence: List[EvidenceCandidate] | None = None,
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

    return RetrievalV2Result(
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
    )


def run_retrieval_v2(user_message: str) -> RetrievalV2Result:
    try:
        plan = build_retrieval_plan(user_message)
    except Exception as exc:
        return _empty_result(user_message, error=str(exc), stage="planner")

    # 只做结构合并，不做语义理解：needs/core_terms 由 AI 产生，代码只是传给 fetcher。
    try:
        plan.core_terms = list(plan.core_terms or [])

        for item in plan.source_plan:
            merged_keywords = list(item.keywords or [])

            for need in plan.needs:
                if need not in merged_keywords:
                    merged_keywords.append(need)

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
        return RetrievalV2Result(
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
        )

    try:
        selection = select_evidence_with_ai(
            user_message=user_message,
            plan=plan.model_dump(),
            candidates=candidates,
        )
    except Exception as exc:
        selection = EvidenceSelection(
            selected=[],
            excluded=[],
            answer_focus=[],
            raw={"fallback": True, "error": str(exc), "stage": "selector"},
        )

    selected_evidence = _validate_selected(candidates, selection.selected)

    return RetrievalV2Result(
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
    )
