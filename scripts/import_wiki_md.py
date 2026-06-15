#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LanternBox Wiki Markdown importer v0.1

用法：
  python scripts/import_wiki_md.py wiki_import/medical/basic-wound-cleaning-and-observation.md
  python scripts/import_wiki_md.py wiki_import

环境变量：
  PB_URL=http://127.0.0.1:8090
  PB_ADMIN_EMAIL=你的PocketBase管理员邮箱
  PB_ADMIN_PASSWORD=你的PocketBase管理员密码

说明：
  - Markdown 文件需要使用 front matter：--- ... ---
  - slug 已存在则更新，不存在则新建
  - category 字段会按 wiki_categories.name 查找，并写入 relation id
"""

from __future__ import annotations

import json
import os
import sys
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any, Dict, Iterable, List, Tuple

PB_URL = os.environ.get("PB_URL", "http://127.0.0.1:8090").rstrip("/")
PB_ADMIN_EMAIL = os.environ.get("PB_ADMIN_EMAIL", "")
PB_ADMIN_PASSWORD = os.environ.get("PB_ADMIN_PASSWORD", "")

ARTICLES_COLLECTION = os.environ.get("PB_ARTICLES_COLLECTION", "wiki_articles")
CATEGORIES_COLLECTION = os.environ.get("PB_CATEGORIES_COLLECTION", "wiki_categories")

REQUIRED_META = ["title", "slug", "category", "summary", "tags", "risk_level", "status", "source"]


def http_json(method: str, url: str, payload: Dict[str, Any] | None = None, token: str | None = None) -> Dict[str, Any]:
    data = None
    headers = {"Accept": "application/json"}

    if payload is not None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        headers["Content-Type"] = "application/json"

    if token:
        headers["Authorization"] = f"Bearer {token}"

    request = urllib.request.Request(url, data=data, headers=headers, method=method)

    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            raw = response.read().decode("utf-8")
            return json.loads(raw) if raw else {}
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"HTTP {error.code} {url}\n{body}") from error


def auth_admin() -> str:
    if not PB_ADMIN_EMAIL or not PB_ADMIN_PASSWORD:
        raise RuntimeError(
            "缺少 PocketBase 管理员环境变量。请先执行：\n"
            "export PB_ADMIN_EMAIL='你的管理员邮箱'\n"
            "export PB_ADMIN_PASSWORD='你的管理员密码'"
        )

    url = f"{PB_URL}/api/collections/_superusers/auth-with-password"
    data = http_json("POST", url, {
        "identity": PB_ADMIN_EMAIL,
        "password": PB_ADMIN_PASSWORD,
    })

    token = data.get("token")
    if not token:
        raise RuntimeError("PocketBase 登录成功但没有返回 token")

    return token


def parse_front_matter(path: Path) -> Tuple[Dict[str, str], str]:
    text = path.read_text(encoding="utf-8")

    if not text.startswith("---"):
        raise ValueError(f"{path} 缺少 front matter 起始 ---")

    parts = text.split("---", 2)
    if len(parts) < 3:
        raise ValueError(f"{path} front matter 格式不完整")

    meta_text = parts[1].strip()
    content = parts[2].lstrip("\n")

    meta: Dict[str, str] = {}
    for line in meta_text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        meta[key.strip()] = value.strip().strip('"').strip("'")

    missing = [key for key in REQUIRED_META if not meta.get(key)]
    if missing:
        raise ValueError(f"{path} 缺少字段：{', '.join(missing)}")

    if not content.strip():
        raise ValueError(f"{path} 正文为空")

    return meta, content


def pb_filter(expression: str) -> str:
    return urllib.parse.urlencode({"filter": expression})


def get_first_record(collection: str, filter_expression: str, token: str) -> Dict[str, Any] | None:
    url = f"{PB_URL}/api/collections/{collection}/records?perPage=1&{pb_filter(filter_expression)}"
    data = http_json("GET", url, token=token)
    items = data.get("items") or []
    return items[0] if items else None


def find_category_id(category_name: str, token: str) -> str:
    safe_name = category_name.replace('"', '\\"')
    record = get_first_record(CATEGORIES_COLLECTION, f'name="{safe_name}"', token)
    if not record:
        raise RuntimeError(f"找不到分类：{category_name}。请先在 wiki_categories 中创建该分类。")
    return record["id"]


def find_article_by_slug(slug: str, token: str) -> Dict[str, Any] | None:
    safe_slug = slug.replace('"', '\\"')
    return get_first_record(ARTICLES_COLLECTION, f'slug="{safe_slug}"', token)


def upsert_article(path: Path, token: str) -> str:
    meta, content = parse_front_matter(path)
    category_id = find_category_id(meta["category"], token)

    payload = {
        "title": meta["title"],
        "slug": meta["slug"],
        "category": category_id,
        "summary": meta["summary"],
        "content": content.strip(),
        "tags": meta["tags"],
        "risk_level": meta["risk_level"],
        "status": meta["status"],
        "source": meta["source"],
    }

    existing = find_article_by_slug(meta["slug"], token)

    if existing:
        url = f"{PB_URL}/api/collections/{ARTICLES_COLLECTION}/records/{existing['id']}"
        http_json("PATCH", url, payload, token=token)
        return f"更新：{meta['title']} ({meta['slug']})"

    url = f"{PB_URL}/api/collections/{ARTICLES_COLLECTION}/records"
    http_json("POST", url, payload, token=token)
    return f"新建：{meta['title']} ({meta['slug']})"


def iter_markdown_files(target: Path) -> Iterable[Path]:
    if target.is_file():
        if target.suffix.lower() == ".md":
            yield target
        return

    for path in sorted(target.rglob("*.md")):
        if path.name.startswith("WIKI_CONTENT_STANDARD"):
            continue
        yield path


def main() -> int:
    if len(sys.argv) < 2:
        print("用法：python scripts/import_wiki_md.py <markdown文件或目录>")
        return 2

    target = Path(sys.argv[1]).expanduser().resolve()
    if not target.exists():
        print(f"路径不存在：{target}")
        return 2

    token = auth_admin()
    files = list(iter_markdown_files(target))

    if not files:
        print("没有找到 Markdown 文件。")
        return 0

    ok_count = 0
    fail_count = 0

    for path in files:
        try:
            result = upsert_article(path, token)
            print(f"✅ {result}")
            ok_count += 1
        except Exception as error:
            print(f"❌ {path}: {error}")
            fail_count += 1

    print(f"\n完成：成功 {ok_count}，失败 {fail_count}")
    return 1 if fail_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
