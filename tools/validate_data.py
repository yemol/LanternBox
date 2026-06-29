import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DATA = ROOT / "data"


def validate_directory(directory, required_fields):

    for file in directory.rglob("*.json"):

        obj = json.loads(file.read_text(encoding="utf-8"))

        for field in required_fields:

            if field not in obj:
                raise ValueError(f"{file} 缺少字段：{field}")

    print(f"✓ {directory.name}")

def build():
    validate_directory(
        DATA / "context_profiles",
        [
            "id",
            "name",
            "match",
            "context",
            "priority",
        ],
    )

    validate_directory(
        DATA / "guides",
        [
            "id",
            "title",
        ],
    )

    print("✓ Validate Finished")