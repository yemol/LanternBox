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
    question = "旧钉子有锈，钉帽也变形了，还能不能用在承重位置？"
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


def test_explicit_bracket_anchor_load_still_triggers_anchor_fastening():
    question = "临时支架的挂点有点松，挂上东西会晃，这个挂点还能承重吗？"
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


def test_wave2_low_light_saw_profile_prefers_repair_stop_line_over_energy():
    question = "晚上停电，我只有头灯，想用手锯修木板，可以继续吗？"
    names = _profile_names(question)
    assert "repair_low_light_work_stop" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0836"
    assert "power" not in (guides[0].raw.get("domains") or [])

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-low-light-stop-line-001"
    assert any(wiki.id == "repair-low-light-repair-downgrade-001" for wiki in wikis)


def test_wave2_damaged_tool_profile_prefers_disable_and_isolation():
    question = "锯子的齿坏了一点还能不能继续给别人用？"
    names = _profile_names(question)
    assert "repair_tool_damage_isolation" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0840"

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-damaged-tool-disable-tag-001"
    assert any(wiki.id == "repair-damaged-tool-report-isolation-001" for wiki in wikis)


def test_wave2_child_watching_profile_prefers_tool_zone_over_medical_child():
    question = "小孩喜欢在旁边看大人修东西，需要注意什么？"
    names = _profile_names(question)
    assert "repair_child_tool_zone" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0838"
    assert "medical" not in (guides[0].raw.get("domains") or [])

    wikis = _wiki_candidates(question)
    assert wikis
    assert wikis[0].id == "repair-children-away-tool-zone-001"
    assert any(wiki.id == "repair-child-nonoperator-safety-card-001" for wiki in wikis)


def test_wave2_multi_person_repair_profile_prefers_site_boundary():
    question = "三个人一起维修，一个切割，一个递工具，一个观察，怎么安排位置？"
    names = _profile_names(question)
    assert "repair_site_clearance_boundary" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0839"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "repair-multi-person-repair-zone-boundary-001" for wiki in wikis)
    assert any(wiki.id == "repair-bystander-position-boundary-001" for wiki in wikis)


def test_wave2_floor_clutter_profile_prefers_clear_zone():
    question = "准备锯木板，地上有很多杂物，需要先处理吗？"
    names = _profile_names(question)
    assert "repair_site_clearance_boundary" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0839"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "repair-floor-clutter-trip-check-001" for wiki in wikis)
    assert any(wiki.id == "repair-knife-saw-clear-zone-001" for wiki in wikis)


def test_wave2_missing_tool_profile_extends_tool_handover():
    question = "维修结束后工具少了一把，应该怎么办？"
    names = _profile_names(question)
    assert "repair_tool_handover" in names

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0150"

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "repair-post-task-tool-count-001" for wiki in wikis)
    assert any(wiki.id == "repair-lost-tool-site-recheck-001" for wiki in wikis)


def test_wave2_temporary_workbench_uses_site_boundary_without_extra_profile():
    question = "临时桌子有点晃，但是还能放工具，需要处理吗？"
    names = _profile_names(question)
    assert "repair_site_clearance_boundary" in names
    assert "repair_workbench_zone_layout" not in names

    wikis = _wiki_candidates(question)
    assert wikis
    assert any(wiki.id == "repair-temporary-workbench-stability-check-001" for wiki in wikis)
    assert any(wiki.id == "repair-work-zone-minimum-layout-001" for wiki in wikis)
