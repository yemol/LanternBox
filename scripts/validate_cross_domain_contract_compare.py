#!/usr/bin/env python3
"""Run Batch16-J Compare Mode against the frozen Batch16-I fixture."""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv/bin/python"
if not os.environ.get("LANTERNBOX_BATCH16_J_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_BATCH16_J_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.contract_compare import evaluate_ownership_comparison  # noqa: E402
from api.retrieval_v2.fetchers import fetch_guide_candidates  # noqa: E402
from api.retrieval_v2.schemas import SourcePlanItem  # noqa: E402


DEFAULT_FIXTURE = ROOT / "tests/fixtures/cross_domain_contract_shadow_validation_cases.json"


def pct(numerator: int, denominator: int) -> float:
    return round((numerator / denominator * 100.0) if denominator else 0.0, 2)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    parser.add_argument("--compact", action="store_true")
    args = parser.parse_args()

    fixture = json.loads(args.fixture.read_text(encoding="utf-8"))
    results = []
    for case in fixture["cases"]:
        candidates = fetch_guide_candidates(
            SourcePlanItem(source_type="guide", query=case["query"], limit=8),
            user_message=case["query"],
        )
        comparison = evaluate_ownership_comparison(
            case["query"],
            candidates,
            expected_contract_primary=case["expected_contract_primary"],
            accepted_retrieval_primary=case["accepted_retrieval_primary"],
        )
        results.append(
            {
                "id": case["id"],
                "group": case["group"],
                "focus": case["focus"],
                "query": case["query"],
                "original_retrieval_ownership": comparison.original_retrieval_ownership.model_dump(),
                "contract_ownership_decision": comparison.contract_ownership_decision.model_dump(),
                "difference_type": comparison.difference_type,
                "difference_reason": comparison.difference_reason,
            }
        )

    counts = Counter(result["difference_type"] for result in results)
    contract_correct = sum(
        result["contract_ownership_decision"]["primary_domain"]
        == case["expected_contract_primary"]
        for result, case in zip(results, fixture["cases"])
    )
    output = {
        "scope": "batch16-j-compare-mode",
        "fixture_version": fixture["fixture_version"],
        "total_cases": len(results),
        "contract_accuracy": pct(contract_correct, len(results)),
        "difference_type_counts": {key: counts[key] for key in ("A", "B", "C", "D")},
        "d_count": counts["D"],
        "a_case_ids": [result["id"] for result in results if result["difference_type"] == "A"],
        "results": results,
    }
    print(json.dumps(output, ensure_ascii=False, indent=None if args.compact else 2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
