"""Local keyword matching plus optional ZIM lookup for Kiwix enrichment."""

import json
import re
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple

from api.kiwix.schema import KiwixResult


ROOT = Path(__file__).resolve().parents[2]
GUIDE_INDEX_FILE = ROOT / "data" / "guide_index.json"
WIKI_IMPORT_DIR = ROOT / "wiki_import"

STOP_TERMS = {
    "怎么",
    "怎么办",
    "什么",
    "哪些",
    "一下",
    "这个",
    "那个",
    "应该",
    "可以",
    "不能",
    "需要",
    "如果",
}
WEAK_CHARS = set("的了和也还又该要在到吗我你他她它已都很先么")
MEDICAL_TERMS = {
    "medical", "medicine", "health", "disease", "fever", "wound", "infection",
    "first aid", "hospital", "doctor", "药", "医疗", "发热", "伤口", "感染", "腹泻", "急救",
}
BIOLOGY_TERMS = {
    "biology", "plant", "animal", "enzyme", "cell", "gene", "species", "taxonomy",
    "botany", "植物", "动物", "细胞", "基因", "酶", "物种", "生物",
}
LANGUAGE_TERMS = {
    "dictionary", "word", "meaning", "definition", "translate", "language",
    "词典", "字典", "翻译", "释义", "定义", "语言",
}


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


def _is_weak_term(term: str) -> bool:
    term = str(term or "").strip()
    if len(term) < 2:
        return True
    if term in STOP_TERMS:
        return True
    if any(stop in term for stop in STOP_TERMS):
        return True

    weak_count = sum(1 for char in term if char in WEAK_CHARS)
    return weak_count >= len(term)


def extract_core_terms(query: str, context: Optional[Dict] = None, limit: int = 24) -> List[str]:
    """Extract stable keyword probes from user text and optional context."""
    context = context or {}
    terms: List[str] = []

    for key in ("core_terms", "keywords", "topics"):
        for item in _as_list(context.get(key)):
            if len(item) >= 2 and not _is_weak_term(item) and item not in terms:
                terms.append(item)

    query_text = str(query or "")
    for token in re.findall(r"[A-Za-z0-9][A-Za-z0-9_-]{1,}", query_text):
        token = token.strip()
        if token and token not in terms:
            terms.append(token)

    compact = _compact_text(query_text)
    for size in (4, 3, 2):
        if len(compact) < size:
            continue
        for index in range(0, len(compact) - size + 1):
            term = compact[index:index + size]
            if not re.search(r"[\u4e00-\u9fff]", term):
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

    text = " ".join([str(query or ""), *[str(term or "") for term in terms]]).lower()

    if any(term in text for term in LANGUAGE_TERMS):
        return "language"
    if any(term in text for term in MEDICAL_TERMS):
        return "medical"
    if any(term in text for term in BIOLOGY_TERMS):
        return "biology"

    return "general"


def _zim_sources_for_domain(domain: str) -> List[str]:
    if domain == "medical":
        return ["medical", "wiki"]
    if domain == "biology":
        return ["medical", "wiki"]
    if domain == "language":
        return ["dictionary", "wiki"]
    return ["wiki"]


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

        max_score = max(8.0, len(terms) * 4.0)
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
                    snippet=snippet[:360],
                    relevance_score=round(relevance_score, 4),
                    topics=record["topics"][:10],
                    url=record.get("url"),
                    matched_terms=matched_terms,
                ),
            )
        )

    scored.sort(key=lambda item: (-item[0], item[1], item[2]))
    local_results = [result for _, _, _, result in scored[:limit]]
    zim_results = _fetch_zim_results(query=query, terms=terms, limit=5, channel="ai")

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

    queries: List[str] = []
    for item in [query, *extract_zim_core_terms(query)]:
        item = str(item or "").strip()
        if item and item not in queries:
            queries.append(item)

    results: List[KiwixResult] = []
    seen = set()
    domains = classify_query_domain(" ".join([str(query or ""), *[str(term or "") for term in terms]]))
    domain = domains[0] if domains else classify_kiwix_domain(query, terms)

    for item in queries[:8]:
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

            zim_source = str(hit.get("zim_source") or "wiki")
            key = ("kiwix_zim", zim_source, title)
            if key in seen:
                continue
            seen.add(key)
            results.append(
                KiwixResult(
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
                )
            )

            if len(results) >= limit:
                return results

    return results
