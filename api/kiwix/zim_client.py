"""Minimal local ZIM reader for the Kiwix enrichment layer."""

import re
from html import unescape
from pathlib import Path
from typing import Dict, Iterable, List, Optional


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ZIM_DIR = ROOT / "data" / "kiwix" / "zim"
ZIM_WEIGHTS = {
    "medical": 1.0,
    "wiki": 0.8,
    "dictionary": 0.6,
}
zim_sources: Dict[str, Path] = {}


def _strip_html(value: str) -> str:
    text = re.sub(r"(?is)<(script|style).*?>.*?</\1>", " ", value or "")
    text = re.sub(r"(?s)<[^>]+>", " ", text)
    text = unescape(text)
    return re.sub(r"\s+", " ", text).strip()


def _source_from_filename(path: Path) -> str:
    name = path.name.lower()
    if any(marker in name for marker in ("medical", "medicine", "medwiki", "health")):
        return "medical"
    if any(marker in name for marker in ("dictionary", "wiktionary", "dict")):
        return "dictionary"
    return "wiki"


def discover_zim_sources(zim_dir: Path = DEFAULT_ZIM_DIR) -> Dict[str, Path]:
    if not zim_dir.exists():
        return {}

    discovered: Dict[str, Path] = {}
    counters: Dict[str, int] = {}

    for path in sorted(zim_dir.glob("*.zim")):
        source = _source_from_filename(path)
        key = source

        if key in discovered:
            counters[source] = counters.get(source, 1) + 1
            key = f"{source}_{counters[source]}"

        discovered[key] = path

    return discovered


def register_zim_source(source: str, file_path: str) -> None:
    source = str(source or "").strip()
    path = Path(file_path)
    if source and path.exists():
        zim_sources[source] = path


def refresh_zim_sources() -> Dict[str, Path]:
    zim_sources.clear()
    zim_sources.update(discover_zim_sources())
    return dict(zim_sources)


def _source_weight(source: str) -> float:
    base_source = str(source or "wiki").split("_", 1)[0]
    return ZIM_WEIGHTS.get(base_source, ZIM_WEIGHTS["wiki"])


def _ordered_sources(preferred: Optional[Iterable[str]] = None) -> List[str]:
    if not zim_sources:
        refresh_zim_sources()

    if not zim_sources:
        return []

    ordered: List[str] = []
    for source in preferred or []:
        source = str(source or "").strip()
        if source in zim_sources and source not in ordered:
            ordered.append(source)

    for source in sorted(zim_sources):
        if source not in ordered:
            ordered.append(source)

    return ordered


class ZimClient:
    def __init__(self, source_name: str = "wiki", zim_weight: Optional[float] = None) -> None:
        self.source_name = source_name
        self.zim_weight = zim_weight if zim_weight is not None else _source_weight(source_name)
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
            score = max(0.0, min(1.0, raw_score * self.zim_weight))
            results.append(
                {
                    "title": article["title"],
                    "snippet": self.extract_snippet(article["content"]),
                    "source": "kiwix_zim",
                    "zim_source": self.source_name,
                    "url": article.get("url"),
                    "raw_score": round(raw_score, 4),
                    "score": round(score, 4),
                }
            )

            if len(results) >= limit:
                break

        return results

    def get_article(self, title: str) -> Optional[Dict]:
        if not self.archive:
            return None

        title = str(title or "").strip()
        if not title:
            return None

        candidates = [
            title,
            title.replace(" ", "_"),
            title.replace("_", " "),
        ]

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
            content = bytes(item.content).decode("utf-8", errors="ignore")
            return {
                "title": str(item.title or entry.title or title),
                "content": _strip_html(content),
                "url": f"zim://{self.source_name}/{item.path}",
            }
        except Exception:
            return None

    def extract_snippet(self, content: str, limit: int = 360) -> str:
        text = re.sub(r"\s+", " ", str(content or "")).strip()
        return text[:limit]


def _default_zim_file() -> Optional[Path]:
    if not zim_sources:
        refresh_zim_sources()
    return zim_sources.get("wiki") or next(iter(zim_sources.values()), None)


def load_zim(file_path: str):
    client = ZimClient()
    client.load_zim(file_path)
    return client


def load_all_zims(sources: Optional[Dict[str, Path]] = None) -> Dict[str, ZimClient]:
    source_map = sources or refresh_zim_sources()
    clients: Dict[str, ZimClient] = {}

    for source, path in source_map.items():
        client = ZimClient(source_name=source)
        if client.load_zim(str(path)):
            clients[source] = client

    return clients


def search(
    query: str,
    limit: int = 5,
    file_path: Optional[str] = None,
    source: Optional[str] = None,
    sources: Optional[List[str]] = None,
) -> List[Dict]:
    if file_path:
        client = ZimClient(source_name=source or "wiki")
        if not client.load_zim(str(Path(file_path))):
            return []
        return client.search(query, limit=limit)

    source_order = _ordered_sources(sources or ([source] if source else None))
    if not source_order:
        return []

    clients = load_all_zims({item: zim_sources[item] for item in source_order if item in zim_sources})
    merged: List[Dict] = []
    seen = set()

    for source_name in source_order:
        client = clients.get(source_name)
        if not client:
            continue

        for hit in client.search(query, limit=limit):
            key = (hit.get("zim_source"), hit.get("title"))
            if key in seen:
                continue
            seen.add(key)
            merged.append(hit)

    merged.sort(key=lambda item: (-float(item.get("score") or 0.0), str(item.get("zim_source") or ""), str(item.get("title") or "")))
    return merged[:limit]


def get_article(title: str, file_path: Optional[str] = None, source: Optional[str] = None) -> Optional[Dict]:
    if source and not file_path:
        if not zim_sources:
            refresh_zim_sources()
        path = zim_sources.get(source)
    else:
        path = Path(file_path) if file_path else _default_zim_file()

    if not path:
        return None

    client = ZimClient(source_name=source or _source_from_filename(path))
    if not client.load_zim(str(path)):
        return None
    return client.get_article(title)


def extract_snippet(content: str, limit: int = 360) -> str:
    return ZimClient().extract_snippet(content, limit=limit)


refresh_zim_sources()
