import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SOURCE = ROOT / "data" / "guides"
OUTPUT = ROOT / "data" / "emergency_guides.json"
INDEX_OUTPUT = ROOT / "data" / "guide_index.json"
OVERRIDES = ROOT / "data" / "guide_ranking_overrides.json"


REQUIRED_FIELDS = [
    "id",
    "title",
    "category",
    "scenario",
    "goal",
    "steps",
]

def load_overrides():
    if not OVERRIDES.exists():
        return {}

    return json.loads(OVERRIDES.read_text(encoding="utf-8"))


def apply_overrides(guide, overrides):
    guide_id = guide.get("id")
    patch = overrides.get(guide_id)

    if not patch:
        return guide

    merged = dict(guide)

    for key, value in patch.items():
        merged[key] = value

    return merged

def load_guides():
    if not SOURCE.exists():
        raise FileNotFoundError(f"Guide source directory not found: {SOURCE}")

    overrides = load_overrides()

    guides = []
    ids = set()

    for file in sorted(SOURCE.rglob("*.json")):
        guide = json.loads(file.read_text(encoding="utf-8"))

        for field in REQUIRED_FIELDS:
            if field not in guide:
                raise ValueError(f"{file} 缺少字段：{field}")

        guide = apply_overrides(guide, overrides)

        guide_id = guide["id"]

        if guide_id in ids:
            raise ValueError(f"重复 Guide ID：{guide_id}")

        ids.add(guide_id)
        guides.append(guide)

    guides.sort(key=lambda item: item["id"])

    return guides


def build_index(guides):
    index = []

    for guide in guides:
        index.append(
            {
                "id": guide.get("id"),
                "title": guide.get("title"),
                "category": guide.get("category"),
                "category_original": guide.get("category_original"),
                "domains": guide.get("domains", []),
                "intents": guide.get("intents", []),
                "situations": guide.get("situations", []),
                "objects": guide.get("objects", []),
                "signals": guide.get("signals", []),
                "risks": guide.get("risks", []),
                "keywords": guide.get("keywords", []),
                "negative_keywords": guide.get("negative_keywords", []),
                "ranking_role": guide.get("ranking_role"),
                "guide_type": guide.get("guide_type"),
                "primary_domain": guide.get("primary_domain"),
                "primary_intent": guide.get("primary_intent"),
                "top1_aliases": guide.get("top1_aliases", []),
            }
        )

    return index


def build():
    guides = load_guides()
    index = build_index(guides)

    OUTPUT.write_text(
        json.dumps(guides, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    INDEX_OUTPUT.write_text(
        json.dumps(index, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    print(f"✓ Generated {len(guides)} Guides")
    print(f"✓ Generated {len(index)} Guide Index Items")


if __name__ == "__main__":
    build()