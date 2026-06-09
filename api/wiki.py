import requests
from typing import Any, Dict, Optional

from .config import (
    POCKETBASE_URL,
    POCKETBASE_WIKI_ARTICLES,
    POCKETBASE_WIKI_MEDIA,
)


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
            "filter": 'status = "published"',
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