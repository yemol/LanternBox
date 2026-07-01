"""Wiki Service。负责 Wiki 查询、过滤、规范化和分类数据访问。"""

import json
import re
from functools import lru_cache
from typing import Any, Dict, Optional, List

from ..config import (
    POCKETBASE_WIKI_ARTICLES,
    WIKI_DOMAIN_TERMS_FILE,
    WIKI_RETRIEVAL_PROFILES_FILE,
    WIKI_STOP_TERMS_FILE,
)
from ..pocketbase_client import pocketbase_get_records



@lru_cache(maxsize=1)
def load_wiki_stop_terms() -> set[str]:
    if not WIKI_STOP_TERMS_FILE.exists():
        return set()

    data = json.loads(WIKI_STOP_TERMS_FILE.read_text(encoding="utf-8"))

    if not isinstance(data, list):
        return set()

    return {
        str(item).strip()
        for item in data
        if str(item).strip()
    }


def trim_text(text: str, max_chars: int = 800) -> str:
    text = (text or "").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "……"


def clean_wiki_search_term(term: str) -> str:
    term = (term or "").strip()
    term = re.sub(r"[，。！？、；：,.!?;:\s]+", "", term)
    return term


@lru_cache(maxsize=1)
def load_wiki_domain_terms() -> Dict[str, List[str]]:
    if not WIKI_DOMAIN_TERMS_FILE.exists():
        return {}

    data = json.loads(WIKI_DOMAIN_TERMS_FILE.read_text(encoding="utf-8"))

    if not isinstance(data, dict):
        return {}

    return {
        str(domain): [str(word) for word in words]
        for domain, words in data.items()
        if isinstance(words, list)
    }


@lru_cache(maxsize=1)
def load_wiki_retrieval_profiles() -> Dict[str, Any]:
    default_profiles = {
        "default": {
            "min_score": 1,
            "max_results": 3,
        },
        "by_intent": {},
        "by_domain": {},
    }

    if not WIKI_RETRIEVAL_PROFILES_FILE.exists():
        return default_profiles

    data = json.loads(WIKI_RETRIEVAL_PROFILES_FILE.read_text(encoding="utf-8"))

    if not isinstance(data, dict):
        return default_profiles

    return {
        **default_profiles,
        **data,
    }


def ensure_list(value: Any) -> List[str]:
    if value is None:
        return []

    if isinstance(value, list):
        return [str(item).strip() for item in value if str(item).strip()]

    if isinstance(value, str):
        raw = value.replace("，", ",").replace("；", ",").replace(";", ",")
        return [item.strip() for item in raw.split(",") if item.strip()]

    return []


def wiki_text_for_filter(item: Dict[str, Any]) -> str:
    return " ".join([
        str(item.get("title", "")),
        str(item.get("summary", "")),
        str(item.get("tags", "")),
        str(item.get("category", "")),
        str(item.get("source", "")),
    ])


def wiki_matches_any(item: Dict[str, Any], words: List[str]) -> bool:
    text = wiki_text_for_filter(item)
    return any(word in text for word in words)


def infer_wiki_domains(item: Dict[str, Any]) -> List[str]:
    """根据外部词表，从 Wiki 标题、摘要、标签和分类粗略推断领域。"""
    text = wiki_text_for_filter(item)
    domains: List[str] = []

    for domain, words in load_wiki_domain_terms().items():
        if any(word in text for word in words):
            domains.append(domain)

    return domains


def get_wiki_domains(item: Dict[str, Any]) -> List[str]:
    domains = ensure_list(item.get("domains"))
    return domains or infer_wiki_domains(item)


def get_wiki_intents(item: Dict[str, Any]) -> List[str]:
    return ensure_list(item.get("intents"))


def get_wiki_profile(query_profile: Dict[str, Any]) -> Dict[str, Any]:
    profiles = load_wiki_retrieval_profiles()
    default_profile = profiles.get("default", {})

    intents = query_profile.get("intents", []) or []
    domains = query_profile.get("domains", []) or []

    by_intent = profiles.get("by_intent", {}) or {}
    by_domain = profiles.get("by_domain", {}) or {}

    for intent in intents:
        if intent in by_intent:
            return {
                **default_profile,
                **by_intent[intent],
            }

    for domain in domains:
        if domain in by_domain:
            return {
                **default_profile,
                **by_domain[domain],
            }

    return default_profile


def score_wiki_for_query(
    item: Dict[str, Any],
    query_profile: Dict[str, Any],
    retrieval_profile: Dict[str, Any],
) -> int:
    score = 0

    query_domains = set(query_profile.get("domains", []) or [])
    query_intents = set(query_profile.get("intents", []) or [])
    query_signals = set(query_profile.get("signals", []) or [])
    query_risks = set(query_profile.get("risks", []) or [])

    wiki_domains = set(get_wiki_domains(item))
    wiki_intents = set(get_wiki_intents(item))
    wiki_signals = set(ensure_list(item.get("signals")))
    wiki_risks = set(ensure_list(item.get("risks")))

    allowed_domains = set(retrieval_profile.get("allowed_domains", []) or [])
    blocked_domains = set(retrieval_profile.get("blocked_domains", []) or [])
    preferred_terms = ensure_list(retrieval_profile.get("preferred_terms"))
    blocked_terms = ensure_list(retrieval_profile.get("blocked_terms"))
    text = wiki_text_for_filter(item)

    if blocked_terms and wiki_matches_any(item, blocked_terms):
        return -100

    if blocked_domains and wiki_domains & blocked_domains:
        if not (allowed_domains and wiki_domains & allowed_domains):
            return -100

    if allowed_domains and wiki_domains:
        if not (wiki_domains & allowed_domains):
            return -50

    score += 12 * len(query_domains & wiki_domains)
    score += 30 * len(query_intents & wiki_intents)
    score += 8 * len(query_signals & wiki_signals)
    score += 5 * len(query_risks & wiki_risks)

    matched_preferred_terms = [
        term for term in preferred_terms
        if term in text
    ]
    score += 10 * len(matched_preferred_terms)

    # Wiki 当前多半还没有结构化 intent。领域命中即可作为背景资料。
    if not wiki_intents and query_domains & wiki_domains:
        score += 5

    return score


def filter_related_wikis_for_query(related_wikis, query_profile):
    """
    按 Context Profile + Wiki Retrieval Profile 过滤 Wiki 噪声。

    具体业务策略来自 data/wiki_retrieval_profiles.json；
    领域词表来自 data/wiki_domain_terms.json。
    """
    if not related_wikis:
        return related_wikis

    query_profile = query_profile or {}
    retrieval_profile = get_wiki_profile(query_profile)
    min_score = int(retrieval_profile.get("min_score", 1))
    max_results = int(retrieval_profile.get("max_results", 3))

    scored = []

    for item in related_wikis:
        score = score_wiki_for_query(
            item,
            query_profile,
            retrieval_profile,
        )

        if score < min_score:
            continue

        enriched = dict(item)
        enriched["_wiki_match_score"] = score
        enriched["_wiki_domains"] = get_wiki_domains(item)
        enriched["_wiki_intents"] = get_wiki_intents(item)
        scored.append(enriched)

    scored.sort(
        key=lambda item: item.get("_wiki_match_score", 0),
        reverse=True,
    )

    return scored[:max_results]


def search_wiki_for_ai(
    keyword: str,
    detected_domains: Optional[List[str]] = None,
    query_profile: Optional[Dict[str, Any]] = None,
    limit: int = 3,
) -> List[dict]:
    keyword = (keyword or "").strip()

    if not keyword:
        return []

    detected_domains = detected_domains or []
    query_profile = query_profile or {}

    articles_by_id: Dict[str, Dict[str, Any]] = {}

    def add_articles(articles: List[Dict[str, Any]]) -> None:
        for article in articles:
            article_id = str(article.get("id") or "").strip()
            if not article_id:
                continue

            if article_id not in articles_by_id:
                articles_by_id[article_id] = article

    # 第一层：原句直接搜索。
    # 注意：这里不再提前 return，避免弱命中阻断 profile 扩展搜索。
    direct_articles = search_wiki_articles(keyword, limit=limit)
    add_articles(direct_articles)

    # 第二层：根据 Context Profile / Wiki Retrieval Profile 扩展搜索词。
    terms = build_wiki_search_terms(
        text=keyword,
        detected_domains=detected_domains,
        query_profile=query_profile,
        max_terms=8,
    )

    if terms:
        wiki_filter = build_wiki_or_filter_for_terms(terms)

        data = pocketbase_get_records(
            POCKETBASE_WIKI_ARTICLES,
            params={
                "page": 1,
                "perPage": max(limit * 3, 10),
                "sort": "-updated",
                "filter": wiki_filter,
            }
        )

        expanded_articles = [
            normalize_wiki_article(item)
            for item in data.get("items", [])
        ]

        add_articles(expanded_articles)

    articles = list(articles_by_id.values())

    return normalize_wiki_articles_for_ai(articles[: max(limit * 3, 10)])


def search_wiki_articles(keyword: str, limit: int = 20) -> list[dict]:
    keyword = keyword.strip()

    if not keyword:
        return []

    safe_keyword = keyword.replace('"', '\"')

    data = pocketbase_get_records(
        POCKETBASE_WIKI_ARTICLES,
        params={
            "page": 1,
            "perPage": limit,
            "sort": "-updated",
            "filter": (
                f'status = "published" && '
                f'(title ~ "{safe_keyword}" || '
                f'summary ~ "{safe_keyword}" || '
                f'content ~ "{safe_keyword}" || '
                f'tags ~ "{safe_keyword}")'
            ),
        }
    )

    return [
        normalize_wiki_article(item)
        for item in data.get("items", [])
    ]


def normalize_wiki_articles_for_ai(articles: List[dict]) -> List[dict]:
    results = []

    for article in articles:
        results.append({
            "id": article.get("id"),
            "title": article.get("title", ""),
            "summary": article.get("summary", ""),
            "content": trim_text(article.get("content", ""), 3000),
            "tags": article.get("tags", ""),
            "category": article.get("category", ""),
            "domains": ensure_list(article.get("domains")),
            "intents": ensure_list(article.get("intents")),
            "signals": ensure_list(article.get("signals")),
            "risks": ensure_list(article.get("risks")),
            "wiki_type": article.get("wiki_type", ""),
            "ranking_role": article.get("ranking_role", ""),
            "risk_level": article.get("risk_level", "normal"),
            "source": article.get("source", ""),
        })

    return results


def normalize_wiki_article(item: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "id": item.get("id"),
        "title": item.get("title", ""),
        "slug": item.get("slug", ""),
        "category": item.get("category", ""),
        "summary": item.get("summary", ""),
        "content": item.get("content", ""),
        "tags": item.get("tags", ""),
        "domains": item.get("domains", []),
        "intents": item.get("intents", []),
        "signals": item.get("signals", []),
        "risks": item.get("risks", []),
        "wiki_type": item.get("wiki_type", ""),
        "ranking_role": item.get("ranking_role", ""),
        "risk_level": item.get("risk_level", "normal"),
        "status": item.get("status", ""),
        "source": item.get("source", ""),
        "updated_note": item.get("updated_note", ""),
        "created": item.get("created", ""),
        "updated": item.get("updated", ""),
    }


def build_wiki_search_terms(
    text: str,
    detected_domains: Optional[List[str]] = None,
    query_profile: Optional[Dict[str, Any]] = None,
    max_terms: int = 8,
) -> List[str]:
    text = (text or "").strip()
    detected_domains = detected_domains or []
    query_profile = query_profile or {}

    terms: List[str] = []
    domain_terms = load_wiki_domain_terms()
    retrieval_profile = get_wiki_profile(query_profile)
    profile_search_terms = ensure_list(retrieval_profile.get("search_terms"))

    def add_term(term: str):
        term = clean_wiki_search_term(term)

        if not term:
            return

        if term in load_wiki_stop_terms():
            return

        if len(term) < 2:
            return

        if term not in terms:
            terms.append(term)

    # 1. 领域关键词优先：只加入用户句子中实际出现的领域词
    for domain in detected_domains:
        for word in domain_terms.get(domain, []):
            if word in text:
                add_term(word)

                if len(terms) >= max_terms:
                    return terms[:max_terms]

    # 1.5 Profile 搜索扩展词：
    # 用于把口语表达映射到 Wiki 术语。
    # 例如：“吃的东西快要吃完了” → 食物 / 口粮 / 配给。
    for word in profile_search_terms:
        add_term(word)

        if len(terms) >= max_terms:
            return terms[:max_terms]

    # 2. 通用领域词：即使 detected_domains 没传，也从全文里找明显词
    for words in domain_terms.values():
        for word in words:
            if word in text:
                add_term(word)

                if len(terms) >= max_terms:
                    return terms[:max_terms]

    # 3. 提取连续中文片段
    chinese_chunks = re.findall(r"[\u4e00-\u9fff]{2,12}", text)

    for chunk in chinese_chunks:
        if len(chunk) <= 6:
            add_term(chunk)

            if len(terms) >= max_terms:
                return terms[:max_terms]
        else:
            # 对较长句子切成 2-4 字窗口，避免整句搜不到
            for size in (4, 3, 2):
                for i in range(0, len(chunk) - size + 1):
                    add_term(chunk[i:i + size])

                    if len(terms) >= max_terms:
                        return terms[:max_terms]

    # 4. 最后兜底：原句前 12 个中文字符
    if not terms:
        fallback = "".join(chinese_chunks)[:12]
        add_term(fallback)

    return terms[:max_terms]


def build_wiki_or_filter_for_terms(terms: List[str]) -> str:
    parts = []

    for term in terms:
        safe_term = term.replace('"', '\"')

        parts.append(
            "("
            f'title~"{safe_term}" || '
            f'summary~"{safe_term}" || '
            f'content~"{safe_term}" || '
            f'tags~"{safe_term}"'
            ")"
        )

    if not parts:
        return 'status="published"'

    return 'status="published" && (' + " || ".join(parts) + ")"


def get_wiki_categories_records() -> Dict[str, Any]:
    params = {
        "page": 1,
        "perPage": 100,
        "sort": "sort_order,name",
    }

    return pocketbase_get_records(
        "wiki_categories",
        params=params,
    )


def get_wiki_articles_by_category_records(
    category_id: str,
    *,
    page: int = 1,
    per_page: int = 10,
) -> Dict[str, Any]:
    page = max(page, 1)
    per_page = min(max(per_page, 1), 50)

    params = {
        "page": page,
        "perPage": per_page,
        "sort": "title",
        "filter": f'status = "published" && category = "{category_id}"',
    }

    return pocketbase_get_records(
        "wiki_articles",
        params=params,
    )
