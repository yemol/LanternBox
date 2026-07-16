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


def _wiki_ids(question: str) -> set[str]:
    return {
        wiki.id
        for wiki in fetch_wiki_candidates(
            SourcePlanItem(source_type="wiki", query=question, limit=8),
            user_message=question,
        )
    }


def test_rain_wet_radio_prefers_communication_stop_over_electronics():
    question = "电台被雨淋了还能不能直接开机测试？"
    assert "communication_wet_device_safety" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0855"
    assert any(guide.id in {"DG-0546", "DG-0842"} for guide in guides[:4])

    wikis = _wiki_ids(question)
    assert "communication-wet-device-stop-use-001" in wikis


def test_lora_node_water_prefers_communication_device_isolation():
    question = "LoRa 节点盒子里面有水怎么办？"
    assert "communication_wet_device_safety" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0855"
    assert "DG-0857" in {guide.id for guide in guides[:3]}

    wikis = _wiki_ids(question)
    assert "communication-wet-device-stop-use-001" in wikis


def test_rain_antenna_setup_prefers_radio_precheck():
    question = "雨天架天线需要注意什么？"
    assert "communication_antenna_weather_safety" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0858"

    wikis = _wiki_ids(question)
    assert "communication-antenna-wet-weather-stop-001" in wikis


def test_oxidized_antenna_interface_prefers_communication_over_repair_primary():
    question = "天线接口氧化导致信号差怎么办？"
    assert "communication_antenna_weather_safety" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id in {"DG-0858", "DG-0855"}
    assert "comms" in set(guides[0].raw.get("domains") or [])

    wikis = _wiki_ids(question)
    assert "communication-antenna-connection-check-001" in wikis


def test_radio_no_reply_prefers_communication_failure_isolation():
    question = "无线电突然没人回应应该怎么排查？"
    assert "communication_device_failure_isolation" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0855"

    wikis = _wiki_ids(question)
    assert "communication-radio-no-receive-check-001" in wikis
    assert "communication-device-fault-tree-001" in wikis


def test_offline_node_without_damage_prefers_lora_deployment_check():
    question = "节点离线但设备没坏怎么办？"
    assert "communication_device_failure_isolation" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0857"
    assert "DG-0855" in {guide.id for guide in guides[:3]}
