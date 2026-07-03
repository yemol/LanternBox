"""Source fetchers for Retrieval v2."""

import json
import math
import re
from pathlib import Path
from typing import Any, Iterable, List

from .schemas import EvidenceCandidate, SourcePlanItem


ROOT = Path(__file__).resolve().parents[2]
EMERGENCY_GUIDES_FILE = ROOT / "data" / "emergency_guides.json"

WEAK_USER_TERM_SUBSTRINGS = {
    "怎么",
    "怎么办",
    "能不能",
    "可不可以",
    "是不是",
    "有没有",
    "应该",
    "哪些",
    "什么",
    "一下",
    "这个",
    "那个",
    "今天",
    "明天",
    "今晚",
    "家里",
    "有人",
    "大家",
    "处理",
    "规则",
    "已经",
    "我该",
    "要先",
    "不能",
    "继续",
    "能喝",
    "想",
}

WEAK_USER_TERM_CHARS = set("的了和也还又该要在到吗我你他她它已都很先么经续")


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
    if any(stop in term for stop in WEAK_USER_TERM_SUBSTRINGS):
        return True

    weak_count = sum(1 for char in term if char in WEAK_USER_TERM_CHARS)
    score = min(len(term), 4) - weak_count
    if term[0] in WEAK_USER_TERM_CHARS:
        score -= 3
    if term[-1] in WEAK_USER_TERM_CHARS:
        score -= 3
    if weak_count == 0:
        score += 4

    return score <= 0


def _searchable_message_text(text: str) -> str:
    return re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", str(text or ""))


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
            if not re.search(r"[\u4e00-\u9fff]", term):
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
    field_weights = {
        "aliases": 10.0,
        "title": 9.0,
        "keywords": 6.0,
        "scenario": 3.0,
        "goal": 2.0,
        "category": 1.5,
        "metadata": 1.0,
        "body": 0.75,
    }
    score = 0.0
    negative_text = _negative_text(guide)

    weighted_terms = [
        (term, 1.0)
        for term in terms
    ] + [
        (term, 1.8)
        for term in core_terms
    ] + [
        (term, 1.2)
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
            term_idf *= 0.75

        if negative_text and term in negative_text:
            score -= 4.0 * term_weight * term_idf

        for field, text in fields.items():
            if term not in text:
                continue
            if field in {"scenario", "body"} and _has_absence_context(text, term):
                score -= 0.5 * field_weights[field] * term_weight * term_idf
                continue

            score += field_weights[field] * term_weight * term_idf

            if text == term:
                score += field_weights[field] * term_weight * term_idf
            elif field in {"title", "keywords"} and len(term) >= 3:
                score += 0.5 * field_weights[field] * term_weight * term_idf

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
