import inspect
import json
from pathlib import Path
from unittest.mock import patch

from api.config import CONTRACT_CANARY_TRANSITION_IDS
from api.retrieval_v2.schemas import (
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
    SourcePlanItem,
)
from api.response.fallback import build_fallback_answer
from scripts import test_resource_allocation_field as runner


ROOT = Path(__file__).resolve().parents[1]
R19_QUERY = (
    "唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，"
    "能不能继续用？"
)


def _r19_plan() -> RetrievalPlan:
    return RetrievalPlan(
        scenario_summary=R19_QUERY,
        core_terms=["太阳能控制器", "冒烟", "停用"],
        source_plan=[
            SourcePlanItem(
                source_type="guide",
                query=R19_QUERY,
                limit=8,
            ),
            SourcePlanItem(
                source_type="wiki",
                query=R19_QUERY,
                limit=8,
            ),
        ],
    )


def _baseline_selection() -> EvidenceSelection:
    return EvidenceSelection(
        selected=[
            SelectedEvidence(
                source_type="guide",
                id="DG-0617",
                reason="baseline owner before Contract",
            )
        ]
    )


def _runtime_settings() -> dict[str, object]:
    return {
        "contract_mode": "production",
        "contract_canary_enabled": True,
        "contract_canary_transitions": list(CONTRACT_CANARY_TRANSITION_IDS),
    }


def _r19_fixture() -> dict[str, object]:
    fixture = json.loads(runner.CASES_PATH.read_text(encoding="utf-8"))
    return next(case for case in fixture["cases"] if case["case_id"] == "R19")


def test_runner_delegates_to_production_orchestrator():
    source = inspect.getsource(runner.select_evidence)
    assert "run_retrieval_v2(question)" in source
    for forbidden_copy in (
        "fetch_guide_candidates",
        "fetch_wiki_candidates",
        "apply_contract_canary_candidates",
        "evaluate_ownership_contract",
    ):
        assert forbidden_copy not in source


def test_r19_uses_production_contract_and_safety_gate():
    wiki_rows = runner.load_wiki_rows()
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}

    with (
        patch(
            "api.retrieval_v2.orchestrator.build_retrieval_plan",
            return_value=_r19_plan(),
        ),
        patch(
            "api.retrieval_v2.orchestrator.select_evidence_with_ai",
            return_value=_baseline_selection(),
        ),
        patch(
            "api.retrieval_v2.orchestrator._should_trigger_kiwix",
            return_value=False,
        ),
        patch(
            "api.retrieval_v2.contract_runtime.load_runtime_settings",
            return_value=_runtime_settings(),
        ),
    ):
        result = runner.run_case(_r19_fixture(), wiki_rows, id_to_slug)

    contract = result["production_path"]["contract_canary"]
    assert contract["status"] == "CANARY_APPLIED"
    assert contract["matched_rule"] == "energy_device_abnormal"
    assert contract["contract_primary_domain"] == "energy_safety"
    assert contract["safety_boundary_present"] is True
    assert result["safety_gate_status"] == "passed"
    assert result["safety_gate_failure"] is False
    assert result["actual_top_guides"][0]["id"] == "DG-0842"
    assert "energy_safety" in result["top1_owners"]
    assert "resource_allocation" not in result["top1_owners"]
    assert result["expected_primary_domain"] == "energy_safety"
    assert result["dangerous_suggestion_count"] == 0
    assert result["scarcity_overrides_safety_count"] == 0
    fallback_answer = build_fallback_answer("emergency", result["actual_top_guides"])
    assert fallback_answer
    assert runner.unsafe_lines(fallback_answer) == []
    assert runner.scarcity_override_lines(fallback_answer) == []
    assert result["verdict"] == "pass"
