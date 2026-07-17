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


def _guide_ids(question: str) -> list[str]:
    return [guide.id for guide in _guide_candidates(question)]


def _wiki_ids(question: str) -> set[str]:
    return {
        wiki.id
        for wiki in fetch_wiki_candidates(
            SourcePlanItem(source_type="wiki", query=question, limit=8),
            user_message=question,
        )
    }


def test_flood_old_road_prefers_navigation_route_risk():
    question = "洪水后原来的道路还能走吗？"
    assert "navigation_terrain_risk" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0865"

    wikis = _wiki_ids(question)
    assert "navigation-flood-terrain-risk-001" in wikis
    assert "navigation-flood-road-stop-line-001" in wikis


def test_night_movement_route_uses_navigation_before_water_or_power():
    question = "夜晚必须移动，路线应该怎么判断？"
    assert "navigation_terrain_risk" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0863", "DG-0865"}
    assert any(guide_id in {"DG-0863", "DG-0865"} for guide_id in guide_ids[:2])

    wikis = _wiki_ids(question)
    assert "navigation-night-route-risk-001" in wikis
    assert "navigation-night-movement-stop-line-001" in wikis


def test_foraging_route_plan_prefers_personal_route_planning():
    question = "出去采集物资前怎么规划路线？"
    assert "navigation_route_planning" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0863"

    wikis = _wiki_ids(question)
    assert "navigation-outbound-route-card-001" in wikis
    assert "navigation-route-checkpoint-numbering-001" in wikis


def test_route_checkpoint_setup_reaches_checkpoint_guide_and_wiki():
    question = "怎么给外出路线设置检查点？"
    profiles = _profile_names(question)
    assert "navigation_route_planning" in profiles
    assert "navigation_checkpoint_management" in profiles

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0866", "DG-0863"}
    assert "DG-0866" in guide_ids[:2]

    wikis = _wiki_ids(question)
    assert "navigation-route-checkpoint-numbering-001" in wikis


def test_safe_route_return_prefers_navigation_return_chain():
    question = "走过一次安全路线，下次怎么回来？"
    assert "navigation_lost_return" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0864", "DG-0863"}
    assert any(guide_id in {"DG-0864", "DG-0863", "DG-0870"} for guide_id in guide_ids[:2])

    wikis = _wiki_ids(question)
    assert "navigation-return-route-plan-001" in wikis
    assert "navigation-track-to-return-summary-001" in wikis


def test_past_routes_record_prefers_track_management():
    question = "过去走过哪些路线怎么记录？"
    assert "navigation_track_management" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0870"

    wikis = _wiki_ids(question)
    assert "navigation-gps-track-minimum-fields-001" in wikis
    assert "navigation-track-to-return-summary-001" in wikis
