"""AI Evidence Selector."""

import json
from typing import Any, Dict, List

from .policy import load_retrieval_policy, policy_int
from .schemas import EvidenceCandidate, EvidenceSelection, SelectedEvidence
from ..config import OLLAMA_MODEL, load_runtime_settings
from ..llm.client import call_ollama


SELECTOR_SYSTEM_PROMPT = """你是 LanternBox 的资料选择器。

你不是回答生成器。
你只能做资料选择。
禁止输出自然语言说明。
禁止输出 Markdown。
禁止输出标题、段落、列表。
你的完整输出必须是一个 JSON 对象。

你的任务：
根据用户问题、场景计划和候选资料，选择最适合回答问题的资料。

选择策略由用户消息中的 retrieval_policy 决定。
不要覆盖、改写或忽略 retrieval_policy。
只能选择候选列表中存在的 id。
不要选择和当前问题只有弱关联的资料。
selected 和 excluded 中每一项都必须包含 source_type、id、reason 三个字段。
source_type 必须是候选资料中的 source_type。
不要输出 title 字段。
如果 candidates 非空，通常至少选择 1 条最相关资料。
不要为了凑数量选择弱相关资料。
excluded 默认输出空数组。
必须输出严格合法 JSON。
所有字符串必须使用双引号。
数组和对象之间必须有逗号。
不要使用 Markdown 代码块。
不要输出注释。

输出 JSON：
{
  "selected": [
    {
      "source_type": "guide",
      "id": "候选资料 id",
      "reason": "简短理由"
    }
  ],
  "excluded": [],
  "answer_focus": ["最终回答应关注的重点"]
}
"""


def _recover_selection_from_text(
    text: str,
    candidates: List[EvidenceCandidate],
) -> Dict[str, Any]:
    candidate_by_id = {item.id: item for item in candidates}
    selected = []
    seen = set()

    ids_in_text = [
        item.id
        for item in candidates
        if item.id and item.id in (text or "")
    ]

    for item_id in ids_in_text:
        if item_id in seen:
            continue

        candidate = candidate_by_id.get(item_id)
        if not candidate:
            continue

        selected.append(
            {
                "source_type": candidate.source_type,
                "id": candidate.id,
                "reason": "从AI文本中恢复选择",
            }
        )
        seen.add(item_id)

        if len(selected) >= policy_int(("retrieval", "selector_max_selected"), 4):
            break

    return {
        "selected": selected,
        "excluded": [],
        "answer_focus": ["优先参考AI明确提到的本地资料"],
    }


def _extract_json(text: str) -> Dict[str, Any]:
    text = (text or "").strip()

    if text.startswith("```"):
        text = text.strip("`")
        if text.lower().startswith("json"):
            text = text[4:].strip()

    start = text.find("{")
    end = text.rfind("}")

    if start >= 0 and end >= start:
        text = text[start:end + 1]

    return json.loads(text)


def _normalize_selection_payload(data: Dict[str, Any], candidates: List[EvidenceCandidate]) -> Dict[str, Any]:
    candidate_type_by_id = {item.id: item.source_type for item in candidates}

    for field in ("selected", "excluded"):
        items = data.get(field) or []
        normalized = []

        for item in items:
            if not isinstance(item, dict):
                continue

            item_id = str(item.get("id") or "").strip()
            if not item_id:
                continue

            source_type = item.get("source_type") or candidate_type_by_id.get(item_id)
            if source_type == "zim":
                source_type = "kiwix"
            if not source_type:
                continue

            normalized.append(
                {
                    "source_type": source_type,
                    "id": item_id,
                    "reason": str(item.get("reason") or "").strip(),
                }
            )

        data[field] = normalized

    if not isinstance(data.get("answer_focus"), list):
        data["answer_focus"] = []

    return data


def _candidate_for_prompt(candidate: EvidenceCandidate) -> Dict[str, Any]:
    raw = candidate.raw if isinstance(candidate.raw, dict) else {}
    payload = {
        "source_type": candidate.source_type,
        "id": candidate.id,
        "title": candidate.title,
        "summary": candidate.summary[:260],
        "category": candidate.category,
        "tags": candidate.tags[:8],
        "snippet": candidate.snippet[:260],
    }
    if raw.get("retrieval_query_profiles"):
        payload.update({
            "retrieval_query_profiles": raw.get("retrieval_query_profiles"),
            "retrieval_profile_target_match": raw.get("retrieval_profile_target_match", False),
            "retrieval_profile_adjustment": raw.get("retrieval_profile_adjustment", 0),
            "retrieval_profile_reasons": raw.get("retrieval_profile_reasons", []),
            "retrieval_profile_fit_tier": raw.get("retrieval_profile_fit_tier", 0),
        })
    if candidate.source_type == "kiwix":
        payload.update({
            "usage_policy": raw.get("usage_policy", ""),
            "source_role": raw.get("source_role") or raw.get("role") or "",
            "language": raw.get("language", ""),
            "zim_filename": raw.get("zim_filename", ""),
            "relevance_score": raw.get("relevance_score", 0.0),
        })
    return payload


def select_evidence_with_ai(
    *,
    user_message: str,
    plan: Dict[str, Any],
    candidates: List[EvidenceCandidate],
) -> EvidenceSelection:
    settings = load_runtime_settings()
    model = settings.get("retrieval_v2_model") or OLLAMA_MODEL

    payload = {
        "user_message": user_message,
        "retrieval_policy": load_retrieval_policy(),
        "retrieval_plan": plan,
        "candidates": [
            _candidate_for_prompt(item)
            for item in candidates[:policy_int(("retrieval", "selector_candidate_limit"), 16)]
        ],
    }

    messages = [
        {"role": "system", "content": SELECTOR_SYSTEM_PROMPT},
        {"role": "user", "content": json.dumps(payload, ensure_ascii=False, indent=2)},
    ]

    content = call_ollama(
        messages=messages,
        model=model,
        force_json=True,
        temperature=0.1,
        num_predict=1800,
        timeout=(5, 120),
    )

    try:
        data = _extract_json(content)
    except Exception:
        print("\n" + "=" * 80)
        print("Selector raw AI output:")
        print(content)
        print("=" * 80 + "\n")

        data = _recover_selection_from_text(content, candidates)

        if not data["selected"]:
            data = {
                "selected": [
                    {
                        "source_type": item.source_type,
                        "id": item.id,
                        "reason": "JSON解析失败，保留高分候选",
                    }
                    for item in candidates[:policy_int(("retrieval", "selector_fallback_count"), 3)]
                ],
                "excluded": [],
                "answer_focus": ["优先参考高相关本地资料"],
            }

    data = _normalize_selection_payload(data, candidates)
    selection = EvidenceSelection.model_validate(data)

    if candidates and not selection.selected:
        first = candidates[0]
        selection.selected.append(
            SelectedEvidence(
                source_type=first.source_type,
                id=first.id,
                reason="AI 未返回选中资料，系统保留最高候选作为兜底。",
            )
        )

    return selection
