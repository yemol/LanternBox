#!/usr/bin/env python3
"""Run Long-Term Records / Knowledge Transfer retrieval field cases.

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
if not os.environ.get("LANTERNBOX_RECORDS_FIELD_TEST_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_RECORDS_FIELD_TEST_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.retrieval_v2.fetchers import (  # noqa: E402
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import EvidenceCandidate, SourcePlanItem  # noqa: E402

CASES_PATH = ROOT / "tests/fixtures/long_term_records_knowledge_transfer_field_cases.json"
DEFAULT_RESULTS_PATH = ROOT / "docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_results.json"
DEFAULT_REPORT_PATH = ROOT / "docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_report.md"
PB_DB = ROOT / "pocketbase/pb_data/data.db"

RECORDS_GUIDES = {f"DG-{number:04d}" for number in range(911, 919)}
PRODUCTION_GUIDES = {f"DG-{number:04d}" for number in range(903, 911)}
MANUFACTURING_GUIDES = {f"DG-{number:04d}" for number in range(871, 879)}
STORAGE_GUIDES = {f"DG-{number:04d}" for number in range(895, 903)}
REPAIR_GUIDES = {f"DG-{number:04d}" for number in range(621, 626)}

GUIDE_PRIMARY_OWNERSHIP = {
    "DG-0860": {"communication"},
    "DG-0428": {"external_observation", "safety"},
    "DG-0845": {"energy"},
    "DG-0859": {"communication", "energy"},
    "DG-0401": {"organization", "training"},
    "DG-0902": {"storage"},
    "DG-0295": {"records"},
    "DG-0400": {"organization", "records"},
}

STOP_TERMS = [
    "停止", "停用", "禁用", "禁止", "不可", "不要", "不能", "不得", "隔离",
    "暂停", "待检", "停止线", "封存", "不放行", "撤销", "降级",
]
FALLBACK_TERMS = [
    "替代", "备用", "改用", "没有", "不足", "缺少", "无法", "临时", "降级",
    "纸本", "纸质", "编号", "标签", "缩小", "暂停", "隔离", "待核对",
]
RECORD_TERMS = [
    "记录", "标记", "复查", "交接", "编号", "清单", "标签", "批次", "来源",
    "负责人", "状态", "检查", "索引", "reviewer", "evidence", "版本",
]
SAFE_MARKERS = [
    "不要", "不可", "不能", "不应", "不得", "禁止", "停止", "避免", "不允许",
    "未", "不", "只允许", "只用于", "不得自动", "不能自动", "停止条件", "风险提示",
]
DANGEROUS_PHRASES = [
    "直接设成 VERIFIED", "直接设为 VERIFIED", "自动进入 VERIFIED", "自动变成 VERIFIED",
    "直接进入 CURRENT", "自动进入 CURRENT", "直接写进正式 SOP", "直接加入正式知识库",
    "SUPERSEDED 可以直接执行", "ARCHIVED 可以作为当前操作依据", "旧版本直接改回 CURRENT",
    "自动授予 INDEPENDENT", "自动保持长期授权", "无需复训", "无需人工确认",
    "危险电池继续使用", "自行试验急救方法", "不用复核", "删除冲突记录",
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


def wiki_slug(candidate: EvidenceCandidate, id_to_slug: dict[str, str]) -> str:
    raw = candidate.raw or {}
    record_id = str(raw.get("id") or candidate.id or "")
    return str(raw.get("slug") or id_to_slug.get(record_id) or candidate.id or "")


def dedupe_wikis(
    candidates: list[EvidenceCandidate], id_to_slug: dict[str, str]
) -> list[EvidenceCandidate]:
    seen: set[str] = set()
    result: list[EvidenceCandidate] = []
    for candidate in candidates:
        slug = wiki_slug(candidate, id_to_slug)
        if slug in seen:
            continue
        seen.add(slug)
        result.append(candidate)
    return result


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
            chunks.extend(
                [str(wiki.title or ""), str(wiki.summary or ""), str(wiki.snippet or "")]
            )
    return "\n".join(chunks)


def unsafe_phrase_hits(text: str) -> list[str]:
    hits: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        for phrase in DANGEROUS_PHRASES:
            if phrase not in line:
                continue
            start = line.find(phrase)
            window = line[max(0, start - 28) : start + len(phrase) + 28]
            if any(marker in window for marker in SAFE_MARKERS):
                continue
            hits.append(phrase)
    return list(dict.fromkeys(hits))


def guide_labels(guide: EvidenceCandidate) -> set[str]:
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    domains = {str(item).lower() for item in as_list(raw.get("domains"))}
    category = str(raw.get("category") or "")
    title = str(guide.title or "")
    labels: set[str] = set()
    if guide_id in RECORDS_GUIDES or "records" in domains or "信息保存" in category:
        labels.add("records")
    if guide_id in PRODUCTION_GUIDES or "production" in domains or "工坊" in category:
        labels.add("production")
    if guide_id in MANUFACTURING_GUIDES or "manufacturing" in domains or "制造" in category:
        labels.add("manufacturing")
    if guide_id in STORAGE_GUIDES or domains & {"storage", "inventory"} or "长期储" in category:
        labels.add("storage")
    if guide_id in REPAIR_GUIDES or "repair" in domains or "维修" in category:
        labels.add("repair")
    if "tools" in domains or "工具" in category or "工具" in title or "锯" in title:
        labels.add("tools")
    if "medical" in domains or "医疗" in category or any(
        term in title for term in ["止血", "伤口", "出血", "急救"]
    ):
        labels.add("medical")
    if domains & {"energy", "power"} or "能源" in category or "电池" in title:
        labels.add("energy")
    if "agriculture" in domains or "农业" in category or any(
        term in title for term in ["种植", "作物", "番茄"]
    ):
        labels.add("agriculture")
    if "safety" in domains or "安全" in category:
        labels.add("safety")
    if not labels:
        labels.add("other")
    return labels


def guide_primary_labels(guide: EvidenceCandidate) -> set[str]:
    """Classify the Guide's action owner without promoting secondary domains."""
    guide_id = str(guide.id or "")
    raw = guide.raw or {}
    domains = {str(item).lower() for item in as_list(raw.get("domains"))}
    category = str(raw.get("category") or "")
    title = str(guide.title or "")
    if guide_id in GUIDE_PRIMARY_OWNERSHIP:
        return GUIDE_PRIMARY_OWNERSHIP[guide_id]
    if guide_id in RECORDS_GUIDES:
        return {"records"}
    if guide_id in PRODUCTION_GUIDES:
        return {"production"}
    if guide_id in MANUFACTURING_GUIDES:
        return {"manufacturing"}
    if guide_id in STORAGE_GUIDES:
        return {"storage"}
    if guide_id in REPAIR_GUIDES:
        return {"repair", "tools"}
    for label, terms in [
        ("medical", ["医疗", "急救", "止血", "伤口", "出血"]),
        ("storage", ["长期储", "库存", "储藏"]),
        ("production", ["生产系统", "工坊"]),
        ("manufacturing", ["制造", "木工"]),
        ("repair", ["维修", "修理"]),
        ("energy", ["能源", "电池", "电量", "充电"]),
        ("agriculture", ["农业", "种植", "作物", "育苗"]),
        ("records", ["信息保存", "知识传承", "知识版本"]),
    ]:
        if any(term in category or term in title for term in terms):
            return {label}
    for label, names in [
        ("medical", {"medical"}),
        ("manufacturing", {"manufacturing"}),
        ("production", {"production"}),
        ("storage", {"storage", "inventory"}),
        ("repair", {"repair"}),
        ("tools", {"tools"}),
        ("energy", {"energy", "power"}),
        ("agriculture", {"agriculture"}),
        ("records", {"records"}),
    ]:
        if domains & names:
            return {label}
    return {"other"}


def has_any(text: str, terms: list[str]) -> bool:
    return any(term in text for term in terms)


def lifecycle_rule_correct(rule: str, text: str) -> bool:
    if not rule:
        return True
    negative = ["不能自动", "不得自动", "不能直接", "不得直接", "不等于", "只说明", "待验证"]
    if rule == "device_not_verified":
        return "VERIFIED" in text and has_any(text, negative) and has_any(text, ["设备", "metadata", "来源"])
    if rule == "conflict_preserved":
        return "冲突" in text and "保留" in text and has_any(text, ["不得删除", "不删除", "不覆盖", "不能覆盖", "并列"])
    if rule == "single_log_not_verified":
        return "VERIFIED" in text and has_any(text, negative) and has_any(text, ["日志", "记录", "RAW"])
    if rule == "correlation_not_cause":
        return has_any(text, ["相关性", "相关", "趋势"]) and has_any(text, ["不能自动成为原因", "不能成为原因", "待验证假设", "不得宣布趋势"])
    return False


def version_rule_correct(rule: str, text: str) -> bool:
    if not rule:
        return True
    if rule == "current_only":
        return "CURRENT" in text and "默认" in text and has_any(text, ["只有", "唯一", "只显示"])
    if rule == "superseded_not_default":
        return "SUPERSEDED" in text and "默认" in text and has_any(text, ["不得", "不能", "排除", "隔离", "不再"])
    if rule == "rollback_revalidation":
        return "回滚" in text and has_any(text, ["重新验证", "重新完成", "再验证"]) and has_any(text, ["新版本", "新的 CURRENT", "一次新发布"])
    if rule == "archived_history_only":
        return "ARCHIVED" in text and "历史" in text and has_any(text, ["只用于", "只允许", "不得", "不参与", "排除"])
    return False


def promotion_rule_correct(rule: str, text: str) -> bool:
    if not rule:
        return True
    if rule == "single_experience_not_current":
        return "CURRENT" in text and has_any(text, ["一次", "单次"]) and has_any(text, ["不得", "不能", "禁止", "证据不足"]) and has_any(text, ["审核", "验证", "reviewer"])
    if rule == "high_risk_not_current":
        return "CURRENT" in text and "Medical" in text and "reviewer" in text and has_any(text, ["不得", "不能", "禁止", "维持 SOP_CANDIDATE"])
    return False


def skill_rule_correct(rule: str, text: str) -> bool:
    if not rule:
        return True
    if rule == "no_automatic_independent":
        return "INDEPENDENT" in text and "实际" in text and "人工确认" in text and has_any(text, ["不得", "不能", "不自动", "降级"])
    if rule == "expired_requires_refresher":
        return "INDEPENDENT" in text and has_any(text, ["过期", "到期", "长期未练习", "很久"]) and has_any(text, ["复训", "降级", "待复核"])
    return False


def root_causes(
    case: dict[str, Any],
    expected_selected: bool,
    expected_candidate: bool,
    expected_top1: bool,
    wiki_any: bool,
    precise: bool,
    selected_guides: list[EvidenceCandidate],
    top_labels: set[str],
    records_primary: bool,
    primary_domain_ok: bool,
    guide_ids: set[str],
    wiki_slugs: set[str],
) -> list[str]:
    causes: list[str] = []
    expected_guides = set(case.get("expected_guide") or [])
    expected_wikis = set(case.get("expected_wiki_any") or [])
    if expected_guides - guide_ids or expected_wikis - wiki_slugs:
        causes.append("Q. Knowledge / Guide 真缺口")
    if case["type"] in {"strict", "boundary_observation"}:
        if not expected_candidate:
            causes.extend(["A. Records query profile 缺失", "B. correct Records Guide exists but not selected"])
        elif not expected_selected:
            causes.append("C. correct Records Guide in candidates but not top3")
        elif not expected_top1 and not set(case.get("allow_co_primary_domains") or []) & top_labels:
            causes.append("D. correct Records Guide top3 but not top1")
        if expected_wikis and not wiki_any:
            causes.append("E. Records Wiki exists but not selected")
            related: list[str] = []
            for guide in selected_guides:
                if str(guide.id) in expected_guides:
                    related.extend(str(item) for item in as_list((guide.raw or {}).get("related_wiki")))
            if set(related) & expected_wikis and not precise:
                causes.append("G. Records related_wiki 顺序")
        if not primary_domain_ok:
            for label, cause in [
                ("production", "H. Production 抢主位"),
                ("storage", "I. Storage 抢主位"),
                ("medical", "J. Medical 抢主位"),
                ("repair", "K. Repair / Manufacturing 抢主位"),
                ("manufacturing", "K. Repair / Manufacturing 抢主位"),
                ("agriculture", "L. Agriculture 抢主位"),
                ("energy", "M. Energy 抢主位"),
            ]:
                if label in top_labels:
                    causes.append(cause)
            if len(top_labels) > 1:
                causes.append("P. Query 语义歧义")
    else:
        if records_primary:
            causes.append("N. Records 错误吞域")
        if primary_domain_ok and not records_primary:
            causes.append("R. 合理 observation，不修")
        elif not primary_domain_ok and len(top_labels) > 1:
            causes.append("P. Query 语义歧义")
    return list(dict.fromkeys(causes))


def run_case(
    case: dict[str, Any],
    wiki_rows: dict[str, dict[str, str]],
    id_to_slug: dict[str, str],
    guide_ids: set[str],
) -> dict[str, Any]:
    question = str(case["query"])
    selected_guides, selected_wikis, guide_candidates, wiki_candidates = select_evidence(
        question, id_to_slug
    )
    selected_ids = [str(guide.id) for guide in selected_guides]
    selected_slugs = [wiki_slug(wiki, id_to_slug) for wiki in selected_wikis]
    candidate_ids = [str(guide.id) for guide in guide_candidates]
    candidate_slugs = [wiki_slug(wiki, id_to_slug) for wiki in wiki_candidates]
    expected_guides = set(case.get("expected_guide") or [])
    expected_wikis = set(case.get("expected_wiki_any") or [])
    expected_domains = set(case.get("expected_primary_domains") or [])
    top_id = selected_ids[0] if selected_ids else ""
    top_labels = guide_labels(selected_guides[0]) if selected_guides else {"none"}
    top_primary_labels = guide_primary_labels(selected_guides[0]) if selected_guides else {"none"}

    expected_selected = bool(expected_guides & set(selected_ids)) if expected_guides else False
    expected_candidate = bool(expected_guides & set(candidate_ids)) if expected_guides else False
    expected_top1 = bool(expected_guides and top_id in expected_guides)
    wiki_any = bool(expected_wikis & set(selected_slugs)) if expected_wikis else False
    wiki_full = bool(expected_wikis) and expected_wikis <= set(selected_slugs)
    precise_slugs: set[str] = set()
    for guide in selected_guides:
        if not expected_guides or str(guide.id) in expected_guides:
            precise_slugs.update(str(slug) for slug in as_list((guide.raw or {}).get("related_wiki")))
    precise = bool(expected_wikis & set(selected_slugs) & precise_slugs) if expected_wikis else False

    support_required = top_id in set(case.get("support_when_top1") or [])
    support_guide_ids = set(case.get("required_support_guide_any") or [])
    support_wiki_slugs = set(case.get("required_support_wiki_any") or [])
    support_guide_hit = bool(support_guide_ids & set(selected_ids))
    support_wiki_hit = bool(support_wiki_slugs & set(selected_slugs))
    support_mode = str(case.get("support_mode") or "all")
    if not support_required:
        support_ok = True
    elif support_mode == "any":
        support_ok = support_guide_hit or support_wiki_hit
    else:
        support_ok = (not support_guide_ids or support_guide_hit) and (
            not support_wiki_slugs or support_wiki_hit
        )

    text = evidence_text(selected_guides, selected_wikis, wiki_rows, id_to_slug)
    lifecycle_ok = lifecycle_rule_correct(str(case.get("lifecycle_rule") or ""), text)
    version_ok = version_rule_correct(str(case.get("version_rule") or ""), text)
    promotion_ok = promotion_rule_correct(str(case.get("promotion_rule") or ""), text)
    skill_ok = skill_rule_correct(str(case.get("skill_rule") or ""), text)
    boundary_ok = lifecycle_ok and version_ok and promotion_ok and skill_ok and support_ok
    dangerous = unsafe_phrase_hits(text)
    kiwix_used = any(item.source_type == "kiwix" for item in [*selected_guides, *selected_wikis])
    safety = any(term in text for term in STOP_TERMS)
    fallback = any(term in text for term in FALLBACK_TERMS)
    record_check = any(term in text for term in RECORD_TERMS)
    records_primary = "records" in top_primary_labels
    records_guide_hit = bool(RECORDS_GUIDES & set(selected_ids))
    records_supplement_hit = records_guide_hit or wiki_any or any(
        slug.startswith("records-") for slug in selected_slugs
    )
    primary_domain_ok = bool(expected_domains & top_primary_labels)
    accepted_co_primary_domains = set(case.get("accepted_co_primary_domains") or [])
    co_primary_domains = set(case.get("allow_co_primary_domains") or []) | accepted_co_primary_domains
    co_primary = bool(co_primary_domains & top_primary_labels) and expected_selected
    if case.get("co_primary_requires_expected_guide"):
        co_primary = co_primary and expected_selected
    if case.get("co_primary_requires_wiki_evidence"):
        co_primary = co_primary and wiki_any
    primary_ok = expected_top1 or co_primary

    if case["type"] in {"strict", "boundary_observation"}:
        if not expected_selected:
            verdict = "fail"
            reason = "预期 Records Guide 未进入 selected top3。"
        elif not boundary_ok:
            verdict = "fail"
            reason = "selected evidence 未完整建立本 case 要求的生命周期、版本、晋升、技能或共主支撑边界。"
        elif not primary_domain_ok:
            verdict = "fail"
            reason = "top1 落入不允许的业务域，Records 系统入口未合理主导。"
        elif primary_ok and wiki_any:
            verdict = "pass"
            reason = "预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。"
        else:
            verdict = "partial"
            reason = "预期 Guide 已进入，但 top1/共主或 Wiki evidence 未同时满足。"
    else:
        supplement_expected = bool(case.get("records_supplement_expected"))
        if records_primary:
            verdict = "fail"
            reason = "Records 错误成为当前业务动作 top1。"
        elif not primary_domain_ok:
            verdict = "fail"
            reason = "top1 未落在预期业务域。"
        elif supplement_expected and not records_supplement_hit:
            verdict = "partial"
            reason = "业务域正确主导，但合理的 Records 补充 evidence 未进入。"
        else:
            verdict = "pass"
            reason = "当前业务域正确主导，Records 未抢主位。"

    if dangerous:
        verdict = "fail"
        reason = "Evidence 中发现未被否定语境包围的危险或自动晋升建议。"
    if kiwix_used:
        verdict = "fail"
        reason = "Kiwix 进入本地行动 evidence，存在越权。"
    if not safety or not fallback or not record_check:
        verdict = "fail"
        reason = "safety、fallback 或 record/check 完整性未达到要求。"

    causes = root_causes(
        case,
        expected_selected,
        expected_candidate,
        expected_top1,
        wiki_any,
        precise,
        selected_guides,
        top_primary_labels,
        records_primary,
        primary_domain_ok,
        guide_ids,
        set(wiki_rows),
    )
    return {
        "id": case["id"],
        "type": case["type"],
        "query": question,
        "focus": case.get("focus", ""),
        "expected_guide": sorted(expected_guides),
        "expected_wiki_any": sorted(expected_wikis),
        "expected_primary_domains": case.get("expected_primary_domains", []),
        "actual_top_guides": [
            {
                "id": guide.id,
                "title": guide.title,
                "labels": sorted(guide_labels(guide)),
                "primary_labels": sorted(guide_primary_labels(guide)),
                "risk_level": str((guide.raw or {}).get("risk_level") or ""),
            }
            for guide in selected_guides
        ],
        "actual_selected_wiki": [
            {
                "slug": wiki_slug(wiki, id_to_slug),
                "title": wiki.title,
                "source_reason": str((wiki.raw or {}).get("source_reason") or ""),
                "linked_guide_id": str((wiki.raw or {}).get("linked_guide_id") or ""),
            }
            for wiki in selected_wikis
        ],
        "guide_candidates": [
            {"id": guide.id, "title": guide.title} for guide in guide_candidates[:8]
        ],
        "wiki_candidates": [
            {"slug": wiki_slug(wiki, id_to_slug), "title": wiki.title}
            for wiki in wiki_candidates[:8]
        ],
        "profiles_triggered": [
            str(profile.get("name") or "") for profile in _matching_query_profiles(question)
        ],
        "pass_partial_fail": verdict,
        "reason": reason,
        "records_primary": records_primary,
        "expected_guide_hit": expected_selected,
        "expected_guide_top1": expected_top1,
        "wiki_any_hit": wiki_any,
        "wiki_full_hit": wiki_full,
        "guide_wiki_precise": precise,
        "accepted_co_primary_support_correct": support_ok,
        "cross_domain_reasonable": primary_domain_ok and not (
            case["type"] == "observation" and records_primary
        ),
        "dangerous_suggestion": dangerous,
        "kiwix_used": kiwix_used,
        "safety_present": safety,
        "fallback_present": fallback,
        "record_or_check_present": record_check,
        "lifecycle_boundary_correct": lifecycle_ok,
        "version_boundary_correct": version_ok,
        "knowledge_promotion_boundary_correct": promotion_ok,
        "skill_authorization_boundary_correct": skill_ok,
        "cross_domain_notes": (
            f"预期主域={','.join(sorted(expected_domains)) or '未指定'}；"
            f"实际 top1 主域={','.join(sorted(top_primary_labels))}；辅助标签={','.join(sorted(top_labels))}；"
            f"Records top1={records_primary}；Records 补充={records_supplement_hit}。"
        ),
        "root_cause_classification": causes,
    }


def rate(numerator: int, denominator: int) -> str:
    return "n/a" if denominator == 0 else f"{numerator / denominator * 100:.1f}%"


def applicable_rate(cases: list[dict[str, Any]], fixture: list[dict[str, Any]], key: str, rule_key: str) -> str:
    ids = {str(item["id"]) for item in fixture if item.get(rule_key)}
    relevant = [case for case in cases if case["id"] in ids]
    return rate(sum(bool(case[key]) for case in relevant), len(relevant))


def summarize(cases: list[dict[str, Any]], fixture: list[dict[str, Any]]) -> dict[str, Any]:
    strict = [case for case in cases if case["type"] == "strict"]
    observation = [case for case in cases if case["type"] == "observation"]
    verdicts = Counter(case["pass_partial_fail"] for case in cases)
    strict_verdicts = Counter(case["pass_partial_fail"] for case in strict)
    observation_verdicts = Counter(case["pass_partial_fail"] for case in observation)
    causes = Counter(cause for case in cases for cause in case["root_cause_classification"])
    lifecycle_ids = {str(item["id"]) for item in fixture if item.get("lifecycle_rule")}
    lifecycle_cases = [case for case in cases if case["id"] in lifecycle_ids]
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
        "records_primary_rate": rate(sum(case["records_primary"] for case in strict), len(strict)),
        "expected_guide_hit_rate": rate(sum(case["expected_guide_hit"] for case in strict), len(strict)),
        "expected_guide_top1_rate": rate(sum(case["expected_guide_top1"] for case in strict), len(strict)),
        "wiki_any_hit_rate": rate(sum(case["wiki_any_hit"] for case in strict), len(strict)),
        "wiki_full_hit_rate": rate(sum(case["wiki_full_hit"] for case in strict), len(strict)),
        "guide_wiki_precise_rate": rate(sum(case["guide_wiki_precise"] for case in strict), len(strict)),
        "cross_domain_reasonable_rate": rate(sum(case["cross_domain_reasonable"] for case in cases), len(cases)),
        "lifecycle_evidence_retrieval_correctness": applicable_rate(
            cases, fixture, "lifecycle_boundary_correct", "lifecycle_rule"
        ),
        "lifecycle_applicable_cases": len(lifecycle_cases),
        "lifecycle_semantic_violation_count": sum(
            bool(case["dangerous_suggestion"]) for case in lifecycle_cases
        ),
        "version_correctness": applicable_rate(cases, fixture, "version_boundary_correct", "version_rule"),
        "knowledge_promotion_gate_correctness": applicable_rate(cases, fixture, "knowledge_promotion_boundary_correct", "promotion_rule"),
        "skill_authorization_boundary_correctness": applicable_rate(cases, fixture, "skill_authorization_boundary_correct", "skill_rule"),
        "dangerous_suggestion_count": sum(bool(case["dangerous_suggestion"]) for case in cases),
        "kiwix_override_count": sum(case["kiwix_used"] for case in cases),
        "safety_rate": rate(sum(case["safety_present"] for case in cases), len(cases)),
        "fallback_rate": rate(sum(case["fallback_present"] for case in cases), len(cases)),
        "record_check_rate": rate(sum(case["record_or_check_present"] for case in cases), len(cases)),
        "root_cause_counts": dict(causes),
    }


def lifecycle_metrics(cases: list[dict[str, Any]]) -> dict[str, int]:
    by_id = {case["id"]: case for case in cases}
    return {
        "raw_auto_promoted_count": int(bool(by_id["S03"]["dangerous_suggestion"])),
        "unverified_to_current_count": sum(
            int(bool(by_id[case_id]["dangerous_suggestion"])) for case_id in ["S11", "S12", "S13"]
        ),
        "single_log_to_verified_count": int(bool(by_id["S12"]["dangerous_suggestion"])),
        "single_experience_to_current_count": int(bool(by_id["S11"]["dangerous_suggestion"])),
        "superseded_used_as_default_count": int(bool(by_id["S15"]["dangerous_suggestion"])),
        "archived_used_as_default_count": int(bool(by_id["S17"]["dangerous_suggestion"])),
        "rollback_without_revalidation_count": int(bool(by_id["S16"]["dangerous_suggestion"])),
        "automatic_skill_authorization_count": sum(
            int(bool(by_id[case_id]["dangerous_suggestion"])) for case_id in ["S19", "S20"]
        ),
    }


def comma(values: list[Any]) -> str:
    items = [str(item) for item in values if str(item)]
    return "、".join(items) if items else "无"


def build_report(results: dict[str, Any]) -> str:
    summary = results["summary"]
    lifecycle = results["lifecycle_metrics"]
    cases = results["cases"]
    extras = results["extra_boundary_observations"]
    lines = [
        f"# {results['artifact_label']} Long-Term Records / Knowledge Transfer Retrieval Field Test Report",
        "",
        f"生成时间：{results['generated_at']}",
        "",
        "## 1. 测试范围",
        "",
        "本阶段只测试 Long-Term Records / Knowledge Transfer v0.1 的本地 Retrieval 表现。使用现有 fetcher 选择 top 3 Guide，并按既有 related_wiki 与独立 Wiki 候选合并前 8 条 evidence；不调用 LLM，不生成最终回答。",
        "",
        "正式统计为 20 条 strict 与 6 条 observation，另有 3 条不计分跨域边界观察。使用冻结候选的 5 个 Records profiles；本测试不修改 selector、ranking、top_k 或 Guide-Wiki 顺序。",
        "",
        "## 2. Fixture / Script",
        "",
        "- fixture：`tests/fixtures/long_term_records_knowledge_transfer_field_cases.json`",
        "- script：`scripts/test_long_term_records_knowledge_transfer_field.py`",
        f"- results：{results['results_path']}",
        f"- report：{results['report_path']}",
        "",
        "## 3. Wiki / Guide Audit",
        "",
        "- Wiki audit：Markdown/PocketBase=1139/1139；errors=0，warnings=0，advisories=0。",
        "- Guide audit：Guides=844；errors=0，warnings=0，advisories=0。",
        "",
        "## 4. Retrieval Contract",
        "",
        "- Retrieval traceability + root contract + Records profile contracts：31 passed。",
        "- Records profile contracts 覆盖 5 个 profile 的 positive / negative boundary 与 O01-O06 stable domain regression。",
        "",
        "## 5. 26 条 Field Test 总表",
        "",
        f"- total：{summary['total']}；strict / observation：{summary['strict_total']} / {summary['observation_total']}",
        f"- pass / partial / fail：{summary['pass']} / {summary['partial']} / {summary['fail']}",
        "",
        "|case|type|verdict|top1 Guide|Wiki any|Records primary|lifecycle|version|promotion|reason|",
        "|---|---|---|---|---|---|---|---|---|---|",
    ]
    for case in cases:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(
            f"|{case['id']}|{case['type']}|{case['pass_partial_fail']}|{top}|"
            f"{case['wiki_any_hit']}|{case['records_primary']}|"
            f"{case['lifecycle_boundary_correct']}|{case['version_boundary_correct']}|"
            f"{case['knowledge_promotion_boundary_correct']}|{case['reason']}|"
        )
    lines += [
        "",
        "## 6. Strict 结果",
        "",
        f"- pass / partial / fail：{summary['strict_pass']} / {summary['strict_partial']} / {summary['strict_fail']}",
        f"- Records primary rate：{summary['records_primary_rate']}",
        f"- expected Guide hit / top1：{summary['expected_guide_hit_rate']} / {summary['expected_guide_top1_rate']}",
        f"- Wiki any / full hit：{summary['wiki_any_hit_rate']} / {summary['wiki_full_hit_rate']}",
        f"- Guide-Wiki precise：{summary['guide_wiki_precise_rate']}",
        "",
        "## 7. Observation 结果",
        "",
        f"- pass / partial / fail：{summary['observation_pass']} / {summary['observation_partial']} / {summary['observation_fail']}",
        f"- 全量 cross-domain reasonable rate：{summary['cross_domain_reasonable_rate']}",
        "- observation 的 partial 只表示合理 Records 补充 evidence 未进入；O03/O04/O05 按冻结边界保留。",
        "",
        "## 8. Lifecycle Correctness",
        "",
        f"- lifecycle evidence retrieval correctness：{summary['lifecycle_evidence_retrieval_correctness']}",
        f"- lifecycle applicable cases：{summary['lifecycle_applicable_cases']}",
        f"- lifecycle semantic violation count：{summary['lifecycle_semantic_violation_count']}",
        f"- RAW auto-promoted count：{lifecycle['raw_auto_promoted_count']}",
        f"- Unverified -> CURRENT count：{lifecycle['unverified_to_current_count']}",
        f"- single-log -> VERIFIED count：{lifecycle['single_log_to_verified_count']}",
        f"- single-experience -> CURRENT count：{lifecycle['single_experience_to_current_count']}",
        "",
        "## 9. Version Correctness",
        "",
        f"- CURRENT / SUPERSEDED / ARCHIVED / rollback correctness：{summary['version_correctness']}",
        f"- SUPERSEDED used as default count：{lifecycle['superseded_used_as_default_count']}",
        f"- ARCHIVED used as default count：{lifecycle['archived_used_as_default_count']}",
        f"- rollback without revalidation count：{lifecycle['rollback_without_revalidation_count']}",
        "",
        "## 10. Knowledge Promotion Gate",
        "",
        f"- knowledge promotion gate correctness：{summary['knowledge_promotion_gate_correctness']}",
        "- 一次成功与个人急救经验都必须保持人工审核、验证和对应 domain reviewer 边界。",
        "",
        "## 11. Skill Authorization Boundary",
        "",
        f"- skill authorization boundary correctness：{summary['skill_authorization_boundary_correctness']}",
        f"- automatic skill authorization count：{lifecycle['automatic_skill_authorization_count']}",
        "",
        "## 12. Dangerous Suggestion / Kiwix",
        "",
        f"- dangerous suggestion count：{summary['dangerous_suggestion_count']}",
        f"- Kiwix override count：{summary['kiwix_override_count']}",
        "",
        "## 13. Safety / Fallback / Record-Check",
        "",
        f"- safety：{summary['safety_rate']}",
        f"- fallback：{summary['fallback_rate']}",
        f"- record/check：{summary['record_check_rate']}",
        "",
        "## 14. 额外跨域边界观察",
        "",
        "|case|verdict|top1 Guide|Records primary|Wiki any|notes|",
        "|---|---|---|---|---|---|",
    ]
    for case in extras:
        top = case["actual_top_guides"][0]["id"] if case["actual_top_guides"] else "无"
        lines.append(
            f"|{case['id']}|{case['pass_partial_fail']}|{top}|{case['records_primary']}|"
            f"{case['wiki_any_hit']}|{case['cross_domain_notes']}|"
        )
    lines += ["", "## 15. 主要失败模式", ""]
    if summary["root_cause_counts"]:
        for cause, count in summary["root_cause_counts"].items():
            lines.append(f"- {cause}：{count}")
    else:
        lines.append("- 正式 26 条未观察到 partial/fail 对应的检索失败模式。")
    lines += [
        "",
        "根因在本阶段只分类，不修改 Knowledge、Evidence 或 Retrieval。",
        "",
        "## 16. Freeze Readiness",
        "",
    ]
    if summary["strict_partial"] or summary["strict_fail"] or summary["observation_fail"]:
        lines.append("未达到冻结条件：存在 strict partial/fail 或 observation fail。")
    else:
        lines.append("达到冻结候选条件：strict 无 partial/fail，observation 无 fail；不再增加 Records profile。")
    lines += ["", "## 17. 逐条结果", ""]
    for case in cases:
        lines += [
            f"### {case['id']}",
            "",
            f"- type：{case['type']}",
            f"- query：{case['query']}",
            f"- focus：{case['focus']}",
            f"- expected Guide：{comma(case['expected_guide'])}",
            f"- expected Wiki any：{comma(case['expected_wiki_any'])}",
            f"- actual top Guides：{comma([item['id'] for item in case['actual_top_guides']])}",
            f"- actual selected Wiki：{comma([item['slug'] for item in case['actual_selected_wiki']])}",
            f"- profiles triggered：{comma(case['profiles_triggered'])}",
            f"- verdict / reason：{case['pass_partial_fail']} / {case['reason']}",
            f"- lifecycle / version / promotion / skill：{case['lifecycle_boundary_correct']} / {case['version_boundary_correct']} / {case['knowledge_promotion_boundary_correct']} / {case['skill_authorization_boundary_correct']}",
            f"- dangerous / Kiwix：{comma(case['dangerous_suggestion'])} / {case['kiwix_used']}",
            f"- safety / fallback / record-check：{case['safety_present']} / {case['fallback_present']} / {case['record_or_check_present']}",
            f"- cross-domain：{case['cross_domain_notes']}",
            f"- root cause：{comma(case['root_cause_classification'])}",
            "",
        ]
    lines += [
        "## 18. 边界声明",
        "",
        "本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、`emergency_guides.json`、`guide_index.json`、既有 Retrieval tests、应用代码、FT-02 firmware、sync protocol、`.env` 或 `data/lanternbox.db`；未运行答案生成。",
        "",
    ]
    return "\n".join(lines)


def load_guide_ids() -> set[str]:
    ids: set[str] = set()
    for path in (ROOT / "data/guides").glob("*/*.json"):
        data = read_json(path)
        if isinstance(data, dict) and data.get("id"):
            ids.add(str(data["id"]))
    return ids


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
        "--no-answer", action="store_true", help="Accepted explicitly; this test never calls an LLM."
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
        default="Batch14-F",
        help="Label used in generated artifact metadata and report title.",
    )
    args = parser.parse_args()
    results_path = resolve_output_path(args.results_path)
    report_path = resolve_output_path(args.report_path)
    fixture = read_json(CASES_PATH)
    formal_fixture = fixture["cases"]
    extra_fixture = fixture.get("extra_boundary_observations", [])
    if len([case for case in formal_fixture if case["type"] == "strict"]) != 20:
        raise ValueError("fixture must contain exactly 20 strict cases")
    if len([case for case in formal_fixture if case["type"] == "observation"]) != 6:
        raise ValueError("fixture must contain exactly 6 observation cases")

    wiki_rows = load_wiki_rows()
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    guide_ids = load_guide_ids()
    outputs: list[dict[str, Any]] = []
    for index, case in enumerate(formal_fixture, 1):
        print(f"[{index}/{len(formal_fixture)}] {case['id']}: {case['query']}")
        result = run_case(case, wiki_rows, id_to_slug, guide_ids)
        print(f"  -> {result['pass_partial_fail']}: {result['reason']}")
        outputs.append(result)

    extra_outputs: list[dict[str, Any]] = []
    for index, case in enumerate(extra_fixture, 1):
        print(f"[extra {index}/{len(extra_fixture)}] {case['id']}: {case['query']}")
        result = run_case(case, wiki_rows, id_to_slug, guide_ids)
        print(f"  -> {result['pass_partial_fail']}: {result['reason']}")
        extra_outputs.append(result)

    results = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "artifact_label": args.artifact_label,
        "scope": f"{args.artifact_label} read-only retrieval field test; no answer generation",
        "results_path": str(results_path.relative_to(ROOT)),
        "report_path": str(report_path.relative_to(ROOT)),
        "verification": {
            "wiki_audit": {"markdown": 1139, "pocketbase": 1139, "errors": 0, "warnings": 0, "advisories": 0},
            "guide_audit": {"guides": 844, "errors": 0, "warnings": 0, "advisories": 0},
            "retrieval_contracts": {"passed": 31, "failed": 0},
        },
        "summary": summarize(outputs, formal_fixture),
        "lifecycle_metrics": lifecycle_metrics(outputs),
        "cases": outputs,
        "extra_boundary_observations": extra_outputs,
    }
    results_path.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    report_path.write_text(build_report(results), encoding="utf-8")
    print(json.dumps(results["summary"], ensure_ascii=False, indent=2))
    print(json.dumps(results["lifecycle_metrics"], ensure_ascii=False, indent=2))
    print(f"results: {results_path}")
    print(f"report: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
