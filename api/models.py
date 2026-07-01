"""FastAPI 请求与数据模型定义。负责 API 层输入输出结构。"""

from typing import List, Optional
from pydantic import BaseModel


class ChatHistoryItem(BaseModel):
    role: str
    content: str


class AiAdviceRequest(BaseModel):
    message: str
    mode: Optional[str] = "emergency"
    model: Optional[str] = None
    metadata_only: Optional[bool] = False
    history: Optional[List[ChatHistoryItem]] = None
    conversation_summary: Optional[str] = ""


class InventoryItem(BaseModel):
    name: str
    category: str = ""
    quantity: float = 0
    unit: str = ""
    expire_date: str = ""
    note: str = ""


class JournalEntry(BaseModel):
    entry_type: str = "日常记录"
    title: str = ""
    content: str


class TtsSpeakRequest(BaseModel):
    text: str
    mode: Optional[str] = "assistant"
    engine: Optional[str] = None


class AiRuntimeSettingsUpdate(BaseModel):
    retrieval_v2_model: Optional[str] = None
    show_retrieval_debug: Optional[bool] = None
