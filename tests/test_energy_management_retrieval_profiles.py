from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem


def _profile_names(question: str) -> set[str]:
    return {item["name"] for item in _matching_query_profiles(question)}


def _guide_candidates(question: str):
    return fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )


def _wiki_candidates(question: str):
    return fetch_wiki_candidates(
        SourcePlanItem(source_type="wiki", query=question, limit=8),
        user_message=question,
    )


def test_three_day_blackout_charge_priority_prefers_daily_budget():
    question = "停电三天了，手机、手电、收音机和移动电源都没多少电，应该优先给什么设备充电？"
    assert "energy_daily_budget_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0844"

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-daily-energy-budget-001" for wiki in wikis)
    assert any(wiki.id == "energy-device-power-priority-tier-001" for wiki in wikis)


def test_no_percent_energy_budget_prefers_daily_budget():
    question = "很多设备看不到准确电量百分比，今天怎么做能源预算？"
    assert "energy_daily_budget_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0844"

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-daily-energy-budget-001" for wiki in wikis)
    assert any(wiki.id == "energy-device-runtime-estimate-card-001" for wiki in wikis)


def test_key_device_minimum_line_stays_with_minimum_line_guide():
    question = "手机和照明都快没电了，最低要给它们留多少，什么时候不能再用了？"
    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0845"


def test_single_solar_port_many_devices_prefers_charging_queue():
    question = "太阳能板只有一个口，手机、收音机、手电和移动电源都要充，怎么排队？"
    assert "energy_charging_queue_management" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0846"

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-charging-queue-schedule-001" for wiki in wikis)
    assert any(wiki.id == "energy-charging-port-rotation-rule-001" for wiki in wikis)


def test_occupied_charging_port_prefers_charging_queue():
    question = "有人一直占着唯一的充电口给自己的设备充电，其他关键设备怎么办？"
    assert "energy_charging_queue_management" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0846"


def test_cloudy_solar_low_output_prefers_solar_schedule():
    question = "今天阴天，太阳能输出很弱，应该先充手机还是移动电源？"
    assert "energy_solar_low_output_management" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0617"

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-cloudy-solar-downgrade-001" for wiki in wikis)
    assert any(wiki.id == "energy-solar-charge-target-selection-001" for wiki in wikis)


def test_two_days_low_solar_output_prefers_solar_with_budget_support():
    question = "太阳能连续两天都没怎么充进去，晚上照明和通信要怎么调整？"
    assert "energy_solar_low_output_management" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0617"

    guide_ids = {guide.id for guide in guides}
    assert {"DG-0844", "DG-0845"} & guide_ids or "DG-0564" in guide_ids


def test_one_light_at_night_reaches_minimum_line_evidence_without_comms_primary():
    question = "晚上为了省电，只留一盏灯可以吗？哪些地方不能全黑？"
    assert "energy_daily_budget_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    guide_ids = [guide.id for guide in guides]
    assert "DG-0845" in guide_ids
    assert guides[0].id != "DG-0582"
    assert "comms" not in (guides[0].raw.get("domains") or [])

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-night-lighting-runtime-plan-001" for wiki in wikis)


def test_day_end_power_review_prefers_daily_budget():
    question = "今天用电情况很乱，晚上怎么复盘明天怎么调整？"
    assert "energy_daily_budget_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0844"

    wikis = _wiki_candidates(question)
    assert any(wiki.id == "energy-day-end-power-review-001" for wiki in wikis)
    assert any(wiki.id == "general-energy-handover-summary-001" for wiki in wikis)
