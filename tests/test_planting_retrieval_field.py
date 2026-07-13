import json
from pathlib import Path
from unittest.mock import patch

from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    _profile_adjustment,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.orchestrator import _source_card
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem
from api.retrieval_v2.selector import _candidate_for_prompt
from scripts.test_planting_retrieval_field import CASES, grade_case


ROOT = Path(__file__).resolve().parents[1]


def test_planting_fixture_has_ten_distinct_realistic_cases():
    cases = json.loads(CASES.read_text(encoding="utf-8"))
    assert len(cases) == 10
    assert len({case["id"] for case in cases}) == 10
    assert all(len(case["question"]) >= 20 for case in cases)
    assert all(case["expected_wikis"] for case in cases)


def test_expected_batch4a_wikis_exist_in_target_category():
    cases = json.loads(CASES.read_text(encoding="utf-8"))
    expected = {
        slug for case in cases for slug in case["expected_wikis"]
        if slug.startswith("agriculture-") and slug != "agriculture-harvest-001"
    }
    for slug in expected:
        matches = list((ROOT / "wiki_import").glob(f"*/{slug}.md"))
        assert len(matches) == 1, slug
        assert "category: 种植与食物生产" in matches[0].read_text(encoding="utf-8")


def test_grade_requires_expected_new_wiki_and_action_contract():
    case = json.loads(CASES.read_text(encoding="utf-8"))[0]
    wiki = {"id": case["expected_wikis"][0], "title": "种子发芽率小样测试", "raw": {"slug": case["expected_wikis"][0]}}
    answer = "先取小样做发芽测试，不要整批播种。缺少盒子时改用湿布，记录批次并复查；发霉时停止。"
    result = grade_case(case, answer, [], [wiki], [], [])
    assert result["verdict"] == "pass"
    assert result["batch4a_wiki_hit"]


def test_grade_marks_domain_only_result_partial_not_pass():
    case = json.loads(CASES.read_text(encoding="utf-8"))[0]
    wiki = {"id": "agriculture-unrelated-001", "title": "其他种植", "raw": {"slug": "agriculture-unrelated-001"}}
    answer = "先记录时间，缺少材料时停止并复查。"
    result = grade_case(case, answer, [], [wiki], [], [])
    assert result["verdict"] == "partial"
    assert not result["batch4a_wiki_hit"]


def test_planting_profile_requires_object_and_state_groups():
    assert not any(
        item["name"] == "planting_low_water"
        for item in _matching_query_profiles("储水不够了，怎么分配？")
    )
    assert any(
        item["name"] == "planting_low_water"
        for item in _matching_query_profiles("菜园储水只够几天，应该先浇谁？")
    )


def test_profile_adjustment_boosts_agriculture_and_penalizes_mismatch():
    profile = next(
        item for item in _matching_query_profiles("幼苗晒蔫了，土还湿，该怎么处理？")
        if item["name"] == "planting_seedling_heat"
    )
    boost, target, boost_reasons = _profile_adjustment(
        profile,
        domains=set(),
        slug_domain="agriculture",
        category="种植与食物生产",
        text="幼苗日灼后遮阴缓苗",
        user_message="幼苗晒蔫了，土还湿，该怎么处理？",
    )
    penalty, mismatched, penalty_reasons = _profile_adjustment(
        profile,
        domains={"livestock"},
        text="小动物饮水遮阴",
        user_message="幼苗晒蔫了，土还湿，该怎么处理？",
    )
    assert target and boost > 0
    assert any("domain_boost" in reason for reason in boost_reasons)
    assert not mismatched and penalty < 0
    assert any("mismatch_penalty" in reason for reason in penalty_reasons)


def test_seedling_profile_keeps_livestock_evidence_below_planting_actions():
    question = "昨天刚移栽的幼苗今天中午全蔫了，土还湿，怎么遮阴缓苗？"
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    assert candidates
    assert "livestock" not in (candidates[0].raw.get("domains") or [])
    planting_positions = [
        index for index, item in enumerate(candidates)
        if "planting" in (item.raw.get("domains") or [])
    ]
    livestock_positions = [
        index for index, item in enumerate(candidates)
        if "livestock" in (item.raw.get("domains") or [])
    ]
    assert planting_positions
    assert not livestock_positions or min(planting_positions) < min(livestock_positions)


def test_low_water_profile_prefers_crop_action_over_food_rationing():
    question = "菜园储水只够几天，幼苗和结果期作物应先浇谁？"
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    assert candidates
    first_text = " ".join([
        candidates[0].title,
        str(candidates[0].raw.get("scenario") or ""),
        str(candidates[0].raw.get("goal") or ""),
    ])
    assert any(term in first_text for term in ["作物", "浇灌", "根区", "幼苗"])
    assert any(
        "candidate_anchor" in reason
        for reason in candidates[0].raw.get("retrieval_profile_reasons", [])
    )
    assert candidates[0].raw["retrieval_profile_fit_tier"] == 2


@patch("api.services.wiki_service.search_wiki_for_ai")
def test_profile_aliases_probe_wiki_and_preserve_slug_trace(mock_search):
    def result_for(query, limit=8):
        if query == "草木灰":
            return [{
                "id": "pb_record_1",
                "record_id": "pb_record_1",
                "slug": "agriculture-ash-contract-001",
                "title": "草木灰来源判断",
                "summary": "来源不明的灰禁用。",
                "category": "种植与食物生产",
                "domains": [],
            }]
        return []

    mock_search.side_effect = result_for
    candidates = fetch_wiki_candidates(
        SourcePlanItem(source_type="wiki", query="旧木板烧成的灰能不能撒到菜地？", limit=4),
        user_message="旧木板烧成的灰能不能撒到菜地当肥料？",
    )
    assert any(call.args[0] == "草木灰" for call in mock_search.call_args_list)
    assert candidates[0].id == "agriculture-ash-contract-001"
    assert candidates[0].raw["metadata"]["record_id"] == "pb_record_1"
    assert candidates[0].raw["retrieval_query_profiles"] == ["planting_wood_ash"]
    assert candidates[0].raw["retrieval_profile_target_match"]
    assert any("domain_boost" in reason for reason in candidates[0].raw["retrieval_profile_reasons"])


@patch("api.services.wiki_service.get_wiki_articles_by_slugs_for_ai")
def test_related_wiki_inherits_profile_trace_for_prompt_and_logs(mock_get):
    mock_get.return_value = [{
        "id": "agriculture-seed-contract-001",
        "slug": "agriculture-seed-contract-001",
        "record_id": "pb_seed_1",
        "title": "种子发芽测试",
        "category": "种植与食物生产",
        "summary": "先做小样测试。",
        "guide_links": ["DG-TEST"],
    }]
    guide = EvidenceCandidate(
        source_type="guide",
        id="DG-TEST",
        title="种子小样测试",
        raw={
            "related_wiki": ["agriculture-seed-contract-001"],
            "retrieval_query_profiles": ["planting_seed_germination"],
            "retrieval_profile_target_match": True,
            "retrieval_profile_adjustment": 36,
            "retrieval_profile_reasons": ["profile_domain_boost:36"],
        },
    )
    wiki = fetch_related_wiki_candidates([guide])[0]
    assert wiki.raw["source_reason"] == "guide_related_wiki"
    assert wiki.raw["retrieval_query_profiles"] == ["planting_seed_germination"]
    assert "inherited_from_guide:profile_domain_boost:36" in wiki.raw["retrieval_profile_reasons"]
    assert _candidate_for_prompt(wiki)["retrieval_profile_target_match"]
    assert _source_card(wiki)["retrieval_profile_adjustment"] == 36


RAIN_FIELD_QUESTION = (
    "昨晚大雨后菜地到中午还有积水，几盆菜叶也开始发黄。"
    "我该先排水还是继续浇，哪些迹象说明这块地暂时不能再种？"
)


def _guide_position(candidates, guide_id):
    return next(index for index, item in enumerate(candidates) if item.id == guide_id)


def test_rain_waterlogging_profile_triggers_and_prioritizes_field_guide():
    profiles = _matching_query_profiles(RAIN_FIELD_QUESTION)
    assert [item["name"] for item in profiles] == ["planting_rain_waterlogging"]
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=RAIN_FIELD_QUESTION, limit=8),
        user_message=RAIN_FIELD_QUESTION,
        core_terms=["土壤积水", "菜叶发黄", "排水操作", "浇灌判断"],
    )
    assert _guide_position(candidates, "DG-0234") < _guide_position(candidates, "DG-0228")
    target = next(item for item in candidates if item.id == "DG-0234")
    assert target.raw["retrieval_profile_fit_tier"] == 2
    assert "planting_rain_waterlogging" in target.raw["retrieval_query_profiles"]


def test_generic_flood_electricity_question_does_not_trigger_planting_profile():
    question = "昨晚大雨后院子里积水很深，屋里电源插座附近也进水了，要不要先断电？"
    names = {item["name"] for item in _matching_query_profiles(question)}
    assert "planting_rain_waterlogging" not in names
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    assert candidates
    assert "planting" not in (candidates[0].raw.get("domains") or [])


def test_explicit_container_waterlogging_keeps_container_guide_primary():
    question = "花盆底部没有排水孔，昨晚雨后盆里一直积水，苗叶开始发黄，我要不要给盆打孔？"
    assert "planting_rain_waterlogging" in {
        item["name"] for item in _matching_query_profiles(question)
    }
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    assert "容器" in candidates[0].title and "排水" in candidates[0].title
    assert _guide_position(candidates, candidates[0].id) < _guide_position(candidates, "DG-0234")
    assert any(item.id == "DG-0228" for item in candidates)


def test_field_object_outweighs_incidental_pots():
    question = "菜地到中午还有积水，几盆菜叶开始发黄，这块地还能不能继续种？"
    assert "planting_rain_waterlogging" in {
        item["name"] for item in _matching_query_profiles(question)
    }
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    assert _guide_position(candidates, "DG-0234") < _guide_position(candidates, "DG-0228")


@patch("api.services.wiki_service.search_wiki_for_ai")
def test_rain_profile_aliases_recall_target_wiki_candidates(mock_search):
    articles = {
        "雨后菜地复查": ("agriculture-post-rain-garden-review-001", "雨后菜地排水与污染复查"),
        "排水下渗测试": ("agriculture-soil-drainage-test-001", "种植地排水下渗测试"),
        "根腐预警": ("agriculture-root-rot-warning-001", "根腐与停水判断"),
    }

    def result_for(query, limit=8):
        if query not in articles:
            return []
        slug, title = articles[query]
        return [{
            "id": slug,
            "slug": slug,
            "record_id": "pb_" + slug[-8:],
            "title": title,
            "summary": query + "的现场判断与停止边界。",
            "category": "种植与食物生产",
            "domains": [],
        }]

    mock_search.side_effect = result_for
    candidates = fetch_wiki_candidates(
        SourcePlanItem(source_type="wiki", query="菜地雨后排不掉水，叶子发黄，怎么判断根腐风险？", limit=8),
        user_message="菜地雨后排不掉水，叶子发黄，怎么判断根腐风险？",
    )
    target_slugs = {slug for slug, _ in articles.values()}
    hits = target_slugs & {item.id for item in candidates}
    assert len(hits) >= 2
    assert all(item.raw["retrieval_profile_fit_tier"] == 2 for item in candidates if item.id in hits)
    queried = {call.args[0] for call in mock_search.call_args_list}
    assert {"雨后菜地复查", "排水下渗测试", "根腐预警"} <= queried
