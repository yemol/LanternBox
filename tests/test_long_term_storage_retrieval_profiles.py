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


def test_storage_zone_labeling_prefers_storage_basic_zone():
    question = "家里的储藏区应该怎么分区和贴标签？"
    assert "storage_basic_zone_labeling" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0895"

    wikis = _related_wiki_ids(question)
    assert "general-storage-zone-basic-layout-001" in wikis


def test_dry_grain_moisture_pest_prefers_storage_grain():
    question = "米面豆类怎么防潮防虫长期保存？"
    assert "storage_food_grain_rotation" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0896"

    wikis = _related_wiki_ids(question)
    assert "food-storage-dry-grain-zone-001" in wikis


def test_rodent_food_batch_isolation_prefers_storage_grain_or_isolation():
    question = "储粮区发现鼠咬痕迹，要怎么隔离批次？"
    assert "storage_food_grain_rotation" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0896", "DG-0901"}

    wikis = _related_wiki_ids(question)
    assert "food-storage-rodent-bite-isolation-001" in wikis


def test_moldy_dry_food_batch_prefers_storage_before_food_entry():
    question = "干粮有霉味，但还没准备吃，应该怎么处理这批？"
    assert "storage_food_grain_rotation" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0896", "DG-0901"}

    wikis = _related_wiki_ids(question)
    assert "food-storage-grain-moisture-clump-check-001" in wikis


def test_fifo_grain_rotation_prefers_storage_records_or_grain():
    question = "储粮应该怎么做先入先出，避免旧粮被忘掉？"
    assert "storage_food_grain_rotation" in _profile_names(question)
    assert "storage_inventory_handover" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0902", "DG-0896"}

    wikis = _related_wiki_ids(question)
    assert "general-storage-fifo-shelf-rule-001" in wikis


def test_seed_moisture_recheck_prefers_storage_seed():
    question = "种子袋有潮气，应该怎么复查和隔离？"
    assert "storage_seed_storage" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0897"

    wikis = _related_wiki_ids(question)
    assert "agriculture-storage-seed-moisture-quarantine-001" in wikis


def test_seed_food_separation_prefers_storage_seed():
    question = "留种和食用种子应该怎么分开放？"
    assert "storage_seed_storage" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0897"

    wikis = _related_wiki_ids(question)
    assert "agriculture-storage-seed-food-separation-shelf-001" in wikis


def test_first_aid_kit_review_prefers_storage_medical_supplies():
    question = "急救包多久复查一次，最少看哪些东西？"
    assert "storage_medical_supply_storage" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0898"

    wikis = _related_wiki_ids(question)
    assert "medical-storage-first-aid-kit-monthly-check-001" in wikis


def test_damaged_medicine_package_prefers_storage_isolation():
    question = "药品包装破损了，能不能放回药箱？"
    assert "storage_medical_supply_storage" in _profile_names(question)
    assert "storage_suspicious_item_isolation" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0898", "DG-0901"}

    wikis = _related_wiki_ids(question)
    assert "medical-storage-expired-unknown-medicine-hold-001" in wikis


def test_battery_power_bank_storage_prefers_storage_energy():
    question = "电池和充电宝长期不用，应该怎么分区保存？"
    assert "storage_energy_fuel_storage" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0900"

    wikis = _related_wiki_ids(question)
    assert "energy-storage-battery-dry-cool-zone-001" in wikis


def test_leak_odor_item_isolation_prefers_storage_suspicious_item():
    question = "有漏液和刺鼻味的物品，应该放进正常仓库吗？"
    assert "storage_suspicious_item_isolation" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0901"

    wikis = _related_wiki_ids(question)
    assert "hygiene-storage-leak-odor-isolation-001" in wikis


def test_storage_handover_minimum_fields_prefers_storage_handover():
    question = "储藏区交接时，下一班最少要知道什么？"
    assert "storage_inventory_handover" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0902"

    wikis = _related_wiki_ids(question)
    assert "general-storage-handover-card-001" in wikis
