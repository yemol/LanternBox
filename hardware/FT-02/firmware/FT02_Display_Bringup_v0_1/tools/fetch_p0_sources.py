#!/usr/bin/env python3
"""Fetch FT-02 P0 source documents from official/open URLs.

Default behavior only fetches entries whose reuse status is APPROVED (currently
CC BY 4.0 sources). No DRM bypassing, paywall bypassing, or pirate mirrors are supported.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path
from urllib.parse import urljoin, urlparse

try:
    import requests
    from bs4 import BeautifulSoup
except ImportError:
    raise SystemExit(
        "Missing dependencies. Run: python3 -m pip install -r requirements_p0.txt"
    )

UA = "FT02-FieldLibrary-P0Fetcher/1.1 (+personal offline library; official sources only)"


def is_pdf_response(resp: requests.Response) -> bool:
    ctype = (resp.headers.get("content-type") or "").lower()
    return "application/pdf" in ctype or resp.content[:5] == b"%PDF-"


def safe_filename(book_id: str) -> str:
    return re.sub(r"[^A-Za-z0-9._-]+", "_", book_id) + ".pdf"


def fetch_direct(session: requests.Session, url: str) -> tuple[bytes, str]:
    r = session.get(url, timeout=60, allow_redirects=True)
    r.raise_for_status()
    if not is_pdf_response(r):
        raise RuntimeError(f"Expected PDF but got {r.headers.get('content-type')} from {r.url}")
    return r.content, r.url


def candidate_pdf_links(html: str, base_url: str) -> list[str]:
    soup = BeautifulSoup(html, "html.parser")
    candidates: list[str] = []
    for a in soup.find_all("a", href=True):
        href = urljoin(base_url, a["href"])
        low = href.lower()
        text = " ".join(a.stripped_strings).lower()
        if (
            ".pdf" in low
            or "/bitstream/" in low
            or "/bitstreams/" in low
            or "download" in low
            or "pdf" in text
        ):
            candidates.append(href)
    # DSpace/JS pages sometimes embed API content links in JSON/script.
    for m in re.finditer(r'https?://[^"\'<> ]+(?:\.pdf[^"\'<> ]*|/bitstreams/[0-9a-fA-F-]+/content)', html):
        candidates.append(m.group(0).replace("\\u0026", "&"))
    for m in re.finditer(r'/(?:server/api/core/bitstreams/[0-9a-fA-F-]+/content|bitstream/handle/[^"\'<> ]+)', html):
        candidates.append(urljoin(base_url, m.group(0).replace("\\u0026", "&")))
    out: list[str] = []
    seen = set()
    for u in candidates:
        if u not in seen:
            seen.add(u)
            out.append(u)
    return out


def fetch_handle_pdf(session: requests.Session, handle_url: str) -> tuple[bytes, str]:
    r = session.get(handle_url, timeout=60, allow_redirects=True)
    r.raise_for_status()
    if is_pdf_response(r):
        return r.content, r.url

    for url in candidate_pdf_links(r.text, r.url):
        try:
            rr = session.get(url, timeout=60, allow_redirects=True)
            rr.raise_for_status()
            if is_pdf_response(rr):
                return rr.content, rr.url
        except requests.RequestException:
            continue

    # Last fallback for migrated DSpace pages: ask the Handle API for the target,
    # then parse that target once more. This does not bypass any access control.
    parsed = urlparse(handle_url)
    handle_id = parsed.path.strip("/")
    if handle_id:
        api = f"https://hdl.handle.net/api/handles/{handle_id}"
        try:
            hr = session.get(api, timeout=30)
            hr.raise_for_status()
            data = hr.json()
            targets = []
            for value in data.get("values", []):
                d = value.get("data", {}).get("value")
                if isinstance(d, str) and d.startswith("http"):
                    targets.append(d)
            for target in targets:
                tr = session.get(target, timeout=60, allow_redirects=True)
                tr.raise_for_status()
                if is_pdf_response(tr):
                    return tr.content, tr.url
                for url in candidate_pdf_links(tr.text, tr.url):
                    try:
                        rr = session.get(url, timeout=60, allow_redirects=True)
                        rr.raise_for_status()
                        if is_pdf_response(rr):
                            return rr.content, rr.url
                    except requests.RequestException:
                        pass
        except Exception:
            pass

    raise RuntimeError(
        "Could not resolve a PDF from this handle. Open the source_record URL, "
        "download the Chinese PDF manually, and save it as book.pdf in the staging folder."
    )


def write_book_stage(root: Path, entry: dict, pdf_data: bytes, resolved_url: str) -> Path:
    folder = root / entry["id"]
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "book.pdf").write_bytes(pdf_data)
    meta = {
        "title": entry["title"],
        "author": entry.get("author", ""),
        "category": entry.get("category", "未分类"),
        "priority": entry.get("priority", "P0"),
        "slot": entry.get("slot", ""),
        "source_priority": entry.get("priority", "P0"),
        "library_role": entry.get("library_role", "AUDIT_REFERENCE"),
        "installable": bool(entry.get("installable", False)),
        "source_url": entry.get("source_url", ""),
        "source_record": entry.get("source_record", ""),
        "resolved_download_url": resolved_url,
        "license": entry.get("license", ""),
        "reuse_status": entry.get("reuse_status", ""),
        "content_status": entry.get("content_status", ""),
        "language": entry.get("language", ""),
        "source_status": entry.get("source_status", ""),
        "content_audit_status": entry.get("content_audit_status", ""),
        "practical_value_status": entry.get("practical_value_status", ""),
        "admission_status": entry.get("admission_status", ""),
    }
    (folder / "metadata.json").write_text(
        json.dumps(meta, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    return folder


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", type=Path, default=Path(__file__).with_name("p0_sources_A6.json"))
    ap.add_argument("--output", type=Path, default=Path("P0_SOURCE_STAGING"))
    ap.add_argument("--id", action="append", dest="ids", help="Fetch only this source ID; may be repeated")
    args = ap.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    selected = []
    for e in manifest["sources"]:
        if args.ids and e["id"] not in args.ids:
            continue
        if e.get("fetch_mode") == "manual":
            continue
        approved = (
            str(e.get("priority", "")).upper() == "P0"
            and str(e.get("language", "")).lower() in {"zh", "zh-cn", "zh-hans"}
            and str(e.get("library_role", "")).upper() == "CORE_CONTENT"
            and e.get("installable") is True
            and str(e.get("source_status", "")).upper() == "APPROVED"
            and str(e.get("content_audit_status", "")).upper() == "APPROVED"
            and str(e.get("practical_value_status", "")).upper() == "APPROVED"
            and str(e.get("admission_status", "")).upper() == "APPROVED"
        )
        if approved and e.get("auto_fetch") and e.get("reuse_status") == "APPROVED":
            selected.append(e)
    if not selected:
        print("No fetchable entries selected.")
        return 0

    args.output.mkdir(parents=True, exist_ok=True)
    session = requests.Session()
    session.headers.update({"User-Agent": UA, "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.5"})

    failures = 0
    for e in selected:
        print(f"[{e['id']}] {e['title']}")
        try:
            mode = e.get("fetch_mode")
            if mode == "direct_pdf":
                data, resolved = fetch_direct(session, e["source_url"])
            elif mode == "handle_pdf":
                data, resolved = fetch_handle_pdf(session, e["source_url"])
            else:
                raise RuntimeError(f"Unsupported fetch_mode={mode}")
            if len(data) < 1024:
                raise RuntimeError("Downloaded file is unexpectedly small")
            folder = write_book_stage(args.output, e, data, resolved)
            print(f"  OK {len(data)/1024/1024:.2f} MiB -> {folder/'book.pdf'}")
        except Exception as exc:
            failures += 1
            print(f"  FAILED: {exc}", file=sys.stderr)
            hint = args.output / e["id"]
            hint.mkdir(parents=True, exist_ok=True)
            (hint / "DOWNLOAD_MANUALLY.txt").write_text(
                f"Title: {e['title']}\nSource: {e.get('source_url','')}\nRecord: {e.get('source_record','')}\n"
                "Save the official Chinese PDF in this folder as book.pdf, then rerun the builder.\n",
                encoding="utf-8",
            )

    print(f"Done: {len(selected)-failures} succeeded, {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
