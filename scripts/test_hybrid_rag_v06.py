"""
LanternBox v0.6 Hybrid RAG 骨架测试。

推荐放置位置：项目根目录/scripts/test_hybrid_rag_v06.py
推荐运行方式：
    python scripts/test_hybrid_rag_v06.py

配套数据建议放在 scripts/ 目录：
    scripts/ai_retrieval_test_cases_v02.json
    scripts/ai_rerank_test_cases_v01.json

测试目标：
1. 现有 40 条来源召回测试继续通过。
2. prepare_ai_context 返回 v0.6 新字段：candidate_sources / selected_sources / excluded_sources / retrieval_decision。
3. AI 重排默认关闭时，规则回退路径可用。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent

# 脚本位于 scripts/ 下时，Python 默认不会把项目根目录加入导入路径。
# 这里显式加入项目根目录，保证可以导入 api.resources / api.ai。
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from api.resources import prepare_ai_context  # noqa: E402
from api.ai import filter_and_rank_ai_references, rerank_candidates_with_local_ai  # noqa: E402


def _resolve_case_file(filename: str) -> Path:
    """优先从 scripts/ 读取测试数据，兼容旧版根目录放置方式。"""
    candidates = [
        SCRIPT_DIR / filename,
        PROJECT_ROOT / filename,
    ]
    for path in candidates:
        if path.exists():
            return path
    raise FileNotFoundError(
        f"找不到测试数据文件：{filename}\n"
        f"请放到 scripts/ 目录，或临时放到项目根目录。已检查：\n"
        + "\n".join(str(path) for path in candidates)
    )


RETRIEVAL_CASES = _resolve_case_file("ai_retrieval_test_cases_v02.json")
RERANK_CASES = _resolve_case_file("ai_rerank_test_cases_v01.json")


def _load_json(path: Path) -> list[dict[str, Any]]:
    return json.loads(path.read_text(encoding="utf-8"))


def _titles(items: list[dict[str, Any]] | None) -> list[str]:
    return [item.get("title", "") for item in items or []]


def _text(items: list[dict[str, Any]] | None) -> str:
    return " ".join(_titles(items))


def run_retrieval_cases() -> tuple[int, list[dict[str, Any]]]:
    cases = _load_json(RETRIEVAL_CASES)
    failures: list[dict[str, Any]] = []

    for case in cases:
        ctx = prepare_ai_context(case["query"], "emergency")
        titles_text = _text(ctx.get("related_guides", []))
        include_ok = any(key in titles_text for key in case.get("should_include_any", []))
        exclude_ok = not any(key in titles_text for key in case.get("should_not_include_any", []))

        if not include_ok or not exclude_ok:
            failures.append({
                "id": case["id"],
                "stage": "prepare_ai_context",
                "include_ok": include_ok,
                "exclude_ok": exclude_ok,
                "titles": _titles(ctx.get("related_guides", [])),
            })

        for required_key in ["candidate_sources", "selected_sources", "excluded_sources", "retrieval_decision"]:
            if required_key not in ctx:
                failures.append({
                    "id": case["id"],
                    "stage": "prepare_ai_context",
                    "missing_key": required_key,
                })

        filtered = filter_and_rank_ai_references(
            case["query"],
            ctx.get("related_guides", []),
            [],
            ctx.get("detected_domains", []),
        )
        filtered_text = _text(filtered.get("guides", []))
        filtered_include_ok = any(key in filtered_text for key in case.get("should_include_any", []))
        filtered_exclude_ok = not any(key in filtered_text for key in case.get("should_not_include_any", []))

        if not filtered_include_ok or not filtered_exclude_ok:
            failures.append({
                "id": case["id"],
                "stage": "filter_and_rank_ai_references",
                "include_ok": filtered_include_ok,
                "exclude_ok": filtered_exclude_ok,
                "titles": _titles(filtered.get("guides", [])),
            })

    return len(cases), failures


def run_rerank_cases() -> tuple[int, list[dict[str, Any]]]:
    cases = _load_json(RERANK_CASES)
    failures: list[dict[str, Any]] = []

    for case in cases:
        ctx = prepare_ai_context(case["query"], "emergency")
        result = rerank_candidates_with_local_ai(
            case["query"],
            ctx.get("candidate_sources", []),
            query_profile=ctx.get("query_profile", {}),
            enable_ai_rerank=False,
        )
        selected_text = _text(result.get("selected_sources", []))
        excluded_text = " ".join(str(item.get("title", "")) for item in result.get("excluded_sources", []))

        include_ok = any(key in selected_text for key in case.get("should_select_any", []))
        hard_exclude_ok = not any(key in selected_text for key in case.get("should_exclude_any", []))

        if not include_ok or not hard_exclude_ok:
            failures.append({
                "id": case["id"],
                "stage": "rerank_candidates_with_local_ai",
                "include_ok": include_ok,
                "hard_exclude_ok": hard_exclude_ok,
                "selected": _titles(result.get("selected_sources", [])),
                "excluded": excluded_text,
                "mode": result.get("mode"),
            })

    return len(cases), failures


def main() -> None:
    print(f"项目根目录：{PROJECT_ROOT}")
    print(f"来源召回测试数据：{RETRIEVAL_CASES}")
    print(f"AI 重排测试数据：{RERANK_CASES}")

    total, failures = run_retrieval_cases()
    print(f"来源召回测试：{total - len(failures)}/{total} 通过")
    if failures:
        print(json.dumps(failures[:5], ensure_ascii=False, indent=2))
        raise SystemExit(1)

    total, failures = run_rerank_cases()
    print(f"AI 重排骨架测试：{total - len(failures)}/{total} 通过")
    if failures:
        print(json.dumps(failures[:5], ensure_ascii=False, indent=2))
        raise SystemExit(1)

    print("v0.6 Hybrid RAG 骨架测试通过。")


if __name__ == "__main__":
    main()
