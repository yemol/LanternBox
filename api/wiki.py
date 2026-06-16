import os
import requests
import re
from typing import Any, Dict, Optional, List
from fastapi import HTTPException
from typing import Optional

from .config import (
    POCKETBASE_URL,
    POCKETBASE_WIKI_ARTICLES,
    POCKETBASE_WIKI_MEDIA,
)

WIKI_DOMAIN_TERMS = {
    "water": ["饮用水", "储水", "停水", "水源", "净水", "过滤", "消毒"],
    "food": ["食物", "储存", "粮食", "保质", "发霉", "烹饪"],
    "power": ["移动电源", "充电宝", "电量", "低电量", "手机", "充电", "断电", "照明", "手电", "通讯"],
    "medical": ["伤口", "发烧", "腹泻", "药品", "消毒", "隔离"],
    "safety": ["避难", "安全", "撤离", "火灾", "燃气", "风险"],
    "tools": ["工具", "维修", "修理", "手锯", "螺丝刀", "胶带"],
    "planting": ["种植", "种子", "土豆", "番茄", "育苗", "土壤"],
}

WIKI_STOP_TERMS = {
    "怎么办", "怎么", "如何", "一下", "现在", "已经", "是不是",
    "可以", "需要", "应该", "感觉", "问题", "情况",
    "这个", "那个", "我们", "家里", "不多", "都有",
}

PB_URL = os.getenv("PB_URL", "http://127.0.0.1:8090")


def pocketbase_get_records(collection: str, params: Optional[dict] = None) -> dict:
    url = f"{POCKETBASE_URL}/api/collections/{collection}/records"

    try:
        response = requests.get(url, params=params or {}, timeout=8)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"PocketBase 读取失败: {collection}", e)
        return {
            "page": 1,
            "perPage": 0,
            "totalItems": 0,
            "totalPages": 0,
            "items": []
        }



def pocketbase_get_record(collection: str, record_id: str) -> Optional[dict]:
    url = f"{POCKETBASE_URL}/api/collections/{collection}/records/{record_id}"

    try:
        response = requests.get(url, timeout=8)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"PocketBase 读取单条失败: {collection}/{record_id}", e)
        return None


def normalize_wiki_article(item: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "id": item.get("id"),
        "title": item.get("title", ""),
        "slug": item.get("slug", ""),
        "category": item.get("category", ""),
        "summary": item.get("summary", ""),
        "content": item.get("content", ""),
        "tags": item.get("tags", ""),
        "risk_level": item.get("risk_level", "normal"),
        "status": item.get("status", ""),
        "source": item.get("source", ""),
        "updated_note": item.get("updated_note", ""),
        "created": item.get("created", ""),
        "updated": item.get("updated", ""),
    }


def normalize_wiki_media(media: Dict[str, Any]) -> Dict[str, Any]:
    file_name = media.get("file", "")
    file_url = ""

    if file_name:
        file_url = (
            f"{POCKETBASE_URL}/api/files/"
            f"{POCKETBASE_WIKI_MEDIA}/"
            f"{media.get('id')}/"
            f"{file_name}"
        )

    return {
        "id": media.get("id"),
        "media_type": media.get("media_type", ""),
        "title": media.get("title", ""),
        "file": file_name,
        "file_url": file_url,
        "description": media.get("description", ""),
        "sort_order": media.get("sort_order", 0),
    }


def get_published_wiki_articles(limit: int = 50) -> list[dict]:
    data = pocketbase_get_records(
        POCKETBASE_WIKI_ARTICLES,
        params={
            "page": 1,
            "perPage": limit,
            "sort": "-updated",
            "filter": 'status = "published" && is_featured = true',
        }
    )

    return [
        normalize_wiki_article(item)
        for item in data.get("items", [])
    ]


def get_wiki_article_detail(article_id: str) -> Optional[dict]:
    item = pocketbase_get_record(
        POCKETBASE_WIKI_ARTICLES,
        article_id
    )

    if not item:
        return None

    article = normalize_wiki_article(item)

    media_data = pocketbase_get_records(
        POCKETBASE_WIKI_MEDIA,
        params={
            "page": 1,
            "perPage": 50,
            "sort": "sort_order",
            "filter": f'article="{article_id}"',
        }
    )

    media_items = [
        normalize_wiki_media(media)
        for media in media_data.get("items", [])
    ]

    return {
        "article": article,
        "media": media_items,
    }


def search_wiki_articles(keyword: str, limit: int = 20) -> list[dict]:
    keyword = keyword.strip()

    if not keyword:
        return []

    safe_keyword = keyword.replace('"', '\\"')

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


def trim_text(text: str, max_chars: int = 800) -> str:
    text = (text or "").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "……"

def search_wiki_for_ai(
    keyword: str,
    detected_domains: Optional[List[str]] = None,
    limit: int = 3,
) -> List[dict]:
    keyword = (keyword or "").strip()

    if not keyword:
        return []

    # 第一层：先用原句直接搜索
    direct_articles = search_wiki_articles(keyword, limit=limit)

    if direct_articles:
        return normalize_wiki_articles_for_ai(direct_articles)

    # 第二层：原句搜不到，再拆动态搜索词
    terms = build_wiki_search_terms(
        text=keyword,
        detected_domains=detected_domains,
        max_terms=8,
    )

    if not terms:
        return []

    wiki_filter = build_wiki_or_filter_for_terms(terms)


    data = pocketbase_get_records(
        POCKETBASE_WIKI_ARTICLES,
        params={
            "page": 1,
            "perPage": limit,
            "sort": "-updated",
            "filter": wiki_filter,
        }
    )

    articles = [
        normalize_wiki_article(item)
        for item in data.get("items", [])
    ]

    return normalize_wiki_articles_for_ai(articles)


def build_wiki_context_for_ai(wiki_articles: list[dict]) -> str:
    if not wiki_articles:
        return ""

    blocks = []

    for index, article in enumerate(wiki_articles, start=1):
        blocks.append(
            "\n".join([
                f"【Wiki {index}】{article.get('title', '')}",
                f"摘要：{article.get('summary', '')}",
                f"风险等级：{article.get('risk_level', 'normal')}",
                f"标签：{article.get('tags', '')}",
                f"来源：{article.get('source', '')}",
                f"正文摘录：{article.get('content', '')}",
            ])
        )

    return "\n\n".join(blocks)

def extract_wiki_keywords(text: str) -> list[str]:
    text = (text or "").strip()

    candidates = [
        "饮用水", "储水", "停水", "水源", "净水",
        "移动电源", "充电宝", "电量", "低电量", "手机", "充电",
        "种植", "土豆", "番茄", "种子",
        "工具", "维修", "手锯", "螺丝刀",
        "照明", "手电", "通讯", "导航",
    ]

    keywords = []

    for word in candidates:
        if word in text:
            keywords.append(word)

    if not keywords and text:
        keywords.append(text[:20])

    return keywords[:5]

def clean_wiki_search_term(term: str) -> str:
    term = (term or "").strip()
    term = re.sub(r"[，。！？、；：,.!?;:\s]+", "", term)
    return term


def build_wiki_search_terms(
    text: str,
    detected_domains: Optional[List[str]] = None,
    max_terms: int = 8,
) -> List[str]:
    text = (text or "").strip()
    detected_domains = detected_domains or []

    terms: List[str] = []

    def add_term(term: str):
        term = clean_wiki_search_term(term)

        if not term:
            return

        if term in WIKI_STOP_TERMS:
            return

        if len(term) < 2:
            return

        if term not in terms:
            terms.append(term)

    # 1. 领域关键词优先：只加入用户句子中实际出现的领域词
    for domain in detected_domains:
        for word in WIKI_DOMAIN_TERMS.get(domain, []):
            if word in text:
                add_term(word)

    # 2. 通用领域词：即使 detected_domains 没传，也从全文里找明显词
    for words in WIKI_DOMAIN_TERMS.values():
        for word in words:
            if word in text:
                add_term(word)

    # 3. 提取连续中文片段
    chinese_chunks = re.findall(r"[\u4e00-\u9fff]{2,12}", text)

    for chunk in chinese_chunks:
        if len(chunk) <= 6:
            add_term(chunk)
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
        safe_term = term.replace('"', '\\"')

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

def normalize_wiki_articles_for_ai(articles: List[dict]) -> List[dict]:
    results = []

    for article in articles:
        results.append({
            "id": article.get("id"),
            "title": article.get("title", ""),
            "summary": article.get("summary", ""),
            "content": trim_text(article.get("content", ""), 3000),
            "tags": article.get("tags", ""),
            "risk_level": article.get("risk_level", "normal"),
            "source": article.get("source", ""),
        })

    return results

def pb_get(path: str, params: Optional[dict] = None):
    url = f"{PB_URL}{path}"

    try:
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()
        return response.json()
    except requests.RequestException as exc:
        raise HTTPException(
            status_code=502,
            detail=f"PocketBase 请求失败：{exc}"
        )