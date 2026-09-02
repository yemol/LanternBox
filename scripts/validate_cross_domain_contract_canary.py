#!/usr/bin/env python3
"""Validate the Batch16 production canary without changing runtime settings."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))


from api.config import CONTRACT_CANARY_TRANSITION_IDS
from api.retrieval_v2.contract_canary import (
    apply_contract_canary_candidates,
)
from api.retrieval_v2.contract_compare import (
    original_retrieval_ownership,
)
from api.retrieval_v2.fetchers import fetch_guide_candidates
from api.retrieval_v2.schemas import (
    RetrievalPlan,
    SourcePlanItem,
)


FIXTURE = (
    ROOT
    / "tests/fixtures/cross_domain_contract_shadow_validation_cases.json"
)


def _case_result(case: dict[str, object]) -> dict[str, object]:
    query = str(case["query"])
    source = SourcePlanItem(
        source_type="guide",
        query=query,
        limit=8,
    )
    plan = RetrievalPlan(
        scenario_summary=query,
        source_plan=[source],
    )
    baseline = fetch_guide_candidates(source, user_message=query)
    before = original_retrieval_ownership(baseline).primary_domain
    result = apply_contract_canary_candidates(
        query,
        plan,
        baseline,
        mode="production",
        canary_enabled=True,
        enabled_transition_ids=CONTRACT_CANARY_TRANSITION_IDS,
    )
    after = (
        result.context.contract_primary_domain
        if result.context.ownership_changed
        else before
    )
    return {
        "id": case["id"],
        "before": before,
        "after": after,
        "expected": case["expected_contract_primary"],
        "status": result.context.status,
        "transition_id": result.context.transition_id,
        "safety_boundary_present": (
            result.context.safety_boundary_present
        ),
        "passed": (
            after == case["expected_contract_primary"]
            if result.context.status == "CANARY_APPLIED"
            else before in case["accepted_retrieval_primary"]
        ),
    }


def main() -> int:
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    results = [
        _case_result(case)
        for case in fixture["cases"]
    ]
    statuses = Counter(
        str(result["status"])
        for result in results
    )
    applied = [
        result
        for result in results
        if result["status"] == "CANARY_APPLIED"
    ]
    payload = {
        "scope": "batch16-p-production-canary",
        "total_cases": len(results),
        "passed": sum(
            bool(result["passed"])
            for result in results
        ),
        "status_counts": dict(statuses),
        "applied_case_ids": [
            result["id"]
            for result in applied
        ],
        "ownership_changed_count": len(applied),
        "difference_d_count": sum(
            not bool(result["passed"])
            for result in results
        ),
        "results": results,
    }
    print(json.dumps(payload, ensure_ascii=False, indent=2))
    return (
        0
        if payload["passed"] == payload["total_cases"]
        else 1
    )


if __name__ == "__main__":
    raise SystemExit(main())
