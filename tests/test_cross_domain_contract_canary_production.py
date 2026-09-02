import inspect
import json
import logging
from collections import Counter
from pathlib import Path
from unittest.mock import patch

import pytest

from api.config import (
    CONTRACT_CANARY_TRANSITION_IDS,
    _normalize_runtime_settings,
)
from api.models import AiRuntimeSettingsUpdate
from api.retrieval_v2 import contract_canary
from api.retrieval_v2.contract_canary import (
    apply_contract_canary_candidates,
    apply_contract_canary_selection,
)
from api.retrieval_v2.contract_compare import (
    infer_candidate_ownership,
    original_retrieval_ownership,
    record_compare_evaluation,
)
from api.retrieval_v2.contract_runtime import (
    get_contract_mode,
    record_shadow_decision,
)
from api.retrieval_v2.fetchers import fetch_guide_candidates
from api.retrieval_v2.orchestrator import run_retrieval_v2
from api.retrieval_v2.schemas import (
    EvidenceSelection,
    RetrievalPlan,
    SelectedEvidence,
    SourcePlanItem,
)


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = (
    ROOT
    / "tests/fixtures/cross_domain_contract_shadow_validation_cases.json"
)
ALL_TRANSITIONS = list(CONTRACT_CANARY_TRANSITION_IDS)


def _plan(query: str) -> RetrievalPlan:
    return RetrievalPlan(
        scenario_summary=query,
        core_terms=[],
        source_plan=[
            SourcePlanItem(
                source_type="guide",
                query=query,
                limit=8,
            )
        ],
    )


def _baseline(query: str):
    return fetch_guide_candidates(
        SourcePlanItem(
            source_type="guide",
            query=query,
            limit=8,
        ),
        user_message=query,
    )


def _apply(query: str):
    return apply_contract_canary_candidates(
        query,
        _plan(query),
        _baseline(query),
        mode="production",
        canary_enabled=True,
        enabled_transition_ids=ALL_TRANSITIONS,
    )


def test_canary_configuration_keeps_repository_default_off():
    settings = _normalize_runtime_settings({})
    assert settings["contract_mode"] == "off"
    assert settings["contract_canary_enabled"] is False
    assert settings["contract_canary_transitions"] == ALL_TRANSITIONS
    update = AiRuntimeSettingsUpdate(contract_canary_enabled=True)
    assert update.contract_canary_enabled is True


def test_canary_scope_uses_structured_transitions_without_evidence_ids():
    source = inspect.getsource(contract_canary)
    assert "query ==" not in source
    assert "DG-" not in source
    assert "wiki slug" not in source.lower()
    assert {
        transition.transition_id
        for transition in contract_canary.CANARY_TRANSITIONS
    } == set(ALL_TRANSITIONS)


@pytest.mark.parametrize(
    ("query", "before", "after", "transition_id"),
    [
        (
            "太阳能控制器正在冒烟但没有明火，还能继续用吗？",
            "energy_management",
            "energy_safety",
            "active_hazard_to_energy_safety",
        ),
        (
            "冬天太阳能产出下降，如何建立跨季长期基线？",
            "energy_management",
            "energy_deepening",
            "long_term_energy_to_energy_deepening",
        ),
        (
            "工作台设备维修日志如何按版本归档？",
            "repair",
            "records",
            "information_object_to_records",
        ),
    ],
)
def test_three_a_class_transitions_apply(
    query,
    before,
    after,
    transition_id,
):
    baseline = _baseline(query)
    result = _apply(query)
    assert original_retrieval_ownership(baseline).primary_domain == before
    assert result.context.status == "CANARY_APPLIED"
    assert result.context.transition_id == transition_id
    assert result.context.contract_primary_domain == after
    assert result.context.ownership_changed is True
    assert result.candidates[0].raw["contract_canary_owner"] == after
    assert {candidate.id for candidate in baseline} <= {
        candidate.id
        for candidate in result.candidates
    }


def test_batch16_i_fixture_changes_only_three_a_cases():
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    statuses = Counter()
    applied_ids = []
    for case in fixture["cases"]:
        result = _apply(case["query"])
        statuses[result.context.status] += 1
        if result.context.status == "CANARY_APPLIED":
            applied_ids.append(case["id"])
            assert (
                result.context.contract_primary_domain
                == case["expected_contract_primary"]
            )
    assert applied_ids == ["E03", "E04", "W01"]
    assert statuses == {"CANARY_APPLIED": 3, "COMPARE_ONLY": 9}


@pytest.mark.parametrize(
    ("query", "expected_owner"),
    [
        (
            "同一个电池运行时间越来越短，怎么确认容量下降",
            "energy_deepening",
        ),
        (
            "接头异常热，但没有烟、无燃烧",
            "energy_safety",
        ),
        (
            "今天太阳能输出不足，怎么调整当天负载",
            "energy_management",
        ),
        (
            "设备已经断电，现在检查并维修接头",
            "repair",
        ),
        (
            "备用零件贴标签后长期入库存放",
            "storage",
        ),
        (
            "种植实验记录与发芽率数据怎么归档",
            "records",
        ),
        (
            "种子本体装袋防潮后入库",
            "agriculture",
        ),
        (
            "昨天设备事故已经安全，现在只做记录归档",
            "records",
        ),
    ],
)
def test_negative_boundaries_do_not_create_unapproved_transition(
    query,
    expected_owner,
):
    baseline = _baseline(query)
    result = _apply(query)
    before = original_retrieval_ownership(baseline).primary_domain
    final = (
        result.context.contract_primary_domain
        if result.context.status == "CANARY_APPLIED"
        else before
    )
    assert final == expected_owner
    if result.context.status != "CANARY_APPLIED":
        assert [candidate.id for candidate in result.candidates] == [
            candidate.id
            for candidate in baseline
        ]


@pytest.mark.parametrize(
    ("query", "contract_primary"),
    [
        (
            "昨天设备起火，已经熄灭且现场安全，现在能重新通电吗",
            "fire",
        ),
        (
            "农业样本低温保存与交接",
            "unassigned",
        ),
    ],
)
def test_limited_scope_blocks_full_expansion_debt(
    query,
    contract_primary,
):
    baseline = _baseline(query)
    result = _apply(query)
    assert result.decision is not None
    assert result.decision.primary_domain == contract_primary
    assert result.context.status == "COMPARE_ONLY"
    assert result.context.ownership_changed is False
    assert [candidate.id for candidate in result.candidates] == [
        candidate.id for candidate in baseline
    ]


def test_active_hazard_canary_preserves_stop_and_suppressed_evidence():
    query = "太阳能控制器正在冒烟但没有明火，还能继续用吗？"
    baseline = _baseline(query)
    result = _apply(query)
    old_primary = baseline[0]
    selection = EvidenceSelection(
        selected=[
            SelectedEvidence(
                source_type="guide",
                id=old_primary.id,
                reason="baseline",
            )
        ]
    )
    selected, selection = apply_contract_canary_selection(
        result,
        [old_primary],
        selection,
    )
    assert infer_candidate_ownership(selected[0]).primary_domain == (
        "energy_safety"
    )
    assert result.context.safety_boundary_present is True
    assert old_primary.id in {candidate.id for candidate in selected}
    assert selected[0].id != old_primary.id
    assert selection.raw["contract_canary"]["status"] == "CANARY_APPLIED"


def test_canary_applied_audit_is_redacted_and_complete(caplog):
    caplog.set_level(
        logging.INFO,
        logger="lanternbox.cross_domain_contract",
    )
    query = "太阳能控制器正在冒烟但没有明火，还能继续用吗？"
    result = _apply(query)
    assert result.context.status == "CANARY_APPLIED"
    record = next(
        item
        for item in caplog.records
        if item.message.startswith("cross_domain_contract_audit ")
    )
    payload = json.loads(record.message.split(" ", 1)[1])
    assert payload["canary"] is True
    assert payload["canary_status"] == "CANARY_APPLIED"
    assert payload["limited_scope"] is True
    assert payload["rule_family"] == "active_hazard"
    assert payload["fallback_used"] is False
    assert payload["original_primary_domain"] == "energy_management"
    assert payload["contract_primary_domain"] == "energy_safety"
    assert payload["mandatory_secondary_domains"] == ["fire"]
    assert "energy_management" in payload["suppressed_primary_domains"]
    assert payload["safety_gate"]["required"] is True
    assert payload["ownership_changed"] is True
    assert payload["contains_full_query"] is False
    assert "query" not in payload
    assert query not in record.message


def test_orchestrator_applies_canary_without_mutating_runtime_mode():
    query = "太阳能控制器正在冒烟但没有明火，还能继续用吗？"
    runtime_before = get_contract_mode()
    plan = _plan(query)
    old_primary = _baseline(query)[0]
    selection = EvidenceSelection(
        selected=[
            SelectedEvidence(
                source_type="guide",
                id=old_primary.id,
                reason="baseline",
            )
        ]
    )
    with (
        patch(
            "api.retrieval_v2.orchestrator.build_retrieval_plan",
            return_value=plan,
        ),
        patch(
            "api.retrieval_v2.orchestrator.select_evidence_with_ai",
            return_value=selection,
        ),
        patch(
            "api.retrieval_v2.orchestrator._should_trigger_kiwix",
            return_value=False,
        ),
        patch(
            "api.retrieval_v2.orchestrator._expand_selected_guide_wikis",
            side_effect=lambda **kwargs: (
                kwargs["candidates"],
                kwargs["selected_evidence"],
                kwargs["selection"],
            ),
        ),
        patch(
            "api.retrieval_v2.contract_runtime.load_runtime_settings",
            return_value={
                "contract_mode": "production",
                "contract_canary_enabled": True,
                "contract_canary_transitions": ALL_TRANSITIONS,
            },
        ),
    ):
        result = run_retrieval_v2(query)
    assert result.retrieval_decision["contract_canary"]["status"] == (
        "CANARY_APPLIED"
    )
    assert infer_candidate_ownership(
        result.selected_evidence[0]
    ).primary_domain == "energy_safety"

    assert get_contract_mode() == runtime_before


def test_canary_rollback_drill_is_migration_free():
    runtime_before = get_contract_mode()
    tracked_state = [
        ROOT / "data/cross_domain_contracts.json",
        ROOT / "data/runtime_settings.json",
        ROOT / "data/lanternbox.db",
    ]
    before_state = {
        path: (path.stat().st_size, path.stat().st_mtime_ns)
        for path in tracked_state
        if path.exists()
    }
    query = "太阳能控制器正在冒烟但没有明火，还能继续用吗？"
    baseline = _baseline(query)

    production = _apply(query)
    assert production.context.status == "CANARY_APPLIED"
    assert record_compare_evaluation(
        query,
        baseline,
        mode="compare",
    ) is not None
    assert record_shadow_decision(query, mode="shadow") is not None
    off = apply_contract_canary_candidates(
        query,
        _plan(query),
        baseline,
        mode="off",
        canary_enabled=True,
        enabled_transition_ids=ALL_TRANSITIONS,
    )
    assert off.context.status == "NOT_APPLICABLE"
    assert [candidate.id for candidate in off.candidates] == [
        candidate.id for candidate in baseline
    ]
    assert get_contract_mode() == runtime_before

    after_state = {
        path: (path.stat().st_size, path.stat().st_mtime_ns)
        for path in tracked_state
        if path.exists()
    }
    assert after_state == before_state
