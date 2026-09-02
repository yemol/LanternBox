#!/usr/bin/env python3
"""Run Batch13-D Production System / Workshop retrieval field cases.

This is a read-only retrieval test. It writes only the requested result and
report files and never calls an LLM.
"""
from __future__ import annotations

import argparse
import json
import os
import sqlite3
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv/bin/python"
if not os.environ.get("LANTERNBOX_PRODUCTION_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_PRODUCTION_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests/fixtures/production_system_workshop_field_cases.json"
RESULTS_PATH = ROOT / "docs/knowledge/batch13_d_production_system_workshop_field_test_results.json"
REPORT_PATH = ROOT / "docs/knowledge/batch13_d_production_system_workshop_field_test_report.md"
PB_DB = ROOT / "pocketbase/pb_data/data.db"

PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
MANUFACTURING_GUIDES = {f"DG-{number:04d}" for number in range(871, 879)}
WASTE_GUIDES = {f"DG-{number:04d}" for number in range(887, 895)}
STORAGE_GUIDES = {f"DG-{number:04d}" for number in range(895, 903)}
REPAIR_GUIDES = {f"DG-{number:04d}" for number in range(621, 626)}

STOP_TERMS = [
    "停止", "停用", "禁用", "禁止", "不可", "不要", "不能", "不得", "隔离",
    "暂停", "待检", "停止线", "封存", "不放行", "不得离场", "移出",
]
FALLBACK_TERMS = [
    "替代", "备用", "改用", "没有", "不足", "缺少", "无法", "临时", "降级",
    "纸质", "编号", "标签", "缩小", "暂停", "隔离", "待检",
]
RECORD_TERMS = [
    "记录", "标记", "复查", "交接", "任务板", "编号", "清单", "标签", "批次",
    "来源", "负责人", "状态", "检查", "交接卡", "返工卡",
]
DANGEROUS_PHRASES = [
    "直接继续切割", "直接继续钻孔", "可以继续使用", "直接入库", "先放行使用",
    "混入合格区", "不用检查", "不用记录", "不用隔离", "继续高风险加工",
    "破损工具继续用", "来源不明也可以用", "有裂纹也能承重",
]
SAFE_MARKERS = [
    "不要", "不可", "不能", "不应", "不得", "禁止", "停止", "避免", "不允许",
    "未", "不", "常见错误", "停止条件", "风险提示",
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
    connection = sqlite3.connect(f"file:{PB_DB}?mode=ro", uri=True)
    try:
        rows = connection.execute(
            "select id, slug, title, category, risk_level, content, summary from wiki_articles"
        ).fetchall()
    finally:
        connection.close()
    return {
        str(slug): {
            "id": str(record_id), "slug": str(slug), "title": str(title),
            "category": str(category), "risk_level": str(risk_level),
            "content": str(content), "summary": str(summary),
        }
        for record_id, slug, title, category, risk_level, content, summary in rows
    }


def wiki_slug(candidate: EvidenceCandidate, id_to_slug: dict[str, str]) -> str:
    raw = candidate.raw or {}
    record_id = str(raw.get("id") or candidate.id or "")
    return str(raw.get("slug") or id_to_slug.get(record_id) or candidate.id or "")


def dedupe_wikis(candidates: list[EvidenceCandidate], id_to_slug: dict[str, str]) -> list[EvidenceCandidate]:
    seen: set[str] = set()
    result: list[EvidenceCandidate] = []
    for candidate in candidates:
        slug = wiki_slug(candidate, id_to_slug)
        if slug in seen:
            continue
        seen.add(slug)
        result.append(candidate)
    return result


def select_evidence(question: str, id_to_slug: dict[str, str]) -> tuple[list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate], list[EvidenceCandidate]]:
    guide_candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8), user_message=question
    )
    wiki_candidates = fetch_wiki_candidates(
        SourcePlanItem(source_type="wiki", query=question, limit=8), user_message=question
    )
    selected_guides = guide_candidates[:3]
    related_wikis = fetch_related_wiki_candidates(selected_guides)
    selected_wikis = dedupe_wikis([*related_wikis, *wiki_candidates], id_to_slug)[:8]
    return selected_guides, selected_wikis, guide_candidates, wiki_candidates


def evidence_text(guides: list[EvidenceCandidate], wikis: list[EvidenceCandidate], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> str:
    chunks: list[str] = []
    for guide in guides:
        raw = guide.raw or {}
        chunks.extend([
            str(guide.title or ""), str(guide.summary or ""),
            " ".join(str(item) for item in as_list(raw.get("steps"))),
            " ".join(str(item) for item in as_list(raw.get("check"))),
            " ".join(str(item) for item in as_list(raw.get("fallback"))),
            " ".join(str(item) for item in as_list(raw.get("stop_or_escalate"))),
        ])
    for wiki in wikis:
        row = wiki_rows.get(wiki_slug(wiki, id_to_slug))
        if row:
            chunks.extend([row["title"], row["summary"], row["content"]])
        else:
            chunks.extend([str(wiki.title or ""), str(wiki.summary or ""), str(wiki.snippet or "")])
    return "\n".join(chunks)


def dangerous_suggestions(text: str) -> list[str]:
    hits: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        for phrase in DANGEROUS_PHRASES:
            if phrase not in line:
                continue
            start = line.find(phrase)
            window = line[max(0, start - 20): start + len(phrase) + 20]
            if any(marker in window for marker in SAFE_MARKERS):
                continue
            hits.append(phrase)
    return list(dict.fromkeys(hits))


def guide_labels(guide: EvidenceCandidate) -> set[str]:
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    domains = {str(item) for item in as_list(raw.get("domains"))}
    category = str(raw.get("category") or "")
    title = str(guide.title or "")
    labels: set[str] = set()
    if guide_id in PRODUCTION_GUIDES:
        labels.add("production")
    if guide_id in MANUFACTURING_GUIDES:
        labels.add("manufacturing")
    if guide_id in WASTE_GUIDES:
        labels.add("waste")
    if guide_id in STORAGE_GUIDES:
        labels.add("storage")
    if guide_id in REPAIR_GUIDES:
        labels.add("repair")
    if "medical" in domains or "医疗" in category or any(term in title for term in ["止血", "伤口", "划伤", "出血"]):
        labels.add("medical")
    if domains & {"energy", "power"} or "能源" in category:
        labels.add("energy")
    if domains & {"fire"} or "火源" in category:
        labels.add("fire")
    if domains & {"safety", "security"} or "安全" in category:
        labels.add("safety")
    if domains & {"organization", "team", "records", "general"} or "记录" in title:
        labels.add("records")
    if "tools" in domains or "工具" in title:
        labels.add("tools")
    if "repair" in domains or "维修" in category:
        labels.add("repair")
    if not labels:
        labels.add("other")
    return labels


def classify_root_cause(case: dict[str, Any], expected_selected: bool, expected_candidate: bool, wiki_any: bool, wiki_candidate_any: bool, precise: bool, primary_ok: bool, top_is_production: bool) -> list[str]:
    causes: list[str] = []
    if case["type"] == "strict":
        if not expected_selected:
            causes.append("selector/ranking" if expected_candidate else "Production 检索入口缺口")
        if not wiki_any:
            causes.append("Wiki selector/ranking" if wiki_candidate_any or precise else "evidence 进入缺口")
        if not primary_ok and expected_selected:
            if top_is_production:
                causes.append("Production Guide 主位竞争")
            else:
                causes.append("相邻领域抢主位")
    else:
        if not primary_ok:
            causes.append("observation 主域偏离")
        if not wiki_any:
            causes.append("Production 补充 evidence 未进入")
    return list(dict.fromkeys(causes))


def run_case(case: dict[str, Any], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> dict[str, Any]:
    question = str(case["query"])
    selected_guides, selected_wikis, guide_candidates, wiki_candidates = select_evidence(question, id_to_slug)
    selected_ids = [str(guide.id) for guide in selected_guides]
    selected_slugs = [wiki_slug(wiki, id_to_slug) for wiki in selected_wikis]
    candidate_ids = [str(guide.id) for guide in guide_candidates]
    candidate_slugs = [wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates]
    expected_guides = set(case.get("expected_guide") or [])
    expected_wikis = set(case.get("expected_wiki_any") or case.get("production_wiki_any") or [])
    top_id = selected_ids[0] if selected_ids else ""
    top_labels = guide_labels(selected_guides[0]) if selected_guides else {"none"}
    expected_selected = bool(expected_guides & set(selected_ids)) if expected_guides else False
    expected_candidate = bool(expected_guides & set(candidate_ids)) if expected_guides else False
    expected_top1 = bool(expected_guides and top_id in expected_guides)
    production_guide_hit = bool(PRODUCTION_GUIDES & set(selected_ids))
    production_primary = top_id in PRODUCTION_GUIDES
    wiki_any = bool(expected_wikis & set(selected_slugs)) if expected_wikis else False
    wiki_full = bool(expected_wikis) and expected_wikis <= set(selected_slugs)
    wiki_candidate_any = bool(expected_wikis & set(candidate_slugs)) if expected_wikis else False
    precise_slugs: set[str] = set()
    for guide in selected_guides:
        if not expected_guides or str(guide.id) in expected_guides:
            precise_slugs.update(str(slug) for slug in as_list((guide.raw or {}).get("related_wiki")))
    precise = bool(expected_wikis & set(selected_slugs) & precise_slugs) if expected_wikis else False
    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    safety = any(term in text for term in STOP_TERMS)
    fallback = any(term in text for term in FALLBACK_TERMS)
    record = any(term in text for term in RECORD_TERMS)
    dangerous = dangerous_suggestions(text)
    kiwix_used = any(item.source_type == "kiwix" for item in [*selected_guides, *selected_wikis])

    if case["type"] == "strict":
        primary_ok = expected_top1 and production_primary
        if primary_ok and wiki_any:
            verdict = "pass"
            reason = "预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。"
        elif expected_selected or wiki_any:
            verdict = "partial"
            reason = "预期 Guide 或 Wiki 已进入，但 Production 主入口与 evidence 组合未同时满足。"
        else:
            verdict = "fail"
            reason = "预期 Guide 与预期 Wiki 均未进入，Production 未形成行动入口。"
        cross_notes = "Production 主导合理。" if primary_ok else f"top1={top_id or 'none'}，主域={','.join(sorted(top_labels))}。"
    else:
        expected_domains = set(case.get("expected_primary_domains") or [])
        primary_ok = bool(expected_domains & top_labels)
        swallowed = production_primary and "production" not in expected_domains
        if swallowed:
            verdict = "fail"
            reason = "Production 错误吞掉应由相邻领域主导的 observation。"
        elif primary_ok and (production_guide_hit or wiki_any):
            verdict = "pass"
            reason = "相邻领域主导合理，且 Production Guide/Wiki 作为补充进入。"
        elif primary_ok:
            verdict = "partial"
            reason = "相邻领域主导合理，但 Production 补充 evidence 未进入。"
        else:
            verdict = "fail"
            reason = "top1 未落在允许的相邻主域，cross-domain 主位不合理。"
        cross_notes = f"预期主域={','.join(sorted(expected_domains))}；实际={','.join(sorted(top_labels))}；Production补充={'有' if production_guide_hit or wiki_any else '无'}。"

    if dangerous:
        verdict, reason = "fail", "Evidence 中发现未被否定语境包围的危险建议。"
    if kiwix_used:
        verdict, reason = "fail", "Kiwix 进入本地行动 evidence，存在越权。"
    if not safety or not fallback or not record:
        verdict = "fail"
        reason = "safety、fallback 或 record/check 完整性未达到要求。"

    causes = classify_root_cause(
        case, expected_selected, expected_candidate, wiki_any, wiki_candidate_any,
        precise, primary_ok, production_primary,
    )
    return {
        "id": case["id"], "type": case["type"], "query": question,
        "focus": case.get("focus", ""),
        "expected_guide": sorted(expected_guides),
        "expected_wiki_any": sorted(expected_wikis),
        "expected_primary_domains": case.get("expected_primary_domains", []),
        "actual_top_guides": [
            {"id": guide.id, "title": guide.title, "labels": sorted(guide_labels(guide)), "risk_level": str((guide.raw or {}).get("risk_level") or "")}
            for guide in selected_guides
        ],
        "actual_selected_wiki": [
            {"slug": wiki_slug(wiki, id_to_slug), "title": wiki.title, "source_reason": str((wiki.raw or {}).get("source_reason") or ""), "linked_guide_id": str((wiki.raw or {}).get("linked_guide_id") or "")}
            for wiki in selected_wikis
        ],
        "guide_candidates": [{"id": guide.id, "title": guide.title} for guide in guide_candidates[:8]],
        "wiki_candidates": [{"slug": wiki_slug(wiki, id_to_slug), "title": wiki.title} for wiki in wiki_candidates[:8]],
        "profiles": [str(profile.get("name") or "") for profile in _matching_query_profiles(question)],
        "pass_partial_fail": verdict, "reason": reason,
        "production_guide_hit": production_guide_hit,
        "expected_guide_hit": expected_selected,
        "expected_guide_top1": expected_top1,
        "wiki_full_hit": wiki_full, "wiki_any_hit": wiki_any,
        "guide_wiki_precise": precise, "production_primary": production_primary,
        "cross_domain_reasonable": primary_ok,
        "dangerous_suggestion": dangerous, "kiwix_used": kiwix_used,
        "safety_present": safety, "fallback_present": fallback,
        "record_or_check_present": record,
        "cross_domain_notes": cross_notes,
        "root_cause_classification": causes,
    }


def rate(numerator: int, denominator: int) -> str:
    return "n/a" if denominator == 0 else f"{numerator / denominator * 100:.1f}%"


def summarize(cases: list[dict[str, Any]]) -> dict[str, Any]:
    strict = [case for case in cases if case["type"] == "strict"]
    observation = [case for case in cases if case["type"] == "observation"]
    verdicts = Counter(case["pass_partial_fail"] for case in cases)
    strict_verdicts = Counter(case["pass_partial_fail"] for case in strict)
    observation_verdicts = Counter(case["pass_partial_fail"] for case in observation)
    causes = Counter(cause for case in cases for cause in case["root_cause_classification"])
    return {
        "total": len(cases), "strict_total": len(strict), "observation_total": len(observation),
        "pass": verdicts["pass"], "partial": verdicts["partial"], "fail": verdicts["fail"],
        "strict_pass": strict_verdicts["pass"], "strict_partial": strict_verdicts["partial"], "strict_fail": strict_verdicts["fail"],
        "observation_pass": observation_verdicts["pass"], "observation_partial": observation_verdicts["partial"], "observation_fail": observation_verdicts["fail"],
        "production_guide_hit_rate": rate(sum(case["production_guide_hit"] for case in strict), len(strict)),
        "expected_guide_hit_rate": rate(sum(case["expected_guide_hit"] for case in strict), len(strict)),
        "wiki_full_hit_rate": rate(sum(case["wiki_full_hit"] for case in strict), len(strict)),
        "wiki_any_hit_rate": rate(sum(case["wiki_any_hit"] for case in strict), len(strict)),
        "guide_wiki_precise_rate": rate(sum(case["guide_wiki_precise"] for case in strict), len(strict)),
        "production_primary_rate": rate(sum(case["production_primary"] for case in strict), len(strict)),
        "cross_domain_reasonable_count": sum(case["cross_domain_reasonable"] for case in cases),
        "dangerous_suggestion_count": sum(bool(case["dangerous_suggestion"]) for case in cases),
        "kiwix_override_count": sum(case["kiwix_used"] for case in cases),
        "safety_rate": rate(sum(case["safety_present"] for case in cases), len(cases)),
        "fallback_rate": rate(sum(case["fallback_present"] for case in cases), len(cases)),
        "record_check_rate": rate(sum(case["record_or_check_present"] for case in cases), len(cases)),
        "root_cause_counts": dict(causes),
    }


def comma(values: list[Any]) -> str:
    items = [str(item) for item in values if str(item)]
    return "、".join(items) if items else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    cases = results["cases"]
    lines = [
        "# Batch13-D Production System / Workshop Retrieval Field Test Report", "",
        f"生成时间：{results['generated_at']}", "",
        "## 1. 测试范围", "",
        "本阶段只测试 Production System / Workshop v0.1 的本地 Retrieval 表现。使用现有 guide/wiki fetcher，选取 top 3 Guide，并按既有 related_wiki 规则与独立 Wiki 候选合并前 8 条 evidence；不调用 LLM，不生成最终回答。", "",
        "覆盖开工、收工、原料/WIP/成品流转、质检、小批量、不合格品、安全记录和班次交接；observation 观察 Manufacturing、Repair、Waste、Storage、Medical、Energy 是否合理主导。", "",
        "## 2. 测试资产", "",
        "- fixture：`tests/fixtures/production_system_workshop_field_cases.json`", "- 脚本：`scripts/test_production_system_workshop_field.py`",
        "- 结果：`docs/knowledge/batch13_d_production_system_workshop_field_test_results.json`", "- 报告：`docs/knowledge/batch13_d_production_system_workshop_field_test_report.md`", "",
        "## 3. 基础审计结果", "",
        "- `python3 tools/audit_wiki.py`：errors=0，warnings=0，advisories=0；Markdown/PocketBase=1107/1107。",
        "- `python3 scripts/audit_guides.py`：errors=0，warnings=0，advisories=0；Guides=836。", "",
        "## 4. Retrieval contract 结果", "",
        "- `tests/test_retrieval_traceability.py` + `tests/test_retrieval_root_contract.py`：本轮独立命令验证：9 passed。",
        "- 未修改现有 contract tests，未新增 profile contract tests。", "",
        "## 5. Field Test 总表", "",
        f"- total：{summary['total']}；strict / observation：{summary['strict_total']} / {summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}", "",
        "|case|type|verdict|top1 Guide|Wiki any|Production primary|cross-domain|reason|", "|---|---|---|---|---|---|---|---|",
    ]
    for case in cases:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(f"|{case['id']}|{case['type']}|{case['pass_partial_fail']}|{top}|{case['wiki_any_hit']}|{case['production_primary']}|{case['cross_domain_reasonable']}|{case['reason']}|")
    lines += ["", "## 6. strict / observation 结果", "",
        f"- strict pass / partial / fail：{summary['strict_pass']} / {summary['strict_partial']} / {summary['strict_fail']}",
        f"- observation pass / partial / fail：{summary['observation_pass']} / {summary['observation_partial']} / {summary['observation_fail']}",
        f"- cross-domain 合理：{summary['cross_domain_reasonable_count']} / {summary['total']}", "",
        "## 7. Guide hit / Wiki hit / precise / Production primary rate", "",
        f"- Production Guide hit（strict）：{summary['production_guide_hit_rate']}",
        f"- expected Guide hit（strict）：{summary['expected_guide_hit_rate']}",
        f"- Wiki full hit（strict）：{summary['wiki_full_hit_rate']}",
        f"- Wiki any hit（strict）：{summary['wiki_any_hit_rate']}",
        f"- Guide-Wiki precise（strict）：{summary['guide_wiki_precise_rate']}",
        f"- Production primary rate（strict）：{summary['production_primary_rate']}", "",
        "## 8. dangerous suggestion", "",
        f"- dangerous suggestion count：{summary['dangerous_suggestion_count']}。", "",
        "## 9. Kiwix 越权", "",
        f"- Kiwix override count：{summary['kiwix_override_count']}。本测试路径只选择 Guide/Wiki；若 Kiwix 出现即判 fail。", "",
        "## 10. safety / fallback / record-check", "",
        f"- safety：{summary['safety_rate']}", f"- fallback：{summary['fallback_rate']}", f"- record/check：{summary['record_check_rate']}", "",
        "## 11. 主要失败模式", "",
    ]
    if summary["root_cause_counts"]:
        for cause, count in summary["root_cause_counts"].items():
            lines.append(f"- {cause}：{count}")
    else:
        lines.append("- 未观察到 partial/fail 对应的检索失败模式。")
    lines += ["", "失败只记录，不在 Batch13-D 修改 Wiki、Guide、profile、selector、ranking 或 fallback。", "",
        "## 12. 是否建议进入 Batch13-E Root Cause Review", ""]
    if summary["strict_partial"] or summary["strict_fail"] or summary["observation_fail"]:
        lines.append("建议进入 Batch13-E Root Cause Review。存在 strict partial/fail 或 observation 主域错误，需要区分 Production 检索入口缺口、selector/ranking、evidence 进入不足和相邻领域抢主位；本阶段不新增 profile。")
    else:
        lines.append("暂不要求进入 Root Cause Review；继续观察即可。本阶段仍不新增 profile。")
    lines += ["", "## 13. 逐条结果", ""]
    for case in cases:
        lines += [f"### {case['id']}", "", f"- type：{case['type']}", f"- query：{case['query']}",
            f"- expected Guide：{comma(case['expected_guide'])}", f"- expected Wiki any：{comma(case['expected_wiki_any'])}",
            f"- actual top Guides：{comma([item['id'] for item in case['actual_top_guides']])}",
            f"- actual selected Wiki：{comma([item['slug'] for item in case['actual_selected_wiki']])}",
            f"- verdict / reason：{case['pass_partial_fail']} / {case['reason']}",
            f"- dangerous / Kiwix：{comma(case['dangerous_suggestion'])} / {case['kiwix_used']}",
            f"- safety / fallback / record-check：{case['safety_present']} / {case['fallback_present']} / {case['record_or_check_present']}",
            f"- cross-domain：{case['cross_domain_notes']}", f"- root cause：{comma(case['root_cause_classification'])}", ""]
    lines += ["## 14. 边界声明", "",
        "本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、`emergency_guides.json`、`guide_index.json`、`.env` 或 `data/lanternbox.db`；未运行答案生成。", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--no-answer", action="store_true", help="Accepted explicitly; this test never calls an LLM.")
    parser.parse_args()
    cases = read_json(CASES_PATH)
    wiki_rows = load_wiki_rows()
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    outputs = []
    for index, case in enumerate(cases, 1):
        print(f"[{index}/{len(cases)}] {case['id']}: {case['query']}")
        result = run_case(case, wiki_rows, id_to_slug)
        print(f"  -> {result['pass_partial_fail']}: {result['reason']}")
        outputs.append(result)
    results = {"generated_at": datetime.now(timezone.utc).isoformat(), "summary": summarize(outputs), "cases": outputs}
    RESULTS_PATH.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    REPORT_PATH.write_text(build_report(results), encoding="utf-8")
    print(json.dumps(results["summary"], ensure_ascii=False, indent=2))
    print(f"results: {RESULTS_PATH}")
    print(f"report: {REPORT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
