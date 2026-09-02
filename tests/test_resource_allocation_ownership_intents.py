from unittest.mock import patch

import pytest

from api.retrieval_v2.orchestrator import run_retrieval_v2
from api.retrieval_v2.ownership_intents import (
    apply_ownership_intent_candidates,
    detect_ownership_intent,
)
from api.retrieval_v2.schemas import EvidenceCandidate, RetrievalPlan, SourcePlanItem


@pytest.mark.parametrize(
    ("query", "intent", "primary", "secondary"),
    [
        (
            "过去半年发电量持续下降，长期供能需要重新评估吗？",
            "long_term_energy_capacity_trend",
            "energy_deepening",
            ["records", "resource_allocation"],
        ),
        (
            "连续几个月系统可用电量越来越低，是否应该重新规划长期容量？",
            "long_term_energy_capacity_trend",
            "energy_deepening",
            ["records", "resource_allocation"],
        ),
        (
            "水泵坏了，怎么检查和修复？",
            "explicit_equipment_repair",
            "repair",
            ["tools"],
        ),
        (
            "只有这一台泵，但现在已经坏了，该先修还是找替代方案？",
            "explicit_equipment_repair",
            "repair",
            ["tools", "resource_allocation"],
        ),
    ],
)
def test_positive_ownership_intents(query, intent, primary, secondary):
    decision = detect_ownership_intent(query)
    assert decision is not None
    assert decision.intent == intent
    assert decision.primary_domain == primary
    assert decision.secondary_domains == secondary


@pytest.mark.parametrize(
    "query",
    [
        "今天太阳能发电偏低，晚上哪些负载要关？",
        "控制器正在冒烟，还能不能继续使用？",
        "三个月发电记录怎么整理成趋势？",
        "水泵正在冒烟，怎么修？",
        "维修日志怎么归档？",
        "需要制造一个新的水泵部件",
        "坏掉的备用泵要怎么长期存放？",
    ],
)
def test_negative_ownership_boundaries_do_not_match(query):
    assert detect_ownership_intent(query) is None


def _candidate(candidate_id: str, *, category: str, domains: list[str], text: str):
    return EvidenceCandidate(
        source_type="guide",
        id=candidate_id,
        title=text,
        category=category,
        summary=text,
        raw={"category": category, "domains": domains, "scenario": text},
    )


def test_candidate_lane_preserves_set_and_within_lane_order():
    query = "过去半年发电量持续下降，长期供能需要重新评估吗？"
    candidates = [
        _candidate("medical-1", category="医疗急救与照护", domains=["medical"], text="伤口处理"),
        _candidate("energy-1", category="能源", domains=["power"], text="长期发电不足检查"),
        _candidate("records-1", category="信息保存与长期重建", domains=["records"], text="长期记录"),
        _candidate("energy-2", category="能源", domains=["power"], text="容量盘点与长期供能"),
    ]

    result = apply_ownership_intent_candidates(
        query,
        candidates,
        contract_status="COMPARE_ONLY",
    )

    assert result.context.status == "APPLIED"
    assert result.context.candidate_set_preserved is True
    assert [item.id for item in result.candidates] == [
        "energy-1",
        "energy-2",
        "medical-1",
        "records-1",
    ]
    assert [item.id for item in candidates] == [
        "medical-1",
        "energy-1",
        "records-1",
        "energy-2",
    ]


def test_no_candidate_fallback_preserves_existing_behavior():
    candidates = [
        _candidate("records-1", category="信息保存与长期重建", domains=["records"], text="设备记录"),
        _candidate("storage-1", category="长期储藏", domains=["storage"], text="设备保存"),
    ]
    result = apply_ownership_intent_candidates(
        "水泵坏了，怎么检查和修复？",
        candidates,
        contract_status="COMPARE_ONLY",
    )

    assert result.context.status == "CANDIDATE_MISSING"
    assert result.context.ownership_lane_candidate_missing is True
    assert [item.id for item in result.candidates] == ["records-1", "storage-1"]


def test_non_compare_mode_keeps_candidates_unchanged():
    candidates = [
        _candidate("repair-1", category="工具维修与替代制作", domains=["repair"], text="设备维修"),
        _candidate("records-1", category="信息保存与长期重建", domains=["records"], text="设备记录"),
    ]
    result = apply_ownership_intent_candidates(
        "水泵坏了，怎么修？",
        candidates,
        contract_status="NOT_APPLICABLE",
    )

    assert result.context.status == "NOT_APPLICABLE"
    assert [item.id for item in result.candidates] == ["repair-1", "records-1"]


def test_contract_precedence_keeps_candidates_unchanged():
    candidates = [
        _candidate("repair-1", category="工具维修与替代制作", domains=["repair"], text="设备维修"),
        _candidate("safety-1", category="能源", domains=["power"], text="设备异常停用"),
    ]
    result = apply_ownership_intent_candidates(
        "水泵坏了，怎么修？",
        candidates,
        contract_status="CANARY_APPLIED",
        contract_safety_boundary=True,
    )

    assert result.context.status == "CONTRACT_PRECEDENCE"
    assert [item.id for item in result.candidates] == ["repair-1", "safety-1"]


def _plan(query: str) -> RetrievalPlan:
    return RetrievalPlan(
        scenario_summary=query,
        core_terms=[query],
        source_plan=[SourcePlanItem(source_type="guide", query=query, limit=8)],
        raw={"fallback": True},
    )


@pytest.mark.parametrize(
    ("query", "intent", "primary"),
    [
        (
            "过去三个月发电越来越低，长期供能能力是不是需要重新规划？",
            "long_term_energy_capacity_trend",
            "energy_deepening",
        ),
        (
            "唯一的水泵坏了，怎么修？",
            "explicit_equipment_repair",
            "repair",
        ),
    ],
)
def test_production_fallback_applies_ownership_lane(query, intent, primary):
    with (
        patch("api.retrieval_v2.orchestrator.build_retrieval_plan", return_value=_plan(query)),
        patch("api.retrieval_v2.orchestrator._should_trigger_kiwix", return_value=False),
        patch(
            "api.retrieval_v2.orchestrator.select_evidence_with_ai",
            side_effect=RuntimeError("offline selector"),
        ),
    ):
        result = run_retrieval_v2(query)

    context = result.retrieval_decision["ownership_intent"]
    selected = next(item for item in result.selected_evidence if item.source_type == "guide")
    assert context["status"] == "APPLIED"
    assert context["intent"] == intent
    assert context["primary_domain"] == primary
    assert context["candidate_set_preserved"] is True
    assert selected.raw["ownership_intent_owner"] == primary


def test_active_hazard_contract_precedes_repair_guard():
    assert detect_ownership_intent("水泵正在冒烟，怎么修？") is None
