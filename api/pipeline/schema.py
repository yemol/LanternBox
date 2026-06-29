"""Pipeline 数据结构定义。包含 PipelineRequest 与 PipelineResult。"""

from pydantic import BaseModel, Field
from typing import Any, Callable, Dict, Generator, List, Optional


class PipelineRequest(BaseModel):
    message: str
    mode: str = "emergency"
    history: List[Dict[str, str]] = Field(default_factory=list)

    matched_triggers: List[Dict[str, Any]] = Field(default_factory=list)
    related_guides: List[Dict[str, Any]] = Field(default_factory=list)
    related_wikis: List[Dict[str, Any]] = Field(default_factory=list)
    detected_domains: List[str] = Field(default_factory=list)

    stream: bool = False
    metadata: Dict[str, Any] = Field(default_factory=dict)


class PipelineResult(BaseModel):
    mode: str
    answer: str = ""
    messages: List[Dict[str, str]] = Field(default_factory=list)
    debug: Dict[str, Any] = Field(default_factory=dict)

