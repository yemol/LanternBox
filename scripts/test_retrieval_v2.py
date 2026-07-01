import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.orchestrator import run_retrieval_v2

questions = [
    "吃的东西快要吃完了",
    "水只剩一桶了，而且今天很热",
    "外面暴雨水位上涨，家里还有食物和水，我先看哪些资料",
    "门外晚上有动静，我该怎么办",
    "插线板泡过水还能用吗",
]

for q in questions:
    print("=" * 80)
    print("Q:", q)
    result = run_retrieval_v2(q)
    print("PLAN:", result.plan.scenario_summary)
    print("NEEDS:", result.plan.needs)
    print("CORE TERMS:", getattr(result.plan, "core_terms", []))
    print("SOURCE PLAN:")
    for item in result.plan.source_plan:
        print(
            "  -",
            item.source_type,
            "| query:",
            item.query,
            "| keywords:",
            item.keywords,
            "| limit:",
            item.limit,
        )
    print("CANDIDATES:", len(result.candidates))
    source_counts = {}
    for item in result.candidates:
        source_counts[item.source_type] = source_counts.get(item.source_type, 0) + 1
    print("CANDIDATE TYPES:", source_counts)

    print("CANDIDATE TITLES:")
    for item in result.candidates[:12]:
        print("  -", item.source_type, item.id, item.title)
    print("SELECTED:")
    for item in result.selected_evidence:
        print("-", item.source_type, item.id, item.title)
    print()
