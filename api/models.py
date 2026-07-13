"""FastAPI 请求与数据模型定义。负责 API 层输入输出结构。"""

from typing import Any, List, Optional
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


class TerminalRegisterRequest(BaseModel):
    device_id: str
    name: str = ""
    role: str = "field_terminal"
    firmware_version: str = ""
    notes: Optional[str] = None


class TerminalTrustRequest(BaseModel):
    trusted: bool


class TerminalDevice(BaseModel):
    device_id: str
    name: str = ""
    role: str = ""
    status: str = "active"
    trusted: bool = True
    created_at: str
    last_seen_at: str
    last_sync_at: Optional[str] = None
    firmware_version: str = ""
    notes: str = ""


class TerminalRegisterResponse(BaseModel):
    ok: bool
    device_id: str
    trusted: bool


class TaskCreateRequest(BaseModel):
    title: str
    description: str = ""
    priority: str = "normal"
    assigned_to: Optional[List[str]] = None
    target: Optional[dict[str, Any]] = None
    tags: Optional[List[str]] = None
    created_by: str = "core"


class TaskUpdateRequest(BaseModel):
    title: Optional[str] = None
    description: Optional[str] = None
    priority: Optional[str] = None
    status: Optional[str] = None
    assigned_to: Optional[List[str]] = None
    target: Optional[dict[str, Any]] = None
    tags: Optional[List[str]] = None


class TaskAssignRequest(BaseModel):
    assigned_to: List[str]


class TaskReportRequest(BaseModel):
    device_id: str
    task_id: str
    status: Optional[str] = None
    note: str = ""
    device_date: str = ""
    device_time: str = ""
    lat: Optional[float] = None
    lon: Optional[float] = None


class TaskItem(BaseModel):
    task_id: str
    title: str
    description: str = ""
    priority: str
    status: str
    assigned_to: List[str]
    created_at: str
    updated_at: str
    revision: int
    target: dict[str, Any]
    tags: List[str]
    created_by: str = "core"


class TaskMutationResponse(BaseModel):
    ok: bool
    task_id: str
    revision: int


class TerminalSyncManifestRequest(BaseModel):
    device_id: str
    firmware_version: str = ""
    sync_session_id: str
    transport: str = ""
    items: dict[str, Any] = {}


class TerminalSyncManifestResponse(BaseModel):
    ok: bool
    sync_session_id: str
    need: dict[str, Any]


class TerminalSyncUploadRecordsRequest(BaseModel):
    device_id: str
    sync_session_id: str
    record_type: str
    records: List[dict[str, Any]]


class TerminalSyncUploadRecordsResponse(BaseModel):
    ok: bool
    record_type: str
    received: int
    imported: int
    skipped_duplicate: int
    ack: bool
