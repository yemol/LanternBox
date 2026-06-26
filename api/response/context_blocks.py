from typing import Any, Dict

from ..context.engine import analyze_context


def build_lantern_context(user_message: str) -> Dict[str, Any]:
    """生成 LanternBox 统一上下文。

    Context Engine 是增强层，不允许因为它失败而影响原有 AI / RAG 流程。
    """
    try:
        ctx = analyze_context(user_message or "")
        if hasattr(ctx, "model_dump"):
            return ctx.model_dump()
        if hasattr(ctx, "dict"):
            return ctx.dict()
        return dict(ctx or {})
    except Exception as exc:
        return {
            "_context_error": str(exc)[:200],
            "input_text": user_message or "",
        }


def format_lantern_context_for_prompt(context: Dict[str, Any]) -> str:
    """把 Lantern Context 转成 prompt 可读文本。

    注意：错误信息不进入 prompt，只用于 debug。
    """
    if not context or context.get("_context_error"):
        return ""

    observations = "、".join(context.get("observations", []) or [])
    domains = "、".join(context.get("domains", []) or [])
    signals = "、".join(context.get("signals", []) or [])
    risks = "、".join(context.get("risks", []) or [])
    inferred_needs = "、".join(context.get("inferred_needs", []) or [])
    retrieval_plan = "、".join(context.get("retrieval_plan", []) or [])
    risk_level = context.get("risk_level", "unknown")
    input_nature = context.get("input_nature", "unknown")

    if not any([observations, domains, signals, risks, inferred_needs, retrieval_plan]):
        return ""

    return (
        "【Lantern Context】\n"
        f"- 输入性质：{input_nature}\n"
        f"- 观察：{observations or '无'}\n"
        f"- 领域：{domains or '无'}\n"
        f"- 信号：{signals or '无'}\n"
        f"- 风险：{risks or '无'}\n"
        f"- 风险等级：{risk_level}\n"
        f"- 推断需求：{inferred_needs or '无'}\n"
        f"- 检索计划：{retrieval_plan or '无'}\n"
    )