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


def selected_guides(result):
    return [
        item
        for item in result.selected_evidence
        if item.source_type == "guide"
    ]


def stability_score(core_terms, guides):
    return int(bool(core_terms)) + int(bool(guides))


def print_result(question, result):
    core_terms = getattr(result.plan, "core_terms", []) or []
    guides = selected_guides(result)
    score = stability_score(core_terms, guides)

    print("=" * 80)
    print("question:", question)
    print("core_terms:", core_terms)
    print("selected guides:")
    if guides:
        for item in guides:
            print("  -", item.id, item.title)
    else:
        print("  - none")
    print("score:", f"{score}/2")
    print()

    return {
        "question": question,
        "score": score,
        "core_terms_empty": not bool(core_terms),
        "no_guide_selected": not bool(guides),
    }


def print_summary(rows):
    core_terms_empty = [
        item["question"]
        for item in rows
        if item["core_terms_empty"]
    ]
    no_guide_selected = [
        item["question"]
        for item in rows
        if item["no_guide_selected"]
    ]
    total_score = sum(item["score"] for item in rows)
    max_score = len(rows) * 2

    print("=" * 80)
    print("summary")
    print("cases:", len(rows))
    print("total_score:", f"{total_score}/{max_score}")
    print("average_score:", f"{(total_score / len(rows)):.2f}/2" if rows else "0.00/2")
    print("core_terms_empty_count:", len(core_terms_empty))
    for question in core_terms_empty:
        print("  -", question)
    print("no_guide_selected_count:", len(no_guide_selected))
    for question in no_guide_selected:
        print("  -", question)


def main():
    rows = []
    for question in QUESTIONS:
        result = run_retrieval_v2(question)
        rows.append(print_result(question, result))

    print_summary(rows)


if __name__ == "__main__":
    main()
