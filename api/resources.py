"""本地资源协调层。

Retrieval v2 迁移后，本文件只负责加载本地基础资料缓存。
"""

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
