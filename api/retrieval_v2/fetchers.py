"""Source fetchers for Retrieval v2."""

import json
from pathlib import Path
from typing import Any, Iterable, List

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


def _score_text_match(text: str, terms: Iterable[str]) -> int:
    text = str(text or "")
    score = 0
    for term in terms or []:
        term = str(term or "").strip()
        if term and term in text:
            score += 1
    return score


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
    limit = plan.limit or 8

    scored = []

    for guide in guides:
        title = _as_text(guide.get("title"))
        category = _as_text(guide.get("category"))
        scenario = _as_text(guide.get("scenario"))
        goal = _as_text(guide.get("goal"))
        keywords = _as_text(guide.get("keywords"))
        domains = _as_text(guide.get("domains"))
        intents = _as_text(guide.get("intents"))

        text = " ".join([
            title,
            category,
            scenario,
            goal,
            keywords,
            domains,
            intents,
        ])

        score = _score_text_match(text, terms)
        score += _score_text_match(title, terms) * 5
        score += _score_text_match(keywords, terms) * 3
        score += _score_text_match(scenario, terms) * 2

        # core_terms 由 AI Planner 输出。代码只将其作为高权重选择信号。
        score += _score_text_match(text, core) * 3
        score += _score_text_match(title, core) * 8
        score += _score_text_match(keywords, core) * 5
        score += _score_text_match(scenario, core) * 3

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
        else:
            fetched = []

        for candidate in fetched:
            key = (candidate.source_type, candidate.id)
            if key in seen:
                continue
            candidates.append(candidate)
            seen.add(key)

    return candidates
