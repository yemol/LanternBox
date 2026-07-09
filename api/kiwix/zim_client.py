"""Local ZIM reader with manifest-based role and channel filtering."""

import json
import mimetypes
import posixpath
import re
from html import escape, unescape
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional
from urllib.parse import quote, unquote, urlsplit

try:
    from bs4 import BeautifulSoup
    from bs4 import NavigableString
except Exception:  # BeautifulSoup is optional at import time; article cleaning falls back to plain text.
    BeautifulSoup = None  # type: ignore
    NavigableString = None  # type: ignore

try:
    from opencc import OpenCC
except Exception:
    OpenCC = None  # type: ignore


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ZIM_DIR = ROOT / "data" / "kiwix" / "zim"
DEFAULT_MANIFEST_PATH = ROOT / "data" / "kiwix" / "zim_manifest.json"

AI_RETRIEVAL_POLICY = "ai_retrieval_allowed"
LOOKUP_POLICIES = {"lookup_only", "background_support_only", "language_support_only", AI_RETRIEVAL_POLICY}
FALLBACK_POLICY = "fallback_only"
LOOKUP_SEARCH_POLICIES = {*LOOKUP_POLICIES, FALLBACK_POLICY}

ZIM_WEIGHTS = {
    "decision": 0.8,
    "lookup": 0.7,
    "support": 0.55,
    "fallback": 0.35,
}

zim_sources: Dict[str, Path] = {}
zim_client_cache: Dict[str, Any] = {}
opencc_converter = OpenCC("t2s") if OpenCC else None

DOMAIN_KEYWORDS: Dict[str, List[str]] = {
    "water": [
        "water", "drinking water", "wastewater", "filter", "filtration", "disinfect",
        "水", "饮用水", "污水", "过滤", "消毒", "异味", "净水", "水源", "水质", "漂白剂",
    ],
    "food": [
        "food", "cook", "mold", "spoil", "粮", "食物", "食品", "发霉", "霉变", "腐败", "变质",
        "罐头", "做饭", "烹饪",
    ],
    "medical": [
        "medical", "medicine", "health", "fever", "wound", "infection", "bleeding", "diarrhea",
        "dehydration", "first aid", "伤口", "红肿", "发热", "发烧", "感染", "出血", "腹泻", "脱水",
        "急救", "医疗", "药", "烧伤",
    ],
    "power": [
        "battery", "voltage", "current", "solar", "charge", "电池", "锂电池", "鼓包", "插线板",
        "充电", "电线", "太阳能", "电压", "电流", "电源", "短路", "漏电",
    ],
    "repair": [
        "repair", "fix", "leak", "wood", "brake", "修理", "维修", "木工", "螺丝", "胶带",
        "材料", "漏水", "自行车", "刹车", "失灵", "木板", "受潮",
    ],
    "tools": [
        "tool", "tools", "screw", "tape", "工具", "螺丝刀", "扳手", "胶带", "材料", "接线",
    ],
    "communication": [
        "radio", "antenna", "lora", "shortwave", "ham", "无线电", "对讲机", "LoRa", "lora",
        "天线", "短波", "电台", "通信",
    ],
    "geography": [
        "map", "route", "flood", "terrain", "地形", "洪水", "地图", "路线", "水位", "坡度",
        "地势", "风险", "地理",
    ],
    "chemistry": [
        "chemistry", "bleach", "acid", "alkali", "pollutant", "化学", "消毒剂", "漂白", "漂白剂",
        "酸碱", "污染物", "氯", "酒精",
    ],
    "physics": [
        "physics", "voltage", "current", "heat", "pressure", "物理", "电压", "电流", "热量",
        "压力", "温度", "能量",
    ],
    "computer": [
        "computer", "raspberry", "arduino", "sensor", "电脑", "树莓派", "Arduino", "arduino",
        "传感器", "接线", "单片机",
    ],
    "biology": [
        "biology", "cell", "fungus", "mold", "生物", "细胞", "真菌", "霉菌", "发霉", "病虫害",
    ],
    "agriculture": [
        "agriculture", "soil", "garden", "plant", "种植", "土壤", "病虫害", "园艺", "潮湿",
        "堆肥", "育苗",
    ],
    "outdoor": [
        "outdoor", "camp", "hike", "fire", "野外", "露营", "徒步", "装备", "生火", "庇护",
    ],
}

DOMAIN_AI_TOPICS: Dict[str, List[str]] = {
    "medical": ["medicine"],
    "chemistry": ["chemistry"],
    "physics": ["physics"],
    "power": ["physics", "chemistry"],
    "computer": ["computer"],
    "geography": ["geography"],
    "biology": ["molcell"],
    "water": ["chemistry", "medicine", "geography"],
    "food": ["medicine", "molcell"],
    "agriculture": ["molcell", "geography"],
    "communication": ["physics"],
    "repair": ["physics"],
    "tools": ["physics"],
    "outdoor": ["geography"],
}

DOMAIN_STACKEXCHANGE_TOPICS: Dict[str, List[str]] = {
    "water": ["earthscience", "diy", "outdoors", "cooking"],
    "food": ["cooking", "outdoors"],
    "power": ["electronics", "engineering", "diy"],
    "repair": ["diy", "woodworking", "mechanics", "bicycles", "engineering"],
    "tools": ["diy", "woodworking", "mechanics", "bicycles"],
    "communication": ["ham", "electronics"],
    "geography": ["earthscience", "gis", "outdoors"],
    "computer": ["arduino", "raspberrypi", "electronics"],
    "agriculture": ["gardening"],
    "outdoor": ["outdoors", "diy"],
    "physics": ["physics", "electronics", "engineering"],
    "chemistry": ["cooking", "diy"],
    "biology": ["gardening"],
}

DOMAIN_PRIORITY = [
    "medical",
    "power",
    "computer",
    "communication",
    "geography",
    "chemistry",
    "water",
    "food",
    "repair",
    "tools",
    "biology",
    "agriculture",
    "outdoor",
    "physics",
]
QUERY_STOP_TERMS = {
    "怎么", "怎么办", "什么", "哪些", "一下", "这个", "那个", "应该", "可以", "不能",
    "需要", "如果", "还能", "如何", "判断", "检查", "基础",
}
GENERIC_CORE_TERMS = {
    "水", "water", "食物", "food", "能源", "energy", "医疗", "medical", "工具", "tools",
    "风险", "基础",
}
UNRELATED_TITLE_PATTERNS = [
    "案件", "法院", "判决", "诉", "电影", "电视剧", "香水", "体育", "足球", "篮球", "棒球",
    "赛季", "球员", "演员", "歌手", "名人", "专辑", "歌曲", "列表",
    "court", "case", "supreme", "film", "movie", "perfume", "fragrance", "sport",
    "football", "basketball", "baseball", "player", "actor", "actress", "singer", "album",
]


def _strip_html(value: str) -> str:
    text = re.sub(r"(?is)<(script|style).*?>.*?</\1>", " ", value or "")
    text = re.sub(r"(?s)<[^>]+>", " ", text)
    text = unescape(text)
    return re.sub(r"\s+", " ", text).strip()


def _to_simplified(value: Any) -> str:
    text = str(value or "")
    if not text or opencc_converter is None:
        return text
    return opencc_converter.convert(text)


def _simplify_soup_content(node: Any) -> None:
    if BeautifulSoup is None or NavigableString is None:
        return

    for text_node in list(node.find_all(string=True)):
        parent_name = str(getattr(text_node.parent, "name", "") or "").lower()
        if parent_name in {"script", "style", "code", "pre"}:
            continue
        simplified = _to_simplified(str(text_node))
        if simplified != str(text_node):
            text_node.replace_with(NavigableString(simplified))

    for tag in list(node.find_all(True)):
        for attr in ("alt", "title"):
            if tag.has_attr(attr):
                tag[attr] = _to_simplified(tag.get(attr, ""))


ARTICLE_DROP_SELECTORS = [
    "script", "style", "noscript", "template", "svg",
    "header", "footer", "nav",
    ".mw-editsection", ".mw-jump-link", ".printfooter",
    ".metadata", ".noprint", ".catlinks", ".navbox", ".vertical-navbox",
    ".authority-control", ".ambox", ".sistersitebox",
    ".reference", ".references", ".reflist", ".refbegin",
    "ol.references", "div.reflist", "sup.reference",
    "table.infobox", "table.sidebar", "table.navbox",
]

ARTICLE_BODY_SELECTORS = [
    "main article",
    "article",
    "main",
    "#mw-content-text .mw-parser-output",
    "#mw-content-text",
    ".mw-parser-output",
    "#content",
    "body",
]

ARTICLE_ALLOWED_TAGS = {
    "article", "section", "h1", "h2", "h3", "h4", "p", "br",
    "ul", "ol", "li", "strong", "b", "em", "i", "code", "pre",
    "blockquote", "table", "thead", "tbody", "tr", "th", "td", "caption",
    "figure", "figcaption", "img", "a", "span", "div",
}

ARTICLE_ALLOWED_ATTRS = {
    "a": {"href", "title"},
    "img": {"src", "alt", "title", "width", "height"},
    "th": {"colspan", "rowspan"},
    "td": {"colspan", "rowspan"},
}

ARTICLE_STOP_HEADINGS = {
    "参考文献", "參考文獻", "参考资料", "參考資料", "注释", "註釋", "脚注", "來源",
    "外部链接", "外部連結", "参见", "參見", "延伸阅读", "延伸閱讀",
    "相关条目", "相關條目", "规范控制", "规范控制数据库", "權威控制", "权威控制",
}

ARTICLE_DROP_HEADING_PATTERNS = [
    re.compile(r"^(参考|參考|注释|註釋|脚注|來源|外部|参见|參見|延伸|相关|相關|规范|權威)"),
]


def _normalize_article_text(value: str) -> str:
    text = unescape(str(value or ""))
    text = _to_simplified(text)
    text = text.replace("\xa0", " ")
    text = re.sub(r"\[\s*编辑\s*\]", "", text)
    text = re.sub(r"\[\s*編輯\s*\]", "", text)
    text = re.sub(r"\[\s*\d+(?:\.\d+)?\s*\]", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def _drop_following_heading(heading: Any) -> None:
    """Remove a stop heading and following siblings until the next same-or-higher heading."""
    try:
        level = int(str(heading.name)[1]) if str(heading.name).startswith("h") else 2
    except Exception:
        level = 2

    current = heading.next_sibling
    heading.decompose()
    while current is not None:
        next_node = current.next_sibling
        name = getattr(current, "name", "") or ""
        if re.fullmatch(r"h[1-6]", str(name)):
            try:
                next_level = int(str(name)[1])
            except Exception:
                next_level = 6
            if next_level <= level:
                break
        try:
            current.decompose()
        except Exception:
            try:
                current.extract()
            except Exception:
                pass
        current = next_node


def _is_stop_heading(text: str) -> bool:
    text = re.sub(r"\s+", "", str(text or "")).strip("：:")
    if not text:
        return False
    if text in ARTICLE_STOP_HEADINGS:
        return True
    return any(pattern.search(text) for pattern in ARTICLE_DROP_HEADING_PATTERNS)


def _safe_internal_href(href: str) -> str:
    href = str(href or "").strip()
    if not href:
        return ""
    if href.startswith("#"):
        return href
    if href.startswith(("A/", "I/", "-/", "../", "./")):
        return href
    if re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", href):
        # Do not allow javascript/data/http links inside the offline article reader.
        return ""
    return href


def _split_reference(value: str) -> tuple[str, str]:
    value = str(value or "").strip()
    if not value:
        return "", ""

    split = urlsplit(value)
    path = split.path or value.split("#", 1)[0].split("?", 1)[0]
    return path, split.fragment or ""


def _resolve_zim_path(value: str, base_path: str = "") -> str:
    path, _ = _split_reference(value)
    path = unescape(unquote(path)).strip()
    if not path:
        return ""

    path = path.replace("\\", "/")
    while path.startswith("/"):
        path = path[1:]

    if path.startswith("./"):
        path = path[2:]

    if path.startswith("../"):
        base_dir = posixpath.dirname(str(base_path or "").replace("\\", "/"))
        path = posixpath.normpath(posixpath.join(base_dir, path))
    else:
        path = posixpath.normpath(path)

    return "" if path in {"", "."} else path


def _api_resource_url(source_name: str, resource_path: str) -> str:
    return (
        f"/api/kiwix/resource?source={quote(str(source_name or ''), safe='')}"
        f"&path={quote(str(resource_path or ''), safe='')}"
    )


def _api_article_url(source_name: str, article_path: str) -> str:
    return (
        f"/api/kiwix/article?source={quote(str(source_name or ''), safe='')}"
        f"&path={quote(str(article_path or ''), safe='')}"
    )


def _rewrite_zim_references(node: Any, source_name: str, base_path: str) -> None:
    for tag in list(node.find_all("img")):
        raw_src = str(
            tag.get("src")
            or tag.get("data-src")
            or tag.get("data-original")
            or tag.get("data-file")
            or ""
        ).strip()
        if not raw_src:
            tag.decompose()
            continue

        src_path = _resolve_zim_path(raw_src, base_path=base_path)
        if not src_path:
            tag.decompose()
            continue

        tag["src"] = _api_resource_url(source_name, src_path)
        tag["loading"] = "lazy"

    for tag in list(node.find_all("a")):
        raw_href = str(tag.get("href") or "").strip()
        if not raw_href or raw_href.startswith("#"):
            if tag.has_attr("href"):
                del tag["href"]
            continue

        if re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", raw_href):
            if tag.has_attr("href"):
                del tag["href"]
            continue

        article_path = _resolve_zim_path(raw_href, base_path=base_path)
        _, fragment = _split_reference(raw_href)
        if not article_path:
            if tag.has_attr("href"):
                del tag["href"]
            continue

        tag["href"] = _api_article_url(source_name, article_path)
        tag["class"] = "kiwix-internal-link"
        tag["data-kiwix-source"] = source_name
        tag["data-kiwix-path"] = article_path
        if fragment:
            tag["data-kiwix-fragment"] = fragment


def _sanitize_article_node(node: Any) -> None:
    for tag in list(node.find_all(True)):
        name = str(tag.name or "").lower()
        if name not in ARTICLE_ALLOWED_TAGS:
            tag.unwrap()
            continue

        if name == "img":
            src = str(
                tag.get("src")
                or tag.get("data-src")
                or tag.get("data-original")
                or tag.get("data-file")
                or ""
            ).strip()
            if src:
                tag["src"] = src

        allowed = ARTICLE_ALLOWED_ATTRS.get(name, set())
        for attr in list(tag.attrs):
            if attr not in allowed:
                del tag.attrs[attr]

        if name == "a":
            href = _safe_internal_href(tag.get("href", ""))
            if href:
                tag["href"] = href
            elif tag.has_attr("href"):
                del tag["href"]

        if name == "img":
            src = str(tag.get("src") or "").strip()
            if not src or re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", src):
                tag.decompose()


def _remove_empty_article_nodes(node: Any) -> None:
    """Remove empty wrappers left after Wikipedia/Kiwix cleanup."""
    removable_tags = {"a", "span", "div", "section", "p", "figure"}

    changed = True
    while changed:
        changed = False
        for tag in list(node.find_all(removable_tags)):
            has_text = bool(tag.get_text(" ", strip=True))
            has_structural_child = bool(tag.find([
                "img", "table", "ul", "ol", "li",
                "p", "h1", "h2", "h3", "h4", "pre", "code", "blockquote",
            ]))
            if not has_text and not has_structural_child:
                tag.decompose()
                changed = True


def _unwrap_layout_wrappers(node: Any) -> None:
    """Flatten non-semantic wrappers so the reader starts with real article blocks."""
    for tag in list(node.find_all(["span", "div"])):
        # After sanitizing, div/span attributes are already removed. If they only
        # serve layout/template purposes, unwrap them and keep their children.
        if not tag.attrs:
            tag.unwrap()


def _drop_leading_non_content_nodes(node: Any) -> None:
    """Remove leftover anchors/empty strings before the first meaningful block."""
    meaningful_tags = {"p", "h2", "h3", "h4", "ul", "ol", "table", "figure", "blockquote", "pre"}

    for child in list(node.children):
        name = str(getattr(child, "name", "") or "").lower()
        if name in meaningful_tags and getattr(child, "get_text", lambda *a, **k: "")(" ", strip=True):
            break

        text = child.get_text(" ", strip=True) if hasattr(child, "get_text") else str(child).strip()
        if not text:
            try:
                child.decompose()
            except Exception:
                try:
                    child.extract()
                except Exception:
                    pass
            continue

        # If the first textual node is meaningful raw text, keep it.
        break


def _postprocess_article_body(body: Any) -> None:
    _remove_empty_article_nodes(body)
    _unwrap_layout_wrappers(body)
    _remove_empty_article_nodes(body)
    _drop_leading_non_content_nodes(body)


def _select_article_body(soup: Any) -> Any:
    for selector in ARTICLE_BODY_SELECTORS:
        found = soup.select_one(selector)
        if found:
            return found
    return soup


def _clean_html_article(raw_html: str, source_name: str = "", article_path: str = "") -> Dict[str, str]:
    """Return reader-safe article HTML plus normalized text from a ZIM HTML item."""
    if BeautifulSoup is None:
        text = _strip_html(raw_html)
        return {
            "html": f"<article class=\"kiwix-article\"><p>{escape(_to_simplified(unescape(text)))}</p></article>",
            "text": _normalize_article_text(text),
            "content_type": "text/plain",
        }

    soup = BeautifulSoup(raw_html or "", "html.parser")

    for selector in ARTICLE_DROP_SELECTORS:
        for tag in list(soup.select(selector)):
            tag.decompose()

    body = _select_article_body(soup)

    # Remove reference/navigation sections at the source instead of guessing in the browser.
    for heading in list(body.find_all(re.compile(r"^h[1-6]$"))):
        if _is_stop_heading(heading.get_text(" ", strip=True)):
            _drop_following_heading(heading)

    # Wikipedia pages often carry a duplicated page title in h1; the modal already owns the title.
    for h1 in list(body.find_all("h1")):
        h1.decompose()

    _sanitize_article_node(body)
    _postprocess_article_body(body)
    _simplify_soup_content(body)
    _rewrite_zim_references(body, source_name=source_name, base_path=article_path)

    # Convert cleaned content into an explicit article contract for the frontend.
    html = "".join(str(child) for child in body.children).strip()
    if not html:
        html = "<p></p>"
    html = f'<article class="kiwix-article">{html}</article>'

    text = _normalize_article_text(body.get_text(" ", strip=True))
    return {
        "html": html,
        "text": text,
        "content_type": "text/html",
    }


def _compact_text(value: Any) -> str:
    return re.sub(r"[^\u4e00-\u9fffA-Za-z0-9]+", "", _to_simplified(str(value or ""))).lower()


def _unique(items: Iterable[str]) -> List[str]:
    values: List[str] = []
    for item in items:
        item = str(item or "").strip()
        if item and item not in values:
            values.append(item)
    return values


def _text_contains(text: str, term: str) -> bool:
    term = str(term or "").strip()
    if not term:
        return False

    if re.search(r"[\u4e00-\u9fff]", term):
        return _compact_text(term) in _compact_text(text)

    return term.lower() in str(text or "").lower()


def classify_query_domain(query: str) -> List[str]:
    """Classify a query into LanternBox survival knowledge domains."""
    text = str(query or "")
    domains: List[str] = []

    for domain, keywords in DOMAIN_KEYWORDS.items():
        if any(_text_contains(text, keyword) for keyword in keywords):
            domains.append(domain)

    domains.sort(key=lambda item: DOMAIN_PRIORITY.index(item) if item in DOMAIN_PRIORITY else len(DOMAIN_PRIORITY))
    return domains or ["general"]


def extract_query_core_terms(query: str, limit: int = 18) -> List[str]:
    """Extract terms that must appear in a title or snippet for a useful hit."""
    text = str(query or "")
    terms: List[str] = []

    for domain in classify_query_domain(text):
        for keyword in DOMAIN_KEYWORDS.get(domain, []):
            if len(keyword) >= 2 and _text_contains(text, keyword) and keyword not in terms:
                terms.append(keyword)

    for token in re.findall(r"[A-Za-z0-9][A-Za-z0-9_+-]{1,}", text):
        if token not in terms:
            terms.append(token)

    if terms:
        return terms[:limit]

    compact = _compact_text(text)
    for size in (4, 3, 2):
        if len(compact) < size:
            continue
        for index in range(0, len(compact) - size + 1):
            term = compact[index:index + size]
            if not re.search(r"[\u4e00-\u9fff]", term):
                continue
            if term in QUERY_STOP_TERMS or any(stop in term for stop in QUERY_STOP_TERMS):
                continue
            if term not in terms:
                terms.append(term)
            if len(terms) >= limit:
                return terms

    return terms[:limit]


def _matched_core_terms(query: str, title: str, snippet: str) -> Dict[str, List[str]]:
    terms = extract_query_core_terms(query)
    title_matches = [term for term in terms if _text_contains(title, term)]
    snippet_matches = [term for term in terms if _text_contains(snippet, term)]
    matched_terms = _unique([*title_matches, *snippet_matches])

    return {
        "terms": terms,
        "title_matches": title_matches,
        "snippet_matches": snippet_matches,
        "matched_terms": matched_terms,
    }


def _unrelated_penalty(title: str) -> float:
    text = str(title or "")
    if re.search(r"[^预]案$", text):
        return 0.2
    lowered = str(title or "").lower()
    if any(pattern.lower() in lowered for pattern in UNRELATED_TITLE_PATTERNS):
        return 0.2
    return 1.0


def _score_relevant_hit(
    query: str,
    title: str,
    snippet: str,
    raw_score: float,
    zim_weight: float,
) -> Optional[Dict[str, Any]]:
    matches = _matched_core_terms(query, title, snippet)
    matched_terms = matches["matched_terms"]
    if not matched_terms:
        return None

    title_matches = matches["title_matches"]
    snippet_matches = matches["snippet_matches"]
    if matched_terms and all(term in GENERIC_CORE_TERMS for term in matched_terms) and not title_matches:
        return None

    title_score = min(0.52, len(title_matches) * 0.18)
    snippet_score = min(0.28, len(snippet_matches) * 0.08)
    base_score = max(0.0, min(1.0, raw_score * zim_weight)) * 0.25
    score = (title_score + snippet_score + base_score) * _unrelated_penalty(title)

    if score < 0.08:
        return None

    return {
        "score": max(0.0, min(1.0, score)),
        "matched_terms": matched_terms,
        "matched_terms_count": len(matched_terms),
    }


def _topics_for_domains(domains: Iterable[str]) -> List[str]:
    topics: List[str] = []
    for domain in domains:
        topics.extend(DOMAIN_AI_TOPICS.get(domain, []))
    return _unique(topics)


def _stackexchange_topics_for_domains(domains: Iterable[str]) -> List[str]:
    topics: List[str] = []
    for domain in domains:
        topics.extend(DOMAIN_STACKEXCHANGE_TOPICS.get(domain, []))
    return _unique(topics)


def _ai_topic_order(domains: Iterable[str]) -> List[str]:
    topics = _topics_for_domains(domains)
    return _unique([*topics, "all"])


def _source_key(entry: Dict[str, Any]) -> str:
    filename = str(entry.get("filename") or "").strip()
    return Path(filename).stem if filename else "zim"


def _source_from_filename(path: Path) -> str:
    return _source_key(_manifest_entry_from_path(path))


def _topic_from_stackexchange(filename: str) -> str:
    return filename.split(".stackexchange.com_", 1)[0].strip().lower()


def _manifest_entry_from_path(path: Path) -> Dict[str, str]:
    filename = path.name
    lower = filename.lower()

    if ".stackexchange.com_en_all_" in lower:
        return {
            "filename": filename,
            "language": "en",
            "topic": _topic_from_stackexchange(lower),
            "variant": "all",
            "role": "support",
            "usage_policy": "background_support_only",
        }

    if lower.startswith("wiktionary_en_"):
        return {
            "filename": filename,
            "language": "en",
            "topic": "wiktionary",
            "variant": "nopic" if "_nopic_" in lower else "all",
            "role": "support",
            "usage_policy": "language_support_only",
        }

    if "_mini_" in lower:
        return {
            "filename": filename,
            "language": "en" if "_en_" in lower else "zh",
            "topic": "mini",
            "variant": "mini",
            "role": "fallback",
            "usage_policy": FALLBACK_POLICY,
        }

    match = re.match(r"^wikipedia_(?P<language>zh|en)_(?P<topic>[a-z0-9]+)_(?P<variant>maxi|nopic|all)_", lower)
    if match:
        language = match.group("language")
        topic = match.group("topic")
        variant = match.group("variant")
        is_decision = language == "zh" and variant == "nopic"
        return {
            "filename": filename,
            "language": language,
            "topic": topic,
            "variant": variant,
            "role": "decision" if is_decision else "lookup",
            "usage_policy": AI_RETRIEVAL_POLICY if is_decision else "lookup_only",
        }

    return {
        "filename": filename,
        "language": "en" if "_en_" in lower else "zh" if "_zh_" in lower else "en",
        "topic": "all",
        "variant": "all",
        "role": "support",
        "usage_policy": "background_support_only",
    }


def _normalize_manifest_entry(entry: Dict[str, Any]) -> Dict[str, str]:
    filename = str(entry.get("filename") or "").strip()
    inferred = _manifest_entry_from_path(Path(filename)) if filename else {}
    normalized: Dict[str, str] = {}
    for key in ("filename", "language", "topic", "variant", "role", "usage_policy"):
        normalized[key] = str(entry.get(key) or inferred.get(key) or "").strip()
    return normalized


def scan_zim_manifest(zim_dir: Path = DEFAULT_ZIM_DIR) -> List[Dict[str, str]]:
    if not zim_dir.exists():
        return []
    return [_manifest_entry_from_path(path) for path in sorted(zim_dir.glob("*.zim"))]


def load_zim_manifest(manifest_path: Path = DEFAULT_MANIFEST_PATH) -> List[Dict[str, str]]:
    if not manifest_path.exists():
        return []

    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    if isinstance(payload, dict):
        payload = payload.get("zim_files", [])
    if not isinstance(payload, list):
        return []

    entries = [_normalize_manifest_entry(item) for item in payload if isinstance(item, dict)]
    return [entry for entry in entries if entry.get("filename")]


def get_zim_manifest(
    manifest_path: Path = DEFAULT_MANIFEST_PATH,
    zim_dir: Path = DEFAULT_ZIM_DIR,
) -> List[Dict[str, str]]:
    entries = load_zim_manifest(manifest_path)
    return entries if entries else scan_zim_manifest(zim_dir)


def _entry_path(entry: Dict[str, Any], zim_dir: Path = DEFAULT_ZIM_DIR) -> Path:
    return zim_dir / str(entry.get("filename") or "")


def _entry_matches_source(entry: Dict[str, Any], sources: Optional[Iterable[str]]) -> bool:
    if not sources:
        return True

    allowed = {str(source or "").strip().lower() for source in sources if str(source or "").strip()}
    if not allowed:
        return True

    filename = str(entry.get("filename") or "").lower()
    stem = Path(filename).stem
    values = {
        filename,
        stem,
        str(entry.get("topic") or "").lower(),
        str(entry.get("role") or "").lower(),
        str(entry.get("usage_policy") or "").lower(),
    }
    return bool(values & allowed)


def _entries_for_policies(
    usage_policies: Optional[Iterable[str]],
    sources: Optional[Iterable[str]] = None,
) -> List[Dict[str, str]]:
    policies = {str(policy or "").strip() for policy in usage_policies or [] if str(policy or "").strip()}
    entries: List[Dict[str, str]] = []

    for entry in get_zim_manifest():
        if policies and entry.get("usage_policy") not in policies:
            continue
        if not _entry_matches_source(entry, sources):
            continue
        path = _entry_path(entry)
        if path.exists():
            entries.append(entry)

    return entries


def _rank_ai_entry(entry: Dict[str, str], domains: List[str]) -> Optional[int]:
    if entry.get("usage_policy") != AI_RETRIEVAL_POLICY:
        return None
    if entry.get("language") != "zh" or entry.get("variant") != "nopic":
        return None

    topic_order = _ai_topic_order(domains)
    topic = str(entry.get("topic") or "").strip()
    if topic not in topic_order:
        return None
    return topic_order.index(topic)


def _rank_lookup_entry(entry: Dict[str, str], domains: List[str]) -> Optional[int]:
    language = str(entry.get("language") or "").strip()
    topic = str(entry.get("topic") or "").strip()
    variant = str(entry.get("variant") or "").strip()
    policy = str(entry.get("usage_policy") or "").strip()

    if policy not in LOOKUP_SEARCH_POLICIES:
        return None

    related_topics = _topics_for_domains(domains)
    stack_topics = _stackexchange_topics_for_domains(domains)

    if language == "zh" and topic in related_topics and variant == "maxi":
        return 0 * 100 + related_topics.index(topic)
    if language == "zh" and topic in related_topics and variant == "nopic":
        return 1 * 100 + related_topics.index(topic)
    if language == "zh" and topic == "all" and variant in {"maxi", "nopic"}:
        return 2 * 100 + (0 if variant == "maxi" else 1)
    if language == "en" and topic in stack_topics and policy == "background_support_only":
        return 3 * 100 + stack_topics.index(topic)
    if topic == "wiktionary" and policy == "language_support_only":
        return 4 * 100
    if variant == "mini" or policy == FALLBACK_POLICY:
        return 5 * 100

    return None


def _entries_for_query(query: str, channel: str) -> List[Dict[str, str]]:
    domains = classify_query_domain(query)
    ranked: List[tuple[int, str, Dict[str, str]]] = []

    for entry in get_zim_manifest():
        path = _entry_path(entry)
        if not path.exists():
            continue

        rank = _rank_lookup_entry(entry, domains) if channel == "lookup" else _rank_ai_entry(entry, domains)
        if rank is None:
            continue

        ranked.append((rank, str(entry.get("filename") or ""), entry))

    ranked.sort(key=lambda item: (item[0], item[1]))
    return [entry for _, _, entry in ranked]


def planned_zim_sources(query: str, channel: str = "ai") -> List[Dict[str, str]]:
    """Return the manifest sources that will be searched for a query/channel."""
    entries = _entries_for_query(query, "lookup" if channel == "lookup" else "ai")
    return [
        {
            "source": _source_key(entry),
            "filename": str(entry.get("filename") or ""),
            "language": str(entry.get("language") or ""),
            "topic": str(entry.get("topic") or ""),
            "variant": str(entry.get("variant") or ""),
            "role": str(entry.get("role") or ""),
            "usage_policy": str(entry.get("usage_policy") or ""),
        }
        for entry in entries
    ]


def discover_zim_sources(zim_dir: Path = DEFAULT_ZIM_DIR) -> Dict[str, Path]:
    discovered: Dict[str, Path] = {}

    for entry in get_zim_manifest(zim_dir=zim_dir):
        path = _entry_path(entry, zim_dir=zim_dir)
        if not path.exists():
            continue
        discovered[_source_key(entry)] = path

    return discovered


def register_zim_source(source: str, file_path: str) -> None:
    source = str(source or "").strip()
    path = Path(file_path)
    if source and path.exists():
        zim_sources[source] = path


def refresh_zim_sources() -> Dict[str, Path]:
    zim_sources.clear()
    zim_client_cache.clear()
    zim_sources.update(discover_zim_sources())
    return dict(zim_sources)


def _source_weight(source: str, metadata: Optional[Dict[str, Any]] = None) -> float:
    role = str((metadata or {}).get("role") or "").strip()
    if role:
        return ZIM_WEIGHTS.get(role, ZIM_WEIGHTS["support"])

    source = str(source or "").lower()
    if "medicine" in source or "medical" in source:
        return ZIM_WEIGHTS["decision"]
    if "wiktionary" in source:
        return ZIM_WEIGHTS["support"]
    return ZIM_WEIGHTS["decision"]


class ZimClient:
    def __init__(
        self,
        source_name: str = "wiki",
        zim_weight: Optional[float] = None,
        metadata: Optional[Dict[str, Any]] = None,
    ) -> None:
        self.source_name = source_name
        self.metadata = metadata or {}
        self.zim_weight = zim_weight if zim_weight is not None else _source_weight(source_name, self.metadata)
        self.file_path: Optional[Path] = None
        self.archive = None
        self.searcher = None

    def load_zim(self, file_path: str) -> bool:
        try:
            from libzim.reader import Archive
            from libzim.search import Searcher
        except Exception:
            self.archive = None
            self.searcher = None
            return False

        path = Path(file_path)
        if not path.exists():
            self.archive = None
            self.searcher = None
            return False

        try:
            self.archive = Archive(str(path))
            self.searcher = Searcher(self.archive)
            self.file_path = path
            return True
        except Exception:
            self.archive = None
            self.searcher = None
            return False

    def search(self, query: str, limit: int = 5) -> List[Dict]:
        if not self.archive or not self.searcher:
            return []

        query = str(query or "").strip()
        if not query:
            return []

        try:
            from libzim.search import Query

            search = self.searcher.search(Query().set_query(query))
            estimated = max(int(search.getEstimatedMatches() or 0), 1)
            paths = list(search.getResults(0, limit * 3))
        except Exception:
            return []

        results = []
        seen = set()

        for rank, path in enumerate(paths, start=1):
            if path in seen:
                continue
            seen.add(path)

            article = self.get_article(str(path))
            if not article:
                continue

            raw_score = max(0.05, min(1.0, 1.0 - ((rank - 1) / max(estimated, limit, 1))))
            snippet = self.extract_snippet(article["content"])
            relevance = _score_relevant_hit(
                query=query,
                title=article["title"],
                snippet=snippet,
                raw_score=raw_score,
                zim_weight=self.zim_weight,
            )
            if not relevance:
                continue

            result = {
                "title": article["title"],
                "snippet": snippet,
                "source": "kiwix_zim",
                "zim_source": self.source_name,
                "url": article.get("url"),
                "article_path": article.get("path"),
                "raw_score": round(raw_score, 4),
                "score": round(float(relevance["score"]), 4),
                "matched_terms": relevance["matched_terms"],
                "matched_terms_count": relevance["matched_terms_count"],
            }
            result.update(
                {
                    "zim_filename": self.metadata.get("filename"),
                    "language": self.metadata.get("language"),
                    "topic": self.metadata.get("topic"),
                    "variant": self.metadata.get("variant"),
                    "role": self.metadata.get("role"),
                    "usage_policy": self.metadata.get("usage_policy"),
                }
            )
            results.append(result)

            if len(results) >= limit:
                break

        return results

    def get_article(self, title: str) -> Optional[Dict]:
        if not self.archive:
            return None

        title = str(title or "").strip()
        if not title:
            return None

        decoded_title = unquote(title)
        normalized_title = _resolve_zim_path(decoded_title) or decoded_title
        candidates = _unique([
            title,
            decoded_title,
            normalized_title,
            normalized_title.replace(" ", "_"),
            normalized_title.replace("_", " "),
        ])

        entry = None
        for candidate in candidates:
            try:
                entry = self.archive.get_entry_by_path(candidate)
                break
            except Exception:
                pass

            try:
                entry = self.archive.get_entry_by_title(candidate)
                break
            except Exception:
                pass

        if not entry:
            return None

        try:
            item = entry.get_item()
            raw_html = bytes(item.content).decode("utf-8", errors="ignore")
            path = str(item.path or getattr(entry, "path", "") or title)
            cleaned = _clean_html_article(
                raw_html,
                source_name=self.source_name,
                article_path=path,
            )
            return {
                "title": _to_simplified(str(item.title or entry.title or title)),
                "html": cleaned["html"],
                "text": cleaned["text"],
                "content": cleaned["text"],  # Backward-compatible alias for old callers.
                "raw_html": raw_html,
                "content_type": cleaned["content_type"],
                "path": path,
                "url": f"zim://{self.source_name}/{path}",
            }
        except Exception:
            return None

    def get_resource(self, resource_path: str) -> Optional[Dict[str, Any]]:
        if not self.archive:
            return None

        normalized_path = _resolve_zim_path(resource_path)
        if not normalized_path:
            return None

        candidates = _unique([
            resource_path,
            normalized_path,
            f"./{normalized_path}",
            unquote(resource_path),
            unquote(normalized_path),
        ])

        entry = None
        for candidate in candidates:
            try:
                entry = self.archive.get_entry_by_path(candidate)
                break
            except Exception:
                pass

        if not entry:
            return None

        try:
            item = entry.get_item()
            content = bytes(item.content)
            path = str(item.path or getattr(entry, "path", "") or normalized_path)
            content_type = str(getattr(item, "mimetype", "") or "").strip()
            if not content_type:
                content_type = mimetypes.guess_type(path)[0] or "application/octet-stream"
            return {
                "content": content,
                "content_type": content_type,
                "path": path,
            }
        except Exception:
            return None

    def extract_snippet(self, content: str, limit: int = 360) -> str:
        text = re.sub(r"\s+", " ", _to_simplified(str(content or ""))).strip()
        return text[:limit]


def _default_zim_file() -> Optional[Path]:
    if not zim_sources:
        refresh_zim_sources()
    return next(iter(zim_sources.values()), None)


def load_zim(file_path: str):
    client = ZimClient()
    client.load_zim(file_path)
    return client


def load_all_zims(sources: Optional[Dict[str, Path]] = None) -> Dict[str, ZimClient]:
    source_map = sources or refresh_zim_sources()
    clients: Dict[str, ZimClient] = {}

    metadata_by_source = {_source_key(entry): entry for entry in get_zim_manifest()}
    for source, path in source_map.items():
        client = ZimClient(source_name=source, metadata=metadata_by_source.get(source, {}))
        if client.load_zim(str(path)):
            clients[source] = client

    return clients


def _client_for_entry(entry: Dict[str, str]) -> Optional[ZimClient]:
    source_name = _source_key(entry)
    path = _entry_path(entry)
    cache_key = str(path)

    cached = zim_client_cache.get(cache_key)
    if cached and getattr(cached, "archive", None) and getattr(cached, "searcher", None):
        return cached

    client = ZimClient(source_name=source_name, metadata=entry)
    if not client.load_zim(str(path)):
        return None

    zim_client_cache[cache_key] = client
    return client


def _search_entries(query: str, entries: List[Dict[str, str]], limit: int) -> List[Dict]:
    if not entries:
        return []

    merged: List[Dict] = []
    seen = set()

    for source_rank, entry in enumerate(entries):
        client = _client_for_entry(entry)
        if not client:
            continue

        for hit in client.search(query, limit=limit):
            key = (hit.get("zim_filename"), hit.get("title"))
            if key in seen:
                continue
            seen.add(key)
            hit["_source_rank"] = source_rank
            merged.append(hit)

    merged.sort(
        key=lambda item: (
            int(item.get("_source_rank") or 0),
            -float(item.get("score") or 0.0),
            str(item.get("usage_policy") or ""),
            str(item.get("zim_filename") or ""),
            str(item.get("title") or ""),
        )
    )
    for item in merged:
        item.pop("_source_rank", None)
    return merged[:limit]


def search(
    query: str,
    limit: int = 5,
    file_path: Optional[str] = None,
    source: Optional[str] = None,
    sources: Optional[List[str]] = None,
    usage_policies: Optional[List[str]] = None,
) -> List[Dict]:
    """Compatibility search entry.

    Without an explicit policy, this is intentionally restricted to the AI
    decision channel so old callers cannot accidentally pull maxi or English
    support ZIMs into scoring paths.
    """
    if file_path:
        path = Path(file_path)
        metadata = _manifest_entry_from_path(path)
        client = ZimClient(source_name=source or _source_key(metadata), metadata=metadata)
        if not client.load_zim(str(path)):
            return []
        return client.search(query, limit=limit)

    policies = usage_policies or [AI_RETRIEVAL_POLICY]
    explicit_sources = sources or ([source] if source else None)
    policy_set = set(policies)
    if not explicit_sources and policy_set == {AI_RETRIEVAL_POLICY}:
        entries = _entries_for_query(query, "ai")
    elif not explicit_sources and policy_set <= LOOKUP_SEARCH_POLICIES:
        entries = _entries_for_query(query, "lookup")
    else:
        entries = _entries_for_policies(policies, explicit_sources)
    return _search_entries(query, entries, limit)


def query_for_ai(query: str, limit: int = 5) -> List[Dict]:
    return search(query=query, limit=limit, usage_policies=[AI_RETRIEVAL_POLICY])


def query_for_lookup(query: str, limit: int = 8) -> List[Dict]:
    return search(query=query, limit=limit, usage_policies=sorted(LOOKUP_SEARCH_POLICIES))


def get_article(title: str, file_path: Optional[str] = None, source: Optional[str] = None) -> Optional[Dict]:
    metadata = None
    if source and not file_path:
        if not zim_sources:
            refresh_zim_sources()
        path = zim_sources.get(source)
        metadata_by_source = {_source_key(entry): entry for entry in get_zim_manifest()}
        metadata = metadata_by_source.get(source)
    else:
        path = Path(file_path) if file_path else _default_zim_file()

    if not path:
        return None

    metadata = metadata or _manifest_entry_from_path(path)
    client = _client_for_entry(metadata) if metadata.get("filename") else None
    if not client:
        client = ZimClient(source_name=source or _source_key(metadata), metadata=metadata)
        if not client.load_zim(str(path)):
            return None
    return client.get_article(title)


def get_resource(resource_path: str, file_path: Optional[str] = None, source: Optional[str] = None) -> Optional[Dict[str, Any]]:
    metadata = None
    if source and not file_path:
        if not zim_sources:
            refresh_zim_sources()
        path = zim_sources.get(source)
        metadata_by_source = {_source_key(entry): entry for entry in get_zim_manifest()}
        metadata = metadata_by_source.get(source)
    else:
        path = Path(file_path) if file_path else _default_zim_file()

    if not path:
        return None

    metadata = metadata or _manifest_entry_from_path(path)
    client = _client_for_entry(metadata) if metadata.get("filename") else None
    if not client:
        client = ZimClient(source_name=source or _source_key(metadata), metadata=metadata)
        if not client.load_zim(str(path)):
            return None
    return client.get_resource(resource_path)


def extract_snippet(content: str, limit: int = 360) -> str:
    return ZimClient().extract_snippet(content, limit=limit)


refresh_zim_sources()
