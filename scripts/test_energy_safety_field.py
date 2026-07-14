#!/usr/bin/env python3
"""Run Batch5-B Energy Safety retrieval field cases.

This script is test/report only. It does not call an LLM and does not modify
Wiki, Guide, profiles, prompt, ranking, or retrieval pipeline behavior.
"""

from __future__ import annotations

import json
import os
import sqlite3
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"
if not os.environ.get("LANTERNBOX_ENERGY_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_ENERGY_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests" / "fixtures" / "energy_safety_field_cases.json"
RESULTS_PATH = ROOT / "docs" / "knowledge" / "batch5_b_energy_safety_field_test_results.json"
REPORT_PATH = ROOT / "docs" / "knowledge" / "batch5_b_energy_safety_field_test_report.md"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"

STOP_TERMS = ["停止", "停用", "禁用", "禁止", "不可", "不要", "不能", "不再", "断开", "隔离", "移出"]
FALLBACK_TERMS = ["替代", "改用", "缺少", "没有", "暂停", "减少", "降级", "分批", "纸质"]
RECORD_TERMS = ["记录", "标记", "编号", "复查", "责任人", "标签", "时间", "清单"]
DANGEROUS_PHRASES = ["可以试一下", "通电看看", "测试看看", "继续充", "继续用", "观察一下再说"]
NEGATION_MARKERS = ["不要", "不可", "不能", "不应", "不得", "不再", "不继续", "禁止", "停止", "停用", "避免", "不是"]
SAFE_CONTEXT_MARKERS = ["不适用", "常见错误", "错误操作", "风险提示", "不要", "不可", "不能", "不应", "不得", "不再", "不继续", "禁止", "停止", "停用", "避免"]
LOCAL_POWER_DOMAINS = {"power", "security", "records"}
RELATED_ACCEPTABLE_DOMAINS = {"repair", "tools"}
CROSS_DOMAIN_GROUPS = {
    "repair": {"repair", "tools"},
    "communication": {"comms", "communication"},
    "computer": {"computer", "core", "sync", "terminal"},
    "food": {"food"},
    "medical": {"medical"},
}
ROOT_CAUSE_CLASSES = [
    "数据缺口",
    "profile 缺口",
    "selector 问题",
    "ranking 问题",
    "Guide 设计问题",
    "Wiki evidence priority",
    "跨域竞争",
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
        chunks.extend([
            str(guide.title or ""),
            str(guide.summary or ""),
            " ".join(str(item) for item in as_list(raw.get("steps"))),
            " ".join(str(item) for item in as_list(raw.get("check"))),
            " ".join(str(item) for item in as_list(raw.get("fallback"))),
            " ".join(str(item) for item in as_list(raw.get("stop_or_escalate"))),
            str(raw.get("notes") or ""),
        ])
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
        if not compact:
            continue
        if any(marker in compact for marker in SAFE_CONTEXT_MARKERS):
            continue
        for phrase in DANGEROUS_PHRASES:
            if phrase not in compact:
                continue
            prefix = compact[: compact.find(phrase)]
            if any(marker in prefix[-12:] for marker in NEGATION_MARKERS):
                continue
            if any(marker in compact[max(0, compact.find(phrase) - 8): compact.find(phrase) + len(phrase) + 8] for marker in NEGATION_MARKERS):
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
    labels: list[str] = []
    top_domains = {str(domain) for domain in as_list((guides[0].raw or {}).get("domains"))}
    if not top_domains:
        return []
    if not top_domains & (LOCAL_POWER_DOMAINS | RELATED_ACCEPTABLE_DOMAINS):
        labels.append("non_power_primary")
    for label, domains in CROSS_DOMAIN_GROUPS.items():
        if label in {"repair"}:
            if top_domains & domains and not top_domains & LOCAL_POWER_DOMAINS:
                labels.append("power_vs_repair")
        elif top_domains & domains:
            labels.append(f"power_vs_{label}")
    return list(dict.fromkeys(labels))


def select_local_evidence(question: str, id_to_slug: dict[str, str]) -> tuple[list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate]]:
    guide_plan = SourcePlanItem(source_type="guide", query=question, limit=8)
    wiki_plan = SourcePlanItem(source_type="wiki", query=question, limit=8)
    guide_candidates = fetch_guide_candidates(guide_plan, user_message=question)
    wiki_candidates = fetch_wiki_candidates(wiki_plan, user_message=question)
    selected_guides = guide_candidates[:3]
    related_wikis = fetch_related_wiki_candidates(selected_guides)
    selected_wikis = dedupe_candidates([*related_wikis, *wiki_candidates], id_to_slug)[:6]
    return selected_guides, selected_wikis, guide_candidates, wiki_candidates


def classify_root_causes(
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
    expected_wikis = set(case.get("expected_wiki") or [])
    selected_guide_ids = {guide.id for guide in selected_guides}
    selected_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in selected_wikis}
    candidate_guide_ids = {guide.id for guide in guide_candidates}
    candidate_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates}

    if expected_guides and not (expected_guides & selected_guide_ids):
        if expected_guides & candidate_guide_ids:
            causes.append("selector 问题")
        elif profiles:
            causes.append("ranking 问题")
        else:
            causes.append("profile 缺口")

    if expected_wikis and not (expected_wikis & selected_wiki_slugs):
        if expected_wikis & guide_related_wiki:
            causes.append("selector 问题")
        elif expected_wikis & candidate_wiki_slugs:
            causes.append("Wiki evidence priority")
        elif expected_guides and expected_guides & selected_guide_ids:
            causes.append("Guide 设计问题")
        else:
            causes.append("数据缺口")

    if any("跨域" in failure for failure in failures):
        causes.append("跨域竞争")
    if any("危险建议" in failure for failure in failures) and not causes:
        causes.append("Guide 设计问题")
    return [cause for cause in ROOT_CAUSE_CLASSES if cause in set(causes)]


def run_case(case: dict[str, Any], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> dict[str, Any]:
    question = case["question"]
    profiles = [str(item.get("name") or "") for item in _matching_query_profiles(question)]
    selected_guides, selected_wikis, guide_candidates, wiki_candidates = select_local_evidence(question, id_to_slug)
    selected_guide_ids = {guide.id for guide in selected_guides}
    selected_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in selected_wikis}
    expected_guides = set(case.get("expected_guides") or [])
    expected_wikis = set(case.get("expected_wiki") or [])
    guide_related_wiki = {
        str(slug)
        for guide in selected_guides
        for slug in as_list((guide.raw or {}).get("related_wiki"))
    }

    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    guide_hit = not expected_guides or bool(expected_guides & selected_guide_ids)
    wiki_hit = not expected_wikis or bool(expected_wikis & selected_wiki_slugs)
    precise_combo = not expected_wikis or bool(expected_wikis & guide_related_wiki)
    has_stop = contains_any(text, STOP_TERMS)
    has_fallback = contains_any(text, FALLBACK_TERMS)
    has_record = contains_any(text, RECORD_TERMS)
    dangerous = dangerous_suggestions(text)
    cross_labels = cross_domain_labels(selected_guides)

    failures: list[str] = []
    if not guide_hit:
        failures.append("未命中预期 Guide")
    if not wiki_hit:
        failures.append("未命中预期 Wiki")
    if not has_stop:
        failures.append("缺少安全边界")
    if not has_fallback:
        failures.append("缺少 fallback")
    if not has_record:
        failures.append("缺少 record/check")
    if dangerous:
        failures.append("存在危险建议")
    if cross_labels:
        failures.append("跨域竞争")

    if not failures:
        verdict = "pass"
    elif dangerous or not has_stop or not has_fallback:
        verdict = "fail"
    else:
        verdict = "partial"

    causes = classify_root_causes(
        case,
        selected_guides,
        selected_wikis,
        guide_candidates,
        wiki_candidates,
        guide_related_wiki,
        id_to_slug,
        profiles,
        failures,
    )

    return {
        "id": case["id"],
        "question": question,
        "observation": bool(case.get("observation")),
        "observation_note": str(case.get("observation_note") or ""),
        "expected_guides": list(expected_guides),
        "expected_wiki": list(expected_wikis),
        "risk": case.get("risk", ""),
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
        "wiki_hit": wiki_hit,
        "guide_wiki_precise": precise_combo,
        "has_stop_or_escalate": has_stop,
        "has_fallback": has_fallback,
        "has_record_or_check": has_record,
        "dangerous_suggestions": dangerous,
        "cross_domain": cross_labels,
        "verdict": verdict,
        "failure_reasons": failures,
        "root_cause_classification": causes,
    }


def pct(numerator: int, denominator: int) -> str:
    if denominator <= 0:
        return "n/a"
    return f"{(numerator / denominator) * 100:.1f}%"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lines: list[str] = [
        "# Batch5-B Energy Safety Retrieval Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. 测试范围",
        "",
        "本阶段只测试 Batch5-A1 新增能源安全 Guide/Wiki 是否进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、关联、profile、Pipeline、Prompt、top_k、selector limit 或 fallback。",
        "",
        "覆盖：电池异常隔离、低压设备异常停用、未知电源安全判断，以及长期能源/太阳能两个观察型场景。",
        "",
        "## 2. 汇总",
        "",
        f"- 用例总数：{summary['total']}",
        f"- 严格用例：{summary['strict_total']}",
        f"- 观察用例：{summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- Guide 命中率：{summary['guide_hit_rate']}",
        f"- Wiki 命中率：{summary['wiki_hit_rate']}",
        f"- Guide-Wiki 精准组合：{summary['guide_wiki_precise_rate']}",
        f"- 安全边界覆盖：{summary['safety_boundary_rate']}",
        f"- fallback 覆盖：{summary['fallback_rate']}",
        f"- record/check 覆盖：{summary['record_rate']}",
        f"- 危险建议：{summary['dangerous_suggestion_count']}",
        f"- 跨域竞争：{summary['cross_domain_count']}",
        "",
        "## 3. 测试用例列表",
        "",
        "| case | verdict | Guide | Wiki | safety | fallback | record/check | root cause |",
        "| --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for case in results["cases"]:
        guides = "、".join(f"{g['id']} {g['title']}" for g in case["guides_selected"]) or "无"
        wikis = "、".join(f"{w['slug']} {w['title']}" for w in case["wikis_selected"]) or "无"
        roots = "、".join(case["root_cause_classification"]) or "无"
        lines.append(
            f"| {case['id']} | {case['verdict']} | {guides} | {wikis} | "
            f"{'是' if case['has_stop_or_escalate'] else '否'} | "
            f"{'是' if case['has_fallback'] else '否'} | "
            f"{'是' if case['has_record_or_check'] else '否'} | {roots} |"
        )

    lines.extend([
        "",
        "## 4. Case 明细",
        "",
    ])
    for case in results["cases"]:
        lines.extend([
            f"### {case['id']}",
            "",
            f"- query：{case['question']}",
            f"- observation：{'是' if case['observation'] else '否'}",
            f"- verdict：{case['verdict']}",
            f"- selected Guide：{', '.join(g['id'] for g in case['guides_selected']) or '无'}",
            f"- selected Wiki：{', '.join(w['slug'] for w in case['wikis_selected']) or '无'}",
            f"- safety boundary：{'是' if case['has_stop_or_escalate'] else '否'}",
            f"- fallback：{'是' if case['has_fallback'] else '否'}",
            f"- record/check：{'是' if case['has_record_or_check'] else '否'}",
            f"- dangerous suggestions：{', '.join(case['dangerous_suggestions']) or '无'}",
            f"- cross domain：{', '.join(case['cross_domain']) or '无'}",
            f"- root cause：{', '.join(case['root_cause_classification']) or '无'}",
            f"- failure reasons：{', '.join(case['failure_reasons']) or '无'}",
        ])
        if case["observation_note"]:
            lines.append(f"- note：{case['observation_note']}")
        lines.append("")

    root_counts = summary["root_cause_counts"]
    cross_counts = summary["cross_domain_counts"]
    lines.extend([
        "## 5. Root Cause 分类",
        "",
    ])
    if root_counts:
        for key, value in root_counts.items():
            lines.append(f"- {key}：{value}")
    else:
        lines.append("- 无 partial/fail root cause。")

    lines.extend([
        "",
        "## 6. 跨域竞争统计",
        "",
    ])
    if cross_counts:
        for key, value in cross_counts.items():
            lines.append(f"- {key}：{value}")
    else:
        lines.append("- 未发现 power vs repair / communication / computer 跨域竞争。")

    lines.extend([
        "",
        "## 7. 重点风险检查",
        "",
        f"- 危险建议（尝试继续使用 / 观察一下再说 / 可以测试看看）：{summary['dangerous_suggestion_count']}",
        f"- high/caution stop_or_escalate 覆盖：{summary['safety_boundary_rate']}",
        f"- high/caution fallback 覆盖：{summary['fallback_rate']}",
        "",
        "## 8. 结论",
        "",
        "本阶段只记录结果，不修复 partial/fail。若存在 partial/fail，应进入 Batch5-C Root Cause Review，再决定是否属于数据缺口、profile 缺口、selector 问题、ranking 问题或 Guide 设计问题。",
    ])
    return "\n".join(lines) + "\n"


def main() -> int:
    cases = read_json(CASES_PATH)
    wiki_rows = load_wiki_rows()
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    case_results = []
    for index, case in enumerate(cases, 1):
        print(f"[{index}/{len(cases)}] {case['id']}: {case['question']}")
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
        "wiki_hit_rate": pct(sum(case["wiki_hit"] for case in strict_cases), len(strict_cases)),
        "guide_wiki_precise_rate": pct(sum(case["guide_wiki_precise"] for case in strict_cases), len(strict_cases)),
        "safety_boundary_rate": pct(sum(case["has_stop_or_escalate"] for case in case_results), len(case_results)),
        "fallback_rate": pct(sum(case["has_fallback"] for case in case_results), len(case_results)),
        "record_rate": pct(sum(case["has_record_or_check"] for case in case_results), len(case_results)),
        "dangerous_suggestion_count": sum(bool(case["dangerous_suggestions"]) for case in case_results),
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
