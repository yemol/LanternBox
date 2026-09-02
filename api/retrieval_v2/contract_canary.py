"""Controlled production ownership lanes for the three Batch16 canary transitions."""

from __future__ import annotations

import json
import logging
from dataclasses import dataclass
from typing import Dict, List, Literal, Optional, Sequence

from pydantic import BaseModel, Field

from .contract_compare import (
    RetrievalOwnership,
    infer_candidate_ownership,
    original_retrieval_ownership,
)
from .contract_runtime import (
    ContractRule,
    OwnershipDecision,
    build_contract_audit_event,
    evaluate_ownership_contract,
    get_contract_canary_settings,
    get_contract_mode,
    load_contract_source,
    normalize_contract_mode,
)
from .fetchers import fetch_guide_candidates
from .schemas import (
    EvidenceCandidate,
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
    SourcePlanItem,
)


LOGGER = logging.getLogger("lanternbox.cross_domain_contract")
CanaryStatus = Literal["NOT_APPLICABLE", "COMPARE_ONLY", "CANARY_APPLIED"]


class CanaryTransition(BaseModel):
    transition_id: str
    rule_ids: List[str]
    original_primary_domains: List[str]
    contract_primary_domain: str


CANARY_TRANSITIONS = (
    CanaryTransition(
        transition_id="active_hazard_to_energy_safety",
        rule_ids=["energy_device_abnormal"],
        original_primary_domains=[
            "energy_management",
            "energy_other",
            "repair",
            "manufacturing",
            "fire",
        ],
        contract_primary_domain="energy_safety",
    ),
    CanaryTransition(
        transition_id="long_term_energy_to_energy_deepening",
        rule_ids=["solar_seasonal_baseline"],
        original_primary_domains=["energy_management", "energy_other", "repair"],
        contract_primary_domain="energy_deepening",
    ),
    CanaryTransition(
        transition_id="information_object_to_records",
        rule_ids=["information_lifecycle"],
        original_primary_domains=["repair", "manufacturing", "tools", "agriculture"],
        contract_primary_domain="records",
    ),
)


class CanaryContext(BaseModel):
    mode: str
    canary: bool = False
    status: CanaryStatus = "NOT_APPLICABLE"
    transition_id: str = ""
    matched_rule: str = ""
    original_primary_domain: str = ""
    contract_primary_domain: str = ""
    mandatory_secondary_domains: List[str] = Field(default_factory=list)
    suppressed_primary_domains: List[str] = Field(default_factory=list)
    ownership_changed: bool = False
    primary_candidate_ids: List[str] = Field(default_factory=list)
    safety_boundary_present: bool = False
    reason: str = ""

    def public_payload(self) -> dict[str, object]:
        return self.model_dump(exclude={"primary_candidate_ids"})


@dataclass
class CanaryLaneResult:
    candidates: List[EvidenceCandidate]
    context: CanaryContext
    decision: Optional[OwnershipDecision] = None


def _candidate_key(candidate: EvidenceCandidate) -> tuple[str, str]:
    return candidate.source_type, candidate.id


def _dedupe_candidates(candidates: Sequence[EvidenceCandidate]) -> list[EvidenceCandidate]:
    results: list[EvidenceCandidate] = []
    seen: set[tuple[str, str]] = set()
    for candidate in candidates:
        key = _candidate_key(candidate)
        if key in seen:
            continue
        seen.add(key)
        results.append(candidate)
    return results


def _transition_for(
    decision: OwnershipDecision,
    retrieval: RetrievalOwnership,
    enabled_transition_ids: Sequence[str],
) -> Optional[CanaryTransition]:
    enabled = set(enabled_transition_ids)
    for transition in CANARY_TRANSITIONS:
        if transition.transition_id not in enabled:
            continue
        if decision.matched_rule not in transition.rule_ids:
            continue
        if decision.primary_domain != transition.contract_primary_domain:
            continue
        if retrieval.primary_domain not in transition.original_primary_domains:
            continue
        return transition
    return None


def _rule_for(decision: OwnershipDecision) -> Optional[ContractRule]:
    return next(
        (
            rule
            for rule in load_contract_source().source.rules
            if rule.id == decision.matched_rule
        ),
        None,
    )


def _guide_anchor_text(guide: dict[str, object]) -> str:
    return " ".join(
        [
            str(guide.get("title") or ""),
            str(guide.get("category") or ""),
            str(guide.get("scenario") or ""),
            str(guide.get("goal") or ""),
            " ".join(str(value) for value in guide.get("keywords", []) or []),
        ]
    ).casefold()


def _guide_matches_primary_domain(
    guide: dict[str, object],
    primary_domain: str,
) -> bool:
    category = str(guide.get("category") or "")
    domains = {
        str(value).strip().lower()
        for value in guide.get("domains", []) or []
    }
    if primary_domain in {"energy_safety", "energy_deepening"}:
        return category == "能源" or "power" in domains or "energy" in domains
    if primary_domain == "records":
        return "信息保存与长期重建" in category and "records" in domains
    return False


def _guide_matches_positive_rule_anchors(
    guide: dict[str, object],
    rule: ContractRule,
) -> bool:
    text = _guide_anchor_text(guide)
    return all(
        not clause.any_terms
        or any(str(term).casefold() in text for term in clause.any_terms)
        for clause in rule.conditions.all
    )


def _guide_filter(decision: OwnershipDecision):
    rule = _rule_for(decision)
    if rule is None:
        return lambda guide: False
    return lambda guide: (
        _guide_matches_primary_domain(guide, decision.primary_domain)
        and _guide_matches_positive_rule_anchors(guide, rule)
    )


def _guide_plan(plan: RetrievalPlan, query: str) -> SourcePlanItem:
    existing = next(
        (item for item in plan.source_plan if item.source_type == "guide"),
        None,
    )
    if existing is not None:
        return existing.model_copy(deep=True)
    return SourcePlanItem(source_type="guide", query=query, limit=8)


def _has_safety_boundary(candidate: EvidenceCandidate) -> bool:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    boundary = " ".join(
        str(value)
        for value in raw.get("stop_or_escalate", []) or []
    )
    return bool(boundary) and any(
        marker in boundary
        for marker in (
            "STOP",
            "停止",
            "停用",
            "断开",
            "隔离",
            "撤离",
            "远离",
            "禁止",
            "不得",
            "不可",
        )
    )


def _annotate(
    candidate: EvidenceCandidate,
    lane: str,
    owner: str,
    transition_id: str,
) -> EvidenceCandidate:
    annotated = candidate.model_copy(deep=True)
    raw = annotated.raw if isinstance(annotated.raw, dict) else {}
    raw.update(
        {
            "contract_canary": True,
            "contract_canary_lane": lane,
            "contract_canary_owner": owner,
            "contract_canary_transition": transition_id,
        }
    )
    annotated.raw = raw
    return annotated


def _candidate_owner(candidate: EvidenceCandidate) -> str:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    canary_owner = str(raw.get("contract_canary_owner") or "")
    return canary_owner or infer_candidate_ownership(candidate).primary_domain


def _log_canary_event(
    query: str,
    result: CanaryLaneResult,
    retrieval: RetrievalOwnership,
) -> None:
    if result.decision is None or result.context.mode != "production":
        return
    event = build_contract_audit_event(
        query,
        result.decision,
        mode="production",
        original_retrieval_ownership=retrieval.model_dump(),
        difference_type="A" if result.context.ownership_changed else "C",
        canary=result.context.status == "CANARY_APPLIED",
        canary_status=result.context.status,
        limited_scope=(
            result.context.status == "CANARY_APPLIED"
        ),
        fallback_used=result.context.status == "COMPARE_ONLY",
        original_primary_domain=retrieval.primary_domain,
        ownership_changed=result.context.ownership_changed,
    )
    LOGGER.info(
        "cross_domain_contract_audit %s",
        json.dumps(event.model_dump(), ensure_ascii=False, sort_keys=True),
    )


def _compare_only(
    query: str,
    candidates: list[EvidenceCandidate],
    decision: OwnershipDecision,
    retrieval: RetrievalOwnership,
    *,
    reason: str,
) -> CanaryLaneResult:
    result = CanaryLaneResult(
        candidates=candidates,
        decision=decision,
        context=CanaryContext(
            mode="production",
            status="COMPARE_ONLY",
            matched_rule=decision.matched_rule,
            original_primary_domain=retrieval.primary_domain,
            contract_primary_domain=decision.primary_domain,
            mandatory_secondary_domains=decision.mandatory_secondary_domains,
            suppressed_primary_domains=decision.suppressed_domains,
            reason=reason,
        ),
    )
    _log_canary_event(query, result, retrieval)
    return result


def apply_contract_canary_candidates(
    query: str,
    plan: RetrievalPlan,
    candidates: Sequence[EvidenceCandidate],
    *,
    mode: Optional[str] = None,
    canary_enabled: Optional[bool] = None,
    enabled_transition_ids: Optional[Sequence[str]] = None,
) -> CanaryLaneResult:
    effective_mode = (
        normalize_contract_mode(mode)
        if mode is not None
        else get_contract_mode()
    )
    runtime_enabled, runtime_transitions = get_contract_canary_settings()
    enabled = runtime_enabled if canary_enabled is None else bool(canary_enabled)
    transition_ids = (
        runtime_transitions
        if enabled_transition_ids is None
        else list(enabled_transition_ids)
    )
    untouched = [
        candidate.model_copy(deep=True)
        for candidate in candidates
    ]

    if effective_mode != "production" or not enabled:
        return CanaryLaneResult(
            candidates=untouched,
            context=CanaryContext(
                mode=effective_mode,
                reason="canary disabled outside production",
            ),
        )

    decision = evaluate_ownership_contract(query)
    retrieval = original_retrieval_ownership(untouched)
    transition = _transition_for(decision, retrieval, transition_ids)
    if transition is None:
        return _compare_only(
            query,
            untouched,
            decision,
            retrieval,
            reason="decision is outside the enabled canary transitions",
        )

    primary_candidates = fetch_guide_candidates(
        _guide_plan(plan, query),
        user_message=query,
        core_terms=plan.core_terms,
        guide_filter=_guide_filter(decision),
        apply_query_profiles=False,
    )
    primary_candidates = [
        _annotate(
            candidate,
            "primary",
            decision.primary_domain,
            transition.transition_id,
        )
        for candidate in primary_candidates
    ]

    if decision.safety_gate.required:
        safe = [
            candidate
            for candidate in primary_candidates
            if _has_safety_boundary(candidate)
        ]
        unsafe = [
            candidate
            for candidate in primary_candidates
            if not _has_safety_boundary(candidate)
        ]
        primary_candidates = [*safe, *unsafe]
        if not safe:
            return _compare_only(
                query,
                untouched,
                decision,
                retrieval,
                reason=(
                    "safety gate blocked canary because "
                    "no stop boundary was recalled"
                ),
            )

    if not primary_candidates:
        return _compare_only(
            query,
            untouched,
            decision,
            retrieval,
            reason="no candidate matched the Contract primary lane",
        )

    mandatory_domains = set(decision.mandatory_secondary_domains)
    suppressed_domains = set(decision.suppressed_domains)
    mandatory: list[EvidenceCandidate] = []
    neutral: list[EvidenceCandidate] = []
    suppressed: list[EvidenceCandidate] = []
    for candidate in untouched:
        owner = _candidate_owner(candidate)
        if owner in mandatory_domains:
            mandatory.append(
                _annotate(
                    candidate,
                    "mandatory_secondary",
                    owner,
                    transition.transition_id,
                )
            )
        elif owner in suppressed_domains:
            suppressed.append(
                _annotate(
                    candidate,
                    "suppressed_primary",
                    owner,
                    transition.transition_id,
                )
            )
        else:
            neutral.append(candidate)

    ordered = _dedupe_candidates(
        [*primary_candidates, *mandatory, *neutral, *suppressed]
    )
    result = CanaryLaneResult(
        candidates=ordered,
        decision=decision,
        context=CanaryContext(
            mode=effective_mode,
            canary=True,
            status="CANARY_APPLIED",
            transition_id=transition.transition_id,
            matched_rule=decision.matched_rule,
            original_primary_domain=retrieval.primary_domain,
            contract_primary_domain=decision.primary_domain,
            mandatory_secondary_domains=decision.mandatory_secondary_domains,
            suppressed_primary_domains=decision.suppressed_domains,
            ownership_changed=True,
            primary_candidate_ids=[
                candidate.id
                for candidate in primary_candidates
            ],
            safety_boundary_present=(
                decision.safety_gate.required
                and any(
                    _has_safety_boundary(candidate)
                    for candidate in primary_candidates
                )
            ),
            reason="structured Contract ownership transition applied",
        ),
    )
    _log_canary_event(query, result, retrieval)
    return result


def apply_contract_canary_selection(
    lane_result: CanaryLaneResult,
    selected_evidence: Sequence[EvidenceCandidate],
    selection: EvidenceSelection,
) -> tuple[list[EvidenceCandidate], EvidenceSelection]:
    if lane_result.context.status != "CANARY_APPLIED":
        return list(selected_evidence), selection

    primary = next(
        (
            candidate
            for candidate in lane_result.candidates
            if isinstance(candidate.raw, dict)
            and candidate.raw.get("contract_canary_lane") == "primary"
        ),
        None,
    )
    if primary is None:
        return list(selected_evidence), selection

    mandatory: list[EvidenceCandidate] = []
    for domain in lane_result.context.mandatory_secondary_domains:
        candidate = next(
            (
                item
                for item in lane_result.candidates
                if isinstance(item.raw, dict)
                and item.raw.get("contract_canary_lane")
                == "mandatory_secondary"
                and item.raw.get("contract_canary_owner") == domain
            ),
            None,
        )
        if candidate is not None:
            mandatory.append(candidate)

    ordered = _dedupe_candidates(
        [primary, *mandatory, *selected_evidence]
    )
    old_reasons: Dict[tuple[str, str], str] = {
        (item.source_type, item.id): item.reason
        for item in selection.selected
    }
    selection.selected = [
        SelectedEvidence(
            source_type=candidate.source_type,
            id=candidate.id,
            reason=(
                "Contract canary primary ownership lane."
                if candidate is primary
                else "Contract canary mandatory-secondary lane."
                if candidate in mandatory
                else old_reasons.get(_candidate_key(candidate), "")
            ),
        )
        for candidate in ordered
    ]
    raw = selection.raw if isinstance(selection.raw, dict) else {}
    raw["contract_canary"] = lane_result.context.public_payload()
    selection.raw = raw
    return ordered, selection
