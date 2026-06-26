# -----------------------------------------------------------------------------
# AI 来源验收器 v0.5.1
# 目标：召回可以宽，但展示和进入 AI 上下文的来源必须通过验收。
# 重点：不依赖大模型，不让“外面”的“面”把夜间安全问题误判成食物问题。
#
# query.py
# 负责把用户输入变成 tokens
# domains.py
# 负责识别领域、判断领域兼容
# references.py
# 负责对候选引用资料进行过滤、打分、排序
#
# -----------------------------------------------------------------------------


from .llm.client import call_ollama, stream_ollama

from .response.prompts import (
    build_ai_messages,
    build_safe_history,
    build_companion_messages,
    build_emergency_messages,
)
from .response.safety import sanitize_ai_answer
from .response.fallback import build_fallback_answer

from .retrieval.references import filter_and_rank_ai_references
from .retrieval.reranker import rerank_candidates_with_local_ai
from .retrieval.apply import (
    apply_rerank_result_to_context,
    build_selected_sources_text,
)