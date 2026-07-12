#!/usr/bin/env python3
"""Run LanternBox Retrieval v2 field cases through retrieval and answer generation."""

import json
import os
import re
import sqlite3
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"
if not os.environ.get("LANTERNBOX_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.pipeline.builder import build_pipeline_request
from api.pipeline.emergency import run_emergency_pipeline
from api.pipeline.preload import prepare_ai_pipeline_context
from api.response.safety import sanitize_ai_answer

CASES = ROOT / "tests" / "fixtures" / "retrieval_field_cases.json"
OUTPUT = ROOT / "docs" / "knowledge" / "retrieval_field_test_results.json"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"
DOMAIN_ALIASES = {
    "water": {"water"}, "medical": {"medical"}, "power": {"power", "energy"},
    "fire": {"fire", "shelter", "security"}, "navigation": {"navigation", "maps", "evacuation"},
    "repair": {"repair", "tools", "manufacturing", "power"},
    "records": {"records", "data", "general", "organization"},
    "security": {"security", "safety", "psychology", "organization"},
}
EXTERNAL = ["联系医院", "等待救援", "上网查询", "联系物业", "联系相关部门", "联系供应商", "拨打电话"]
STOP = ["停止", "停用", "禁用", "禁止", "不可", "不要", "撤离", "隔离", "断电", "不进入", "不通电", "放弃"]
FALLBACK = ["替代", "如果没有", "没有时", "缺少", "改用", "降级", "无法", "不足时"]
RECORD = ["记录", "标记", "复查", "交接", "时间", "清点"]


def load_wiki_slug_map():
    if not PB_DB.exists():
        return {}
    connection = sqlite3.connect(f"file:{PB_DB}?mode=ro", uri=True)
    try:
        return {str(row[0]): str(row[1]) for row in connection.execute("select id, slug from wiki_articles")}
    finally:
        connection.close()


WIKI_SLUGS = load_wiki_slug_map()


def as_list(value):
    return value if isinstance(value, list) else ([] if value in (None, "") else [value])


def selected(raw, source_type):
    return [item for item in raw.get("selected_evidence", []) if item.get("source_type") == source_type]


def wiki_slug(item):
    raw = item.get("raw") or {}
    article_id = str(raw.get("id") or item.get("id") or "")
    return str(raw.get("slug") or WIKI_SLUGS.get(article_id) or article_id)


def external_violations(answer):
    violations = []
    for line in answer.splitlines():
        for term in EXTERNAL:
            if term not in line:
                continue
            prefix = line[:line.find(term)]
            if any(marker in prefix[-12:] for marker in ["不要", "不可", "不得", "不应", "无需", "不必"]):
                continue
            violations.append(term)
    return list(dict.fromkeys(violations))


def run_case(case):
    prepared = prepare_ai_pipeline_context(user_message=case["question"], mode="emergency")
    payload = SimpleNamespace(message=case["question"], mode="emergency", history=[], conversation_summary="")
    request = build_pipeline_request(
        payload,
        related_guides=prepared["related_guides"], related_wikis=prepared["related_wikis"],
        related_kiwix=prepared["related_kiwix"], detected_domains=prepared["detected_domains"],
        context_data=prepared["context_data"], retrieval_v2=prepared["retrieval_v2"],
    )
    answer = sanitize_ai_answer(run_emergency_pipeline(request).answer, "emergency")
    retrieval = prepared["retrieval_v2"]
    guides = selected(retrieval, "guide")
    wikis = selected(retrieval, "wiki")
    kiwix = selected(retrieval, "kiwix")
    guide_ids = [item.get("id", "") for item in guides]
    guide_domains = {str(d) for item in guides for d in as_list((item.get("raw") or {}).get("domains"))}
    related_slugs = {str(s) for item in guides for s in as_list((item.get("raw") or {}).get("related_wiki"))}
    wiki_slugs = {wiki_slug(item) for item in wikis}
    risks = [str((item.get("raw") or {}).get("risk_level") or "") for item in guides]
    guide_hit = bool(set(guide_ids) & set(case["guides"]))
    domain_hit = bool(guide_domains & DOMAIN_ALIASES[case["domain"]])
    wiki_hit = bool(wiki_slugs and related_slugs & wiki_slugs)
    has_stop = any(term in answer for term in STOP)
    has_fallback = any(term in answer for term in FALLBACK)
    has_record = any(term in answer for term in RECORD)
    external = external_violations(answer)
    high = any(risk in {"high", "critical"} for risk in risks)
    risk_used = (not high) or has_stop
    kiwix_override = bool(kiwix and not guides) or bool(kiwix and guides and not any(term in answer for term in STOP + RECORD))
    excluded_sources = retrieval.get("excluded_sources", [])
    semantic_kiwix_excluded = any(
        item.get("source_type") == "kiwix" and "domain_anchor_mismatch" in str(item.get("excluded_reason") or "")
        for item in excluded_sources
    )
    kiwix_expected = bool(case.get("expect_kiwix"))
    kiwix_ok = (bool(kiwix) or semantic_kiwix_excluded) and not kiwix_override if kiwix_expected else not kiwix_override
    checks = [guide_hit, domain_hit, wiki_hit, not external, risk_used, has_fallback, has_record, kiwix_ok]
    passed = sum(checks)
    verdict = "pass" if passed == len(checks) else ("partial" if passed >= 5 else "fail")
    failures = []
    labels = ["Guide 未命中预期", "领域不正确", "Wiki 未与命中 Guide 关联", "出现错误外部依赖", "高风险边界不足", "缺少替代方案", "缺少记录建议", "Kiwix 缺失或可能覆盖行动证据"]
    for ok, label in zip(checks, labels):
        if not ok:
            failures.append(label)
    return {
        **case, "guide_hit": guide_hit, "domain_hit": domain_hit, "wiki_hit": wiki_hit,
        "guides_selected": [{"id": i.get("id"), "title": i.get("title"), "risk_level": (i.get("raw") or {}).get("risk_level")} for i in guides],
        "wikis_selected": [{"slug": wiki_slug(i), "title": i.get("title")} for i in wikis],
        "guide_related_wiki": sorted(related_slugs),
        "kiwix_selected": [{"id": i.get("id"), "title": i.get("title"), "usage_policy": (i.get("raw") or {}).get("usage_policy")} for i in kiwix],
        "external_dependencies": external, "has_stop_condition": has_stop,
        "has_fallback": has_fallback, "has_record_advice": has_record,
        "kiwix_override": kiwix_override, "answer": answer,
        "selected_sources": retrieval.get("selected_sources", []),
        "excluded_sources": excluded_sources,
        "verdict": verdict, "failure_reasons": failures,
        "retrieval_debug": retrieval.get("debug", {}),
    }


def regrade_existing():
    payload = json.loads(OUTPUT.read_text(encoding="utf-8"))
    for row in payload.get("cases", []):
        for wiki in row.get("wikis_selected", []):
            wiki["slug"] = WIKI_SLUGS.get(str(wiki.get("slug") or ""), str(wiki.get("slug") or ""))
        wiki_slugs = {str(item.get("slug") or "") for item in row.get("wikis_selected", [])}
        related = set(row.get("guide_related_wiki", []))
        wiki_hit = bool(wiki_slugs and wiki_slugs & related)
        row["wiki_hit"] = wiki_hit
        failures = [item for item in row.get("failure_reasons", []) if item != "Wiki 未与命中 Guide 关联"]
        if not wiki_hit:
            failures.append("Wiki 未与命中 Guide 关联")
        row["failure_reasons"] = failures
        passed = 8 - len(failures)
        row["verdict"] = "pass" if not failures else ("partial" if passed >= 5 else "fail")
    OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"regraded: {OUTPUT}")


def main():
    if "--regrade" in sys.argv[1:]:
        regrade_existing()
        return
    cases = json.loads(CASES.read_text(encoding="utf-8"))
    requested = [item for item in sys.argv[1:] if not item.startswith("--")]
    if requested:
        cases = [case for case in cases if case["id"] in requested]
    rows = []
    for index, case in enumerate(cases, 1):
        print(f"[{index}/{len(cases)}] {case['id']}: {case['question']}", flush=True)
        try:
            row = run_case(case)
        except Exception as exc:
            row = {**case, "verdict": "fail", "failure_reasons": [f"pipeline exception: {exc}"], "answer": ""}
        rows.append(row)
        print(f"  -> {row['verdict']}: {', '.join(row.get('failure_reasons', [])) or 'ok'}", flush=True)
    if requested and OUTPUT.exists():
        existing = json.loads(OUTPUT.read_text(encoding="utf-8")).get("cases", [])
        rows = [row for row in existing if row.get("id") not in requested] + rows
    payload = {"cases": rows}
    OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"results: {OUTPUT}")


if __name__ == "__main__":
    main()
