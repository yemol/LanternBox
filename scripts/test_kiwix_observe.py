import json
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"

if not os.environ.get("LANTERNBOX_KIWIX_OBSERVE_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_KIWIX_OBSERVE_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])

sys.path.insert(0, str(ROOT))

from api.kiwix.orchestrator import run_kiwix_query
from api.kiwix.zim_client import DEFAULT_ZIM_DIR, search as search_zim, zim_sources


TEST_QUERIES = [
    "United States",
    "water",
    "medicine",
    "energy",
]


def main():
    zim_files = sorted(str(path) for path in DEFAULT_ZIM_DIR.glob("*.zim")) if DEFAULT_ZIM_DIR.exists() else []
    direct_hits = []
    integrated_hits = []
    zim_source_breakdown = {}

    for query in TEST_QUERIES:
        direct = search_zim(query, limit=3)
        integrated = run_kiwix_query(query, {"core_terms": query.split()})

        direct_hits.extend(direct)
        for item in direct:
            source = item.get("zim_source", "unknown")
            zim_source_breakdown[source] = zim_source_breakdown.get(source, 0) + 1

        integrated_hits.extend([
            item.to_dict()
            for item in integrated
            if item.source == "kiwix_zim"
        ])

    report = {
        "zim_files": zim_files,
        "zim_sources": {source: str(path) for source, path in zim_sources.items()},
        "kiwix_zim_hits": len(direct_hits),
        "multi_zim_hits": len(direct_hits),
        "zim_source_breakdown": zim_source_breakdown,
        "integrated_kiwix_zim_hits": len(integrated_hits),
        "sample_direct_hits": direct_hits[:5],
        "sample_integrated_hits": integrated_hits[:5],
        "ok": bool(direct_hits),
    }

    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
