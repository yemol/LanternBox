#!/usr/bin/env python3
"""Read-only trace runner for Batch17-N retrieval variance analysis."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VENV_PYTHON = ROOT / "venv/bin/python"
if (
    __name__ == "__main__"
    and not os.environ.get("LANTERNBOX_BATCH17_N_TRACE_VENV")
    and VENV_PYTHON.exists()
):
    os.environ["LANTERNBOX_BATCH17_N_TRACE_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.config import load_runtime_settings  # noqa: E402
from api.retrieval_v2 import orchestrator, planner, selector  # noqa: E402
from api.retrieval_v2.contract_runtime import normalize_contract_query  # noqa: E402
from scripts import test_resource_allocation_field as field_runner  # noqa: E402

TARGET_IDS = {"R02", "R05", "R07"}


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256_json(value: Any) -> str:
    payload = json.dumps(value, ensure_ascii=False, sort_keys=True, default=str)
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def candidate_row(candidate: Any, rank: int) -> dict[str, Any]:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    diagnostics = {
        key: value
        for key, value in raw.items()
        if any(token in str(key) for token in ("score", "rank", "profile", "lane", "intent"))
    }
    return {
        "rank": rank,
        "source_type": str(candidate.source_type),
        "id": str(candidate.id),
        "title": str(candidate.title),
        "category": str(candidate.category),
        "diagnostics": diagnostics,
    }


def candidate_rows(candidates: list[Any]) -> list[dict[str, Any]]:
    return [candidate_row(item, index) for index, item in enumerate(candidates, 1)]


def candidate_keys(candidates: list[Any]) -> list[str]:
    return [f"{item.source_type}:{item.id}" for item in candidates]


def classify_call_error(exc: Exception) -> tuple[str, int | None]:
    text = str(exc)
    folded = text.casefold()
    if "500" in folded or "internal server error" in folded:
        return "http_500", 500
    if "timed out" in folded or "timeout" in folded:
        return "timeout", None
    return "exception", None


class TraceCapture:
    def __init__(self) -> None:
        self.trace: dict[str, Any] = {}
        self._originals: dict[str, Any] = {}
        self._first_fetch_args: tuple[Any, ...] | None = None
        self._first_fetch_kwargs: dict[str, Any] | None = None

    def reset(self, case_id: str, query: str, sequence_index: int) -> None:
        self.trace = {
            "case_id": case_id,
            "sequence_index": sequence_index,
            "timestamp_start": utc_now(),
            "query": query,
            "normalized_query": normalize_contract_query(query),
            "model_calls": [],
            "candidate_stages": [],
            "cache": {
                "status": "not_exposed",
                "note": "Production retrieval exposes no cache hit/miss signal.",
            },
        }
        self._first_fetch_args = None
        self._first_fetch_kwargs = None

    def install(self) -> None:
        self._originals = {
            "planner_llm": planner.call_ollama,
            "selector_llm": selector.call_ollama,
            "fetch": orchestrator.fetch_candidates_from_plan,
            "canary": orchestrator.apply_contract_canary_candidates,
            "ownership": orchestrator.apply_ownership_intent_candidates,
        }

        def planner_llm(*args: Any, **kwargs: Any) -> str:
            return self._capture_llm("planner", self._originals["planner_llm"], *args, **kwargs)

        def selector_llm(*args: Any, **kwargs: Any) -> str:
            return self._capture_llm("selector", self._originals["selector_llm"], *args, **kwargs)

        def fetch(*args: Any, **kwargs: Any) -> list[Any]:
            started = time.monotonic()
            result = self._originals["fetch"](*args, **kwargs)
            rows = candidate_rows(result)
            self.trace["candidate_stages"].append(
                {
                    "stage": "fetch",
                    "latency_ms": round((time.monotonic() - started) * 1000, 1),
                    "source_plan": [item.model_dump() for item in args[0]],
                    "core_terms": list(kwargs.get("core_terms") or []),
                    "candidates": rows,
                    "candidate_order_sha256": sha256_json(candidate_keys(result)),
                }
            )
            if self._first_fetch_args is None:
                self._first_fetch_args = args
                self._first_fetch_kwargs = dict(kwargs)
            return result

        def canary(*args: Any, **kwargs: Any) -> Any:
            incoming = args[2]
            result = self._originals["canary"](*args, **kwargs)
            self.trace["contract"] = {
                "mode": kwargs.get("mode"),
                "decision": result.decision.model_dump() if result.decision else None,
                "context": result.context.model_dump(),
                "input_candidates": candidate_keys(incoming),
                "output_candidates": candidate_keys(result.candidates),
            }
            self.trace["candidate_stages"].append(
                {
                    "stage": "contract_lane",
                    "candidates": candidate_rows(result.candidates),
                    "candidate_order_sha256": sha256_json(candidate_keys(result.candidates)),
                }
            )
            return result

        def ownership(*args: Any, **kwargs: Any) -> Any:
            incoming = args[1]
            result = self._originals["ownership"](*args, **kwargs)
            self.trace["ownership"] = {
                "decision": result.decision.model_dump() if result.decision else None,
                "context": result.context.model_dump(),
                "input_candidates": candidate_keys(incoming),
                "output_candidates": candidate_keys(result.candidates),
            }
            self.trace["candidate_stages"].append(
                {
                    "stage": "ownership_lane_selector_input",
                    "candidates": candidate_rows(result.candidates),
                    "candidate_order_sha256": sha256_json(candidate_keys(result.candidates)),
                }
            )
            return result

        planner.call_ollama = planner_llm
        selector.call_ollama = selector_llm
        orchestrator.fetch_candidates_from_plan = fetch
        orchestrator.apply_contract_canary_candidates = canary
        orchestrator.apply_ownership_intent_candidates = ownership

    def uninstall(self) -> None:
        planner.call_ollama = self._originals["planner_llm"]
        selector.call_ollama = self._originals["selector_llm"]
        orchestrator.fetch_candidates_from_plan = self._originals["fetch"]
        orchestrator.apply_contract_canary_candidates = self._originals["canary"]
        orchestrator.apply_ownership_intent_candidates = self._originals["ownership"]

    def _capture_llm(self, stage: str, target: Any, *args: Any, **kwargs: Any) -> str:
        messages = kwargs.get("messages") or (args[0] if args else [])
        user_content = next(
            (
                str(item.get("content") or "")
                for item in reversed(messages)
                if item.get("role") == "user"
            ),
            "",
        )
        call: dict[str, Any] = {
            "stage": stage,
            "timestamp_start": utc_now(),
            "model": kwargs.get("model"),
            "temperature": kwargs.get("temperature"),
            "timeout": kwargs.get("timeout"),
            "input_sha256": hashlib.sha256(user_content.encode("utf-8")).hexdigest(),
        }
        if stage == "planner":
            call["input"] = {"query": user_content}
        else:
            try:
                payload = json.loads(user_content)
                call["input"] = {
                    "query": payload.get("user_message"),
                    "retrieval_plan": payload.get("retrieval_plan"),
                    "candidates": payload.get("candidates"),
                }
            except Exception:
                call["input"] = {"unparsed_sha256": call["input_sha256"]}
        started = time.monotonic()
        try:
            output = target(*args, **kwargs)
        except Exception as exc:
            status, http_status = classify_call_error(exc)
            call.update(
                {
                    "status": status,
                    "http_status": http_status,
                    "exception_type": type(exc).__name__,
                    "exception": str(exc),
                    "output": None,
                }
            )
            raise
        else:
            call.update(
                {
                    "status": "healthy",
                    "http_status": 200,
                    "exception_type": "",
                    "exception": "",
                    "output": output,
                    "output_sha256": hashlib.sha256(output.encode("utf-8")).hexdigest(),
                }
            )
            return output
        finally:
            call["latency_ms"] = round((time.monotonic() - started) * 1000, 1)
            call["timestamp_end"] = utc_now()
            self.trace["model_calls"].append(call)

    def fixed_plan_refetch(self) -> dict[str, Any]:
        if self._first_fetch_args is None or self._first_fetch_kwargs is None:
            return {"available": False, "reason": "initial fetch was not reached"}
        attempts = []
        for _ in range(3):
            started = time.monotonic()
            rows = self._originals["fetch"](*self._first_fetch_args, **self._first_fetch_kwargs)
            keys = candidate_keys(rows)
            attempts.append(
                {
                    "latency_ms": round((time.monotonic() - started) * 1000, 1),
                    "candidate_keys": keys,
                    "candidate_order_sha256": sha256_json(keys),
                }
            )
        return {
            "available": True,
            "attempts": attempts,
            "stable": len({item["candidate_order_sha256"] for item in attempts}) == 1,
        }


def execute_case(
    capture: TraceCapture,
    case: dict[str, Any],
    wiki_rows: dict[str, dict[str, str]],
    id_to_slug: dict[str, str],
    sequence_index: int,
) -> dict[str, Any]:
    capture.reset(str(case["case_id"]), str(case["query"]), sequence_index)
    started = time.monotonic()
    result = field_runner.run_case(case, wiki_rows, id_to_slug)
    trace = capture.trace
    trace["timestamp_end"] = utc_now()
    trace["total_latency_ms"] = round((time.monotonic() - started) * 1000, 1)
    trace["fixed_plan_refetch"] = capture.fixed_plan_refetch()
    trace["planner_status"] = next(
        (item["status"] for item in trace["model_calls"] if item["stage"] == "planner"),
        "not_called",
    )
    trace["selector_status"] = next(
        (item["status"] for item in trace["model_calls"] if item["stage"] == "selector"),
        "not_called",
    )
    trace["model_health"] = (
        "MODEL_HEALTHY"
        if trace["planner_status"] == "healthy" and trace["selector_status"] == "healthy"
        else "MODEL_TIMEOUT_OR_500"
    )
    trace["result"] = {
        key: result[key]
        for key in (
            "verdict",
            "reason",
            "actual_top_guides",
            "actual_selected_wiki",
            "guide_candidates",
            "wiki_candidates",
            "profiles_triggered",
            "top1_owners",
            "expected_guide_hit",
            "expected_guide_candidate",
            "primary_domain_correct",
            "wiki_any_hit",
            "wiki_full_hit",
            "wiki_candidate_any",
            "guide_wiki_precise",
            "safety_gate_status",
            "production_path",
        )
    }
    return trace


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--case-id", choices=sorted(TARGET_IDS))
    group.add_argument("--full-sequence", action="store_true")
    parser.add_argument("--run-label", required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    fixture = field_runner.read_json(field_runner.CASES_PATH)
    wiki_rows = field_runner.load_wiki_rows()
    guide_ids = field_runner.load_guide_ids()
    field_runner.validate_fixture(fixture, guide_ids, set(wiki_rows))
    id_to_slug = {row["id"]: slug for slug, row in wiki_rows.items()}
    cases = fixture["cases"]
    if args.case_id:
        cases = [case for case in cases if case["case_id"] == args.case_id]

    capture = TraceCapture()
    capture.install()
    traces = []
    sequence_summary = []
    try:
        for index, case in enumerate(cases, 1):
            trace = execute_case(capture, case, wiki_rows, id_to_slug, index)
            sequence_summary.append(
                {
                    "sequence_index": index,
                    "case_id": case["case_id"],
                    "verdict": trace["result"]["verdict"],
                    "model_health": trace["model_health"],
                    "planner_status": trace["planner_status"],
                    "selector_status": trace["selector_status"],
                    "latency_ms": trace["total_latency_ms"],
                }
            )
            if case["case_id"] in TARGET_IDS:
                traces.append(trace)
    finally:
        capture.uninstall()

    payload = {
        "schema_version": "batch17-n-trace-v1",
        "run_label": args.run_label,
        "execution_mode": "full_sequence" if args.full_sequence else "isolated_fresh_process",
        "runtime_settings": load_runtime_settings(),
        "timestamp": utc_now(),
        "sequence_summary": sequence_summary,
        "traces": traces,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(json.dumps(sequence_summary, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
