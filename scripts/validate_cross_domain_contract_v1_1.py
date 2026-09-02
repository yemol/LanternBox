#!/usr/bin/env python3
"""Validate Contract v1.1 through the production single-source evaluator."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.contract_runtime import (  # noqa: E402
    evaluate_ownership_contract,
    load_contract_source,
)


@dataclass(frozen=True)
class ValidationCase:
    case_id: str
    group: str
    query: str
    expected_primary: tuple[str, ...]
    expected_secondary: tuple[str, ...] = ()
    forbidden_secondary: tuple[str, ...] = ()


VALIDATION_CASES = (
    ValidationCase("F-N01", "fire", "充电器冒烟但没有看到明火", ("energy_safety",), ("fire",)),
    ValidationCase("F-N02", "fire", "接头异常热，但没有烟、无燃烧", ("energy_safety",), (), ("fire",)),
    ValidationCase("F-N03", "fire", "昨天起火，已经熄灭且现场安全，现在做复盘", ("records",), ("fire", "energy_safety")),
    ValidationCase("F-P01", "fire", "设备已经起火并出现浓烟", ("fire",), ("energy_safety",)),
    ValidationCase("F-P02", "fire", "之前没有明火，现在已经起火", ("fire",), ("energy_safety",)),
    ValidationCase("H-01", "human_harm", "人员电击烧伤，设备已断电", ("medical",), ("energy_safety",)),
    ValidationCase("H-02", "human_harm", "人员吸入烟气后咳嗽头晕", ("medical",), ("fire", "energy_safety")),
    ValidationCase("H-03", "human_harm", "设备火花但明确无人受伤", ("energy_safety",)),
    ValidationCase("H-04", "human_harm", "有人受伤且现场仍有明火，询问伤者处置", ("medical",), ("fire", "energy_safety")),
    ValidationCase("H-05", "human_harm", "无人受伤、设备已断电，询问外部线缆检查", ("repair",), ("energy_safety",)),
    ValidationCase("P-01", "information_object", "种子本体装袋、防潮、入库", ("storage",), ("records",)),
    ValidationCase("P-02", "information_object", "种植试验日志与发芽率数据归档", ("records",), ("agriculture",)),
    ValidationCase("P-03", "information_object", "医疗样本试管低温保存与交接", ("medical_bio_safety",), ("records",)),
    ValidationCase("P-04", "information_object", "医疗样本检测结果标记可信度并归档", ("records",), ("medical",)),
    ValidationCase("P-05", "information_object", "卫生耗材贴标签后入库存放", ("medical_storage",), ("records",)),
    ValidationCase("P-06", "information_object", "卫生耗材消耗日志做长期趋势", ("records",), ("medical",)),
)

REGRESSION_CASES = (
    ValidationCase("R-01", "regression", "移动电源鼓包发烫，下一步先做什么", ("energy_safety",)),
    ValidationCase("R-02", "regression", "同一个电池运行时间越来越短，怎么确认容量下降", ("energy_deepening",)),
    ValidationCase("R-03", "regression", "两个未知来源电源能不能直接并联", ("energy_safety",)),
    ValidationCase("R-04", "regression", "离网系统全年发电与电池日志建立版本和归档索引", ("records",)),
    ValidationCase("R-05", "regression", "备用零件贴标签后入库存放", ("storage",), ("records",)),
)


def evaluate(case: ValidationCase) -> dict:
    decision = evaluate_ownership_contract(case.query)
    primary_ok = decision.primary_domain in case.expected_primary
    secondary_ok = set(case.expected_secondary) <= set(decision.mandatory_secondary_domains)
    secondary_ok = secondary_ok and not (
        set(case.forbidden_secondary) & set(decision.mandatory_secondary_domains)
    )
    passed = primary_ok and secondary_ok
    return {
        "case_id": case.case_id,
        "group": case.group,
        "query": case.query,
        "expected_primary": list(case.expected_primary),
        "expected_secondary": list(case.expected_secondary),
        "forbidden_secondary": list(case.forbidden_secondary),
        "actual": {
            "rule": decision.matched_rule,
            "primary": decision.primary_domain,
            "secondary": decision.mandatory_secondary_domains,
            "suppressed": decision.suppressed_domains,
            "safety_gate": decision.safety_gate.model_dump(),
        },
        "passed": passed,
        "false_positive": int(decision.decision_status == "matched" and not primary_ok),
        "false_negative": int(not primary_ok or not secondary_ok),
    }


def rate(results: list[dict]) -> float:
    return round(100.0 * sum(item["passed"] for item in results) / len(results), 1)


def main() -> int:
    loaded = load_contract_source()
    validation = [evaluate(case) for case in VALIDATION_CASES]
    regression = [evaluate(case) for case in REGRESSION_CASES]
    groups = {
        group: [item for item in validation if item["group"] == group]
        for group in ("fire", "human_harm", "information_object")
    }
    payload = {
        "contract_version": loaded.source.contract_version,
        "schema_version": loaded.source.schema_version,
        "contract_checksum": loaded.checksum,
        "contract_source": str(loaded.path.relative_to(ROOT)),
        "runtime_integration": loaded.source.runtime_integration,
        "summary": {
            "contract_pass_rate": rate(validation),
            "contract_pass_count": sum(item["passed"] for item in validation),
            "contract_case_count": len(validation),
            "fire_boundary_accuracy": rate(groups["fire"]),
            "human_harm_ownership_accuracy": rate(groups["human_harm"]),
            "information_object_accuracy": rate(groups["information_object"]),
            "regression_count": sum(not item["passed"] for item in regression),
            "regression_pass_count": sum(item["passed"] for item in regression),
            "regression_case_count": len(regression),
            "false_positive_count": sum(item["false_positive"] for item in validation),
            "false_negative_count": sum(item["false_negative"] for item in validation),
        },
        "validation_cases": validation,
        "regression_cases": regression,
    }
    print(json.dumps(payload, ensure_ascii=False, indent=2))
    return 0 if all(item["passed"] for item in validation + regression) else 1


if __name__ == "__main__":
    raise SystemExit(main())
