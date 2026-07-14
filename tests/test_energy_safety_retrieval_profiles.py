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


def test_power_bank_swollen_prefers_battery_abnormal_isolation():
    question = "移动电源鼓起来了还能不能继续充？"
    assert "energy_battery_abnormal_isolation" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0841"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-damaged-power-bank-quarantine-001" for wiki in wikis)


def test_spare_battery_hot_after_storage_prefers_battery_abnormal_isolation():
    question = "备用电池放了半年，现在发热还有必要测试吗？"
    assert "energy_battery_abnormal_isolation" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0841"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-battery-storage-temperature-boundary-001" for wiki in wikis)


def test_battery_leak_prefers_battery_abnormal_isolation():
    question = "电池漏液了怎么处理？"
    assert "energy_battery_abnormal_isolation" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0841"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-battery-leak-corrosion-isolation-001" for wiki in wikis)


def test_usb_device_overheating_prefers_low_voltage_fault_stop():
    question = "USB设备突然发烫还能继续用吗？"
    assert "energy_low_voltage_fault_stop" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0842"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-low-voltage-system-stop-boundary-001" for wiki in wikis)
    assert any(wiki.id == "energy-device-smell-heat-no-restart-001" for wiki in wikis)


def test_wire_getting_hot_after_wiring_prefers_low_voltage_fault_stop():
    question = "接线后线越来越热，是不是功率不够？"
    assert "energy_low_voltage_fault_stop" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0842"
    assert "medical" not in (guides[0].raw.get("domains") or [])

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-wire-heating-load-limit-001" for wiki in wikis)
    assert any(wiki.id == "energy-short-circuit-warning-signs-001" for wiki in wikis)


def test_red_black_reverse_polarity_allows_low_voltage_or_unknown_power():
    question = "红黑线接反会怎么样？"
    assert "energy_low_voltage_fault_stop" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id in {"DG-0842", "DG-0843"}

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-dc-polarity-reverse-check-001" for wiki in wikis)


def test_unknown_old_charger_prefers_unknown_power_adapter_match():
    question = "找到一个旧充电器，不知道是多少伏，可以试一下吗？"
    assert "energy_unknown_power_adapter_match" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0843"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-unknown-adapter-stop-use-001" for wiki in wikis)


def test_unknown_matching_port_prefers_unknown_power_adapter_match():
    question = "这个接口能插进去，但是不知道是不是匹配，可以通电看看吗？"
    assert "energy_unknown_power_adapter_match" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0843"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "energy-unknown-adapter-stop-use-001" for wiki in wikis)
    assert any(wiki.id == "energy-loose-connector-stop-use-001" for wiki in wikis)
