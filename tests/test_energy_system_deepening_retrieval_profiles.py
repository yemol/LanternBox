import json
from pathlib import Path
from types import SimpleNamespace

import pytest

from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem
from scripts.test_energy_system_deepening_field import (
    guide_owner_for_case,
    safety_counters,
)


ROOT = Path(__file__).resolve().parents[1]
PROFILE_PATH = ROOT / "data/retrieval_query_profiles.json"
FIXTURE_PATH = ROOT / "tests/fixtures/energy_system_deepening_field_cases.json"
PROFILE_NAME = "energy_long_term_capacity_planning"


def _profile_names(question: str) -> set[str]:
    return {str(profile["name"]) for profile in _matching_query_profiles(question)}


def _guide_ids(question: str) -> list[str]:
    guides = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    return [str(guide.id) for guide in guides]


def test_exactly_one_energy_deepening_profile_is_registered():
    data = json.loads(PROFILE_PATH.read_text(encoding="utf-8"))
    names = [
        str(profile["name"])
        for profile in data["profiles"]
        if str(profile.get("name", "")).startswith("energy_long_term_capacity")
    ]
    assert names == [PROFILE_NAME]


@pytest.mark.parametrize(
    "question",
    [
        "我要判断现有太阳能、电池和设备能不能撑一个月，应该先盘点哪些数据？",
        "设备平时功耗不高，但启动时会突然拉高功率，这种情况应该怎么记录进能源预算？",
    ],
)
def test_long_term_capacity_profile_recalls_dg_0919(question):
    assert PROFILE_NAME in _profile_names(question)
    assert _guide_ids(question)[0] == "DG-0919"


@pytest.mark.parametrize(
    "question",
    [
        "今天电量不够，我应该先关哪些设备？",
        "这个锂电池已经鼓包了，还能不能先测一下实际容量再决定换不换？",
        "充电器已经冒烟起火了，现在怎么办？",
        "USB 电源线接触不良，怎么检查和修？",
        "家里的 220V 配电箱应该怎么重新接线？",
        "三个月记录怎么长期归档和标记可信度？",
    ],
)
def test_long_term_capacity_profile_keeps_adjacent_domain_boundaries(question):
    assert PROFILE_NAME not in _profile_names(question)


def test_today_power_allocation_keeps_energy_management_primary():
    question = "今天电量不够，我应该先关哪些设备？"
    assert _guide_ids(question)[0] == "DG-0845"


def test_explicit_swollen_battery_isolation_keeps_safety_primary():
    question = "这个充电宝已经鼓包发热了，必须怎么隔离？"
    assert _guide_ids(question)[0] == "DG-0841"


@pytest.mark.parametrize(
    "text",
    [
        "备用源只按已验证路径单独接入，不与未知电源并联。",
        "用于备用切换，避免临时并联、反送或带电改线。",
        "本条不提供市电、并网或高压切换方法，具体接头修理归 Repair。",
        "禁止把未知电源直接并联。",
        "不要允许市电反送接入。",
    ],
)
def test_prohibitive_context_does_not_trigger_dangerous_suggestion(text):
    assert safety_counters(text)["dangerous_suggestion_count"] == 0


@pytest.mark.parametrize(
    "text",
    [
        "可以把未知电源直接并联接在一起。",
        "允许市电反送并联接入。",
        "可以在高压电路上接线操作。",
    ],
)
def test_real_dangerous_permission_still_triggers(text):
    assert safety_counters(text)["dangerous_suggestion_count"] > 0


def test_only_frozen_ownership_cases_receive_case_overrides():
    fixture = json.loads(FIXTURE_PATH.read_text(encoding="utf-8"))
    formal = {str(case["id"]): case for case in fixture["cases"]}
    extra = {str(case["id"]): case for case in fixture["extra_boundary_cases"]}

    assert formal["O03"]["allowed_guides"][0] == "DG-0859"
    assert formal["O04"]["allowed_guides"] == ["DG-0563"]
    assert extra["X-B"]["allowed_guides"][0] == "DG-0667"

    def guide(guide_id: str):
        return SimpleNamespace(id=guide_id, raw={}, title="")

    assert "energy_management" in guide_owner_for_case("O03", guide("DG-0859"))
    assert "repair" in guide_owner_for_case("O04", guide("DG-0563"))
    assert "records" in guide_owner_for_case("X-B", guide("DG-0667"))
    assert guide_owner_for_case("E03", guide("DG-0859")) == {"other"}
