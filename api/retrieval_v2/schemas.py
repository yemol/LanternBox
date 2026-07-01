"""Retrieval v2 schemas.

AI 负责理解、规划、筛选和综合。
代码负责取数、串联、校验、整理和兜底。
"""

from typing import Any, Dict, List, Literal

from pydantic import BaseModel, Field, field_validator


SourceType = Literal["guide", "wiki", "kiwix", "log", "inventory", "sensor", "map"]


class SourcePlanItem(BaseModel):
    source_type: SourceType
    purpose: str = ""
    query: str = ""
    categories: List[str] = Field(default_factory=list)
    keywords: List[str] = Field(default_factory=list)
    limit: int = 8

    @field_validator("categories", "keywords", mode="before")
    @classmethod
    def _ensure_string_list(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        if isinstance(value, str):
            return [value]
        return []


class RetrievalPlan(BaseModel):
    scenario_summary: str = ""
    urgency: str = "unknown"
    needs: List[str] = Field(default_factory=list)
    core_terms: List[str] = Field(default_factory=list)
    source_plan: List[SourcePlanItem] = Field(default_factory=list)
    raw: Dict[str, Any] = Field(default_factory=dict)

    @field_validator("needs", "core_terms", mode="before")
    @classmethod
    def _ensure_string_list(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        if isinstance(value, str):
            return [value]
        return []

    @field_validator("source_plan", mode="before")
    @classmethod
    def _ensure_source_plan_list(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        return []


class EvidenceCandidate(BaseModel):
    source_type: SourceType
    id: str
    title: str
    summary: str = ""
    category: str = ""
    tags: List[str] = Field(default_factory=list)
    snippet: str = ""
    raw: Dict[str, Any] = Field(default_factory=dict)

    @field_validator("tags", mode="before")
    @classmethod
    def _ensure_tags(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        if isinstance(value, str):
            return [value]
        return []


class SelectedEvidence(BaseModel):
    source_type: SourceType
    id: str
    reason: str = ""


class EvidenceSelection(BaseModel):
    selected: List[SelectedEvidence] = Field(default_factory=list)
    excluded: List[SelectedEvidence] = Field(default_factory=list)
    answer_focus: List[str] = Field(default_factory=list)
    raw: Dict[str, Any] = Field(default_factory=dict)

    @field_validator("selected", "excluded", mode="before")
    @classmethod
    def _ensure_selection_list(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        return []

    @field_validator("answer_focus", mode="before")
    @classmethod
    def _ensure_answer_focus(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        if isinstance(value, str):
            return [value]
        return []


class RetrievalDebug(BaseModel):
    engine: str = "retrieval_v2_ai_orchestrated"
    candidate_types: Dict[str, int] = Field(default_factory=dict)
    candidate_count: int = 0
    selected_count: int = 0
    source_plan_count: int = 0
    core_terms: List[str] = Field(default_factory=list)
    ok: bool = True
    error: str = ""
    stage: str = ""

    @field_validator("core_terms", mode="before")
    @classmethod
    def _ensure_core_terms(cls, value):
        if value is None:
            return []
        if isinstance(value, list):
            return value
        if isinstance(value, str):
            return [value]
        return []


class RetrievalV2Result(BaseModel):
    engine: str = "retrieval_v2_ai_orchestrated"
    plan: RetrievalPlan
    candidates: List[EvidenceCandidate] = Field(default_factory=list)
    selection: EvidenceSelection
    selected_evidence: List[EvidenceCandidate] = Field(default_factory=list)
    debug: RetrievalDebug = Field(default_factory=RetrievalDebug)
