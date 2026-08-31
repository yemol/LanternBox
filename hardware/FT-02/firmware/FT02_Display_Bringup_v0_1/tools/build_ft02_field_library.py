#!/usr/bin/env python3
"""Build an FT-02 Field Library runtime package from Chinese TXT/EPUB/PDF books.

Source layout (one directory per book):
  <source>/<book_id>/metadata.json
  <source>/<book_id>/book.epub   # preferred
  OR
  <source>/<book_id>/book.pdf    # PC-side import, converted to text chapters
  OR
  <source>/<book_id>/book.txt    # legacy/compatibility

metadata.json fields:
  title (required), author, category, priority (default P0)

The builder never downloads copyrighted books. It converts EPUB/PDF into a compact
runtime cache (plain UTF-8 chapter files + TOC) for the ESP32-S3 while keeping
source.epub beside the cache. This avoids doing ZIP/XML/HTML/CSS layout work on
the e-paper terminal and gives deterministic TOC/search behavior offline.
"""
from __future__ import annotations

import argparse
import html
import io
import json
import posixpath
import re
import shutil
import zipfile
from html.parser import HTMLParser
from pathlib import Path, PurePosixPath
import xml.etree.ElementTree as ET

BLOCK_TAGS = {
    "address", "article", "aside", "blockquote", "br", "div", "dl", "dt", "dd",
    "figcaption", "figure", "footer", "h1", "h2", "h3", "h4", "h5", "h6", "header",
    "hr", "li", "main", "nav", "ol", "p", "pre", "section", "table", "tr", "ul"
}
SKIP_TAGS = {"script", "style", "svg", "math"}
MEDIA_MAX_WIDTH = 784


def write_utf8_text(path: Path, text: str) -> None:
    """Write UTF-8 with LF newlines on Python 3.9+.

    pathlib.Path.write_text() did not gain a newline= argument until newer
    Python versions, so FT-02 tooling must use Path.open() for macOS system
    Python 3.9 compatibility.
    """
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write(text)


class TextExtractor(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []
        self.skip = 0

    def handle_starttag(self, tag: str, attrs) -> None:
        tag = tag.lower()
        if tag in SKIP_TAGS:
            self.skip += 1
            return
        if self.skip:
            return
        if tag in BLOCK_TAGS:
            self.parts.append("\n")
        if tag == "li":
            self.parts.append("• ")

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if tag in SKIP_TAGS:
            self.skip = max(0, self.skip - 1)
            return
        if self.skip:
            return
        if tag in BLOCK_TAGS:
            self.parts.append("\n")

    def handle_data(self, data: str) -> None:
        if not self.skip:
            self.parts.append(data)

    def text(self) -> str:
        raw = html.unescape("".join(self.parts)).replace("\xa0", " ")
        raw = raw.replace("\r\n", "\n").replace("\r", "\n")
        raw = re.sub(r"[ \t\f\v]+", " ", raw)
        raw = re.sub(r" *\n *", "\n", raw)
        raw = re.sub(r"\n{2,}", "\n", raw)
        return raw.strip() + "\n"


class ImageRefExtractor(HTMLParser):
    """Collect raster image references in reading order from one XHTML file."""
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.refs: list[tuple[str, str]] = []

    def handle_starttag(self, tag: str, attrs) -> None:
        if tag.lower() != "img":
            return
        values = {str(k).lower(): str(v) for k, v in attrs if k and v is not None}
        src = values.get("src", "").strip()
        if not src:
            return
        caption = values.get("alt", "").strip() or values.get("title", "").strip()
        self.refs.append((src, caption))




class FlowExtractor(HTMLParser):
    """Extract readable text while preserving EPUB image positions.

    Images are emitted as stable temporary markers and converted to runtime
    media indices by build_epub(). This lets the FT-02 reader render figures
    inline in the same reading flow instead of hiding them behind a separate
    image browser.
    """
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []
        self.refs: list[tuple[str, str]] = []
        self.skip = 0

    def handle_starttag(self, tag: str, attrs) -> None:
        tag = tag.lower()
        if tag in SKIP_TAGS:
            self.skip += 1
            return
        if self.skip:
            return
        if tag in BLOCK_TAGS:
            self.parts.append("\n")
        if tag == "li":
            self.parts.append("• ")
        if tag == "img":
            values = {str(k).lower(): str(v) for k, v in attrs if k and v is not None}
            src = values.get("src", "").strip()
            if src:
                caption = values.get("alt", "").strip() or values.get("title", "").strip()
                ordinal = len(self.refs)
                self.refs.append((src, caption))
                self.parts.append(f"\n[[FT02IMG:{ordinal}]]\n")

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if tag in SKIP_TAGS:
            self.skip = max(0, self.skip - 1)
            return
        if self.skip:
            return
        if tag in BLOCK_TAGS:
            self.parts.append("\n")

    def handle_data(self, data: str) -> None:
        if not self.skip:
            self.parts.append(data)

    def flow(self) -> tuple[str, list[tuple[str, str]]]:
        raw = html.unescape("".join(self.parts)).replace("\xa0", " ")
        raw = raw.replace("\r\n", "\n").replace("\r", "\n")
        raw = re.sub(r"[ \t\f\v]+", " ", raw)
        raw = re.sub(r" *\n *", "\n", raw)
        raw = re.sub(r"\n{2,}", "\n", raw)
        return raw.strip() + "\n", self.refs


def html_to_flow(data: bytes) -> tuple[str, list[tuple[str, str]]]:
    for enc in ("utf-8", "utf-8-sig", "gb18030"):
        try:
            source = data.decode(enc)
            break
        except UnicodeDecodeError:
            continue
    else:
        source = data.decode("utf-8", errors="replace")
    parser = FlowExtractor()
    parser.feed(source)
    return parser.flow()

def html_image_refs(data: bytes) -> list[tuple[str, str]]:
    for enc in ("utf-8", "utf-8-sig", "gb18030"):
        try:
            source = data.decode(enc)
            break
        except UnicodeDecodeError:
            continue
    else:
        source = data.decode("utf-8", errors="replace")
    parser = ImageRefExtractor()
    parser.feed(source)
    return parser.refs


def _norm_flow_compare(text: str) -> str:
    return re.sub(r"\s+", "", text or "").strip().lower()


def normalize_epub_flow(flow_text: str, media_caption_by_index: dict[int, str]) -> str:
    """Remove extraction duplicates without changing real paragraph content.

    Some EPUBs repeat a heading twice in adjacent block elements, and many
    figure layouts contain both an <img alt="图..."> caption and the same
    visible caption text immediately after the image. Keeping both made the
    first line after a figure look like a page-overlap bug on FT-02.
    """
    lines = [line.strip() for line in flow_text.replace("\r", "").split("\n")]
    out: list[str] = []
    i = 0
    marker_re = re.compile(r"^\[\[IMG:(\d+)\]\]$")
    while i < len(lines):
        line = lines[i]
        if not line:
            i += 1
            continue
        m = marker_re.match(line)
        if m:
            idx = int(m.group(1))
            out.append(line)
            caption = _norm_flow_compare(media_caption_by_index.get(idx, ""))
            if caption and i + 1 < len(lines) and _norm_flow_compare(lines[i + 1]) == caption:
                i += 2
                continue
            i += 1
            continue
        # Collapse only short adjacent exact duplicates (typically chapter
        # headings), not arbitrary repeated prose.
        if out and not marker_re.match(out[-1]) and len(line) <= 96 and _norm_flow_compare(out[-1]) == _norm_flow_compare(line):
            i += 1
            continue
        out.append(line)
        i += 1
    return "\n".join(out).strip() + "\n"


def _otsu_threshold(gray) -> int:
    hist = gray.histogram()
    total = sum(hist)
    weighted = sum(i * count for i, count in enumerate(hist))
    sum_b = 0
    weight_b = 0
    best = -1.0
    threshold = 170
    for i, count in enumerate(hist):
        weight_b += count
        if weight_b == 0:
            continue
        weight_f = total - weight_b
        if weight_f <= 0:
            break
        sum_b += i * count
        mean_b = sum_b / weight_b
        mean_f = (weighted - sum_b) / weight_f
        between = weight_b * weight_f * (mean_b - mean_f) ** 2
        if between > best:
            best = between
            threshold = i
    return threshold


def image_bytes_to_pbm(
    data: bytes,
    output: Path,
    *,
    dither: bool | None,
    technical: bool = False,
    crop_pct: tuple[float, float, float, float] | None = None,
) -> tuple[int, int] | None:
    """Normalize raster media to a compact 1-bit PBM for FT-02.

    technical=True is tuned for scanned text/line-art pages: optional reviewed
    margin crop, high-quality resize, mild unsharp masking and an Otsu-derived
    threshold. EPUB artwork keeps Floyd-Steinberg dithering instead.
    """
    try:
        from PIL import Image, ImageOps, ImageFilter
    except ImportError as exc:
        raise RuntimeError("Pillow is required for image/PDF media import: python3 -m pip install Pillow") from exc
    try:
        with Image.open(io.BytesIO(data)) as im:
            im.load()
            if im.mode in {"RGBA", "LA"} or (im.mode == "P" and "transparency" in im.info):
                rgba = im.convert("RGBA")
                bg = Image.new("RGBA", rgba.size, "white")
                bg.alpha_composite(rgba)
                im = bg.convert("RGB")
            gray = ImageOps.grayscale(im)

            # EPUB diagrams often carry generous white canvas margins. Keeping
            # those margins makes "整图" mode shrink the useful drawing too
            # aggressively on the 800x480 panel. Trim only near-white outer
            # margins and keep a small safety pad so labels/arrows are not cut.
            if not technical and crop_pct is None:
                try:
                    ink = gray.point(lambda px: 255 if px < 245 else 0, mode="1")
                    bbox = ink.getbbox()
                    if bbox is not None:
                        x0, y0, x1, y1 = bbox
                        w0, h0 = gray.size
                        pad_x = max(4, round(w0 * 0.025))
                        pad_y = max(4, round(h0 * 0.025))
                        x0 = max(0, x0 - pad_x)
                        y0 = max(0, y0 - pad_y)
                        x1 = min(w0, x1 + pad_x)
                        y1 = min(h0, y1 + pad_y)
                        # Avoid surprising crops for photographs/full-bleed art.
                        if (x1 - x0) < w0 * 0.96 or (y1 - y0) < h0 * 0.96:
                            gray = gray.crop((x0, y0, x1, y1))
                except Exception:
                    pass

            if crop_pct is not None:
                left, top, right, bottom = crop_pct
                w, h = gray.size
                x0 = max(0, min(w - 1, round(w * left / 100.0)))
                y0 = max(0, min(h - 1, round(h * top / 100.0)))
                x1 = max(x0 + 1, min(w, round(w * (1.0 - right / 100.0))))
                y1 = max(y0 + 1, min(h, round(h * (1.0 - bottom / 100.0))))
                gray = gray.crop((x0, y0, x1, y1))

            gray = ImageOps.autocontrast(gray, cutoff=0.2 if technical else 0)

            # EPUB figures are often only 250-500 px wide. Scaling a 1-bit
            # bitmap on the ESP32 makes labels and thin lines jagged/faint.
            # Upscale the grayscale source first, then binarize at near-panel
            # width so the terminal mostly displays it 1:1.
            if not technical and gray.width < 720:
                target_w = min(MEDIA_MAX_WIDTH, 720)
                target_h = max(1, round(gray.height * target_w / gray.width))
                gray = gray.resize((target_w, target_h), Image.Resampling.LANCZOS)
            elif gray.width > MEDIA_MAX_WIDTH:
                h = max(1, round(gray.height * MEDIA_MAX_WIDTH / gray.width))
                gray = gray.resize((MEDIA_MAX_WIDTH, h), Image.Resampling.LANCZOS)
            if gray.width < 32 or gray.height < 32:
                return None

            if technical:
                gray = gray.filter(ImageFilter.UnsharpMask(radius=1.0, percent=170, threshold=2))

            auto_lineart = False
            if not technical and dither is None:
                # White-background diagrams should be thresholded, not dithered.
                # That keeps Chinese labels and botanical line art solid.
                probe = gray.copy()
                if probe.width > 256:
                    ph = max(1, round(probe.height * 256 / probe.width))
                    probe = probe.resize((256, ph), Image.Resampling.BILINEAR)
                hist = probe.histogram()
                total = max(1, sum(hist))
                white_ratio = sum(hist[232:]) / total
                dark_ratio = sum(hist[:150]) / total
                auto_lineart = white_ratio >= 0.55 and dark_ratio <= 0.30

            if technical or auto_lineart:
                if auto_lineart:
                    gray = ImageOps.autocontrast(gray, cutoff=0.1)
                    gray = gray.filter(ImageFilter.UnsharpMask(radius=0.8, percent=220, threshold=1))
                    threshold = max(170, min(215, _otsu_threshold(gray) + 24))
                else:
                    threshold = max(145, min(180, _otsu_threshold(gray) + 12))
                bw = gray.point(lambda px: 255 if px >= threshold else 0, mode="1")
            elif dither is False:
                bw = gray.point(lambda px: 255 if px >= 190 else 0, mode="1")
            else:
                bw = gray.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
            output.parent.mkdir(parents=True, exist_ok=True)
            bw.save(output, format="PPM")  # mode=1 => raw PBM (P4)
            return bw.width, bw.height
    except Exception:
        return None


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def safe_xml(data: bytes) -> ET.Element:
    return ET.fromstring(data)


def norm_epub_path(base_file: str, href: str) -> str:
    href = href.split("#", 1)[0]
    href = href.split("?", 1)[0]
    base_dir = posixpath.dirname(base_file)
    return posixpath.normpath(posixpath.join(base_dir, href))


def element_text(el: ET.Element) -> str:
    return "".join(el.itertext()).strip()


def html_to_text(data: bytes) -> str:
    for enc in ("utf-8", "utf-8-sig", "gb18030"):
        try:
            source = data.decode(enc)
            break
        except UnicodeDecodeError:
            continue
    else:
        source = data.decode("utf-8", errors="replace")
    parser = TextExtractor()
    parser.feed(source)
    return parser.text()


def first_heading_or_title(data: bytes, fallback: str) -> str:
    for enc in ("utf-8", "utf-8-sig", "gb18030"):
        try:
            source = data.decode(enc)
            break
        except UnicodeDecodeError:
            continue
    else:
        source = data.decode("utf-8", errors="replace")
    for pattern in (r"<h1\b[^>]*>(.*?)</h1>", r"<h2\b[^>]*>(.*?)</h2>", r"<title\b[^>]*>(.*?)</title>"):
        m = re.search(pattern, source, flags=re.I | re.S)
        if m:
            text = re.sub(r"<[^>]+>", "", m.group(1))
            text = html.unescape(text).strip()
            if text:
                return text[:96]
    return fallback


def parse_nav_toc(zf: zipfile.ZipFile, nav_path: str, opf_path: str) -> list[tuple[str, str]]:
    try:
        root = safe_xml(zf.read(nav_path))
    except Exception:
        return []
    result: list[tuple[str, str]] = []
    navs = [el for el in root.iter() if local_name(el.tag) == "nav"]
    toc_nav = None
    for nav in navs:
        typ = " ".join(str(v) for k, v in nav.attrib.items() if local_name(k) == "type")
        if "toc" in typ.split():
            toc_nav = nav
            break
    if toc_nav is None and navs:
        toc_nav = navs[0]
    if toc_nav is None:
        return result
    for el in toc_nav.iter():
        if local_name(el.tag) != "a":
            continue
        href = el.attrib.get("href", "").strip()
        title = element_text(el)
        if not href or not title:
            continue
        result.append((norm_epub_path(nav_path, href), title[:96]))
    return result


def parse_ncx_toc(zf: zipfile.ZipFile, ncx_path: str) -> list[tuple[str, str]]:
    try:
        root = safe_xml(zf.read(ncx_path))
    except Exception:
        return []
    result: list[tuple[str, str]] = []
    for navpoint in root.iter():
        if local_name(navpoint.tag) != "navPoint":
            continue
        title = ""
        href = ""
        for el in navpoint.iter():
            name = local_name(el.tag)
            if name == "text" and not title:
                title = element_text(el)
            elif name == "content" and not href:
                href = el.attrib.get("src", "")
        if title and href:
            result.append((norm_epub_path(ncx_path, href), title[:96]))
    return result


def build_epub(epub_path: Path, dst_dir: Path) -> tuple[list[tuple[int, str, str]], str, list[tuple[int, int, str, str, int, int]]]:
    chapters_dir = dst_dir / "chapters"
    media_dir = dst_dir / "media"
    chapters_dir.mkdir(parents=True, exist_ok=True)
    media_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(epub_path, dst_dir / "source.epub")

    with zipfile.ZipFile(epub_path) as zf:
        container = safe_xml(zf.read("META-INF/container.xml"))
        opf_path = ""
        for el in container.iter():
            if local_name(el.tag) == "rootfile":
                opf_path = el.attrib.get("full-path", "")
                if opf_path:
                    break
        if not opf_path:
            raise ValueError("EPUB container.xml has no rootfile")
        opf_root = safe_xml(zf.read(opf_path))

        manifest: dict[str, dict[str, str]] = {}
        spine_ids: list[str] = []
        spine_toc_id = ""
        for el in opf_root.iter():
            name = local_name(el.tag)
            if name == "item":
                item_id = el.attrib.get("id", "")
                href = el.attrib.get("href", "")
                if item_id and href:
                    manifest[item_id] = {
                        "path": norm_epub_path(opf_path, href),
                        "media": el.attrib.get("media-type", ""),
                        "properties": el.attrib.get("properties", ""),
                    }
            elif name == "spine":
                spine_toc_id = el.attrib.get("toc", "")
            elif name == "itemref":
                ref = el.attrib.get("idref", "")
                if ref:
                    spine_ids.append(ref)

        nav_entries: list[tuple[str, str]] = []
        nav_item = next((v for v in manifest.values() if "nav" in v["properties"].split()), None)
        if nav_item:
            nav_entries = parse_nav_toc(zf, nav_item["path"], opf_path)
        if not nav_entries:
            ncx_item = manifest.get(spine_toc_id) if spine_toc_id else None
            if ncx_item is None:
                ncx_item = next((v for v in manifest.values() if v["media"] == "application/x-dtbncx+xml"), None)
            if ncx_item:
                nav_entries = parse_ncx_toc(zf, ncx_item["path"])

        title_by_path: dict[str, str] = {}
        for path, title in nav_entries:
            title_by_path.setdefault(path, title)

        toc_rows: list[tuple[int, str, str]] = []
        media_rows: list[tuple[int, int, str, str, int, int]] = []
        seen_paths: set[str] = set()
        for ref in spine_ids:
            item = manifest.get(ref)
            if not item:
                continue
            path = item["path"]
            if path in seen_paths:
                continue
            seen_paths.add(path)
            if item["media"] not in ("application/xhtml+xml", "text/html", "application/xml", "text/xml"):
                continue
            try:
                data = zf.read(path)
            except KeyError:
                continue
            flow_text, flow_refs = html_to_flow(data)
            if not flow_text.strip() and not flow_refs:
                continue
            index = len(toc_rows)
            chapter_name = f"{index:04d}.txt"
            chapter_path = chapters_dir / chapter_name
            title = title_by_path.get(path) or first_heading_or_title(data, f"第 {index + 1} 节")

            # Convert EPUB figures first, then replace temporary HTML-order
            # markers with stable runtime media indices embedded in the chapter.
            # The ESP32 reader can therefore place an image exactly where the
            # original EPUB placed it. Duplicate references in one chapter reuse
            # the same converted media item.
            media_by_path: dict[str, int] = {}
            ordinal_to_media: dict[int, int] = {}
            for ordinal, (raw_src, caption) in enumerate(flow_refs):
                img_path = norm_epub_path(path, raw_src)
                if not img_path:
                    continue
                if img_path in media_by_path:
                    ordinal_to_media[ordinal] = media_by_path[img_path]
                    continue
                try:
                    img_data = zf.read(img_path)
                except KeyError:
                    continue
                media_index = len(media_rows)
                media_name = f"{media_index:04d}.pbm"
                converted = image_bytes_to_pbm(img_data, media_dir / media_name, dither=None)
                if converted is None:
                    continue
                w, h = converted
                label = caption.strip()[:96] if caption.strip() else f"{title} · 图 {media_index + 1}"
                media_rows.append((media_index, index, label, media_name, w, h))
                media_by_path[img_path] = media_index
                ordinal_to_media[ordinal] = media_index

            for ordinal in range(len(flow_refs)):
                token = f"[[FT02IMG:{ordinal}]]"
                if ordinal in ordinal_to_media:
                    flow_text = flow_text.replace(token, f"[[IMG:{ordinal_to_media[ordinal]}]]")
                else:
                    flow_text = flow_text.replace(token, "")
            media_caption_by_index = {row[0]: row[2] for row in media_rows if row[1] == index}
            flow_text = normalize_epub_flow(flow_text, media_caption_by_index)
            write_utf8_text(chapter_path, flow_text)

            runtime_path = f"{dst_dir.as_posix()}/chapters/{chapter_name}"  # replaced below
            toc_rows.append((index, title, runtime_path))

        if not toc_rows:
            raise ValueError("EPUB has no readable spine chapters")

    # The caller rewrites filesystem paths to SD runtime paths.
    return toc_rows, opf_path, media_rows




def _pdf_group_starts(page_count: int, raw_toc: list[tuple[int, str, int]] | None = None) -> list[tuple[int, str]]:
    """Return stable chapter starts from a PDF TOC, or 10-page fallback groups."""
    raw_toc = raw_toc or []
    starts: list[tuple[int, str]] = []
    try:
        top = [
            (int(page) - 1, str(title).strip())
            for level, title, page in raw_toc
            if int(level) == 1 and int(page) > 0
        ]
        source_entries = top if len(top) >= 2 else [
            (int(page) - 1, str(title).strip())
            for level, title, page in raw_toc
            if int(page) > 0
        ]
    except Exception:
        source_entries = []

    seen_pages: set[int] = set()
    for page, title in source_entries:
        page = max(0, min(page_count - 1, page))
        if page in seen_pages:
            continue
        seen_pages.add(page)
        starts.append((page, title or f"第 {len(starts) + 1} 节"))
    starts.sort(key=lambda x: x[0])

    if not starts:
        return [(p, f"第 {p + 1}-{min(p + 10, page_count)} 页") for p in range(0, page_count, 10)]
    if starts[0][0] > 0:
        starts.insert(0, (0, "前言与前置内容"))
    return starts


def _extract_pdf_with_pymupdf(pdf_path: Path) -> tuple[list[str], list[tuple[int, str]], str]:
    """Extract PDF text with PyMuPDF while isolating recoverable xref noise."""
    import fitz  # PyMuPDF

    # Some institutional PDFs have technically broken xref / annotation objects
    # but perfectly readable page streams. MuPDF can repair these while opening,
    # yet prints one line per missing object. Keep that noise out of the pipeline;
    # real failures are still raised and handled by the caller.
    try:
        fitz.TOOLS.mupdf_display_errors(False)
        fitz.TOOLS.mupdf_display_warnings(False)
        fitz.TOOLS.reset_mupdf_warnings()
    except Exception:
        pass

    doc = fitz.open(pdf_path)
    try:
        page_count = doc.page_count
        if page_count <= 0:
            raise ValueError("PDF has no pages")
        try:
            raw_toc = doc.get_toc(simple=True) or []
        except Exception:
            raw_toc = []
        starts = _pdf_group_starts(page_count, raw_toc)

        pages: list[str] = []
        failed_pages = 0
        for page_no in range(page_count):
            try:
                text = doc.load_page(page_no).get_text("text") or ""
            except Exception:
                failed_pages += 1
                text = ""
            text = text.replace("\r\n", "\n").replace("\r", "\n").strip()
            pages.append(text)

        chars = sum(len(x) for x in pages)
        if chars < 200:
            raise ValueError(f"PyMuPDF extracted too little text ({chars} chars)")
        if failed_pages and failed_pages >= max(2, page_count // 5):
            raise ValueError(f"PyMuPDF could not read {failed_pages}/{page_count} pages")
        return pages, starts, "pymupdf-recovered" if failed_pages else "pymupdf"
    finally:
        doc.close()


def _flatten_pypdf_outline(reader) -> list[tuple[int, str]]:
    """Best-effort pypdf outline extraction; broken outlines are simply ignored."""
    result: list[tuple[int, str]] = []
    try:
        outline = reader.outline
    except Exception:
        return result

    def walk(items) -> None:
        if not isinstance(items, (list, tuple)):
            items = [items]
        for item in items:
            if isinstance(item, list):
                walk(item)
                continue
            title = str(getattr(item, "title", "") or "").strip()
            if not title:
                continue
            try:
                page = int(reader.get_destination_page_number(item))
            except Exception:
                continue
            if page >= 0:
                result.append((page, title[:96]))

    try:
        walk(outline)
    except Exception:
        return []
    # Keep first title for each page to avoid explosive nested outlines.
    dedup: dict[int, str] = {}
    for page, title in sorted(result, key=lambda x: x[0]):
        dedup.setdefault(page, title)
    return list(dedup.items())


def _extract_pdf_with_pypdf(pdf_path: Path) -> tuple[list[str], list[tuple[int, str]], str]:
    """Fallback parser for malformed xref tables / missing non-page objects."""
    try:
        from pypdf import PdfReader
    except ImportError as exc:
        raise RuntimeError(
            "pypdf fallback is unavailable. Install with: python3 -m pip install pypdf"
        ) from exc

    # pypdf reports repairable xref problems through logging. Keep the normal
    # pipeline concise; a true extraction failure is still returned below.
    import logging
    pypdf_logger = logging.getLogger("pypdf")
    old_level = pypdf_logger.level
    pypdf_logger.setLevel(logging.ERROR)
    try:
        reader = PdfReader(str(pdf_path), strict=False)
    finally:
        pypdf_logger.setLevel(old_level)
    page_count = len(reader.pages)
    if page_count <= 0:
        raise ValueError("PDF has no pages")

    outline_starts = _flatten_pypdf_outline(reader)
    pseudo_toc = [(1, title, page + 1) for page, title in outline_starts]
    starts = _pdf_group_starts(page_count, pseudo_toc)

    pages: list[str] = []
    failed_pages = 0
    for page in reader.pages:
        try:
            text = page.extract_text() or ""
        except Exception:
            failed_pages += 1
            text = ""
        text = text.replace("\r\n", "\n").replace("\r", "\n").strip()
        pages.append(text)

    chars = sum(len(x) for x in pages)
    if chars < 200:
        raise ValueError(f"pypdf extracted too little text ({chars} chars)")
    if failed_pages and failed_pages >= max(2, page_count // 5):
        raise ValueError(f"pypdf could not read {failed_pages}/{page_count} pages")
    return pages, starts, "pypdf-recovery"


def build_pdf(pdf_path: Path, dst_dir: Path) -> list[tuple[int, str, str]]:
    """Convert a text PDF into FT-02 UTF-8 chapter cache with resilient recovery.

    The source PDF is preserved unchanged. Parsing first uses PyMuPDF in quiet,
    recovery-friendly mode. If malformed xref/object references still prevent
    reliable extraction, pypdf strict=False becomes the automatic fallback.
    Image-only/scanned PDFs are deliberately rejected rather than silently OCR'd.
    """
    try:
        import fitz  # noqa: F401
    except ImportError as exc:
        raise SystemExit(
            "PDF import requires PyMuPDF. Install with: python3 -m pip install pymupdf"
        ) from exc

    chapters_dir = dst_dir / "chapters"
    chapters_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(pdf_path, dst_dir / "source.pdf")

    errors: list[str] = []
    try:
        pages, starts, parser_name = _extract_pdf_with_pymupdf(pdf_path)
    except Exception as exc:
        errors.append(f"PyMuPDF: {exc}")
        try:
            pages, starts, parser_name = _extract_pdf_with_pypdf(pdf_path)
        except Exception as fallback_exc:
            errors.append(f"pypdf: {fallback_exc}")
            raise ValueError(
                "PDF text extraction failed after automatic recovery. "
                + " | ".join(errors)
                + ". If the PDF is image-only, prepare a reviewed OCR/text PDF before import."
            ) from fallback_exc

    page_count = len(pages)
    toc_rows: list[tuple[int, str, str]] = []
    total_chars = 0
    for idx, (start_page, title) in enumerate(starts):
        end_page = starts[idx + 1][0] if idx + 1 < len(starts) else page_count
        if end_page <= start_page:
            continue
        chapter_text = "\n\n".join(x for x in pages[start_page:end_page] if x).strip()
        if not chapter_text:
            continue
        total_chars += len(chapter_text)
        chapter_name = f"{len(toc_rows):04d}.txt"
        write_utf8_text(chapters_dir / chapter_name, chapter_text + "\n")
        toc_rows.append((len(toc_rows), title[:96], ""))

    if not toc_rows or total_chars < 200:
        raise ValueError(
            "PDF contains too little extractable text; it may be scanned/image-only and needs reviewed OCR before import"
        )

    # Diagnostic file stays on the PC/SD package and makes parser provenance auditable.
    (dst_dir / "IMPORT.txt").write_text(
        f"PDF parser: {parser_name}\n"
        f"Pages: {page_count}\n"
        f"Chapters: {len(toc_rows)}\n"
        f"Extracted characters: {total_chars}\n",
        encoding="utf-8",
    )
    return toc_rows


def _scan_pdf_page_raster(doc, page) -> bytes:
    """Prefer the original dominant page raster over re-rasterizing PDF.

    Image-first technical PDFs commonly contain one full-page JPEG. Extracting
    it avoids a lossy PDF-render -> PNG -> resize chain and materially improves
    small Chinese text on the 800px e-paper panel.
    """
    import fitz
    try:
        images = page.get_images(full=True)
        candidates = []
        for info in images:
            if len(info) < 4:
                continue
            xref = int(info[0])
            width = int(info[2])
            height = int(info[3])
            if width >= 400 and height >= 400:
                candidates.append((width * height, xref))
        if candidates:
            _, xref = max(candidates)
            extracted = doc.extract_image(xref)
            raw = extracted.get("image")
            if raw:
                return raw
    except Exception:
        pass

    rect = page.rect
    target_width = max(1200.0, float(MEDIA_MAX_WIDTH) * 2.0)
    scale = max(1.0, target_width / max(1.0, float(rect.width)))
    pix = page.get_pixmap(matrix=fitz.Matrix(scale, scale), colorspace=fitz.csGRAY, alpha=False)
    return pix.tobytes("png")


def build_scan_pdf(pdf_path: Path, dst_dir: Path, meta: dict) -> tuple[list[tuple[int, str, str]], list[tuple[int, int, str, str, int, int]]]:
    """Render a scanned/image-first PDF into 1-bit PBM pages for FT-02.

    No OCR is used. The original page remains the authority, while metadata may
    provide a reviewed TOC mapping through scan_toc=[{title,pdf_page}, ...].
    """
    try:
        import fitz
    except ImportError as exc:
        raise SystemExit("Scanned PDF import requires PyMuPDF: python3 -m pip install pymupdf") from exc

    try:
        fitz.TOOLS.mupdf_display_errors(False)
        fitz.TOOLS.mupdf_display_warnings(False)
    except Exception:
        pass

    pages_dir = dst_dir / "media"
    pages_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(pdf_path, dst_dir / "source.pdf")
    doc = fitz.open(pdf_path)
    exclude_tail = max(0, int(meta.get("scan_exclude_tail_pages", 0) or 0))
    page_count = max(0, len(doc) - exclude_tail)
    if page_count <= 0:
        doc.close()
        raise ValueError("scanned PDF has no importable pages")

    media_rows: list[tuple[int, int, str, str, int, int]] = []
    for page_index in range(page_count):
        page = doc[page_index]
        raster = _scan_pdf_page_raster(doc, page)
        media_name = f"{page_index:04d}.pbm"
        raw_crop = meta.get("scan_crop_pct", [0, 0, 0, 0])
        crop_pct = None
        if isinstance(raw_crop, (list, tuple)) and len(raw_crop) == 4:
            try:
                crop_pct = tuple(float(v) for v in raw_crop)
            except Exception:
                crop_pct = None
        converted = image_bytes_to_pbm(
            raster, pages_dir / media_name, dither=False, technical=True, crop_pct=crop_pct
        )
        if converted is None:
            doc.close()
            raise ValueError(f"failed to render scanned PDF page {page_index + 1}")
        w, h = converted
        media_rows.append((page_index, -1, f"第 {page_index + 1} 页", media_name, w, h))
        if (page_index + 1) % 20 == 0 or page_index + 1 == page_count:
            print(f"  scanpdf: rendered {page_index + 1}/{page_count} pages")
    doc.close()

    toc_rows: list[tuple[int, str, str]] = []
    raw_toc = meta.get("scan_toc", [])
    if isinstance(raw_toc, list):
        for entry in raw_toc:
            if not isinstance(entry, dict):
                continue
            title = str(entry.get("title", "")).strip()
            try:
                pdf_page = int(entry.get("pdf_page", 0))
            except Exception:
                continue
            target = pdf_page - 1
            if not title or target < 0 or target >= page_count:
                continue
            toc_rows.append((len(toc_rows), title[:96], f"@media:{target}"))
    if not toc_rows:
        # Safe generic fallback: navigation anchors every 10 pages.
        for target in range(0, page_count, 10):
            toc_rows.append((len(toc_rows), f"第 {target + 1} 页", f"@media:{target}"))

    write_utf8_text(
        dst_dir / "IMPORT.txt",
        f"PDF parser: scan-image-mode\nPages: {page_count}\nMedia pages: {len(media_rows)}\nOCR: disabled\n",
    )
    return toc_rows, media_rows

def validate_field(value: str, folder: Path) -> None:
    if any(c in value for c in "\t\r\n"):
        raise SystemExit(f"tabs/newlines are not allowed in metadata: {folder}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("output", type=Path, help="Output /library directory")
    args = ap.parse_args()

    source = args.source
    output = args.output
    if not source.exists():
        print(f"A6: source staging does not exist; creating empty staging: {source}")
        source.mkdir(parents=True, exist_ok=True)
    if not source.is_dir():
        raise SystemExit(f"source is not a directory: {source}")
    books_out = output / "books"
    user_out = output / "user"
    if output.exists():
        # Only clear generated book/catalog data. Keep user progress when rebuilding in place.
        if books_out.exists():
            shutil.rmtree(books_out)
    books_out.mkdir(parents=True, exist_ok=True)
    user_out.mkdir(parents=True, exist_ok=True)

    rows: list[tuple[str, ...]] = []
    for folder in sorted(p for p in source.iterdir() if p.is_dir()):
        meta_path = folder / "metadata.json"
        if not meta_path.is_file():
            continue
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        book_id = folder.name.strip()
        title = str(meta.get("title", "")).strip()
        author = str(meta.get("author", "")).strip()
        category = str(meta.get("category", "未分类")).strip() or "未分类"
        priority = str(meta.get("priority", "")).strip().upper()
        language = str(meta.get("language", "")).strip().lower()
        source_priority = str(meta.get("source_priority", "")).strip().upper()
        library_role = str(meta.get("library_role", "")).strip().upper()
        installable = meta.get("installable", False)
        source_status = str(meta.get("source_status", "")).strip().upper()
        content_audit_status = str(meta.get("content_audit_status", "")).strip().upper()
        practical_value_status = str(meta.get("practical_value_status", "")).strip().upper()
        admission_status = str(meta.get("admission_status", "")).strip().upper()

        # A6 P0 Admission Gate: zero-default-deny, no legacy metadata compatibility.
        # A book enters the runtime only after every required P0 gate is explicitly APPROVED.
        admitted_core = (
            priority == "P0"
            and language in {"zh", "zh-cn", "zh-hans"}
            and source_priority == "P0"
            and library_role == "CORE_CONTENT"
            and installable is True
            and source_status == "APPROVED"
            and content_audit_status == "APPROVED"
            and practical_value_status == "APPROVED"
            and admission_status == "APPROVED"
        )
        if not admitted_core:
            reason = (
                f"priority={priority or 'MISSING'}, language={language or 'MISSING'}, "
                f"source_priority={source_priority or 'MISSING'}, role={library_role or 'MISSING'}, "
                f"installable={installable}, source={source_status or 'MISSING'}, "
                f"content={content_audit_status or 'MISSING'}, practical={practical_value_status or 'MISSING'}, "
                f"admission={admission_status or 'MISSING'}"
            )
            print(f"Skip {book_id}: A6 P0 Admission Gate denied ({reason})")
            continue
        if not book_id or not title:
            raise SystemExit(f"invalid metadata: {folder}")
        for value in (book_id, title, author, category, priority):
            validate_field(value, folder)

        epub_path = folder / "book.epub"
        pdf_path = folder / "book.pdf"
        text_path = folder / "book.txt"
        dst_dir = books_out / book_id
        dst_dir.mkdir(parents=True, exist_ok=True)
        runtime_root = f"/library/books/{book_id}"
        toc_rows: list[tuple[int, str, str]] = []
        media_rows: list[tuple[int, int, str, str, int, int]] = []

        if epub_path.is_file():
            temp_rows, _, temp_media = build_epub(epub_path, dst_dir)
            for idx, chapter_title, _ in temp_rows:
                toc_rows.append((idx, chapter_title, f"{runtime_root}/chapters/{idx:04d}.txt"))
            for media_idx, chapter_idx, caption, media_name, w, h in temp_media:
                media_rows.append((media_idx, chapter_idx, caption, f"{runtime_root}/media/{media_name}", w, h))
            fmt = "epub"
        elif pdf_path.is_file():
            pdf_mode = str(meta.get("pdf_mode", "auto")).strip().lower()
            if pdf_mode == "scan":
                temp_rows, temp_media = build_scan_pdf(pdf_path, dst_dir, meta)
                toc_rows.extend(temp_rows)
                for media_idx, chapter_idx, caption, media_name, w, h in temp_media:
                    media_rows.append((media_idx, chapter_idx, caption, f"{runtime_root}/media/{media_name}", w, h))
                fmt = "scanpdf"
            else:
                temp_rows = build_pdf(pdf_path, dst_dir)
                for idx, chapter_title, _ in temp_rows:
                    toc_rows.append((idx, chapter_title, f"{runtime_root}/chapters/{idx:04d}.txt"))
                fmt = "pdf"
        elif text_path.is_file():
            text = text_path.read_text(encoding="utf-8")
            chapters = dst_dir / "chapters"
            chapters.mkdir(parents=True, exist_ok=True)
            write_utf8_text(chapters / "0000.txt", text)
            shutil.copy2(text_path, dst_dir / "source.txt")
            toc_rows.append((0, "正文", f"{runtime_root}/chapters/0000.txt"))
            fmt = "txt"
        else:
            raise SystemExit(f"{folder}: expected book.epub, book.pdf or book.txt")

        toc_path = dst_dir / "toc.tsv"
        with toc_path.open("w", encoding="utf-8", newline="\n") as f:
            f.write("# index<TAB>title<TAB>path\n")
            for idx, chapter_title, path in toc_rows:
                clean_title = re.sub(r"[\t\r\n]+", " ", chapter_title).strip() or f"第 {idx + 1} 节"
                f.write(f"{idx}\t{clean_title}\t{path}\n")

        if media_rows:
            media_path = dst_dir / "media.tsv"
            with media_path.open("w", encoding="utf-8", newline="\n") as f:
                f.write("# index<TAB>chapter<TAB>caption<TAB>path<TAB>width<TAB>height\n")
                for media_idx, chapter_idx, caption, path, w, h in media_rows:
                    clean_caption = re.sub(r"[\t\r\n]+", " ", caption).strip() or "图片"
                    f.write(f"{media_idx}\t{chapter_idx}\t{clean_caption}\t{path}\t{w}\t{h}\n")

        runtime_meta = {
            "id": book_id,
            "title": title,
            "author": author,
            "category": category,
            "priority": priority,
            "format": fmt,
            "chapter_count": len(toc_rows),
            "media_count": len(media_rows),
            "slot": str(meta.get("slot", "")),
            "source_priority": str(meta.get("source_priority", "")),
            "library_role": str(meta.get("library_role", "")),
            "installable": meta.get("installable", None),
            "source_url": str(meta.get("source_url", "")),
            "source_record": str(meta.get("source_record", "")),
            "license": str(meta.get("license", "")),
            "reuse_status": str(meta.get("reuse_status", "")),
            "content_status": str(meta.get("content_status", "")),
            "language": language,
            "source_status": source_status,
            "content_audit_status": content_audit_status,
            "practical_value_status": practical_value_status,
            "admission_status": admission_status,
        }
        (dst_dir / "metadata.json").write_text(
            json.dumps(runtime_meta, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )
        provenance = [
            f"Title: {title}",
            f"Author: {author}",
            f"Source: {runtime_meta['source_url']}",
            f"Record: {runtime_meta['source_record']}",
            f"License: {runtime_meta['license']}",
            f"Reuse status: {runtime_meta['reuse_status']}",
            f"Content status: {runtime_meta['content_status']}",
            f"Source status: {runtime_meta['source_status']}",
            f"Content audit: {runtime_meta['content_audit_status']}",
            f"Practical value: {runtime_meta['practical_value_status']}",
            f"Admission: {runtime_meta['admission_status']}",
            f"Library role: {runtime_meta['library_role']}",
            f"Installable: {runtime_meta['installable']}",
        ]
        (dst_dir / "SOURCE.txt").write_text("\n".join(provenance) + "\n", encoding="utf-8")
        rows.append((book_id, title, author, category, priority, fmt, runtime_root, f"{runtime_root}/toc.tsv"))

    catalog = output / "catalog.tsv"
    with catalog.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# FT-02 Field Library catalog v0.2\n")
        f.write("# id<TAB>title<TAB>author<TAB>category<TAB>priority<TAB>format<TAB>root<TAB>toc\n")
        for row in rows:
            f.write("\t".join(row) + "\n")

    print(f"Built {len(rows)} P0 books -> {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
