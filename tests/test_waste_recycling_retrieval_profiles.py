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


def test_mixed_trash_prefers_waste_basic_sorting():
    question = "家里产生一堆混合垃圾，应该先怎么分类和临时隔离？"
    assert "waste_basic_sorting" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0887"

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-basic-sorting-isolation-001" in wikis


def test_broken_glass_prefers_sharp_waste_before_safety():
    question = "碎玻璃散在地上，怎么收集和标记才安全？"
    assert "waste_sharp_broken_material" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0888"

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-sharp-glass-temporary-container-001" in wikis


def test_metal_edge_and_nails_prefers_sharp_waste():
    question = "废金属边角和钉子怎么处理，避免扎伤？"
    assert "waste_sharp_broken_material" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0888"

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-metal-edge-scrap-isolation-001" in wikis


def test_battery_leak_trash_prefers_waste_contaminated_trash():
    question = "电池漏液的垃圾能不能和普通垃圾放一起？"
    assert "waste_contaminated_trash" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0889", "DG-0887"}

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-battery-leakage-boundary-001" in wikis


def test_patient_tissue_trash_prefers_patient_waste_split():
    question = "病人用过的纸巾和垃圾能不能混进普通垃圾？"
    assert "waste_contaminated_trash" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0889"

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-patient-trash-double-bag-zone-001" in wikis


def test_rotten_kitchen_scrap_prefers_waste_before_compost():
    question = "厨余已经发臭渗液，还能不能进堆肥？"
    assert "waste_kitchen_scrap_boundary" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0890"

    wikis = _related_wiki_ids(question)
    assert "hygiene-waste-food-rot-wet-isolation-001" in wikis


def test_hot_ash_trash_bin_prefers_waste_ash_boundary():
    question = "火堆刚灭的热灰能不能直接倒进垃圾桶？"
    assert "waste_ash_char_boundary" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0891"

    wikis = _related_wiki_ids(question)
    assert "fire-waste-hot-ash-not-trash-001" in wikis


def test_salvaged_wood_material_pool_prefers_waste_reuse_intake():
    question = "废木板能不能直接放进材料池？"
    assert "waste_material_pool_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0892"

    wikis = _related_wiki_ids(question)
    assert "repair-recycling-salvaged-wood-intake-check-001" in wikis


def test_smelly_plastic_bucket_prefers_container_reuse_boundary():
    question = "旧塑料桶有异味，还能不能拿来装东西？"
    assert "waste_material_pool_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0893"

    wikis = _related_wiki_ids(question)
    assert "repair-recycling-plastic-container-intake-check-001" in wikis


def test_material_pool_ledger_prefers_waste_record_handover():
    question = "可用材料池怎么登记，避免以后没人知道能不能用？"
    assert "waste_material_pool_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0894"

    wikis = _related_wiki_ids(question)
    assert "general-recycling-material-pool-ledger-001" in wikis


def test_waste_handover_minimum_fields_prefers_waste_handover():
    question = "废弃物交接时要告诉下一班什么？"
    assert "waste_material_pool_reuse" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0894"

    wikis = _related_wiki_ids(question)
    assert "general-waste-disposal-handover-card-001" in wikis
