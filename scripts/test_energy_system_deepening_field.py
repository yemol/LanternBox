#!/usr/bin/env python3
"""Run the frozen Energy System Deepening retrieval field cases.

This runner is read-only with respect to knowledge, guides, retrieval, and
PocketBase. It writes only the requested result and report artifacts and never
calls an LLM.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sqlite3
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv/bin/python"
if not os.environ.get("LANTERNBOX_ENERGY_DEEPENING_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_ENERGY_DEEPENING_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests/fixtures/energy_system_deepening_field_cases.json"
DEFAULT_RESULTS_PATH = ROOT / "docs/knowledge/batch15_d_energy_system_deepening_field_test_results.json"
DEFAULT_REPORT_PATH = ROOT / "docs/knowledge/batch15_d_energy_system_deepening_field_test_report.md"
PB_DB = ROOT / "pocketbase/pb_data/data.db"

ENERGY_DEEPENING_GUIDES = {f"DG-{number:04d}" for number in range(919, 926)}
ENERGY_SAFETY_GUIDES = {
    "DG-0546", "DG-0616", "DG-0620", "DG-0841", "DG-0842", "DG-0843"
}
ENERGY_MANAGEMENT_GUIDES = {
    "DG-0339", "DG-0561", "DG-0562", "DG-0563", "DG-0564", "DG-0565",
    "DG-0617", "DG-0618", "DG-0619", "DG-0844", "DG-0845", "DG-0846",
}
RECORDS_GUIDES = {f"DG-{number:04d}" for number in range(911, 919)} | {
    "DG-0295", "DG-0400"
}
FIRE_GUIDES = {
    "DG-0579", "DG-0646", "DG-0647", "DG-0648", "DG-0649", "DG-0650",
    "DG-0849", "DG-0850", "DG-0851",
}
PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
REPAIR_GUIDES = (
    {f"DG-{number:04d}" for number in range(621, 626)}
    | {f"DG-{number:04d}" for number in range(834, 841)}
    | {"DG-0382", "DG-0383", "DG-0385"}
)

CASE_OWNER_OVERRIDES: dict[tuple[str, str], set[str]] = {
    ("O03", "DG-0859"): {"communication", "energy_management"},
    ("O04", "DG-0563"): {"repair", "tools"},
    ("X-B", "DG-0667"): {"records", "repair", "risk_decision"},
}

STOP_TERMS = [
    "停止", "停用", "禁止", "不得", "不能", "不可", "不要", "隔离",
    "断开", "撤离", "停止条件", "STOP",
]
FALLBACK_TERMS = [
    "替代", "备用", "改用", "没有", "不足", "无法", "UNKNOWN", "降级",
    "纸本", "缩短", "保守", "停用", "最少关键负载",
]
RECORD_TERMS = [
    "记录", "复查", "检查", "编号", "标签", "清单", "状态", "条件", "来源",
    "样本", "趋势", "日期", "reviewer", "instrument", "runtime",
]
NEGATION_TERMS = [
    "停止", "停用", "禁止", "不得", "不能", "不可", "不要", "不应", "不允许",
    "不适用", "不与", "避免", "不提供", "隔离", "立即断开", "没有", "无", "STOP", "专业边界",
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
            "id": str(record_id),
            "slug": str(slug),
            "title": str(title),
            "category": str(category),
            "risk_level": str(risk_level),
            "content": str(content),
            "summary": str(summary),
        }
        for record_id, slug, title, category, risk_level, content, summary in rows
    }


def load_guide_ids() -> set[str]:
    ids: set[str] = set()
    for path in (ROOT / "data/guides").glob("*/*.json"):
        value = read_json(path)
        if isinstance(value, dict) and value.get("id"):
            ids.add(str(value["id"]))
    return ids


def wiki_slug(candidate: EvidenceCandidate, id_to_slug: dict[str, str]) -> str:
    raw = candidate.raw or {}
    record_id = str(raw.get("id") or candidate.id or "")
    return str(raw.get("slug") or id_to_slug.get(record_id) or candidate.id or "")


def dedupe_wikis(
    candidates: list[EvidenceCandidate], id_to_slug: dict[str, str]
) -> list[EvidenceCandidate]:
    seen: set[str] = set()
    output: list[EvidenceCandidate] = []
    for candidate in candidates:
        slug = wiki_slug(candidate, id_to_slug)
        if slug in seen:
            continue
        seen.add(slug)
        output.append(candidate)
    return output


def select_evidence(
    question: str, id_to_slug: dict[str, str]
) -> tuple[
    list[EvidenceCandidate],
    list[EvidenceCandidate],
    list[EvidenceCandidate],
    list[EvidenceCandidate],
]:
    guide_candidates = fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )
    wiki_candidates = fetch_wiki_candidates(
        SourcePlanItem(source_type="wiki", query=question, limit=8),
        user_message=question,
    )
    selected_guides = guide_candidates[:3]
    related_wikis = fetch_related_wiki_candidates(selected_guides)
    selected_wikis = dedupe_wikis([*related_wikis, *wiki_candidates], id_to_slug)[:8]
    return selected_guides, selected_wikis, guide_candidates, wiki_candidates


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
                str(raw.get("scenario") or ""),
                str(raw.get("goal") or ""),
                " ".join(str(item) for item in as_list(raw.get("steps"))),
                " ".join(str(item) for item in as_list(raw.get("check"))),
                " ".join(str(item) for item in as_list(raw.get("fallback"))),
                " ".join(str(item) for item in as_list(raw.get("stop_or_escalate"))),
                str(raw.get("notes") or ""),
            ]
        )
    for wiki in wikis:
        row = wiki_rows.get(wiki_slug(wiki, id_to_slug))
        if row:
            chunks.extend([row["title"], row["summary"], row["content"]])
        else:
            chunks.extend([str(wiki.title or ""), str(wiki.summary or ""), str(wiki.snippet or "")])
    return "\n".join(chunks)


def guide_owner(guide: EvidenceCandidate) -> set[str]:
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    category = str(raw.get("category") or "")
    domains = {str(item).lower() for item in as_list(raw.get("domains"))}
    title = str(guide.title or "")
    if guide_id in ENERGY_DEEPENING_GUIDES:
        return {"energy_deepening"}
    if guide_id in ENERGY_SAFETY_GUIDES:
        return {"energy_safety"}
    if guide_id in ENERGY_MANAGEMENT_GUIDES:
        return {"energy_management"}
    if guide_id in RECORDS_GUIDES:
        return {"records"}
    if guide_id in FIRE_GUIDES:
        return {"fire"}
    if guide_id in PRODUCTION_GUIDES:
        return {"production"}
    if guide_id in REPAIR_GUIDES:
        return {"repair", "tools"}
    if "火源" in category or "一氧化碳" in category or "火灾" in title:
        return {"fire"}
    if "信息保存" in category or "records" in domains:
        return {"records"}
    if "工具维修" in category or "维修 /" in category or "repair" in domains:
        return {"repair", "tools"}
    if "生产系统" in category or "production" in domains:
        return {"production"}
    if "能源" in category or "power" in domains:
        return {"energy_other"}
    return {"other"}


def guide_owner_for_case(case_id: str, guide: EvidenceCandidate) -> set[str]:
    return CASE_OWNER_OVERRIDES.get((case_id, str(guide.id or "")), guide_owner(guide))


def safe_line(line: str) -> bool:
    return any(term in line for term in NEGATION_TERMS)


def action_in_line(line: str, action: str) -> bool:
    if action != "接":
        return action in line
    return bool(re.search(r"接(?!头|口|触|手|近|收|替|班)", line))


def unsafe_line_hits(text: str, targets: list[str], actions: list[str]) -> list[str]:
    hits: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or safe_line(line):
            continue
        if any(target in line for target in targets) and any(action_in_line(line, action) for action in actions):
            hits.append(line)
    return list(dict.fromkeys(hits))


def safety_counters(text: str) -> dict[str, int]:
    high_voltage = unsafe_line_hits(
        text, ["高压", "high-voltage", "高电压"], ["接", "测量", "操作", "改造", "并联"]
    )
    mains = unsafe_line_hits(
        text, ["市电", "110V", "220V", "配电箱"], ["接线", "配线", "测量", "改造", "施工"]
    )
    disassembly = unsafe_line_hits(
        text, ["电池包", "锂电池包"], ["拆开", "拆包", "拆解"]
    )
    cell_replace = unsafe_line_hits(
        text, ["电芯", "换芯"], ["更换", "替换", "换掉", "点焊"]
    )
    bms = unsafe_line_hits(text, ["BMS"], ["修改", "改造", "绕过", "更换"])
    unknown_parallel = unsafe_line_hits(
        text, ["未知电源", "不同来源", "多源"], ["并联", "接在一起"]
    )
    backfeed = unsafe_line_hits(text, ["反送"], ["允许", "接入", "并联"])
    current_parallel = unsafe_line_hits(
        text, ["电流档"], ["直接并接", "并在", "并联"]
    )
    swollen_test = unsafe_line_hits(
        text, ["鼓包"], ["继续测试", "继续测", "复测", "测容量"]
    )
    all_hits = (
        high_voltage + mains + disassembly + cell_replace + bms
        + unknown_parallel + backfeed + current_parallel + swollen_test
    )
    return {
        "dangerous_suggestion_count": len(list(dict.fromkeys(all_hits))),
        "high_voltage_instruction_count": len(high_voltage),
        "mains_wiring_instruction_count": len(mains),
        "battery_pack_disassembly_instruction_count": len(disassembly),
        "battery_cell_replacement_instruction_count": len(cell_replace),
        "bms_modification_instruction_count": len(bms),
        "unknown_source_parallel_allowed_count": len(unknown_parallel),
        "backfeed_allowed_count": len(backfeed),
        "current_range_parallel_measurement_allowed_count": len(current_parallel),
        "swollen_battery_continue_test_count": len(swollen_test),
    }


def has_rule_line(text: str, targets: list[str], required: list[str] | None = None) -> bool:
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not any(target in line for target in targets):
            continue
        if required and not any(term in line for term in required):
            continue
        if safe_line(line):
            return True
    return False


def boundary_checks(required: list[str], text: str) -> dict[str, bool]:
    low_voltage = any(term in text for term in ["5V", "USB", "低压 DC", "低压直流"])
    professional = has_rule_line(
        text, ["市电", "110V", "220V", "高压 DC", "高压太阳能", "配电箱"],
        ["停止", "禁止", "不适用", "专业边界"],
    )
    battery_repair = all(
        has_rule_line(text, targets)
        for targets in [
            ["拆包", "拆电池包", "拆解"],
            ["换芯", "电芯重组"],
            ["点焊"],
            ["BMS"],
        ]
    )
    parallel_backfeed = has_rule_line(
        text, ["未知电源并联", "多源直接并联", "直接并联", "市电反送"],
        ["禁止", "不得", "不能", "停止"],
    )
    measurement = (
        "电压" in text
        and "电流" in text
        and any(term in text for term in ["串入", "串联", "串在"])
        and any(term in text for term in ["并接", "两端"])
    )
    current_parallel = has_rule_line(
        text, ["电流档不得直接并接", "电流档并接", "电流档直接并"],
        ["不得", "禁止", "停止"],
    )
    swollen = has_rule_line(
        text, ["鼓包"], ["停止", "停用", "隔离", "不得", "禁止"]
    )
    fire = has_rule_line(
        text, ["冒烟", "起火", "火花"], ["停止", "断开", "隔离", "撤离", "禁止"]
    )
    single_measurement = (
        any(term in text for term in ["单次", "一次读数", "只有一次"])
        and any(term in text for term in ["不得", "不能", "不自动", "不直接", "待复测"])
        and any(term in text for term in ["长期结论", "容量", "健康", "故障"])
    )
    disconnect_switch = all(
        term in text for term in ["断开", "检查", "单一路径", "验证", "再接负载"]
    )
    solar_safety = has_rule_line(
        text, ["破裂面板", "裸露导体", "控制器冒烟", "屋顶高处", "积水"],
        ["停止", "不适用", "隔离"],
    )
    maintenance_safety = (
        any(term in text for term in ["断电后的低风险", "断电后", "安全可达"])
        and has_rule_line(text, ["拆壳", "带电维修", "屋顶高处"], ["停止", "不包含", "不适用"])
    )
    repair_boundary = (
        "Repair" in text
        and any(term in text for term in ["修理", "维修", "替换"])
    )
    records_boundary = (
        "Records" in text
        and any(term in text for term in ["归档", "可信", "历史", "趋势", "原始"])
    )
    checks = {
        "low_voltage": low_voltage,
        "professional_boundary": professional,
        "mains_stop": professional,
        "battery_repair_prohibition": battery_repair,
        "battery_safety": swollen or has_rule_line(
            text, ["漏液", "异常热", "异味"], ["停止", "隔离", "禁止"]
        ),
        "parallel_backfeed": parallel_backfeed,
        "measurement": measurement,
        "current_range_parallel": current_parallel,
        "swollen_battery": swollen,
        "fire_stop": fire,
        "single_measurement": single_measurement,
        "disconnect_before_switch": disconnect_switch,
        "solar_safety": solar_safety,
        "maintenance_safety": maintenance_safety,
        "repair_boundary": repair_boundary,
        "records_boundary": records_boundary,
    }
    return {name: checks.get(name, False) for name in required}


def classify_root_causes(
    case: dict[str, Any],
    expected_selected: bool,
    expected_candidate: bool,
    accepted_top1: bool,
    wiki_any: bool,
    wiki_candidate_any: bool,
    precise: bool,
    boundaries_ok: bool,
    top_owners: set[str],
    dangerous: bool,
    kiwix_used: bool,
) -> list[str]:
    causes: list[str] = []
    if not expected_selected and case.get("expected_guide"):
        causes.append(
            "A. correct Guide exists but not candidate"
            if not expected_candidate
            else "B. correct Guide candidate but outside selected top3"
        )
    if expected_selected and not accepted_top1:
        causes.append("C. adjacent domain / ranking top1 competition")
    if case.get("expected_wiki_any") and not wiki_any:
        causes.append(
            "D. expected Wiki outside candidate set"
            if not wiki_candidate_any and not precise
            else "E. Wiki evidence selected-set entry gap"
        )
    if wiki_any and not precise:
        causes.append("F. Guide-Wiki evidence combination not precise")
    if not boundaries_ok:
        causes.append("G. required safety / ownership boundary evidence missing")
    if case["type"] == "observation" and "energy_deepening" in top_owners:
        causes.append("H. Energy Deepening swallowed observation domain")
    if dangerous:
        causes.append("I. dangerous instruction in selected evidence")
    if kiwix_used:
        causes.append("J. Kiwix override")
    return causes


def run_case(
    case: dict[str, Any],
    wiki_rows: dict[str, dict[str, str]],
    id_to_slug: dict[str, str],
) -> dict[str, Any]:
    question = str(case["query"])
    selected_guides, selected_wikis, guide_candidates, wiki_candidates = select_evidence(
        question, id_to_slug
    )
    selected_ids = [str(guide.id) for guide in selected_guides]
    candidate_ids = [str(guide.id) for guide in guide_candidates]
    selected_slugs = [wiki_slug(wiki, id_to_slug) for wiki in selected_wikis]
    candidate_slugs = [wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates]
    expected_guides = set(case.get("expected_guide") or [])
    allowed_guides = set(case.get("allowed_guides") or expected_guides)
    expected_wikis = set(case.get("expected_wiki_any") or [])
    expected_domains = set(case.get("expected_primary_domains") or [])
    top_id = selected_ids[0] if selected_ids else ""
    top_owners = guide_owner_for_case(str(case["id"]), selected_guides[0]) if selected_guides else {"none"}

    expected_selected = bool(expected_guides & set(selected_ids)) if expected_guides else True
    expected_candidate = bool(expected_guides & set(candidate_ids)) if expected_guides else True
    expected_top1 = bool(expected_guides and top_id in expected_guides)
    accepted_top1 = bool(
        (allowed_guides and top_id in allowed_guides)
        or expected_domains & top_owners
    )
    wiki_any = bool(expected_wikis & set(selected_slugs)) if expected_wikis else True
    wiki_full = expected_wikis <= set(selected_slugs) if expected_wikis else True
    wiki_candidate_any = bool(expected_wikis & set(candidate_slugs)) if expected_wikis else True
    precise_slugs: set[str] = set()
    for guide in selected_guides:
        if not expected_guides or str(guide.id) in expected_guides:
            precise_slugs.update(str(slug) for slug in as_list((guide.raw or {}).get("related_wiki")))
    precise = bool(expected_wikis & set(selected_slugs) & precise_slugs) if expected_wikis else True

    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    safety_counts = safety_counters(text)
    required_boundaries = [str(item) for item in case.get("required_boundaries", [])]
    boundary_results = boundary_checks(required_boundaries, text)
    boundaries_ok = all(boundary_results.values())
    dangerous = safety_counts["dangerous_suggestion_count"] > 0
    kiwix_used = any(
        item.source_type == "kiwix" for item in [*selected_guides, *selected_wikis]
    )
    safety_present = any(term in text for term in STOP_TERMS)
    fallback_present = any(term in text for term in FALLBACK_TERMS)
    record_present = any(term in text for term in RECORD_TERMS)

    deepening_primary = "energy_deepening" in top_owners
    safety_primary = "energy_safety" in top_owners or "fire" in top_owners
    management_primary = "energy_management" in top_owners
    records_primary = "records" in top_owners
    repair_primary = bool({"repair", "tools"} & top_owners)
    observation_swallowed = case["type"] == "observation" and deepening_primary
    cross_domain_reasonable = accepted_top1 and not observation_swallowed

    if dangerous:
        verdict = "fail"
        reason = "selected evidence contains an unsafe instruction not protected by a stop/prohibition context."
    elif kiwix_used:
        verdict = "fail"
        reason = "Kiwix entered local action evidence."
    elif not boundaries_ok:
        verdict = "fail"
        reason = "one or more frozen safety or adjacent-domain boundaries are absent."
    elif not safety_present or not fallback_present or not record_present:
        verdict = "fail"
        reason = "safety, fallback, or record/check evidence is incomplete."
    elif case["type"] == "observation":
        if not cross_domain_reasonable:
            verdict = "fail"
            reason = "observation top1 ownership is outside the frozen adjacent-domain boundary."
        else:
            verdict = "pass"
            reason = "the frozen adjacent domain owns top1 and required stop boundaries are present."
    elif expected_selected and accepted_top1 and wiki_any and precise:
        verdict = "pass"
        reason = "expected/allowed Guide ownership and precise Wiki evidence are present."
    elif expected_candidate or expected_selected or wiki_any:
        verdict = "partial"
        reason = "some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete."
    else:
        verdict = "fail"
        reason = "expected Guide and Wiki evidence did not enter the selected set."

    causes = classify_root_causes(
        case, expected_selected, expected_candidate, accepted_top1, wiki_any,
        wiki_candidate_any, precise, boundaries_ok, top_owners, dangerous, kiwix_used,
    )
    profiles = _matching_query_profiles(question)
    return {
        "id": case["id"],
        "type": case["type"],
        "query": question,
        "focus": case.get("focus", ""),
        "expected_guide": sorted(expected_guides),
        "allowed_guides": sorted(allowed_guides),
        "expected_wiki_any": sorted(expected_wikis),
        "expected_primary_domains": sorted(expected_domains),
        "actual_top_guides": [
            {
                "id": str(guide.id),
                "title": str(guide.title or ""),
                "owners": sorted(guide_owner_for_case(str(case["id"]), guide)),
                "risk_level": str((guide.raw or {}).get("risk_level") or ""),
            }
            for guide in selected_guides
        ],
        "actual_selected_wiki": [
            {
                "slug": wiki_slug(wiki, id_to_slug),
                "title": str(wiki.title or ""),
                "source_reason": str((wiki.raw or {}).get("source_reason") or ""),
                "linked_guide_id": str((wiki.raw or {}).get("linked_guide_id") or ""),
            }
            for wiki in selected_wikis
        ],
        "guide_candidates": [
            {"id": str(guide.id), "title": str(guide.title or "")}
            for guide in guide_candidates[:8]
        ],
        "wiki_candidates": [
            {"slug": wiki_slug(wiki, id_to_slug), "title": str(wiki.title or "")}
            for wiki in wiki_candidates[:8]
        ],
        "profiles_triggered": [
            str(profile.get("name") or profile.get("id") or "")
            for profile in profiles
        ],
        "verdict": verdict,
        "pass_partial_fail": verdict,
        "reason": reason,
        "expected_guide_hit": expected_selected,
        "expected_top1": expected_top1,
        "accepted_top1": accepted_top1,
        "wiki_any_hit": wiki_any,
        "wiki_full_hit": wiki_full,
        "guide_wiki_precise": precise,
        "energy_deepening_primary": deepening_primary,
        "safety_primary": safety_primary,
        "energy_management_primary": management_primary,
        "records_primary": records_primary,
        "repair_primary": repair_primary,
        "cross_domain_reasonable": cross_domain_reasonable,
        "dangerous_suggestion": dangerous,
        "kiwix_used": kiwix_used,
        "safety_present": safety_present,
        "fallback_present": fallback_present,
        "record_or_check_present": record_present,
        "low_voltage_boundary_correct": boundary_results.get("low_voltage", True),
        "professional_boundary_correct": all(
            boundary_results.get(name, True)
            for name in ["professional_boundary", "mains_stop"]
        ),
        "battery_repair_prohibition_correct": boundary_results.get(
            "battery_repair_prohibition", True
        ),
        "battery_safety_boundary_correct": all(
            boundary_results.get(name, True)
            for name in ["battery_safety", "battery_repair_prohibition", "swollen_battery"]
        ),
        "parallel_backfeed_prohibition_correct": boundary_results.get(
            "parallel_backfeed", True
        ),
        "measurement_boundary_correct": all(
            boundary_results.get(name, True)
            for name in ["measurement", "current_range_parallel"]
        ),
        "required_boundary_results": boundary_results,
        **safety_counts,
        "root_cause_classification": causes,
    }


def rate(numerator: int, denominator: int) -> str:
    return "n/a" if denominator == 0 else f"{numerator / denominator * 100:.1f}%"


def boundary_rate(
    cases: list[dict[str, Any]],
    fixture: list[dict[str, Any]],
    required_names: set[str],
    result_key: str,
) -> str:
    fixture_by_id = {str(case["id"]): case for case in fixture}
    relevant = [
        case for case in cases
        if required_names & set(fixture_by_id[case["id"]].get("required_boundaries", []))
    ]
    return rate(sum(bool(case[result_key]) for case in relevant), len(relevant))


def owner_rate(
    cases: list[dict[str, Any]],
    fixture: list[dict[str, Any]],
    expected_domain: str,
    result_key: str,
) -> str:
    fixture_by_id = {str(case["id"]): case for case in fixture}
    relevant = [
        case for case in cases
        if expected_domain in fixture_by_id[case["id"]].get("expected_primary_domains", [])
    ]
    return rate(sum(bool(case[result_key]) for case in relevant), len(relevant))


def summarize(cases: list[dict[str, Any]], fixture: list[dict[str, Any]]) -> dict[str, Any]:
    strict = [case for case in cases if case["type"] == "strict"]
    observation = [case for case in cases if case["type"] == "observation"]
    deepening_strict = [
        case for case, source in zip(cases, fixture)
        if case["type"] == "strict"
        and "energy_deepening" in source.get("expected_primary_domains", [])
    ]
    verdicts = Counter(case["verdict"] for case in cases)
    strict_verdicts = Counter(case["verdict"] for case in strict)
    observation_verdicts = Counter(case["verdict"] for case in observation)
    causes = Counter(cause for case in cases for cause in case["root_cause_classification"])
    safety_keys = [
        "dangerous_suggestion_count",
        "high_voltage_instruction_count",
        "mains_wiring_instruction_count",
        "battery_pack_disassembly_instruction_count",
        "battery_cell_replacement_instruction_count",
        "bms_modification_instruction_count",
        "unknown_source_parallel_allowed_count",
        "backfeed_allowed_count",
        "current_range_parallel_measurement_allowed_count",
        "swollen_battery_continue_test_count",
    ]
    summary: dict[str, Any] = {
        "total": len(cases),
        "strict_total": len(strict),
        "observation_total": len(observation),
        "pass": verdicts["pass"],
        "partial": verdicts["partial"],
        "fail": verdicts["fail"],
        "strict_pass": strict_verdicts["pass"],
        "strict_partial": strict_verdicts["partial"],
        "strict_fail": strict_verdicts["fail"],
        "observation_pass": observation_verdicts["pass"],
        "observation_partial": observation_verdicts["partial"],
        "observation_fail": observation_verdicts["fail"],
        "expected_guide_hit_rate": rate(sum(case["expected_guide_hit"] for case in strict), len(strict)),
        "expected_top1_rate": rate(sum(case["expected_top1"] for case in strict), len(strict)),
        "accepted_top1_rate": rate(sum(case["accepted_top1"] for case in strict), len(strict)),
        "wiki_any_hit_rate": rate(sum(case["wiki_any_hit"] for case in strict), len(strict)),
        "wiki_full_hit_rate": rate(sum(case["wiki_full_hit"] for case in strict), len(strict)),
        "guide_wiki_precise_rate": rate(sum(case["guide_wiki_precise"] for case in strict), len(strict)),
        "energy_deepening_primary_rate": rate(
            sum(case["energy_deepening_primary"] for case in deepening_strict),
            len(deepening_strict),
        ),
        "cross_domain_reasonable_rate": rate(
            sum(case["cross_domain_reasonable"] for case in cases), len(cases)
        ),
        "safety_ownership_rate": owner_rate(
            cases, fixture, "energy_safety", "safety_primary"
        ),
        "energy_management_ownership_rate": owner_rate(
            cases, fixture, "energy_management", "energy_management_primary"
        ),
        "records_ownership_rate": owner_rate(
            cases, fixture, "records", "records_primary"
        ),
        "repair_ownership_rate": owner_rate(
            cases, fixture, "repair", "repair_primary"
        ),
        "kiwix_override_count": sum(case["kiwix_used"] for case in cases),
        "safety_rate": rate(sum(case["safety_present"] for case in cases), len(cases)),
        "fallback_rate": rate(sum(case["fallback_present"] for case in cases), len(cases)),
        "record_check_rate": rate(
            sum(case["record_or_check_present"] for case in cases), len(cases)
        ),
        "professional_stop_boundary": boundary_rate(
            cases, fixture, {"professional_boundary", "mains_stop"},
            "professional_boundary_correct",
        ),
        "low_voltage_boundary": boundary_rate(
            cases, fixture, {"low_voltage"}, "low_voltage_boundary_correct"
        ),
        "measurement_boundary": boundary_rate(
            cases, fixture, {"measurement", "current_range_parallel"},
            "measurement_boundary_correct",
        ),
        "battery_safety_boundary": boundary_rate(
            cases, fixture,
            {"battery_safety", "battery_repair_prohibition", "swollen_battery"},
            "battery_safety_boundary_correct",
        ),
        "root_cause_counts": dict(causes),
    }
    for key in safety_keys:
        summary[key] = sum(int(case[key]) for case in cases)
    return summary


def comma(values: list[Any]) -> str:
    output = [str(item) for item in values if str(item)]
    return "、".join(output) if output else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lines = [
        f"# {results['artifact_label']} Energy System Deepening Retrieval Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. 范围",
        "",
        "使用冻结 fixture 运行本地 Guide/Wiki retrieval；不调用 LLM，不修改 Knowledge、Evidence、profile 或 Retrieval。",
        "",
        f"- fixture：{results['fixture_path']}",
        f"- results：{results['results_path']}",
        f"- report：{results['report_path']}",
        "",
        "## 2. 正式结果",
        "",
        f"- total / strict / observation：{summary['total']} / {summary['strict_total']} / {summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        f"- strict pass / partial / fail：{summary['strict_pass']} / {summary['strict_partial']} / {summary['strict_fail']}",
        f"- observation pass / partial / fail：{summary['observation_pass']} / {summary['observation_partial']} / {summary['observation_fail']}",
        "",
        "## 3. Retrieval 指标",
        "",
        f"- Expected Guide hit：{summary['expected_guide_hit_rate']}",
        f"- Expected top1 / accepted top1：{summary['expected_top1_rate']} / {summary['accepted_top1_rate']}",
        f"- Wiki any / full：{summary['wiki_any_hit_rate']} / {summary['wiki_full_hit_rate']}",
        f"- Guide-Wiki precise：{summary['guide_wiki_precise_rate']}",
        f"- Energy Deepening primary：{summary['energy_deepening_primary_rate']}",
        f"- Cross-domain reasonable：{summary['cross_domain_reasonable_rate']}",
        "",
        "## 4. Ownership",
        "",
        f"- Safety ownership：{summary['safety_ownership_rate']}",
        f"- Energy Management ownership：{summary['energy_management_ownership_rate']}",
        f"- Records ownership：{summary['records_ownership_rate']}",
        f"- Repair ownership：{summary['repair_ownership_rate']}",
        "",
        "## 5. 零容忍安全计数",
        "",
    ]
    for key in [
        "dangerous_suggestion_count",
        "high_voltage_instruction_count",
        "mains_wiring_instruction_count",
        "battery_pack_disassembly_instruction_count",
        "battery_cell_replacement_instruction_count",
        "bms_modification_instruction_count",
        "unknown_source_parallel_allowed_count",
        "backfeed_allowed_count",
        "current_range_parallel_measurement_allowed_count",
        "swollen_battery_continue_test_count",
        "kiwix_override_count",
    ]:
        lines.append(f"- {key}：{summary[key]}")
    lines += [
        "",
        "## 6. 结构与边界",
        "",
        f"- Safety / fallback / record-check：{summary['safety_rate']} / {summary['fallback_rate']} / {summary['record_check_rate']}",
        f"- Professional / low-voltage / measurement / battery：{summary['professional_stop_boundary']} / {summary['low_voltage_boundary']} / {summary['measurement_boundary']} / {summary['battery_safety_boundary']}",
        "",
        "## 7. 正式 28 条",
        "",
        "|case|type|verdict|top1|expected Guide|Wiki any|accepted top1|reason|",
        "|---|---|---|---|---|---|---|---|",
    ]
    for case in results["cases"]:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(
            f"|{case['id']}|{case['type']}|{case['verdict']}|{top}|"
            f"{case['expected_guide_hit']}|{case['wiki_any_hit']}|"
            f"{case['accepted_top1']}|{case['reason']}|"
        )
    lines += ["", "## 8. 额外 Boundary Cases", ""]
    for case in results["extra_boundary_cases"]:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(
            f"- {case['id']}：{case['verdict']}；top1={top}；"
            f"cross-domain={case['cross_domain_reasonable']}；{case['reason']}"
        )
    lines += ["", "## 9. Root Cause Classification", ""]
    if summary["root_cause_counts"]:
        for cause, count in summary["root_cause_counts"].items():
            lines.append(f"- {cause}：{count}")
    else:
        lines.append("- 无。")
    lines += [
        "",
        "## 10. Artifact Isolation",
        "",
        "Runner 只写显式 results/report 路径；默认使用 Batch15-D 独立 artifact，不覆盖 Batch15-C 冻结报告或其他历史结果。",
        "",
        "## 11. 逐条明细",
        "",
    ]
    for case in results["cases"]:
        lines += [
            f"### {case['id']}",
            "",
            f"- query：{case['query']}",
            f"- focus：{case['focus']}",
            f"- expected / allowed Guide：{comma(case['expected_guide'])} / {comma(case['allowed_guides'])}",
            f"- expected Wiki any：{comma(case['expected_wiki_any'])}",
            f"- actual top Guide：{comma([item['id'] for item in case['actual_top_guides']])}",
            f"- selected Wiki：{comma([item['slug'] for item in case['actual_selected_wiki']])}",
            f"- profiles：{comma(case['profiles_triggered'])}",
            f"- verdict：{case['verdict']} / {case['reason']}",
            f"- boundaries：{json.dumps(case['required_boundary_results'], ensure_ascii=False)}",
            f"- root cause：{comma(case['root_cause_classification'])}",
            "",
        ]
    lines += [
        "## 12. 边界声明",
        "",
        "本次运行不修改 Wiki、Guide、Guide-Wiki、profile、Retrieval、Prompt、ranking、top_k、selector、fallback、schema、PocketBase、FT-02、Core、firmware、hardware、sync protocol、.env 或 data/lanternbox.db。",
        "",
    ]
    return "\n".join(lines)


def resolve_output_path(value: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    path = path.resolve()
    if path != ROOT and ROOT not in path.parents:
        raise ValueError(f"output path must stay inside repository: {path}")
    if not path.parent.is_dir():
        raise ValueError(f"output directory does not exist: {path.parent}")
    return path


def validate_fixture(
    fixture: dict[str, Any], guide_ids: set[str], wiki_slugs: set[str]
) -> None:
    cases = fixture.get("cases") or []
    extras = fixture.get("extra_boundary_cases") or []
    if len([case for case in cases if case.get("type") == "strict"]) != 20:
        raise ValueError("fixture must contain exactly 20 strict cases")
    if len([case for case in cases if case.get("type") == "observation"]) != 8:
        raise ValueError("fixture must contain exactly 8 observation cases")
    if len(extras) != 3:
        raise ValueError("fixture must contain exactly 3 extra boundary cases")
    ids = [str(case.get("id") or "") for case in [*cases, *extras]]
    if len(ids) != len(set(ids)) or any(not value for value in ids):
        raise ValueError("fixture IDs must be non-empty and unique")
    required = {
        "id", "type", "query", "focus", "expected_guide", "allowed_guides",
        "expected_wiki_any", "expected_primary_domains", "required_boundaries",
    }
    for case in [*cases, *extras]:
        missing = sorted(required - set(case))
        if missing:
            raise ValueError(f"{case.get('id')}: missing fields {missing}")
        invalid_guides = (set(case["expected_guide"]) | set(case["allowed_guides"])) - guide_ids
        invalid_wikis = set(case["expected_wiki_any"]) - wiki_slugs
        if invalid_guides:
            raise ValueError(f"{case['id']}: invalid Guide IDs {sorted(invalid_guides)}")
        if invalid_wikis:
            raise ValueError(f"{case['id']}: invalid Wiki slugs {sorted(invalid_wikis)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--no-answer", action="store_true",
        help="Accepted explicitly; this runner never calls an LLM.",
    )
    parser.add_argument(
        "--results-path",
        default=str(DEFAULT_RESULTS_PATH.relative_to(ROOT)),
        help="Repository-relative or absolute JSON output path.",
    )
    parser.add_argument(
        "--report-path",
        default=str(DEFAULT_REPORT_PATH.relative_to(ROOT)),
        help="Repository-relative or absolute Markdown output path.",
    )
    parser.add_argument(
        "--artifact-label",
        default="Batch15-D",
        help="Label used in generated artifact metadata and report title.",
    )
    args = parser.parse_args()
    results_path = resolve_output_path(args.results_path)
    report_path = resolve_output_path(args.report_path)

    fixture = read_json(CASES_PATH)
    wiki_rows = load_wiki_rows()
    guide_ids = load_guide_ids()
    validate_fixture(fixture, guide_ids, set(wiki_rows))
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}

    formal_fixture = fixture["cases"]
    outputs: list[dict[str, Any]] = []
    for index, case in enumerate(formal_fixture, 1):
        print(f"[{index}/{len(formal_fixture)}] {case['id']}: {case['query']}")
        output = run_case(case, wiki_rows, id_to_slug)
        print(f"  -> {output['verdict']}: {output['reason']}")
        outputs.append(output)

    extra_outputs: list[dict[str, Any]] = []
    for case in fixture["extra_boundary_cases"]:
        print(f"[extra] {case['id']}: {case['query']}")
        extra_outputs.append(run_case(case, wiki_rows, id_to_slug))

    generated_at = datetime.now(timezone.utc).isoformat()
    results = {
        "artifact_label": args.artifact_label,
        "generated_at": generated_at,
        "scope": "read-only Energy System Deepening retrieval field test; no answer generation",
        "fixture_path": str(CASES_PATH.relative_to(ROOT)),
        "results_path": str(results_path.relative_to(ROOT)),
        "report_path": str(report_path.relative_to(ROOT)),
        "summary": summarize(outputs, formal_fixture),
        "cases": outputs,
        "extra_boundary_cases": extra_outputs,
    }
    results_path.write_text(
        json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    report_path.write_text(build_report(results), encoding="utf-8")
    print(json.dumps(results["summary"], ensure_ascii=False, indent=2))
    print(f"results: {results_path}")
    print(f"report: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
