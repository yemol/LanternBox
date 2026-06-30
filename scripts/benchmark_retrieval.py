# 现在支持三种模式：
# python scripts/benchmark_retrieval.py
# 默认只输出总分和失败摘要。
# python scripts/benchmark_retrieval.py --verbose
# 输出每个 case 的简版结果。
# python scripts/benchmark_retrieval.py --debug
# 输出完整调试信息，包括 Query Profile 和 Top Guide Candidates。
# 启用AI模式
# LANTERNBOX_BENCHMARK_AI_RERANK=1 python scripts/benchmark_retrieval.py

from pathlib import Path
import argparse
import json
import os
import sys

PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from api.context.engine import analyze_context
from api.retrieval.strategy import build_retrieval_strategy
from api.retrieval.guide import build_guide_query
from api.retrieval.candidates import build_guide_candidates
from api.resources import load_local_resources
from api.config import GUIDES_CACHE


AI_RERANK_CASE_IDS = {
    "disaster_001",
    "security_001",
    "security_002",
    "water_002",
}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run LanternBox retrieval benchmark."
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print per-case benchmark summary.",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Print full query profile, rerank info, and top guide candidates.",
    )
    return parser.parse_args()


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


def guide_title_matches_expected(actual_guide, expected_top1, acceptable_top1=None):
    if expected_top1 is None:
        return actual_guide is None

    if actual_guide is None:
        return False

    acceptable_titles = [expected_top1]

    if acceptable_top1:
        acceptable_titles.extend(acceptable_top1)

    actual_title = actual_guide.get("title")
    aliases = actual_guide.get("top1_aliases") or []

    for expected_title in acceptable_titles:
        if actual_title == expected_title:
            return True

        if expected_title in aliases:
            return True

        if actual_title and (
            actual_title.startswith(expected_title)
            or expected_title.startswith(actual_title)
        ):
            return True

    return False


def print_case_summary(
    *,
    case,
    question,
    expected_domains,
    actual_domains,
    domain_ok,
    expected_intents,
    actual_intents,
    intent_ok,
    expected_top1,
    rule_top1,
    actual_top1,
    actual_top1_guide,
    guide_ok,
):
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
    print("Rule Top1        :", rule_top1)
    print("Final Top1       :", actual_top1)

    if actual_top1_guide:
        print("Rerank Mode      :", actual_top1_guide.get("_guide_rerank_mode"))
        print("Rerank Used AI   :", actual_top1_guide.get("_guide_rerank_used_ai"))

    print("PASS" if guide_ok else "FAIL")


def print_failed_case_summary(failed_cases):
    print(f"Failed Cases: {len(failed_cases)}")

    if not failed_cases:
        return

    print()
    print("Failed Case Summary:")

    for item in failed_cases:
        print("-" * 70)
        print(item["id"])
        print(item.get("question", ""))

        if not item["context_ok"]:
            print("Context FAIL")
            print("  Expected Domains:", item["expected_domains"])
            print("  Actual Domains  :", item["actual_domains"])

        if not item["intent_ok"]:
            print("Intent FAIL")
            print("  Expected Intents:", item["expected_intents"])
            print("  Actual Intents  :", item["actual_intents"])

        if not item["guide_ok"]:
            print("Guide FAIL")
            print("  Expected Top1:", item["expected_top1"])
            print("  Rule Top1    :", item["rule_top1"])
            print("  Final Top1   :", item["final_top1"])


def main():
    args = parse_args()
    verbose = args.verbose or args.debug
    debug = args.debug

    load_local_resources()

    with open(
        "data/retrieval_benchmark.json",
        "r",
        encoding="utf-8",
    ) as f:
        benchmark = json.load(f)

    total = 0
    context_pass = 0
    intent_pass = 0
    guide_pass = 0
    failed_cases = []

    for case in benchmark["cases"]:
        if not case.get("enabled", True):
            continue

        total += 1
        question = case["question"]

        context = analyze_context(question)
        strategy = build_retrieval_strategy(context)

        if (
            os.getenv("LANTERNBOX_BENCHMARK_AI_RERANK") == "1"
            and case["id"] in AI_RERANK_CASE_IDS
        ):
            strategy["enable_ai_rerank"] = True
        else:
            strategy["enable_ai_rerank"] = False

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
        acceptable_top1 = expected.get("acceptable_top1", [])

        domain_ok = actual_domains == expected_domains
        intent_ok = actual_intents == expected_intents
        guide_ok = guide_title_matches_expected(
            actual_top1_guide,
            expected_top1,
            acceptable_top1,
        )

        rule_top1 = None
        if actual_top1_guide:
            rule_top1 = actual_top1_guide.get("_rule_top1_title") or actual_top1

        if domain_ok:
            context_pass += 1

        if intent_ok:
            intent_pass += 1

        if guide_ok:
            guide_pass += 1

        case_failed = not (domain_ok and intent_ok and guide_ok)

        if case_failed:
            failed_cases.append(
                {
                    "id": case["id"],
                    "question": question,
                    "context_ok": domain_ok,
                    "intent_ok": intent_ok,
                    "guide_ok": guide_ok,
                    "expected_domains": expected_domains,
                    "actual_domains": actual_domains,
                    "expected_intents": expected_intents,
                    "actual_intents": actual_intents,
                    "expected_top1": expected_top1,
                    "rule_top1": rule_top1,
                    "final_top1": actual_top1,
                }
            )

        if verbose:
            print_case_summary(
                case=case,
                question=question,
                expected_domains=expected_domains,
                actual_domains=actual_domains,
                domain_ok=domain_ok,
                expected_intents=expected_intents,
                actual_intents=actual_intents,
                intent_ok=intent_ok,
                expected_top1=expected_top1,
                rule_top1=rule_top1,
                actual_top1=actual_top1,
                actual_top1_guide=actual_top1_guide,
                guide_ok=guide_ok,
            )

            if debug:
                print()
                print("Query Profile    :", query_profile)
                print_top_guides_debug(guides)

    print()
    print("=" * 70)
    print("Benchmark Finished")
    print()
    print(f"Total Cases : {total}")

    if total:
        print(f"Context : {context_pass}/{total} ({context_pass / total * 100:.1f}%)")
        print(f"Intent  : {intent_pass}/{total} ({intent_pass / total * 100:.1f}%)")
        print(f"Guide   : {guide_pass}/{total} ({guide_pass / total * 100:.1f}%)")
    else:
        print("Context : 0/0 (0.0%)")
        print("Intent  : 0/0 (0.0%)")
        print("Guide   : 0/0 (0.0%)")

    print("=" * 70)
    print_failed_case_summary(failed_cases)


if __name__ == "__main__":
    main()
