"""Unified entry point for the Kiwix stub layer."""

import json
import sys
from pathlib import Path
from typing import Dict, List

if __package__ in (None, ""):
    sys.path.append(str(Path(__file__).resolve().parents[2]))

from api.kiwix.fetcher import query_for_ai, query_for_lookup
from api.kiwix.schema import KiwixResult


def run_kiwix_query(query: str, context: Dict) -> List[KiwixResult]:
    """Compatibility wrapper for the AI-safe Kiwix decision channel."""
    return query_for_ai(query=query, context=context)


def run_kiwix_lookup(query: str, context: Dict) -> List[KiwixResult]:
    """Human lookup wrapper that can surface lookup/support ZIM sources."""
    return query_for_lookup(query=query, context=context)


if __name__ == "__main__":
    sample_results = run_kiwix_query(
        "无线电通信和天线怎么查离线资料",
        {"core_terms": ["无线电", "通信", "天线"]},
    )
    print(json.dumps([item.to_dict() for item in sample_results], ensure_ascii=False, indent=2))
