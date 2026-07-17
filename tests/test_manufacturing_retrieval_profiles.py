from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
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


def _related_wiki_ids(question: str) -> set[str]:
    guides = _guide_candidates(question)[:3]
    return {wiki.id for wiki in fetch_related_wiki_candidates(guides)}


def test_scrap_wood_shelf_prefers_material_reuse_chain():
    question = "废木板能不能拿来做架子？"
    assert "manufacturing_material_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0872"
    assert "DG-0877" in guide_ids[:3]

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-scrap-material-check-001" in wikis
    assert "repair-manufacturing-wood-selection-001" in wikis
    assert "repair-manufacturing-load-test-001" in wikis


def test_wood_cutting_clamping_blocks_foot_sawing():
    question = "切木板前怎么固定，能不能用脚踩？"
    assert "manufacturing_cutting_clamping" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0873"

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-clamp-before-processing-001" in wikis
    assert "repair-manufacturing-wood-saw-support-001" in wikis


def test_low_light_cutting_prefers_manufacturing_workspace_or_cutting():
    question = "低光环境下还能继续切割材料吗？"
    profiles = _profile_names(question)
    assert "manufacturing_workspace_safety" in profiles
    assert "manufacturing_cutting_clamping" in profiles

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0871", "DG-0873"}
    assert "DG-0871" in guide_ids[:2]

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-workbench-start-check-001" in wikis
    assert "repair-manufacturing-dust-spark-stop-line-001" in wikis


def test_sparks_near_fabric_prefers_workspace_safety():
    question = "火花飞到旁边布料上，工作区怎么处理？"
    assert "manufacturing_workspace_safety" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0871"

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-cut-drill-clear-zone-001" in wikis
    assert "repair-manufacturing-dust-spark-stop-line-001" in wikis


def test_repeat_same_size_blocks_reach_structure_or_load_check():
    question = "做几个一样大小的小木块，怎么保证尺寸一致？"
    assert "manufacturing_connection_load_check" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0874", "DG-0877"}
    assert any(guide_id in {"DG-0874", "DG-0877"} for guide_id in guide_ids[:2])


def test_finished_shelf_heavy_load_prefers_load_check():
    question = "做好的架子能不能直接放重物？"
    assert "manufacturing_connection_load_check" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0877", "DG-0874"}
    assert "DG-0877" in guide_ids[:2]

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-load-test-001" in wikis
    assert "repair-manufacturing-quality-recheck-001" in wikis


def test_connection_method_choice_prefers_connection_guide():
    question = "胶水螺丝绳子什么时候用哪种连接？"
    assert "manufacturing_connection_load_check" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0875"

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-connection-choice-001" in wikis
    assert "repair-manufacturing-wood-screw-join-001" in wikis


def test_sharp_scrap_metal_bracket_prefers_metal_handling():
    question = "废金属片边缘很锋利，能不能做支架？"
    assert "manufacturing_material_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0876"

    wikis = _related_wiki_ids(question)
    assert "repair-manufacturing-metal-scrap-check-001" in wikis
    assert "repair-manufacturing-metal-edge-safe-001" in wikis
