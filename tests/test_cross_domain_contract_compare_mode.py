import json
import logging
from collections import Counter
from pathlib import Path
from unittest.mock import patch

from api.config import _normalize_runtime_settings
from api.models import AiRuntimeSettingsUpdate
from api.retrieval_v2.contract_compare import (
    OwnershipComparison,
    evaluate_ownership_comparison,
    record_compare_evaluation,
)
from api.retrieval_v2.contract_runtime import normalize_contract_mode
from api.retrieval_v2.fetchers import fetch_guide_candidates
from api.retrieval_v2.orchestrator import run_retrieval_v2
from api.retrieval_v2.schemas import (
    EvidenceCandidate,
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
    SourcePlanItem,
)


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests/fixtures/cross_domain_contract_shadow_validation_cases.json"


def _fixture_comparisons():
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    results = []
    for case in fixture["cases"]:
        candidates = fetch_guide_candidates(
            SourcePlanItem(source_type="guide", query=case["query"], limit=8),
            user_message=case["query"],
        )
        comparison = evaluate_ownership_comparison(
            case["query"],
            candidates,
            expected_contract_primary=case["expected_contract_primary"],
            accepted_retrieval_primary=case["accepted_retrieval_primary"],
        )
        results.append((case, comparison))
    return results


def test_compare_and_production_framework_modes_are_allowed():
    assert normalize_contract_mode("compare") == "compare"
    assert normalize_contract_mode("production") == "production"
    assert _normalize_runtime_settings({"contract_mode": "compare"})["contract_mode"] == "compare"
    assert _normalize_runtime_settings({"contract_mode": "production"})["contract_mode"] == "production"
    assert AiRuntimeSettingsUpdate(contract_mode="compare").contract_mode == "compare"
    assert AiRuntimeSettingsUpdate(contract_mode="production").contract_mode == "production"


def test_batch16_i_fixture_contract_accuracy_and_difference_types():
    results = _fixture_comparisons()
    assert len(results) == 12
    assert all(
        comparison.contract_ownership_decision.primary_domain
        == case["expected_contract_primary"]
        for case, comparison in results
    )
    counts = Counter(comparison.difference_type for _, comparison in results)
    assert counts == {"C": 9, "A": 3}
    assert counts["D"] == 0


def test_batch16_i_three_a_cases_record_old_vs_contract():
    by_id = {case["id"]: comparison for case, comparison in _fixture_comparisons()}
    expected = {
        "E03": ("energy_management", "energy_safety"),
        "E04": ("energy_management", "energy_deepening"),
        "W01": ("repair", "records"),
    }
    for case_id, (old_owner, contract_owner) in expected.items():
        comparison = by_id[case_id]
        assert comparison.difference_type == "A"
        assert comparison.original_retrieval_ownership.primary_domain == old_owner
        assert comparison.contract_ownership_decision.primary_domain == contract_owner


def test_compare_output_contains_required_decisions_and_type():
    candidate = EvidenceCandidate(
        source_type="guide",
        id="guide-existing",
        title="太阳能充电白天排程",
        category="能源",
        raw={"category": "能源", "domains": ["power"]},
    )
    comparison = evaluate_ownership_comparison(
        "太阳能控制器正在冒烟但没有明火，还能继续用吗？",
        [candidate],
    )
    assert isinstance(comparison, OwnershipComparison)
    assert comparison.original_retrieval_ownership.primary_domain == "energy_management"
    assert comparison.contract_ownership_decision.primary_domain == "energy_safety"
    assert comparison.difference_type == "A"
    assert set(comparison.model_dump()) == {
        "query",
        "compare_version",
        "original_retrieval_ownership",
        "contract_ownership_decision",
        "difference_type",
        "difference_reason",
        "timestamp",
    }


def test_compare_mode_logs_redacted_shared_audit_event(caplog):
    caplog.set_level(logging.INFO, logger="lanternbox.cross_domain_contract")
    candidate = EvidenceCandidate(
        source_type="guide",
        id="guide-existing",
        title="太阳能充电白天排程",
        category="能源",
        raw={"category": "能源", "domains": ["power"]},
    )
    comparison = record_compare_evaluation(
        "太阳能控制器正在冒烟但没有明火，还能继续用吗？",
        [candidate],
        mode="compare",
    )
    assert comparison is not None
    record = next(
        item for item in caplog.records
        if item.message.startswith("cross_domain_contract_audit ")
    )
    payload = json.loads(record.message.split(" ", 1)[1])
    assert payload["contract_mode"] == "compare"
    assert payload["difference_type"] == comparison.difference_type
    assert payload["primary_domain"] == comparison.contract_ownership_decision.primary_domain
    assert payload["original_retrieval_ownership"]["primary_domain"] == "energy_management"
    assert payload["contains_full_query"] is False
    assert "query" not in payload
    assert "太阳能控制器正在冒烟" not in record.message


def test_off_and_shadow_do_not_emit_compare_audit(caplog):
    caplog.set_level(logging.INFO, logger="lanternbox.cross_domain_contract")
    assert record_compare_evaluation("普通医疗问题", [], mode="off") is None
    assert record_compare_evaluation("普通医疗问题", [], mode="shadow") is None
    assert "cross_domain_contract_audit " not in caplog.text


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


def test_compare_mode_does_not_change_retrieval_output():
    assert _retrieval_result("compare") == _retrieval_result("off")
