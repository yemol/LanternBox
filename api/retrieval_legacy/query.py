"""查询文本处理模块。负责检索文本规范化、切词和 Context 词提取。"""

from typing import Any, List

from ..context.engine import analyze_context
from .constants import REFERENCE_DOMAIN_KEYWORDS


def normalize_reference_text(value: Any) -> str:
    """把来源、用户输入、列表或字典统一转成可检索文本。"""
    if isinstance(value, list):
        return " ".join(normalize_reference_text(item) for item in value)

    if isinstance(value, dict):
        return " ".join(normalize_reference_text(v) for v in value.values())

    return str(value or "").lower().replace("\n", " ").strip()


def re_split_reference(text: str) -> List[str]:
    """按常见中英文标点和空白切分查询文本。"""
    import re

    return re.split(
        r"[\s,，、。；;：:！!？?（）()【】\[\]「」\"'“”‘’/\\]+",
        text,
    )


def get_context_terms(user_message: str) -> List[str]:
    """从 Context Engine 提取可用于检索增强的词。

    注意：
    - 这里只依赖底层 analyze_context。
    - 不允许依赖 response.context_blocks。
    - Context Engine 失败时静默降级，不影响原检索流程。
    """
    try:
        ctx = analyze_context(user_message or "")
        if hasattr(ctx, "model_dump"):
            data = ctx.model_dump()
        elif hasattr(ctx, "dict"):
            data = ctx.dict()
        else:
            data = dict(ctx or {})
    except Exception:
        return []

    terms: List[str] = []

    for key in [
        "retrieval_plan",
        "signals",
        "domains",
        "risks",
        "inferred_needs",
        "observations",
    ]:
        value = data.get(key, [])
        if isinstance(value, list):
            terms.extend(str(item) for item in value if item)
        elif isinstance(value, str) and value:
            terms.append(value)

    return terms


def tokenize_reference_query(user_message: str, max_tokens: int = 32) -> List[str]:
    """生成用于引用召回和打分的查询 token。

    输入来源包括：
    1. 用户原始输入分词
    2. 领域关键词命中
    3. Context Engine 生成的检索计划、风险、信号和观察
    """
    text = normalize_reference_text(user_message)
    tokens: List[str] = []

    for token in re_split_reference(text):
        token = token.strip()
        if len(token) >= 2:
            tokens.append(token)

    for domain, words in REFERENCE_DOMAIN_KEYWORDS.items():
        for word in words:
            keyword = str(word).lower().strip()
            if len(keyword) >= 2 and keyword in text:
                tokens.append(keyword)
                tokens.append(domain)

    tokens.extend(get_context_terms(user_message))

    seen = set()
    unique_tokens: List[str] = []

    for token in tokens:
        token = str(token).strip()
        if not token:
            continue
        if token in seen:
            continue
        seen.add(token)
        unique_tokens.append(token)

    return unique_tokens[:max_tokens]
