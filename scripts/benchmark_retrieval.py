from pathlib import Path
import sys
PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

import json

from api.context.engine import analyze_context
from api.retrieval.strategy import build_retrieval_strategy
from api.retrieval.guide import build_guide_query
from api.retrieval.candidates import build_guide_candidates
from api.resources import load_local_resources
from api.config import GUIDES_CACHE

load_local_resources()

with open(
    "data/retrieval_benchmark.json",
    "r",
    encoding="utf-8",
) as f:
    benchmark = json.load(f)

cases = benchmark["cases"]

total = 0

context_pass = 0
intent_pass = 0
guide_pass = 0

for case in benchmark["cases"]:

    if not case.get("enabled", True):
        continue

    total += 1

    question = case["question"]

    context = analyze_context(question)

    strategy = build_retrieval_strategy(context)

    guides = build_guide_candidates(
        strategy=strategy,
        user_message=question,
        guides=GUIDES_CACHE,
        matched_triggers=[],
    )

    expected = case["expect"]

    actual_domains = sorted(context.domains)
    actual_intents = sorted(context.intents)

    expected_domains = sorted(expected.get("domains", []))
    expected_intents = sorted(expected.get("intents", []))

    actual_top1 = None
    if guides:
        actual_top1 = guides[0]["title"]

    expected_top1 = expected.get("top1")

    domain_ok = actual_domains == expected_domains
    intent_ok = actual_intents == expected_intents
    guide_ok = actual_top1 == expected_top1

    if domain_ok:
        context_pass += 1

    if intent_ok:
        intent_pass += 1

    if guide_ok:
        guide_pass += 1

    print("=" * 70)
    print(case["id"])
    print(question)

    print()

    print("Expected Domains :", expected_domains)
    print("Actual Domains   :", actual_domains)
    print("PASS" if domain_ok else "FAIL")

    print()

    print("Expected Intents :", expected_intents)
    print("Actual Intents   :", actual_intents)
    print("PASS" if intent_ok else "FAIL")

    print()

    print("Expected Top1    :", expected_top1)
    print("Actual Top1      :", actual_top1)
    print("PASS" if guide_ok else "FAIL")

print()
print("=" * 70)
print("Benchmark Finished")
print()
print(f"Total Cases : {total}")
print(
    f"Context : {context_pass}/{total} ({context_pass/total*100:.1f}%)"
)
print(
    f"Intent  : {intent_pass}/{total} ({intent_pass/total*100:.1f}%)"
)
print(
    f"Guide   : {guide_pass}/{total} ({guide_pass/total*100:.1f}%)"
)
print("=" * 70)