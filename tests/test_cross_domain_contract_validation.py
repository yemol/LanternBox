from api.retrieval_v2.contract_runtime import evaluate_ownership_contract


def test_battery_abnormal_generalizes_to_mobile_power_bank():
    decision = evaluate_ownership_contract("移动电源外壳突然鼓起来而且发烫，下一步先做什么？")
    assert decision.matched_rule == "battery_abnormal"
    assert decision.primary_domain == "energy_safety"
    assert {"energy_deepening", "energy_management", "repair", "storage"} <= set(decision.suppressed_domains)
    assert {"stop", "disconnect", "isolate"} <= set(decision.safety_gate.prohibited_actions)


def test_smoke_without_fire_keeps_energy_safety_primary():
    decision = evaluate_ownership_contract("充电器有焦味并持续冒烟，但没有看到明火，应该先检查输出吗？")
    assert decision.matched_rule == "energy_device_abnormal"
    assert decision.primary_domain == "energy_safety"
    assert "fire" in decision.mandatory_secondary_domains
    assert {"energy_deepening", "energy_management", "repair"} <= set(decision.suppressed_domains)


def test_abnormal_heat_without_smoke_does_not_require_fire_secondary():
    decision = evaluate_ownership_contract("接头异常热，但没有烟、无燃烧")
    assert decision.matched_rule == "energy_device_abnormal"
    assert decision.primary_domain == "energy_safety"
    assert "fire" not in decision.mandatory_secondary_domains


def test_fire_escalation_promotes_fire_primary():
    decision = evaluate_ownership_contract("电源模块已经起火并出现浓烟，出口仍然通畅，现在怎么办？")
    assert decision.matched_rule == "active_fire"
    assert decision.primary_domain == "fire"
    assert "energy_safety" in decision.mandatory_secondary_domains
    assert {"energy_deepening", "energy_management", "repair", "records"} <= set(decision.suppressed_domains)


def test_electrical_injury_promotes_medical_primary():
    decision = evaluate_ownership_contract("有人被电源线电击后手部烧伤，设备已经断电，应该先处理什么？")
    assert decision.matched_rule == "human_harm_priority"
    assert decision.primary_domain == "medical"
    assert "energy_safety" in decision.mandatory_secondary_domains
    assert "repair" in decision.suppressed_domains


def test_energy_records_are_information_objects():
    decision = evaluate_ownership_contract("离网系统一整年的发电与电池日志需要建立版本和归档索引。")
    assert decision.matched_rule == "information_lifecycle"
    assert decision.primary_domain == "records"
    assert "storage" in decision.suppressed_domains


def test_manufacturing_repair_logs_keep_records_primary():
    decision = evaluate_ownership_contract("工作台设备连续维修失败的日志怎么按版本归档？")
    assert decision.matched_rule == "information_lifecycle"
    assert decision.primary_domain == "records"


def test_agriculture_experiment_records_keep_records_primary():
    decision = evaluate_ownership_contract("三季种植试验数据如何标记可信度并长期归档？")
    assert decision.matched_rule == "information_lifecycle"
    assert decision.primary_domain == "records"
    assert "agriculture" in decision.mandatory_secondary_domains


def test_seed_physical_storage_keeps_storage_primary():
    decision = evaluate_ownership_contract("备用种子装袋贴标签后放入干燥区长期存放。")
    assert decision.matched_rule == "biological_material_storage"
    assert decision.primary_domain == "storage"
    assert decision.mandatory_secondary_domains == ["records"]


def test_spare_parts_storage_keeps_storage_primary():
    decision = evaluate_ownership_contract("备用零件贴标签后入库存放。")
    assert decision.matched_rule == "physical_storage"
    assert decision.primary_domain == "storage"
    assert decision.mandatory_secondary_domains == ["records"]
