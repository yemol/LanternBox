"""AI 兼容门面。只保留旧接口的轻量转发，核心流程由 pipeline 接管。"""

from .llm.client import call_ollama, stream_ollama

from .response.prompts import (
    build_safe_history,
    build_companion_messages,
    build_emergency_messages,
)
from .response.safety import sanitize_ai_answer
from .response.fallback import build_fallback_answer

from .retrieval.references import filter_and_rank_ai_references
from .retrieval.reranker import rerank_candidates_with_local_ai
