import json
import logging
from unittest.mock import patch

import pytest

from api.models import AiRuntimeSettingsUpdate

from api.config import _normalize_runtime_settings
from api.retrieval_v2.contract_runtime import (
    OwnershipDecision,
    evaluate_ownership_contract,
    load_contract_source,
    normalize_contract_mode,
    record_shadow_decision,
)
from api.retrieval_v2.orchestrator import run_retrieval_v2
from api.retrieval_v2.schemas import (
    EvidenceCandidate,
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
)


@pytest.mark.parametrize(
    ("query", "primary", "secondary"),
    [
        ("鼓包电池还能不能测一下实际容量", "energy_safety", {"fire"}),
        ("充电器冒烟但没有看到明火", "energy_safety", {"fire"}),
        ("设备已经起火并出现浓烟", "fire", {"energy_safety"}),
        ("人员电击烧伤，设备已断电", "medical", {"energy_safety"}),
    ],
)
def test_active_hazard_shadow_decisions(query, primary, secondary):
    decision = evaluate_ownership_contract(query)
    assert decision.primary_domain == primary
    assert secondary <= set(decision.mandatory_secondary_domains)
    assert decision.confidence > 0.0


@pytest.mark.parametrize(
    ("query", "primary", "secondary"),
    [
        ("三个月太阳能和电池记录如何长期归档", "records", {"energy"}),
        ("备用零件长期保存并入库", "storage", {"records"}),
        ("种子本体装袋防潮后入库", "storage", {"records"}),
        ("种植实验数据如何标记可信度并归档", "records", {"agriculture"}),
    ],
)
def test_information_object_shadow_decisions(query, primary, secondary):
    decision = evaluate_ownership_contract(query)
    assert decision.primary_domain == primary
    assert secondary <= set(decision.mandatory_secondary_domains)


@pytest.mark.parametrize(
    ("query", "primary"),
    [
        ("同一个电池运行时间越来越短，怎么确认容量下降", "energy_deepening"),
        ("两个未知来源电源能不能直接并联", "energy_safety"),
        ("离网系统全年发电与电池日志建立版本和归档索引", "records"),
        ("备用零件贴标签后入库存放", "storage"),
        ("无人受伤、设备已断电，询问外部线缆检查", "repair"),
    ],
)
def test_stable_domain_regression_decisions(query, primary):
    assert evaluate_ownership_contract(query).primary_domain == primary


def test_ownership_decision_contains_required_shadow_fields():
    query = "  鼓包电池\n还能不能测一下容量  "
    decision = evaluate_ownership_contract(query)
    payload = decision.model_dump()
    assert isinstance(decision, OwnershipDecision)
    assert {
        "query",
        "normalized_query",
        "contract_version",
        "contract_checksum",
        "matched_rule",
        "decision_status",
        "primary_domain",
        "mandatory_secondary_domains",
        "suppressed_domains",
        "ownership_lanes",
        "safety_gate",
        "contract_reason",
        "trigger_reason",
        "confidence",
        "timestamp",
    } == set(payload)
    assert payload["query"] == query
    assert payload["normalized_query"] == "鼓包电池 还能不能测一下容量"
    assert payload["contract_version"] == load_contract_source().source.contract_version
    assert payload["timestamp"].endswith("+00:00")


def test_contract_mode_accepts_readiness_modes():
    assert normalize_contract_mode("off") == "off"
    assert normalize_contract_mode("shadow") == "shadow"
    assert normalize_contract_mode("compare") == "compare"
    assert normalize_contract_mode("production") == "production"
    assert _normalize_runtime_settings({"contract_mode": "shadow"})["contract_mode"] == "shadow"
    assert _normalize_runtime_settings({"contract_mode": "compare"})["contract_mode"] == "compare"
    assert _normalize_runtime_settings({"contract_mode": "production"})["contract_mode"] == "production"
    assert AiRuntimeSettingsUpdate(contract_mode="shadow").contract_mode == "shadow"
    assert AiRuntimeSettingsUpdate(contract_mode="compare").contract_mode == "compare"
    assert AiRuntimeSettingsUpdate(contract_mode="production").contract_mode == "production"


def test_off_mode_does_not_log_or_return_decision(caplog):
    caplog.set_level(logging.INFO, logger="lanternbox.cross_domain_contract")
    assert record_shadow_decision("鼓包电池", mode="off") is None
    assert "cross_domain_contract_audit" not in caplog.text


def test_shadow_mode_logs_redacted_audit_event(caplog):
    caplog.set_level(logging.INFO, logger="lanternbox.cross_domain_contract")
    decision = record_shadow_decision("充电器冒烟但没有看到明火", mode="shadow")
    assert decision is not None
    record = next(
        record for record in caplog.records if record.message.startswith("cross_domain_contract_audit ")
    )
    payload = json.loads(record.message.split(" ", 1)[1])
    assert payload["primary_domain"] == "energy_safety"
    assert payload["contract_version"] == decision.contract_version
    assert payload["contains_full_query"] is False
    assert "query" not in payload
    assert "充电器冒烟但没有看到明火" not in record.message


def _retrieval_result(contract_mode: str) -> dict:
    plan = RetrievalPlan(
        scenario_summary="普通库存查询",
        urgency="normal",
        core_terms=["库存"],
        source_plan=[],
    )
    candidate = EvidenceCandidate(
        source_type="inventory",
        id="inventory-1",
        title="库存记录",
        summary="现有库存",
    )
    selection = EvidenceSelection(
        selected=[SelectedEvidence(source_type="inventory", id="inventory-1", reason="baseline")]
    )
    with (
        patch("api.retrieval_v2.orchestrator.build_retrieval_plan", return_value=plan),
        patch("api.retrieval_v2.orchestrator.fetch_candidates_from_plan", return_value=[candidate]),
        patch("api.retrieval_v2.orchestrator.select_evidence_with_ai", return_value=selection),
        patch("api.retrieval_v2.orchestrator._should_trigger_kiwix", return_value=False),
        patch("api.retrieval_v2.contract_runtime.load_runtime_settings", return_value={"contract_mode": contract_mode}),
    ):
        return run_retrieval_v2("普通库存查询").model_dump()


def test_shadow_mode_does_not_change_retrieval_output():
    assert _retrieval_result("shadow") == _retrieval_result("off")
