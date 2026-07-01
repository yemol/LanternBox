"""Retrieval v2 schemas.

AI 负责理解、规划、筛选和综合。
代码负责取数、串联、校验、整理和兜底。
"""

from typing import Any, Dict, List, Literal

from pydantic import BaseModel, Field


SourceType = Literal["guide", "wiki", "kiwix", "log", "inventory", "sensor", "map"]


class SourcePlanItem(BaseModel):
    source_type: SourceType
    purpose: str = ""
    query: str = ""
    categories: List[str] = Field(default_factory=list)
    keywords: List[str] = Field(default_factory=list)
    limit: int = 8


class RetrievalPlan(BaseModel):
    scenario_summary: str = ""
    urgency: str = "unknown"
    needs: List[str] = Field(default_factory=list)
    core_terms: List[str] = Field(default_factory=list)
    source_plan: List[SourcePlanItem] = Field(default_factory=list)
    raw: Dict[str, Any] = Field(default_factory=dict)


class EvidenceCandidate(BaseModel):
    source_type: SourceType
    id: str
    title: str
    summary: str = ""
    category: str = ""
    tags: List[str] = Field(default_factory=list)
    snippet: str = ""
    raw: Dict[str, Any] = Field(default_factory=dict)


class SelectedEvidence(BaseModel):
    source_type: SourceType
    id: str
    reason: str = ""


class EvidenceSelection(BaseModel):
    selected: List[SelectedEvidence] = Field(default_factory=list)
    excluded: List[SelectedEvidence] = Field(default_factory=list)
    answer_focus: List[str] = Field(default_factory=list)
    raw: Dict[str, Any] = Field(default_factory=dict)


class RetrievalV2Result(BaseModel):
    engine: str = "retrieval_v2_ai_orchestrated"
    plan: RetrievalPlan
    candidates: List[EvidenceCandidate] = Field(default_factory=list)
    selection: EvidenceSelection
    selected_evidence: List[EvidenceCandidate] = Field(default_factory=list)
    debug: Dict[str, Any] = Field(default_factory=dict)
