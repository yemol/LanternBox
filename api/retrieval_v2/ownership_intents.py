"""Narrow ownership-intent guards for existing Retrieval candidates."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import List, Literal, Sequence

from pydantic import BaseModel, Field

from .contract_compare import infer_candidate_ownership
from .schemas import EvidenceCandidate


OwnershipIntentName = Literal[
    "long_term_energy_capacity_trend",
    "explicit_equipment_repair",
]
OwnershipLaneStatus = Literal[
    "NOT_APPLICABLE",
    "CONTRACT_PRECEDENCE",
    "APPLIED",
    "CANDIDATE_MISSING",
]

_HAZARD_TERMS = (
    "冒烟", "烟味", "浓烟", "火花", "过热", "异常热", "发烫", "烫手",
    "漏液", "泄漏", "明火", "起火", "燃烧", "电气暴露", "裸线", "漏电",
    "电击", "触电", "鼓包",
)
_HUMAN_HARM_TERMS = (
    "人员受伤", "有人受伤", "伤者", "烧伤", "电击伤", "吸入烟气", "流血",
)
_LONG_HORIZON_TERMS = (
    "多月", "数月", "几个月", "三个月", "半年", "长期", "跨季", "季节",
)
_ENERGY_OBJECT_TERMS = ("发电", "供能", "电量", "容量", "储能", "能源")
_ENERGY_CHANGE_TERMS = ("下降", "越来越低", "变化", "衰减", "减少", "不足", "趋势")
_ENERGY_REVIEW_TERMS = (
    "重新规划", "重新评估", "重新估算", "长期容量", "长期供能", "供能能力", "趋势判断",
)
_CURRENT_LOAD_TERMS = (
    "今天", "今晚", "晚上", "当前负载", "哪些负载", "关哪些", "负载要关",
)
_RECORD_ACTION_TERMS = (
    "怎么归档", "如何归档", "怎么整理", "如何整理", "标记可信度", "记录可信度", "日志归档",
)
_REPAIR_ACTION_TERMS = ("怎么修", "如何修", "先修", "维修", "修复", "检查故障", "排查故障")
_FAILURE_TERMS = ("坏了", "损坏", "故障", "不能工作", "无法工作", "不工作", "不运转", "失灵")
_PHYSICAL_EQUIPMENT_TERMS = (
    "设备", "机器", "工具", "水泵", "泵", "电机", "控制器", "充电器", "逆变器",
    "发电机", "阀门", "炉具", "插座", "线缆",
)
_MANUFACTURING_TERMS = ("制造一个", "制造新的", "制作一个", "制作新的", "从头制造", "加工一个")
_STORAGE_ACTION_TERMS = (
    "长期存放", "怎么存放", "如何存放", "长期保存", "怎么保存", "如何保存", "入库存放", "封存",
)
_RA_SECONDARY_TERMS = ("唯一", "只有这一", "只剩", "稀缺", "短缺", "替代", "降级", "延期")
_ENERGY_DEEPENING_GUIDE_TERMS = (
    "长期发电", "长期供能", "长期容量", "容量盘点", "容量下降", "容量复测",
    "跨季", "季节基线", "产出基线", "未来数日到数周",
)


def _contains_any(text: str, terms: Sequence[str]) -> bool:
    return any(term in text for term in terms)


def normalize_ownership_query(query: str) -> str:
    return re.sub(r"\s+", "", str(query or "")).casefold()


class OwnershipIntentDecision(BaseModel):
    intent: OwnershipIntentName
    primary_domain: str
    secondary_domains: List[str] = Field(default_factory=list)
    trigger_reason: str


class OwnershipIntentContext(BaseModel):
    status: OwnershipLaneStatus = "NOT_APPLICABLE"
    intent: str = ""
    primary_domain: str = ""
    secondary_domains: List[str] = Field(default_factory=list)
    trigger_reason: str = ""
    ownership_lane_candidate_missing: bool = False
    original_candidate_keys: List[str] = Field(default_factory=list)
    primary_lane_candidate_keys: List[str] = Field(default_factory=list)
    candidate_set_preserved: bool = True
    reason: str = ""


@dataclass
class OwnershipIntentLaneResult:
    candidates: List[EvidenceCandidate]
    context: OwnershipIntentContext
    decision: OwnershipIntentDecision | None = None


def detect_ownership_intent(query: str) -> OwnershipIntentDecision | None:
    text = normalize_ownership_query(query)
    if not text or _contains_any(text, (*_HAZARD_TERMS, *_HUMAN_HARM_TERMS)):
        return None

    if (
        _contains_any(text, _LONG_HORIZON_TERMS)
        and _contains_any(text, _ENERGY_OBJECT_TERMS)
        and _contains_any(text, _ENERGY_CHANGE_TERMS)
        and _contains_any(text, _ENERGY_REVIEW_TERMS)
        and not _contains_any(text, _CURRENT_LOAD_TERMS)
        and not _contains_any(text, _RECORD_ACTION_TERMS)
        and not _contains_any(text, _REPAIR_ACTION_TERMS)
    ):
        secondary = ["records"]
        if _contains_any(text, ("规划", "重新评估", "重新估算", "供能能力")):
            secondary.append("resource_allocation")
        return OwnershipIntentDecision(
            intent="long_term_energy_capacity_trend",
            primary_domain="energy_deepening",
            secondary_domains=secondary,
            trigger_reason="long horizon + energy capacity/generation change + reassessment",
        )

    if (
        _contains_any(text, _PHYSICAL_EQUIPMENT_TERMS)
        and _contains_any(text, _FAILURE_TERMS)
        and _contains_any(text, _REPAIR_ACTION_TERMS)
        and not _contains_any(text, _MANUFACTURING_TERMS)
        and not _contains_any(text, _RECORD_ACTION_TERMS)
        and not _contains_any(text, _STORAGE_ACTION_TERMS)
    ):
        secondary = ["tools"]
        if _contains_any(text, _RA_SECONDARY_TERMS):
            secondary.append("resource_allocation")
        return OwnershipIntentDecision(
            intent="explicit_equipment_repair",
            primary_domain="repair",
            secondary_domains=secondary,
            trigger_reason="failed physical equipment + explicit repair action",
        )

    return None


def _candidate_key(candidate: EvidenceCandidate) -> str:
    return f"{candidate.source_type}:{candidate.id}"


def _candidate_text(candidate: EvidenceCandidate) -> str:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    return " ".join(
        [
            str(candidate.title or ""), str(candidate.summary or ""),
            str(candidate.category or ""), str(candidate.snippet or ""),
            str(raw.get("scenario") or ""), str(raw.get("goal") or ""),
            " ".join(str(value) for value in raw.get("keywords", []) or []),
        ]
    ).casefold()


def _matches_primary_domain(candidate: EvidenceCandidate, decision: OwnershipIntentDecision) -> bool:
    if candidate.source_type != "guide":
        return False
    inferred = infer_candidate_ownership(candidate).primary_domain
    if inferred == decision.primary_domain:
        return True
    if decision.primary_domain != "energy_deepening":
        return False
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    category = str(raw.get("category") or candidate.category or "")
    domains = {str(value).casefold() for value in raw.get("domains", []) or []}
    text = _candidate_text(candidate)
    return (
        (category == "能源" or bool(domains & {"power", "energy"}))
        and _contains_any(text, _ENERGY_DEEPENING_GUIDE_TERMS)
        and not _contains_any(text, _HAZARD_TERMS)
    )


def _annotate_primary(candidate: EvidenceCandidate, decision: OwnershipIntentDecision) -> EvidenceCandidate:
    annotated = candidate.model_copy(deep=True)
    raw = annotated.raw if isinstance(annotated.raw, dict) else {}
    raw.update(
        {
            "ownership_intent_lane": "primary",
            "ownership_intent": decision.intent,
            "ownership_intent_owner": decision.primary_domain,
            "ownership_intent_secondary_domains": decision.secondary_domains,
        }
    )
    annotated.raw = raw
    return annotated


def apply_ownership_intent_candidates(
    query: str,
    candidates: Sequence[EvidenceCandidate],
    *,
    contract_status: str,
    contract_safety_boundary: bool = False,
) -> OwnershipIntentLaneResult:
    original = [candidate.model_copy(deep=True) for candidate in candidates]
    original_keys = [_candidate_key(candidate) for candidate in original]

    if contract_status == "CANARY_APPLIED" or contract_safety_boundary:
        return OwnershipIntentLaneResult(
            candidates=original,
            context=OwnershipIntentContext(
                status="CONTRACT_PRECEDENCE",
                original_candidate_keys=original_keys,
                reason="Contract or Safety ownership takes precedence",
            ),
        )

    if contract_status != "COMPARE_ONLY":
        return OwnershipIntentLaneResult(
            candidates=original,
            context=OwnershipIntentContext(
                original_candidate_keys=original_keys,
                reason="ownership intent is enabled only on Contract COMPARE_ONLY paths",
            ),
        )

    decision = detect_ownership_intent(query)
    if decision is None:
        return OwnershipIntentLaneResult(
            candidates=original,
            context=OwnershipIntentContext(
                original_candidate_keys=original_keys,
                reason="no normalized ownership intent matched",
            ),
        )

    primary_keys = {
        _candidate_key(candidate)
        for candidate in original
        if _matches_primary_domain(candidate, decision)
    }
    if not primary_keys:
        return OwnershipIntentLaneResult(
            candidates=original,
            decision=decision,
            context=OwnershipIntentContext(
                status="CANDIDATE_MISSING",
                intent=decision.intent,
                primary_domain=decision.primary_domain,
                secondary_domains=decision.secondary_domains,
                trigger_reason=decision.trigger_reason,
                ownership_lane_candidate_missing=True,
                original_candidate_keys=original_keys,
                reason="ownership intent matched but no primary-domain Guide candidate exists",
            ),
        )

    primary: list[EvidenceCandidate] = []
    remainder: list[EvidenceCandidate] = []
    for candidate in original:
        if _candidate_key(candidate) in primary_keys:
            primary.append(_annotate_primary(candidate, decision))
        else:
            remainder.append(candidate)
    ordered = [*primary, *remainder]
    ordered_keys = [_candidate_key(candidate) for candidate in ordered]
    return OwnershipIntentLaneResult(
        candidates=ordered,
        decision=decision,
        context=OwnershipIntentContext(
            status="APPLIED",
            intent=decision.intent,
            primary_domain=decision.primary_domain,
            secondary_domains=decision.secondary_domains,
            trigger_reason=decision.trigger_reason,
            original_candidate_keys=original_keys,
            primary_lane_candidate_keys=[_candidate_key(candidate) for candidate in primary],
            candidate_set_preserved=(set(ordered_keys) == set(original_keys)),
            reason="existing primary-domain candidates moved to ownership lane",
        ),
    )
