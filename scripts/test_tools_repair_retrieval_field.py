#!/usr/bin/env python3
"""Run Batch4-I Tools & Repair retrieval field cases.

This script is intentionally test/report only. It reads the existing
Retrieval v2 pipeline and knowledge database, then writes a JSON result file
and a Markdown report without changing Guide, Wiki, prompt, profile, or
pipeline code.
"""

from __future__ import annotations

import json
import os
import re
import sqlite3
import sys
from datetime import datetime, timezone
from pathlib import Path
from types import SimpleNamespace
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"
if not os.environ.get("LANTERNBOX_TOOLS_REPAIR_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_TOOLS_REPAIR_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.pipeline.builder import build_pipeline_request
from api.pipeline.emergency import run_emergency_pipeline
from api.pipeline.preload import prepare_ai_pipeline_context
from api.response.safety import sanitize_ai_answer

CASES_PATH = ROOT / "tests" / "fixtures" / "tools_repair_retrieval_field_cases.json"
RESULTS_PATH = ROOT / "docs" / "knowledge" / "tools_repair_retrieval_field_test_results.json"
REPORT_PATH = ROOT / "docs" / "knowledge" / "tools_repair_retrieval_field_test_report.md"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"

STOP_TERMS = ["停止", "停用", "禁用", "禁止", "不可", "不要", "断电", "不通电", "禁承重", "换固定点"]
FALLBACK_TERMS = ["替代", "如果没有", "没有时", "缺少", "改用", "降级", "分批", "减载", "无法"]
RECORD_TERMS = ["记录", "标记", "复查", "交接", "编号", "时间", "责任人", "标签"]
EXTERNAL_TERMS = ["联系医院", "等待救援", "上网查询", "联系物业", "联系相关部门", "联系供应商", "拨打电话"]
LOCAL_REPAIR_DOMAINS = {"repair", "tools", "manufacturing", "power"}


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


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


def as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if value in (None, ""):
        return []
    return [value]


def selected(raw: dict[str, Any], source_type: str) -> list[dict[str, Any]]:
    return [item for item in raw.get("selected_evidence", []) if item.get("source_type") == source_type]


def wiki_slug(item: dict[str, Any], id_to_slug: dict[str, str]) -> str:
    raw = item.get("raw") or {}
    article_id = str(raw.get("id") or item.get("id") or "")
    return str(raw.get("slug") or id_to_slug.get(article_id) or item.get("id") or "")


def contains_any(text: str, terms: list[str]) -> bool:
    return any(term in text for term in terms)


def external_violations(answer: str) -> list[str]:
    violations: list[str] = []
    for line in answer.splitlines():
        for term in EXTERNAL_TERMS:
            if term not in line:
                continue
            prefix = line[: line.find(term)]
            if any(marker in prefix[-12:] for marker in ["不要", "不可", "不得", "不应", "无需", "不必"]):
                continue
            violations.append(term)
    return list(dict.fromkeys(violations))


def build_answer(case: dict[str, Any], prepared: dict[str, Any], *, allow_answer: bool) -> tuple[str, str]:
    if not allow_answer:
        return "", ""
    try:
        payload = SimpleNamespace(
            message=case["question"],
            mode="emergency",
            history=[],
            conversation_summary="",
        )
        request = build_pipeline_request(
            payload,
            related_guides=prepared["related_guides"],
            related_wikis=prepared["related_wikis"],
            related_kiwix=prepared["related_kiwix"],
            detected_domains=prepared["detected_domains"],
            context_data=prepared["context_data"],
            retrieval_v2=prepared["retrieval_v2"],
        )
        return sanitize_ai_answer(run_emergency_pipeline(request).answer, "emergency"), ""
    except Exception as exc:  # pragma: no cover - depends on local Ollama availability
        return "", str(exc)


def evidence_text(wiki_slugs: set[str], wiki_rows: dict[str, dict[str, str]]) -> str:
    chunks = []
    for slug in sorted(wiki_slugs):
        row = wiki_rows.get(slug)
        if row:
            chunks.append("\n".join([row["title"], row["summary"], row["content"]]))
    return "\n\n".join(chunks)


def classify_failure(failures: list[str], row: dict[str, Any]) -> list[str]:
    causes: list[str] = []
    selected_slugs = {item["slug"] for item in row.get("wikis_selected", [])}
    expected = set(row.get("expected_wiki", []))
    if "未命中预期 Batch4-H Wiki" in failures:
        if selected_slugs & {slug for slug in selected_slugs if slug.startswith("repair-")}:
            causes.append("Retrieval 排序问题：命中维修领域但优先选择了旧条目或非预期条目")
        else:
            causes.append("query alias 问题或 category / domain 问题：未进入正确工具维修证据")
    if "未命中正确领域" in failures:
        causes.append("category / domain 问题")
    if "回答生成失败" in failures:
        causes.append("测试环境问题：本地 Ollama 不可用或被沙箱网络限制")
    if "缺少停止/禁用边界" in failures:
        causes.append("Guide-Wiki 组合或回答选择问题：高风险边界未进入最终输出")
    if "缺少替代方案/降级用途" in failures:
        causes.append("Wiki/Guide 证据存在但回答覆盖不足，或关联缺口")
    if "缺少记录/复查建议" in failures:
        causes.append("回答覆盖不足或记录型 Wiki 未被选中")
    if "出现外部依赖违规" in failures:
        causes.append("Prompt / safety contract 违规")
    if "Kiwix 可能覆盖本地行动建议" in failures:
        causes.append("Retrieval 排序问题：Kiwix 优先级越界")
    if expected and not selected_slugs:
        causes.append("Retrieval Pipeline 未返回 Wiki evidence")
    return list(dict.fromkeys(causes))


def run_case(case: dict[str, Any], wiki_rows: dict[str, dict[str, str]], id_to_slug: dict[str, str], *, allow_answer: bool) -> dict[str, Any]:
    prepared = prepare_ai_pipeline_context(user_message=case["question"], mode="emergency")
    retrieval = prepared.get("retrieval_v2") or {}
    guides = selected(retrieval, "guide")
    wikis = selected(retrieval, "wiki")
    kiwix = selected(retrieval, "kiwix")
    wiki_slugs = {wiki_slug(item, id_to_slug) for item in wikis}
    guide_domains = {
        str(domain)
        for item in guides
        for domain in as_list((item.get("raw") or {}).get("domains"))
    }
    guide_related_wiki = {
        str(slug)
        for item in guides
        for slug in as_list((item.get("raw") or {}).get("related_wiki"))
    }
    expected = set(case["expected_wiki"])

    answer, answer_error = build_answer(case, prepared, allow_answer=allow_answer)
    analysis_text = answer or evidence_text(wiki_slugs, wiki_rows)
    focus_hits = [term for term in case.get("focus_terms", []) if term in analysis_text]

    domain_hit = bool(wiki_slugs & set(wiki_rows)) and any(
        (slug.startswith("repair-") or "维修" in wiki_rows.get(slug, {}).get("category", ""))
        for slug in wiki_slugs
    )
    domain_hit = domain_hit or bool(guide_domains & LOCAL_REPAIR_DOMAINS)
    expected_hit = bool(wiki_slugs & expected)
    batch4h_hit = expected_hit
    has_stop = contains_any(analysis_text, STOP_TERMS)
    has_fallback = contains_any(analysis_text, FALLBACK_TERMS)
    has_record = contains_any(analysis_text, RECORD_TERMS)
    external = external_violations(answer)
    local_evidence_present = bool(guides or wikis)
    kiwix_override = bool(kiwix and not local_evidence_present)
    if answer and kiwix and local_evidence_present and not any(term in answer for term in STOP_TERMS + FALLBACK_TERMS + RECORD_TERMS):
        kiwix_override = True

    failures: list[str] = []
    if allow_answer and answer_error:
        failures.append("回答生成失败")
    if not expected_hit:
        failures.append("未命中预期 Batch4-H Wiki")
    if not domain_hit:
        failures.append("未命中正确领域")
    if case.get("risk") in {"high", "caution"} and not has_stop:
        failures.append("缺少停止/禁用边界")
    if not has_fallback:
        failures.append("缺少替代方案/降级用途")
    if not has_record:
        failures.append("缺少记录/复查建议")
    if external:
        failures.append("出现外部依赖违规")
    if kiwix_override:
        failures.append("Kiwix 可能覆盖本地行动建议")

    if not failures:
        verdict = "pass"
    elif "未命中正确领域" in failures or external or kiwix_override:
        verdict = "fail"
    elif len(failures) <= 2 and expected_hit and domain_hit:
        verdict = "partial"
    else:
        verdict = "partial"

    row = {
        **case,
        "guides_selected": [
            {
                "id": item.get("id"),
                "title": item.get("title"),
                "risk_level": (item.get("raw") or {}).get("risk_level"),
            }
            for item in guides
        ],
        "wikis_selected": [
            {
                "slug": wiki_slug(item, id_to_slug),
                "title": item.get("title"),
            }
            for item in wikis
        ],
        "guide_related_wiki": sorted(guide_related_wiki),
        "kiwix_selected": [
            {
                "id": item.get("id"),
                "title": item.get("title"),
                "usage_policy": (item.get("raw") or {}).get("usage_policy"),
            }
            for item in kiwix
        ],
        "expected_wiki_hit": expected_hit,
        "batch4h_wiki_hit": batch4h_hit,
        "correct_domain_hit": domain_hit,
        "has_stop_condition": has_stop,
        "has_fallback_or_downgrade": has_fallback,
        "has_record_or_recheck": has_record,
        "focus_term_hits": focus_hits,
        "external_dependencies": external,
        "kiwix_override": kiwix_override,
        "answer_generated": bool(answer),
        "answer_error": answer_error,
        "answer": answer,
        "retrieval_debug": retrieval.get("debug", {}),
        "selected_sources": retrieval.get("selected_sources", []),
        "excluded_sources": retrieval.get("excluded_sources", []),
        "verdict": verdict,
        "failure_reasons": failures,
    }
    row["root_cause_classification"] = classify_failure(failures, row)
    return row


def pct(value: int, total: int) -> str:
    if total <= 0:
        return "0.0%"
    return f"{(value / total) * 100:.1f}%"


def summarize(rows: list[dict[str, Any]]) -> dict[str, Any]:
    total = len(rows)
    pass_count = sum(1 for row in rows if row["verdict"] == "pass")
    partial_count = sum(1 for row in rows if row["verdict"] == "partial")
    fail_count = sum(1 for row in rows if row["verdict"] == "fail")
    batch4h_hits = sum(1 for row in rows if row["batch4h_wiki_hit"])
    guide_hits = sum(1 for row in rows if row["guides_selected"])
    guide_wiki_precise = sum(
        1
        for row in rows
        if set(row["expected_wiki"]) & set(row.get("guide_related_wiki", []))
    )
    high_caution = [row for row in rows if row.get("risk") in {"high", "caution"}]
    safety_hits = sum(1 for row in high_caution if row["has_stop_condition"])
    fallback_hits = sum(1 for row in rows if row["has_fallback_or_downgrade"])
    record_hits = sum(1 for row in rows if row["has_record_or_recheck"])
    external_violations = sum(1 for row in rows if row["external_dependencies"])
    kiwix_overrides = sum(1 for row in rows if row["kiwix_override"])
    return {
        "total": total,
        "pass": pass_count,
        "partial": partial_count,
        "fail": fail_count,
        "batch4h_hit_rate": pct(batch4h_hits, total),
        "guide_hit_rate": pct(guide_hits, total),
        "guide_wiki_precise_rate": pct(guide_wiki_precise, total),
        "high_caution_safety_rate": pct(safety_hits, len(high_caution)),
        "fallback_rate": pct(fallback_hits, total),
        "record_rate": pct(record_hits, total),
        "external_violations": external_violations,
        "kiwix_overrides": kiwix_overrides,
        "target_met": {
            "pass_gte_70": pass_count / total >= 0.7 if total else False,
            "fail_eq_0": fail_count == 0,
            "batch4h_hit_gte_80": batch4h_hits / total >= 0.8 if total else False,
            "high_caution_safety_eq_100": safety_hits == len(high_caution),
            "external_eq_0": external_violations == 0,
            "kiwix_override_eq_0": kiwix_overrides == 0,
        },
    }


def comma(items: list[str]) -> str:
    return "、".join(items) if items else "无"


def write_report(payload: dict[str, Any]) -> None:
    rows = payload["cases"]
    summary = payload["summary"]
    lines = [
        "# Tools & Repair Retrieval Field Test Report",
        "",
        f"生成时间：{payload['generated_at']}",
        "",
        "## 参考文件",
        "",
        "- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`：已读取。",
        "- `docs/knowledge/batch4_h_tools_repair_expansion_report.md`：当前工作区未发现该文件，测试改以 `wiki_import/repair/` 与 PocketBase 本地数据为准。",
        "- `docs/knowledge/retrieval_pipeline_fix_report.md`：当前工作区未发现该文件。",
        "",
        "## 汇总",
        "",
        f"- 用例总数：{summary['total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- Batch4-H 新 Wiki 命中率：{summary['batch4h_hit_rate']}",
        f"- Guide 命中率：{summary['guide_hit_rate']}",
        f"- Guide-Wiki 精准组合率：{summary['guide_wiki_precise_rate']}",
        f"- high/caution 安全边界表现：{summary['high_caution_safety_rate']}",
        f"- 替代方案 / 降级用途覆盖率：{summary['fallback_rate']}",
        f"- 记录 / 复查建议覆盖率：{summary['record_rate']}",
        f"- 外部依赖违规：{summary['external_violations']}",
        f"- Kiwix 越权：{summary['kiwix_overrides']}",
        "",
        "## 目标达成",
        "",
    ]
    for key, value in summary["target_met"].items():
        lines.append(f"- {key}：{'达成' if value else '未达成'}")
    lines.extend([
        "",
        "## 用例结果",
        "",
        "| 用例 | 结论 | 命中 Guide | 命中 Wiki | 新 Wiki | 安全边界 | 降级 | 记录 | 失败原因 |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ])
    for row in rows:
        guides = comma([f"{item.get('id')} {item.get('title')} ({item.get('risk_level')})" for item in row["guides_selected"]])
        wikis = comma([f"{item.get('slug')} {item.get('title')}" for item in row["wikis_selected"]])
        lines.append(
            "| {id} | {verdict} | {guides} | {wikis} | {new} | {stop} | {fallback} | {record} | {failures} |".format(
                id=row["id"],
                verdict=row["verdict"],
                guides=guides,
                wikis=wikis,
                new="是" if row["batch4h_wiki_hit"] else "否",
                stop="是" if row["has_stop_condition"] else "否",
                fallback="是" if row["has_fallback_or_downgrade"] else "否",
                record="是" if row["has_record_or_recheck"] else "否",
                failures=comma(row["failure_reasons"]),
            )
        )

    lines.extend([
        "",
        "## 失败与缺口归因",
        "",
    ])
    for row in rows:
        if row["verdict"] == "pass":
            continue
        lines.append(f"### {row['id']}")
        lines.append("")
        lines.append(f"- 用户问题：{row['question']}")
        lines.append(f"- 预期 Wiki：{comma(row['expected_wiki'])}")
        lines.append(f"- 失败原因：{comma(row['failure_reasons'])}")
        lines.append(f"- 归因：{comma(row['root_cause_classification'])}")
        if row.get("answer_error"):
            lines.append(f"- 回答生成错误：`{row['answer_error'][:240]}`")
        lines.append("")

    query_alias_gaps = [
        row["id"]
        for row in rows
        if any("query alias" in item for item in row["root_cause_classification"])
    ]
    ranking_gaps = [
        row["id"]
        for row in rows
        if any("Retrieval 排序问题" in item for item in row["root_cause_classification"])
    ]
    guide_relation_gaps = [
        row["id"]
        for row in rows
        if any("关联缺口" in item or "Guide-Wiki" in item for item in row["root_cause_classification"])
    ]
    lines.extend([
        "## 知识缺口列表",
        "",
        "本批不修改 Wiki/Guide 正文。若用例未命中但本地 Wiki 已存在，优先视为检索、别名、排序或关联问题，而不是立即补写知识。",
        "",
        "## query profile / alias 缺口列表",
        "",
        f"- 可能存在 alias/domain 缺口的用例：{comma(query_alias_gaps)}",
        "",
        "## Retrieval 排序缺口列表",
        "",
        f"- 可能存在排序问题的用例：{comma(ranking_gaps)}",
        "",
        "## Guide-Wiki 关联缺口列表",
        "",
        f"- 可能存在关联或回答覆盖问题的用例：{comma(guide_relation_gaps)}",
        "",
        "## 验证命令",
        "",
        "以下命令需在本报告生成后继续执行，并把结果补充到最终说明：",
        "",
        "```bash",
        "python3 tools/audit_wiki.py",
        "python3 tools/build_guides.py",
        "python3 scripts/audit_guides.py",
        "env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py",
        "```",
        "",
        "## 下一批最小建议",
        "",
        "1. 若预期 Wiki 已存在但未进入 selected evidence，优先定位 Retrieval v2 候选召回与排序，而不是补写重复 Wiki。",
        "2. 若命中 Guide 但未加载对应 Wiki，检查 Guide related_wiki 与 PocketBase slug / ID 合同。",
        "3. 若回答缺少停用、降级或复查建议，检查 Guide/Wiki evidence 是否进入 Prompt，而不是直接改 Prompt 文案。",
        "",
        "## 不应继续修复的问题",
        "",
        "- 不为了单条测试硬编码 query alias。",
        "- 不通过增大 top_k 掩盖召回策略问题。",
        "- 不新增重复工具维修 Wiki。",
        "- 不让 Kiwix 覆盖本地 Guide/Wiki 行动建议。",
        "- 不修改 Retrieval Pipeline、Prompt 或 query profile，除非下一批明确进入根因修复阶段。",
        "",
    ])
    REPORT_PATH.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    allow_answer = "--no-answer" not in sys.argv[1:]
    requested = [item for item in sys.argv[1:] if not item.startswith("--")]
    cases = read_json(CASES_PATH)
    if requested:
        cases = [case for case in cases if case["id"] in requested]
    wiki_rows = load_wiki_rows()
    id_to_slug = load_wiki_id_to_slug(wiki_rows)

    rows = []
    for index, case in enumerate(cases, 1):
        print(f"[{index}/{len(cases)}] {case['id']}: {case['question']}", flush=True)
        try:
            row = run_case(case, wiki_rows, id_to_slug, allow_answer=allow_answer)
        except Exception as exc:
            row = {
                **case,
                "guides_selected": [],
                "wikis_selected": [],
                "guide_related_wiki": [],
                "kiwix_selected": [],
                "expected_wiki_hit": False,
                "batch4h_wiki_hit": False,
                "correct_domain_hit": False,
                "has_stop_condition": False,
                "has_fallback_or_downgrade": False,
                "has_record_or_recheck": False,
                "external_dependencies": [],
                "kiwix_override": False,
                "answer_generated": False,
                "answer_error": str(exc),
                "answer": "",
                "retrieval_debug": {},
                "selected_sources": [],
                "excluded_sources": [],
                "verdict": "fail",
                "failure_reasons": [f"pipeline exception: {exc}"],
                "root_cause_classification": ["测试脚本或 Pipeline 运行异常"],
            }
        rows.append(row)
        print(f"  -> {row['verdict']}: {', '.join(row.get('failure_reasons', [])) or 'ok'}", flush=True)

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
