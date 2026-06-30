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


def print_top_guides_debug(guides, limit=5):
    if not guides:
        print()
        print("Top Guide Candidates: []")
        return

    print()
    print("Top Guide Candidates:")
    for idx, guide in enumerate(guides[:limit], 1):
        title = guide.get("title")
        score = guide.get("_match_score")
        reason = guide.get("_match_reason")
        domains = guide.get("domains") or guide.get("domain") or guide.get("category")
        intents = guide.get("intents")

        print(
            f"{idx}. {title} "
            f"score={score} "
            f"domains={domains} "
            f"intents={intents} "
            f"reason={reason}"
        )


def guide_title_matches_expected(actual_guide, expected_top1):
    if expected_top1 is None:
        return actual_guide is None

    if actual_guide is None:
        return False

    actual_title = actual_guide.get("title")

    if actual_title == expected_top1:
        return True

    aliases = actual_guide.get("top1_aliases") or []

    if expected_top1 in aliases:
        return True

    # 允许 benchmark 使用短标题，
    # 例如 expected=大规模停水，actual=大规模停水第一天。
    if actual_title and (
        actual_title.startswith(expected_top1)
        or expected_top1.startswith(actual_title)
    ):
        return True

    return False



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
    query_profile = build_guide_query(strategy)

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

    actual_top1_guide = guides[0] if guides else None
    actual_top1 = actual_top1_guide.get("title") if actual_top1_guide else None

    expected_top1 = expected.get("top1")

    domain_ok = actual_domains == expected_domains
    intent_ok = actual_intents == expected_intents
    guide_ok = guide_title_matches_expected(
        actual_top1_guide,
        expected_top1,
    )

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

    print("Query Profile    :", query_profile)

    print()

    print("Expected Top1    :", expected_top1)
    print("Actual Top1      :", actual_top1)
    print("PASS" if guide_ok else "FAIL")

    print_top_guides_debug(guides)


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