"""Local keyword matching plus optional ZIM lookup for Kiwix enrichment."""

import json
import re
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple

from api.kiwix.schema import KiwixResult
from api.retrieval_v2.policy import policy_float, policy_int, policy_set, policy_string


ROOT = Path(__file__).resolve().parents[2]
GUIDE_INDEX_FILE = ROOT / "data" / "guide_index.json"
WIKI_IMPORT_DIR = ROOT / "wiki_import"

STOP_TERMS = policy_set("term_filter", "query_stop_terms")


def _as_list(value: Any) -> List[str]:
    if value is None:
        return []
    if isinstance(value, list):
        return [str(item).strip() for item in value if str(item).strip()]
    if isinstance(value, str):
        text = value
        for separator in ["，", "、", ";", "；", "|", "\n", "\t"]:
            text = text.replace(separator, ",")
        return [part.strip() for part in text.split(",") if part.strip()]
    return [str(value).strip()] if str(value).strip() else []


def _compact_text(value: Any) -> str:
    return re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", str(value or ""))


def _is_cjk_term(term: str) -> bool:
    return bool(re.fullmatch(r"[\u4e00-\u9fff]+", str(term or "")))


def _is_valid_search_term(term: str) -> bool:
    compact = _compact_text(term).lower()
    if not compact:
        return False

    has_cjk = bool(re.search(r"[\u4e00-\u9fff]", compact))
    has_latin = bool(re.search(r"[a-z]", compact))
    if has_cjk and has_latin:
        return False

    if has_latin and len(compact) < policy_int(("zim", "search_scoring", "latin_token_min_chars"), 3):
        return False

    return True


def _is_weak_term(term: str) -> bool:
    term = str(term or "").strip()
    if len(term) < 2:
        return True
    if term in STOP_TERMS:
        return True
    if any(stop in term for stop in STOP_TERMS):
        return True

    weak_chars = set(policy_string(("term_filter", "weak_user_term_chars"), ""))
    weak_count = sum(1 for char in term if char in weak_chars)
    return weak_count >= len(term)


def extract_core_terms(query: str, context: Optional[Dict] = None, limit: int = 24) -> List[str]:
    """Extract stable keyword probes from user text and optional context."""
    context = context or {}
    terms: List[str] = []

    for key in ("core_terms", "keywords", "topics"):
        for item in _as_list(context.get(key)):
            if len(item) >= 2 and _is_valid_search_term(item) and not _is_weak_term(item) and item not in terms:
                terms.append(item)

    query_text = str(query or "")
    for token in re.findall(r"[A-Za-z0-9][A-Za-z0-9_-]{1,}", query_text):
        token = token.strip()
        if token.lower() in STOP_TERMS:
            continue
        if not _is_valid_search_term(token):
            continue
        if token and token not in terms:
            terms.append(token)

    compact = _compact_text(query_text)
    for size in (4, 3, 2):
        if len(compact) < size:
            continue
        for index in range(0, len(compact) - size + 1):
            term = compact[index:index + size]
            if not _is_cjk_term(term):
                continue
            if _is_weak_term(term):
                continue
            if term not in terms:
                terms.append(term)
            if len(terms) >= limit:
                return terms

    return terms[:limit]


def _load_json(path: Path, default: Any) -> Any:
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def _parse_front_matter(path: Path) -> Dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---"):
        return {"body": text}

    parts = text.split("---", 2)
    if len(parts) < 3:
        return {"body": text}

    metadata: Dict[str, Any] = {}
    for line in parts[1].splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        metadata[key.strip()] = value.strip()

    metadata["body"] = parts[2].strip()
    return metadata


def _wiki_records() -> List[Dict[str, Any]]:
    if not WIKI_IMPORT_DIR.exists():
        return []

    records = []
    for path in sorted(WIKI_IMPORT_DIR.rglob("*.md")):
        metadata = _parse_front_matter(path)
        title = str(metadata.get("title") or path.stem).strip()
        topics = [
            *_as_list(metadata.get("kiwix_topics")),
            *_as_list(metadata.get("tags")),
            *_as_list(metadata.get("category")),
        ]
        records.append(
            {
                "title": title,
                "source": "wiki_import",
                "snippet": str(metadata.get("summary") or "").strip(),
                "topics": topics,
                "search_text": " ".join(
                    [
                        title,
                        str(metadata.get("summary") or ""),
                        str(metadata.get("category") or ""),
                        " ".join(topics),
                        str(metadata.get("body") or "")[:1200],
                    ]
                ),
                "url": None,
            }
        )
    return records


def _guide_records() -> List[Dict[str, Any]]:
    guides = _load_json(GUIDE_INDEX_FILE, [])
    records = []

    for guide in guides:
        title = str(guide.get("title") or guide.get("id") or "").strip()
        if not title:
            continue

        topics = [
            *_as_list(guide.get("keywords")),
            *_as_list(guide.get("domains")),
            *_as_list(guide.get("intents")),
            *_as_list(guide.get("category")),
        ]
        snippet = str(guide.get("scenario") or guide.get("goal") or "").strip()
        records.append(
            {
                "title": title,
                "source": "guide_keyword_index",
                "snippet": snippet,
                "topics": topics,
                "search_text": " ".join(
                    [
                        title,
                        snippet,
                        str(guide.get("category") or ""),
                        " ".join(topics),
                    ]
                ),
                "url": None,
            }
        )

    return records


def _score_record(record: Dict[str, Any], terms: Iterable[str]) -> Tuple[float, List[str]]:
    title = str(record.get("title") or "")
    topics = _as_list(record.get("topics"))
    snippet = str(record.get("snippet") or "")
    search_text = str(record.get("search_text") or "")

    score = 0.0
    matched_terms = []

    for term in terms:
        term = str(term or "").strip()
        if not term:
            continue

        term_score = 0.0
        if term in title:
            term_score += 4.0
        if any(term in topic for topic in topics):
            term_score += 3.0
        if term in snippet:
            term_score += 2.0
        if term in search_text:
            term_score += 1.0

        if term_score > 0:
            score += term_score
            matched_terms.append(term)

    return score, matched_terms


def classify_kiwix_domain(query: str, terms: List[str]) -> str:
    try:
        from api.kiwix.zim_client import classify_query_domain

        domains = classify_query_domain(" ".join([str(query or ""), *[str(term or "") for term in terms]]))
        return domains[0] if domains else "general"
    except Exception:
        pass

    return "general"


def _zim_sources_for_domain(domain: str) -> List[str]:
    return [domain] if domain and domain != "general" else []


def _is_anchor_term(term: str) -> bool:
    term = str(term or "").strip()
    if not term:
        return False
    if not _is_valid_search_term(term):
        return False
    compact = _compact_text(term)
    if len(compact) < policy_int(("zim", "search_scoring", "anchor_term_min_chars"), 2):
        return False
    return not _is_weak_term(term)


def _hit_matches_anchor(hit: Dict[str, Any], anchors: List[str]) -> bool:
    if not anchors:
        return True

    text = _compact_text(" ".join([
        str(hit.get("title") or ""),
        str(hit.get("snippet") or ""),
        " ".join(str(term or "") for term in hit.get("matched_terms") or []),
    ])).lower()

    return any(_compact_text(anchor).lower() in text for anchor in anchors)


def _result_matches_anchor(result: KiwixResult, anchors: List[str]) -> bool:
    if not anchors:
        return True

    text = _compact_text(" ".join([
        result.title,
        result.snippet,
        result.article_path or "",
        " ".join(result.matched_terms or []),
    ])).lower()

    return any(_compact_text(anchor).lower() in text for anchor in anchors)


def _result_anchor_order(result: KiwixResult, anchors: List[str]) -> int:
    text = _compact_text(" ".join([
        result.title,
        result.article_path or "",
        " ".join(result.matched_terms or []),
    ])).lower()

    for index, anchor in enumerate(anchors or []):
        if _compact_text(anchor).lower() in text:
            return index
    return len(anchors or [])


def fetch_mock_kiwix_results(
    query: str,
    context: Optional[Dict] = None,
    limit: int = 5,
) -> List[KiwixResult]:
    """Fetch local enrichment results without affecting Guide/Wiki selection."""
    terms = extract_core_terms(query, context)
    if not terms:
        return []

    scored = []
    seen = set()
    for record in [*_wiki_records(), *_guide_records()]:
        score, matched_terms = _score_record(record, terms)
        if score <= 0:
            continue

        key = (record["source"], record["title"])
        if key in seen:
            continue
        seen.add(key)

        max_score = max(
            policy_float(("kiwix", "search", "local_score_baseline"), 1.0),
            len(terms) * policy_float(("kiwix", "search", "local_score_term_multiplier"), 1.0),
        )
        relevance_score = min(score / max_score, 1.0)
        snippet = record["snippet"] or f"Local Kiwix stub keyword match: {', '.join(matched_terms[:5])}"

        scored.append(
            (
                relevance_score,
                record["source"],
                record["title"],
                KiwixResult(
                    title=record["title"],
                    source=record["source"],
                    snippet=snippet[:policy_int(("kiwix", "search", "snippet_chars"), 360)],
                    relevance_score=round(relevance_score, 4),
                    topics=record["topics"][:10],
                    url=record.get("url"),
                    matched_terms=matched_terms,
                ),
            )
        )

    scored.sort(key=lambda item: (-item[0], item[1], item[2]))
    local_results = [result for _, _, _, result in scored[:limit]]
    zim_results = _fetch_zim_results(
        query=query,
        terms=terms,
        limit=policy_int(("kiwix", "search", "zim_result_merge_limit"), 5),
        channel="ai",
    )

    merged: List[Tuple[float, str, str, KiwixResult]] = []
    seen = set()
    for result in [*local_results, *zim_results]:
        key = (result.source, result.title)
        if key in seen:
            continue
        seen.add(key)
        merged.append((result.relevance_score, result.source, result.title, result))

    merged.sort(key=lambda item: (-item[0], item[1], item[2]))
    return [result for _, _, _, result in merged[:limit]]


def query_for_ai(
    query: str,
    context: Optional[Dict] = None,
    limit: int = 5,
) -> List[KiwixResult]:
    """Query the ZIM decision channel for Retrieval v2 background only."""
    terms = extract_core_terms(query, context)
    return _fetch_zim_results(query=query, terms=terms, limit=limit, channel="ai")


def query_for_lookup(
    query: str,
    context: Optional[Dict] = None,
    limit: int = 8,
) -> List[KiwixResult]:
    """Query the human lookup channel, including lookup/support language sources."""
    terms = extract_core_terms(query, context)
    return _fetch_zim_results(query=query, terms=terms, limit=limit, channel="lookup")


def _fetch_zim_results(
    query: str,
    terms: List[str],
    limit: int = 5,
    channel: str = "ai",
) -> List[KiwixResult]:
    try:
        from api.kiwix.zim_client import query_for_ai as query_zim_for_ai
        from api.kiwix.zim_client import query_for_lookup as query_zim_for_lookup
        from api.kiwix.zim_client import classify_query_domain
        from api.kiwix.zim_client import extract_query_core_terms as extract_zim_core_terms
    except Exception:
        return []

    results_by_key: Dict[Tuple[str, str], KiwixResult] = {}
    domains = classify_query_domain(" ".join([str(query or ""), *[str(term or "") for term in terms]]))
    domain = domains[0] if domains else classify_kiwix_domain(query, terms)
    anchor_terms = []
    for term in terms:
        if _is_anchor_term(term) and term not in anchor_terms:
            anchor_terms.append(term)

    queries: List[str] = []
    for item in [*anchor_terms, query, *terms, *extract_zim_core_terms(query)]:
        item = str(item or "").strip()
        if item and _is_valid_search_term(item) and item not in queries:
            queries.append(item)

    for item in queries[:policy_int(("kiwix", "search", "query_probe_limit"), 8)]:
        try:
            zim_hits = (
                query_zim_for_lookup(item, limit=limit)
                if channel == "lookup"
                else query_zim_for_ai(item, limit=limit)
            )
        except Exception:
            zim_hits = []

        for hit in zim_hits:
            title = str(hit.get("title") or "").strip()
            snippet = str(hit.get("snippet") or "").strip()
            if not title or not snippet:
                continue
            if not _hit_matches_anchor(hit, anchor_terms):
                continue

            zim_source = str(hit.get("zim_source") or "wiki")
            key = ("kiwix_zim", zim_source, title)
            candidate = KiwixResult(
                title=title,
                source="kiwix_zim",
                snippet=snippet,
                relevance_score=float(hit.get("score") or 0.0),
                topics=[*domains, zim_source],
                url=hit.get("url"),
                zim_filename=hit.get("zim_filename"),
                language=hit.get("language"),
                role=hit.get("role"),
                usage_policy=hit.get("usage_policy"),
                article_path=hit.get("article_path"),
                matched_terms=hit.get("matched_terms") or [],
                matched_terms_count=int(hit.get("matched_terms_count") or 0),
                match_type=str(hit.get("match_type") or ""),
            )
            previous = results_by_key.get(key)
            if (
                not previous
                or candidate.matched_terms_count > previous.matched_terms_count
                or (
                    candidate.matched_terms_count == previous.matched_terms_count
                    and candidate.relevance_score > previous.relevance_score
                )
            ):
                results_by_key[key] = candidate

    results = list(results_by_key.values())
    direct_anchors = [
        result.title
        for result in results
        if result.match_type == "direct" and _is_anchor_term(result.title)
    ]
    if direct_anchors:
        results = [
            result
            for result in results
            if result.match_type == "direct" or _result_matches_anchor(result, direct_anchors)
        ]
    results.sort(
        key=lambda item: (
            0 if item.match_type == "direct" else 1,
            _result_anchor_order(item, anchor_terms),
            -item.matched_terms_count,
            -item.relevance_score,
            item.source,
            item.title,
        )
    )
    return results[:limit]
