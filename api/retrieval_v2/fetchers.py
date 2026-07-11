"""Source fetchers for Retrieval v2."""

import json
import math
import re
from pathlib import Path
from typing import Any, Iterable, List

from .policy import policy_float, policy_int, policy_map, policy_set, policy_str_list, policy_string
from .schemas import EvidenceCandidate, SourcePlanItem


ROOT = Path(__file__).resolve().parents[2]
EMERGENCY_GUIDES_FILE = ROOT / "data" / "emergency_guides.json"

def _split_search_terms(value: Any) -> List[str]:
    """Split AI-provided search terms into stable tokens.

    This is format cleanup only. Semantic selection stays in the Planner.
    """
    text = str(value or "").strip()
    if not text:
        return []

    for sep in ["、", "，", ",", "；", ";", "/", "|", "\n", "\t"]:
        text = text.replace(sep, " ")

    return [part.strip() for part in text.split(" ") if part.strip()]


def _load_json(path: Path, default):
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def _as_text(value: Any) -> str:
    if isinstance(value, list):
        return " ".join(str(item) for item in value)
    return str(value or "")


def _as_tags(value: Any) -> List[str]:
    if isinstance(value, list):
        return [str(item).strip() for item in value if str(item).strip()]

    if isinstance(value, str):
        parts = (
            value.replace("，", ",")
            .replace("、", ",")
            .replace(";", ",")
            .replace("；", ",")
            .split(",")
        )
        return [part.strip() for part in parts if part.strip()]

    return []


def _is_weak_user_term(term: str) -> bool:
    term = str(term or "").strip()
    if not term:
        return True
    if any(stop in term for stop in policy_str_list("term_filter", "weak_user_term_substrings")):
        return True
    if any(stop in term for stop in policy_str_list("term_filter", "query_stop_terms")):
        return True

    weak_chars = set(policy_string(("term_filter", "weak_user_term_chars"), ""))
    weak_count = sum(1 for char in term if char in weak_chars)
    score = min(len(term), 4) - weak_count
    if term[0] in weak_chars:
        score -= 3
    if term[-1] in weak_chars:
        score -= 3
    if weak_count == 0:
        score += 4

    return score <= 0


def _searchable_message_text(text: str) -> str:
    return re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", str(text or ""))


def _compact_match_text(text: Any) -> str:
    return re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", str(text or "")).lower()


def _is_cjk_term(term: str) -> bool:
    return bool(re.fullmatch(r"[\u4e00-\u9fff]+", str(term or "")))


def _is_valid_search_term(term: str) -> bool:
    compact = _compact_match_text(term)
    if not compact:
        return False

    has_cjk = bool(re.search(r"[\u4e00-\u9fff]", compact))
    has_latin = bool(re.search(r"[a-z]", compact))
    if has_cjk and has_latin:
        return False

    if has_latin and len(compact) < policy_int(("zim", "search_scoring", "latin_token_min_chars"), 3):
        return False

    return True


def _score_text_match(text: str, terms: Iterable[str]) -> int:
    text = str(text or "")
    score = 0
    for term in terms or []:
        term = str(term or "").strip()
        if term and term in text:
            score += 1
    return score


def _char_ngrams(text: str, sizes: tuple[int, ...] = (2, 3, 4), limit: int = 48) -> List[str]:
    text = _searchable_message_text(text)
    terms: List[str] = []

    for size in sizes:
        if len(text) < size:
            continue
        for index in range(0, len(text) - size + 1):
            term = text[index:index + size]
            if not _is_cjk_term(term):
                continue
            if _is_weak_user_term(term):
                continue
            if term not in terms:
                terms.append(term)
            if len(terms) >= limit:
                return terms

    return terms


def _clean_terms(terms: Iterable[str], limit: int = 40) -> List[str]:
    cleaned: List[str] = []
    for term in terms:
        term = str(term or "").strip()
        if not term:
            continue
        if len(term) < 2:
            continue
        if term not in cleaned:
            cleaned.append(term)
    return cleaned[:limit]


def _guide_field_texts(guide: dict[str, Any]) -> dict[str, str]:
    return {
        "aliases": _as_text(guide.get("top1_aliases")),
        "title": _as_text(guide.get("title")),
        "keywords": _as_text(guide.get("keywords")),
        "scenario": _as_text(guide.get("scenario")),
        "goal": _as_text(guide.get("goal")),
        "category": _as_text(guide.get("category")),
        "metadata": " ".join([
            _as_text(guide.get("domains")),
            _as_text(guide.get("intents")),
            _as_text(guide.get("objects")),
            _as_text(guide.get("signals")),
        ]),
        "body": " ".join([
            _as_text(guide.get("steps")),
            _as_text(guide.get("check")),
            _as_text(guide.get("common_mistakes")),
            _as_text(guide.get("fallback")),
            _as_text(guide.get("stop_or_escalate")),
            _as_text(guide.get("notes")),
        ]),
    }


def _negative_text(guide: dict[str, Any]) -> str:
    return _as_text(guide.get("negative_keywords"))


def _has_absence_context(text: str, term: str) -> bool:
    text = str(text or "")
    term = str(term or "")
    if not text or not term:
        return False

    for match in re.finditer(re.escape(term), text):
        prefix = text[max(0, match.start() - 5):match.start()]
        if any(marker in prefix for marker in ("没有", "无明显", "无", "未见", "不是")):
            return True

    return False


def _term_idf(terms: List[str], guides: list[dict[str, Any]]) -> dict[str, float]:
    total = len(guides) or 1
    document_frequency = {term: 0 for term in terms}

    for guide in guides:
        text = " ".join(_guide_field_texts(guide).values())
        for term in terms:
            if term in text:
                document_frequency[term] += 1

    return {
        term: math.log((total + 1) / (count + 1)) + 1
        for term, count in document_frequency.items()
    }


def _weighted_guide_score(
    guide: dict[str, Any],
    *,
    terms: List[str],
    core_terms: List[str],
    user_terms: List[str],
    idf: dict[str, float],
) -> float:
    fields = _guide_field_texts(guide)
    field_weights = policy_map("guide_scoring", "field_weights")
    score = 0.0
    negative_text = _negative_text(guide)

    weighted_terms = [
        (term, 1.0)
        for term in terms
    ] + [
        (term, policy_float(("guide_scoring", "core_term_multiplier"), 1.0))
        for term in core_terms
    ] + [
        (term, policy_float(("guide_scoring", "user_term_multiplier"), 1.0))
        for term in user_terms
    ]

    seen: set[tuple[str, float]] = set()
    for term, term_weight in weighted_terms:
        term = str(term or "").strip()
        if len(term) < 2:
            continue

        key = (term, term_weight)
        if key in seen:
            continue
        seen.add(key)

        term_idf = idf.get(term, 1.0)
        if len(term) <= 2:
            term_idf *= policy_float(("guide_scoring", "short_term_idf_multiplier"), 1.0)

        if negative_text and term in negative_text:
            score -= policy_float(("guide_scoring", "negative_keyword_penalty"), 0.0) * term_weight * term_idf

        for field, text in fields.items():
            if term not in text:
                continue
            if field in {"scenario", "body"} and _has_absence_context(text, term):
                score -= policy_float(("guide_scoring", "absence_context_penalty_multiplier"), 0.0) * float(field_weights.get(field, 0.0)) * term_weight * term_idf
                continue

            score += float(field_weights.get(field, 0.0)) * term_weight * term_idf

            if text == term:
                score += policy_float(("guide_scoring", "exact_field_match_bonus_multiplier"), 0.0) * float(field_weights.get(field, 0.0)) * term_weight * term_idf
            elif field in {"title", "keywords"} and len(term) >= 3:
                score += policy_float(("guide_scoring", "title_keyword_partial_bonus_multiplier"), 0.0) * float(field_weights.get(field, 0.0)) * term_weight * term_idf

    return score


def _plan_terms(plan: SourcePlanItem) -> List[str]:
    raw_terms: List[str] = []

    for item in plan.keywords or []:
        raw_terms.extend(_split_search_terms(item))

    for item in plan.categories or []:
        raw_terms.extend(_split_search_terms(item))

    raw_terms.extend(_split_search_terms(plan.query))

    return _clean_terms(raw_terms, limit=40)


def _core_terms(core_terms: List[str] | None) -> List[str]:
    raw_terms: List[str] = []
    for item in core_terms or []:
        raw_terms.extend(_split_search_terms(item))
    return _clean_terms(raw_terms, limit=16)


def fetch_guide_candidates(
    plan: SourcePlanItem,
    user_message: str = "",
    core_terms: List[str] | None = None,
) -> List[EvidenceCandidate]:
    guides = _load_json(EMERGENCY_GUIDES_FILE, [])
    terms = _plan_terms(plan)
    core = _core_terms(core_terms)
    user_terms = _clean_terms(_char_ngrams(user_message), limit=48)
    all_terms = _clean_terms([*terms, *core, *user_terms], limit=96)
    idf = _term_idf(all_terms, guides)
    limit = plan.limit or 8

    scored = []

    for guide in guides:
        score = _weighted_guide_score(
            guide,
            terms=terms,
            core_terms=core,
            user_terms=user_terms,
            idf=idf,
        )

        if score <= 0:
            continue

        scored.append((score, guide))

    scored.sort(key=lambda item: item[0], reverse=True)

    results: List[EvidenceCandidate] = []
    for _, guide in scored[:limit]:
        results.append(
            EvidenceCandidate(
                source_type="guide",
                id=str(guide.get("id") or guide.get("title") or ""),
                title=str(guide.get("title") or ""),
                summary=str(guide.get("goal") or guide.get("scenario") or ""),
                category=str(guide.get("category") or ""),
                tags=_as_tags(guide.get("keywords"))[:8],
                snippet=str(guide.get("scenario") or ""),
                raw=guide,
            )
        )

    return results


def fetch_wiki_candidates(
    plan: SourcePlanItem,
    user_message: str = "",
    core_terms: List[str] | None = None,
) -> List[EvidenceCandidate]:
    from api.services.wiki_service import search_wiki_for_ai

    limit = plan.limit or 8
    terms = _plan_terms(plan)
    core = _core_terms(core_terms)

    queries: List[str] = []

    if plan.query:
        queries.append(plan.query)

    # Use planner terms and core_terms as additional search probes.
    for term in terms + core:
        if term not in queries:
            queries.append(term)

    seen = set()
    scored: List[tuple[int, EvidenceCandidate]] = []

    for query in queries[:12]:
        query = str(query or "").strip()
        if not query:
            continue

        try:
            results = search_wiki_for_ai(query, limit=limit)
        except Exception as exc:
            continue

        for article in results or []:
            article_id = str(
                article.get("id")
                or article.get("article_id")
                or article.get("slug")
                or article.get("title")
                or ""
            ).strip()

            title = str(article.get("title") or "").strip()

            if not article_id or not title:
                continue

            key = ("wiki", article_id)
            if key in seen:
                continue

            content = str(
                article.get("snippet")
                or article.get("summary")
                or article.get("description")
                or article.get("content")
                or ""
            )

            tags = _as_tags(article.get("tags"))

            match_text = " ".join([
                title,
                str(article.get("category") or ""),
                " ".join(tags),
                content[:500],
            ])

            score = _score_text_match(match_text, terms)
            score += _score_text_match(title, terms) * 2
            score += _score_text_match(match_text, core) * 2
            score += _score_text_match(title, core) * 4

            if score <= 0:
                continue

            candidate = EvidenceCandidate(
                source_type="wiki",
                id=article_id,
                title=title,
                summary=content[:300],
                category=str(article.get("category") or ""),
                tags=tags,
                snippet=content[:300],
                raw=article,
            )

            scored.append((score, candidate))
            seen.add(key)

    scored.sort(key=lambda item: item[0], reverse=True)

    return [candidate for _, candidate in scored[:limit]]


def _kiwix_candidate_id(result: Any) -> str:
    zim_filename = str(getattr(result, "zim_filename", "") or "").strip()
    article_path = str(getattr(result, "article_path", "") or "").strip()
    title = str(getattr(result, "title", "") or "").strip()
    source = str(getattr(result, "source", "") or "kiwix_zim").strip()
    stable_key = article_path or title
    return f"kiwix:{zim_filename or source}:{stable_key}"


def _kiwix_candidate_raw(result: Any, *, channel: str) -> dict[str, Any]:
    title = str(getattr(result, "title", "") or "").strip()
    snippet = str(getattr(result, "snippet", "") or "").strip()
    zim_filename = str(getattr(result, "zim_filename", "") or "").strip()
    zim_source = re.sub(r"\.zim$", "", zim_filename, flags=re.IGNORECASE)
    article_path = str(getattr(result, "article_path", "") or "").strip()
    usage_policy = str(getattr(result, "usage_policy", "") or "").strip()
    source_role = str(getattr(result, "role", "") or "").strip()
    language = str(getattr(result, "language", "") or "").strip()
    relevance_score = float(getattr(result, "relevance_score", 0.0) or 0.0)
    source_id = _kiwix_candidate_id(result)

    return {
        "source_type": "kiwix",
        "source_id": source_id,
        "id": source_id,
        "title": title,
        "label": policy_map("source_labels").get("kiwix", "kiwix"),
        "summary": snippet,
        "snippet": snippet,
        "content": snippet,
        "url": getattr(result, "url", None),
        "zim_url": getattr(result, "url", None),
        "article_path": article_path,
        "zim_filename": zim_filename,
        "source_role": source_role,
        "role": source_role,
        "usage_policy": usage_policy,
        "language": language,
        "score": relevance_score,
        "relevance_score": relevance_score,
        "matched_terms": list(getattr(result, "matched_terms", []) or []),
        "matched_terms_count": int(getattr(result, "matched_terms_count", 0) or 0),
        "match_type": str(getattr(result, "match_type", "") or ""),
        "open_url": (
            f"/kiwix.html?source={zim_source or zim_filename}&path={article_path}"
            if (zim_source or zim_filename) and article_path
            else ""
        ),
        "retrieval_channel": channel,
        "metadata": {
            "zim_filename": zim_filename,
            "article_path": article_path,
            "usage_policy": usage_policy,
            "source_role": source_role,
            "language": language,
            "topics": list(getattr(result, "topics", []) or []),
            "match_type": str(getattr(result, "match_type", "") or ""),
        },
    }


def _kiwix_focus_terms(
    *,
    plan: SourcePlanItem,
    user_message: str = "",
    core_terms: List[str] | None = None,
) -> List[str]:
    raw_terms = [
        *_core_terms(core_terms),
        *_plan_terms(plan),
        *_char_ngrams(user_message, sizes=(4, 3, 2), limit=24),
    ]
    terms: List[str] = []
    for term in raw_terms:
        term = str(term or "").strip()
        compact = _compact_match_text(term)
        if len(compact) < 2:
            continue
        if not _is_valid_search_term(term):
            continue
        if any(stop.lower() in compact for stop in policy_str_list("kiwix", "query_stop_substrings")):
            continue
        if compact not in {_compact_match_text(item) for item in terms}:
            terms.append(term)
    return terms[:16]


def _kiwix_policy_rank(candidate: EvidenceCandidate) -> int:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    policy = str(raw.get("usage_policy") or "").strip()
    role = str(raw.get("source_role") or raw.get("role") or "").strip()
    role_to_policy = policy_map("kiwix", "usage_policy", "role_to_policy")
    rank = policy_map("kiwix", "usage_policy", "policy_rank")
    resolved_policy = policy or str(role_to_policy.get(role, ""))
    return int(rank.get(resolved_policy, rank.get("default", 0)))


def _kiwix_title_match_rank(candidate: EvidenceCandidate, focus_terms: List[str]) -> tuple[int, int, int]:
    title = _compact_match_text(candidate.title)
    snippet = _compact_match_text(candidate.snippet)
    compact_terms = [
        (index, _compact_match_text(term))
        for index, term in enumerate(focus_terms or [])
        if _compact_match_text(term)
    ]

    exact_matches = [(index, len(term)) for index, term in compact_terms if title == term]
    if exact_matches:
        index, length = min(exact_matches, key=lambda item: (item[0], -item[1]))
        return (0, index, -length)
    title_matches = [(index, len(term)) for index, term in compact_terms if term in title]
    if title_matches:
        index, length = min(title_matches, key=lambda item: (item[0], -item[1]))
        return (1, index, -length)
    snippet_matches = [(index, len(term)) for index, term in compact_terms if term in snippet]
    if snippet_matches:
        index, length = min(snippet_matches, key=lambda item: (item[0], -item[1]))
        return (2, index, -length)
    return (3, len(compact_terms), 0)


def _sort_kiwix_candidates(candidates: List[EvidenceCandidate], focus_terms: List[str]) -> List[EvidenceCandidate]:
    def sort_key(candidate: EvidenceCandidate):
        raw = candidate.raw if isinstance(candidate.raw, dict) else {}
        language = str(raw.get("language") or "").strip()
        score = float(raw.get("relevance_score") or raw.get("score") or 0.0)
        matched_count = int(raw.get("matched_terms_count") or 0)
        title_rank, focus_order_rank, specificity_rank = _kiwix_title_match_rank(candidate, focus_terms)
        preferred_language = policy_string(("language", "preferred"), "")
        return (
            title_rank,
            focus_order_rank,
            specificity_rank,
            _kiwix_policy_rank(candidate),
            0 if language == preferred_language else policy_int(("language", "non_preferred_penalty"), 1),
            -matched_count,
            -score,
            candidate.title,
            candidate.id,
        )

    return sorted(candidates, key=sort_key)


def _candidate_matches_anchor(candidate: EvidenceCandidate, anchors: List[str]) -> bool:
    if not anchors:
        return True

    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    text = _compact_match_text(" ".join([
        candidate.title,
        candidate.snippet,
        str(raw.get("article_path") or ""),
        " ".join(str(term or "") for term in raw.get("matched_terms") or []),
    ]))

    return any(_compact_match_text(anchor) in text for anchor in anchors)


def _filter_kiwix_candidates_by_direct_anchor(
    candidates: List[EvidenceCandidate],
    focus_terms: List[str],
) -> List[EvidenceCandidate]:
    direct_candidates = [
        candidate
        for candidate in candidates
        if isinstance(candidate.raw, dict)
        and candidate.raw.get("match_type") == "direct"
        and _is_valid_search_term(candidate.title)
    ]
    if not direct_candidates:
        return candidates

    ranked_direct = [
        (_kiwix_title_match_rank(candidate, focus_terms), candidate)
        for candidate in direct_candidates
    ]
    best_rank = min(rank for rank, _ in ranked_direct)
    direct_anchors = [
        candidate.title
        for rank, candidate in ranked_direct
        if rank == best_rank
    ]
    if not direct_anchors:
        return candidates

    return [
        candidate
        for candidate in candidates
        if _candidate_matches_anchor(candidate, direct_anchors)
    ]


def fetch_kiwix_candidates(
    plan: SourcePlanItem,
    user_message: str = "",
    core_terms: List[str] | None = None,
) -> List[EvidenceCandidate]:
    """Fetch Kiwix/ZIM candidates through the existing Kiwix service layer."""
    from api.kiwix.orchestrator import run_kiwix_lookup, run_kiwix_query

    min_limit = policy_int(("kiwix", "search", "limit_min"), 1)
    max_limit = policy_int(("kiwix", "search", "limit_max"), 8)
    limit = min(max(plan.limit or min_limit, min_limit), max_limit)
    terms = _plan_terms(plan)
    core = _core_terms(core_terms)
    focus_terms = _kiwix_focus_terms(plan=plan, user_message=user_message, core_terms=core_terms)
    context = {
        "core_terms": focus_terms or core,
        "keywords": terms,
        "topics": [plan.query, *terms, *core, *focus_terms],
    }
    query = " ".join([*(focus_terms or []), plan.query or user_message, *terms, *core]).strip()
    if not query:
        return []

    raw_results: List[tuple[str, Any]] = []

    try:
        raw_results.extend(("ai", item) for item in run_kiwix_query(query, context))
    except Exception:
        pass

    try:
        raw_results.extend(("lookup", item) for item in run_kiwix_lookup(query, context))
    except Exception:
        pass

    seen = set()
    candidates: List[EvidenceCandidate] = []

    for channel, result in raw_results:
        title = str(getattr(result, "title", "") or "").strip()
        snippet = str(getattr(result, "snippet", "") or "").strip()
        if not title or not snippet:
            continue

        candidate_id = _kiwix_candidate_id(result)
        if candidate_id in seen:
            continue
        seen.add(candidate_id)

        raw = _kiwix_candidate_raw(result, channel=channel)
        candidates.append(
            EvidenceCandidate(
                source_type="kiwix",
                id=candidate_id,
                title=title,
                summary=snippet[:300],
                category=str(raw.get("source_role") or "background"),
                tags=[
                    value
                    for value in [
                        raw.get("usage_policy"),
                        raw.get("language"),
                        raw.get("zim_filename"),
                    ]
                    if value
                ][:8],
                snippet=snippet[:300],
                raw=raw,
            )
        )

    candidates = _filter_kiwix_candidates_by_direct_anchor(candidates, focus_terms)
    return _sort_kiwix_candidates(candidates, focus_terms)[:limit]


def fetch_candidates_from_plan(
    source_plan: List[SourcePlanItem],
    user_message: str = "",
    core_terms: List[str] | None = None,
) -> List[EvidenceCandidate]:
    candidates: List[EvidenceCandidate] = []
    seen = set()
    core = core_terms or []

    for item in source_plan:
        if item.source_type == "guide":
            fetched = fetch_guide_candidates(
                item,
                user_message=user_message,
                core_terms=core,
            )
        elif item.source_type == "wiki":
            fetched = fetch_wiki_candidates(
                item,
                user_message=user_message,
                core_terms=core,
            )
        elif item.source_type == "kiwix":
            fetched = fetch_kiwix_candidates(
                item,
                user_message=user_message,
                core_terms=core,
            )
        else:
            fetched = []

        for candidate in fetched:
            key = (candidate.source_type, candidate.id)
            if key in seen:
                continue
            candidates.append(candidate)
            seen.add(key)

    return candidates
