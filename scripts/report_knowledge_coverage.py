#!/usr/bin/env python3
"""Report LanternBox Guide/Wiki knowledge coverage.

Read-only coverage report. This script does not modify Guide, Wiki, database,
or retrieval code.
"""

from __future__ import annotations

import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parent.parent
GUIDE_DIR = ROOT / "data" / "guides"
WIKI_DIR = ROOT / "wiki_import"

GUIDE_LOW_THRESHOLD = 20
WIKI_LOW_THRESHOLD = 20
PRIORITIES = ("P0", "P1", "P2")
UNKNOWN_PRIORITY = "UNKNOWN"
UNKNOWN_LIST_LIMIT = 80

CANONICAL_GUIDE_CATEGORIES = (
    # P0
    "水与食物保障",
    "医疗急救与照护",
    "能源照明与设备",
    "工具维修与替代制作",
    "居住卫生与空间管理",
    "安全与风险",
    "通信信息与数据安全",
    "撤离交通与路线",
    "灾害应对与灾后恢复",
    "个人装备与防护",
    # P1
    "种植养殖与采集",
    "组织秩序与训练演练",
    "外部接触与交换风险",
    "心理韧性与冲突降温",
    # P2
    "库存资料与成员档案",
    "地图地形与环境监测",
    "信息保存与长期重建",
)

CANONICAL_GUIDE_CATEGORY_SET = set(CANONICAL_GUIDE_CATEGORIES)

GUIDE_CATEGORY_ALIASES = {
    "水": "水与食物保障",
    "食物": "水与食物保障",
    "食物获取与口粮管理": "水与食物保障",
    "医疗急救": "医疗急救与照护",
    "医疗与照护": "医疗急救与照护",
    "能源": "能源照明与设备",
    "工具维修与替代": "工具维修与替代制作",
    "维修 / 制作 / 替代 / 拆解再利用": "工具维修与替代制作",
    "卫生": "居住卫生与空间管理",
    "卫生与清洁": "居住卫生与空间管理",
    "庇护空间分区": "居住卫生与空间管理",
    "污染控制 / 隔离 / 清洁分区": "居住卫生与空间管理",
    "火源保温与通风": "居住卫生与空间管理",
    "火源 / 保温 / 通风 / 一氧化碳风险": "居住卫生与空间管理",
    "安全": "安全与风险",
    "通讯": "通信信息与数据安全",
    "通讯与记录": "通信信息与数据安全",
    "通信与信息": "通信信息与数据安全",
    "避难转移": "撤离交通与路线",
    "衣物 / 鞋袜 / 体温防护": "个人装备与防护",
    "种植": "种植养殖与采集",
    "小规模养殖": "种植养殖与采集",
    "野外食物获取 / 狩猎捕捞 / 动物蛋白补充": "种植养殖与采集",
    "团队轮值与任务管理": "组织秩序与训练演练",
    "外部接触与物资交换风险": "外部接触与交换风险",
}


def iter_json_files(path: Path) -> Iterable[Path]:
    return sorted(path.glob("**/*.json"))


def iter_wiki_files(path: Path) -> Iterable[Path]:
    return sorted(path.glob("*/*.md"))


def status(count: int, threshold: int) -> str:
    return "LOW" if count < threshold else "OK"


def canonical_guide_category(category: str, priority: str) -> str:
    if category == "风险决策":
        return "安全与风险" if priority == "P0" else "组织秩序与训练演练"
    if category in CANONICAL_GUIDE_CATEGORY_SET:
        return category
    return GUIDE_CATEGORY_ALIASES.get(category, category)


def guide_directory(path: Path) -> str:
    try:
        return path.relative_to(GUIDE_DIR).parts[0]
    except (ValueError, IndexError):
        return "(root)"


def wiki_directory(path: Path) -> str:
    try:
        return path.relative_to(WIKI_DIR).parts[0]
    except (ValueError, IndexError):
        return "(root)"


def parse_wiki_frontmatter(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---"):
        return {}

    parts = text.split("---", 2)
    if len(parts) < 3:
        return {}

    meta: dict[str, str] = {}
    for raw_line in parts[1].splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or ":" not in line:
            continue
        key, value = line.split(":", 1)
        meta[key.strip()] = value.strip().strip('"').strip("'")
    return meta


def load_guides() -> tuple[list[dict[str, str]], list[str]]:
    guides: list[dict[str, str]] = []
    warnings: list[str] = []

    for path in iter_json_files(GUIDE_DIR):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            warnings.append(f"{path.relative_to(ROOT)}: JSON parse error: {exc}")
            continue

        category = str(data.get("category") or guide_directory(path))
        priority = str(data.get("priority") or UNKNOWN_PRIORITY)
        guides.append(
            {
                "path": str(path.relative_to(ROOT)),
                "directory": guide_directory(path),
                "category": category,
                "canonical_category": canonical_guide_category(category, priority),
                "priority": priority,
            }
        )

    return guides, warnings


def load_wikis() -> list[dict[str, str]]:
    wikis: list[dict[str, str]] = []

    for path in iter_wiki_files(WIKI_DIR):
        meta = parse_wiki_frontmatter(path)
        wikis.append(
            {
                "path": str(path.relative_to(ROOT)),
                "directory": wiki_directory(path),
                "category": meta.get("category", ""),
                "priority": meta.get("priority", UNKNOWN_PRIORITY) or UNKNOWN_PRIORITY,
            }
        )

    return wikis


def print_counter_table(title: str, counter: Counter[str], threshold: int) -> None:
    print(f"\n## {title}")
    print(f"Threshold: {threshold}")
    print("| field | count | status |")
    print("| --- | ---: | --- |")
    for name, count in sorted(counter.items(), key=lambda item: (-item[1], item[0])):
        print(f"| {name} | {count} | {status(count, threshold)} |")


def print_priority_coverage(
    title: str,
    records: list[dict[str, str]],
    field: str,
    threshold: int,
) -> dict[str, list[tuple[str, int]]]:
    print(f"\n## {title}")
    lows: dict[str, list[tuple[str, int]]] = {}

    by_priority: dict[str, Counter[str]] = defaultdict(Counter)
    for record in records:
        priority = record.get("priority", UNKNOWN_PRIORITY)
        by_priority[priority][record[field]] += 1

    for priority in PRIORITIES:
        counter = by_priority.get(priority, Counter())
        total = sum(counter.values())
        low_items = [
            (name, count)
            for name, count in sorted(counter.items(), key=lambda item: (-item[1], item[0]))
            if count < threshold
        ]
        lows[priority] = low_items

        print(f"\n### {priority}")
        print(f"Total: {total}")
        if not counter:
            print(f"Status: LOW (no {priority} records)")
            continue

        print("| field | count | status |")
        print("| --- | ---: | --- |")
        for name, count in sorted(counter.items(), key=lambda item: (-item[1], item[0])):
            print(f"| {name} | {count} | {status(count, threshold)} |")

    unknown_total = sum(by_priority.get(UNKNOWN_PRIORITY, Counter()).values())
    if unknown_total:
        print(f"\n### {UNKNOWN_PRIORITY}")
        print(f"Total: {unknown_total}")

    return lows


def summarize_low_items(label: str, counter: Counter[str], threshold: int) -> list[str]:
    return [
        f"{name} ({count})"
        for name, count in sorted(counter.items(), key=lambda item: (item[1], item[0]))
        if count < threshold
    ]


def priority_totals(records: list[dict[str, str]]) -> Counter[str]:
    return Counter(record.get("priority", UNKNOWN_PRIORITY) for record in records)


def print_priority_distribution(title: str, records: list[dict[str, str]]) -> Counter[str]:
    totals = priority_totals(records)
    print(f"\n## {title}")
    print("| priority | count |")
    print("| --- | ---: |")
    for priority in (*PRIORITIES, UNKNOWN_PRIORITY):
        print(f"| {priority} | {totals.get(priority, 0)} |")
    return totals


def print_unknown_files(title: str, records: list[dict[str, str]], limit: int = UNKNOWN_LIST_LIMIT) -> None:
    unknown_records = [
        record for record in sorted(records, key=lambda item: item["path"])
        if record.get("priority") == UNKNOWN_PRIORITY
    ]
    print(f"\n## {title}")
    print(f"Total UNKNOWN: {len(unknown_records)}")
    if not unknown_records:
        print("None")
        return

    print("| path | directory | category |")
    print("| --- | --- | --- |")
    for record in unknown_records[:limit]:
        print(f"| {record['path']} | {record['directory']} | {record.get('category', '')} |")
    if len(unknown_records) > limit:
        print(f"| ... | ... | {len(unknown_records) - limit} more not shown |")


def print_low_list(title: str, items: list[str]) -> None:
    print(f"\n## {title}")
    if not items:
        print("None")
        return
    for item in items:
        print(f"- {item}")


def main() -> None:
    guides, guide_warnings = load_guides()
    wikis = load_wikis()

    guide_by_category = Counter(record["category"] for record in guides)
    canonical_guide_by_category = Counter(record["canonical_category"] for record in guides)
    guide_by_directory = Counter(record["directory"] for record in guides)
    wiki_by_directory = Counter(record["directory"] for record in wikis)

    print("# LanternBox Knowledge Coverage Report")
    print(f"Root: {ROOT}")
    print(f"Guide JSON files: {len(guides)}")
    print(f"Wiki Markdown files: {len(wikis)}")

    print_counter_table("Raw Guide category coverage", guide_by_category, GUIDE_LOW_THRESHOLD)
    print_counter_table(
        "Canonical Guide category coverage",
        canonical_guide_by_category,
        GUIDE_LOW_THRESHOLD,
    )
    print_counter_table("Guide coverage by directory", guide_by_directory, GUIDE_LOW_THRESHOLD)
    print_counter_table("Wiki coverage by top-level directory", wiki_by_directory, WIKI_LOW_THRESHOLD)

    guide_priority_totals = print_priority_distribution("Guide priority distribution", guides)
    wiki_priority_totals = print_priority_distribution("Wiki priority distribution", wikis)

    guide_priority_lows = print_priority_coverage(
        "P0 / P1 / P2 Raw Guide coverage by category",
        guides,
        "category",
        GUIDE_LOW_THRESHOLD,
    )
    canonical_guide_priority_lows = print_priority_coverage(
        "P0 / P1 / P2 Canonical Guide coverage by category",
        guides,
        "canonical_category",
        GUIDE_LOW_THRESHOLD,
    )
    wiki_priority_lows = print_priority_coverage(
        "P0 / P1 / P2 Wiki coverage by directory",
        wikis,
        "directory",
        WIKI_LOW_THRESHOLD,
    )

    low_guides = summarize_low_items("Guide", guide_by_category, GUIDE_LOW_THRESHOLD)
    canonical_low_guides = summarize_low_items(
        "Canonical Guide",
        canonical_guide_by_category,
        GUIDE_LOW_THRESHOLD,
    )
    low_wikis = summarize_low_items("Wiki", wiki_by_directory, WIKI_LOW_THRESHOLD)

    print_unknown_files("UNKNOWN Guide priority files", guides)
    print_unknown_files("UNKNOWN Wiki priority files", wikis)
    print_low_list("Raw LOW Guide categories", low_guides)
    print_low_list("Canonical LOW Guide categories", canonical_low_guides)
    print_low_list("LOW Wiki directories", low_wikis)

    print("\n## Summary")
    print(f"- Guides total: {len(guides)}")
    print(f"- Wiki entries total: {len(wikis)}")
    print(f"- Raw Guide categories: {len(guide_by_category)}")
    print(f"- Raw Guide categories LOW (<{GUIDE_LOW_THRESHOLD}): {len(low_guides)}")
    print(f"- Canonical Guide categories: {len(canonical_guide_by_category)}")
    print(
        f"- Canonical Guide categories LOW (<{GUIDE_LOW_THRESHOLD}): "
        f"{len(canonical_low_guides)}"
    )
    print(f"- Wiki directories: {len(wiki_by_directory)}")
    print(f"- Wiki directories LOW (<{WIKI_LOW_THRESHOLD}): {len(low_wikis)}")
    for priority in PRIORITIES:
        guide_total = guide_priority_totals.get(priority, 0)
        wiki_total = wiki_priority_totals.get(priority, 0)
        guide_priority_status = "LOW" if guide_total == 0 else "OK"
        wiki_priority_status = "LOW" if wiki_total == 0 else "OK"
        print(
            f"- {priority} Guide total: {guide_total} ({guide_priority_status}); "
            f"raw LOW fields: {len(guide_priority_lows.get(priority, []))}; "
            f"canonical LOW fields: {len(canonical_guide_priority_lows.get(priority, []))}"
        )
        print(
            f"- {priority} Wiki total: {wiki_total} ({wiki_priority_status}); "
            f"LOW fields: {len(wiki_priority_lows.get(priority, []))}"
        )
    if guide_priority_totals.get(UNKNOWN_PRIORITY, 0):
        print(f"- UNKNOWN Guide priority total: {guide_priority_totals[UNKNOWN_PRIORITY]}")
    if wiki_priority_totals.get(UNKNOWN_PRIORITY, 0):
        print(f"- UNKNOWN Wiki priority total: {wiki_priority_totals[UNKNOWN_PRIORITY]}")

    if low_guides:
        print(f"- Raw LOW Guide categories: {', '.join(low_guides)}")
    if canonical_low_guides:
        print(f"- Canonical LOW Guide categories: {', '.join(canonical_low_guides)}")
    if low_wikis:
        print(f"- LOW Wiki directories: {', '.join(low_wikis)}")
    if guide_warnings:
        print(f"- Guide parse warnings: {len(guide_warnings)}")
        for warning in guide_warnings:
            print(f"  - {warning}")


if __name__ == "__main__":
    main()
