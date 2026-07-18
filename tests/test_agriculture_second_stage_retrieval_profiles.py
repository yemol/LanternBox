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


def test_low_germination_seed_batch_prefers_seed_library():
    question = "旧种子发芽率很低，还能不能大面积播种？"
    assert "agriculture_seed_library" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0879"

    wikis = _related_wiki_ids(question)
    assert "agriculture-seed-batch-viability-ledger-001" in wikis


def test_moldy_wet_seed_stays_in_seed_library_boundary():
    question = "种子受潮有霉味，能不能混回种子库？"
    assert "agriculture_seed_library" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0879"

    wikis = _related_wiki_ids(question)
    assert "agriculture-seed-storage-moisture-failure-001" in wikis


def test_diseased_pruner_prefers_agriculture_disease_tool_split():
    question = "剪过病株的剪刀还能继续剪别的菜吗？"
    assert "agriculture_pest_disease_control" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0883"

    wikis = _related_wiki_ids(question)
    assert "agriculture-diseased-tool-zone-separation-001" in wikis


def test_chemical_contaminated_plot_prefers_agriculture_plot_boundary():
    question = "这块地疑似有化学污染，还能种吃的吗？"
    assert "agriculture_contaminated_plot_boundary" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0883"

    wikis = _related_wiki_ids(question)
    assert "agriculture-unknown-chemical-plot-stop-001" in wikis


def test_stinky_kitchen_compost_prefers_compost_food_zone():
    question = "厨余堆肥还发臭，能不能倒在叶菜旁边？"
    assert "agriculture_contaminated_plot_boundary" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0882"

    wikis = _related_wiki_ids(question)
    assert "agriculture-kitchen-waste-compost-boundary-001" in wikis
    assert "agriculture-immature-compost-stop-line-001" in wikis


def test_manure_direct_edible_plot_prefers_compost_boundary():
    question = "粪肥能不能直接用在食用菜地？"
    assert "agriculture_contaminated_plot_boundary" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0882"

    wikis = _related_wiki_ids(question)
    assert "agriculture-manure-compost-food-zone-boundary-001" in wikis


def test_harvested_beans_drying_prefers_postharvest_storage():
    question = "收获后的豆子怎么晾晒，避免发霉？"
    assert "agriculture_postharvest_storage" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0885"

    wikis = _related_wiki_ids(question)
    assert "agriculture-postharvest-drydown-record-001" in wikis
    assert "agriculture-harvest-drying-rack-check-001" in wikis


def test_seed_food_batch_separation_prefers_postharvest_or_seed_library():
    question = "留种和食用批次怎么分开？"
    profiles = _profile_names(question)
    assert "agriculture_postharvest_storage" in profiles
    assert "agriculture_seed_library" in profiles

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0885", "DG-0879"}

    wikis = _related_wiki_ids(question)
    assert "agriculture-seed-food-harvest-separation-001" in wikis


def test_poor_harvest_do_not_eat_seed_reserve_first():
    question = "今年收成少，要不要吃掉留种？"
    assert "agriculture_seed_library" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0879", "DG-0886"}

    wikis = _related_wiki_ids(question)
    assert "agriculture-seed-reserve-use-line-001" in wikis
