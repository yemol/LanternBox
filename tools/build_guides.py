import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SOURCE = ROOT / "data" / "guides"
OUTPUT = ROOT / "data" / "emergency_guides.json"

def build():
    guides = []
    ids = set()

    for category in sorted(SOURCE.iterdir()):

        if not category.is_dir():
            continue

        for file in sorted(category.glob("*.json")):

            guide = json.loads(file.read_text(encoding="utf-8"))

            if guide["id"] in ids:
                raise ValueError(f"重复 Guide ID：{guide['id']}")

            ids.add(guide["id"])

            guides.append(guide)

    guides.sort(
        key=lambda x: (
            x.get("priority", 0),
            x["id"],
        ),
        reverse=True,
    )

    OUTPUT.write_text(
        json.dumps(guides, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    print(f"✓ Generated {len(guides)} Guides")