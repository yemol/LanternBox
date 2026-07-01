"""Response context blocks.

Retrieval v2 migration removed the old rule-first Context Engine from the AI main chain.
This module remains as a compatibility shim for any older imports.
"""

from typing import Any, Dict


def build_lantern_context(user_message: str) -> Dict[str, Any]:
    return {
        "input_text": user_message or "",
        "engine": "retrieval_v2_ai_orchestrated",
    }


def format_lantern_context_for_prompt(context: Dict[str, Any]) -> str:
    return ""
