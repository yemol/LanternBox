"""Terminal sync API routes for manifest, record upload, audio upload, and commit."""

from fastapi import APIRouter, HTTPException, Query, Request
from fastapi.responses import FileResponse, JSONResponse

from .models import (
    TerminalSyncManifestRequest,
    TerminalSyncManifestResponse,
    TerminalSyncCommitRequest,
    TerminalSyncCommitResponse,
    TerminalSyncUploadAudioResponse,
    TerminalSyncUploadRecordsRequest,
    TerminalSyncUploadRecordsResponse,
)
from .services.terminal_sync_service import (
    TerminalSyncAudioConflictError,
    TerminalSyncDeviceNotFoundError,
    TerminalSyncDeviceNotTrustedError,
    commit_sync,
    receive_manifest,
    upload_audio_file,
    upload_records,
)
from .services.terminal_audio_service import (
    TerminalAudioNotFoundError,
    TerminalAudioPathError,
    get_audio_file_for_playback,
)
from .services.terminal_usb_sync_job_service import (
    TerminalUsbSyncError,
    get_usb_sync_job,
    list_sync_ports,
    list_usb_sync_jobs,
    start_usb_sync_job,
)

router = APIRouter(prefix="/api/terminal-sync", tags=["terminal-sync"])


def _parse_audio_size(size: str | None) -> int | None:
    if size is None or str(size).strip() == "":
        return None
    try:
        parsed = int(str(size).strip())
    except ValueError:
        raise ValueError("size must be a non-negative integer")
    if parsed < 0:
        raise ValueError("size must be a non-negative integer")
    return parsed


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


@router.post("/upload-audio", response_model=TerminalSyncUploadAudioResponse)
async def upload_terminal_sync_audio(
    request: Request,
    device_id: str = Query(...),
    sync_session_id: str = Query(...),
    audio_id: str = Query(...),
    filename: str = Query(""),
    size: str | None = Query(None),
):
    content = b""
    try:
        parsed_size = _parse_audio_size(size)
        content = await request.body()
        return upload_audio_file(
            device_id=device_id,
            sync_session_id=sync_session_id,
            audio_id=audio_id,
            filename=filename,
            expected_size=parsed_size,
            content=content,
        )
    except TerminalSyncDeviceNotFoundError as error:
        raise HTTPException(status_code=404, detail=f"terminal device not found: {error}")
    except TerminalSyncDeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")
    except TerminalSyncAudioConflictError as error:
        return JSONResponse(
            status_code=409,
            content={
                "ok": False,
                "audio_id": audio_id,
                "size": len(content),
                "status": "conflict",
                "imported": False,
                "duplicate": False,
                "conflict": True,
                "path": "",
                "ack": False,
                "message": str(error),
            },
        )
    except ValueError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.get("/audio")
def play_terminal_sync_audio(audio_id: str = Query(...)):
    try:
        path, record = get_audio_file_for_playback(audio_id)
    except TerminalAudioNotFoundError as error:
        raise HTTPException(status_code=404, detail=str(error))
    except TerminalAudioPathError as error:
        raise HTTPException(status_code=403, detail=str(error))

    filename = str(record.get("filename") or path.name)
    return FileResponse(
        path,
        media_type="audio/wav",
        filename=filename,
    )


@router.get("/ports")
def list_terminal_sync_ports():
    return list_sync_ports()


@router.get("/jobs")
def list_terminal_sync_jobs():
    return {"ok": True, "jobs": list_usb_sync_jobs()}


@router.post("/jobs")
async def create_terminal_sync_job(request: Request):
    try:
        payload = await request.json()
    except Exception:
        payload = {}

    try:
        return start_usb_sync_job(
            port=str(payload.get("port") or ""),
            baud=int(payload.get("baud") or 115200),
            api_base=str(payload.get("api_base") or str(request.base_url).rstrip("/")),
            upload_records=bool(payload.get("upload_records", True)),
            upload_audio_files=bool(payload.get("upload_audio_files", True)),
        )
    except TerminalUsbSyncError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.get("/jobs/{job_id}")
def get_terminal_sync_job(job_id: str):
    job = get_usb_sync_job(job_id)
    if not job:
        raise HTTPException(status_code=404, detail=f"sync job not found: {job_id}")
    return {"ok": True, "job": job}


@router.post("/commit", response_model=TerminalSyncCommitResponse)
def commit_terminal_sync(payload: TerminalSyncCommitRequest):
    try:
        return commit_sync(payload)
    except TerminalSyncDeviceNotFoundError as error:
        raise HTTPException(status_code=404, detail=f"terminal device not found: {error}")
    except TerminalSyncDeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")
    except ValueError as error:
        raise HTTPException(status_code=400, detail=str(error))
