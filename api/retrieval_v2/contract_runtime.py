"""Single-source Cross-Domain Contract evaluator and read-only runtime hooks."""

from __future__ import annotations

import hashlib
import json
import logging
import re
import unicodedata
import uuid
from dataclasses import dataclass
from datetime import datetime, timezone
from functools import lru_cache
from pathlib import Path
from typing import Dict, List, Literal, Optional, Sequence

from pydantic import BaseModel, Field

from ..config import BASE_DIR, load_runtime_settings


CONTRACT_PATH = BASE_DIR / "data/cross_domain_contracts.json"
SUPPORTED_SCHEMA_VERSION = "1.1"
ALLOWED_CONTRACT_MODES = {"off", "shadow", "compare", "production"}
LOGGER = logging.getLogger("lanternbox.cross_domain_contract")


class TermClause(BaseModel):
    any_terms: List[str] = Field(default_factory=list)
    mask_phrases: List[str] = Field(default_factory=list)


class RuleConditions(BaseModel):
    all: List[TermClause] = Field(default_factory=list)
    none: List[TermClause] = Field(default_factory=list)


class SecondaryContext(BaseModel):
    conditions: RuleConditions
    domains: List[str] = Field(default_factory=list)
    position: Literal["append", "prepend"] = "append"


class SafetyGateSpec(BaseModel):
    required: bool = False
    gate_type: str = "none"
    prohibited_actions: List[str] = Field(default_factory=list)


class ContractRule(BaseModel):
    id: str
    family: str
    priority: int
    conditions: RuleConditions
    primary_domain: str
    mandatory_secondary_domains: List[str] = Field(default_factory=list)
    suppressed_domains: List[str] = Field(default_factory=list)
    contract_reason: str
    confidence: float = Field(ge=0.0, le=1.0)
    safety_gate: SafetyGateSpec = Field(default_factory=SafetyGateSpec)
    secondary_by_context: List[SecondaryContext] = Field(default_factory=list)


class AuditPolicy(BaseModel):
    event_version: str
    retention_days: int = Field(gt=0)
    retention_class: str
    contains_full_query: bool = False
    query_identifier: Literal["sha256"] = "sha256"


class ContractSource(BaseModel):
    schema_version: str
    contract_version: str
    status: str
    runtime_integration: bool
    default_mode: str
    description: str
    contract_precedence: List[str]
    known_domains: List[str]
    audit_policy: AuditPolicy
    rules: List[ContractRule]


@dataclass(frozen=True)
class LoadedContractSource:
    source: ContractSource
    checksum: str
    path: Path


class SafetyGateDecision(BaseModel):
    required: bool = False
    gate_type: str = "none"
    mode: Literal["observe_only"] = "observe_only"
    status: Literal["required", "not_applicable"] = "not_applicable"
    required_primary_domains: List[str] = Field(default_factory=list)
    required_secondary_domains: List[str] = Field(default_factory=list)
    prohibited_actions: List[str] = Field(default_factory=list)


class OwnershipLanes(BaseModel):
    primary: List[str] = Field(default_factory=list)
    mandatory_secondary: List[str] = Field(default_factory=list)
    suppressed: List[str] = Field(default_factory=list)


class OwnershipDecision(BaseModel):
    query: str
    normalized_query: str
    contract_version: str
    contract_checksum: str
    matched_rule: str = ""
    decision_status: Literal["matched", "not_matched"]
    primary_domain: str
    mandatory_secondary_domains: List[str] = Field(default_factory=list)
    suppressed_domains: List[str] = Field(default_factory=list)
    ownership_lanes: OwnershipLanes
    safety_gate: SafetyGateDecision
    contract_reason: str
    trigger_reason: str
    confidence: float = Field(ge=0.0, le=1.0)
    timestamp: str


class ContractAuditEvent(BaseModel):
    event_version: str
    request_id: str
    timestamp: str
    contract_mode: str
    contract_version: str
    contract_checksum: str
    query_sha256: str
    query_length: int
    contains_full_query: Literal[False] = False
    retention_days: int
    retention_class: str
    matched_rule: str
    decision_status: str
    primary_domain: str
    mandatory_secondary_domains: List[str] = Field(default_factory=list)
    suppressed_domains: List[str] = Field(default_factory=list)
    safety_gate: SafetyGateDecision
    contract_reason: str
    original_retrieval_ownership: Optional[Dict[str, object]] = None
    difference_type: str = ""
    canary: bool = False
    canary_status: Literal["NOT_APPLICABLE", "COMPARE_ONLY", "CANARY_APPLIED"] = "NOT_APPLICABLE"
    original_primary_domain: str = ""
    limited_scope: bool = False
    rule_family: str = ""
    fallback_used: bool = False
    contract_primary_domain: str = ""
    suppressed_primary_domains: List[str] = Field(default_factory=list)
    ownership_changed: bool = False


def normalize_contract_query(query: str) -> str:
    normalized = unicodedata.normalize("NFKC", str(query or ""))
    return re.sub(r"\s+", " ", normalized).strip()


def _timestamp() -> str:
    return datetime.now(timezone.utc).isoformat()


def _dedupe(values: Sequence[str]) -> list[str]:
    return list(dict.fromkeys(str(value) for value in values if str(value)))


def _contains_any(text: str, terms: Sequence[str]) -> bool:
    folded = text.casefold()
    return any(str(term).casefold() in folded for term in terms)


def _mask_phrases(text: str, phrases: Sequence[str]) -> str:
    masked = text
    for phrase in sorted((str(item) for item in phrases), key=len, reverse=True):
        masked = masked.replace(phrase, " " * len(phrase))
    return masked


def _clause_matches(text: str, clause: TermClause) -> bool:
    scoped = _mask_phrases(text, clause.mask_phrases)
    return not clause.any_terms or _contains_any(scoped, clause.any_terms)


def _conditions_match(text: str, conditions: RuleConditions) -> bool:
    if not all(_clause_matches(text, clause) for clause in conditions.all):
        return False
    return not any(_clause_matches(text, clause) for clause in conditions.none)


def _validate_contract_source(source: ContractSource) -> None:
    if source.schema_version != SUPPORTED_SCHEMA_VERSION:
        raise ValueError(f"unsupported contract schema: {source.schema_version}")
    if not source.runtime_integration:
        raise ValueError("contract source is not enabled for runtime integration")
    if source.default_mode != "off":
        raise ValueError("contract source default_mode must remain off")
    if source.audit_policy.contains_full_query:
        raise ValueError("contract audit policy must not retain full queries")
    if not source.rules:
        raise ValueError("contract source contains no rules")

    known = set(source.known_domains)
    rule_ids = [rule.id for rule in source.rules]
    if len(rule_ids) != len(set(rule_ids)):
        raise ValueError("duplicate contract rule id")
    for rule in source.rules:
        referenced = {
            rule.primary_domain,
            *rule.mandatory_secondary_domains,
            *rule.suppressed_domains,
        }
        for context in rule.secondary_by_context:
            referenced.update(context.domains)
        unknown = referenced - known
        if unknown:
            raise ValueError(f"rule {rule.id} references unknown domains: {sorted(unknown)}")


@lru_cache(maxsize=4)
def _load_contract_source(path_text: str) -> LoadedContractSource:
    path = Path(path_text)
    raw_bytes = path.read_bytes()
    raw = json.loads(raw_bytes.decode("utf-8"))
    source = ContractSource.model_validate(raw)
    _validate_contract_source(source)
    return LoadedContractSource(
        source=source,
        checksum=hashlib.sha256(raw_bytes).hexdigest(),
        path=path,
    )


def load_contract_source(path: Optional[Path] = None) -> LoadedContractSource:
    return _load_contract_source(str(path or CONTRACT_PATH))


def clear_contract_source_cache() -> None:
    _load_contract_source.cache_clear()


def _secondary_domains(rule: ContractRule, text: str) -> list[str]:
    secondary = list(rule.mandatory_secondary_domains)
    for context in rule.secondary_by_context:
        if not _conditions_match(text, context.conditions):
            continue
        if context.position == "prepend":
            secondary = [*context.domains, *secondary]
        else:
            secondary.extend(context.domains)
    return _dedupe(secondary)


def _safety_gate(rule: ContractRule, secondary: Sequence[str]) -> SafetyGateDecision:
    spec = rule.safety_gate
    if not spec.required:
        return SafetyGateDecision()
    return SafetyGateDecision(
        required=True,
        gate_type=spec.gate_type,
        status="required",
        required_primary_domains=[rule.primary_domain],
        required_secondary_domains=list(secondary),
        prohibited_actions=list(spec.prohibited_actions),
    )


def evaluate_ownership_contract(query: str) -> OwnershipDecision:
    original = str(query or "")
    text = normalize_contract_query(original)
    loaded = load_contract_source()

    for rule in sorted(loaded.source.rules, key=lambda item: item.priority, reverse=True):
        if not _conditions_match(text, rule.conditions):
            continue
        secondary = _secondary_domains(rule, text)
        suppressed = _dedupe(rule.suppressed_domains)
        return OwnershipDecision(
            query=original,
            normalized_query=text,
            contract_version=loaded.source.contract_version,
            contract_checksum=loaded.checksum,
            matched_rule=rule.id,
            decision_status="matched",
            primary_domain=rule.primary_domain,
            mandatory_secondary_domains=secondary,
            suppressed_domains=suppressed,
            ownership_lanes=OwnershipLanes(
                primary=[rule.primary_domain],
                mandatory_secondary=secondary,
                suppressed=suppressed,
            ),
            safety_gate=_safety_gate(rule, secondary),
            contract_reason=rule.contract_reason,
            trigger_reason=f"{rule.id}: {rule.contract_reason}",
            confidence=rule.confidence,
            timestamp=_timestamp(),
        )

    return OwnershipDecision(
        query=original,
        normalized_query=text,
        contract_version=loaded.source.contract_version,
        contract_checksum=loaded.checksum,
        decision_status="not_matched",
        primary_domain="unassigned",
        ownership_lanes=OwnershipLanes(),
        safety_gate=SafetyGateDecision(),
        contract_reason="no cross-domain contract applies",
        trigger_reason="not_matched: no cross-domain contract applies",
        confidence=0.0,
        timestamp=_timestamp(),
    )


def normalize_contract_mode(value: object) -> str:
    mode = str(value or "off").strip().lower()
    return mode if mode in ALLOWED_CONTRACT_MODES else "off"


def get_contract_mode() -> str:
    return normalize_contract_mode(load_runtime_settings().get("contract_mode", "off"))

def get_contract_canary_settings() -> tuple[bool, list[str]]:
    settings = load_runtime_settings()
    enabled = bool(settings.get("contract_canary_enabled", False))
    transitions = settings.get("contract_canary_transitions", [])
    if not isinstance(transitions, list):
        transitions = []
    return enabled, _dedupe([str(value) for value in transitions])



def build_contract_audit_event(
    query: str,
    decision: OwnershipDecision,
    *,
    mode: str,
    original_retrieval_ownership: Optional[Dict[str, object]] = None,
    difference_type: str = "",
    canary: bool = False,
    canary_status: Literal["NOT_APPLICABLE", "COMPARE_ONLY", "CANARY_APPLIED"] = "NOT_APPLICABLE",
    original_primary_domain: str = "",
    ownership_changed: bool = False,
    limited_scope: bool = False,
    fallback_used: bool = False,
) -> ContractAuditEvent:
    policy = load_contract_source().source.audit_policy
    normalized = normalize_contract_query(query)
    rule_family = next(
        (
            rule.family
            for rule in load_contract_source().source.rules
            if rule.id == decision.matched_rule
        ),
        "",
    )
    return ContractAuditEvent(
        event_version=policy.event_version,
        request_id=str(uuid.uuid4()),
        timestamp=_timestamp(),
        contract_mode=normalize_contract_mode(mode),
        contract_version=decision.contract_version,
        contract_checksum=decision.contract_checksum,
        query_sha256=hashlib.sha256(normalized.encode("utf-8")).hexdigest(),
        query_length=len(normalized),
        retention_days=policy.retention_days,
        retention_class=policy.retention_class,
        matched_rule=decision.matched_rule,
        decision_status=decision.decision_status,
        primary_domain=decision.primary_domain,
        mandatory_secondary_domains=decision.mandatory_secondary_domains,
        suppressed_domains=decision.suppressed_domains,
        safety_gate=decision.safety_gate,
        contract_reason=decision.contract_reason,
        original_retrieval_ownership=original_retrieval_ownership,
        difference_type=difference_type,
        canary=canary,
        canary_status=canary_status,
        original_primary_domain=original_primary_domain,
        contract_primary_domain=decision.primary_domain,
        limited_scope=limited_scope,
        rule_family=rule_family,
        fallback_used=fallback_used,
        suppressed_primary_domains=decision.suppressed_domains,
        ownership_changed=ownership_changed,
    )

def _evaluate_and_log(query: str, mode: str) -> OwnershipDecision:
    decision = evaluate_ownership_contract(query)
    event = build_contract_audit_event(query, decision, mode=mode)
    LOGGER.info(
        "cross_domain_contract_audit %s",
        json.dumps(event.model_dump(), ensure_ascii=False, sort_keys=True),
    )
    return decision


def record_contract_decision(
    query: str,
    *,
    mode: Optional[str] = None,
) -> Optional[OwnershipDecision]:
    effective_mode = normalize_contract_mode(mode) if mode is not None else get_contract_mode()
    if effective_mode not in {"shadow", "production"}:
        return None
    try:
        return _evaluate_and_log(query, effective_mode)
    except Exception:
        LOGGER.exception("cross_domain_contract_audit_error")
        return None


def record_shadow_decision(
    query: str,
    *,
    mode: Optional[str] = None,
) -> Optional[OwnershipDecision]:
    effective_mode = normalize_contract_mode(mode) if mode is not None else get_contract_mode()
    if effective_mode != "shadow":
        return None
    return record_contract_decision(query, mode="shadow")
