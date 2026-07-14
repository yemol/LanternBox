#!/usr/bin/env python3
"""Run Batch5-F Energy Management retrieval field cases.

This script is test/report only. It does not call an LLM and does not modify
Wiki, Guide, Guide-Wiki relations, profiles, prompt, ranking, or retrieval
pipeline behavior.
"""

from __future__ import annotations

import argparse
import json
import os
import sqlite3
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"
if not os.environ.get("LANTERNBOX_ENERGY_MANAGEMENT_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_ENERGY_MANAGEMENT_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests" / "fixtures" / "energy_management_field_cases.json"
RESULTS_PATH = ROOT / "docs" / "knowledge" / "batch5_f_energy_management_field_test_results.json"
REPORT_PATH = ROOT / "docs" / "knowledge" / "batch5_f_energy_management_field_test_report.md"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"

STOP_TERMS = [
    "停止",
    "停用",
    "禁用",
    "禁止",
    "不可",
    "不要",
    "不能",
    "不再",
    "断开",
    "保留",
    "最低",
    "底线",
    "必须留下",
    "不能全黑",
]
FALLBACK_TERMS = [
    "替代",
    "改用",
    "缺少",
    "没有",
    "暂停",
    "减少",
    "降级",
    "分批",
    "轮流",
    "手写",
    "纸质",
    "只做",
]
RECORD_TERMS = [
    "记录",
    "标记",
    "编号",
    "复查",
    "交接",
    "清单",
    "台账",
    "时间",
    "负责人",
    "借用人",
]
DANGEROUS_PHRASES = ["随便试", "直接试", "通电看看", "测试看看", "一直充", "不用记录", "可以不留余量"]
NEGATION_MARKERS = ["不要", "不可", "不能", "不应", "不得", "不再", "不继续", "禁止", "停止", "停用", "避免"]
SAFE_CONTEXT_MARKERS = ["不适用", "常见错误", "错误操作", "风险提示", "不要", "不可", "不能", "不应", "不得", "不再", "禁止", "停止"]
LOCAL_POWER_DOMAINS = {"power", "records", "security"}
RELATED_ACCEPTABLE_DOMAINS = {"comms", "communication", "computer", "core", "terminal"}
P0_ENERGY_SAFETY_GUIDES = {"DG-0841", "DG-0842", "DG-0843"}
CROSS_DOMAIN_GROUPS = {
    "repair": {"repair", "tools"},
    "communication": {"comms", "communication"},
    "computer": {"computer", "core", "sync", "terminal"},
    "medical": {"medical"},
    "food": {"food"},
    "night_safety": {"shelter", "security"},
}
ROOT_CAUSE_CLASSES = [
    "数据缺口",
    "profile 缺口",
    "selector 问题",
    "ranking 问题",
    "Guide 设计问题",
    "合理 partial",
    "跨域竞争",
    "P0 电气安全误触发",
]


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if value in (None, ""):
        return []
    return [value]


def load_wiki_rows() -> dict[str, dict[str, str]]:
    if not PB_DB.exists():
        return {}
    con = sqlite3.connect(f"file:{PB_DB}?mode=ro", uri=True)
    try:
        rows = con.execute(
            "select id, slug, title, category, risk_level, content, summary from wiki_articles"
        ).fetchall()
    finally:
        con.close()
    return {
        str(slug): {
            "id": str(article_id),
            "slug": str(slug),
            "title": str(title),
            "category": str(category),
            "risk_level": str(risk_level),
            "content": str(content),
            "summary": str(summary),
        }
        for article_id, slug, title, category, risk_level, content, summary in rows
    }


def wiki_slug(candidate: EvidenceCandidate, id_to_slug: dict[str, str]) -> str:
    raw = candidate.raw or {}
    article_id = str(raw.get("id") or candidate.id or "")
    return str(raw.get("slug") or id_to_slug.get(article_id) or candidate.id or "")


def dedupe_candidates(candidates: list[EvidenceCandidate], id_to_slug: dict[str, str]) -> list[EvidenceCandidate]:
    seen: set[tuple[str, str]] = set()
    result: list[EvidenceCandidate] = []
    for candidate in candidates:
        cid = wiki_slug(candidate, id_to_slug) if candidate.source_type == "wiki" else candidate.id
        key = (candidate.source_type, cid)
        if key in seen:
            continue
        seen.add(key)
        result.append(candidate)
    return result


def evidence_text(
    guides: list[EvidenceCandidate],
    wikis: list[EvidenceCandidate],
    wiki_rows: dict[str, dict[str, str]],
    id_to_slug: dict[str, str],
) -> str:
    chunks: list[str] = []
    for guide in guides:
        raw = guide.raw or {}
        chunks.extend(
            [
                str(guide.title or ""),
                str(guide.summary or ""),
                " ".join(str(item) for item in as_list(raw.get("steps"))),
                " ".join(str(item) for item in as_list(raw.get("check"))),
                " ".join(str(item) for item in as_list(raw.get("fallback"))),
                " ".join(str(item) for item in as_list(raw.get("stop_or_escalate"))),
                str(raw.get("notes") or ""),
            ]
        )
    for wiki in wikis:
        slug = wiki_slug(wiki, id_to_slug)
        row = wiki_rows.get(slug)
        if row:
            chunks.extend([row["title"], row["summary"], row["content"]])
        else:
            chunks.extend([wiki.title, wiki.summary, wiki.snippet])
    return "\n".join(chunks)


def contains_any(text: str, terms: list[str]) -> bool:
    return any(term in text for term in terms)


def dangerous_suggestions(text: str) -> list[str]:
    hits: list[str] = []
    for line in text.splitlines():
        compact = line.strip()
        if not compact or any(marker in compact for marker in SAFE_CONTEXT_MARKERS):
            continue
        for phrase in DANGEROUS_PHRASES:
            if phrase not in compact:
                continue
            prefix = compact[: compact.find(phrase)]
            if any(marker in prefix[-12:] for marker in NEGATION_MARKERS):
                continue
            hits.append(phrase)
    return list(dict.fromkeys(hits))


def selected_domains(guides: list[EvidenceCandidate]) -> list[str]:
    domains: list[str] = []
    for guide in guides:
        for domain in as_list((guide.raw or {}).get("domains")):
            text = str(domain)
            if text not in domains:
                domains.append(text)
    return domains


def cross_domain_labels(guides: list[EvidenceCandidate]) -> list[str]:
    if not guides:
        return []
    top_domains = {str(domain) for domain in as_list((guides[0].raw or {}).get("domains"))}
    if not top_domains:
        return []
    labels: list[str] = []
    if not top_domains & (LOCAL_POWER_DOMAINS | RELATED_ACCEPTABLE_DOMAINS):
        labels.append("non_power_primary")
    for label, domains in CROSS_DOMAIN_GROUPS.items():
        if label == "night_safety":
            if top_domains & domains and not top_domains & {"power", "records"}:
                labels.append("power_vs_night_safety")
        elif label in {"communication", "computer"}:
            if top_domains & domains and not top_domains & {"power", "records"}:
                labels.append(f"power_vs_{label}")
        elif top_domains & domains:
            labels.append(f"power_vs_{label}")
    return list(dict.fromkeys(labels))


def select_local_evidence(
    question: str, id_to_slug: dict[str, str]
) -> tuple[list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate]]:
    guide_plan = SourcePlanItem(source_type="guide", query=question, limit=8)
    wiki_plan = SourcePlanItem(source_type="wiki", query=question, limit=8)
    guide_candidates = fetch_guide_candidates(guide_plan, user_message=question)
    wiki_candidates = fetch_wiki_candidates(wiki_plan, user_message=question)
    selected_guides = guide_candidates[:3]
    related_wikis = fetch_related_wiki_candidates(selected_guides)
    selected_wikis = dedupe_candidates([*related_wikis, *wiki_candidates], id_to_slug)[:6]
    return selected_guides, selected_wikis, guide_candidates, wiki_candidates


def classify_root_causes(
    *,
    case: dict[str, Any],
    selected_guides: list[EvidenceCandidate],
    selected_wikis: list[EvidenceCandidate],
    guide_candidates: list[EvidenceCandidate],
    wiki_candidates: list[EvidenceCandidate],
    guide_related_wiki: set[str],
    id_to_slug: dict[str, str],
    profiles: list[str],
    failures: list[str],
) -> list[str]:
    causes: list[str] = []
    expected_guides = set(case.get("expected_guides") or [])
    allowed_guides = set(case.get("allowed_secondary_guides") or [])
    acceptable_guides = expected_guides | allowed_guides
    expected_wikis = set(case.get("expected_wiki") or [])
    selected_guide_ids = {guide.id for guide in selected_guides}
    selected_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in selected_wikis}
    candidate_guide_ids = {guide.id for guide in guide_candidates}
    candidate_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates}

    if "P0 电气安全误触发" in failures:
        causes.append("P0 电气安全误触发")
    if "严重跨域抢位" in failures:
        causes.append("跨域竞争")

    if acceptable_guides and not (acceptable_guides & selected_guide_ids):
        if acceptable_guides & candidate_guide_ids:
            causes.append("selector 问题")
        elif profiles:
            causes.append("ranking 问题")
        else:
            causes.append("profile 缺口")

    if expected_wikis and not (expected_wikis & selected_wiki_slugs):
        if expected_wikis & guide_related_wiki:
            causes.append("selector 问题")
        elif expected_wikis & candidate_wiki_slugs:
            causes.append("ranking 问题")
        elif acceptable_guides and acceptable_guides & selected_guide_ids:
            causes.append("Guide 设计问题")
        else:
            causes.append("数据缺口")

    if case.get("observation") and failures:
        causes.append("合理 partial")
    if any("缺少" in failure for failure in failures) and not causes:
        causes.append("Guide 设计问题")
    return [cause for cause in ROOT_CAUSE_CLASSES if cause in set(causes)]


def run_case(case: dict[str, Any], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> dict[str, Any]:
    question = str(case.get("query") or case.get("question") or "")
    profiles = [str(item.get("name") or "") for item in _matching_query_profiles(question)]
    selected_guides, selected_wikis, guide_candidates, wiki_candidates = select_local_evidence(question, id_to_slug)

    selected_guide_ids = {guide.id for guide in selected_guides}
    selected_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in selected_wikis}
    expected_guides = set(case.get("expected_guides") or [])
    allowed_guides = set(case.get("allowed_secondary_guides") or [])
    acceptable_guides = expected_guides | allowed_guides
    expected_wikis = set(case.get("expected_wiki") or [])
    guide_related_wiki = {
        str(slug)
        for guide in selected_guides
        for slug in as_list((guide.raw or {}).get("related_wiki"))
    }

    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    guide_hit = not acceptable_guides or bool(acceptable_guides & selected_guide_ids)
    expected_guide_hit = not expected_guides or bool(expected_guides & selected_guide_ids)
    wiki_hit = not expected_wikis or bool(expected_wikis & selected_wiki_slugs)
    precise_combo = not expected_wikis or bool(expected_wikis & guide_related_wiki)
    has_stop = contains_any(text, STOP_TERMS)
    has_fallback = contains_any(text, FALLBACK_TERMS)
    has_record = contains_any(text, RECORD_TERMS)
    dangerous = dangerous_suggestions(text)
    cross_labels = cross_domain_labels(selected_guides)
    p0_misfire = bool(selected_guide_ids & P0_ENERGY_SAFETY_GUIDES)

    failures: list[str] = []
    if not guide_hit:
        failures.append("未命中预期或允许 Guide")
    if not wiki_hit:
        failures.append("未命中预期 Wiki")
    if case.get("should_have_safety_boundary", True) and not has_stop:
        failures.append("缺少 safety boundary")
    if case.get("should_have_fallback", True) and not has_fallback:
        failures.append("缺少 fallback")
    if case.get("should_have_record_check", True) and not has_record:
        failures.append("缺少 record/check")
    if dangerous:
        failures.append("存在危险建议")
    if cross_labels:
        failures.append("严重跨域抢位")
    if p0_misfire:
        failures.append("P0 电气安全误触发")

    if not failures:
        verdict = "pass"
    elif dangerous or p0_misfire or cross_labels or not has_stop or not has_fallback or not has_record:
        verdict = "fail"
    else:
        verdict = "partial"
    if case.get("observation") and verdict == "fail" and not (dangerous or p0_misfire or cross_labels):
        verdict = "partial"

    causes = classify_root_causes(
        case=case,
        selected_guides=selected_guides,
        selected_wikis=selected_wikis,
        guide_candidates=guide_candidates,
        wiki_candidates=wiki_candidates,
        guide_related_wiki=guide_related_wiki,
        id_to_slug=id_to_slug,
        profiles=profiles,
        failures=failures,
    )

    return {
        "id": case["id"],
        "query": question,
        "observation": bool(case.get("observation")),
        "focus": str(case.get("focus") or ""),
        "expected_guides": sorted(expected_guides),
        "allowed_secondary_guides": sorted(allowed_guides),
        "expected_wiki": sorted(expected_wikis),
        "profiles": profiles,
        "domains": selected_domains(selected_guides),
        "guides_selected": [
            {
                "id": guide.id,
                "title": guide.title,
                "risk_level": str((guide.raw or {}).get("risk_level") or ""),
                "domains": as_list((guide.raw or {}).get("domains")),
            }
            for guide in selected_guides
        ],
        "wikis_selected": [
            {
                "slug": wiki_slug(wiki, id_to_slug),
                "title": wiki.title,
                "risk_level": str((wiki.raw or {}).get("risk_level") or ""),
                "source_reason": str((wiki.raw or {}).get("source_reason") or ""),
            }
            for wiki in selected_wikis
        ],
        "guide_candidates": [{"id": guide.id, "title": guide.title} for guide in guide_candidates[:8]],
        "wiki_candidates": [{"slug": wiki_slug(wiki, id_to_slug), "title": wiki.title} for wiki in wiki_candidates[:8]],
        "guide_related_wiki": sorted(guide_related_wiki),
        "excluded_sources": [],
        "guide_hit": guide_hit,
        "expected_guide_hit": expected_guide_hit,
        "wiki_hit": wiki_hit,
        "guide_wiki_precise": precise_combo,
        "has_safety_boundary": has_stop,
        "has_fallback": has_fallback,
        "has_record_or_check": has_record,
        "dangerous_suggestions": dangerous,
        "cross_domain": cross_labels,
        "p0_energy_safety_misfire": p0_misfire,
        "verdict": verdict,
        "failure_reasons": failures,
        "root_cause_classification": causes,
    }


def pct(numerator: int, denominator: int) -> str:
    if denominator <= 0:
        return "n/a"
    return f"{(numerator / denominator) * 100:.1f}%"


def comma(values: list[str]) -> str:
    return "、".join(values) if values else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lines: list[str] = [
        "# Batch5-F Energy Management Retrieval Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. 测试范围",
        "",
        "本阶段只测试 Batch5-E 新增能源管理 Guide/Wiki 是否进入本地 Retrieval evidence。脚本默认不调用 LLM，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 PocketBase schema。",
        "",
        "覆盖：每日能源预算、关键设备最低电量线、充电队列和排班、太阳能弱输出排程、低电量断负载、夜间照明、能源记录和交接。",
        "",
        "## 2. 汇总",
        "",
        f"- 用例总数：{summary['total']}",
        f"- strict / observation：{summary['strict_total']} / {summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- Guide 命中率（严格用例，含 allowed secondary）：{summary['guide_hit_rate']}",
        f"- 主 Guide 命中率（严格用例，仅 expected）：{summary['expected_guide_hit_rate']}",
        f"- Wiki 命中率（严格用例）：{summary['wiki_hit_rate']}",
        f"- Guide-Wiki 精准组合率（严格用例）：{summary['guide_wiki_precise_rate']}",
        f"- safety boundary 覆盖：{summary['safety_boundary_rate']}",
        f"- fallback 覆盖：{summary['fallback_rate']}",
        f"- record/check 覆盖：{summary['record_rate']}",
        f"- 危险建议数量：{summary['dangerous_suggestion_count']}",
        f"- P0 电气安全误触发：{summary['p0_misfire_count']}",
        f"- 跨域竞争数量：{summary['cross_domain_count']}",
        "",
        "## 3. Case 明细",
        "",
        "| case | type | verdict | Guide | Wiki | profiles | safety | fallback | record | root cause |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for case in results["cases"]:
        guides = comma([f"{g['id']} {g['title']}" for g in case["guides_selected"]])
        wikis = comma([f"{w['slug']} {w['title']}" for w in case["wikis_selected"]])
        roots = comma(case["root_cause_classification"])
        lines.append(
            f"| {case['id']} | {'observation' if case['observation'] else 'strict'} | {case['verdict']} | "
            f"{guides} | {wikis} | {comma(case['profiles'])} | "
            f"{'是' if case['has_safety_boundary'] else '否'} | "
            f"{'是' if case['has_fallback'] else '否'} | "
            f"{'是' if case['has_record_or_check'] else '否'} | {roots} |"
        )

    lines.extend(["", "## 4. 逐条复盘", ""])
    for case in results["cases"]:
        lines.extend(
            [
                f"### {case['id']}",
                "",
                f"- query：{case['query']}",
                f"- 类型：{'observation' if case['observation'] else 'strict'}",
                f"- focus：{case['focus']}",
                f"- verdict：{case['verdict']}",
                f"- expected Guide：{comma(case['expected_guides'])}",
                f"- allowed secondary：{comma(case['allowed_secondary_guides'])}",
                f"- selected Guide：{comma([g['id'] for g in case['guides_selected']])}",
                f"- selected Wiki：{comma([w['slug'] for w in case['wikis_selected']])}",
                f"- profiles：{comma(case['profiles'])}",
                f"- domains：{comma(case['domains'])}",
                f"- safety / fallback / record：{'是' if case['has_safety_boundary'] else '否'} / {'是' if case['has_fallback'] else '否'} / {'是' if case['has_record_or_check'] else '否'}",
                f"- dangerous suggestions：{comma(case['dangerous_suggestions'])}",
                f"- cross domain：{comma(case['cross_domain'])}",
                f"- P0 电气安全误触发：{'是' if case['p0_energy_safety_misfire'] else '否'}",
                f"- root cause：{comma(case['root_cause_classification'])}",
                f"- failure reasons：{comma(case['failure_reasons'])}",
                "",
            ]
        )

    lines.extend(["## 5. Root Cause 分类", ""])
    if summary["root_cause_counts"]:
        for key, value in summary["root_cause_counts"].items():
            lines.append(f"- {key}：{value}")
    else:
        lines.append("- 无 partial/fail root cause。")

    lines.extend(["", "## 6. 跨域竞争统计", ""])
    if summary["cross_domain_counts"]:
        for key, value in summary["cross_domain_counts"].items():
            lines.append(f"- {key}：{value}")
    else:
        lines.append("- 未发现通信、电脑、夜间安全、维修或 P0 电气安全抢主位。")

    lines.extend(
        [
            "",
            "## 7. 是否建议进入 Batch5-G Review",
            "",
        ]
    )
    if summary["partial"] or summary["fail"]:
        lines.append("建议进入 Batch5-G Root Cause Review：本批只记录检索 evidence 表现，不直接修复。Review 应判断问题属于数据缺口、profile 缺口、selector 问题、ranking 问题、Guide 设计问题或合理 partial。")
    else:
        lines.append("暂不需要 Batch5-G 修复；可在更高覆盖的能源管理场景中继续观察。")
    lines.extend(
        [
            "",
            "## 8. 边界声明",
            "",
            "- 本批没有修改 Wiki 正文、Guide 正文、Guide-Wiki 关系、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 PocketBase schema。",
            "- 本批没有修复 partial/fail，也没有宣告 Energy Management Retrieval stable。",
        ]
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Batch5-F energy management retrieval field cases.")
    parser.add_argument("--no-answer", action="store_true", help="Accepted for parity; this script never calls an LLM.")
    parser.parse_args()

    cases = read_json(CASES_PATH)
    wiki_rows = load_wiki_rows()
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    case_results = []
    for index, case in enumerate(cases, 1):
        question = case.get("query") or case.get("question")
        print(f"[{index}/{len(cases)}] {case['id']}: {question}")
        result = run_case(case, wiki_rows, id_to_slug)
        print(f"  -> {result['verdict']}: {', '.join(result['failure_reasons']) or 'ok'}")
        case_results.append(result)

    strict_cases = [case for case in case_results if not case["observation"]]
    root_counts: dict[str, int] = {}
    cross_counts: dict[str, int] = {}
    for case in case_results:
        for cause in case["root_cause_classification"]:
            root_counts[cause] = root_counts.get(cause, 0) + 1
        for label in case["cross_domain"]:
            cross_counts[label] = cross_counts.get(label, 0) + 1

    summary = {
        "total": len(case_results),
        "strict_total": len(strict_cases),
        "observation_total": len(case_results) - len(strict_cases),
        "pass": sum(case["verdict"] == "pass" for case in case_results),
        "partial": sum(case["verdict"] == "partial" for case in case_results),
        "fail": sum(case["verdict"] == "fail" for case in case_results),
        "guide_hit_rate": pct(sum(case["guide_hit"] for case in strict_cases), len(strict_cases)),
        "expected_guide_hit_rate": pct(sum(case["expected_guide_hit"] for case in strict_cases), len(strict_cases)),
        "wiki_hit_rate": pct(sum(case["wiki_hit"] for case in strict_cases), len(strict_cases)),
        "guide_wiki_precise_rate": pct(sum(case["guide_wiki_precise"] for case in strict_cases), len(strict_cases)),
        "safety_boundary_rate": pct(sum(case["has_safety_boundary"] for case in case_results), len(case_results)),
        "fallback_rate": pct(sum(case["has_fallback"] for case in case_results), len(case_results)),
        "record_rate": pct(sum(case["has_record_or_check"] for case in case_results), len(case_results)),
        "dangerous_suggestion_count": sum(bool(case["dangerous_suggestions"]) for case in case_results),
        "p0_misfire_count": sum(case["p0_energy_safety_misfire"] for case in case_results),
        "cross_domain_count": sum(bool(case["cross_domain"]) for case in case_results),
        "root_cause_counts": root_counts,
        "cross_domain_counts": cross_counts,
    }
    results = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "summary": summary,
        "cases": case_results,
    }
    RESULTS_PATH.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    REPORT_PATH.write_text(build_report(results), encoding="utf-8")
    print(f"results: {RESULTS_PATH}")
    print(f"report: {REPORT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
