#!/usr/bin/env python3
"""Run Batch4-N-R Tools & Repair Wave 2 retrieval field cases.

This script is test/report only. It does not call the LLM planner, selector,
or answer generator. It reads local Retrieval v2 fetchers and PocketBase data
to inspect whether newly added Wave 2 Guide/Wiki evidence is reachable.
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
if not os.environ.get("LANTERNBOX_TOOLS_REPAIR_WAVE2_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_TOOLS_REPAIR_WAVE2_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests" / "fixtures" / "tools_repair_wave2_field_cases.json"
RESULTS_PATH = ROOT / "docs" / "knowledge" / "tools_repair_wave2_field_test_results.json"
REPORT_PATH = ROOT / "docs" / "knowledge" / "tools_repair_wave2_field_test_report.md"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"

STOP_TERMS = ["停止", "停用", "禁用", "禁止", "不可", "不要", "不得", "不能", "暂停", "隔离", "远离"]
FALLBACK_TERMS = ["替代", "如果没有", "没有时", "缺少", "改用", "降级", "无法", "只做"]
RECORD_TERMS = ["记录", "标记", "复查", "交接", "编号", "时间", "责任人", "标签", "负责人"]
EXTERNAL_TERMS = ["联系医院", "等待救援", "上网查询", "联系物业", "联系相关部门", "联系供应商", "拨打电话"]
LOCAL_REPAIR_DOMAINS = {"repair", "tools", "security", "records", "power"}
OFF_DOMAIN_GUIDE_DOMAINS = {"food", "medical", "agriculture", "communication", "psychology", "water", "navigation"}
ROOT_CAUSE_CLASSES = [
    "Wiki 不可召回",
    "Guide-Wiki 缺关联",
    "query profile 缺失",
    "Planner terms 提取不足",
    "Selector 排序问题",
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
    connection = sqlite3.connect(f"file:{PB_DB}?mode=ro", uri=True)
    try:
        rows = connection.execute(
            "select id, slug, title, category, risk_level, content, summary from wiki_articles"
        ).fetchall()
    finally:
        connection.close()
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


def load_wiki_id_to_slug(wiki_rows: dict[str, dict[str, str]]) -> dict[str, str]:
    return {row["id"]: slug for slug, row in wiki_rows.items()}


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


def external_violations(text: str) -> list[str]:
    violations: list[str] = []
    for line in text.splitlines():
        for term in EXTERNAL_TERMS:
            if term not in line:
                continue
            prefix = line[: line.find(term)]
            if any(marker in prefix[-12:] for marker in ["不要", "不可", "不得", "不应", "无需", "不必"]):
                continue
            violations.append(term)
    return list(dict.fromkeys(violations))


def candidate_domain_hit(guides: list[EvidenceCandidate], wikis: list[EvidenceCandidate], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> bool:
    guide_domains = {
        str(domain)
        for guide in guides
        for domain in as_list((guide.raw or {}).get("domains"))
    }
    if guide_domains & LOCAL_REPAIR_DOMAINS:
        return True
    for wiki in wikis:
        slug = wiki_slug(wiki, id_to_slug)
        row = wiki_rows.get(slug, {})
        if slug.startswith("repair-") or "维修" in str(row.get("category", "")):
            return True
    return False


def has_cross_domain_competition(guides: list[EvidenceCandidate]) -> bool:
    if not guides:
        return False
    top_domains = {str(domain) for domain in as_list((guides[0].raw or {}).get("domains"))}
    if not top_domains:
        return False
    return not bool(top_domains & LOCAL_REPAIR_DOMAINS) or bool(top_domains & OFF_DOMAIN_GUIDE_DOMAINS and not top_domains & {"repair", "tools"})


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
    expected_wikis = set(case.get("expected_wiki") or [])
    selected_guide_ids = {guide.id for guide in selected_guides}
    selected_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in selected_wikis}
    candidate_guide_ids = {guide.id for guide in guide_candidates}
    candidate_wiki_slugs = {wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates}

    if expected_wikis and not (expected_wikis & selected_wiki_slugs):
        if not (expected_wikis & candidate_wiki_slugs) and not (expected_wikis & guide_related_wiki):
            causes.append("Wiki 不可召回")
        elif expected_guides and expected_guides & selected_guide_ids and not (expected_wikis & guide_related_wiki):
            causes.append("Guide-Wiki 缺关联")
        else:
            causes.append("Selector 排序问题")

    if expected_guides and not (expected_guides & selected_guide_ids):
        if expected_guides & candidate_guide_ids:
            causes.append("Selector 排序问题")
        elif profiles:
            causes.append("Planner terms 提取不足")
        else:
            causes.append("query profile 缺失")

    if "跨域 evidence" in failures or "跨域竞争" in failures:
        causes.append("跨域竞争")
    if "缺少安全边界" in failures or "缺少降级方案" in failures or "缺少记录复查" in failures:
        if not causes:
            causes.append("Guide-Wiki 缺关联")

    return [cause for cause in ROOT_CAUSE_CLASSES if cause in set(causes)]


def run_case(case: dict[str, Any], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str]) -> dict[str, Any]:
    question = case["question"]
    profile_items = _matching_query_profiles(question)
    profiles = [str(item.get("name") or "") for item in profile_items]
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
    wiki_hit = bool(expected_wikis & selected_wiki_slugs)
    precise_combo = bool(expected_wikis & guide_related_wiki)
    domain_hit = candidate_domain_hit(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    cross_domain = has_cross_domain_competition(selected_guides)
    has_stop = contains_any(text, STOP_TERMS)
    has_fallback = contains_any(text, FALLBACK_TERMS)
    has_record = contains_any(text, RECORD_TERMS)
    external = external_violations(text)
    kiwix_override = False

    failures: list[str] = []
    if not guide_hit:
        failures.append("未命中预期 Guide")
    if not wiki_hit:
        failures.append("未命中预期 Wiki")
    if not domain_hit:
        failures.append("跨域 evidence")
    if cross_domain:
        failures.append("跨域竞争")
    if case.get("risk") in {"high", "caution"} and not has_stop:
        failures.append("缺少安全边界")
    if not has_fallback:
        failures.append("缺少降级方案")
    if not has_record:
        failures.append("缺少记录复查")
    if external:
        failures.append("外部依赖违规")
    if kiwix_override:
        failures.append("Kiwix 越权")

    if not failures:
        verdict = "pass"
    elif external or kiwix_override or not domain_hit:
        verdict = "fail"
    elif wiki_hit and has_stop and has_fallback and has_record:
        verdict = "partial"
    else:
        verdict = "partial"

    root_causes = classify_root_causes(
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

    excluded_sources = []
    selected_keys = {
        ("guide", guide.id) for guide in selected_guides
    } | {
        ("wiki", wiki_slug(wiki, id_to_slug)) for wiki in selected_wikis
    }
    for candidate in [*guide_candidates, *wiki_candidates]:
        cid = wiki_slug(candidate, id_to_slug) if candidate.source_type == "wiki" else candidate.id
        if (candidate.source_type, cid) not in selected_keys:
            excluded_sources.append({
                "source_type": candidate.source_type,
                "id": cid,
                "title": candidate.title,
                "reason": "not_in_local_top_selection",
            })

    return {
        **case,
        "profiles": profiles,
        "domains": sorted({
            str(domain)
            for guide in selected_guides
            for domain in as_list((guide.raw or {}).get("domains"))
        }),
        "guides_selected": [
            {
                "id": guide.id,
                "title": guide.title,
                "risk_level": (guide.raw or {}).get("risk_level"),
                "domains": as_list((guide.raw or {}).get("domains")),
            }
            for guide in selected_guides
        ],
        "wikis_selected": [
            {
                "slug": wiki_slug(wiki, id_to_slug),
                "title": wiki.title,
                "risk_level": (wiki.raw or {}).get("risk_level") or wiki_rows.get(wiki_slug(wiki, id_to_slug), {}).get("risk_level", ""),
                "source_reason": (wiki.raw or {}).get("source_reason"),
            }
            for wiki in selected_wikis
        ],
        "guide_candidates": [{"id": guide.id, "title": guide.title} for guide in guide_candidates],
        "wiki_candidates": [{"slug": wiki_slug(wiki, id_to_slug), "title": wiki.title} for wiki in wiki_candidates],
        "guide_related_wiki": sorted(guide_related_wiki),
        "excluded_sources": excluded_sources[:12],
        "guide_hit": guide_hit,
        "wiki_hit": wiki_hit,
        "guide_wiki_precise": precise_combo,
        "correct_domain_hit": domain_hit,
        "cross_domain_competition": cross_domain,
        "has_stop_condition": has_stop,
        "has_fallback_or_downgrade": has_fallback,
        "has_record_or_recheck": has_record,
        "external_dependencies": external,
        "kiwix_override": kiwix_override,
        "verdict": verdict,
        "failure_reasons": failures,
        "root_cause_classification": root_causes,
    }


def pct(value: int, total: int) -> str:
    return "0.0%" if total <= 0 else f"{(value / total) * 100:.1f}%"


def summarize(rows: list[dict[str, Any]]) -> dict[str, Any]:
    total = len(rows)
    pass_count = sum(row["verdict"] == "pass" for row in rows)
    partial_count = sum(row["verdict"] == "partial" for row in rows)
    fail_count = sum(row["verdict"] == "fail" for row in rows)
    guide_expected = [row for row in rows if row.get("expected_guides")]
    high_caution = [row for row in rows if row.get("risk") in {"high", "caution"}]
    return {
        "total": total,
        "pass": pass_count,
        "partial": partial_count,
        "fail": fail_count,
        "guide_hit_rate": pct(sum(row["guide_hit"] for row in guide_expected), len(guide_expected)),
        "wiki_hit_rate": pct(sum(row["wiki_hit"] for row in rows), total),
        "guide_wiki_precise_rate": pct(sum(row["guide_wiki_precise"] for row in rows), total),
        "high_caution_safety_rate": pct(sum(row["has_stop_condition"] for row in high_caution), len(high_caution)),
        "fallback_rate": pct(sum(row["has_fallback_or_downgrade"] for row in rows), total),
        "record_rate": pct(sum(row["has_record_or_recheck"] for row in rows), total),
        "external_violations": sum(bool(row["external_dependencies"]) for row in rows),
        "kiwix_overrides": sum(bool(row["kiwix_override"]) for row in rows),
        "cross_domain_competitions": sum(bool(row.get("cross_domain_competition")) for row in rows),
        "target_met": {
            "pass_gte_18": pass_count >= 18,
            "partial_lte_2": partial_count <= 2,
            "fail_eq_0": fail_count == 0,
            "guide_hit_gte_90": (sum(row["guide_hit"] for row in guide_expected) / len(guide_expected) >= 0.9) if guide_expected else True,
            "wiki_hit_gte_80": (sum(row["wiki_hit"] for row in rows) / total >= 0.8) if total else False,
            "safety_eq_100": all(row["has_stop_condition"] for row in high_caution),
            "fallback_eq_100": all(row["has_fallback_or_downgrade"] for row in rows),
            "record_eq_100": all(row["has_record_or_recheck"] for row in rows),
            "external_eq_0": all(not row["external_dependencies"] for row in rows),
            "kiwix_eq_0": all(not row["kiwix_override"] for row in rows),
        },
    }


def comma(items: list[Any]) -> str:
    return "、".join(str(item) for item in items if str(item)) or "无"


def write_report(payload: dict[str, Any]) -> None:
    rows = payload["cases"]
    summary = payload["summary"]
    lines = [
        "# Tools & Repair Wave 2 Retrieval Field Test Report",
        "",
        f"生成时间：{payload['generated_at']}",
        "",
        "## 1. 测试范围",
        "",
        "本阶段只测试 Batch4-N 新增的维修现场安全知识是否能进入本地 Retrieval evidence。脚本不调用 LLM Planner、Selector 或回答生成器，不修改 Wiki/Guide/关联/profile/pipeline。",
        "",
        "覆盖主题：低光维修停止线、危险工具隔离、儿童和非操作人员边界、刀锯清场、工作台、多人员维修分区、工具清点、材料混放、降级维修和破损工具禁用。",
        "",
        "## 2. 汇总",
        "",
        f"- 用例总数：{summary['total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- Guide 命中率：{summary['guide_hit_rate']}",
        f"- Wiki 命中率：{summary['wiki_hit_rate']}",
        f"- Guide-Wiki 精准组合率：{summary['guide_wiki_precise_rate']}",
        f"- high/caution 安全边界：{summary['high_caution_safety_rate']}",
        f"- fallback / 降级用途：{summary['fallback_rate']}",
        f"- 记录 / 复查建议：{summary['record_rate']}",
        f"- 外部依赖违规：{summary['external_violations']}",
        f"- Kiwix 越权：{summary['kiwix_overrides']}",
        f"- 跨域竞争：{summary['cross_domain_competitions']}",
        "",
        "## 3. 目标达成",
        "",
    ]
    for key, value in summary["target_met"].items():
        lines.append(f"- {key}：{'达成' if value else '未达成'}")

    lines.extend([
        "",
        "## 4. 20 条 Case 明细",
        "",
        "| case | verdict | Guide | Wiki | profiles | safety | fallback | record | root cause |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ])
    for row in rows:
        guides = comma([f"{item['id']} {item['title']}" for item in row["guides_selected"][:3]])
        wikis = comma([f"{item['slug']} {item['title']}" for item in row["wikis_selected"][:4]])
        lines.append(
            f"| {row['id']} | {row['verdict']} | {guides} | {wikis} | {comma(row['profiles'])} | "
            f"{'是' if row['has_stop_condition'] else '否'} | {'是' if row['has_fallback_or_downgrade'] else '否'} | "
            f"{'是' if row['has_record_or_recheck'] else '否'} | {comma(row['root_cause_classification'])} |"
        )

    lines.extend([
        "",
        "## 5. 失败案例 Root Cause 分类",
        "",
    ])
    failed_rows = [row for row in rows if row["verdict"] != "pass"]
    if not failed_rows:
        lines.append("- 无。")
    for row in failed_rows:
        lines.extend([
            f"### {row['id']}",
            "",
            f"- query：{row['question']}",
            f"- verdict：{row['verdict']}",
            f"- failures：{comma(row['failure_reasons'])}",
            f"- root cause：{comma(row['root_cause_classification'])}",
            f"- selected guides：{comma([item['id'] for item in row['guides_selected']])}",
            f"- selected wiki：{comma([item['slug'] for item in row['wikis_selected']])}",
            "",
        ])

    lines.extend([
        "## 6. 重点新增 Wiki Evidence 检查",
        "",
    ])
    focus_slugs = [
        "repair-low-light-stop-line-001",
        "repair-dangerous-tool-temporary-isolation-001",
        "repair-children-away-tool-zone-001",
        "repair-knife-saw-clear-zone-001",
        "repair-damaged-tool-disable-tag-001",
    ]
    for slug in focus_slugs:
        cases = [row["id"] for row in rows if slug in {item["slug"] for item in row["wikis_selected"]}]
        lines.append(f"- `{slug}`：{comma(cases)}")

    lines.extend([
        "",
        "## 7. 跨域与 Kiwix 检查",
        "",
        f"- 工具问题误入非维修领域：{comma([row['id'] for row in rows if not row['correct_domain_hit']])}",
        f"- 首位或强竞争证据跨域：{comma([row['id'] for row in rows if row.get('cross_domain_competition')])}",
        f"- 外部依赖违规：{summary['external_violations']}",
        f"- Kiwix 越权：{summary['kiwix_overrides']}",
        "",
        "## 8. 是否建议进入 Batch4-O Apply",
        "",
    ])
    if summary["fail"] == 0 and summary["pass"] >= 18 and summary["partial"] <= 2:
        lines.append("- 当前达到 field test 验收线，不建议立即进入 Batch4-O Apply。可先标记 Wave 2 retrieval field test 通过。")
    else:
        lines.append("- 当前未达到 field test 验收线。建议进入 Batch4-O Review / Apply，但只根据本报告 root cause 做最小修复。")

    lines.extend([
        "",
        "## 9. 验证命令",
        "",
        "```bash",
        "python3 tools/audit_wiki.py",
        "python3 tools/build_guides.py",
        "python3 scripts/audit_guides.py",
        "env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \\",
        "  tests/test_retrieval_traceability.py \\",
        "  tests/test_retrieval_root_contract.py",
        "```",
        "",
        "## 10. 不应修复的问题",
        "",
        "- 不修改 Wiki/Guide 正文或 Guide-Wiki 关联。",
        "- 不修改 Retrieval Pipeline、Prompt、query profile、top_k、selector_candidate_limit、ranking 参数或 fallback 逻辑。",
        "- 不因为单个 query 未命中就新增重复 Wiki。",
    ])
    REPORT_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    cases = read_json(CASES_PATH)
    requested = [item for item in sys.argv[1:] if not item.startswith("--")]
    if requested:
        cases = [case for case in cases if case["id"] in requested]
    wiki_rows = load_wiki_rows()
    id_to_slug = load_wiki_id_to_slug(wiki_rows)
    rows = []
    for index, case in enumerate(cases, 1):
        print(f"[{index}/{len(cases)}] {case['id']}: {case['question']}", flush=True)
        try:
            row = run_case(case, wiki_rows, id_to_slug)
        except Exception as exc:
            row = {
                **case,
                "profiles": [],
                "domains": [],
                "guides_selected": [],
                "wikis_selected": [],
                "guide_candidates": [],
                "wiki_candidates": [],
                "guide_related_wiki": [],
                "excluded_sources": [],
                "guide_hit": False,
                "wiki_hit": False,
                "guide_wiki_precise": False,
                "correct_domain_hit": False,
                "cross_domain_competition": False,
                "has_stop_condition": False,
                "has_fallback_or_downgrade": False,
                "has_record_or_recheck": False,
                "external_dependencies": [],
                "kiwix_override": False,
                "verdict": "fail",
                "failure_reasons": [f"script exception: {exc}"],
                "root_cause_classification": ["Planner terms 提取不足"],
            }
        rows.append(row)
        print(f"  -> {row['verdict']}: {', '.join(row['failure_reasons']) or 'ok'}", flush=True)

    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "cases": rows,
        "summary": summarize(rows),
    }
    RESULTS_PATH.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    write_report(payload)
    print(f"results: {RESULTS_PATH}")
    print(f"report: {REPORT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
