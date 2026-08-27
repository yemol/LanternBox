"""Local ebook library service for PDF and EPUB files."""

from __future__ import annotations

import hashlib
import json
import mimetypes
import posixpath
import re
import shutil
import subprocess
import sys
import uuid
import zipfile
from datetime import datetime
from pathlib import Path
from typing import Any, AsyncIterable, Iterable
from urllib.parse import quote
from xml.etree import ElementTree

from bs4 import BeautifulSoup
from ..config import (
    LIBRARY_ARCHIVE_DIR,
    LIBRARY_BOOKS_DIR,
    LIBRARY_EPUB_CACHE_DIR,
    LIBRARY_INDEX_FILE,
    LIBRARY_MANUAL_OUTLINES_FILE,
    LIBRARY_PDF_CACHE_DIR,
)

ALLOWED_BOOK_EXTENSIONS = {".pdf", ".epub"}
EPUB_HTML_MEDIA_TYPES = {
    "application/xhtml+xml",
    "text/html",
    "application/x-dtbook+xml",
}
EPUB_IMAGE_MEDIA_PREFIX = "image/"


class LibraryError(Exception):
    """Expected user-facing library error."""


class BookNotFoundError(LibraryError):
    """Raised when a requested book does not exist."""


class UnsupportedBookFormatError(LibraryError):
    """Raised when an uploaded or scanned file is not a supported ebook."""


def _now_iso() -> str:
    return datetime.now().replace(microsecond=0).isoformat()


def _load_index() -> dict[str, Any]:
    if not LIBRARY_INDEX_FILE.exists():
        return {"books": []}

    try:
        with LIBRARY_INDEX_FILE.open("r", encoding="utf-8") as file:
            data = json.load(file)
    except Exception:
        return {"books": []}

    if not isinstance(data, dict) or not isinstance(data.get("books"), list):
        return {"books": []}
    return data


def _save_index(data: dict[str, Any]) -> None:
    LIBRARY_INDEX_FILE.parent.mkdir(parents=True, exist_ok=True)
    with LIBRARY_INDEX_FILE.open("w", encoding="utf-8") as file:
        json.dump(data, file, ensure_ascii=False, indent=2)


def _book_format(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix == ".pdf":
        return "pdf"
    if suffix == ".epub":
        return "epub"
    raise UnsupportedBookFormatError("仅支持 PDF 和 EPUB 电子书。")


def _safe_text(value: str | None, fallback: str = "") -> str:
    text = str(value if value is not None else fallback).strip()
    if text.lower() in {"none", "null", "undefined"}:
        text = ""
    if not text and fallback and fallback is not value:
        text = str(fallback).strip()
        if text.lower() in {"none", "null", "undefined"}:
            return ""
    return text


def _safe_stem(value: str) -> str:
    stem = re.sub(r"[^0-9A-Za-z._\u4e00-\u9fff-]+", "-", value).strip(".-_")
    return stem[:80] or "book"


def _stable_scanned_id(filename: str) -> str:
    digest = hashlib.sha1(filename.encode("utf-8")).hexdigest()[:16]
    return f"scan-{digest}"


def _public_book(record: dict[str, Any]) -> dict[str, Any]:
    public = dict(record)
    public.pop("filename", None)
    public.pop("metadata_locked", None)
    for field in ["title", "author", "category", "description", "original_filename"]:
        if field in public:
            fallback = "未分类" if field == "category" else ""
            public[field] = _safe_text(public.get(field), fallback)
    public["file_url"] = f"/api/library/books/{record['id']}/file"
    return public


def _record_base_dir(record: dict[str, Any]) -> Path:
    if record.get("archived") or record.get("storage") == "archive":
        return LIBRARY_ARCHIVE_DIR
    return LIBRARY_BOOKS_DIR


def _record_path(record: dict[str, Any]) -> Path:
    filename = str(record.get("filename") or "")
    base_dir = _record_base_dir(record).resolve()
    path = (base_dir / filename).resolve()
    if base_dir not in path.parents and path != base_dir:
        raise BookNotFoundError("电子书路径无效。")
    return path


def _extract_epub_metadata(path: Path) -> dict[str, str]:
    try:
        with zipfile.ZipFile(path) as archive:
            opf_path = _find_opf_path(archive)
            if not opf_path:
                return {}
            root = ElementTree.fromstring(archive.read(opf_path))
    except Exception:
        return {}

    ns = {
        "opf": "http://www.idpf.org/2007/opf",
        "dc": "http://purl.org/dc/elements/1.1/",
    }

    def first_text(xpath: str) -> str:
        node = root.find(xpath, ns)
        return (node.text or "").strip() if node is not None and node.text else ""

    return {
        "title": first_text(".//dc:title"),
        "author": first_text(".//dc:creator"),
        "language": first_text(".//dc:language"),
    }


def _build_record_for_file(path: Path, book_id: str | None = None, metadata: dict[str, str] | None = None) -> dict[str, Any]:
    book_format = _book_format(path)
    file_metadata = _extract_epub_metadata(path) if book_format == "epub" else {}
    metadata = metadata or {}
    stat = path.stat()
    title = _safe_text(metadata.get("title"), file_metadata.get("title") or _display_title_from_filename(path.name))

    return {
        "id": book_id or _book_id_from_filename(path.name) or _stable_scanned_id(path.name),
        "title": title,
        "author": _safe_text(metadata.get("author"), file_metadata.get("author")),
        "category": _safe_text(metadata.get("category"), "未分类"),
        "description": _safe_text(metadata.get("description")),
        "format": book_format,
        "filename": path.name,
        "original_filename": metadata.get("original_filename") or path.name,
        "size_bytes": stat.st_size,
        "created_at": metadata.get("created_at") or _now_iso(),
        "updated_at": _now_iso(),
        "archived": False,
        "storage": "books",
    }


def _book_id_from_filename(filename: str) -> str | None:
    match = re.match(r"^([0-9a-f]{32})-", filename, re.IGNORECASE)
    return match.group(1).lower() if match else None


def _display_title_from_filename(filename: str) -> str:
    stem = Path(filename).stem
    stem = re.sub(r"^[0-9a-f]{32}-", "", stem, flags=re.IGNORECASE)
    stem = re.sub(r"-(?:z-library\.sk|1lib\.sk|z-lib\.sk).*$", "", stem, flags=re.IGNORECASE)
    stem = re.sub(r"[\s_-]+$", "", stem).strip()

    stop_pattern = re.compile(
        r"^(?:第?[0-9一二三四五六七八九十百]+版|[中英美日德法俄奥]+|.*(?:主编|编著|组织编写|著|译|师傅|中心|出版社|z-lib).*)$",
        re.IGNORECASE,
    )
    title_parts = []
    for part in stem.split("-"):
        part = part.strip()
        if not part:
            continue
        if title_parts and stop_pattern.match(part):
            break
        title_parts.append(part)

    title = "-".join(title_parts).strip()
    return title or stem or Path(filename).stem


def _is_auto_generated_title(record: dict[str, Any]) -> bool:
    title = str(record.get("title") or "").strip()
    filename = str(record.get("filename") or "")
    if not title or not filename:
        return False

    file_stem = Path(filename).stem
    parsed_id = _book_id_from_filename(filename) or ""
    return (
        title == file_stem
        or bool(parsed_id and title.startswith(f"{parsed_id}-"))
        or (str(record.get("id") or "").startswith("scan-") and title == _display_title_from_filename(filename))
    )


def _record_dedupe_score(record: dict[str, Any]) -> tuple[int, str]:
    book_id = str(record.get("id") or "")
    created_at = str(record.get("created_at") or "")
    updated_at = str(record.get("updated_at") or "")
    score = 0
    if not book_id.startswith("scan-"):
        score += 20
    if updated_at and updated_at != created_at:
        score += 10
    if record.get("description"):
        score += 3
    if record.get("author"):
        score += 2
    if record.get("category") and record.get("category") != "未分类":
        score += 1
    return score, updated_at


def _dedupe_records(records: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    by_file: dict[tuple[str, str], dict[str, Any]] = {}
    order: list[tuple[str, str]] = []
    for record in records:
        filename = str(record.get("filename") or "")
        storage = str(record.get("storage") or ("archive" if record.get("archived") else "books"))
        key = (storage, filename)
        if not filename:
            continue
        current = by_file.get(key)
        if current is None:
            by_file[key] = record
            order.append(key)
            continue
        if _record_dedupe_score(record) > _record_dedupe_score(current):
            by_file[key] = record
    return [by_file[key] for key in order]


def sync_library_index() -> list[dict[str, Any]]:
    LIBRARY_BOOKS_DIR.mkdir(parents=True, exist_ok=True)
    LIBRARY_ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    data = _load_index()
    records = []

    for record in data.get("books", []):
        if not isinstance(record, dict):
            continue
        try:
            refreshed = dict(record)
            refreshed["archived"] = bool(refreshed.get("archived"))
            refreshed["storage"] = "archive" if refreshed["archived"] else "books"
            was_scan_id = str(refreshed.get("id") or "").startswith("scan-")
            parsed_id = _book_id_from_filename(str(refreshed.get("filename") or ""))
            if parsed_id and was_scan_id:
                refreshed["id"] = parsed_id
            path = _record_path(refreshed)
            if not path.exists() or path.suffix.lower() not in ALLOWED_BOOK_EXTENSIONS:
                continue
            refreshed["format"] = _book_format(path)
            refreshed["size_bytes"] = path.stat().st_size
            for field in ["author", "description"]:
                refreshed[field] = _safe_text(refreshed.get(field))
            refreshed["category"] = _safe_text(refreshed.get("category"), "未分类") or "未分类"
            if not refreshed.get("metadata_locked") and (_is_auto_generated_title(refreshed) or (was_scan_id and refreshed["format"] == "pdf")):
                refreshed["title"] = _display_title_from_filename(path.name)
            refreshed["title"] = _safe_text(refreshed.get("title"), _display_title_from_filename(path.name))
            if refreshed.get("filename"):
                records.append(refreshed)
        except LibraryError:
            continue

    indexed_active_filenames = {
        record["filename"]
        for record in records
        if not record.get("archived") and record.get("filename")
    }
    for path in sorted(LIBRARY_BOOKS_DIR.iterdir()):
        if not path.is_file() or path.suffix.lower() not in ALLOWED_BOOK_EXTENSIONS:
            continue
        if path.name in indexed_active_filenames:
            continue
        records.append(_build_record_for_file(path))

    records = _dedupe_records(records)
    records.sort(key=lambda item: item.get("created_at") or "", reverse=True)
    data["books"] = records
    _save_index(data)
    return records


def list_books(q: str = "", include_archived: bool = False) -> list[dict[str, Any]]:
    keyword = q.strip().lower()
    records = [
        record
        for record in sync_library_index()
        if include_archived or not record.get("archived")
    ]
    books = [_public_book(record) for record in records]
    if not keyword:
        return books

    def matches(book: dict[str, Any]) -> bool:
        haystack = " ".join(
            str(book.get(field) or "")
            for field in ["title", "author", "category", "description", "original_filename", "format"]
        ).lower()
        return keyword in haystack

    return [book for book in books if matches(book)]


def get_book_record(book_id: str, include_archived: bool = False) -> dict[str, Any]:
    for record in sync_library_index():
        if record.get("id") == book_id and (include_archived or not record.get("archived")):
            return record
    raise BookNotFoundError("没有找到这本电子书。")


def get_book(book_id: str) -> dict[str, Any]:
    return _public_book(get_book_record(book_id))


def update_book_metadata(book_id: str, patch: dict[str, Any]) -> dict[str, Any]:
    records = sync_library_index()
    target = None
    for record in records:
        if record.get("id") == book_id:
            target = record
            break

    if not target and book_id.startswith("scan-"):
        for record in records:
            filename = str(record.get("filename") or "")
            if _stable_scanned_id(filename) == book_id and not record.get("archived"):
                target = record
                break

    if not target:
        raise BookNotFoundError("没有找到这本电子书。")

    for field in ["title", "author", "category", "description"]:
        if field not in patch or patch[field] is None:
            continue
        value = str(patch[field]).strip()
        if field == "title" and not value:
            value = Path(str(target.get("original_filename") or target.get("filename") or "未命名电子书")).stem
        if field == "category" and not value:
            value = "未分类"
        target[field] = value

    target["updated_at"] = _now_iso()
    target["metadata_locked"] = True
    _save_index({"books": records})
    return _public_book(target)


def get_book_file_path(book_id: str) -> Path:
    record = get_book_record(book_id)
    path = _record_path(record)
    if not path.exists():
        raise BookNotFoundError("电子书文件不存在。")
    return path


def _get_pdf_record_and_path(book_id: str) -> tuple[dict[str, Any], Path]:
    record = get_book_record(book_id)
    if record.get("format") != "pdf":
        raise UnsupportedBookFormatError("这本书不是 PDF。")

    path = _record_path(record)
    if not path.exists():
        raise BookNotFoundError("PDF 文件不存在。")
    return record, path


def _find_pdf_tool(command: str) -> str:
    candidates = [
        shutil.which(command),
        f"/Users/yemol/.cache/codex-runtimes/codex-primary-runtime/dependencies/bin/override/{command}",
        f"/opt/homebrew/bin/{command}",
        f"/usr/local/bin/{command}",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return str(candidate)
    raise LibraryError(f"缺少 PDF 渲染工具：{command}。请安装 Poppler 后再打开 PDF。")


def get_pdf_info(book_id: str) -> dict[str, Any]:
    record, path = _get_pdf_record_and_path(book_id)
    pdfinfo = _find_pdf_tool("pdfinfo")
    try:
        result = subprocess.run(
            [pdfinfo, str(path)],
            check=True,
            capture_output=True,
            text=True,
            timeout=20,
        )
    except subprocess.TimeoutExpired:
        raise LibraryError("PDF 信息读取超时。")
    except subprocess.CalledProcessError as error:
        detail = (error.stderr or error.stdout or "").strip()
        raise LibraryError(detail or "PDF 信息读取失败。")

    page_count = 0
    for line in result.stdout.splitlines():
        if line.lower().startswith("pages:"):
            try:
                page_count = int(line.split(":", 1)[1].strip())
            except ValueError:
                page_count = 0
            break

    if page_count <= 0:
        raise LibraryError("无法识别 PDF 页数。")

    return {"book": _public_book(record), "page_count": page_count}


def _flatten_pdf_outline_with_pypdf(path: Path, max_items: int = 600) -> list[dict[str, Any]]:
    from pypdf import PdfReader

    reader = PdfReader(str(path))
    outline = getattr(reader, "outline", []) or []
    items: list[dict[str, Any]] = []

    def add_items(nodes: Any, level: int = 0) -> None:
        if len(items) >= max_items:
            return
        if not isinstance(nodes, list):
            nodes = [nodes]

        for node in nodes:
            if len(items) >= max_items:
                return
            if isinstance(node, list):
                add_items(node, level + 1)
                continue

            title = str(getattr(node, "title", "") or "").strip()
            if not title and hasattr(node, "get"):
                title = str(node.get("/Title") or "").strip()
            if not title:
                title = "未命名目录"

            try:
                page = int(reader.get_destination_page_number(node)) + 1
            except Exception:
                page = 0

            if page > 0:
                items.append({
                    "title": title[:120],
                    "page": page,
                    "level": max(0, min(level, 6)),
                })

    add_items(outline)
    return items


def _find_pdf_python_runtime() -> str | None:
    candidates = [
        sys.executable,
        "/Users/yemol/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3",
    ]
    for candidate in candidates:
        if not candidate or not Path(candidate).exists():
            continue
        try:
            subprocess.run(
                [candidate, "-c", "import pypdf"],
                check=True,
                capture_output=True,
                text=True,
                timeout=5,
            )
        except Exception:
            continue
        return candidate
    return None


def _flatten_pdf_outline_with_worker(path: Path, max_items: int = 600) -> list[dict[str, Any]]:
    python = _find_pdf_python_runtime()
    if not python:
        return []

    worker = """
import json
import sys
from pathlib import Path
from typing import Any
from pypdf import PdfReader

pdf_path = Path(sys.argv[1])
max_items = int(sys.argv[2])
reader = PdfReader(str(pdf_path))
outline = getattr(reader, 'outline', []) or []
items = []

def add_items(nodes: Any, level: int = 0) -> None:
    if len(items) >= max_items:
        return
    if not isinstance(nodes, list):
        nodes = [nodes]
    for node in nodes:
        if len(items) >= max_items:
            return
        if isinstance(node, list):
            add_items(node, level + 1)
            continue
        title = str(getattr(node, 'title', '') or '').strip()
        if not title and hasattr(node, 'get'):
            title = str(node.get('/Title') or '').strip()
        if not title:
            title = '未命名目录'
        try:
            page = int(reader.get_destination_page_number(node)) + 1
        except Exception:
            page = 0
        if page > 0:
            items.append({'title': title[:120], 'page': page, 'level': max(0, min(level, 6))})

add_items(outline)
print(json.dumps(items, ensure_ascii=False))
"""

    try:
        result = subprocess.run(
            [python, "-c", worker, str(path), str(max_items)],
            check=True,
            capture_output=True,
            text=True,
            timeout=35,
        )
    except Exception:
        return []

    try:
        data = json.loads(result.stdout or "[]")
    except json.JSONDecodeError:
        return []
    return data if isinstance(data, list) else []


def _is_page_bookmark_title(title: str) -> bool:
    compact = re.sub(r"\s+", "", title).lower()
    return bool(re.fullmatch(r"(?:第)?页\d+|page\d+|p\.?\d+", compact))


def _is_bad_pdf_outline_title(title: str) -> bool:
    compact = re.sub(r"[\s\x00]+", "", title).lower()
    if not compact or len(compact) <= 1:
        return True
    if _is_page_bookmark_title(title):
        return True
    if re.fullmatch(r"\d{1,4}", compact):
        return True
    if re.search(r"\d{2,4}x\d{2,4}mm", compact):
        return True
    bad_terms = ["封面", "书名", "版权", "版权页", "内封", "前言", "目录", "完稿", "封面封底", "出血位", "书脊", "成品"]
    if compact in bad_terms or compact.startswith("版权"):
        return True
    return any(term in compact for term in bad_terms[7:])


def _clean_pdf_outline(items: list[dict[str, Any]]) -> list[dict[str, Any]]:
    normalized = []
    for item in items:
        title = str(item.get("title") or "").replace("\x00", "").strip()
        page = int(item.get("page") or 0)
        level = int(item.get("level") or 0)
        if title and page > 0 and not _is_bad_pdf_outline_title(title):
            normalized.append({"title": title[:120], "page": page, "level": max(0, min(level, 6))})

    if not normalized:
        return []

    min_level = min(item["level"] for item in normalized)
    for item in normalized:
        item["level"] = max(0, item["level"] - min_level)
    return normalized


def _extract_pdf_heading_outline_with_worker(path: Path, max_items: int = 240) -> list[dict[str, Any]]:
    python = _find_pdf_python_runtime()
    if not python:
        return []

    worker = """
import json
import re
import sys
from pathlib import Path
from pypdf import PdfReader

pdf_path = Path(sys.argv[1])
max_items = int(sys.argv[2])
heading_pattern = re.compile(r'(第[一二三四五六七八九十百零〇两]+[篇章节])\s*([^\s|，。；：:、（）()]{2,28})')
items = []
seen_keys = set()
reader = PdfReader(str(pdf_path))

for page_index, page in enumerate(reader.pages):
    if len(items) >= max_items:
        break
    try:
        text = page.extract_text() or ''
    except Exception:
        continue
    for line in text.splitlines():
        for match in heading_pattern.finditer(line):
            key = match.group(1)
            if key in seen_keys:
                continue
            title = f'{key} {match.group(2)}'.strip()
            seen_keys.add(key)
            items.append({'title': title[:120], 'page': page_index + 1, 'level': 0})
            break

print(json.dumps(items, ensure_ascii=False))
"""

    try:
        result = subprocess.run(
            [python, "-c", worker, str(path), str(max_items)],
            check=True,
            capture_output=True,
            text=True,
            timeout=60,
        )
    except Exception:
        return []

    try:
        data = json.loads(result.stdout or "[]")
    except json.JSONDecodeError:
        return []
    return data if isinstance(data, list) else []


def _load_manual_pdf_outline(record: dict[str, Any]) -> list[dict[str, Any]]:
    if not LIBRARY_MANUAL_OUTLINES_FILE.exists():
        return []

    try:
        data = json.loads(LIBRARY_MANUAL_OUTLINES_FILE.read_text(encoding="utf-8"))
    except Exception:
        return []

    if not isinstance(data, dict):
        return []

    books = data.get("books") if isinstance(data.get("books"), dict) else data
    candidates = [str(record.get("id") or ""), str(record.get("title") or ""), str(record.get("original_filename") or "")]
    items: Any = []
    for key in candidates:
        if key and isinstance(books.get(key), list):
            items = books[key]
            break

    return _clean_pdf_outline(items if isinstance(items, list) else [])


def get_pdf_outline(book_id: str) -> dict[str, Any]:
    record, path = _get_pdf_record_and_path(book_id)
    manual_outline = _load_manual_pdf_outline(record)
    if len(manual_outline) >= 2:
        return {
            "book": _public_book(record),
            "outline": manual_outline,
            "total": len(manual_outline),
            "source": "manual",
        }

    stat = path.stat()
    cache_dir = LIBRARY_PDF_CACHE_DIR / str(record["id"])
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_path = cache_dir / f"outline-v3-{stat.st_mtime_ns}-{stat.st_size}.json"

    if cache_path.exists():
        try:
            cached = json.loads(cache_path.read_text(encoding="utf-8"))
            if isinstance(cached, dict) and isinstance(cached.get("outline"), list):
                return {"book": _public_book(record), **cached}
        except Exception:
            pass

    try:
        bookmark_outline = _flatten_pdf_outline_with_pypdf(path)
    except ImportError:
        bookmark_outline = _flatten_pdf_outline_with_worker(path)
    except Exception:
        bookmark_outline = []
    bookmark_outline = _clean_pdf_outline(bookmark_outline)

    if len(bookmark_outline) >= 6:
        outline = bookmark_outline
        source = "bookmarks"
    else:
        outline = _extract_pdf_heading_outline_with_worker(path)
        source = "headings" if len(outline) >= 3 else "bookmarks"
        if len(outline) < 3:
            outline = bookmark_outline

    if len(outline) < 2:
        outline = []
        source = "none"

    payload = {
        "outline": outline,
        "total": len(outline),
        "source": source,
    }
    try:
        cache_path.write_text(json.dumps(payload, ensure_ascii=False), encoding="utf-8")
    except Exception:
        pass
    return {"book": _public_book(record), **payload}


def get_pdf_page_image_path(book_id: str, page: int, width: int = 1500) -> Path:
    record, path = _get_pdf_record_and_path(book_id)
    page_count = int(get_pdf_info(book_id)["page_count"])
    if page < 1 or page > page_count:
        raise BookNotFoundError("没有找到这个 PDF 页面。")

    width = max(900, min(int(width or 1500), 3600))
    stat = path.stat()
    cache_dir = LIBRARY_PDF_CACHE_DIR / str(record["id"])
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_path = cache_dir / f"page-{page}-w{width}-{stat.st_mtime_ns}-{stat.st_size}.png"
    if cache_path.exists() and cache_path.stat().st_size > 0:
        return cache_path

    pdftoppm = _find_pdf_tool("pdftoppm")
    prefix = cache_dir / f"render-{page}-w{width}-{uuid.uuid4().hex}"
    output_path = prefix.with_suffix(".png")
    try:
        subprocess.run(
            [
                pdftoppm,
                "-q",
                "-f",
                str(page),
                "-l",
                str(page),
                "-singlefile",
                "-scale-to",
                str(width),
                "-png",
                str(path),
                str(prefix),
            ],
            check=True,
            capture_output=True,
            text=True,
            timeout=45,
        )
    except subprocess.TimeoutExpired:
        output_path.unlink(missing_ok=True)
        raise LibraryError("PDF 页面渲染超时。")
    except subprocess.CalledProcessError as error:
        output_path.unlink(missing_ok=True)
        detail = (error.stderr or error.stdout or "").strip()
        raise LibraryError(detail or "PDF 页面渲染失败。")

    if not output_path.exists() or output_path.stat().st_size <= 0:
        raise LibraryError("PDF 页面渲染没有生成图片。")

    output_path.replace(cache_path)
    return cache_path


def _prepare_upload_target(original_filename: str) -> tuple[str, Path, str]:
    original_filename = Path(original_filename or "").name
    if not original_filename:
        raise UnsupportedBookFormatError("请提供 PDF 或 EPUB 文件名。")

    suffix = Path(original_filename).suffix.lower()
    if suffix not in ALLOWED_BOOK_EXTENSIONS:
        raise UnsupportedBookFormatError("仅支持 PDF 和 EPUB 电子书。")

    book_id = uuid.uuid4().hex
    stem = _safe_stem(Path(original_filename).stem)
    filename = f"{book_id}-{stem}{suffix}"
    target_path = LIBRARY_BOOKS_DIR / filename
    LIBRARY_BOOKS_DIR.mkdir(parents=True, exist_ok=True)
    return book_id, target_path, original_filename


def _save_new_book_record(
    target_path: Path,
    book_id: str,
    original_filename: str,
    title: str = "",
    author: str = "",
    category: str = "",
    description: str = "",
) -> dict[str, Any]:
    if target_path.stat().st_size <= 0:
        target_path.unlink(missing_ok=True)
        raise UnsupportedBookFormatError("上传文件为空。")

    metadata = {
        "title": title,
        "author": author,
        "category": category,
        "description": description,
        "original_filename": original_filename,
        "created_at": _now_iso(),
    }
    record = _build_record_for_file(target_path, book_id=book_id, metadata=metadata)

    data = _load_index()
    records = [item for item in data.get("books", []) if isinstance(item, dict)]
    records.insert(0, record)
    data["books"] = _dedupe_records(records)
    _save_index(data)
    return _public_book(record)


def create_book_from_bytes(
    content: bytes,
    original_filename: str,
    title: str = "",
    author: str = "",
    category: str = "",
    description: str = "",
) -> dict[str, Any]:
    if not content:
        raise UnsupportedBookFormatError("上传文件为空。")

    book_id, target_path, original_filename = _prepare_upload_target(original_filename)
    try:
        target_path.write_bytes(content)
    except Exception:
        target_path.unlink(missing_ok=True)
        raise

    return _save_new_book_record(target_path, book_id, original_filename, title, author, category, description)


async def create_book_from_stream(
    chunks: AsyncIterable[bytes],
    original_filename: str,
    title: str = "",
    author: str = "",
    category: str = "",
    description: str = "",
) -> dict[str, Any]:
    book_id, target_path, original_filename = _prepare_upload_target(original_filename)
    try:
        with target_path.open("wb") as target:
            async for chunk in chunks:
                if chunk:
                    target.write(chunk)
    except Exception:
        target_path.unlink(missing_ok=True)
        raise

    return _save_new_book_record(target_path, book_id, original_filename, title, author, category, description)


def archive_book(book_id: str) -> bool:
    records = sync_library_index()
    target = None
    for record in records:
        if record.get("id") == book_id and not record.get("archived"):
            target = record
            break

    if not target:
        return False

    source_path = _record_path(target)
    archive_filename = str(target.get("filename") or source_path.name)
    archive_path = (LIBRARY_ARCHIVE_DIR / archive_filename).resolve()
    archive_dir = LIBRARY_ARCHIVE_DIR.resolve()
    if archive_dir not in archive_path.parents and archive_path != archive_dir:
        raise BookNotFoundError("电子书归档路径无效。")

    LIBRARY_ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    if source_path.exists():
        if archive_path.exists():
            stem = archive_path.stem
            suffix = archive_path.suffix
            archive_path = archive_path.with_name(f"{stem}-{uuid.uuid4().hex[:8]}{suffix}")
            archive_filename = archive_path.name
        shutil.move(str(source_path), str(archive_path))

    target["filename"] = archive_filename
    target["archived"] = True
    target["storage"] = "archive"
    target["archived_at"] = _now_iso()
    target["updated_at"] = target["archived_at"]
    _save_index({"books": records})
    return True


def delete_book(book_id: str) -> bool:
    return archive_book(book_id)


def _find_opf_path(archive: zipfile.ZipFile) -> str | None:
    try:
        container = archive.read("META-INF/container.xml")
        root = ElementTree.fromstring(container)
        for node in root.iter():
            if node.tag.endswith("rootfile"):
                full_path = node.attrib.get("full-path")
                if full_path:
                    return full_path
    except Exception:
        pass

    for name in archive.namelist():
        if name.lower().endswith(".opf"):
            return name
    return None


def _zip_join(base_path: str, href: str) -> str:
    raw_href = href.split("#", 1)[0]
    joined = posixpath.normpath(posixpath.join(posixpath.dirname(base_path), raw_href))
    if joined.startswith("../") or joined == ".." or joined.startswith("/"):
        raise LibraryError("EPUB 内部资源路径无效。")
    return joined


def _find_ncx_path(archive: zipfile.ZipFile) -> str | None:
    for name in archive.namelist():
        if name.lower().endswith(".ncx"):
            return name
    return None


def _find_zip_tool() -> str | None:
    candidates = [
        shutil.which("zip"),
        "/usr/bin/zip",
        "/opt/homebrew/bin/zip",
        "/usr/local/bin/zip",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return str(candidate)
    return None


def _epub_has_readable_navigation(path: Path) -> bool:
    try:
        with zipfile.ZipFile(path) as archive:
            opf_path = _find_opf_path(archive)
            if opf_path:
                try:
                    archive.read(opf_path)
                    return True
                except Exception:
                    pass
            ncx_path = _find_ncx_path(archive)
            if ncx_path:
                archive.read(ncx_path)
                return True
    except Exception:
        return False
    return False


def _repair_epub_zip(path: Path, book_id: str) -> Path | None:
    zip_tool = _find_zip_tool()
    if not zip_tool:
        return None

    try:
        stat = path.stat()
    except OSError:
        return None

    cache_dir = LIBRARY_EPUB_CACHE_DIR / book_id
    cache_dir.mkdir(parents=True, exist_ok=True)
    repaired_path = cache_dir / f"repaired-{stat.st_mtime_ns}-{stat.st_size}.epub"
    if repaired_path.exists() and repaired_path.stat().st_size > 0:
        return repaired_path if _epub_has_readable_navigation(repaired_path) else None

    temp_path = repaired_path.with_suffix(".tmp.epub")
    temp_path.unlink(missing_ok=True)
    try:
        subprocess.run(
            [zip_tool, "-FF", str(path), "--out", str(temp_path)],
            input="",
            check=True,
            capture_output=True,
            text=True,
            timeout=90,
        )
        if not temp_path.exists() or temp_path.stat().st_size <= 0:
            return None
        if not _epub_has_readable_navigation(temp_path):
            return None
        temp_path.replace(repaired_path)
        return repaired_path
    except Exception:
        temp_path.unlink(missing_ok=True)
        return None


def _get_epub_read_path(path: Path, book_id: str) -> Path:
    if _epub_has_readable_navigation(path):
        return path

    repaired_path = _repair_epub_zip(path, book_id)
    if repaired_path:
        return repaired_path

    raise LibraryError("EPUB 文件损坏，无法读取目录和正文。")


def _epub_href_fragment(href: str) -> str:
    return href.split("#", 1)[1].strip() if "#" in href else ""


def _epub_ncx_items(path: Path) -> list[dict[str, str]]:
    try:
        archive = zipfile.ZipFile(path)
    except Exception:
        return []

    try:
        ncx_path = _find_ncx_path(archive)
        if not ncx_path:
            return []
        root = ElementTree.fromstring(archive.read(ncx_path))

        def child_nodes(node: ElementTree.Element, suffix: str) -> list[ElementTree.Element]:
            return [child for child in list(node) if child.tag.endswith(suffix)]

        def first_deep_text(node: ElementTree.Element, suffix: str) -> str:
            for child in node.iter():
                if child.tag.endswith(suffix) and child.text:
                    return child.text.strip()
            return ""

        chapters: list[dict[str, str]] = []

        def walk(nodes: list[ElementTree.Element], level: int = 0) -> None:
            for nav_point in nodes:
                if not nav_point.tag.endswith("navPoint"):
                    continue
                title = first_deep_text(nav_point, "text") or f"章节 {len(chapters) + 1}"
                content = next((child for child in child_nodes(nav_point, "content")), None)
                href = content.attrib.get("src", "") if content is not None else ""
                if href:
                    try:
                        chapter_path = _zip_join(ncx_path, href)
                    except LibraryError:
                        chapter_path = ""
                    if chapter_path:
                        chapters.append({
                            "href": href,
                            "media_type": "application/xhtml+xml",
                            "path": chapter_path,
                            "fragment": _epub_href_fragment(href),
                            "title": title[:120],
                            "level": str(max(0, min(level, 6))),
                        })
                walk(child_nodes(nav_point, "navPoint"), level + 1)

        nav_map = next((node for node in root.iter() if node.tag.endswith("navMap")), None)
        walk(child_nodes(nav_map, "navPoint") if nav_map is not None else [])
        return chapters
    except Exception:
        return []
    finally:
        archive.close()


def _read_epub_package(path: Path) -> tuple[zipfile.ZipFile, str, ElementTree.Element]:
    archive = zipfile.ZipFile(path)
    opf_path = _find_opf_path(archive)
    if not opf_path:
        archive.close()
        raise LibraryError("EPUB 缺少 OPF 包文件。")
    try:
        root = ElementTree.fromstring(archive.read(opf_path))
    except Exception:
        archive.close()
        raise LibraryError("EPUB 包文件无法解析。")
    return archive, opf_path, root


def _epub_spine_items(path: Path) -> list[dict[str, str]]:
    try:
        archive, opf_path, root = _read_epub_package(path)
    except LibraryError:
        ncx_items = _epub_ncx_items(path)
        if ncx_items:
            return ncx_items
        raise

    try:
        ns = {"opf": "http://www.idpf.org/2007/opf"}
        manifest: dict[str, dict[str, str]] = {}
        for item in root.findall(".//opf:manifest/opf:item", ns) or root.findall(".//manifest/item"):
            item_id = item.attrib.get("id")
            href = item.attrib.get("href")
            if item_id and href:
                media_type = item.attrib.get("media-type", "")
                manifest[item_id] = {
                    "href": href,
                    "media_type": media_type,
                    "path": _zip_join(opf_path, href),
                    "fragment": _epub_href_fragment(href),
                }

        spine = root.findall(".//opf:spine/opf:itemref", ns) or root.findall(".//spine/itemref")
        chapters = []
        for itemref in spine:
            idref = itemref.attrib.get("idref")
            item = manifest.get(idref or "")
            if not item:
                continue
            if item.get("media_type") and item["media_type"] not in EPUB_HTML_MEDIA_TYPES:
                continue
            chapters.append(item)

        ncx_items = _epub_ncx_items(path)
        if len(ncx_items) > len(chapters):
            return ncx_items
        return chapters or ncx_items
    finally:
        archive.close()


def get_epub_manifest(book_id: str) -> dict[str, Any]:
    record = get_book_record(book_id)
    if record.get("format") != "epub":
        raise UnsupportedBookFormatError("这本书不是 EPUB。")

    path = _get_epub_read_path(_record_path(record), str(record["id"]))
    chapters = _epub_spine_items(path)
    items = []
    with zipfile.ZipFile(path) as archive:
        for index, chapter in enumerate(chapters):
            title = chapter.get("title") or f"章节 {index + 1}"
            try:
                html = archive.read(chapter["path"])
                soup = BeautifulSoup(html, "html.parser")
                heading = soup.find(["h1", "h2", "h3", "title"])
                if heading and heading.get_text(strip=True):
                    title = heading.get_text(" ", strip=True)[:80]
            except Exception:
                pass
            items.append({
                "index": index,
                "title": title,
                "level": int(chapter.get("level") or 0),
            })

    return {"book": _public_book(record), "chapters": items}


def _decode_html(raw: bytes) -> str:
    for encoding in ["utf-8", "utf-16", "gb18030", "latin-1"]:
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", errors="replace")


def _sanitize_epub_html(html: str, book_id: str, chapter_path: str) -> str:
    soup = BeautifulSoup(html, "html.parser")
    root = soup.body or soup

    for tag in root.find_all(["script", "iframe", "object", "embed", "form", "input", "button"]):
        tag.decompose()

    for tag in root.find_all(True):
        for attr in list(tag.attrs):
            if attr.lower().startswith("on"):
                del tag.attrs[attr]

        if tag.name in {"img", "image"}:
            src_attr = "src" if tag.has_attr("src") else "href" if tag.has_attr("href") else None
            if src_attr:
                try:
                    resource_path = _zip_join(chapter_path, str(tag.attrs[src_attr]))
                    tag.attrs[src_attr] = f"/api/library/books/{book_id}/epub/resource?path={quote(resource_path)}"
                except LibraryError:
                    del tag.attrs[src_attr]

        if tag.name == "a" and tag.has_attr("href"):
            href = str(tag.attrs.get("href") or "")
            if href.startswith("#"):
                continue
            tag.attrs["target"] = "_blank"
            tag.attrs["rel"] = "noopener noreferrer"

    return "".join(str(child) for child in root.children)


def get_epub_chapter(book_id: str, chapter_index: int) -> dict[str, Any]:
    record = get_book_record(book_id)
    if record.get("format") != "epub":
        raise UnsupportedBookFormatError("这本书不是 EPUB。")

    path = _get_epub_read_path(_record_path(record), str(record["id"]))
    chapters = _epub_spine_items(path)
    if chapter_index < 0 or chapter_index >= len(chapters):
        raise BookNotFoundError("没有找到这个 EPUB 章节。")

    chapter = chapters[chapter_index]
    with zipfile.ZipFile(path) as archive:
        html = _decode_html(archive.read(chapter["path"]))

    content = _sanitize_epub_html(html, str(record["id"]), chapter["path"])
    return {
        "book": _public_book(record),
        "chapter": {
            "index": chapter_index,
            "title": chapter.get("title") or f"章节 {chapter_index + 1}",
            "anchor": chapter.get("fragment") or "",
            "total": len(chapters),
            "html": content,
        },
    }


def get_epub_resource(book_id: str, resource_path: str) -> tuple[bytes, str]:
    record = get_book_record(book_id)
    if record.get("format") != "epub":
        raise UnsupportedBookFormatError("这本书不是 EPUB。")

    normalized = posixpath.normpath(resource_path.strip())
    if normalized.startswith("../") or normalized == ".." or normalized.startswith("/"):
        raise BookNotFoundError("EPUB 内部资源路径无效。")

    original_path = _record_path(record)
    try:
        with zipfile.ZipFile(original_path) as archive:
            data = archive.read(normalized)
            content_type = mimetypes.guess_type(normalized)[0] or "application/octet-stream"
            return data, content_type
    except Exception:
        pass

    repaired_path = _repair_epub_zip(original_path, str(record["id"]))
    if repaired_path:
        try:
            with zipfile.ZipFile(repaired_path) as archive:
                data = archive.read(normalized)
                content_type = mimetypes.guess_type(normalized)[0] or "application/octet-stream"
                return data, content_type
        except Exception:
            pass

    raise BookNotFoundError("没有找到 EPUB 内部资源。")


def copy_book_to_library(source_path: Path) -> dict[str, Any]:
    source_path = source_path.expanduser().resolve()
    if not source_path.exists() or not source_path.is_file():
        raise BookNotFoundError("源电子书文件不存在。")
    suffix = source_path.suffix.lower()
    if suffix not in ALLOWED_BOOK_EXTENSIONS:
        raise UnsupportedBookFormatError("仅支持 PDF 和 EPUB 电子书。")

    book_id = uuid.uuid4().hex
    target = LIBRARY_BOOKS_DIR / f"{book_id}-{_safe_stem(source_path.stem)}{suffix}"
    LIBRARY_BOOKS_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_path, target)

    record = _build_record_for_file(
        target,
        book_id=book_id,
        metadata={"original_filename": source_path.name, "created_at": _now_iso()},
    )
    data = _load_index()
    records = [item for item in data.get("books", []) if isinstance(item, dict)]
    records.insert(0, record)
    data["books"] = _dedupe_records(records)
    _save_index(data)
    return _public_book(record)
