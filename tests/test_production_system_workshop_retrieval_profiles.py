import pytest

from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem


PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
PRODUCTION_PROFILES = {
    "production_workshop_start_close",
    "production_material_wip_flow",
    "production_batch_repetition",
    "production_quality_release",
    "production_workshop_safety_incident",
    "production_shift_handover",
}


def _profile_names(question: str) -> set[str]:
    return {item["name"] for item in _matching_query_profiles(question)}


def _guide_candidates(question: str):
    return fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )


def _guide_ids(question: str) -> list[str]:
    return [guide.id for guide in _guide_candidates(question)]


@pytest.mark.parametrize(
    ("question", "expected"),
    [
        ("今天工坊准备开工，应该先检查什么？", {"DG-0903"}),
        ("工坊今天做不完了，收工前怎么封存？", {"DG-0904"}),
    ],
)
def test_start_close_profile_prefers_expected_production_guide(question, expected):
    assert "production_workshop_start_close" in _profile_names(question)
    assert _guide_ids(question)[0] in expected


@pytest.mark.parametrize(
    ("question", "expected"),
    [
        ("从材料池拿来的原料进工坊前要怎么登记？", {"DG-0905"}),
        ("工坊产生的边角料和废料要怎么交给废料区？", {"DG-0905", "DG-0908"}),
    ],
)
def test_material_wip_profile_prefers_flow_guide(question, expected):
    assert "production_material_wip_flow" in _profile_names(question)
    assert _guide_ids(question)[0] in expected


@pytest.mark.parametrize(
    ("question", "expected"),
    [
        ("要连续做十个同样的小支架，为什么要先做首件确认？", {"DG-0906"}),
        ("一批零件做出来大小不太一样，应该怎么复查？", {"DG-0906", "DG-0907"}),
    ],
)
def test_batch_repetition_profile_prefers_repeat_or_quality_guide(question, expected):
    assert "production_batch_repetition" in _profile_names(question)
    assert _guide_ids(question)[0] in expected


@pytest.mark.parametrize(
    ("question", "expected"),
    [
        ("一批东西刚做完，还没检查，能不能直接入库？", {"DG-0905", "DG-0907"}),
        ("做好的东西边缘有毛刺和突出的钉子，能不能先放行使用？", {"DG-0907", "DG-0908"}),
        ("成品螺丝和扎带有松动，入库前怎么办？", {"DG-0907", "DG-0908"}),
    ],
)
def test_quality_release_profile_prefers_quality_flow(question, expected):
    assert "production_quality_release" in _profile_names(question)
    assert _guide_ids(question)[0] in expected


@pytest.mark.parametrize(
    ("question", "expected"),
    [
        ("工坊灯光很暗，但还想继续切割和钻孔，可以吗？", {"DG-0909"}),
        ("连续做了很久开始手抖，还能继续做高风险加工吗？", {"DG-0909", "DG-0910"}),
        ("刚才打磨时火花和碎屑差点伤到人，复工前要记录什么？", {"DG-0909"}),
    ],
)
def test_workshop_safety_profile_prefers_incident_record(question, expected):
    assert "production_workshop_safety_incident" in _profile_names(question)
    assert _guide_ids(question)[0] in expected


def test_shift_handover_profile_prefers_workshop_handover():
    question = "下一班要接手未完成的工坊任务，交接卡最少写什么？"
    assert "production_shift_handover" in _profile_names(question)
    assert _guide_ids(question)[0] == "DG-0910"


def test_specific_shelf_build_remains_manufacturing_primary():
    question = "我想做一个结实的架子，具体怎么切木板和连接？"
    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0873"
    assert guide_ids[0] not in PRODUCTION_GUIDES


def test_broken_saw_remains_repair_tools_primary():
    question = "锯子坏了还能不能继续用，怎么修？"
    guides = _guide_candidates(question)
    assert guides[0].id == "DG-0837"
    assert guides[0].id not in PRODUCTION_GUIDES
    assert set((guides[0].raw or {}).get("domains", [])) & {"repair", "tools"}


def test_long_term_finished_storage_remains_storage_primary():
    question = "检查合格的成品要长期放在哪里，怎么保存？"
    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0895", "DG-0899"}
    assert guide_ids[0] not in PRODUCTION_GUIDES


def test_workshop_injury_does_not_activate_or_promote_production():
    question = "工坊切割时有人被划伤流血了，应该先做什么？"
    assert not (_profile_names(question) & PRODUCTION_PROFILES)
    assert _guide_ids(question)[0] not in PRODUCTION_GUIDES


@pytest.mark.xfail(
    strict=True,
    reason="Known Medical / Workshop injury cross-domain gap; Batch13-F must not fix it with Production.",
)
def test_workshop_injury_should_eventually_be_medical_primary():
    question = "工坊切割时有人被划伤流血了，应该先做什么？"
    guide = _guide_candidates(question)[0]
    domains = set((guide.raw or {}).get("domains", []))
    category = str((guide.raw or {}).get("category") or "")
    assert "medical" in domains or "医疗" in category
