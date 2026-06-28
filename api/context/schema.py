"""Context Engine 数据结构定义。描述 LanternContext 及其组成字段。"""

from pydantic import BaseModel, Field
from typing import Dict, List, Any


class LanternContext(BaseModel):
    source: str = "text"
    input_text: str = ""

    input_nature: str = "unknown"

    observations: List[str] = Field(default_factory=list)
    domains: List[str] = Field(default_factory=list)
    signals: List[str] = Field(default_factory=list)
    risks: List[str] = Field(default_factory=list)

    risk_level: str = "low"

    inferred_needs: List[str] = Field(default_factory=list)
    retrieval_plan: List[str] = Field(default_factory=list)

    confidence: Dict[str, float] = Field(default_factory=dict)
    metadata: Dict[str, Any] = Field(default_factory=dict)

    intents: List[str] = Field(default_factory=list)
