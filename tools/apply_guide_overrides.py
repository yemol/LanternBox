import json
from pathlib import Path
from typing import Any, Dict


ROOT = Path(__file__).resolve().parents[1]
GUIDES_DIR = ROOT / "data" / "guides"
OVERRIDES_FILE = ROOT / "data" / "guide_ranking_overrides.json"


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: Any) -> None:
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def index_guides() -> Dict[str, Path]:
    guide_paths: Dict[str, Path] = {}

    for path in GUIDES_DIR.rglob("*.json"):
        guide = load_json(path)
        guide_id = guide.get("id")

        if not guide_id:
            continue

        if guide_id in guide_paths:
            raise ValueError(
                f"Duplicate guide id found: {guide_id}\n"
                f"- {guide_paths[guide_id]}\n"
                f"- {path}"
            )

        guide_paths[guide_id] = path

    return guide_paths


def apply_override(
    guide: Dict[str, Any],
    override: Dict[str, Any],
) -> Dict[str, Any]:
    merged = dict(guide)

    for key, value in override.items():
        merged[key] = value

    return merged


def main() -> None:
    if not OVERRIDES_FILE.exists():
        print(f"No override file found: {OVERRIDES_FILE}")
        return

    overrides = load_json(OVERRIDES_FILE)

    if not isinstance(overrides, dict):
        raise ValueError("guide_ranking_overrides.json must be a JSON object")

    guide_paths = index_guides()

    applied = 0
    missing = []

    for guide_id, override in overrides.items():
        path = guide_paths.get(guide_id)

        if not path:
            missing.append(guide_id)
            continue

        guide = load_json(path)
        merged = apply_override(guide, override)
        write_json(path, merged)

        applied += 1
        print(f"✓ Applied {guide_id} -> {path.relative_to(ROOT)}")

    print()
    print(f"Applied overrides: {applied}")

    if missing:
        print()
        print("Missing guide ids:")
        for guide_id in missing:
            print(f"- {guide_id}")

        raise SystemExit(1)

    print("✓ Guide overrides applied")


if __name__ == "__main__":
    main()