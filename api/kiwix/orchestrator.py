"""Unified entry point for the Kiwix stub layer."""

import json
import sys
from pathlib import Path
from typing import Dict, List

if __package__ in (None, ""):
    sys.path.append(str(Path(__file__).resolve().parents[2]))

from api.kiwix.client import KiwixClient
from api.kiwix.fetcher import fetch_mock_kiwix_results
from api.kiwix.schema import KiwixResult


def run_kiwix_query(query: str, context: Dict) -> List[KiwixResult]:
    """Run a stable local-only Kiwix query.

    The real Kiwix client boundary is present but intentionally unused for now:
    this version does not read ZIM files and does not call external services.
    """
    client = KiwixClient()
    zim_results = client.search(query, context)
    if zim_results:
        return zim_results

    return fetch_mock_kiwix_results(query=query, context=context)


if __name__ == "__main__":
    sample_results = run_kiwix_query(
        "无线电通信和天线怎么查离线资料",
        {"core_terms": ["无线电", "通信", "天线"]},
    )
    print(json.dumps([item.to_dict() for item in sample_results], ensure_ascii=False, indent=2))
