import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"

if not os.environ.get("LANTERNBOX_RETRIEVAL_V2_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_RETRIEVAL_V2_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])

sys.path.insert(0, str(ROOT))

from api.retrieval_v2.orchestrator import run_retrieval_v2


TEST_CASES = [
    # food
    {"question": "吃的东西快要吃完了，今晚开始要怎么分配", "expected_domain": "food"},
    {"question": "家里只剩两包饼干和一点米，三个人要撑两天，怎么分配", "expected_domain": "food"},
    {"question": "停电一天了，冰箱里的肉和熟食应该先吃哪些", "expected_domain": "food"},
    {"question": "罐头鼓起来了，还有一点怪味，能不能煮一下再吃", "expected_domain": "food"},
    {"question": "主食不多了，明天还要搬东西，饭量怎么安排", "expected_domain": "food"},
    {"question": "孩子和老人都在，食物不够时谁应该优先吃什么", "expected_domain": "food"},
    {"question": "剩饭放了一夜又没有冰箱，还能不能继续吃", "expected_domain": "food"},
    {"question": "只有干粮和罐头，怎么安排三天不浪费", "expected_domain": "food"},
    {"question": "食物残渣开始发臭，怎么处理才不招虫", "expected_domain": "food"},
    {"question": "大家都很饿，有人想先把零食吃完，我该怎么定规则", "expected_domain": "food"},

    # water
    {"question": "水只剩一桶了，而且今天很热", "expected_domain": "water"},
    {"question": "饮用水只够今天，明天还不确定有没有水，我先做什么", "expected_domain": "water"},
    {"question": "刚停水了，家里还有几个空桶和锅，第一小时该怎么接水", "expected_domain": "water"},
    {"question": "雨水接了一盆，有点浑浊，能不能过滤煮开喝", "expected_domain": "water"},
    {"question": "桶里的水有塑料味，能不能继续当饮用水", "expected_domain": "water"},
    {"question": "高温下老人一直出汗，水又不多，怎么补水和降温", "expected_domain": "water"},
    {"question": "洗手和喝水都要用水，水不够时怎么排序", "expected_domain": "water"},
    {"question": "储水桶以前装过清洁剂，现在能不能清洗后装饮用水", "expected_domain": "water"},
    {"question": "水里有油膜和怪味，煮开以后能喝吗", "expected_domain": "water"},
    {"question": "多人共用一桶饮用水，怎么记录消耗防止用太快", "expected_domain": "water"},

    # medical
    {"question": "手臂被铁皮划伤出血了，水也不多，怎样先处理伤口", "expected_domain": "medical"},
    {"question": "孩子发热又咳嗽，家里药不多，怎么观察和照护", "expected_domain": "medical"},
    {"question": "有人腹泻好几次，还吐了，怎么判断有没有脱水", "expected_domain": "medical"},
    {"question": "手被热水烫红起泡了，第一步应该怎么处理", "expected_domain": "medical"},
    {"question": "小伤口沾了泥，干净水不多，怎么清洗和覆盖", "expected_domain": "medical"},
    {"question": "老人发烧不太想喝水，尿也少，应该观察什么", "expected_domain": "medical"},
    {"question": "脚踝扭了但还要转移，怎么先固定和减少负担", "expected_domain": "medical"},
    {"question": "有人流鼻血了，仰头止血可以吗", "expected_domain": "medical"},
    {"question": "伤口周围越来越红还发热，是不是要停止普通处理", "expected_domain": "medical"},
    {"question": "处理呕吐物后还要照顾孩子，怎样避免交叉感染", "expected_domain": "medical"},

    # power
    {"question": "停电后晚上要照明，但电池不多，灯怎么安排", "expected_domain": "power"},
    {"question": "手机只剩百分之十电，还要保持联系，应该关哪些功能", "expected_domain": "power"},
    {"question": "移动电源只剩一半电，手机、手电和收音机先给谁充", "expected_domain": "power"},
    {"question": "太阳能板白天能用，怎么安排充电顺序才不浪费", "expected_domain": "power"},
    {"question": "充电宝有点鼓起来还发热，还能继续用吗", "expected_domain": "power"},
    {"question": "晚上值守需要灯，但又怕太亮暴露，怎么布置照明", "expected_domain": "power"},
    {"question": "电池混在一起不知道哪些有电，怎么轮换记录", "expected_domain": "power"},
    {"question": "延长线被雨水打湿了，插上电会不会漏电", "expected_domain": "power"},
    {"question": "插线板泡过水还能用吗", "expected_domain": "power"},
    {"question": "充电线外皮破了，临时用胶带缠一下可以吗", "expected_domain": "power"},

    # security
    {"question": "门外晚上有动静，我该怎么办", "expected_domain": "security"},
    {"question": "半夜门外有人低声说话，还敲了一下门，我该怎么处理", "expected_domain": "security"},
    {"question": "闻到燃气味，厨房还有蜡烛和酒精炉，能不能点火查看", "expected_domain": "security"},
    {"question": "门窗被风吹松了，晚上怎么临时加固又不暴露", "expected_domain": "security"},
    {"question": "陌生人来敲门说要换物资，我应该怎么降低风险", "expected_domain": "security"},
    {"question": "夜里必须去厕所，怎么用灯才不容易暴露位置", "expected_domain": "security"},
    {"question": "外出找水前，家里人要怎么记录路线和返回时间", "expected_domain": "security"},
    {"question": "有人想单独出去找吃的，我该怎么评估风险", "expected_domain": "security"},
    {"question": "门口玻璃碎了，怎么先处理不让孩子靠近", "expected_domain": "security"},
    {"question": "家里人因为分配物资吵起来，怎么先降温避免冲突", "expected_domain": "security"},

    # disaster
    {"question": "外面暴雨水位上涨，家里还有食物和水，我先看哪些资料", "expected_domain": "disaster"},
    {"question": "暴雨还在下，门口已经开始积水，需不需要先把东西搬高", "expected_domain": "disaster"},
    {"question": "如果明天必须转移避难，今晚要先收拾哪些东西", "expected_domain": "disaster"},
    {"question": "水已经进到客厅了，电器和插排要先怎么处理", "expected_domain": "disaster"},
    {"question": "外面有烟味飘进屋里，窗户要开还是封起来", "expected_domain": "disaster"},
    {"question": "临时避难点只有一个房间，睡眠区和污染区怎么分开", "expected_domain": "disaster"},
    {"question": "必须撤离但东西太多，哪些物资应该放弃", "expected_domain": "disaster"},
    {"question": "出去探路的人还没回来，超过约定时间怎么办", "expected_domain": "disaster"},
    {"question": "洪水退了以后，能不能马上回去拿冰箱里的食物", "expected_domain": "disaster"},
    {"question": "手机没信号，网络也断了，怎么和家人保持通讯计划", "expected_domain": "disaster"},
]


def selected_guides(result):
    return [
        item
        for item in result.selected_evidence
        if item.source_type == "guide"
    ]


def stability_score(core_terms, guides):
    return int(bool(core_terms)) + int(bool(guides))


def print_result(test_case, result):
    question = test_case["question"]
    expected_domain = test_case.get("expected_domain", "")
    core_terms = getattr(result.plan, "core_terms", []) or []
    guides = selected_guides(result)
    score = stability_score(core_terms, guides)

    print("=" * 80)
    print("question:", question)
    print("expected_domain:", expected_domain)
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
        "expected_domain": expected_domain,
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
    for test_case in TEST_CASES:
        result = run_retrieval_v2(test_case["question"])
        rows.append(print_result(test_case, result))

    print_summary(rows)


if __name__ == "__main__":
    main()
