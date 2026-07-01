"""领域识别与兼容判断。负责引用来源和用户查询的领域检测。"""

from typing import Dict, List, Optional

from ..context.engine import analyze_context
from .constants import (
    REFERENCE_DOMAIN_KEYWORDS,
    CATEGORY_DOMAIN_HINTS,
    DOMAIN_COMPATIBILITY,
)
from .query import normalize_reference_text

def detect_reference_domains(user_message: str, detected_domains: Optional[List[str]] = None) -> List[str]:
    """用户原话优先，上游 detected_domains 与 Context Engine 做兜底。

    这里保持柔性：Context 只增加候选领域，不做硬判定。
    """
    explicit_domains = detect_domains_from_text(user_message, for_query=True)

    if explicit_domains:
        return explicit_domains

    domains: List[str] = []
    for domain in detected_domains or []:
        domain = str(domain or "").lower().strip()
        if domain in REFERENCE_DOMAIN_KEYWORDS and domain not in domains:
            domains.append(domain)

    try:
        ctx = analyze_context(user_message or "")
        if hasattr(ctx, "model_dump"):
            lantern_context = ctx.model_dump()
        elif hasattr(ctx, "dict"):
            lantern_context = ctx.dict()
        else:
            lantern_context = dict(ctx or {})
    except Exception:
        lantern_context = {}
        
    for domain in lantern_context.get("domains") or []:
        domain = str(domain or "").lower().strip()
        if domain in REFERENCE_DOMAIN_KEYWORDS and domain not in domains:
            domains.append(domain)

    return domains

def detect_domains_from_text(text: str, *, for_query: bool = False) -> List[str]:
    normalized = normalize_reference_text(text)
    hits: Dict[str, int] = {}

    for domain, keywords in REFERENCE_DOMAIN_KEYWORDS.items():
        score = 0
        for word in keywords:
            keyword = str(word).lower().strip()
            if not keyword:
                continue

            # 用户问题领域识别不吃单字，避免“外面”的“面”之类误判。
            if for_query and len(keyword) <= 1:
                continue

            if keyword in normalized:
                score += 1

        if score:
            hits[domain] = score

    return [
        domain
        for domain, _score in sorted(hits.items(), key=lambda item: item[1], reverse=True)
    ]


def domains_compatible(query_domains: List[str], item_domains: List[str]) -> bool:
    if not query_domains:
        return True

    if not item_domains:
        return False

    for query_domain in query_domains:
        allowed = DOMAIN_COMPATIBILITY.get(query_domain, {query_domain})
        if any(item_domain in allowed for item_domain in item_domains):
            return True

    return False
