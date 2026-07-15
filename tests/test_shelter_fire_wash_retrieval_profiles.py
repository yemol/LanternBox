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


def _wiki_ids(question: str) -> set[str]:
    return {wiki.id for wiki in _wiki_candidates(question)}


def test_shelter_ventilation_heat_balance_avoids_repair_primary():
    question = "晚上很冷，大家想把门窗都堵死保暖，这样可以吗？"
    assert "shelter_ventilation_heat_balance" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id in {"DG-0848", "DG-0850"}
    assert guides[0].id not in {"DG-0621", "DG-0569", "DG-0580"}
    assert "repair" not in (guides[0].raw.get("domains") or [])

    wikis = _wiki_ids(question)
    assert "shelter-ventilation-heat-balance-001" in wikis
    assert "fire-carbon-monoxide-suspect-stop-001" in wikis


def test_fire_smoke_backdraft_prefers_combustion_stop_over_food():
    question = "做饭的时候烟一直往屋里倒灌，应该开窗还是赶紧灭火？"
    assert "fire_smoke_combustion_stop" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0850"
    assert "food" not in (guides[0].raw.get("domains") or [])

    guide_ids = {guide.id for guide in guides[:3]}
    assert "DG-0849" in guide_ids

    wikis = _wiki_ids(question)
    assert "fire-smoke-backdraft-room-response-001" in wikis
    assert "fire-carbon-monoxide-suspect-stop-001" in wikis


def test_wet_socks_outing_prefers_clothing_ppe():
    question = "袜子湿了，鞋里也有水，但还要出去找东西，能不能继续走？"
    assert "clothing_wet_cold_ppe" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0852"

    wikis = _wiki_ids(question)
    assert "clothing-foot-check-after-wet-work-001" in wikis
    assert "clothing-shoe-sole-failure-outing-stop-001" in wikis


def test_shivering_work_question_prefers_clothing_or_shelter_stop_line():
    question = "人已经冷得一直发抖，但活还没干完，可以继续撑一下吗？"
    assert "clothing_wet_cold_ppe" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id in {"DG-0852", "DG-0848"}

    wikis = _wiki_ids(question)
    assert "clothing-wet-cold-early-hypothermia-signs-001" in wikis


def test_contaminated_glove_damage_prefers_clothing_ppe_over_hygiene_medical():
    question = "清理污染物的时候手套破了，但手还没碰到东西，可以继续用吗？"
    assert "clothing_wet_cold_ppe" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0852"
    top_domains = set(guides[0].raw.get("domains") or [])
    assert "clothing" in top_domains

    wikis = _wiki_ids(question)
    assert "clothing-glove-contamination-cut-boundary-001" in wikis
    assert "clothing-contaminated-laundry-zone-001" in wikis


def test_wash_space_layout_prefers_wash_zone_profile():
    question = "地方很小，水桶、做饭的地方和桶厕都只能放在一个房间附近，怎么分区？"
    assert "wash_zone_layout_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0853"

    wikis = _wiki_ids(question)
    assert "hygiene-wash-zone-layout-minimum-001" in wikis
    assert "hygiene-food-water-toilet-distance-review-001" in wikis
    assert "hygiene-contamination-zone-visible-marking-001" in wikis


def test_bucket_toilet_full_prefers_wash_with_bucket_secondary():
    question = "桶厕快满了，还有味道，什么时候必须更换或封起来？"
    assert "wash_zone_layout_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0853"
    assert "DG-0626" in {guide.id for guide in guides[:3]}

    wikis = _wiki_ids(question)
    assert "hygiene-bucket-toilet-changeover-001" in wikis
    assert "hygiene-wash-abnormal-record-001" in wikis


def test_limited_handwater_keeps_existing_wash_pass():
    question = "洗手水不够了，哪些时候必须洗手，哪些可以降级处理？"
    assert "wash_zone_layout_priority" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0853"

    wikis = _wiki_ids(question)
    assert "hygiene-handwater-priority-table-001" in wikis
    assert "hygiene-wash-zone-layout-minimum-001" in wikis


def test_hot_ash_trash_bag_prefers_ash_ember_guide():
    question = "灰烬看似灭了但仍热，是否可以倒垃圾袋？"

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0851"

    related_wiki = set(guides[0].raw.get("related_wiki") or [])
    assert "fire-ash-ember-cooling-disposal-001" in related_wiki
    assert "fire-night-final-extinguish-log-001" in related_wiki


def test_patient_cup_towel_kitchen_prefers_patient_kitchen_isolation_guide():
    question = "病人杯子毛巾进入厨房区域怎么办？"

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0854"

    related_wiki = set(guides[0].raw.get("related_wiki") or [])
    assert "hygiene-patient-cup-towel-isolation-001" in related_wiki
    assert "hygiene-kitchen-raw-cooked-contamination-line-001" in related_wiki
    assert "hygiene-wash-abnormal-record-001" in related_wiki
