import json
from pathlib import Path
from unittest.mock import patch

import pytest

from api.config import _normalize_runtime_settings
from api.models import AiRuntimeSettingsUpdate
from api.retrieval_v2.contract_compare import record_compare_evaluation
from api.retrieval_v2.contract_runtime import (
    CONTRACT_PATH,
    build_contract_audit_event,
    evaluate_ownership_contract,
    load_contract_source,
    normalize_contract_mode,
    record_contract_decision,
)
from api.retrieval_v2.orchestrator import run_retrieval_v2
from api.retrieval_v2.schemas import (
    EvidenceCandidate,
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
    SourcePlanItem,
)


ROOT = Path(__file__).resolve().parents[1]


def test_ownership_decision_contains_read_only_lanes():
    decision = evaluate_ownership_contract("太阳能控制器正在冒烟但没有明火")
    assert decision.primary_domain == "energy_safety"
    assert decision.mandatory_secondary_domains == ["fire"]
    assert {"repair", "energy_deepening", "energy_management"} <= set(decision.suppressed_domains)
    assert decision.ownership_lanes.primary == ["energy_safety"]
    assert decision.ownership_lanes.mandatory_secondary == ["fire"]
    assert decision.ownership_lanes.suppressed == decision.suppressed_domains
    assert decision.contract_reason


def test_active_hazard_safety_gate_is_observe_only():
    decision = evaluate_ownership_contract("太阳能控制器正在冒烟但没有明火")
    gate = decision.safety_gate
    assert gate.required is True
    assert gate.gate_type == "active_hazard"
    assert gate.mode == "observe_only"
    assert gate.status == "required"
    assert gate.required_primary_domains == ["energy_safety"]
    assert gate.required_secondary_domains == ["fire"]
    assert "stop" in gate.prohibited_actions


def test_human_harm_safety_gate_preserves_medical_priority():
    decision = evaluate_ownership_contract("有人受伤且现场仍有明火，询问伤者处置")
    assert decision.primary_domain == "medical"
    assert decision.safety_gate.required is True
    assert decision.safety_gate.gate_type == "human_harm"
    assert decision.safety_gate.required_primary_domains == ["medical"]
    assert {"fire", "energy_safety"} <= set(decision.safety_gate.required_secondary_domains)


def test_non_hazard_lane_has_no_safety_gate():
    decision = evaluate_ownership_contract("同一个电池运行时间越来越短，怎么确认容量下降")
    assert decision.primary_domain == "energy_deepening"
    assert decision.safety_gate.required is False
    assert decision.safety_gate.status == "not_applicable"


def test_audit_event_is_versioned_redacted_and_retained():
    query = "人员电击烧伤，设备已断电，姓名张三"
    decision = evaluate_ownership_contract(query)
    event = build_contract_audit_event(query, decision, mode="production")
    payload = event.model_dump()
    serialized = json.dumps(payload, ensure_ascii=False)
    assert event.event_version == load_contract_source().source.audit_policy.event_version
    assert event.contract_version == "1.1.0"
    assert event.contract_checksum == load_contract_source().checksum
    assert event.contains_full_query is False
    assert event.retention_days == 30
    assert event.retention_class == "local_rotating_jsonl"
    assert len(event.query_sha256) == 64
    assert query not in serialized
    assert "张三" not in serialized
    assert "query" not in payload


def test_contract_source_rejects_unsupported_schema(tmp_path):
    raw = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    raw["schema_version"] = "9.9"
    invalid = tmp_path / "invalid_contract.json"
    invalid.write_text(json.dumps(raw, ensure_ascii=False), encoding="utf-8")
    with pytest.raises(ValueError, match="unsupported contract schema"):
        load_contract_source(invalid)


def test_production_framework_is_available_but_default_remains_off():
    assert normalize_contract_mode("production") == "production"
    assert _normalize_runtime_settings({})["contract_mode"] == "off"
    assert _normalize_runtime_settings({"contract_mode": "production"})["contract_mode"] == "production"
    assert AiRuntimeSettingsUpdate(contract_mode="production").contract_mode == "production"


def test_off_mode_does_not_load_or_evaluate_contract_source():
    with patch("api.retrieval_v2.contract_runtime.load_contract_source") as source_loader:
        assert record_contract_decision("鼓包电池", mode="off") is None
        source_loader.assert_not_called()


def test_rollback_drill_requires_no_data_migration():
    watched = [CONTRACT_PATH, ROOT / "data/lanternbox.db"]
    before = {
        path: (path.stat().st_size, path.stat().st_mtime_ns)
        for path in watched
        if path.exists()
    }

    transitions = ["production", "compare", "shadow", "off"]
    assert [normalize_contract_mode(mode) for mode in transitions] == transitions
    assert record_contract_decision("鼓包电池", mode="production") is not None
    assert record_compare_evaluation("普通医疗问题", [], mode="compare") is not None
    assert record_contract_decision("鼓包电池", mode="shadow") is not None
    assert record_contract_decision("鼓包电池", mode="off") is None

    after = {
        path: (path.stat().st_size, path.stat().st_mtime_ns)
        for path in watched
        if path.exists()
    }
    assert after == before


def _retrieval_result(contract_mode: str) -> dict:
    plan = RetrievalPlan(
        scenario_summary="电池异常",
        urgency="high",
        core_terms=["鼓包电池"],
        source_plan=[SourcePlanItem(source_type="guide", query="鼓包电池", limit=8)],
    )
    candidate = EvidenceCandidate(
        source_type="guide",
        id="guide-safety",
        title="电池异常隔离",
        category="能源",
        raw={"category": "能源", "domains": ["power", "security"]},
    )
    selection = EvidenceSelection(
        selected=[SelectedEvidence(source_type="guide", id="guide-safety", reason="baseline")]
    )
    with (
        patch("api.retrieval_v2.orchestrator.build_retrieval_plan", return_value=plan),
        patch("api.retrieval_v2.orchestrator.fetch_candidates_from_plan", return_value=[candidate]),
        patch("api.retrieval_v2.orchestrator.select_evidence_with_ai", return_value=selection),
        patch("api.retrieval_v2.orchestrator._should_trigger_kiwix", return_value=False),
        patch("api.retrieval_v2.orchestrator._build_kiwix_explanations", return_value=[]),
        patch(
            "api.retrieval_v2.contract_runtime.load_runtime_settings",
            return_value={"contract_mode": contract_mode},
        ),
    ):
        return run_retrieval_v2("鼓包电池还能不能测容量").model_dump()


def test_production_framework_does_not_change_retrieval_output():
    assert _retrieval_result("production") == _retrieval_result("off")
