"""PocketBase 统一访问客户端。集中封装 records 与 record 请求。"""

import math
import re
import sqlite3
from typing import Any, Dict, Optional

import requests
from fastapi import HTTPException

from .config import POCKETBASE_DATA_DB_PATH, POCKETBASE_URL


LOCAL_POCKETBASE_COLLECTIONS = {
    "wiki_articles",
    "wiki_categories",
    "wiki_media",
}


def pb_get(path: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    url = f"{POCKETBASE_URL}{path}"

    try:
        response = requests.get(url, params=params or {}, timeout=10)
        response.raise_for_status()
        return response.json()
    except requests.RequestException as exc:
        raise HTTPException(
            status_code=502,
            detail=f"PocketBase 请求失败：{exc}",
        )


def _local_pb_available() -> bool:
    return POCKETBASE_DATA_DB_PATH.exists()


def _connect_local_pb() -> sqlite3.Connection:
    db_uri = f"file:{POCKETBASE_DATA_DB_PATH}?mode=ro"
    connection = sqlite3.connect(db_uri, timeout=10, uri=True)
    connection.row_factory = sqlite3.Row
    return connection


def _get_table_columns(connection: sqlite3.Connection, collection: str) -> set[str]:
    rows = connection.execute(f'pragma table_info("{collection}")').fetchall()
    return {str(row["name"]) for row in rows}


def _extract_filter_values(filter_text: str, field: str) -> list[str]:
    pattern = rf'\b{re.escape(field)}\s*=\s*"([^"]+)"'
    return re.findall(pattern, filter_text or "")


def _extract_search_terms(filter_text: str) -> list[str]:
    terms = re.findall(r'\b(?:title|summary|content|tags)\s*~\s*"([^"]*)"', filter_text or "")
    deduped: list[str] = []

    for term in terms:
        if term and term not in deduped:
            deduped.append(term)

    return deduped


def _build_local_where(collection: str, filter_text: str) -> tuple[str, list[Any]]:
    clauses: list[str] = []
    args: list[Any] = []

    if not filter_text:
        return "", args

    for status in _extract_filter_values(filter_text, "status"):
        clauses.append("status = ?")
        args.append(status)

    if collection == "wiki_articles":
        for category in _extract_filter_values(filter_text, "category"):
            clauses.append("category = ?")
            args.append(category)

        search_terms = _extract_search_terms(filter_text)
        if search_terms:
            term_clauses: list[str] = []
            for term in search_terms:
                like_term = f"%{term}%"
                term_clauses.append("(title like ? or summary like ? or content like ? or tags like ?)")
                args.extend([like_term, like_term, like_term, like_term])
            clauses.append("(" + " or ".join(term_clauses) + ")")

    if collection == "wiki_media":
        for article_id in _extract_filter_values(filter_text, "article"):
            clauses.append("article = ?")
            args.append(article_id)

    if not clauses:
        return "", args

    return " where " + " and ".join(clauses), args


def _build_local_sort(sort_text: str, columns: set[str]) -> str:
    parts: list[str] = []

    for raw_part in (sort_text or "").split(","):
        part = raw_part.strip()
        if not part:
            continue

        direction = "asc"
        if part.startswith("-"):
            direction = "desc"
            part = part[1:].strip()

        if part in columns:
            parts.append(f'"{part}" {direction}')

    if not parts:
        return ""

    return " order by " + ", ".join(parts)


def _local_get_records(
    collection: str,
    params: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    if collection not in LOCAL_POCKETBASE_COLLECTIONS or not _local_pb_available():
        raise FileNotFoundError("本地 PocketBase 数据库不可用")

    params = params or {}
    page = max(int(params.get("page") or 1), 1)
    per_page = min(max(int(params.get("perPage") or 30), 1), 500)
    filter_text = str(params.get("filter") or "")
    sort_text = str(params.get("sort") or "")

    with _connect_local_pb() as connection:
        columns = _get_table_columns(connection, collection)
        if not columns:
            raise FileNotFoundError(f"本地 PocketBase 表不存在：{collection}")

        where_sql, args = _build_local_where(collection, filter_text)
        order_sql = _build_local_sort(sort_text, columns)
        offset = (page - 1) * per_page

        total_items = connection.execute(
            f'select count(*) from "{collection}"{where_sql}',
            args,
        ).fetchone()[0]
        rows = connection.execute(
            f'select * from "{collection}"{where_sql}{order_sql} limit ? offset ?',
            [*args, per_page, offset],
        ).fetchall()

    total_pages = math.ceil(total_items / per_page) if total_items else 0

    return {
        "page": page,
        "perPage": per_page,
        "totalItems": total_items,
        "totalPages": total_pages,
        "items": [dict(row) for row in rows],
    }


def _local_get_record(collection: str, record_id: str) -> Dict[str, Any]:
    if collection not in LOCAL_POCKETBASE_COLLECTIONS or not _local_pb_available():
        raise FileNotFoundError("本地 PocketBase 数据库不可用")

    with _connect_local_pb() as connection:
        columns = _get_table_columns(connection, collection)
        if not columns:
            raise FileNotFoundError(f"本地 PocketBase 表不存在：{collection}")

        row = connection.execute(
            f'select * from "{collection}" where id = ? limit 1',
            [record_id],
        ).fetchone()

    return dict(row) if row else {}


def pocketbase_get_records(
    collection: str,
    params: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    try:
        return pb_get(
            f"/api/collections/{collection}/records",
            params=params,
        )
    except HTTPException as exc:
        try:
            return _local_get_records(collection, params=params)
        except Exception:
            raise exc


def pocketbase_get_record(
    collection: str,
    record_id: str,
) -> Dict[str, Any]:
    try:
        return pb_get(
            f"/api/collections/{collection}/records/{record_id}",
        )
    except HTTPException as exc:
        try:
            return _local_get_record(collection, record_id)
        except Exception:
            raise exc
