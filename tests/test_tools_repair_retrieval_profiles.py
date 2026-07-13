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


def test_anchor_fastening_profile_keeps_comms_fixed_window_out_of_primary_slot():
    question = "棚布绑在墙角固定点上会晃，拉紧时还有咔咔响，这个点还能继续绑东西吗？"
    assert "repair_anchor_fastening" in _profile_names(question)
    candidates = _guide_candidates(question)
    assert candidates
    assert candidates[0].id != "DG-0636"
    assert "comms" not in (candidates[0].raw.get("domains") or [])


def test_cutting_fixture_profile_keeps_comms_fixed_window_out_of_primary_slot():
    question = "要手锯一块木板，但没有工作台，锯之前怎么把板子固定住才不容易滑和夹锯？"
    assert "repair_cutting_fixture" in _profile_names(question)
    candidates = _guide_candidates(question)
    assert candidates
    assert candidates[0].id != "DG-0636"
    assert "comms" not in (candidates[0].raw.get("domains") or [])


def test_glue_surface_profile_keeps_seed_grain_guide_out_of_primary_slot():
    question = "两块塑料片用胶水粘了又开，表面还有灰和水汽，重新粘之前要怎么处理？"
    assert "repair_glue_surface" in _profile_names(question)
    candidates = _guide_candidates(question)
    assert candidates
    assert candidates[0].id != "DG-0051"
    assert "planting" not in (candidates[0].raw.get("domains") or [])


def test_tool_handover_profile_keeps_medical_consumable_guide_out_of_primary_slot():
    question = "工具经常借出去，回来才发现钳子松了、刀片缺口了，借用和损坏怎么交接记录？"
    assert "repair_tool_handover" in _profile_names(question)
    candidates = _guide_candidates(question)
    assert candidates
    assert candidates[0].id != "DG-0212"
    assert "medical" not in (candidates[0].raw.get("domains") or [])


def test_rope_wear_burn_profile_triggers_repair_rope_load():
    question = "这根绳子磨毛发白还有烧焦痕迹，还能不能继续承重吊东西？"
    assert "repair_rope_load" in _profile_names(question)


def test_rope_wear_burn_load_prefers_rope_evidence_over_anchor_profile():
    question = "这根旧绳有几段磨毛发白，还有一处像被火烫过，今天能不能继续拿来吊水桶或拉重物？"
    names = _profile_names(question)
    assert "repair_rope_load" in names
    assert "repair_anchor_fastening" not in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0156"
    assert guides[0].id != "DG-0833"

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-rope-full-length-inspection-001"
    assert any(wiki.id == "repair-knot-slip-check-001" for wiki in wikis)


def test_wire_jacket_damage_profile_triggers_and_prefers_repair_over_water_damage():
    question = "延长线外皮破了能看到内芯，能不能缠胶带继续通电？"
    assert "repair_wire_damage" in _profile_names(question)
    candidates = _guide_candidates(question)
    assert candidates
    assert candidates[0].id != "DG-0546"
    assert any(domain in (candidates[0].raw.get("domains") or []) for domain in ["repair", "tools", "power", "security"])


def test_blade_chip_profile_triggers_repair_blade_damage():
    question = "美工刀刀片有崩口，切纸板时会卡一下，还能继续用吗？"
    assert "repair_blade_damage" in _profile_names(question)


def test_used_screw_reuse_profile_triggers_repair_fastener_reuse():
    question = "拆下来的旧螺丝和旧钉子有点锈，螺丝头花了，还能复用在承重位置吗？"
    assert "repair_fastener_reuse" in _profile_names(question)


def test_used_fastener_reuse_does_not_let_anchor_profile_take_primary_slot():
    question = "拆下来的旧螺丝和旧钉子有点锈，有的头也花了，哪些还能复用，哪些不能再用在承重位置？"
    names = _profile_names(question)
    assert "repair_fastener_reuse" in names
    assert "repair_anchor_fastening" not in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id != "DG-0833"

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-used-screw-nail-reuse-check-001"
    assert any(wiki.id == "repair-fastener-washer-storage-001" for wiki in wikis)


def test_old_nail_load_position_stays_fastener_reuse_not_anchor_fastening():
    question = "这些拆下来的旧钉子有锈也有点弯，还能不能复用在承重位置？"
    names = _profile_names(question)
    assert "repair_fastener_reuse" in names
    assert "repair_anchor_fastening" not in names

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-used-screw-nail-reuse-check-001"


def test_loose_anchor_still_prefers_anchor_failure_evidence():
    question = "棚布绑在墙角固定点上会晃，拉紧时还有咔咔响，这个点还能继续绑东西吗？"
    names = _profile_names(question)
    assert "repair_anchor_fastening" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0833"

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-anchor-point-failure-check-001"


def test_explicit_bracket_hook_load_still_triggers_anchor_fastening():
    question = "这个临时支架和挂钩要承重，轻拉有异响，还能继续挂东西吗？"
    names = _profile_names(question)
    assert "repair_anchor_fastening" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0833"

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id in {
        "repair-simple-bracket-load-check-001",
        "repair-anchor-point-failure-check-001",
    }


def test_fixed_time_network_window_does_not_trigger_repair_fixed_profiles():
    question = "断网以后家里固定时间开机窗口怎么安排？"
    names = _profile_names(question)
    assert "repair_anchor_fastening" not in names
    assert "repair_cutting_fixture" not in names


def test_medical_consumable_record_does_not_trigger_tool_handover_profile():
    question = "外伤耗材消耗记录怎么写，敷料、绷带和消毒棉还剩多少？"
    assert "repair_tool_handover" not in _profile_names(question)
