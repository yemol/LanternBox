import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SOURCE = ROOT / "data" / "retrieval_benchmark"
OUTPUT = ROOT / "data" / "retrieval_benchmark.json"

cases = []
ids = set()

def build():
    for folder in sorted(SOURCE.iterdir()):

        if not folder.is_dir():
            continue

        for file in sorted(folder.glob("*.json")):

            obj = json.loads(file.read_text(encoding="utf-8"))

            if obj["id"] in ids:
                raise ValueError(f"重复 Benchmark ID：{obj['id']}")

            ids.add(obj["id"])

            cases.append(obj)

    cases.sort(key=lambda x: x["id"])

    OUTPUT.write_text(
        json.dumps(
            {
                "version": "1.0.0",
                "cases": cases,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )

    print(f"✓ Generated {len(cases)} Benchmark Cases")