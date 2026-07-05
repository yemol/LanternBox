import json
import os
import re
import sys
import tempfile
from contextlib import contextmanager
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv" / "bin" / "python"

if not os.environ.get("LANTERNBOX_KIWIX_OBSERVE_VENV") and VENV_PYTHON.exists():
    os.environ["LANTERNBOX_KIWIX_OBSERVE_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])

sys.path.insert(0, str(ROOT))

from api.kiwix.orchestrator import run_kiwix_query
from api.kiwix.zim_client import (
    DEFAULT_ZIM_DIR,
    classify_query_domain,
    planned_zim_sources,
    zim_sources,
)


TEST_QUERIES = [
    "饮用水有异味还能喝吗",
    "伤口红肿发热怎么办",
    "锂电池鼓包还能用吗",
    "插线板进水怎么办",
    "土壤长期潮湿怎么处理",
    "短波电台天线基础",
    "Arduino 传感器怎么接线",
    "自行车刹车失灵怎么检查",
    "木板受潮发霉怎么处理",
    "洪水后如何判断地形风险",
    "食物发霉还能吃吗",
    "漂白剂能不能用于饮水消毒",
]

BAD_TITLE_MARKERS = [
    "案件", "法院", "判决", "电影", "电视剧", "香水", "体育", "足球", "篮球", "棒球",
    "赛季", "球员", "演员", "歌手", "名人", "专辑", "歌曲",
    "court", "case", "supreme", "film", "movie", "perfume", "fragrance", "sport",
    "football", "basketball", "baseball", "player", "actor", "actress", "singer",
]


@contextmanager
def suppress_known_libzim_warnings():
    """Hide known libzim zh stemming warnings while preserving real output."""
    original_stdout = os.dup(1)
    original_stderr = os.dup(2)
    with tempfile.TemporaryFile(mode="w+t", encoding="utf-8") as captured_stdout, tempfile.TemporaryFile(mode="w+t", encoding="utf-8") as captured_stderr:
        os.dup2(captured_stdout.fileno(), 1)
        os.dup2(captured_stderr.fileno(), 2)
        try:
            yield
        finally:
            os.dup2(original_stdout, 1)
            os.dup2(original_stderr, 2)
            os.close(original_stdout)
            os.close(original_stderr)
            captured_stdout.seek(0)
            for line in captured_stdout:
                if "No stemming for language 'zh'" in line:
                    continue
                sys.stdout.write(line)
            captured_stderr.seek(0)
            for line in captured_stderr:
                if "No stemming for language 'zh'" in line:
                    continue
                sys.stderr.write(line)


def result_to_observe_dict(item):
    zim_filename = item.zim_filename or ""
    zim_source = Path(zim_filename).stem if zim_filename else item.source
    return {
        "title": item.title,
        "zim_source": zim_source,
        "zim_filename": zim_filename or None,
        "language": item.language,
        "role": item.role,
        "usage_policy": item.usage_policy,
        "matched_terms": item.matched_terms,
        "score": item.relevance_score,
    }


def title_looks_bad(title: str) -> bool:
    lowered = str(title or "").lower()
    if re.search(r"[^预]案$", str(title or "")):
        return True
    return any(marker.lower() in lowered for marker in BAD_TITLE_MARKERS)


def main():
    zim_files = sorted(str(path) for path in DEFAULT_ZIM_DIR.glob("*.zim")) if DEFAULT_ZIM_DIR.exists() else []
    query_reports = []
    bad_results = []
    total_hits = 0

    for query in TEST_QUERIES:
        classified_domains = classify_query_domain(query)
        searched_sources = planned_zim_sources(query, channel="ai")

        with suppress_known_libzim_warnings():
            integrated = run_kiwix_query(query, {"core_terms": query.split()})

        top_results = [
            result_to_observe_dict(item)
            for item in integrated
            if item.source == "kiwix_zim"
        ]
        total_hits += len(top_results)

        for result in top_results:
            if title_looks_bad(result.get("title", "")):
                bad_results.append({"query": query, "title": result.get("title")})

        query_reports.append(
            {
                "query": query,
                "classified_domains": classified_domains,
                "searched_zim_sources": searched_sources,
                "top_results": top_results,
            }
        )

    report = {
        "zim_files": zim_files,
        "zim_sources": {source: str(path) for source, path in zim_sources.items()},
        "query_count": len(TEST_QUERIES),
        "integrated_kiwix_zim_hits": total_hits,
        "queries": query_reports,
        "bad_results": bad_results,
        "ok": bool(zim_files) and total_hits > 0 and not bad_results and all(item["searched_zim_sources"] for item in query_reports),
        "status": "PASS" if bool(zim_files) and total_hits > 0 and not bad_results and all(item["searched_zim_sources"] for item in query_reports) else "FAIL",
    }

    print(json.dumps(report, ensure_ascii=False, indent=2))
    if not report["ok"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
