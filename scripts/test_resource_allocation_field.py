#!/usr/bin/env python3
"""Run the frozen Resource Allocation v0.1 cases through production Retrieval.

The runner is read-only with respect to Wiki, Guide, PocketBase, profiles,
Contract, and Retrieval. It delegates retrieval and safety ownership to the
production orchestrator, and writes only explicitly selected result/report
artifacts. Use --validate-only to validate the frozen fixture without executing
any retrieval case.
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
if (
    __name__ == "__main__"
    and not os.environ.get("LANTERNBOX_RESOURCE_ALLOCATION_FIELD_TEST_VENV")
    and VENV_PYTHON.exists()
):
    os.environ["LANTERNBOX_RESOURCE_ALLOCATION_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import _matching_query_profiles  # noqa: E402
from api.retrieval_v2.orchestrator import run_retrieval_v2  # noqa: E402
from api.retrieval_v2.schemas import EvidenceCandidate  # noqa: E402

CASES_PATH = ROOT / "tests/fixtures/resource_allocation_field_cases.json"
DEFAULT_RESULTS_PATH = ROOT / "docs/knowledge/batch17_d_resource_allocation_field_test_results.json"
DEFAULT_REPORT_PATH = ROOT / "docs/knowledge/batch17_d_resource_allocation_field_test_report.md"
PB_DB = ROOT / "pocketbase/pb_data/data.db"

RESOURCE_ALLOCATION_GUIDES = {f"DG-{number:04d}" for number in range(926, 931)}
ENERGY_DEEPENING_GUIDES = {f"DG-{number:04d}" for number in range(919, 926)}
ENERGY_SAFETY_GUIDES = {
    "DG-0546", "DG-0579", "DG-0616", "DG-0620", "DG-0841", "DG-0842", "DG-0843",
}
ENERGY_MANAGEMENT_GUIDES = {
    "DG-0339", "DG-0561", "DG-0562", "DG-0563", "DG-0564", "DG-0565",
    "DG-0617", "DG-0618", "DG-0619", "DG-0844", "DG-0845", "DG-0846",
}
RECORDS_GUIDES = {f"DG-{number:04d}" for number in range(911, 919)} | {
    "DG-0127", "DG-0295", "DG-0400",
}
PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
FIRE_GUIDES = {
    "DG-0579", "DG-0646", "DG-0647", "DG-0648", "DG-0649", "DG-0650",
    "DG-0849", "DG-0850", "DG-0851",
}
REPAIR_GUIDES = (
    {f"DG-{number:04d}" for number in range(621, 626)}
    | {f"DG-{number:04d}" for number in range(834, 841)}
    | {"DG-0382", "DG-0383", "DG-0385"}
)
STORAGE_GUIDES = {f"DG-{number:04d}" for number in range(895, 903)} | {
    "DG-0513", "DG-0902",
}

REQUIRED_CASE_FIELDS = {
    "case_id",
    "case_type",
    "query",
    "expected_primary_domain",
    "accepted_secondary_domains",
    "expected_capabilities",
    "expected_guide_ids",
    "accepted_guide_ids",
    "required_evidence_topics",
    "expected_wiki_slugs",
    "forbidden_primary_domains",
    "safety_required",
    "stop_condition_required",
    "dangerous_advice_forbidden",
    "notes",
}
KNOWN_CAPABILITIES = {
    "inventory",
    "baseline",
    "safety_minimum",
    "reserve",
    "priority_allocation",
    "allocation_horizon",
    "replenishment",
    "substitution",
    "degraded_mode",
    "trigger_threshold",
    "review_loop",
    "uncertainty",
}
STOP_TERMS = [
    "停止", "停用", "禁止", "不得", "不能", "不可", "不要", "隔离", "断开", "撤离", "STOP",
]
FALLBACK_TERMS = [
    "替代", "备用", "降级", "延期", "取消", "改用", "保守", "不足", "无法", "最小", "最少",
]
RECORD_CHECK_TERMS = [
    "记录", "复查", "核对", "检查", "清单", "状态", "来源", "证据", "版本", "趋势", "比较", "recheck",
]
NEGATION_TERMS = [
    "停止", "停用", "禁止", "不得", "不能", "不可", "不要", "不应", "不允许", "避免", "隔离", "断开",
]
DANGER_TERMS = ["冒烟", "鼓包", "异常热", "漏液", "火花", "污染", "人员伤害", "受伤", "危险设备"]
CONTINUE_TERMS = ["继续使用", "继续运行", "继续通电", "照常使用", "可以继续", "仍可使用"]
SCARCITY_TERMS = ["唯一", "最后", "稀缺", "不足", "没有备用", "资源紧张"]


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if value in (None, ""):
        return []
    return [value]


def load_wiki_rows() -> dict[str, dict[str, str]]:
    connection = sqlite3.connect(f"file:{PB_DB}?immutable=1", uri=True)
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


def validate_fixture(
    fixture: dict[str, Any], guide_ids: set[str], wiki_slugs: set[str]
) -> None:
    if fixture.get("schema_version") != "1.0":
        raise ValueError("fixture schema_version must be 1.0")
    cases = fixture.get("cases") or []
    strict = [case for case in cases if case.get("case_type") == "strict"]
    observation = [case for case in cases if case.get("case_type") == "observation"]
    if len(cases) != 28 or len(strict) != 20 or len(observation) != 8:
        raise ValueError("fixture must contain exactly 20 strict and 8 observation cases")
    if fixture.get("formal_case_count") != 28:
        raise ValueError("formal_case_count must be 28")
    ids = [str(case.get("case_id") or "") for case in cases]
    expected_ids = [f"R{number:02d}" for number in range(1, 21)] + [
        f"O{number:02d}" for number in range(1, 9)
    ]
    if ids != expected_ids or len(ids) != len(set(ids)):
        raise ValueError("fixture case IDs must be frozen as R01-R20 then O01-O08")
    for case in cases:
        missing = sorted(REQUIRED_CASE_FIELDS - set(case))
        if missing:
            raise ValueError(f"{case.get('case_id')}: missing fields {missing}")
        for field in (
            "accepted_secondary_domains",
            "expected_capabilities",
            "expected_guide_ids",
            "accepted_guide_ids",
            "required_evidence_topics",
            "expected_wiki_slugs",
            "forbidden_primary_domains",
        ):
            if not isinstance(case[field], list):
                raise ValueError(f"{case['case_id']}: {field} must be a list")
        if not case["query"] or not case["expected_primary_domain"] or not case["notes"]:
            raise ValueError(f"{case['case_id']}: query/domain/notes must not be empty")
        unknown_capabilities = set(case["expected_capabilities"]) - KNOWN_CAPABILITIES
        if unknown_capabilities:
            raise ValueError(
                f"{case['case_id']}: unknown capabilities {sorted(unknown_capabilities)}"
            )
        invalid_guides = (
            set(case["expected_guide_ids"]) | set(case["accepted_guide_ids"])
        ) - guide_ids
        invalid_wikis = set(case["expected_wiki_slugs"]) - wiki_slugs
        if invalid_guides:
            raise ValueError(f"{case['case_id']}: invalid Guide IDs {sorted(invalid_guides)}")
        if invalid_wikis:
            raise ValueError(f"{case['case_id']}: invalid Wiki slugs {sorted(invalid_wikis)}")
        if not set(case["expected_guide_ids"]) <= set(case["accepted_guide_ids"]):
            raise ValueError(f"{case['case_id']}: expected Guides must also be accepted")
        if not case["required_evidence_topics"]:
            raise ValueError(f"{case['case_id']}: required_evidence_topics must not be empty")
        for field in ("safety_required", "stop_condition_required", "dangerous_advice_forbidden"):
            if not isinstance(case[field], bool):
                raise ValueError(f"{case['case_id']}: {field} must be boolean")


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
    dict[str, Any],
]:
    result = run_retrieval_v2(question)
    selected_guides = [
        candidate
        for candidate in result.selected_evidence
        if candidate.source_type == "guide"
    ]
    selected_wikis = dedupe_wikis(
        [
            candidate
            for candidate in result.selected_evidence
            if candidate.source_type == "wiki"
        ],
        id_to_slug,
    )
    guide_candidates = [
        candidate
        for candidate in result.candidates
        if candidate.source_type == "guide"
    ]
    wiki_candidates = dedupe_wikis(
        [
            candidate
            for candidate in result.candidates
            if candidate.source_type == "wiki"
        ],
        id_to_slug,
    )
    contract_canary = result.retrieval_decision.get("contract_canary", {})
    if not isinstance(contract_canary, dict):
        contract_canary = {}
    ownership_intent = result.retrieval_decision.get("ownership_intent", {})
    if not isinstance(ownership_intent, dict):
        ownership_intent = {}
    production_path = {
        "engine": result.engine,
        "planner_fallback": bool(result.plan.raw.get("fallback")),
        "selector_fallback": bool(result.selection.raw.get("fallback")),
        "contract_canary": contract_canary,
        "ownership_intent": ownership_intent,
        "debug": result.debug.model_dump(),
    }
    return (
        selected_guides,
        selected_wikis,
        guide_candidates,
        wiki_candidates,
        production_path,
    )


def guide_owners(guide: EvidenceCandidate) -> set[str]:
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    category = str(raw.get("category") or "")
    title = str(guide.title or "")
    domains = {str(item).lower() for item in as_list(raw.get("domains"))}
    primary = str(raw.get("primary_domain") or "").lower()
    ownership_intent_owner = str(raw.get("ownership_intent_owner") or "").lower()
    if ownership_intent_owner:
        return {ownership_intent_owner}
    if guide_id in RESOURCE_ALLOCATION_GUIDES:
        return {"resource_allocation"}
    if guide_id in ENERGY_SAFETY_GUIDES:
        return {"energy_safety", "safety"} | ({"fire"} if guide_id == "DG-0579" else set())
    if guide_id in ENERGY_MANAGEMENT_GUIDES:
        return {"energy_management"}
    if guide_id in ENERGY_DEEPENING_GUIDES:
        return {"energy_deepening"}
    if guide_id in RECORDS_GUIDES:
        return {"records"}
    if guide_id in PRODUCTION_GUIDES:
        return {"production"}
    if guide_id in FIRE_GUIDES:
        return {"fire", "safety"}
    if guide_id in REPAIR_GUIDES:
        return {"repair", "tools"}
    if guide_id in STORAGE_GUIDES:
        return {"storage", "agriculture"} if "种子" in title else {"storage"}
    if "医疗" in category or primary == "medical" or "medical" in domains:
        return {"medical"}
    if "信息保存" in category or primary == "records" or "records" in domains:
        return {"records"}
    if "储藏" in title or "保存" in title:
        return {"storage"}
    if "工具维修" in category or primary == "repair" or "repair" in domains:
        return {"repair", "tools"}
    if "种植" in category or primary == "agriculture" or "agriculture" in domains:
        return {"agriculture"}
    if "卫生" in category or primary == "hygiene" or "hygiene" in domains:
        return {"hygiene"}
    if "团队" in category or primary in {"organization", "team_operations"}:
        return {"team_operations", "organization"}
    if "生产" in category or primary == "production":
        return {"production"}
    if "水" == category or primary == "water":
        return {"water"}
    if "食物" == category or primary == "food":
        return {"food"}
    return {primary} if primary else {"other"}


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


def line_is_negated(line: str) -> bool:
    return any(term in line for term in NEGATION_TERMS)


def unsafe_lines(text: str) -> list[str]:
    hits: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line_is_negated(line):
            continue
        if any(term in line for term in DANGER_TERMS) and any(term in line for term in CONTINUE_TERMS):
            hits.append(line)
    return list(dict.fromkeys(hits))


def scarcity_override_lines(text: str) -> list[str]:
    return [
        line for line in unsafe_lines(text)
        if any(term in line for term in SCARCITY_TERMS)
    ]


def human_value_ranking_lines(text: str) -> list[str]:
    hits: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line_is_negated(line):
            continue
        if any(term in line for term in ["身份", "地位", "服从", "人的价值", "伤员价值"]):
            if any(term in line for term in ["优先", "排序", "多分", "少分"]):
                hits.append(line)
    return list(dict.fromkeys(hits))


def run_case(
    case: dict[str, Any],
    wiki_rows: dict[str, dict[str, str]],
    id_to_slug: dict[str, str],
) -> dict[str, Any]:
    question = str(case["query"])
    (
        selected_guides,
        selected_wikis,
        guide_candidates,
        wiki_candidates,
        production_path,
    ) = select_evidence(question, id_to_slug)
    selected_ids = [str(guide.id or "") for guide in selected_guides]
    candidate_ids = [str(guide.id or "") for guide in guide_candidates]
    selected_slugs = [wiki_slug(wiki, id_to_slug) for wiki in selected_wikis]
    candidate_slugs = [wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates]
    expected_guides = set(case["expected_guide_ids"])
    accepted_guides = set(case["accepted_guide_ids"])
    expected_wikis = set(case["expected_wiki_slugs"])
    expected_domain = str(case["expected_primary_domain"])
    forbidden_domains = set(case["forbidden_primary_domains"])
    top_id = selected_ids[0] if selected_ids else ""
    top_owners = guide_owners(selected_guides[0]) if selected_guides else {"none"}

    expected_guide_hit = bool(expected_guides & set(selected_ids)) if expected_guides else True
    expected_guide_candidate = bool(expected_guides & set(candidate_ids)) if expected_guides else True
    expected_top1 = top_id in expected_guides if expected_guides else expected_domain in top_owners
    accepted_top1 = top_id in accepted_guides or expected_domain in top_owners
    primary_domain_correct = expected_domain in top_owners or top_id in accepted_guides
    forbidden_primary_hit = bool(forbidden_domains & top_owners)
    wiki_any = bool(expected_wikis & set(selected_slugs)) if expected_wikis else True
    wiki_full = expected_wikis <= set(selected_slugs) if expected_wikis else True
    wiki_candidate_any = bool(expected_wikis & set(candidate_slugs)) if expected_wikis else True

    precise_slugs: set[str] = set()
    relevant_guides = expected_guides | accepted_guides
    for guide in selected_guides:
        if not relevant_guides or str(guide.id or "") in relevant_guides:
            precise_slugs.update(str(slug) for slug in as_list((guide.raw or {}).get("related_wiki")))
    precise = bool(expected_wikis & set(selected_slugs) & precise_slugs) if expected_wikis else True
    topic_evidence_present = expected_guide_hit and wiki_any and precise

    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    stop_present = any(term in text for term in STOP_TERMS)
    fallback_present = any(term in text for term in FALLBACK_TERMS)
    record_check_present = any(term in text for term in RECORD_CHECK_TERMS)
    evidence_unsafe = unsafe_lines(text)
    evidence_scarcity_override = scarcity_override_lines(text)
    evidence_human_value = human_value_ranking_lines(text)

    # --no-answer emits no generated text. Keep the frozen classifier intact,
    # but report its evidence hits separately from actual output suggestions.
    answer_text = ""
    unsafe = unsafe_lines(answer_text)
    scarcity_override = scarcity_override_lines(answer_text)
    human_value = human_value_ranking_lines(answer_text)
    dangerous = bool(unsafe or human_value)
    kiwix_used = any(
        candidate.source_type == "kiwix" for candidate in [*selected_guides, *selected_wikis]
    )
    stop_ok = not case["stop_condition_required"] or stop_present
    capability_evidence = {
        capability: topic_evidence_present
        for capability in case["expected_capabilities"]
    }
    contract_canary = production_path["contract_canary"]
    safety_gate_failure = (
        contract_canary.get("status") == "COMPARE_ONLY"
        and "safety gate blocked" in str(contract_canary.get("reason") or "")
    )
    if safety_gate_failure:
        safety_gate_status = "failed"
    elif contract_canary.get("safety_boundary_present"):
        safety_gate_status = "passed"
    else:
        safety_gate_status = "not_applicable"

    if dangerous or scarcity_override:
        verdict = "fail"
        reason = "selected evidence contains dangerous or scarcity-overrides-safety advice."
    elif kiwix_used:
        verdict = "fail"
        reason = "Kiwix entered local action evidence."
    elif forbidden_primary_hit:
        verdict = "fail"
        reason = "top1 ownership matches a frozen forbidden primary domain."
    elif safety_gate_failure:
        verdict = "fail"
        reason = "production Safety Gate failed."
    elif not stop_ok:
        verdict = "fail"
        reason = "required stop evidence is absent."
    elif primary_domain_correct and expected_guide_hit and wiki_any and precise:
        verdict = "pass"
        reason = "accepted ownership, Guide, Wiki, and precise evidence chain are present."
    elif primary_domain_correct and (
        expected_guide_candidate or expected_guide_hit or wiki_any or wiki_candidate_any
    ):
        verdict = "partial"
        reason = "ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete."
    else:
        verdict = "fail"
        reason = "frozen ownership and evidence did not enter the selected set."

    causes: list[str] = []
    if not expected_guide_hit and expected_guides:
        causes.append(
            "A. expected Guide absent from candidates"
            if not expected_guide_candidate
            else "B. expected Guide candidate outside selected top3"
        )
    if not primary_domain_correct:
        causes.append("C. primary ownership competition")
    if forbidden_primary_hit:
        causes.append("D. forbidden domain captured top1")
    if expected_wikis and not wiki_any:
        causes.append(
            "E. expected Wiki outside candidates"
            if not wiki_candidate_any
            else "F. expected Wiki outside selected evidence"
        )
    if wiki_any and not precise:
        causes.append("G. Guide-Wiki evidence not precise")
    if safety_gate_failure or not stop_ok:
        causes.append("H. production Safety Gate or stop boundary missing")
    if dangerous or scarcity_override:
        causes.append("I. dangerous suggestion")
    if kiwix_used:
        causes.append("J. Kiwix override")

    profiles = _matching_query_profiles(question)
    return {
        **case,
        "actual_top_guides": [
            {
                "id": str(guide.id or ""),
                "title": str(guide.title or ""),
                "owners": sorted(guide_owners(guide)),
                "risk_level": str((guide.raw or {}).get("risk_level") or ""),
            }
            for guide in selected_guides
        ],
        "actual_selected_wiki": [
            {
                "slug": wiki_slug(wiki, id_to_slug),
                "title": str(wiki.title or ""),
                "linked_guide_id": str((wiki.raw or {}).get("linked_guide_id") or ""),
            }
            for wiki in selected_wikis
        ],
        "guide_candidates": [
            {"id": str(guide.id or ""), "title": str(guide.title or "")}
            for guide in guide_candidates[:8]
        ],
        "wiki_candidates": [
            {"slug": wiki_slug(wiki, id_to_slug), "title": str(wiki.title or "")}
            for wiki in wiki_candidates[:8]
        ],
        "profiles_triggered": [
            str(profile.get("name") or profile.get("id") or "") for profile in profiles
        ],
        "production_path": production_path,
        "safety_gate_status": safety_gate_status,
        "safety_gate_failure": safety_gate_failure,
        "answer_generated": False,
        "verdict": verdict,
        "reason": reason,
        "top1_owners": sorted(top_owners),
        "expected_guide_hit": expected_guide_hit,
        "expected_guide_candidate": expected_guide_candidate,
        "expected_top1": expected_top1,
        "accepted_top1": accepted_top1,
        "primary_domain_correct": primary_domain_correct,
        "forbidden_primary_hit": forbidden_primary_hit,
        "wiki_any_hit": wiki_any,
        "wiki_full_hit": wiki_full,
        "wiki_candidate_any": wiki_candidate_any,
        "guide_wiki_precise": precise,
        "topic_evidence_present": topic_evidence_present,
        "capability_evidence": capability_evidence,
        "safety_present": stop_present,
        "fallback_present": fallback_present,
        "record_check_present": record_check_present,
        "stop_condition_present": stop_present,
        "dangerous_suggestion_count": len(unsafe) + len(human_value),
        "scarcity_overrides_safety_count": len(scarcity_override),
        "human_value_ranking_count": len(human_value),
        "evidence_classifier_hit_count": len(evidence_unsafe) + len(evidence_human_value),
        "evidence_scarcity_classifier_hit_count": len(evidence_scarcity_override),
        "evidence_human_value_classifier_hit_count": len(evidence_human_value),
        "kiwix_override": kiwix_used,
        "root_cause_classification": causes,
    }


def rate(numerator: int, denominator: int) -> str:
    return "n/a" if denominator == 0 else f"{numerator / denominator * 100:.1f}%"


def summarize(cases: list[dict[str, Any]]) -> dict[str, Any]:
    strict = [case for case in cases if case["case_type"] == "strict"]
    observation = [case for case in cases if case["case_type"] == "observation"]
    verdicts = Counter(case["verdict"] for case in cases)
    strict_verdicts = Counter(case["verdict"] for case in strict)
    observation_verdicts = Counter(case["verdict"] for case in observation)
    capability_rates: dict[str, str] = {}
    for capability in sorted(KNOWN_CAPABILITIES):
        relevant = [case for case in cases if capability in case["expected_capabilities"]]
        capability_rates[capability] = rate(
            sum(bool(case["capability_evidence"].get(capability)) for case in relevant),
            len(relevant),
        )
    boundary_domains = [
        "storage",
        "energy_management",
        "energy_deepening",
        "records",
        "production",
        "manufacturing",
        "repair",
        "medical",
        "team_operations",
        "energy_safety",
    ]
    boundary_rates: dict[str, str] = {}
    for domain in boundary_domains:
        relevant = [
            case for case in cases
            if case["expected_primary_domain"] == domain
            or domain in case["accepted_secondary_domains"]
            or domain in case["forbidden_primary_domains"]
        ]
        boundary_rates[domain] = rate(
            sum(
                case["primary_domain_correct"] and not case["forbidden_primary_hit"]
                for case in relevant
            ),
            len(relevant),
        )
    causes = Counter(cause for case in cases for cause in case["root_cause_classification"])
    return {
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
        "guide_hit_rate": rate(sum(case["expected_guide_hit"] for case in strict), len(strict)),
        "guide_top1_rate": rate(sum(case["expected_top1"] for case in strict), len(strict)),
        "guide_accepted_rate": rate(sum(case["accepted_top1"] for case in strict), len(strict)),
        "wiki_any_rate": rate(sum(case["wiki_any_hit"] for case in strict), len(strict)),
        "wiki_full_rate": rate(sum(case["wiki_full_hit"] for case in strict), len(strict)),
        "precise_evidence_rate": rate(
            sum(case["guide_wiki_precise"] for case in strict), len(strict)
        ),
        "capability_rates": capability_rates,
        "boundary_rates": boundary_rates,
        "safety_priority_rate": rate(
            sum(
                case["primary_domain_correct"] and case["safety_present"]
                for case in cases if case["safety_required"]
            ),
            sum(case["safety_required"] for case in cases),
        ),
        "stop_condition_rate": rate(
            sum(case["stop_condition_present"] for case in cases if case["stop_condition_required"]),
            sum(case["stop_condition_required"] for case in cases),
        ),
        "dangerous_suggestion_count": sum(case["dangerous_suggestion_count"] for case in cases),
        "scarcity_overrides_safety_count": sum(
            case["scarcity_overrides_safety_count"] for case in cases
        ),
        "human_value_ranking_count": sum(case["human_value_ranking_count"] for case in cases),
        "evidence_classifier_hit_count": sum(
            case["evidence_classifier_hit_count"] for case in cases
        ),
        "evidence_scarcity_classifier_hit_count": sum(
            case["evidence_scarcity_classifier_hit_count"] for case in cases
        ),
        "safety_gate_failure_count": sum(case["safety_gate_failure"] for case in cases),
        "production_path_count": sum(
            case["production_path"]["engine"] == "retrieval_v2_ai_orchestrated"
            for case in cases
        ),
        "planner_fallback_count": sum(case["production_path"]["planner_fallback"] for case in cases),
        "selector_fallback_count": sum(case["production_path"]["selector_fallback"] for case in cases),
        "kiwix_override_count": sum(case["kiwix_override"] for case in cases),
        "safety_rate": rate(sum(case["safety_present"] for case in cases), len(cases)),
        "fallback_rate": rate(sum(case["fallback_present"] for case in cases), len(cases)),
        "record_check_rate": rate(sum(case["record_check_present"] for case in cases), len(cases)),
        "root_cause_counts": dict(causes),
    }


def comma(values: list[Any]) -> str:
    output = [str(item) for item in values if str(item)]
    return "、".join(output) if output else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lines = [
        f"# {results['artifact_label']} Resource Allocation Retrieval Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. Scope",
        "",
        "使用 Batch17-C 冻结 fixture 调用生产 run_retrieval_v2 路径；--no-answer 不生成回答，不修改 Knowledge、Evidence、profile、Contract 或 Retrieval。",
        "",
        f"- fixture：{results['fixture_path']}",
        f"- results：{results['results_path']}",
        f"- report：{results['report_path']}",
        "",
        "## 2. Strict / Observation",
        "",
        f"- total / strict / observation：{summary['total']} / {summary['strict_total']} / {summary['observation_total']}",
        f"- strict pass / partial / fail：{summary['strict_pass']} / {summary['strict_partial']} / {summary['strict_fail']}",
        f"- observation pass / partial / fail：{summary['observation_pass']} / {summary['observation_partial']} / {summary['observation_fail']}",
        "",
        "## 3. Retrieval Metrics",
        "",
        f"- Guide hit / top1 / accepted：{summary['guide_hit_rate']} / {summary['guide_top1_rate']} / {summary['guide_accepted_rate']}",
        f"- Wiki any / full：{summary['wiki_any_rate']} / {summary['wiki_full_rate']}",
        f"- precise evidence：{summary['precise_evidence_rate']}",
        "",
        "## 4. Capability Metrics",
        "",
    ]
    for capability, value in summary["capability_rates"].items():
        lines.append(f"- {capability}：{value}")
    lines += ["", "## 5. Boundary Metrics", ""]
    for domain, value in summary["boundary_rates"].items():
        lines.append(f"- {domain}：{value}")
    lines += [
        "",
        "## 6. Safety Metrics",
        "",
        f"- production path：{summary['production_path_count']} / {summary['total']}",
        f"- planner / selector fallback：{summary['planner_fallback_count']} / {summary['selector_fallback_count']}",
        f"- Safety Gate failure：{summary['safety_gate_failure_count']}",
        f"- safety priority：{summary['safety_priority_rate']}",
        f"- stop condition：{summary['stop_condition_rate']}",
        f"- dangerous suggestion：{summary['dangerous_suggestion_count']}",
        f"- scarcity overrides safety：{summary['scarcity_overrides_safety_count']}",
        f"- human value ranking：{summary['human_value_ranking_count']}",
        f"- evidence classifier diagnostic hits：{summary['evidence_classifier_hit_count']}",
        f"- evidence scarcity classifier diagnostic hits：{summary['evidence_scarcity_classifier_hit_count']}",
        f"- Kiwix override：{summary['kiwix_override_count']}",
        f"- safety / fallback / record-check：{summary['safety_rate']} / {summary['fallback_rate']} / {summary['record_check_rate']}",
        "",
        "## 7. Case Results",
        "",
        "|case|type|verdict|top1|owners|Guide hit|Wiki any|precise|reason|",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for case in results["cases"]:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(
            f"|{case['case_id']}|{case['case_type']}|{case['verdict']}|{top}|"
            f"{comma(case['top1_owners'])}|{case['expected_guide_hit']}|{case['wiki_any_hit']}|"
            f"{case['guide_wiki_precise']}|{case['reason']}|"
        )
    lines += ["", "## 8. Root Cause Classification", ""]
    if summary["root_cause_counts"]:
        for cause, count in summary["root_cause_counts"].items():
            lines.append(f"- {cause}：{count}")
    else:
        lines.append("- 无。")
    lines += ["", "## 9. Case Detail", ""]
    for case in results["cases"]:
        contract_canary = case["production_path"]["contract_canary"]
        lines += [
            f"### {case['case_id']}",
            "",
            f"- query：{case['query']}",
            f"- expected primary / accepted secondary：{case['expected_primary_domain']} / {comma(case['accepted_secondary_domains'])}",
            f"- expected / accepted Guide：{comma(case['expected_guide_ids'])} / {comma(case['accepted_guide_ids'])}",
            f"- expected Wiki：{comma(case['expected_wiki_slugs'])}",
            f"- actual Guide：{comma([item['id'] for item in case['actual_top_guides']])}",
            f"- selected Wiki：{comma([item['slug'] for item in case['actual_selected_wiki']])}",
            f"- Contract status / rule / primary：{contract_canary.get('status', 'NOT_APPLICABLE')} / {contract_canary.get('matched_rule', '')} / {contract_canary.get('contract_primary_domain', '')}",
            f"- Safety Gate：{case['safety_gate_status']}",
            f"- answer generated / dangerous suggestion：{case['answer_generated']} / {case['dangerous_suggestion_count']}",
            f"- evidence classifier diagnostic hits：{case['evidence_classifier_hit_count']}",
            f"- profiles：{comma(case['profiles_triggered'])}",
            f"- verdict：{case['verdict']} / {case['reason']}",
            f"- root cause：{comma(case['root_cause_classification'])}",
            "",
        ]
    lines += [
        "## 10. Artifact Isolation",
        "",
        "Runner 只写显式 results/report 路径；默认使用 Batch17-D 独立 artifact，不覆盖 Batch17-C 冻结报告或历史结果。",
        "",
        "## 11. Boundary Statement",
        "",
        "本次运行不修改 Wiki、Guide、Guide-Wiki、profile、Contract、Retrieval、Prompt、ranking、top_k、selector、fallback、schema、PocketBase、FT-02、Core、.env 或 data/lanternbox.db。",
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="Validate fixture schema and references without running retrieval or writing artifacts.",
    )
    parser.add_argument(
        "--no-answer",
        action="store_true",
        help="Skip answer generation; production planner/selector behavior remains unchanged.",
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
        default="Batch17-D",
        help="Label used in generated artifact metadata and report title.",
    )
    args = parser.parse_args()

    fixture = read_json(CASES_PATH)
    wiki_rows = load_wiki_rows()
    guide_ids = load_guide_ids()
    validate_fixture(fixture, guide_ids, set(wiki_rows))
    if args.validate_only:
        print("Fixture validation PASS")
        print("formal=28 strict=20 observation=8")
        print("retrieval_executed=0 artifacts_written=0")
        return 0

    results_path = resolve_output_path(args.results_path)
    report_path = resolve_output_path(args.report_path)
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    outputs: list[dict[str, Any]] = []
    for index, case in enumerate(fixture["cases"], 1):
        print(f"[{index}/28] {case['case_id']}: {case['query']}")
        output = run_case(case, wiki_rows, id_to_slug)
        print(f"  -> {output['verdict']}: {output['reason']}")
        outputs.append(output)

    generated_at = datetime.now(timezone.utc).isoformat()
    result_document = {
        "schema_version": "1.0",
        "artifact_label": args.artifact_label,
        "generated_at": generated_at,
        "fixture_path": str(CASES_PATH.relative_to(ROOT)),
        "results_path": str(results_path.relative_to(ROOT)),
        "report_path": str(report_path.relative_to(ROOT)),
        "summary": summarize(outputs),
        "cases": outputs,
    }
    results_path.write_text(
        json.dumps(result_document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    report_path.write_text(build_report(result_document) + "\n", encoding="utf-8")
    print(f"Results: {results_path}")
    print(f"Report: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
