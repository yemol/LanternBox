from pathlib import Path

import pytest

from api.retrieval_v2.contract_runtime import (
    CONTRACT_PATH,
    evaluate_ownership_contract,
    load_contract_source,
)


ROOT = Path(__file__).resolve().parents[1]


def test_contract_file_is_v11_single_runtime_source():
    loaded = load_contract_source()
    source = loaded.source
    assert loaded.path == CONTRACT_PATH
    assert source.schema_version == "1.1"
    assert source.contract_version == "1.1.0"
    assert source.status == "production_ready_not_enabled"
    assert source.runtime_integration is True
    assert source.default_mode == "off"
    assert len(source.rules) == 15
    assert len(loaded.checksum) == 64


def test_runtime_contains_no_domain_vocabulary_copy():
    runtime_text = (ROOT / "api/retrieval_v2/contract_runtime.py").read_text(encoding="utf-8")
    for term in ["鼓包", "没有看到明火", "备用种子", "卫生耗材", "运行时间越来越短"]:
        assert term not in runtime_text


@pytest.mark.parametrize(
    ("case_id", "query", "expected_owner", "expected_rule"),
    [
        ("A01", "鼓包电池还能不能测一下实际容量", "energy_safety", "battery_abnormal"),
        ("A02", "太阳能控制器正在冒烟但还有输出能不能继续用", "energy_safety", "energy_device_abnormal"),
        ("A03", "两个未知来源电源能不能直接并联", "energy_safety", "unknown_power_action"),
        ("A04", "旧锂电池包怎么拆开换芯", "energy_safety", "battery_invasive_operation"),
    ],
)
def test_active_hazard_contract(case_id, query, expected_owner, expected_rule):
    decision = evaluate_ownership_contract(query)
    assert decision.primary_domain == expected_owner, case_id
    assert decision.matched_rule == expected_rule
    assert decision.safety_gate.required is True
    assert "energy_deepening" in decision.suppressed_domains


def test_active_smoke_allows_fire_escalation():
    decision = evaluate_ownership_contract("太阳能控制器正在冒烟但还有输出能不能继续用")
    assert decision.primary_domain == "energy_safety"
    assert "fire" in decision.mandatory_secondary_domains
    assert "fire_escalation" in decision.safety_gate.prohibited_actions


@pytest.mark.parametrize(
    ("case_id", "query", "expected_owner", "expected_rule"),
    [
        ("N01", "同一个电池运行时间越来越短，怎么确认容量下降", "energy_deepening", "normal_capacity_degradation"),
        ("N02", "冬天太阳能产出下降如何建立跨季基线", "energy_deepening", "solar_seasonal_baseline"),
    ],
)
def test_active_hazard_negative_boundaries_preserve_deepening(case_id, query, expected_owner, expected_rule):
    decision = evaluate_ownership_contract(query)
    assert decision.primary_domain == expected_owner, case_id
    assert decision.matched_rule == expected_rule
    assert decision.safety_gate.required is False


@pytest.mark.parametrize(
    ("case_id", "query", "expected_owner", "expected_rule"),
    [
        ("I01", "三个月太阳能和电池记录如何长期归档", "records", "information_lifecycle"),
        ("I02", "备用电池长期不用如何保存", "storage", "physical_storage"),
        ("I03", "电池测试数据如何标记可信度", "records", "information_lifecycle"),
        ("I04", "设备贴标签后入库存放", "storage", "physical_storage"),
    ],
)
def test_information_object_contract(case_id, query, expected_owner, expected_rule):
    decision = evaluate_ownership_contract(query)
    assert decision.primary_domain == expected_owner, case_id
    assert decision.matched_rule == expected_rule


def test_physical_storage_keeps_records_supplemental():
    decision = evaluate_ownership_contract("设备贴标签后入库存放")
    assert decision.primary_domain == "storage"
    assert decision.mandatory_secondary_domains == ["records"]


def test_active_hazard_precedes_information_object_contract():
    decision = evaluate_ownership_contract("电池正在冒烟，记录表怎么归档")
    assert decision.matched_rule == "battery_abnormal"
    assert decision.primary_domain == "energy_safety"


def test_resolved_hazard_can_exit_to_records():
    decision = evaluate_ownership_contract("鼓包电池已经停用隔离且冷却稳定，相关记录如何长期归档")
    assert decision.matched_rule == "information_lifecycle"
    assert decision.primary_domain == "records"


def test_prohibition_statement_is_not_an_active_operation_request():
    decision = evaluate_ownership_contract("禁止把未知来源电源直接并联")
    assert decision.primary_domain == "unassigned"
    assert decision.decision_status == "not_matched"
