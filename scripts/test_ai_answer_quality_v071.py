#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LanternBox v0.7 AI 最终回答质量测试

用途：
- 调用 /api/ai/advice 获取完整回答和来源。
- 检查回答是否包含必要方向、是否出现现代外部依赖、是否误引用排除来源。
- 支持测试 AI 重排开启 / 关闭后的最终回答质量。

建议放置：scripts/test_ai_answer_quality_v070.py
测试数据：scripts/ai_answer_test_cases_v070.json

运行：
  python scripts/test_ai_answer_quality_v070.py
  python scripts/test_ai_answer_quality_v070.py --enable-rerank
  python scripts/test_ai_answer_quality_v070.py --case AQ-002 --enable-rerank
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Tuple

try:
    import requests
except ImportError:
    print("缺少 requests，请先执行：pip install requests")
    raise

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent

DEFAULT_CASES_PATHS = [
    SCRIPT_DIR / "ai_answer_test_cases_v070.json",
    PROJECT_ROOT / "ai_answer_test_cases_v070.json",
]

MODERN_DEPENDENCY_FORBIDDEN = [
    "联系物业",
    "联系供水公司",
    "联系供电公司",
    "联系电力公司",
    "拨打供电",
    "拨打供水",
    "叫外卖",
    "点外卖",
    "快递",
    "网上搜索",
    "上网查询",
]


def load_cases(path: str | None) -> List[Dict[str, Any]]:
    if path:
        case_path = Path(path)
    else:
        case_path = next((p for p in DEFAULT_CASES_PATHS if p.exists()), None)
        if not case_path:
            raise FileNotFoundError("找不到 ai_answer_test_cases_v070.json，请放到 scripts/ 目录。")

    with case_path.open("r", encoding="utf-8") as f:
        cases = json.load(f)
    if not isinstance(cases, list):
        raise ValueError("测试用例文件必须是 JSON 数组。")
    print(f"读取测试用例：{case_path}")
    return cases


def post_json(base_url: str, path: str, payload: Dict[str, Any], timeout: int = 180) -> Dict[str, Any]:
    url = base_url.rstrip("/") + path
    response = requests.post(url, json=payload, timeout=timeout)
    if not response.ok:
        raise RuntimeError(f"POST {path} 失败：HTTP {response.status_code} {response.text[:500]}")
    return response.json()


def get_json(base_url: str, path: str, timeout: int = 30) -> Dict[str, Any]:
    url = base_url.rstrip("/") + path
    response = requests.get(url, timeout=timeout)
    if not response.ok:
        raise RuntimeError(f"GET {path} 失败：HTTP {response.status_code} {response.text[:500]}")
    return response.json()


def set_rerank(base_url: str, enabled: bool) -> Dict[str, Any] | None:
    try:
        return post_json(
            base_url,
            "/api/settings/ai",
            {
                "ai_rerank_enabled": enabled,
                "retrieval_mode": "hybrid" if enabled else "rule",
            },
            timeout=30,
        )
    except Exception as exc:
        print(f"警告：无法切换 AI 重排开关：{exc}")
        return None


def flatten_active_reference_titles(data: Dict[str, Any]) -> str:
    """只统计真正展示/参与回答的来源。

    注意：excluded_sources 是“被排除来源”，里面出现禁用词反而可能是正确的，
    因此不能把 excluded_sources 拼进禁用来源检查。
    """
    guides = data.get("related_guides") or []
    wikis = data.get("related_wikis") or []
    selected = data.get("selected_sources") or []

    chunks: List[str] = []
    for group in (guides, wikis, selected):
        if not isinstance(group, list):
            continue
        for item in group:
            if not isinstance(item, dict):
                continue
            chunks.extend(
                str(item.get(k) or "")
                for k in ["title", "source_title", "candidate_id", "source_id", "reason"]
            )
    return "\n".join(chunks)


def flatten_excluded_reference_titles(data: Dict[str, Any]) -> str:
    excluded = data.get("excluded_sources") or []
    chunks: List[str] = []
    if isinstance(excluded, list):
        for item in excluded:
            if isinstance(item, dict):
                chunks.extend(str(item.get(k) or "") for k in ["title", "candidate_id", "reason"])
    return "\n".join(chunks)


def get_rerank_debug(data: Dict[str, Any]) -> Dict[str, Any]:
    decision = data.get("retrieval_decision") or {}
    rerank = decision.get("rerank_result") or {}
    return {
        "ai_rerank_enabled": (data.get("runtime_settings") or {}).get("ai_rerank_enabled"),
        "retrieval_mode": (data.get("runtime_settings") or {}).get("retrieval_mode"),
        "rerank_mode": rerank.get("mode") or decision.get("mode"),
        "used_ai": rerank.get("used_ai") if "used_ai" in rerank else decision.get("used_ai"),
        "fallback_reason": rerank.get("fallback_reason") or decision.get("fallback_reason"),
    }

def contains_any(text: str, words: List[str]) -> bool:
    return any(word and word in text for word in words)


def collect_hits(text: str, words: List[str]) -> List[str]:
    return [word for word in words if word and word in text]


def evaluate_case(case: Dict[str, Any], data: Dict[str, Any]) -> Tuple[bool, List[str]]:
    problems: List[str] = []
    answer = str(data.get("answer") or "")
    refs_text = flatten_active_reference_titles(data)
    excluded_refs_text = flatten_excluded_reference_titles(data)
    whole_text = answer + "\n" + refs_text

    expected_answer_any = case.get("expected_answer_include_any") or []
    if expected_answer_any and not contains_any(answer, expected_answer_any):
        problems.append(f"回答未命中期望方向之一：{expected_answer_any}")

    expected_ref_any = case.get("expected_reference_include_any") or []
    if expected_ref_any and not contains_any(refs_text, expected_ref_any):
        problems.append(f"来源未命中期望方向之一：{expected_ref_any}")

    forbidden_answer = list(MODERN_DEPENDENCY_FORBIDDEN)
    forbidden_answer.extend(case.get("forbidden_answer_any") or [])
    hits = collect_hits(answer, forbidden_answer)
    if hits:
        problems.append(f"回答出现禁用/现代外部依赖词：{hits}")

    forbidden_refs = case.get("forbidden_reference_any") or []
    ref_hits = collect_hits(refs_text, forbidden_refs)
    if ref_hits:
        problems.append(f"来源出现禁用来源词：{ref_hits}")

    expected_excluded_any = case.get("expected_excluded_include_any") or []
    if expected_excluded_any and not contains_any(excluded_refs_text, expected_excluded_any):
        problems.append(f"排除来源未命中期望方向之一：{expected_excluded_any}")

    if case.get("mode") == "emergency":
        # 简单结构检查：不强制固定标题，但至少应该有行动性表达。
        action_markers = ["先", "不要", "接下来", "记录", "复查", "安排", "检查"]
        if not contains_any(answer, action_markers):
            problems.append("应急回答缺少明显行动性表达。")

    return (not problems), problems


def run_case(base_url: str, case: Dict[str, Any], model: str | None = None) -> Tuple[bool, Dict[str, Any], List[str], float]:
    payload: Dict[str, Any] = {
        "message": case["query"],
        "mode": case.get("mode", "emergency"),
        "metadata_only": False,
        "history": [],
    }
    if model:
        payload["model"] = model

    start = time.time()
    data = post_json(base_url, "/api/ai/advice", payload, timeout=240)
    elapsed = time.time() - start
    ok, problems = evaluate_case(case, data)
    return ok, data, problems, elapsed


def main() -> int:
    parser = argparse.ArgumentParser(description="LanternBox v0.7 AI 最终回答质量测试")
    parser.add_argument("--base-url", default="http://127.0.0.1:8787", help="FastAPI 地址")
    parser.add_argument("--cases", default=None, help="测试用例 JSON 路径")
    parser.add_argument("--case", default=None, help="只运行指定用例 ID，例如 AQ-002")
    parser.add_argument("--enable-rerank", action="store_true", help="测试前开启 AI 重排")
    parser.add_argument("--disable-rerank", action="store_true", help="测试前关闭 AI 重排")
    parser.add_argument("--model", default=None, help="指定回答模型，可选")
    args = parser.parse_args()

    cases = load_cases(args.cases)
    if args.case:
        cases = [c for c in cases if c.get("id") == args.case]
        if not cases:
            print(f"未找到用例：{args.case}")
            return 2

    print(f"后端地址：{args.base_url}")
    try:
        settings = get_json(args.base_url, "/api/settings/ai")
        print("当前 AI 设置：", json.dumps(settings, ensure_ascii=False))
    except Exception as exc:
        print(f"警告：读取 /api/settings/ai 失败：{exc}")

    if args.enable_rerank and args.disable_rerank:
        print("不能同时指定 --enable-rerank 和 --disable-rerank")
        return 2
    if args.enable_rerank:
        print("切换：开启 AI 重排")
        set_rerank(args.base_url, True)
    elif args.disable_rerank:
        print("切换：关闭 AI 重排")
        set_rerank(args.base_url, False)

    passed = 0
    failed = 0

    for case in cases:
        case_id = case.get("id", "UNKNOWN")
        name = case.get("name", "")
        print("\n" + "=" * 72)
        print(f"{case_id} {name}")
        print(f"Q: {case.get('query')}")

        try:
            ok, data, problems, elapsed = run_case(args.base_url, case, model=args.model)
        except Exception as exc:
            failed += 1
            print(f"结果：FAIL，请求失败：{exc}")
            continue

        answer = str(data.get("answer") or "")
        guides = [g.get("title") for g in data.get("related_guides") or [] if isinstance(g, dict)]
        debug = get_rerank_debug(data)

        if ok:
            passed += 1
            print(f"结果：OK，用时 {elapsed:.1f}s")
        else:
            failed += 1
            print(f"结果：FAIL，用时 {elapsed:.1f}s")
            for item in problems:
                print(f"  - {item}")

        print("来源：", "、".join([str(x) for x in guides[:6] if x]) or "无")
        print("检索：", json.dumps(debug, ensure_ascii=False))
        print("回答预览：", answer.replace("\n", " ")[:220])

    print("\n" + "=" * 72)
    print(f"总计：{passed} 通过 / {failed} 失败 / {passed + failed} 总数")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
