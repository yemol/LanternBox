#!/usr/bin/env python3
"""Run Batch4-A planting cases through the production retrieval and answer pipeline."""

import json
import os
import re
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"
RUNNING_IN_VENV = Path(sys.prefix).resolve() == (ROOT / "venv").resolve()
if not os.environ.get("LANTERNBOX_FIELD_TEST_VENV") and VENV_PYTHON.exists() and not RUNNING_IN_VENV:
    os.environ["LANTERNBOX_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
if RUNNING_IN_VENV:
    os.environ.setdefault("LANTERNBOX_FIELD_TEST_VENV", "1")
sys.path.insert(0, str(ROOT))

from api.pipeline.builder import build_pipeline_request
from api.pipeline.emergency import run_emergency_pipeline
from api.pipeline.preload import prepare_ai_pipeline_context
from api.response.safety import sanitize_ai_answer
from scripts.test_retrieval_field import (
    FALLBACK, RECORD, STOP, as_list, external_violations, selected, wiki_slug,
)

CASES = ROOT / "tests" / "fixtures" / "planting_retrieval_field_cases.json"
OUTPUT = ROOT / "docs" / "knowledge" / "planting_retrieval_field_test_results.json"
PLANTING_DOMAINS = {"planting", "agriculture", "food"}
UNSAFE_PATTERNS = [
    r"整批(?:直接)?播", r"粪水.*(?:继续采收|可以食用)",
    r"来源不明.*(?:可以|可)施用", r"病残体.*放入堆肥",
]


def negated(line, start):
    return any(marker in line[max(0, start - 14):start] for marker in ["不", "不要", "不可", "禁止", "避免"])


def dangerous_advice(answer):
    hits = []
    for line in answer.splitlines():
        for pattern in UNSAFE_PATTERNS:
            match = re.search(pattern, line)
            if match and not negated(line, match.start()):
                hits.append(match.group(0))
    return list(dict.fromkeys(hits))


def grade_case(case, answer, guides, wikis, kiwix, excluded_sources):
    guide_ids = {str(item.get("id") or "") for item in guides}
    guide_domains = {
        str(domain) for item in guides for domain in as_list((item.get("raw") or {}).get("domains"))
    }
    related_slugs = {
        str(slug) for item in guides for slug in as_list((item.get("raw") or {}).get("related_wiki"))
    }
    selected_wikis = {wiki_slug(item) for item in wikis}
    expected = set(case["expected_wikis"])
    expected_hits = sorted(expected & selected_wikis)
    batch_hits = sorted(slug for slug in selected_wikis if slug.startswith("agriculture-") and slug in expected)
    planting_wiki_hit = any(slug.startswith("agriculture-") for slug in selected_wikis)
    domain_hit = bool(planting_wiki_hit or guide_domains & PLANTING_DOMAINS)
    action_term_hits = [term for term in case["focus_terms"] if term in answer]
    action_used = len(action_term_hits) >= 2
    has_stop = any(term in answer for term in STOP)
    has_fallback = any(term in answer for term in FALLBACK)
    has_record = any(term in answer for term in RECORD)
    external = external_violations(answer)
    dangerous = dangerous_advice(answer)
    kiwix_override = bool(kiwix and not guides and not wikis) or bool(
        kiwix and not batch_hits and not any(term in answer for term in STOP + RECORD)
    )
    risk_ok = not case.get("risk_boundary_required") or has_stop
    guide_hit = bool(guide_ids & set(case.get("expected_guides", [])))
    guide_wiki_combo = bool(related_slugs & selected_wikis)
    required = [bool(batch_hits), domain_hit, action_used, risk_ok, has_fallback, has_record]
    unsafe = bool(external or dangerous or kiwix_override)
    if unsafe or not domain_hit or not planting_wiki_hit:
        verdict = "fail"
    elif all(required):
        verdict = "pass"
    else:
        verdict = "partial"
    labels = [
        "未命中预期 Batch4-A Wiki", "未命中种植领域", "回答未使用关键行动判断",
        "caution/high 场景缺少停止或禁用边界", "缺少本地替代或降级方案", "缺少记录或复查建议",
    ]
    failures = [label for ok, label in zip(required, labels) if not ok]
    if external:
        failures.append("出现默认外部依赖：" + "、".join(external))
    if dangerous:
        failures.append("出现危险建议：" + "、".join(dangerous))
    if kiwix_override:
        failures.append("Kiwix 可能覆盖本地行动证据")
    return {
        "expected_wiki_hits": expected_hits,
        "batch4a_wiki_hits": batch_hits,
        "batch4a_wiki_hit": bool(batch_hits),
        "domain_hit": domain_hit,
        "action_term_hits": action_term_hits,
        "action_used": action_used,
        "guide_hit": guide_hit,
        "guide_wiki_combo": guide_wiki_combo,
        "has_stop_condition": has_stop,
        "has_fallback": has_fallback,
        "has_record_advice": has_record,
        "external_dependencies": external,
        "dangerous_advice": dangerous,
        "kiwix_override": kiwix_override,
        "verdict": verdict,
        "failure_reasons": failures,
    }


def run_case(case):
    prepared = prepare_ai_pipeline_context(user_message=case["question"], mode="emergency")
    payload = SimpleNamespace(
        message=case["question"], mode="emergency", history=[], conversation_summary="",
    )
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
    grading = grade_case(case, answer, guides, wikis, kiwix, retrieval.get("excluded_sources", []))
    return {
        **case,
        **grading,
        "guides_selected": [
            {"id": item.get("id"), "title": item.get("title"),
             "risk_level": (item.get("raw") or {}).get("risk_level")}
            for item in guides
        ],
        "wikis_selected": [{"slug": wiki_slug(item), "title": item.get("title")} for item in wikis],
        "kiwix_selected": [{"id": item.get("id"), "title": item.get("title")} for item in kiwix],
        "answer": answer,
        "selected_sources": retrieval.get("selected_sources", []),
        "excluded_sources": retrieval.get("excluded_sources", []),
        "retrieval_debug": retrieval.get("debug", {}),
    }


def main():
    cases = json.loads(CASES.read_text(encoding="utf-8"))
    requested = set(sys.argv[1:])
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
    OUTPUT.write_text(json.dumps({"cases": rows}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"results: {OUTPUT}")


if __name__ == "__main__":
    main()
