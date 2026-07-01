"""Wiki 页面与详情辅助接口。AI 检索逻辑已迁移到 Wiki Service。"""

from typing import Any, Dict, Optional

from .config import (
    POCKETBASE_URL,
    POCKETBASE_WIKI_ARTICLES,
    POCKETBASE_WIKI_MEDIA,
)

from .pocketbase_client import pocketbase_get_records, pocketbase_get_record
from .services.wiki_service import normalize_wiki_article


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
