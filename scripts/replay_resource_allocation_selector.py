#!/usr/bin/env python3
"""Replay one captured selector input without rerunning planner or candidate fetch."""
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
    and not os.environ.get("LANTERNBOX_BATCH17_N_REPLAY_VENV")
    and VENV_PYTHON.exists()
):
    os.environ["LANTERNBOX_BATCH17_N_REPLAY_VENV"] = "1"
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), *sys.argv])
sys.path.insert(0, str(ROOT))

from api.llm.client import call_ollama  # noqa: E402
from api.retrieval_v2.selector import (  # noqa: E402
    SELECTOR_SYSTEM_PROMPT,
    _extract_json,
    _normalize_selection_payload,
)
from api.retrieval_v2.schemas import EvidenceCandidate  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-trace", type=Path, required=True)
    parser.add_argument("--repeat", type=int, default=5)
    parser.add_argument("--run-label", required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def classify_error(exc: Exception) -> tuple[str, int | None]:
    text = str(exc).casefold()
    if "500" in text or "internal server error" in text:
        return "http_500", 500
    if "timed out" in text or "timeout" in text:
        return "timeout", None
    return "exception", None


def main() -> int:
    args = parse_args()
    source = json.loads(args.source_trace.read_text(encoding="utf-8"))
    trace = source["traces"][0]
    captured = next(
        item for item in trace["model_calls"] if item["stage"] == "selector"
    )
    selector_input = captured["input"]
    # Rebuild the exact serialized selector user message captured by its hash source.
    candidates = [
        EvidenceCandidate(
            source_type=item["source_type"],
            id=item["id"],
            title=item.get("title", ""),
            summary=item.get("summary", ""),
            category=item.get("category", ""),
            tags=item.get("tags", []),
            snippet=item.get("snippet", ""),
            raw={
                key: value
                for key, value in item.items()
                if key.startswith("retrieval_")
            },
        )
        for item in selector_input["candidates"]
    ]
    from api.retrieval_v2.policy import load_retrieval_policy

    exact_payload = {
        "user_message": selector_input["query"],
        "retrieval_policy": load_retrieval_policy(),
        "retrieval_plan": selector_input["retrieval_plan"],
        "candidates": selector_input["candidates"],
    }
    exact_user_message = json.dumps(exact_payload, ensure_ascii=False, indent=2)
    messages = [
        {"role": "system", "content": SELECTOR_SYSTEM_PROMPT},
        {"role": "user", "content": exact_user_message},
    ]
    input_hash = hashlib.sha256(exact_user_message.encode("utf-8")).hexdigest()
    if input_hash != captured["input_sha256"]:
        raise RuntimeError("selector replay input does not match captured input")

    attempts: list[dict[str, Any]] = []
    for index in range(1, args.repeat + 1):
        started = time.monotonic()
        timestamp = datetime.now(timezone.utc).isoformat()
        try:
            content = call_ollama(
                messages=messages,
                model=captured["model"],
                force_json=True,
                temperature=captured["temperature"],
                num_predict=1800,
                timeout=tuple(captured["timeout"]),
            )
            data = _normalize_selection_payload(_extract_json(content), candidates)
        except Exception as exc:
            status, http_status = classify_error(exc)
            attempts.append(
                {
                    "attempt": index,
                    "timestamp": timestamp,
                    "input_sha256": input_hash,
                    "status": status,
                    "http_status": http_status,
                    "exception": str(exc),
                    "latency_ms": round((time.monotonic() - started) * 1000, 1),
                    "raw_output": None,
                    "selected": [],
                }
            )
        else:
            attempts.append(
                {
                    "attempt": index,
                    "timestamp": timestamp,
                    "input_sha256": input_hash,
                    "status": "healthy",
                    "http_status": 200,
                    "exception": "",
                    "latency_ms": round((time.monotonic() - started) * 1000, 1),
                    "raw_output": content,
                    "raw_output_sha256": hashlib.sha256(content.encode("utf-8")).hexdigest(),
                    "selected": [
                        f"{item['source_type']}:{item['id']}"
                        for item in data.get("selected", [])
                    ],
                }
            )

    output = {
        "schema_version": "batch17-n-selector-replay-v1",
        "run_label": args.run_label,
        "case_id": trace["case_id"],
        "source_trace": str(args.source_trace),
        "selector_input_sha256": input_hash,
        "candidate_keys": [
            f"{item['source_type']}:{item['id']}"
            for item in selector_input["candidates"]
        ],
        "attempts": attempts,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    print(
        json.dumps(
            [
                {
                    "attempt": item["attempt"],
                    "status": item["status"],
                    "selected": item["selected"],
                }
                for item in attempts
            ],
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
