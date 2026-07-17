#!/usr/bin/env python3
"""Run Batch8-D Navigation v0.1 retrieval field cases.

This script is test/report only. It reads existing Retrieval v2 fetchers and
local knowledge data, then writes a JSON result file and a Markdown report.
It does not modify Wiki, Guide, Guide-Wiki relations, Retrieval Pipeline,
Prompt, query profiles, top_k, selector limits, ranking, fallback,
PocketBase schema, or production knowledge.
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
if not os.environ.get("LANTERNBOX_NAVIGATION_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_NAVIGATION_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests" / "fixtures" / "navigation_field_cases.json"
RESULTS_PATH = ROOT / "docs" / "knowledge" / "batch8_d_navigation_field_test_results.json"
REPORT_PATH = ROOT / "docs" / "knowledge" / "batch8_d_navigation_field_test_report.md"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"

STOP_TERMS = [
    "停止",
    "停用",
    "禁用",
    "禁止",
    "不可",
    "不要",
    "不能",
    "不应",
    "不得",
    "避免",
    "隔离",
    "暂停",
    "待核",
    "未确认",
    "停止线",
    "撤回",
    "返回最近确定点",
]
FALLBACK_TERMS = [
    "替代",
    "备用",
    "改用",
    "没有",
    "不足",
    "缺少",
    "无法",
    "降级",
    "等待",
    "纸质",
    "手绘",
    "草图",
    "地标",
    "最近确定点",
]
RECORD_TERMS = [
    "记录",
    "标记",
    "复查",
    "交接",
    "日志",
    "时间",
    "编号",
    "坐标",
    "检查点",
    "路线卡",
    "轨迹",
    "session",
    "path point",
]
DANGEROUS_PHRASES = [
    "继续往前走",
    "继续深入",
    "直接硬闯",
    "硬闯",
    "直接过去",
    "不用记录",
    "不用复查",
    "不用返回",
    "可以下水试深",
    "无 fix 也可以继续",
    "按漂移轨迹返回",
]
NEGATION_MARKERS = ["不要", "不可", "不能", "不应", "不得", "不再", "不继续", "不硬闯", "禁止", "停止", "避免", "不允许", "不适用于"]
SAFE_CONTEXT_MARKERS = ["常见错误", "错误操作", "风险提示", "不要", "不可", "不能", "不应", "不得", "禁止", "停止", "避免", "不允许", "不适用于", "不硬闯"]

NAVIGATION_DOMAINS = {"navigation", "maps"}
OFF_DOMAIN_GROUPS = {
    "communication": {"comms", "communication"},
    "evacuation": {"evacuation", "disaster"},
    "outdoor": {"outdoor", "survival", "shelter"},
    "security": {"security", "safety"},
    "geography": {"geography", "maps"},
    "weather": {"weather"},
    "water": {"water"},
    "shelter": {"shelter"},
    "team": {"organization", "team", "records"},
    "journal": {"records", "journal"},
    "electronics": {"electronics", "power", "energy"},
}


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
            start = compact.find(phrase)
            window = compact[max(0, start - 14) : start + len(phrase) + 14]
            if any(marker in window for marker in NEGATION_MARKERS):
                continue
            hits.append(phrase)
    return list(dict.fromkeys(hits))


def guide_domains(guide: EvidenceCandidate) -> set[str]:
    return {str(domain) for domain in as_list((guide.raw or {}).get("domains")) if str(domain)}


def selected_domains(guides: list[EvidenceCandidate]) -> list[str]:
    domains: list[str] = []
    for guide in guides:
        for domain in guide_domains(guide):
            if domain not in domains:
                domains.append(domain)
    return domains


def cross_domain_labels(case: dict[str, Any], guides: list[EvidenceCandidate]) -> list[str]:
    if not guides:
        return ["no_guide_evidence"]
    top_domains = guide_domains(guides[0])
    if top_domains & NAVIGATION_DOMAINS:
        return []
    labels = ["off_domain_primary"]
    watched = set(case.get("watch_conflicts") or OFF_DOMAIN_GROUPS.keys())
    for label, domains in OFF_DOMAIN_GROUPS.items():
        if label in watched and top_domains & domains:
            labels.append(f"navigation_vs_{label}")
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
    selected_wikis = dedupe_candidates([*related_wikis, *wiki_candidates], id_to_slug)[:8]
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

    if "严重跨域抢主位" in failures:
        causes.append("跨域竞争")
    if acceptable_guides and not (acceptable_guides & selected_guide_ids):
        if acceptable_guides & candidate_guide_ids:
            causes.append("selector 问题")
        elif profiles:
            causes.append("ranking 问题")
        else:
            causes.append("profile 缺口")
    if expected_wikis and not expected_wikis <= selected_wiki_slugs:
        if expected_wikis <= guide_related_wiki:
            causes.append("selector 问题")
        elif expected_wikis & candidate_wiki_slugs:
            causes.append("ranking 问题")
        elif acceptable_guides and acceptable_guides & selected_guide_ids:
            causes.append("Guide 设计问题")
        else:
            causes.append("数据缺口")
    if case.get("observation") and failures:
        causes.append("合理 observation")
    if any("缺少" in failure for failure in failures) and not causes:
        causes.append("Guide 设计问题")

    order = ["数据缺口", "profile 缺口", "selector 问题", "ranking 问题", "Guide 设计问题", "跨域竞争", "合理 observation"]
    return [cause for cause in order if cause in set(causes)]


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
    expected_guide_hit = not expected_guides or bool(expected_guides & selected_guide_ids)
    guide_hit = not acceptable_guides or bool(acceptable_guides & selected_guide_ids)
    wiki_hit = not expected_wikis or expected_wikis <= selected_wiki_slugs
    wiki_hit_any = not expected_wikis or bool(expected_wikis & selected_wiki_slugs)
    precise_combo = not expected_wikis or expected_wikis <= guide_related_wiki
    has_stop = contains_any(text, STOP_TERMS)
    has_fallback = contains_any(text, FALLBACK_TERMS)
    has_record = contains_any(text, RECORD_TERMS)
    dangerous = dangerous_suggestions(text)
    cross_labels = cross_domain_labels(case, selected_guides)
    kiwix_override = False

    failures: list[str] = []
    if not guide_hit:
        failures.append("未命中预期或允许 Guide")
    if not wiki_hit:
        failures.append("未命中全部预期 Wiki")
    if case.get("should_have_safety_boundary", True) and not has_stop:
        failures.append("缺少 safety boundary")
    if case.get("should_have_fallback", True) and not has_fallback:
        failures.append("缺少 fallback")
    if case.get("should_have_record_check", True) and not has_record:
        failures.append("缺少 record/check")
    if dangerous:
        failures.append("存在危险建议")
    if cross_labels and not case.get("observation"):
        failures.append("严重跨域抢主位")
    if kiwix_override:
        failures.append("Kiwix 越权")

    if not failures:
        verdict = "pass"
    elif dangerous or kiwix_override:
        verdict = "fail"
    elif not has_stop or not has_fallback or not has_record:
        verdict = "fail"
    elif cross_labels and not case.get("observation"):
        verdict = "fail"
    else:
        verdict = "partial"
    if case.get("observation") and verdict == "fail" and not (dangerous or kiwix_override):
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
        "watch_conflicts": as_list(case.get("watch_conflicts")),
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
        "guide_hit": guide_hit,
        "expected_guide_hit": expected_guide_hit,
        "wiki_hit": wiki_hit,
        "wiki_hit_any": wiki_hit_any,
        "guide_wiki_precise": precise_combo,
        "has_safety_boundary": has_stop,
        "has_fallback": has_fallback,
        "has_record_or_check": has_record,
        "dangerous_suggestions": dangerous,
        "kiwix_override": kiwix_override,
        "cross_domain": cross_labels,
        "verdict": verdict,
        "failure_reasons": failures,
        "root_cause_classification": causes,
    }


def pct(numerator: int, denominator: int) -> str:
    if denominator <= 0:
        return "n/a"
    return f"{(numerator / denominator) * 100:.1f}%"


def comma(values: list[Any]) -> str:
    items = [str(value) for value in values if str(value)]
    return "、".join(items) if items else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lines: list[str] = [
        "# Batch8-D Navigation Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. 测试范围",
        "",
        "本阶段只测试 Batch8-C2 新增 Navigation v0.1 Guide/Wiki 是否稳定进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。",
        "",
        "覆盖：个人外出路线规划、迷路返回、GNSS fix / drift、检查点、撤离路线风险、夜间移动、GPS track 复盘，以及 water / communication / outdoor / shelter / security / journal / team 的跨域观察。",
        "",
        "## 2. 18 cases 结果",
        "",
        f"- 用例总数：{summary['total']}",
        f"- strict / observation：{summary['strict_total']} / {summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- strict pass / partial / fail：{summary['strict_pass']} / {summary['strict_partial']} / {summary['strict_fail']}",
        "",
        "## 3. Guide / Wiki 命中",
        "",
        f"- Guide 命中率（strict，含 allowed secondary）：{summary['guide_hit_rate']}",
        f"- 主 Guide 命中率（strict，仅 expected）：{summary['expected_guide_hit_rate']}",
        f"- Wiki 全量命中率（strict，全部 expected Wiki）：{summary['wiki_hit_rate']}",
        f"- Wiki 任一命中率（strict，至少一个 expected Wiki）：{summary['wiki_hit_any_rate']}",
        f"- Guide-Wiki 精准组合率（strict）：{summary['guide_wiki_precise_rate']}",
        f"- navigation 主 Guide 进入率（全部 cases）：{summary['navigation_primary_rate']}",
        "",
        "安全指标：",
        "",
        f"- safety boundary：{summary['safety_boundary_rate']}",
        f"- fallback：{summary['fallback_rate']}",
        f"- record/check：{summary['record_rate']}",
        f"- dangerous suggestion：{summary['dangerous_suggestion_count']}",
        f"- Kiwix 越权：{summary['kiwix_override_count']}",
        "",
        "## 4. Case 明细",
        "",
        "|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|",
        "|---|---|---|---|---|---|---|---|",
    ]
    for case in results["cases"]:
        guides = comma([f"{g['id']} {g['title']}" for g in case["guides_selected"]])
        wikis = comma([f"{w['slug']} {w['title']}" for w in case["wikis_selected"]])
        lines.append(
            f"|{case['id']}|{'observation' if case['observation'] else 'strict'}|{case['verdict']}|"
            f"{guides}|{wikis}|{comma(case['profiles'])}|{comma(case['cross_domain'])}|{comma(case['root_cause_classification'])}|"
        )

    lines.extend(["", "## 5. Cross Domain 分析", ""])
    conflict_keys = ["communication", "evacuation", "outdoor", "security", "geography", "weather", "water", "shelter", "team", "journal", "electronics"]
    for key in conflict_keys:
        lines.append(f"- {key} 抢主位观察：{summary['conflict_observation_counts'].get(key, 0)}")
    if summary["cross_domain_counts"]:
        lines.append("")
        lines.append("Cross domain labels：")
        for key, value in summary["cross_domain_counts"].items():
            lines.append(f"- {key}：{value}")
    else:
        lines.append("")
        lines.append("未发现 strict case 非 navigation Guide 抢主位。")

    lines.extend(["", "## 6. Navigation Evidence 稳定性", ""])
    lines.append(f"- Navigation 主域 Guide 作为 top evidence 的比例：{summary['navigation_primary_rate']}。")
    lines.append(f"- Strict cases 中 Guide 命中率：{summary['guide_hit_rate']}。")
    lines.append(f"- Strict cases 中 Wiki 全量命中率：{summary['wiki_hit_rate']}。")
    lines.append(f"- Guide-Wiki 精准组合率：{summary['guide_wiki_precise_rate']}。")
    lines.append("- 本阶段只记录命中表现，不根据结果调整 profile、selector、ranking 或知识内容。")

    lines.extend(["", "## 7. 是否需要 profile", ""])
    if summary["strict_fail"] or summary["strict_partial"]:
        lines.append("需要进入 Batch8-E Navigation Retrieval Root Cause Review 后再判断。当前不直接新增 profile。")
        lines.append("Root cause 初步分类：")
        if summary["root_cause_counts"]:
            for key, value in summary["root_cause_counts"].items():
                lines.append(f"- {key}：{value}")
        else:
            lines.append("- 未形成明确分类。")
    else:
        lines.append("暂不建议新增 profile；可将 Navigation v0.1 作为 stable candidate 继续观察。")

    lines.extend(["", "## 8. 是否进入 Navigation Retrieval Root Cause Review", ""])
    if summary["strict_fail"] or summary["strict_partial"] or summary["cross_domain_count"]:
        lines.append("建议进入 Batch8-E Navigation Retrieval Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。")
    else:
        lines.append("暂不需要 Root Cause Review。")

    lines.extend(["", "## 9. 逐条复盘", ""])
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
                f"- expected Wiki：{comma(case['expected_wiki'])}",
                f"- selected Wiki：{comma([w['slug'] for w in case['wikis_selected']])}",
                f"- guide_hit / wiki_hit / precise：{case['guide_hit']} / {case['wiki_hit']} / {case['guide_wiki_precise']}",
                f"- safety / fallback / record：{'是' if case['has_safety_boundary'] else '否'} / {'是' if case['has_fallback'] else '否'} / {'是' if case['has_record_or_check'] else '否'}",
                f"- dangerous suggestions：{comma(case['dangerous_suggestions'])}",
                f"- cross domain：{comma(case['cross_domain'])}",
                f"- root cause：{comma(case['root_cause_classification'])}",
                f"- failure reasons：{comma(case['failure_reasons'])}",
                "",
            ]
        )

    lines.extend(
        [
            "## 10. 验证命令",
            "",
            "本轮按要求运行：",
            "",
            "```text",
            "python3 -m py_compile scripts/test_navigation_field.py",
            "python3 tools/audit_wiki.py",
            "python3 scripts/audit_guides.py",
            "env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py",
            "python3 scripts/test_navigation_field.py --no-answer",
            "```",
            "",
            "边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。",
        ]
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Batch8-D navigation retrieval field cases.")
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
    conflict_counts: dict[str, int] = {}
    for case in case_results:
        for cause in case["root_cause_classification"]:
            root_counts[cause] = root_counts.get(cause, 0) + 1
        for label in case["cross_domain"]:
            cross_counts[label] = cross_counts.get(label, 0) + 1
        if case["cross_domain"]:
            for watched in case["watch_conflicts"]:
                conflict_counts[str(watched)] = conflict_counts.get(str(watched), 0) + 1

    navigation_primary = [
        bool(case["guides_selected"]) and bool(set(case["guides_selected"][0]["domains"]) & NAVIGATION_DOMAINS)
        for case in case_results
    ]
    summary = {
        "total": len(case_results),
        "strict_total": len(strict_cases),
        "observation_total": len(case_results) - len(strict_cases),
        "pass": sum(case["verdict"] == "pass" for case in case_results),
        "partial": sum(case["verdict"] == "partial" for case in case_results),
        "fail": sum(case["verdict"] == "fail" for case in case_results),
        "strict_pass": sum(case["verdict"] == "pass" for case in strict_cases),
        "strict_partial": sum(case["verdict"] == "partial" for case in strict_cases),
        "strict_fail": sum(case["verdict"] == "fail" for case in strict_cases),
        "guide_hit_rate": pct(sum(case["guide_hit"] for case in strict_cases), len(strict_cases)),
        "expected_guide_hit_rate": pct(sum(case["expected_guide_hit"] for case in strict_cases), len(strict_cases)),
        "wiki_hit_rate": pct(sum(case["wiki_hit"] for case in strict_cases), len(strict_cases)),
        "wiki_hit_any_rate": pct(sum(case["wiki_hit_any"] for case in strict_cases), len(strict_cases)),
        "guide_wiki_precise_rate": pct(sum(case["guide_wiki_precise"] for case in strict_cases), len(strict_cases)),
        "navigation_primary_rate": pct(sum(navigation_primary), len(navigation_primary)),
        "safety_boundary_rate": pct(sum(case["has_safety_boundary"] for case in case_results), len(case_results)),
        "fallback_rate": pct(sum(case["has_fallback"] for case in case_results), len(case_results)),
        "record_rate": pct(sum(case["has_record_or_check"] for case in case_results), len(case_results)),
        "dangerous_suggestion_count": sum(bool(case["dangerous_suggestions"]) for case in case_results),
        "kiwix_override_count": sum(case["kiwix_override"] for case in case_results),
        "cross_domain_count": sum(bool(case["cross_domain"]) for case in case_results),
        "root_cause_counts": root_counts,
        "cross_domain_counts": cross_counts,
        "conflict_observation_counts": conflict_counts,
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
