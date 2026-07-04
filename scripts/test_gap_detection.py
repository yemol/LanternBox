import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from api.analysis.gap_detector import analyze_coverage_gap, detect_domain_coverage


FORBIDDEN_TERMS = [
    "download plan",
    "下载计划",
    "自动下载",
    "新增 zim 文件",
    "新增外部知识源",
    "runtime resource discovery",
]


def main():
    coverage = detect_domain_coverage()
    gap = analyze_coverage_gap()
    serialized_recommendations = json.dumps(gap["recommendations"], ensure_ascii=False).lower()
    forbidden_hits = [
        term
        for term in FORBIDDEN_TERMS
        if term.lower() in serialized_recommendations
    ]

    low_coverage_list = [
        {
            "domain": domain,
            **item,
        }
        for domain, item in coverage.items()
        if item["gap_level"] in {"missing", "high", "medium"}
    ]

    report = {
        "missing domains": gap["missing_domains"],
        "low coverage list": low_coverage_list,
        "static recommendations only": not forbidden_hits,
        "forbidden_terms_found": forbidden_hits,
        "recommendations": gap["recommendations"],
    }

    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
