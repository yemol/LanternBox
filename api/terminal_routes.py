"""Terminal device management API routes."""

from fastapi import APIRouter, HTTPException

from .models import (
    TerminalDevice,
    TerminalRegisterRequest,
    TerminalRegisterResponse,
    TerminalTrustRequest,
)
from .services.terminal_service import (
    get_terminal_device,
    list_terminal_devices,
    register_terminal_device,
    set_terminal_trust,
)

router = APIRouter(prefix="/api/terminals", tags=["terminals"])


@router.get("", response_model=list[TerminalDevice])
def list_terminals():
    return list_terminal_devices()


@router.post("/register", response_model=TerminalRegisterResponse)
def register_terminal(payload: TerminalRegisterRequest):
    if not payload.device_id.strip():
        raise HTTPException(status_code=400, detail="device_id is required")

    device = register_terminal_device(payload)
    return {
        "ok": True,
        "device_id": device.device_id,
        "trusted": device.trusted,
    }


@router.get("/{device_id}", response_model=TerminalDevice)
def get_terminal(device_id: str):
    device = get_terminal_device(device_id)
    if not device:
        raise HTTPException(status_code=404, detail="terminal device not found")
    return device


@router.post("/{device_id}/trust", response_model=TerminalRegisterResponse)
def update_terminal_trust(device_id: str, payload: TerminalTrustRequest):
    device = set_terminal_trust(device_id, payload.trusted)
    if not device:
        raise HTTPException(status_code=404, detail="terminal device not found")

    return {
        "ok": True,
        "device_id": device.device_id,
        "trusted": device.trusted,
    }
