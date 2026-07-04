"""Offline-only knowledge coverage gap detector.

This module only inspects resources already present in the LanternBox workspace.
It must not generate runtime download plans, discover external resources, or
depend on future connectivity.
"""

import json
from pathlib import Path
from typing import Any, Dict, List


ROOT = Path(__file__).resolve().parents[2]
GUIDE_INDEX_FILE = ROOT / "data" / "guide_index.json"
WIKI_IMPORT_DIR = ROOT / "wiki_import"

DOMAINS = [
    "water",
    "food",
    "medical",
    "power",
    "shelter",
    "evacuation",
    "hygiene",
    "security",
    "tools",
    "communication",
    "planting",
    "wild_food",
    "repair",
    "psychology",
]

DOMAIN_ALIASES = {
    "communication": {"communication", "comms"},
    "wild_food": {"wild_food", "wild-food", "wild food"},
}

GUIDE_TARGET = 10
WIKI_TARGET = 6


def _aliases_for(domain: str) -> set[str]:
    aliases = DOMAIN_ALIASES.get(domain, {domain})
    return {item.lower() for item in aliases}


def _load_json(path: Path, default: Any) -> Any:
    if not path.exists():
        return default

    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return default


def _parse_front_matter(path: Path) -> Dict[str, str]:
    try:
        text = path.read_text(encoding="utf-8")
    except Exception:
        return {}

    if not text.startswith("---"):
        return {}

    parts = text.split("---", 2)
    if len(parts) < 3:
        return {}

    metadata: Dict[str, str] = {}
    for line in parts[1].splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        metadata[key.strip()] = value.strip()
    return metadata


def _guide_counts() -> Dict[str, int]:
    counts = {domain: 0 for domain in DOMAINS}
    guides = _load_json(GUIDE_INDEX_FILE, [])

    for guide in guides:
        guide_domains = {
            str(item or "").lower()
            for item in guide.get("domains", []) or []
        }

        for domain in DOMAINS:
            if guide_domains & _aliases_for(domain):
                counts[domain] += 1

    return counts


def _wiki_counts() -> Dict[str, int]:
    counts = {domain: 0 for domain in DOMAINS}
    if not WIKI_IMPORT_DIR.exists():
        return counts

    for path in WIKI_IMPORT_DIR.rglob("*.md"):
        parts = {part.lower() for part in path.relative_to(WIKI_IMPORT_DIR).parts}
        metadata = _parse_front_matter(path)
        metadata_text = " ".join([
            metadata.get("category", ""),
            metadata.get("tags", ""),
            metadata.get("kiwix_topics", ""),
            metadata.get("slug", ""),
        ]).lower()

        for domain in DOMAINS:
            aliases = _aliases_for(domain)
            if parts & aliases or any(alias in metadata_text for alias in aliases):
                counts[domain] += 1

    return counts


def _zim_distribution() -> Dict[str, int]:
    counts = {domain: 0 for domain in DOMAINS}

    try:
        from api.kiwix.zim_client import zim_sources
    except Exception:
        return counts

    for source in zim_sources:
        source = str(source or "").lower()
        if source == "medical":
            counts["medical"] += 1
        elif source == "dictionary":
            counts["communication"] += 1
        elif source == "wiki":
            for domain in DOMAINS:
                counts[domain] += 1

    return counts


def _gap_level(guide_count: int, wiki_count: int, zim_count: int) -> str:
    total = guide_count + wiki_count + zim_count
    if total == 0:
        return "missing"
    if guide_count < 2 or wiki_count == 0:
        return "high"
    if guide_count < GUIDE_TARGET or wiki_count < WIKI_TARGET:
        return "medium"
    return "low"


def _coverage_score(guide_count: int, wiki_count: int, zim_count: int) -> float:
    guide_score = min(guide_count / GUIDE_TARGET, 1.0) * 0.55
    wiki_score = min(wiki_count / WIKI_TARGET, 1.0) * 0.35
    zim_score = min(zim_count, 1) * 0.10
    return round(guide_score + wiki_score + zim_score, 4)


def detect_domain_coverage() -> Dict[str, Dict[str, Any]]:
    guide_counts = _guide_counts()
    wiki_counts = _wiki_counts()
    zim_counts = _zim_distribution()

    coverage: Dict[str, Dict[str, Any]] = {}
    for domain in DOMAINS:
        guide_count = guide_counts.get(domain, 0)
        wiki_count = wiki_counts.get(domain, 0)
        zim_count = zim_counts.get(domain, 0)

        coverage[domain] = {
            "coverage_score": _coverage_score(guide_count, wiki_count, zim_count),
            "source_distribution": {
                "guide": guide_count,
                "wiki": wiki_count,
                "kiwix_zim": zim_count,
            },
            "gap_level": _gap_level(guide_count, wiki_count, zim_count),
        }

    return coverage


def _recommendation_for(domain: str, item: Dict[str, Any]) -> Dict[str, str]:
    distribution = item.get("source_distribution", {})
    guide_count = int(distribution.get("guide", 0) or 0)
    wiki_count = int(distribution.get("wiki", 0) or 0)
    zim_count = int(distribution.get("kiwix_zim", 0) or 0)
    gap_level = item.get("gap_level", "medium")

    if gap_level == "missing":
        return {
            "domain": domain,
            "issue": "本地 Guide/Wiki/Kiwix 均未覆盖该领域。",
            "suggestion": "基于现有本地资料先补一组最小 Guide 和 Wiki 条目；仅允许未来人工扩展，不依赖外部获取。",
            "priority": "P0",
        }

    if guide_count < 2:
        return {
            "domain": domain,
            "issue": "本地 Guide 数量不足，行动边界可能不稳定。",
            "suggestion": "从已有团队流程和本地知识中补足关键 Guide，优先覆盖停止条件、优先级和低资源替代方案。",
            "priority": "P1",
        }

    if wiki_count == 0:
        return {
            "domain": domain,
            "issue": "缺少精选 Wiki 背景解释。",
            "suggestion": "用现有 wiki_import 规范补写背景、判断标准和常见误区；Kiwix 只能作为已有分类补充。",
            "priority": "P1",
        }

    if gap_level == "medium":
        return {
            "domain": domain,
            "issue": "覆盖存在但来源厚度不足。",
            "suggestion": "复核已有 Guide/Wiki 的覆盖面，补齐高频场景、记录字段和复查触发条件。",
            "priority": "P2",
        }

    if zim_count == 0:
        return {
            "domain": domain,
            "issue": "已有 Guide/Wiki 可用，但缺少本地已注册 ZIM 背景补充。",
            "suggestion": "未来人工扩展时可把已有 ZIM 分类纳入复核；保持离线静态清单。",
            "priority": "P2",
        }

    return {
        "domain": domain,
        "issue": "覆盖基本稳定。",
        "suggestion": "保持本地资料复核节奏，优先维护现有 Guide/Wiki 的准确性。",
        "priority": "P2",
    }


def analyze_coverage_gap() -> Dict[str, Any]:
    coverage = detect_domain_coverage()

    missing_domains = [
        domain
        for domain, item in coverage.items()
        if item["gap_level"] == "missing"
    ]
    low_confidence_domains = [
        domain
        for domain, item in coverage.items()
        if item["gap_level"] in {"high", "medium"}
    ]

    recommendations = [
        _recommendation_for(domain, coverage[domain])
        for domain in [*missing_domains, *low_confidence_domains]
    ]

    return {
        "missing_domains": missing_domains,
        "low_confidence_domains": low_confidence_domains,
        "recommendations": recommendations,
    }
