"""LoRa bridge API routes."""

from fastapi import APIRouter, HTTPException, Query

from .models import LoraConnectRequest, LoraSendRequest
from .services.lora_service import (
    LoraBridgeError,
    clear_lora_messages,
    connect_lora_bridge,
    disconnect_lora_bridge,
    get_lora_status,
    list_lora_messages,
    list_lora_nodes,
    list_lora_ports,
    send_lora_message,
)

router = APIRouter(prefix="/api/lora", tags=["lora"])


@router.get("/ports")
def get_lora_ports(transport: str = Query("usb_serial")):
    try:
        return list_lora_ports(transport=transport)
    except LoraBridgeError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.get("/status")
def read_lora_status():
    return get_lora_status()


@router.post("/connect")
def connect_lora(payload: LoraConnectRequest):
    try:
        return connect_lora_bridge(
            port=payload.port,
            baud=payload.baud,
            transport=payload.transport,
        )
    except LoraBridgeError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.post("/disconnect")
def disconnect_lora():
    return disconnect_lora_bridge()


@router.post("/send")
def send_lora(payload: LoraSendRequest):
    try:
        targets = payload.targets if payload.targets is not None else payload.target
        return send_lora_message(payload.text, target=targets)
    except LoraBridgeError as error:
        raise HTTPException(status_code=400, detail=str(error))


@router.get("/messages")
def get_lora_messages(limit: int = Query(120, ge=1, le=500)):
    return list_lora_messages(limit=limit)


@router.get("/nodes")
def get_lora_nodes(
    limit: int = Query(120, ge=1, le=500),
    online_window_seconds: int = Query(900, ge=30, le=86400),
):
    return list_lora_nodes(
        limit=limit,
        online_window_seconds=online_window_seconds,
    )


@router.delete("/messages")
def delete_lora_messages():
    return clear_lora_messages()
