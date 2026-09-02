import json
from pathlib import Path

import pytest

from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem


ROOT = Path(__file__).resolve().parents[1]
PROFILE_PATH = ROOT / "data/retrieval_query_profiles.json"
RESOURCE_ALLOCATION_PROFILES = {
    "resource_allocation_consumption_horizon",
    "resource_allocation_emergency_reserve_release",
    "resource_allocation_high_energy_task_decision",
}


def _profile_names(question: str) -> set[str]:
    return {str(profile["name"]) for profile in _matching_query_profiles(question)}


def _guide_ids(question: str) -> list[str]:
    guides = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    return [str(guide.id) for guide in guides]


def test_exactly_three_resource_allocation_profiles_are_registered():
    data = json.loads(PROFILE_PATH.read_text(encoding="utf-8"))
    names = {
        str(profile["name"])
        for profile in data["profiles"]
        if str(profile.get("name", "")).startswith("resource_allocation_")
    }
    assert names == RESOURCE_ALLOCATION_PROFILES


@pytest.mark.parametrize(
    ("question", "profile_name", "expected_guide"),
    [
        (
            "过去每天耗水 8 升，这两天增加到 11 升，库存还能撑多久？",
            "resource_allocation_consumption_horizon",
            "DG-0927",
        ),
        (
            "备用燃料什么时候才应该动用？",
            "resource_allocation_emergency_reserve_release",
            "DG-0928",
        ),
        (
            "今天计划启动一个高耗能生产任务，但未来三天发电不足，还应该启动吗？",
            "resource_allocation_high_energy_task_decision",
            "DG-0929",
        ),
    ],
)
def test_target_intents_recall_expected_guide(question, profile_name, expected_guide):
    assert _profile_names(question) & RESOURCE_ALLOCATION_PROFILES == {profile_name}
    assert _guide_ids(question)[0] == expected_guide


@pytest.mark.parametrize(
    "question",
    [
        "今天电量不够，我应该先关哪些设备？",
        "过去三个月发电越来越低，长期供能能力是不是需要重新规划？",
        "备用电池长期不用如何保存？",
        "木架具体怎么切割和生产？",
        "唯一的水泵坏了，怎么修？",
        "过去三个月的资源消耗记录怎么整理成可信趋势？",
        "卫生耗材不足时怎样处理正在受伤的病人？",
        "唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，能不能继续用？",
        "电池鼓包了，但这是最后一块可用电池。",
    ],
)
def test_resource_allocation_profiles_keep_cross_domain_boundaries(question):
    assert not (_profile_names(question) & RESOURCE_ALLOCATION_PROFILES)


def test_same_day_energy_management_candidate_is_unchanged():
    question = "今天电量不够，我应该先关哪些设备？"
    assert _guide_ids(question)[0] == "DG-0845"


def test_specific_production_method_candidate_is_unchanged():
    question = "木架具体怎么切割和生产？"
    assert _guide_ids(question)[0] == "DG-0873"
