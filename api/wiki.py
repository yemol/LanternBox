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

from .pocketbase_client import pocketbase_get_records



PB_URL = os.getenv("PB_URL", "http://127.0.0.1:8090")


def pocketbase_get_record(collection: str, record_id: str) -> Optional[dict]:
    url = f"{POCKETBASE_URL}/api/collections/{collection}/records/{record_id}"

    try:
        response = requests.get(url, timeout=8)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"PocketBase 读取单条失败: {collection}/{record_id}", e)
        return None


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


def trim_text(text: str, max_chars: int = 800) -> str:
    text = (text or "").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "……"


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