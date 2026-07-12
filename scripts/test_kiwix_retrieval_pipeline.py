#!/usr/bin/env python3
"""Minimal verification for Kiwix integration in Retrieval v2."""

from __future__ import annotations

import re
import sys
from html import unescape
from pathlib import Path
from urllib.parse import parse_qs, urlparse

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from api.kiwix.orchestrator import run_kiwix_lookup, run_kiwix_query
from api.pipeline.postprocess import build_ai_advice_response
from api.pipeline.schema import PipelineResult
from api.retrieval_v2.fetchers import fetch_kiwix_candidates
import api.retrieval_v2.orchestrator as retrieval_orchestrator
from api.retrieval_v2.orchestrator import _apply_usage_policy, _build_source_outputs, _ensure_explicit_kiwix_selected
from api.retrieval_v2.schemas import EvidenceCandidate, EvidenceSelection, RetrievalPlan, SelectedEvidence, SourcePlanItem
from api.routes import api_kiwix_article, api_kiwix_asset, api_kiwix_search


def assert_true(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def make_candidate(
    source_type: str,
    item_id: str,
    title: str,
    *,
    usage_policy: str = "",
    role: str = "",
    language: str = "zh",
) -> EvidenceCandidate:
    raw = {
        "source_id": item_id,
        "label": "Kiwix / ZIM" if source_type == "kiwix" else source_type,
        "usage_policy": usage_policy,
        "source_role": role,
        "role": role,
        "language": language,
        "zim_filename": "wikipedia_zh_all_maxi_2026-05.zim" if source_type == "kiwix" else "",
        "article_path": "A/Test" if source_type == "kiwix" else "",
        "snippet": f"{title} snippet",
        "relevance_score": 0.8,
    }
    return EvidenceCandidate(
        source_type=source_type,
        id=item_id,
        title=title,
        summary=f"{title} summary",
        category=role or source_type,
        tags=[usage_policy] if usage_policy else [],
        snippet=f"{title} snippet",
        raw=raw,
    )


def test_kiwix_service() -> None:
    query = "尿路感染是什么"
    context = {"core_terms": ["尿路感染"], "keywords": ["感染"], "topics": ["医学"]}
    results = run_kiwix_query(query, context) or run_kiwix_lookup(query, context)
    assert_true(isinstance(results, list), "Kiwix service should return a list")
    assert_true(bool(results), "Kiwix service should return at least one result for a medical lookup")
    first = results[0]
    assert_true(bool(first.title), "Kiwix result should include title")
    assert_true(bool(first.snippet), "Kiwix result should include snippet")
    print("PASS kiwix service callable")


def test_retrieval_fetcher() -> None:
    plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="帮我在zim查一下，尿路感染是什么？",
        keywords=["尿路感染", "医学", "感染"],
        limit=4,
    )
    candidates = fetch_kiwix_candidates(plan, user_message="帮我在zim查一下，尿路感染是什么？", core_terms=["尿路感染", "感染"])
    assert_true(candidates, "Retrieval fetcher should return Kiwix candidates")
    first = candidates[0]
    assert_true(first.source_type == "kiwix", "Kiwix candidate source_type should be kiwix")
    assert_true(first.title == "尿路感染", "Specific ZIM lookup should rank exact title before generic infection articles")
    assert_true(bool(first.raw.get("zim_filename")), "Kiwix candidate should include zim_filename")
    assert_true(bool(first.raw.get("article_path")), "Kiwix candidate should include article_path")

    redirect_plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="败血症如何判断",
        keywords=["败血症"],
        limit=4,
    )
    redirect_candidates = fetch_kiwix_candidates(
        redirect_plan,
        user_message="败血症如何判断",
        core_terms=["败血症"],
    )
    assert_true(redirect_candidates, "Redirect/title lookup should return Kiwix candidates")
    assert_true(
        redirect_candidates[0].title == "脓毒症",
        "ZIM title/redirect lookup should outrank broad full-text matches",
    )
    assert_true(
        redirect_candidates[0].raw.get("match_type") == "direct",
        "Top redirected ZIM match should be marked as direct",
    )

    mixed_script_plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="查一下zim，尿毒症如何判断",
        keywords=["尿毒症"],
        limit=4,
    )
    mixed_script_candidates = fetch_kiwix_candidates(
        mixed_script_plan,
        user_message="查一下zim，尿毒症如何判断",
        core_terms=["尿毒症"],
    )
    assert_true(mixed_script_candidates, "Explicit ZIM query with Latin UI words should return candidates")
    assert_true(
        mixed_script_candidates[0].title == "尿毒症",
        "CJK entity direct match should outrank mixed-script noise from UI words",
    )
    assert_true(
        all(".im" not in item.title.lower() for item in mixed_script_candidates),
        "Mixed-script query cleanup should not emit .im style noise candidates",
    )
    assert_true(
        all(
            "尿毒症" in " ".join([
                item.title,
                item.snippet,
                " ".join(str(term) for term in item.raw.get("matched_terms", [])),
            ])
            for item in mixed_script_candidates
        ),
        "Auxiliary probes should not return candidates detached from the primary CJK anchor",
    )

    auxiliary_probe_plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="尿毒症 诊断标准 症状 判断依据",
        keywords=["尿毒症", "诊断", "标准", "症状", "判断"],
        limit=8,
    )
    auxiliary_probe_candidates = fetch_kiwix_candidates(
        auxiliary_probe_plan,
        user_message="查一下zim，尿毒症如何判断",
        core_terms=["尿毒症", "判断", "诊断标准", "症状"],
    )
    assert_true(auxiliary_probe_candidates, "Direct primary article should remain available with auxiliary probes")
    assert_true(
        auxiliary_probe_candidates[0].title == "尿毒症",
        "Direct primary article should outrank auxiliary diagnostic-standard probes",
    )
    assert_true(
        all(item.title != "中国精神疾病分类方案与诊断标准" for item in auxiliary_probe_candidates),
        "Direct primary article should filter auxiliary probes detached from the direct anchor",
    )

    burn_plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="烧伤 急救 处理",
        keywords=["烧伤", "处理", "急救"],
        limit=8,
    )
    burn_candidates = fetch_kiwix_candidates(
        burn_plan,
        user_message="查一下zim，烧伤应该如何处理",
        core_terms=["烧伤", "处理", "急救"],
    )
    assert_true(burn_candidates, "Burn query should return Kiwix candidates")
    assert_true(
        burn_candidates[0].title == "烧伤",
        "Primary injury entity should outrank auxiliary action terms such as first aid",
    )
    assert_true(
        burn_candidates[0].raw.get("match_type") == "direct",
        "Burn primary entity should be selected through direct ZIM lookup",
    )

    wound_infection_plan = SourcePlanItem(
        source_type="kiwix",
        purpose="背景解释",
        query="伤口感染 处理",
        keywords=["伤口", "感染", "处理"],
        limit=8,
    )
    wound_infection_candidates = fetch_kiwix_candidates(
        wound_infection_plan,
        user_message="查一下zim，伤口感染应该如何处理",
        core_terms=["伤口", "感染", "处理"],
    )
    assert_true(wound_infection_candidates, "Wound infection query should return Kiwix candidates")
    assert_true(
        wound_infection_candidates[0].title == "伤口",
        "Compound CJK entity should fall back to the primary component direct article",
    )
    assert_true(
        wound_infection_candidates[0].raw.get("match_type") == "direct",
        "Compound CJK fallback should use direct ZIM lookup for the primary component",
    )
    assert_true(
        all(item.title != "抗原" for item in wound_infection_candidates),
        "Detached generic immunology entries should not be returned for wound infection",
    )
    print("PASS retrieval fetcher recalls kiwix")


def _fake_selector(user_message: str, plan: dict, candidates: list[EvidenceCandidate]) -> EvidenceSelection:
    selected: list[SelectedEvidence] = []
    seen_types: set[str] = set()

    for source_type in ("guide", "wiki", "kiwix"):
        for candidate in candidates:
            if candidate.source_type != source_type or candidate.source_type in seen_types:
                continue
            raw = candidate.raw if isinstance(candidate.raw, dict) else {}
            if raw.get("usage_policy") == "lookup_only":
                continue
            selected.append(
                SelectedEvidence(
                    source_type=candidate.source_type,
                    id=candidate.id,
                    reason=f"测试选择 {candidate.source_type}",
                )
            )
            seen_types.add(candidate.source_type)
            break

    return EvidenceSelection(selected=selected[:4], excluded=[], answer_focus=["测试 Kiwix 编排"])


def test_retrieval_orchestrator_adds_kiwix() -> None:
    original_planner = retrieval_orchestrator.build_retrieval_plan
    original_selector = retrieval_orchestrator.select_evidence_with_ai

    try:
        def planner_for_definition(_: str) -> RetrievalPlan:
            return RetrievalPlan(
                scenario_summary="解释尿路感染",
                urgency="low",
                needs=["解释医学术语"],
                core_terms=["尿路感染"],
                source_plan=[
                    SourcePlanItem(source_type="guide", purpose="行动指南", query="尿路感染", keywords=["尿路感染"], limit=4),
                    SourcePlanItem(source_type="wiki", purpose="背景知识", query="尿路感染", keywords=["尿路感染"], limit=4),
                ],
            )

        retrieval_orchestrator.build_retrieval_plan = planner_for_definition
        retrieval_orchestrator.select_evidence_with_ai = _fake_selector
        result = retrieval_orchestrator.run_retrieval_v2("尿路感染是什么？")

        assert_true(
            any(item.source_type == "kiwix" for item in result.plan.source_plan),
            "Definition/background question should add Kiwix to source_plan",
        )
        assert_true(
            result.debug.candidate_types.get("kiwix", 0) > 0,
            "Definition/background question should recall Kiwix candidates",
        )

        def selector_unavailable(*args, **kwargs) -> EvidenceSelection:
            raise RuntimeError("selector unavailable")

        retrieval_orchestrator.select_evidence_with_ai = selector_unavailable
        result = retrieval_orchestrator.run_retrieval_v2("帮我查一下zim，尿路感染是什么？")
        assert_true(
            any(item.get("source_type") == "kiwix" for item in result.selected_sources),
            "Explicit ZIM lookup should keep at least one Kiwix selected source even when selector falls back",
        )

        def planner_for_insufficient(_: str) -> RetrievalPlan:
            return RetrievalPlan(
                scenario_summary="技术问题",
                urgency="low",
                needs=["补充技术背景"],
                core_terms=["Arduino", "气压", "传感器"],
                source_plan=[
                    SourcePlanItem(source_type="guide", purpose="行动指南", query="Arduino 气压 传感器", keywords=["Arduino", "气压", "传感器"], limit=4),
                    SourcePlanItem(source_type="wiki", purpose="背景知识", query="Arduino 气压 传感器", keywords=["Arduino", "气压", "传感器"], limit=4),
                ],
            )

        retrieval_orchestrator.build_retrieval_plan = planner_for_insufficient
        result = retrieval_orchestrator.run_retrieval_v2("Arduino 传感器如何读取气压？")

        assert_true(
            any(item.source_type == "kiwix" for item in result.plan.source_plan),
            "Guide/Wiki insufficient case should add Kiwix fallback source_plan",
        )
        assert_true(
            result.debug.candidate_types.get("kiwix", 0) > 0,
            "Guide/Wiki insufficient case should recall Kiwix candidates",
        )
        print("PASS retrieval orchestrator triggers Kiwix")
    finally:
        retrieval_orchestrator.build_retrieval_plan = original_planner
        retrieval_orchestrator.select_evidence_with_ai = original_selector


def test_usage_policy_and_priority() -> dict:
    guide = make_candidate("guide", "DG-0001", "野外饮水有异味")
    decision_kiwix = make_candidate(
        "kiwix",
        "kiwix:decision",
        "饮用水",
        usage_policy="ai_retrieval_allowed",
        role="decision",
    )
    lookup_kiwix = make_candidate(
        "kiwix",
        "kiwix:lookup",
        "查阅词典",
        usage_policy="lookup_only",
        role="lookup",
    )
    fallback_kiwix = make_candidate(
        "kiwix",
        "kiwix:fallback",
        "备用百科",
        usage_policy="fallback_only",
        role="fallback",
    )
    candidates = [decision_kiwix, lookup_kiwix, fallback_kiwix, guide]
    selection = EvidenceSelection(
        selected=[
            SelectedEvidence(source_type="kiwix", id=lookup_kiwix.id, reason="测试 lookup"),
            SelectedEvidence(source_type="kiwix", id=fallback_kiwix.id, reason="测试 fallback"),
            SelectedEvidence(source_type="kiwix", id=decision_kiwix.id, reason="测试 decision"),
            SelectedEvidence(source_type="guide", id=guide.id, reason="测试 guide"),
        ],
        excluded=[],
        answer_focus=["测试"],
    )
    selected, selection = _apply_usage_policy(
        selection=selection,
        candidates=candidates,
        selected_evidence=candidates,
    )
    assert_true(selected[0].source_type == "guide", "Guide should stay ahead of Kiwix")
    assert_true(all(item.id != lookup_kiwix.id for item in selected), "lookup_only should not stay selected")
    excluded_reasons = {item.id: item.reason for item in selection.excluded}
    assert_true(excluded_reasons.get(lookup_kiwix.id) == "usage_policy_lookup_only", "lookup_only exclusion reason missing")
    assert_true(
        excluded_reasons.get(fallback_kiwix.id) == "fallback_lower_priority_than_guide_wiki",
        "fallback exclusion reason missing when guide/wiki selected",
    )
    selected_sources, excluded_sources, retrieval_decision = _build_source_outputs(
        candidates=candidates,
        selected_evidence=selected,
        selection=selection,
    )
    kiwix_cards = [item for item in selected_sources if item.get("source_type") == "kiwix"]
    assert_true(kiwix_cards, "Selected sources should include a Kiwix source card")
    assert_true(kiwix_cards[0].get("label") == "Kiwix / ZIM", "Kiwix source card label missing")
    assert_true(any(item.get("excluded_reason") == "usage_policy_lookup_only" for item in excluded_sources), "Excluded Kiwix reason missing")
    assert_true(retrieval_decision["selected_types"].get("guide") == 1, "Retrieval decision should count selected guide")
    print("PASS usage policy and guide priority")
    return {
        "selected_sources": selected_sources,
        "excluded_sources": excluded_sources,
        "retrieval_decision": retrieval_decision,
    }


def test_explicit_kiwix_prefers_local_primary() -> None:
    primary = make_candidate(
        "kiwix",
        "kiwix:burn",
        "烧伤",
        usage_policy="ai_retrieval_allowed",
        role="decision",
    )
    secondary = make_candidate(
        "kiwix",
        "kiwix:scald",
        "烫伤",
        usage_policy="ai_retrieval_allowed",
        role="decision",
    )
    candidates = [primary, secondary]
    selection = EvidenceSelection(
        selected=[
            SelectedEvidence(source_type="kiwix", id=secondary.id, reason="AI 选择了相近条目"),
        ],
        excluded=[],
        answer_focus=[],
    )
    selected, selection = _ensure_explicit_kiwix_selected(
        user_message="查一下zim，烧伤应该如何处理",
        plan=RetrievalPlan(
            scenario_summary="烧伤处理",
            core_terms=["烧伤", "处理", "急救"],
            source_plan=[SourcePlanItem(source_type="kiwix", query="烧伤 急救", keywords=["烧伤", "急救"])],
        ),
        candidates=candidates,
        selected_evidence=[secondary],
        selection=selection,
    )
    assert_true(selected and selected[-1].id == primary.id, "Explicit ZIM lookup should restore local primary Kiwix candidate")
    assert_true(
        all(item.id != secondary.id for item in selection.selected),
        "Explicit ZIM lookup should replace secondary Kiwix selection with local primary candidate",
    )
    print("PASS explicit Kiwix selection prefers local primary")


def test_api_response_shape(source_payload: dict) -> None:
    response = build_ai_advice_response(
        result=PipelineResult(mode="emergency", answer="测试回答"),
        mode="emergency",
        related_guides=[{"title": "野外饮水有异味"}],
        related_wikis=[],
        related_kiwix=[{"title": "饮用水", "source_type": "kiwix"}],
        retrieval_v2=source_payload,
    )
    assert_true(response["related_kiwix"], "AI API response should expose related_kiwix")
    assert_true(response["selected_sources"], "AI API response should expose selected_sources")
    assert_true(response["excluded_sources"], "AI API response should expose excluded_sources")
    assert_true(response["retrieval_decision"], "AI API response should expose retrieval_decision")
    print("PASS AI API response exposes Kiwix source metadata")


def test_kiwix_routes() -> None:
    search = api_kiwix_search(q="尿路感染")
    assert_true(search.get("ok") is True, "/api/kiwix/search should return ok")
    assert_true(search.get("results"), "/api/kiwix/search should return results")

    first = search["results"][0]
    article = api_kiwix_article(source=first["zim_filename"], path=first["article_path"])
    assert_true(article.get("ok") is True, "/api/kiwix/article should return ok")
    assert_true(bool(article.get("html") or article.get("text")), "/api/kiwix/article should include content")

    html = article.get("html") or ""
    match = re.search(r"/api/kiwix/(?:resource|asset)\?[^\"']+", html)
    if match:
        parsed = urlparse(unescape(match.group(0)))
        params = parse_qs(parsed.query)
        source = (params.get("source") or [""])[0]
        path = (params.get("path") or [""])[0]
        response = api_kiwix_asset(source=source, path=path)
        assert_true(response.media_type, "/api/kiwix/asset should return a response for article assets")

    print("PASS Kiwix HTTP route functions remain callable")


def main() -> None:
    print(f"LanternBox root: {ROOT}")
    test_kiwix_service()
    test_retrieval_fetcher()
    test_retrieval_orchestrator_adds_kiwix()
    source_payload = test_usage_policy_and_priority()
    test_explicit_kiwix_prefers_local_primary()
    test_api_response_shape(source_payload)
    test_kiwix_routes()
    print("ALL PASS")


if __name__ == "__main__":
    main()
