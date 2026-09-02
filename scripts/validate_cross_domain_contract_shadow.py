#!/usr/bin/env python3
"""Compare shadow ownership decisions with the existing Guide retrieval top result.

This Batch16-I runner is read-only. It does not enable shadow mode globally and
does not feed OwnershipDecision back into candidate retrieval or selection.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import Counter
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv/bin/python"
if not os.environ.get("LANTERNBOX_BATCH16_I_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_BATCH16_I_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.contract_runtime import (  # noqa: E402
    evaluate_ownership_contract,
    load_contract_source,
)
from api.retrieval_v2.fetchers import fetch_guide_candidates  # noqa: E402
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402


DEFAULT_FIXTURE = ROOT / "tests/fixtures/cross_domain_contract_shadow_validation_cases.json"

ENERGY_DEEPENING_GUIDES = {f"DG-{number:04d}" for number in range(919, 926)}
ENERGY_SAFETY_GUIDES = {
    "DG-0546", "DG-0616", "DG-0620", "DG-0841", "DG-0842", "DG-0843"
}
ENERGY_MANAGEMENT_GUIDES = {
    "DG-0339", "DG-0561", "DG-0562", "DG-0563", "DG-0564", "DG-0565",
    "DG-0617", "DG-0618", "DG-0619", "DG-0844", "DG-0845", "DG-0846",
}
FIRE_GUIDES = {
    "DG-0579", "DG-0646", "DG-0647", "DG-0648", "DG-0649", "DG-0650",
    "DG-0849", "DG-0850", "DG-0851",
}
RECORDS_GUIDES = {f"DG-{number:04d}" for number in range(911, 919)} | {
    "DG-0295", "DG-0400"
}
STORAGE_GUIDES = {f"DG-{number:04d}" for number in range(895, 903)}
MANUFACTURING_GUIDES = {f"DG-{number:04d}" for number in range(871, 879)}
AGRICULTURE_GUIDES = {f"DG-{number:04d}" for number in range(879, 887)}
PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
REPAIR_GUIDES = (
    {f"DG-{number:04d}" for number in range(621, 626)}
    | {f"DG-{number:04d}" for number in range(834, 841)}
    | {"DG-0382", "DG-0383", "DG-0385"}
)


def as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if value in (None, ""):
        return []
    return [value]


def guide_primary_domains(guide: EvidenceCandidate) -> set[str]:
    """Test-only primary ownership mapping; it does not alter Guide metadata."""
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    category = str(raw.get("category") or "")
    title = str(guide.title or "")
    domains = {str(item).lower() for item in as_list(raw.get("domains"))}

    if guide_id in ENERGY_DEEPENING_GUIDES:
        return {"energy_deepening"}
    if guide_id in ENERGY_SAFETY_GUIDES:
        return {"energy_safety"}
    if guide_id in ENERGY_MANAGEMENT_GUIDES:
        return {"energy_management"}
    if guide_id in FIRE_GUIDES:
        return {"fire"}
    if guide_id in RECORDS_GUIDES:
        return {"records"}
    if guide_id in STORAGE_GUIDES:
        return {"storage"}
    if guide_id in MANUFACTURING_GUIDES:
        return {"manufacturing"}
    if guide_id in AGRICULTURE_GUIDES:
        return {"agriculture"}
    if guide_id in PRODUCTION_GUIDES:
        return {"production"}
    if guide_id in REPAIR_GUIDES:
        return {"repair", "tools"}

    ordered_hints = [
        ("medical", ("医疗", "急救", "止血", "伤口", "出血", "烧伤", "发烧")),
        ("fire", ("火源", "火灾", "明火", "灭火")),
        ("storage", ("长期储", "库存", "储藏", "入库", "种子库")),
        ("records", ("信息保存", "知识传承", "知识版本", "归档", "可信度")),
        ("manufacturing", ("制造", "木工", "制作")),
        ("repair", ("维修", "修理")),
        ("agriculture", ("农业", "种植", "作物", "育苗", "种子")),
        ("energy_safety", ("电池异常", "电气安全", "能源安全")),
        ("energy_management", ("能源管理", "负载调度", "电量分配")),
    ]
    for owner, terms in ordered_hints:
        if any(term in category or term in title for term in terms):
            return {owner}

    domain_hints = [
        ("medical", {"medical"}),
        ("storage", {"storage", "inventory"}),
        ("records", {"records"}),
        ("manufacturing", {"manufacturing"}),
        ("repair", {"repair"}),
        ("agriculture", {"agriculture", "planting"}),
        ("fire", {"fire"}),
        ("energy_safety", {"energy_safety"}),
        ("energy_management", {"energy_management"}),
        ("energy_other", {"energy", "power"}),
    ]
    for owner, names in domain_hints:
        if domains & names:
            return {owner}
    return {"other"}


def evaluate_case(case: dict[str, Any]) -> dict[str, Any]:
    query = str(case["query"])
    decision = evaluate_ownership_contract(query)
    candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=query, limit=8),
        user_message=query,
    )
    top = candidates[0] if candidates else None
    retrieval_domains = guide_primary_domains(top) if top else {"no_candidate"}
    expected_contract = str(case["expected_contract_primary"])
    accepted_retrieval = {str(value) for value in case["accepted_retrieval_primary"]}
    contract_correct = decision.primary_domain == expected_contract
    retrieval_correct = bool(retrieval_domains & accepted_retrieval)
    exact_agreement = decision.primary_domain in retrieval_domains

    if exact_agreement and contract_correct and retrieval_correct:
        classification = "MATCH"
    elif contract_correct and retrieval_correct:
        classification = "C"
    elif contract_correct:
        classification = "A"
    elif retrieval_correct:
        classification = "B"
    else:
        classification = "D"

    accepted_agreement = classification in {"MATCH", "C"}
    return {
        "id": case["id"],
        "group": case["group"],
        "focus": case["focus"],
        "query": query,
        "contract_primary_domain": decision.primary_domain,
        "contract_expected_domain": expected_contract,
        "contract_correct": contract_correct,
        "contract_secondary_domains": decision.mandatory_secondary_domains,
        "contract_trigger_reason": decision.trigger_reason,
        "contract_confidence": decision.confidence,
        "retrieval_primary_domain": sorted(retrieval_domains),
        "retrieval_accepted_domains": sorted(accepted_retrieval),
        "retrieval_correct": retrieval_correct,
        "retrieval_top_guide": str(top.id or "") if top else "",
        "retrieval_top_title": str(top.title or "") if top else "",
        "exact_agreement": exact_agreement,
        "accepted_agreement": accepted_agreement,
        "classification": classification,
    }


def pct(numerator: int, denominator: int) -> float:
    return round((numerator / denominator * 100.0) if denominator else 0.0, 2)


def build_summary(fixture: dict[str, Any], results: list[dict[str, Any]]) -> dict[str, Any]:
    total = len(results)
    classifications = Counter(result["classification"] for result in results)
    contract_counts = Counter(result["contract_primary_domain"] for result in results)
    retrieval_counts: Counter[str] = Counter()
    for result in results:
        retrieval_counts.update(result["retrieval_primary_domain"])

    exact = sum(bool(result["exact_agreement"]) for result in results)
    accepted = sum(bool(result["accepted_agreement"]) for result in results)
    conflicts = classifications["A"] + classifications["B"] + classifications["D"]

    active_hazard_ids = {"E02", "E03", "M01"}
    information_object_ids = {"R01", "R02", "S01", "S02", "W01", "W02"}
    energy_deepening_ids = {"E01", "E04"}

    def contract_accuracy(case_ids: set[str]) -> dict[str, Any]:
        subset = [result for result in results if result["id"] in case_ids]
        correct = sum(bool(result["contract_correct"]) for result in subset)
        return {"correct": correct, "total": len(subset), "rate": pct(correct, len(subset))}

    return {
        "fixture_version": fixture["fixture_version"],
        "total_cases": total,
        "contract_primary_domain_counts": dict(sorted(contract_counts.items())),
        "retrieval_primary_domain_counts": dict(sorted(retrieval_counts.items())),
        "contract_accuracy": pct(sum(result["contract_correct"] for result in results), total),
        "retrieval_fixture_accuracy": pct(sum(result["retrieval_correct"] for result in results), total),
        "exact_agreement_count": exact,
        "exact_agreement_rate": pct(exact, total),
        "accepted_agreement_count": accepted,
        "agreement_rate": pct(accepted, total),
        "conflict_count": conflicts,
        "conflict_rate": pct(conflicts, total),
        "classification_counts": {
            key: classifications[key] for key in ("MATCH", "A", "B", "C", "D")
        },
        "active_hazard_contract_accuracy": contract_accuracy(active_hazard_ids),
        "information_object_contract_accuracy": contract_accuracy(information_object_ids),
        "energy_deepening_protection_accuracy": contract_accuracy(energy_deepening_ids),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    parser.add_argument("--compact", action="store_true")
    args = parser.parse_args()

    fixture = json.loads(args.fixture.read_text(encoding="utf-8"))
    results = [evaluate_case(case) for case in fixture["cases"]]
    loaded = load_contract_source()
    output = {
        "scope": "batch16-i-shadow-validation",
        "contract_version": loaded.source.contract_version,
        "contract_checksum": loaded.checksum,
        "retrieval_path": "fetch_guide_candidates(limit=8), top candidate observed only",
        "summary": build_summary(fixture, results),
        "cases": results,
    }
    print(json.dumps(output, ensure_ascii=False, indent=None if args.compact else 2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
