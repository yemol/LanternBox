"""Retrieval v2 orchestrator."""

from typing import Dict, List, Tuple

from .fetchers import fetch_candidates_from_plan
from .planner import build_retrieval_plan
from .schemas import EvidenceCandidate, RetrievalV2Result
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


def run_retrieval_v2(user_message: str) -> RetrievalV2Result:
    plan = build_retrieval_plan(user_message)

    # 只做结构合并，不做语义理解：needs/core_terms 由 AI 产生，代码只是传给 fetcher。
    for item in plan.source_plan:
        merged_keywords = list(item.keywords or [])

        for need in plan.needs:
            if need not in merged_keywords:
                merged_keywords.append(need)

        for core_term in plan.core_terms:
            if core_term not in merged_keywords:
                merged_keywords.append(core_term)

        item.keywords = merged_keywords

    candidates = fetch_candidates_from_plan(
        plan.source_plan,
        user_message=user_message,
        core_terms=plan.core_terms,
    )

    selection = select_evidence_with_ai(
        user_message=user_message,
        plan=plan.model_dump(),
        candidates=candidates,
    )

    selected_evidence = _validate_selected(candidates, selection.selected)

    debug = {
        "candidate_count": len(candidates),
        "selected_count": len(selected_evidence),
        "source_plan_count": len(plan.source_plan),
        "candidate_types": _candidate_type_counts(candidates),
        "core_terms": list(plan.core_terms or []),
    }

    return RetrievalV2Result(
        plan=plan.model_dump(),
        candidates=[item.model_dump() for item in candidates],
        selection=selection.model_dump(),
        selected_evidence=[item.model_dump() for item in selected_evidence],
        debug=debug,
    )
