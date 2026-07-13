"""Terminal sync API routes for manifest and JSONL record upload."""

from fastapi import APIRouter, HTTPException

from .models import (
    TerminalSyncManifestRequest,
    TerminalSyncManifestResponse,
    TerminalSyncUploadRecordsRequest,
    TerminalSyncUploadRecordsResponse,
)
from .services.terminal_sync_service import (
    TerminalSyncDeviceNotFoundError,
    TerminalSyncDeviceNotTrustedError,
    receive_manifest,
    upload_records,
)

router = APIRouter(prefix="/api/terminal-sync", tags=["terminal-sync"])


@router.post("/manifest", response_model=TerminalSyncManifestResponse)
def receive_terminal_sync_manifest(payload: TerminalSyncManifestRequest):
    try:
        return receive_manifest(payload)
    except TerminalSyncDeviceNotFoundError as error:
        raise HTTPException(status_code=404, detail=f"terminal device not found: {error}")
    except TerminalSyncDeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")
    except ValueError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.post("/upload-records", response_model=TerminalSyncUploadRecordsResponse)
def upload_terminal_sync_records(payload: TerminalSyncUploadRecordsRequest):
    try:
        return upload_records(payload)
    except TerminalSyncDeviceNotFoundError as error:
        raise HTTPException(status_code=404, detail=f"terminal device not found: {error}")
    except TerminalSyncDeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")
    except ValueError as error:
        raise HTTPException(status_code=400, detail=str(error))
