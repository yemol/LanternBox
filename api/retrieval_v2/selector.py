"""AI Evidence Selector."""

import json
import re
from typing import Any, Dict, List

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

原则：
1. 优先选择能直接解决当前问题的本地应急指南 guide。
2. Wiki 用于补充背景、判断标准、知识说明。
3. 不要选择和当前问题只有弱关联的资料。
4. 只能选择候选列表中存在的 id。
5. 必须输出严格合法 JSON。
6. 所有字符串必须使用双引号。
7. 数组和对象之间必须有逗号。
8. 不要使用 Markdown 代码块。
9. 不要输出注释。
10. 如果 candidates 非空，通常选择 1-4 条资料。
11. 至少选择 1 条最核心 guide。
12. 如果存在相关 wiki，可选择 1-2 条作为背景补充。
13. 不要为了凑数量选择弱相关资料。
14. guide 负责行动步骤，wiki 负责背景知识和判断标准。
15. 如果 wiki 与用户核心对象不匹配，可以不选 wiki。
16. selected 和 excluded 中每一项都必须包含 source_type、id、reason 三个字段。
17. source_type 必须是候选资料中的 source_type，例如 guide 或 wiki。
18. 不要输出 title 字段。
19. selected 不超过 4 条。
20. excluded 默认输出空数组。

候选排序规则：
1. candidates 已经按本地相关性排序，越靠前通常越相关。
2. 如果前 3 条候选中存在与用户核心场景直接匹配的 guide，必须优先选择。
3. 不要跳过明显匹配用户主问题的 guide，去选择只处理后续阶段或次要问题的资料。
4. 对灾害进行中或风险正在上升的场景，优先选择“事前 / 当前应对”资料，而不是“灾后 / 事后处理”资料。
5. guide 的优先级通常高于 wiki；wiki 只用于补充背景、判断标准和解释。

不要选择与用户核心对象明显不同的资料。例如电气问题不要选择药品、食物、水桶、文件备份类资料；缺水问题不要选择库存流程、食物配给、药品耗材类资料；暴雨洪水问题不要选择普通天气记录或无关设备维护资料。
如果 guide 候选中存在与用户核心场景直接匹配的资料，应优先选择 guide。不要因为 wiki 文字更像解释，就忽略直接行动指南。
selected 中必须包含至少 1 条最直接的 guide。如果候选中存在明显直接匹配的 guide，不得只选择间接资料。

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

    guide_ids = re.findall(r"DG-\d{4}", text or "")
    wiki_ids = re.findall(r"\b[a-z0-9]{12,20}\b", text or "")

    for item_id in guide_ids + wiki_ids:
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

        if len(selected) >= 4:
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
    return {
        "source_type": candidate.source_type,
        "id": candidate.id,
        "title": candidate.title,
        "summary": candidate.summary[:260],
        "category": candidate.category,
        "tags": candidate.tags[:8],
        "snippet": candidate.snippet[:260],
    }


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
        "retrieval_plan": plan,
        "candidates": [_candidate_for_prompt(item) for item in candidates[:16]],
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
                    for item in candidates[:3]
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
