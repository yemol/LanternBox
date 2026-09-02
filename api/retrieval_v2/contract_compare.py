"""Read-only comparison of Contract and existing Retrieval ownership."""

from __future__ import annotations

import json
import logging
from datetime import datetime, timezone
from typing import List, Literal, Optional, Sequence

from pydantic import BaseModel, Field

from .contract_runtime import (
    OwnershipDecision,
    build_contract_audit_event,
    evaluate_ownership_contract,
    get_contract_mode,
)
from .schemas import EvidenceCandidate


COMPARE_VERSION = "1.0"
LOGGER = logging.getLogger("lanternbox.cross_domain_contract")
DifferenceType = Literal["A", "B", "C", "D"]


class RetrievalOwnership(BaseModel):
    primary_domain: str
    co_domains: List[str] = Field(default_factory=list)
    source_type: str = ""
    source_id: str = ""
    source_title: str = ""
    source_category: str = ""


class OwnershipComparison(BaseModel):
    query: str
    compare_version: str = COMPARE_VERSION
    original_retrieval_ownership: RetrievalOwnership
    contract_ownership_decision: OwnershipDecision
    difference_type: DifferenceType
    difference_reason: str
    timestamp: str


def _as_list(value: object) -> list[object]:
    if isinstance(value, list):
        return value
    if value in (None, ""):
        return []
    return [value]


def _contains_any(text: str, terms: Sequence[str]) -> bool:
    return any(term in text for term in terms)


def infer_candidate_ownership(candidate: EvidenceCandidate) -> RetrievalOwnership:
    """Infer the existing Guide owner from its current metadata and display text."""
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    title = str(candidate.title or "")
    category = str(raw.get("category") or candidate.category or "")
    domains = {
        str(value).strip().lower()
        for value in _as_list(raw.get("domains"))
        if str(value).strip()
    }
    combined = f"{title} {category}"

    primary = "other"
    co_domains: list[str] = []

    if _contains_any(title, ("实际容量下降", "容量复测", "跨季", "季节基线", "产出基线")):
        primary = "energy_deepening"
    elif _contains_any(title, ("电池异常", "异常隔离", "异常停用", "能源安全", "电气安全")):
        primary = "energy_safety"
    elif _contains_any(title, ("白天排程", "每日能源预算", "电量分配", "负载调度", "充电队列")):
        primary = "energy_management"
    elif _contains_any(combined, ("医疗", "急救", "止血", "伤口", "出血", "烧烫伤", "发热")):
        primary = "medical"
    elif domains & {"agriculture", "planting"} or _contains_any(category, ("农业", "种植", "食物生产")):
        primary = "agriculture"
    elif _contains_any(title, ("储藏", "库存", "入库", "防潮防锈", "长期保存")):
        primary = "storage"
    elif _contains_any(title, ("信息可信度", "归档", "知识版本", "重复问题与经验", "长期趋势")) or "信息保存" in category:
        primary = "records"
    elif _contains_any(combined, ("火灾", "明火", "灭火", "火源")) or "fire" in domains:
        primary = "fire"
    elif "production" in domains or _contains_any(category, ("生产系统", "工坊")):
        primary = "production"
    elif "manufacturing" in domains or _contains_any(category, ("基础制造", "制造")):
        primary = "manufacturing"
    elif domains & {"repair", "tools"} or _contains_any(category, ("工具维修", "维修")):
        primary = "repair"
        if "tools" in domains:
            co_domains.append("tools")
    elif "medical" in domains:
        primary = "medical"
    elif "records" in domains:
        primary = "records"
    elif domains & {"storage", "inventory"}:
        primary = "storage"
    elif domains & {"power", "energy"} or "能源" in category:
        primary = "energy_other"

    return RetrievalOwnership(
        primary_domain=primary,
        co_domains=co_domains,
        source_type=str(candidate.source_type or ""),
        source_id=str(candidate.id or ""),
        source_title=title,
        source_category=category,
    )


def original_retrieval_ownership(candidates: Sequence[EvidenceCandidate]) -> RetrievalOwnership:
    guide = next((candidate for candidate in candidates if candidate.source_type == "guide"), None)
    if guide is None:
        return RetrievalOwnership(primary_domain="unassigned")
    return infer_candidate_ownership(guide)


def _runtime_compatible(
    decision: OwnershipDecision,
    retrieval: RetrievalOwnership,
) -> bool:
    retrieval_domains = {retrieval.primary_domain, *retrieval.co_domains}
    if decision.primary_domain == "unassigned":
        return True
    if decision.primary_domain in retrieval_domains:
        return True
    if retrieval_domains & set(decision.mandatory_secondary_domains):
        return True
    if (
        decision.trigger_reason.startswith("biological_material_storage")
        and "agriculture" in retrieval_domains
    ):
        return True
    return False


def _classify_difference(
    decision: OwnershipDecision,
    retrieval: RetrievalOwnership,
    *,
    expected_contract_primary: Optional[str] = None,
    accepted_retrieval_primary: Optional[Sequence[str]] = None,
) -> tuple[DifferenceType, str]:
    if expected_contract_primary is not None and accepted_retrieval_primary is not None:
        contract_correct = decision.primary_domain == expected_contract_primary
        retrieval_domains = {retrieval.primary_domain, *retrieval.co_domains}
        retrieval_correct = bool(retrieval_domains & set(accepted_retrieval_primary))
        if contract_correct and retrieval_correct:
            return "C", "fixture_adjudication: both ownership decisions are acceptable"
        if contract_correct:
            return "A", "fixture_adjudication: Contract identifies existing Retrieval debt"
        if retrieval_correct:
            return "B", "fixture_adjudication: Retrieval is better than Contract"
        return "D", "fixture_adjudication: Contract error and Retrieval is not accepted"

    if _runtime_compatible(decision, retrieval):
        return "C", "runtime_comparison: exact, supplemental, or stable-domain passthrough"
    if decision.primary_domain != "unassigned":
        return "A", "runtime_comparison: matched Contract owner differs from existing Retrieval owner"
    return "B", "runtime_comparison: Retrieval has an owner while Contract is unassigned"


def evaluate_ownership_comparison(
    query: str,
    candidates: Sequence[EvidenceCandidate],
    *,
    expected_contract_primary: Optional[str] = None,
    accepted_retrieval_primary: Optional[Sequence[str]] = None,
) -> OwnershipComparison:
    decision = evaluate_ownership_contract(query)
    retrieval = original_retrieval_ownership(candidates)
    difference_type, reason = _classify_difference(
        decision,
        retrieval,
        expected_contract_primary=expected_contract_primary,
        accepted_retrieval_primary=accepted_retrieval_primary,
    )
    return OwnershipComparison(
        query=str(query or ""),
        original_retrieval_ownership=retrieval,
        contract_ownership_decision=decision,
        difference_type=difference_type,
        difference_reason=reason,
        timestamp=datetime.now(timezone.utc).isoformat(),
    )


def record_compare_evaluation(
    query: str,
    candidates: Sequence[EvidenceCandidate],
    *,
    mode: Optional[str] = None,
) -> Optional[OwnershipComparison]:
    effective_mode = str(mode or get_contract_mode()).strip().lower()
    if effective_mode != "compare":
        return None

    try:
        comparison = evaluate_ownership_comparison(query, candidates)
        event = build_contract_audit_event(
            query,
            comparison.contract_ownership_decision,
            mode="compare",
            original_retrieval_ownership=comparison.original_retrieval_ownership.model_dump(),
            difference_type=comparison.difference_type,
            canary_status="COMPARE_ONLY",
            original_primary_domain=comparison.original_retrieval_ownership.primary_domain,
        )
        LOGGER.info(
            "cross_domain_contract_audit %s",
            json.dumps(event.model_dump(), ensure_ascii=False, sort_keys=True),
        )
        return comparison
    except Exception:
        LOGGER.exception("cross_domain_contract_compare_error")
        return None
