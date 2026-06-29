"""Context Engine 规则库。负责输入性质、风险与需求的规则识别。"""

from .schema import LanternContext
from ..config import CONTEXT_PROFILES_CACHE
from .profile_engine import match_profile, apply_profile


def apply_context_profiles(text: str, context: LanternContext) -> LanternContext:

    # ---------- V2 Context Profile ----------
    # 只保留真正的规则（如果还有）
    ...

    # 最后统一跑 Profile
    for profile in CONTEXT_PROFILES_CACHE:
        if match_profile(profile, text):
            apply_profile(profile, context)

    return context
