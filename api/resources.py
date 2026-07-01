"""本地资源协调层。

Retrieval v2 迁移后，本文件只负责加载本地基础资料缓存，以及把
v2 选中的指南整理成 Prompt 可读文本。
"""

from typing import Any, Dict, List

from .config import (
    DATA_DIR,
    GUIDES_CACHE,
    RESOURCE_CACHE_INFO,
    TRIGGERS_CACHE,
    CONTEXT_PROFILES_CACHE,
    GUIDE_TAXONOMY_CACHE,
)
from .utils import read_json_file


def load_local_resources() -> None:
    """Load local guide / trigger data into memory.

    旧 context_profiles / guide_taxonomy 已不再参与 AI 主检索。
    这里清空对应缓存，避免旧规则数据被误用。
    """
    guides_path = DATA_DIR / "emergency_guides.json"
    triggers_path = DATA_DIR / "ai_triggers.json"

    GUIDES_CACHE.clear()
    GUIDES_CACHE.extend(read_json_file(guides_path, []))

    TRIGGERS_CACHE.clear()
    TRIGGERS_CACHE.extend(read_json_file(triggers_path, []))

    CONTEXT_PROFILES_CACHE.clear()
    GUIDE_TAXONOMY_CACHE.clear()

    RESOURCE_CACHE_INFO.update({
        "emergency_guides_count": len(GUIDES_CACHE),
        "ai_triggers_count": len(TRIGGERS_CACHE),
        "loaded": True,
        "guides_path": str(guides_path),
        "triggers_path": str(triggers_path),
        "context_profiles_count": 0,
        "retrieval_engine": "retrieval_v2_ai_orchestrated",
    })


def build_local_context(
    matched_triggers: List[Dict[str, Any]],
    related_guides: List[Dict[str, Any]],
) -> Dict[str, str]:
    """Build prompt context from selected local guides.

    matched_triggers 目前由 v2 主流程置空保留兼容；真正的证据来自
    related_guides / related_wikis。
    """
    trigger_blocks = []

    for trigger in matched_triggers[:3]:
        suggested_actions = trigger.get("suggested_actions", [])
        follow_up = trigger.get("follow_up", [])

        trigger_blocks.append(
            "\n".join([
                f"触发场景：{trigger.get('title', '')}",
                f"严重等级：{trigger.get('severity', '')}",
                f"AI提示：{trigger.get('ai_prompt', '')}",
                f"建议动作：{'；'.join(suggested_actions[:10])}",
                f"后续追踪：{'；'.join(follow_up[:3])}",
            ])
        )

    guide_blocks = []
    for guide in related_guides[:10]:
        steps = guide.get("steps", [])
        stop_or_escalate = guide.get("stop_or_escalate", [])
        retrieval_reason = (guide.get("_retrieval_v2") or {}).get("reason", "")

        guide_blocks.append(
            "\n".join([
                f"指南标题：{guide.get('title', '')}",
                f"分类：{guide.get('category', '')}",
                f"适用场景：{guide.get('scenario', '')}",
                f"目标：{guide.get('goal', '')}",
                f"选择理由：{retrieval_reason}",
                f"关键步骤：{'；'.join(steps[:10])}",
                f"停止或升级：{'；'.join(stop_or_escalate[:3])}",
            ])
        )

    return {
        "trigger_text": "\n\n".join(trigger_blocks),
        "guide_text": "\n\n".join(guide_blocks),
    }


def prepare_ai_context(user_message: str, mode: str) -> Dict[str, Any]:
    """Deprecated compatibility wrapper.

    AI 主流程已经迁移到 api.pipeline.preload.prepare_ai_pipeline_context。
    保留此函数只为旧调用点不崩溃，不再执行旧检索。
    """
    from .pipeline.preload import prepare_ai_pipeline_context

    return prepare_ai_pipeline_context(user_message=user_message, mode=mode)
