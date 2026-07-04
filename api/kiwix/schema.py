"""Structured result schema for the Kiwix stub layer."""

from dataclasses import asdict, dataclass, field
from typing import Any, Dict, List, Optional


@dataclass(frozen=True)
class KiwixResult:
    title: str
    source: str
    snippet: str
    relevance_score: float
    topics: List[str] = field(default_factory=list)
    url: Optional[str] = None
    content_type: str = "kiwix"

    def __post_init__(self) -> None:
        object.__setattr__(self, "title", str(self.title or "").strip())
        object.__setattr__(self, "source", str(self.source or "").strip())
        object.__setattr__(self, "snippet", str(self.snippet or "").strip())
        object.__setattr__(self, "url", str(self.url).strip() if self.url else None)
        object.__setattr__(self, "content_type", "kiwix")

        score = float(self.relevance_score or 0.0)
        object.__setattr__(self, "relevance_score", max(0.0, min(score, 1.0)))

        topics = self.topics or []
        normalized_topics = []
        for topic in topics:
            topic = str(topic or "").strip()
            if topic and topic not in normalized_topics:
                normalized_topics.append(topic)
        object.__setattr__(self, "topics", normalized_topics)

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)
