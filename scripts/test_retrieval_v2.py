import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.orchestrator import run_retrieval_v2


QUESTIONS = [
    "吃的东西快要吃完了",
    "水只剩一桶了，而且今天很热",
    "外面暴雨水位上涨，家里还有食物和水，我先看哪些资料",
    "门外晚上有动静，我该怎么办",
    "插线板泡过水还能用吗",
    "家里只剩两包饼干和一点米，三个人要撑两天，怎么分配",
    "饮用水只够今天，明天还不确定有没有水，我先做什么",
    "屋里很闷热，老人一直出汗，电风扇也没法一直开",
    "暴雨还在下，门口已经开始积水，需不需要先把东西搬高",
    "半夜门外有人低声说话，还敲了一下门，我该怎么处理",
    "延长线被雨水打湿了，插上电会不会漏电",
    "孩子发热又咳嗽，家里药不多，怎么观察和照护",
    "手臂被铁皮划伤出血了，水也不多，怎样先处理伤口",
    "停电后晚上要照明，但电池不多，灯怎么安排",
    "手机只剩百分之十电，还要保持联系，应该关哪些功能",
    "闻到燃气味，厨房还有蜡烛和酒精炉，能不能点火查看",
    "厕所不能冲水了，家里还有几个人，临时厕所怎么安排",
    "垃圾开始发臭，有食物残渣和湿垃圾，怎么处理才不招虫",
    "如果明天必须转移避难，今晚要先收拾哪些东西",
    "手机没信号，网络也断了，怎么和家人保持通讯计划",
]


def candidate_type_counts(candidates):
    counts = {}
    for item in candidates:
        counts[item.source_type] = counts.get(item.source_type, 0) + 1
    return counts


def print_source_plan(source_plan):
    for item in source_plan:
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


def print_selected_evidence(selected_evidence):
    if not selected_evidence:
        print("  - none")
        return

    for item in selected_evidence:
        print("  -", item.source_type, item.id, item.title)


def print_full_result(question, result):
    print("=" * 80)
    print("question:", question)
    print("scenario_summary:", result.plan.scenario_summary)
    print("core_terms:", getattr(result.plan, "core_terms", []))
    print("source_plan:")
    print_source_plan(result.plan.source_plan)
    print("candidate count:", len(result.candidates))
    print("candidate types:", candidate_type_counts(result.candidates))
    print("selected evidence:")
    print_selected_evidence(result.selected_evidence)
    print()


def print_compact_result(question, result):
    print("=" * 80)
    print("question:", question)
    print("core_terms:", getattr(result.plan, "core_terms", []))
    print("selected evidence:")
    print_selected_evidence(result.selected_evidence)
    print()


def main():
    parser = argparse.ArgumentParser(description="Run Retrieval v2 scenario checks.")
    parser.add_argument(
        "--compact",
        action="store_true",
        help="Only print question, core_terms, and selected evidence.",
    )
    args = parser.parse_args()

    for question in QUESTIONS:
        result = run_retrieval_v2(question)
        if args.compact:
            print_compact_result(question, result)
        else:
            print_full_result(question, result)


if __name__ == "__main__":
    main()
