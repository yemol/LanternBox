import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SOURCE_DIR = ROOT / "data" / "context_profiles"
OUTPUT_FILE = ROOT / "data" / "context_profiles.json"


def load_profiles():
    profiles = []

    ids = set()

    for file in sorted(SOURCE_DIR.glob("*.json")):

        with open(file, "r", encoding="utf-8") as f:
            profile = json.load(f)

        required = [
            "id",
            "name",
            "match",
            "context",
            "priority",
        ]

        for key in required:
            if key not in profile:
                raise ValueError(f"{file.name} 缺少字段：{key}")

        if profile["id"] in ids:
            raise ValueError(f"重复 id：{profile['id']}")

        ids.add(profile["id"])

        profiles.append(profile)

    profiles.sort(
        key=lambda p: (
            -p.get("priority", 0),
            p["id"],
        )
    )

    return profiles


def build():

    profiles = load_profiles()

    with open(
        OUTPUT_FILE,
        "w",
        encoding="utf-8",
    ) as f:

        json.dump(
            profiles,
            f,
            ensure_ascii=False,
            indent=2,
        )

    print(f"✓ Generated {len(profiles)} Context Profiles")


if __name__ == "__main__":
    build()