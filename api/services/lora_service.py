"""LoRa bridge service.

Core talks to the Heltec receiver through USB serial or a Bluetooth serial
profile. The Heltec firmware is expected to emit line-delimited received
messages and accept line-delimited outbound messages.
"""

from __future__ import annotations

import json
import asyncio
import re
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any

from ..db import get_db_connection

try:
    import serial
    from serial.tools import list_ports
except Exception:  # pragma: no cover - surfaced through API status
    serial = None
    list_ports = None

try:
    from bleak import BleakClient, BleakScanner
except Exception:  # pragma: no cover - surfaced through API status
    BleakClient = None
    BleakScanner = None


MAX_MESSAGE_CHARS = 240
MAX_TARGET_CHARS = 96
SERIAL_READ_TIMEOUT_SECONDS = 0.25
SUPPORTED_TRANSPORTS = {"usb_serial", "bluetooth_serial", "ble_uart"}
BLE_SCAN_TIMEOUT_SECONDS = 5.0
LORA_NODE_ONLINE_WINDOW_SECONDS = 15 * 60
MESHTASTIC_NODE_BOOTSTRAP_TIMEOUT_SECONDS = 12
MESHTASTIC_SEND_TIMEOUT_SECONDS = 8
BLE_NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
BLE_NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
ANSI_ESCAPE_RE = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")
MESH_FROM_RE = re.compile(r"(?:\bfrom|\bfr)=0x([0-9a-fA-F]+)")
MESH_UPDATE_NODE_RE = re.compile(r"Update DB node 0x([0-9a-fA-F]+)", re.IGNORECASE)
MESH_RSSI_RE = re.compile(r"\brxRSSI=(-?\d+(?:\.\d+)?)", re.IGNORECASE)
MESH_SNR_RE = re.compile(r"\brxSNR=(-?\d+(?:\.\d+)?)", re.IGNORECASE)
MESH_LOG_LINE_RE = re.compile(r"^(?:RX\s+)?(?:DEBUG|INFO|WARN|WARNING|ERROR|TRACE)\s*\|", re.IGNORECASE)
MESH_LOG_MARKERS = (
    "[Router]",
    "[SerialConsole]",
    "[RadioIf]",
    "[NodeDB]",
    "phone downloaded packet",
    "Received routing",
    "Update DB node",
    "wantsPacket",
)


class LoraBridgeError(Exception):
    pass


def _now() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def _meshtastic_available() -> bool:
    try:
        import meshtastic.serial_interface  # noqa: F401
        return True
    except Exception:
        return False


@dataclass
class LoraBridgeState:
    connected: bool = False
    transport: str = "usb_serial"
    port: str = ""
    baud: int = 115200
    status: str = "idle"
    error: str = ""
    connected_at: str = ""
    updated_at: str = field(default_factory=lambda: _now())
    rx_count: int = 0
    tx_count: int = 0
    last_rx_at: str = ""
    last_tx_at: str = ""
    node_bootstrap_at: str = ""
    node_bootstrap_status: str = ""
    node_bootstrap_count: int = 0
    node_bootstrap_error: str = ""


_STATE = LoraBridgeState()
_SERIAL: Any = None
_BLE_CLIENT: Any = None
_BLE_LOOP: asyncio.AbstractEventLoop | None = None
_BLE_RX_BUFFER = ""
_STOP_EVENT: threading.Event | None = None
_READER_THREAD: threading.Thread | None = None
_LOCK = threading.RLock()


def _state_dict() -> dict[str, Any]:
    return {
        "connected": _STATE.connected,
        "transport": _STATE.transport,
        "port": _STATE.port,
        "baud": _STATE.baud,
        "status": _STATE.status,
        "error": _STATE.error,
        "connected_at": _STATE.connected_at,
        "updated_at": _STATE.updated_at,
        "rx_count": _STATE.rx_count,
        "tx_count": _STATE.tx_count,
        "last_rx_at": _STATE.last_rx_at,
        "last_tx_at": _STATE.last_tx_at,
        "node_bootstrap_at": _STATE.node_bootstrap_at,
        "node_bootstrap_status": _STATE.node_bootstrap_status,
        "node_bootstrap_count": _STATE.node_bootstrap_count,
        "node_bootstrap_error": _STATE.node_bootstrap_error,
        "serial_available": serial is not None,
        "ble_available": BleakClient is not None and BleakScanner is not None,
        "meshtastic_available": _meshtastic_available(),
    }


def _append_message(
    *,
    direction: str,
    text: str,
    raw: str = "",
    transport: str = "",
    port: str = "",
    rssi: float | None = None,
    snr: float | None = None,
    metadata: dict[str, Any] | None = None,
) -> dict[str, Any]:
    value = str(text or "").strip()
    raw_value = str(raw or value)
    if not value:
        value = raw_value.strip()
    if not value:
        value = "(empty)"

    conn = get_db_connection()
    cursor = conn.execute(
        """
        INSERT INTO lora_messages
        (direction, text, raw, transport, port, rssi, snr, metadata_json, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (
            direction,
            value,
            raw_value,
            normalize_transport(transport),
            port,
            rssi,
            snr,
            json.dumps(metadata or {}, ensure_ascii=False, separators=(",", ":")),
            _now(),
        ),
    )
    conn.commit()
    row = conn.execute(
        "SELECT * FROM lora_messages WHERE id = ?",
        (cursor.lastrowid,),
    ).fetchone()
    conn.close()
    return _row_to_message(row)


def _row_to_message(row) -> dict[str, Any]:
    if not row:
        return {}
    metadata = {}
    try:
        metadata = json.loads(row["metadata_json"] or "{}")
    except Exception:
        metadata = {}
    return {
        "id": row["id"],
        "direction": row["direction"],
        "text": row["text"],
        "raw": row["raw"] or "",
        "transport": row["transport"] or "usb_serial",
        "port": row["port"] or "",
        "rssi": row["rssi"],
        "snr": row["snr"],
        "metadata": metadata,
        "created_at": row["created_at"],
    }


def _metadata_user(metadata: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(metadata, dict):
        return {}
    user = metadata.get("user")
    if isinstance(user, dict):
        return user
    node = metadata.get("node")
    if isinstance(node, dict) and isinstance(node.get("user"), dict):
        return node["user"]
    return {}


def _row_to_node(row, online_window_seconds: int = LORA_NODE_ONLINE_WINDOW_SECONDS) -> dict[str, Any]:
    metadata = {}
    try:
        metadata = json.loads(row["metadata_json"] or "{}")
    except Exception:
        metadata = {}
    user = _metadata_user(metadata)
    long_name = str(
        user.get("longName")
        or user.get("long_name")
        or metadata.get("longName")
        or metadata.get("long_name")
        or ""
    ).strip()
    short_name = str(
        metadata.get("short_name")
        or user.get("shortName")
        or user.get("short_name")
        or metadata.get("shortName")
        or ""
    ).strip()
    row_name = str(row["name"] or "").strip()
    display_name = long_name or short_name or row_name

    last_seen = str(row["last_seen_at"] or "")
    online = False
    try:
        last_seen_dt = datetime.strptime(last_seen, "%Y-%m-%d %H:%M:%S")
        online = (datetime.now() - last_seen_dt).total_seconds() <= online_window_seconds
    except Exception:
        online = False

    return {
        "node_id": row["node_id"],
        "name": display_name,
        "long_name": long_name,
        "short_name": short_name,
        "role": row["role"] or "",
        "status": "online" if online else (row["status"] or "seen"),
        "online": online,
        "transport": row["transport"] or "",
        "port": row["port"] or "",
        "rssi": row["rssi"],
        "snr": row["snr"],
        "message_count": row["message_count"] or 0,
        "metadata": metadata,
        "first_seen_at": row["first_seen_at"] or "",
        "last_seen_at": last_seen,
    }


def _parse_timestamp(value: Any) -> datetime:
    try:
        return datetime.strptime(str(value or ""), "%Y-%m-%d %H:%M:%S")
    except Exception:
        return datetime.min


def _is_placeholder_node_name(name: Any, node_id: Any) -> bool:
    value = str(name or "").strip()
    if not value:
        return True
    lowered = value.lower()
    normalized_id = _normalize_node_id(node_id).lower()
    if lowered == normalized_id or lowered == str(node_id or "").strip().lower():
        return True
    return lowered.startswith("0x") or lowered.startswith("!")


def _has_better_node_identity(candidate: dict[str, Any], existing: dict[str, Any]) -> bool:
    candidate_has_name = not _is_placeholder_node_name(candidate.get("name"), candidate.get("node_id"))
    existing_has_name = not _is_placeholder_node_name(existing.get("name"), existing.get("node_id"))
    if candidate_has_name != existing_has_name:
        return candidate_has_name

    candidate_source = str((candidate.get("metadata") or {}).get("source") or "")
    existing_source = str((existing.get("metadata") or {}).get("source") or "")
    if candidate_source == "meshtastic_api" and existing_source != "meshtastic_api":
        return True

    return len(str(candidate.get("name") or "")) > len(str(existing.get("name") or ""))


def _merge_lora_node(existing: dict[str, Any], candidate: dict[str, Any]) -> dict[str, Any]:
    canonical_id = _normalize_node_id(candidate.get("node_id")) or str(candidate.get("node_id") or "")
    if not existing:
        merged = dict(candidate)
        merged["node_id"] = canonical_id
        return merged

    existing_last_seen = _parse_timestamp(existing.get("last_seen_at"))
    candidate_last_seen = _parse_timestamp(candidate.get("last_seen_at"))
    candidate_is_newer = candidate_last_seen >= existing_last_seen

    existing["message_count"] = int(existing.get("message_count") or 0) + int(candidate.get("message_count") or 0)
    if candidate.get("first_seen_at") and (
        not existing.get("first_seen_at")
        or _parse_timestamp(candidate.get("first_seen_at")) < _parse_timestamp(existing.get("first_seen_at"))
    ):
        existing["first_seen_at"] = candidate.get("first_seen_at")

    if candidate_is_newer:
        existing["last_seen_at"] = candidate.get("last_seen_at") or existing.get("last_seen_at", "")
        existing["online"] = bool(candidate.get("online"))
        existing["status"] = candidate.get("status") or existing.get("status", "seen")
        existing["transport"] = candidate.get("transport") or existing.get("transport", "")
        existing["port"] = candidate.get("port") or existing.get("port", "")
        if candidate.get("rssi") is not None:
            existing["rssi"] = candidate.get("rssi")
        if candidate.get("snr") is not None:
            existing["snr"] = candidate.get("snr")
    elif candidate.get("online"):
        existing["online"] = True
        existing["status"] = "online"

    if _has_better_node_identity(candidate, existing):
        existing["name"] = candidate.get("name") or existing.get("name", "")
        existing["role"] = candidate.get("role") or existing.get("role", "")
        existing["metadata"] = candidate.get("metadata") or existing.get("metadata", {})
    elif not existing.get("role") and candidate.get("role"):
        existing["role"] = candidate.get("role")

    return existing


def _coerce_float(value: Any) -> float | None:
    if value is None or value == "":
        return None
    try:
        return float(value)
    except Exception:
        return None


def _parse_inbound_line(line: str) -> dict[str, Any]:
    raw = str(line or "").strip()
    parsed: dict[str, Any] = {
        "text": raw,
        "raw": raw,
        "rssi": None,
        "snr": None,
        "metadata": {},
    }
    if not raw:
        return parsed

    try:
        data = json.loads(raw)
    except Exception:
        data = None

    if isinstance(data, dict):
        text = data.get("text") or data.get("message") or data.get("payload") or data.get("raw") or raw
        parsed["text"] = str(text)
        parsed["rssi"] = _coerce_float(data.get("rssi"))
        parsed["snr"] = _coerce_float(data.get("snr"))
        parsed["metadata"] = data
        return parsed

    # Common bridge output: LORA_RX rssi=-91.5 snr=8.2 text=hello
    if raw.startswith("LORA_RX "):
        body = raw[len("LORA_RX "):]
        metadata: dict[str, Any] = {}
        text = body
        if " text=" in body:
            head, text = body.split(" text=", 1)
            for token in head.split():
                if "=" not in token:
                    continue
                key, value = token.split("=", 1)
                metadata[key] = value
            parsed["rssi"] = _coerce_float(metadata.get("rssi"))
            parsed["snr"] = _coerce_float(metadata.get("snr"))
        parsed["text"] = text.strip() or raw
        parsed["metadata"] = metadata

    return parsed


def _decode_line(data: bytes) -> str:
    return data.decode("utf-8", errors="replace").strip()


def _strip_ansi(value: str) -> str:
    return ANSI_ESCAPE_RE.sub("", str(value or ""))


def _parse_key_value_tokens(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in str(text or "").split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        key = key.strip().lower()
        value = value.strip().strip('"').strip("'")
        if key:
            result[key] = value
    return result


def _first_present(data: dict[str, Any], keys: tuple[str, ...]) -> Any:
    for key in keys:
        value = data.get(key)
        if value is not None and str(value).strip() != "":
            return value
    return None


def _normalize_node_id(value: Any) -> str:
    text = str(value or "").strip()
    if not text or text.lower() in {"unknown", "none", "null"}:
        return ""
    if text.startswith("!") and re.fullmatch(r"![0-9a-fA-F]+", text):
        try:
            return f"!{int(text[1:], 16):08x}"
        except Exception:
            return text.lower()
    if text.lower().startswith("0x"):
        try:
            return f"!{int(text, 16):08x}"
        except Exception:
            return text.lower()
    return text[:96]


def _normalize_message_target(value: Any) -> str:
    text = str(value or "").strip()
    if not text or text.lower() in {"broadcast", "all", "*"}:
        return ""
    return _normalize_node_id(text)[:MAX_TARGET_CHARS]


def _normalize_message_targets(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, str):
        text = value.strip()
        if not text or text.lower() in {"broadcast", "all", "*"}:
            return []
        raw_values = [item.strip() for item in text.split(",")]
    elif isinstance(value, (list, tuple, set)):
        raw_values = [str(item or "").strip() for item in value]
    else:
        raw_values = [str(value or "").strip()]

    targets: list[str] = []
    for item in raw_values:
        normalized = _normalize_message_target(item)
        if normalized and normalized not in targets:
            targets.append(normalized)
    return targets[:50]


def _format_outbound_payload(text: str, target: str = "") -> str:
    if not target:
        return text
    return f"LORA_TX target={target} text={text}"


def _format_outbound_payloads(text: str, targets: list[str]) -> list[str]:
    if not targets:
        return [text]
    return [_format_outbound_payload(text, target) for target in targets]


def _target_mode(targets: list[str]) -> str:
    if not targets:
        return "broadcast"
    return "direct" if len(targets) == 1 else "multicast"


def _is_lora_log_line(text: str) -> bool:
    value = _strip_ansi(text).strip()
    if not value:
        return False
    if MESH_LOG_LINE_RE.search(value):
        return True
    return any(marker in value for marker in MESH_LOG_MARKERS)


def _record_seen_node(
    *,
    node_id: str,
    name: str = "",
    role: str = "",
    status: str = "online",
    transport: str = "",
    port: str = "",
    rssi: float | None = None,
    snr: float | None = None,
    metadata: dict[str, Any] | None = None,
) -> None:
    normalized_id = _normalize_node_id(node_id)
    if not normalized_id:
        return

    now = _now()
    transport_value = normalize_transport(transport)
    node_name = str(name or "")
    if _is_placeholder_node_name(node_name, normalized_id):
        node_name = ""
    metadata_json = json.dumps(metadata or {}, ensure_ascii=False, separators=(",", ":"))
    conn = get_db_connection()
    try:
        conn.execute(
            """
            INSERT INTO lora_nodes
            (
                node_id, name, role, status, transport, port, rssi, snr,
                message_count, metadata_json, first_seen_at, last_seen_at
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(node_id) DO UPDATE SET
                name = CASE WHEN excluded.name != '' THEN excluded.name ELSE lora_nodes.name END,
                role = CASE WHEN excluded.role != '' THEN excluded.role ELSE lora_nodes.role END,
                status = excluded.status,
                transport = excluded.transport,
                port = excluded.port,
                rssi = COALESCE(excluded.rssi, lora_nodes.rssi),
                snr = COALESCE(excluded.snr, lora_nodes.snr),
                message_count = lora_nodes.message_count + 1,
                metadata_json = CASE
                    WHEN excluded.metadata_json LIKE '%"source":"meshtastic_log"%'
                        AND lora_nodes.metadata_json LIKE '%"source":"meshtastic_api"%'
                    THEN lora_nodes.metadata_json
                    ELSE excluded.metadata_json
                END,
                last_seen_at = excluded.last_seen_at
            """,
            (
                normalized_id,
                node_name,
                str(role or ""),
                str(status or "online"),
                transport_value,
                str(port or ""),
                rssi,
                snr,
                1,
                metadata_json,
                now,
                now,
            ),
        )
        conn.commit()
    finally:
        conn.close()


def _mesh_node_num_to_id(value: Any) -> str:
    if value is None or value == "":
        return ""
    if isinstance(value, int):
        return f"0x{value:08x}"
    text = str(value).strip()
    if not text:
        return ""
    if text.startswith("!"):
        return text
    if text.lower().startswith("0x"):
        try:
            return f"0x{int(text, 16):08x}"
        except Exception:
            return text.lower()
    try:
        return f"0x{int(text):08x}"
    except Exception:
        return text


def _node_last_seen_text(node: dict[str, Any]) -> str:
    raw = node.get("lastHeard") or node.get("last_heard") or node.get("lastSeen")
    if raw is None:
        return ""
    try:
        return datetime.fromtimestamp(float(raw)).strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        return ""


def _record_meshtastic_node(
    node_key: Any,
    node: dict[str, Any],
    *,
    transport: str,
    port: str,
) -> bool:
    if not isinstance(node, dict):
        return False

    user = node.get("user") if isinstance(node.get("user"), dict) else {}
    node_id = _normalize_node_id(
        user.get("id")
        or node.get("id")
        or node.get("num")
        or node.get("nodeNum")
        or _mesh_node_num_to_id(node_key)
    )
    if not node_id:
        return False

    long_name = str(user.get("longName") or user.get("long_name") or node.get("longName") or "")
    short_name = str(user.get("shortName") or user.get("short_name") or node.get("shortName") or "")
    name = long_name or short_name or node_id
    role = str(user.get("role") or node.get("role") or "meshtastic_node")
    rssi = _coerce_float(node.get("rssi"))
    snr = _coerce_float(node.get("snr"))
    last_seen = _node_last_seen_text(node)
    metadata = {
        "source": "meshtastic_api",
        "user": user,
        "node": node,
    }
    if short_name:
        metadata["short_name"] = short_name

    _record_seen_node(
        node_id=node_id,
        name=name,
        role=role,
        status="online" if last_seen else "seen",
        transport=transport,
        port=port,
        rssi=rssi,
        snr=snr,
        metadata=metadata,
    )

    if last_seen:
        conn = get_db_connection()
        try:
            conn.execute(
                """
                UPDATE lora_nodes
                SET last_seen_at = ?
                WHERE node_id = ?
                """,
                (last_seen, node_id),
            )
            conn.commit()
        finally:
            conn.close()
    return True


def bootstrap_meshtastic_nodes(port: str, transport: str = "usb_serial") -> dict[str, Any]:
    port_value = str(port or "").strip()
    if not port_value:
        return {"ok": False, "count": 0, "message": "port is required"}

    try:
        from meshtastic.serial_interface import SerialInterface
    except Exception as exc:
        return {
            "ok": False,
            "count": 0,
            "message": f"meshtastic package is not available: {exc}",
        }

    interface = None
    try:
        interface = SerialInterface(
            devPath=port_value,
            debugOut=None,
            noProto=False,
            connectNow=True,
            noNodes=False,
            timeout=MESHTASTIC_NODE_BOOTSTRAP_TIMEOUT_SECONDS,
        )
        nodes = getattr(interface, "nodes", None) or getattr(interface, "nodesByNum", None) or {}
        count = 0
        if isinstance(nodes, dict):
            for key, node in nodes.items():
                if _record_meshtastic_node(key, node, transport=transport, port=port_value):
                    count += 1

        my_node = interface.getMyNodeInfo() if hasattr(interface, "getMyNodeInfo") else None
        if isinstance(my_node, dict):
            if _record_meshtastic_node(
                my_node.get("num") or my_node.get("nodeNum") or "self",
                my_node,
                transport=transport,
                port=port_value,
            ):
                count = max(count, 1)

        return {"ok": True, "count": count, "message": f"loaded {count} Meshtastic nodes"}
    except Exception as exc:
        return {"ok": False, "count": 0, "message": str(exc)}
    finally:
        if interface is not None:
            try:
                interface.close()
            except Exception:
                pass


def _extract_nodes_from_metadata(metadata: dict[str, Any], rssi: float | None, snr: float | None) -> list[dict[str, Any]]:
    if not isinstance(metadata, dict):
        return []

    nodes: list[dict[str, Any]] = []
    raw_nodes = metadata.get("nodes") or metadata.get("terminals") or metadata.get("devices")
    if isinstance(raw_nodes, list):
        for item in raw_nodes:
            if isinstance(item, dict):
                nodes.extend(_extract_nodes_from_metadata(item, rssi, snr))
            else:
                node_id = _normalize_node_id(item)
                if node_id:
                    nodes.append({"node_id": node_id, "metadata": {"source": "node_list"}})

    node_id = _normalize_node_id(_first_present(
        metadata,
        ("device_id", "node_id", "terminal_id", "from", "from_id", "sender", "sender_id", "node"),
    ))
    if node_id:
        nodes.append({
            "node_id": node_id,
            "name": str(_first_present(metadata, ("name", "short_name", "long_name", "device_name", "terminal_name")) or ""),
            "role": str(_first_present(metadata, ("role", "type", "kind")) or ""),
            "status": str(_first_present(metadata, ("status", "state")) or "online"),
            "rssi": _coerce_float(metadata.get("rssi")) if metadata.get("rssi") is not None else rssi,
            "snr": _coerce_float(metadata.get("snr")) if metadata.get("snr") is not None else snr,
            "metadata": metadata,
        })
    return nodes


def _extract_nodes_from_line(raw: str, parsed: dict[str, Any]) -> list[dict[str, Any]]:
    nodes = _extract_nodes_from_metadata(
        parsed.get("metadata") or {},
        parsed.get("rssi"),
        parsed.get("snr"),
    )

    text = _strip_ansi(raw).strip()
    if text.startswith("LORA_NODE ") or text.startswith("LORA_ONLINE "):
        values = _parse_key_value_tokens(text)
        node_id = _normalize_node_id(_first_present(
            values,
            ("device_id", "node_id", "terminal_id", "id", "from", "sender", "node"),
        ))
        if node_id:
            nodes.append({
                "node_id": node_id,
                "name": values.get("name", ""),
                "role": values.get("role", ""),
                "status": values.get("status", "online"),
                "rssi": _coerce_float(values.get("rssi")),
                "snr": _coerce_float(values.get("snr")),
                "metadata": values,
            })

    mesh_node_ids = set()
    mesh_node_ids.update(f"0x{match.group(1).lower()}" for match in MESH_FROM_RE.finditer(text))
    mesh_node_ids.update(f"0x{match.group(1).lower()}" for match in MESH_UPDATE_NODE_RE.finditer(text))
    if mesh_node_ids:
        rssi = _coerce_float(MESH_RSSI_RE.search(text).group(1)) if MESH_RSSI_RE.search(text) else parsed.get("rssi")
        snr = _coerce_float(MESH_SNR_RE.search(text).group(1)) if MESH_SNR_RE.search(text) else parsed.get("snr")
        for node_id in sorted(mesh_node_ids):
            nodes.append({
                "node_id": node_id,
                "name": node_id,
                "role": "meshtastic_node",
                "status": "online",
                "rssi": rssi,
                "snr": snr,
                "metadata": {
                    "source": "meshtastic_log",
                    "line": text[:500],
                },
            })
    return nodes


def _record_inbound_line(line: str, transport: str, port: str) -> None:
    text = _strip_ansi(line).strip()
    if not text:
        return

    parsed = _parse_inbound_line(text)
    for node in _extract_nodes_from_line(text, parsed):
        _record_seen_node(
            node_id=node.get("node_id", ""),
            name=node.get("name", ""),
            role=node.get("role", ""),
            status=node.get("status", "online"),
            transport=transport,
            port=port,
            rssi=node.get("rssi"),
            snr=node.get("snr"),
            metadata=node.get("metadata") or {},
        )

    if _is_lora_log_line(text):
        return

    _append_message(
        direction="rx",
        text=parsed["text"],
        raw=parsed["raw"],
        transport=transport,
        port=port,
        rssi=parsed["rssi"],
        snr=parsed["snr"],
        metadata=parsed["metadata"],
    )
    with _LOCK:
        _STATE.rx_count += 1
        _STATE.last_rx_at = _now()
        _STATE.updated_at = _STATE.last_rx_at


def normalize_transport(transport: str) -> str:
    value = str(transport or "usb_serial").strip().lower()
    if value in {"usb", "serial"}:
        return "usb_serial"
    if value in {"bluetooth", "bt", "ble", "ble_uart"}:
        return "ble_uart"
    if value in {"bluetooth_serial", "bt_serial", "rfcomm"}:
        return "bluetooth_serial"
    if value not in SUPPORTED_TRANSPORTS:
        raise LoraBridgeError(f"unsupported transport: {transport}")
    return value


def _transport_label(transport: str) -> str:
    if transport == "ble_uart":
        return "Bluetooth BLE UART"
    return "Bluetooth Serial" if transport == "bluetooth_serial" else "USB Serial"


def _is_bluetooth_port(lowered: str) -> bool:
    if "bluetooth-incoming-port" in lowered:
        return False
    return any(marker in lowered for marker in ("bluetooth", "bt-", "ble", "rfcomm"))


def _is_usb_lora_candidate(lowered: str) -> bool:
    return any(
        marker in lowered
        for marker in ("heltec", "usbmodem", "usbserial", "wchusbserial", "cp210", "ch340")
    )


def _is_ble_lora_candidate(lowered: str) -> bool:
    return any(marker in lowered for marker in ("heltec", "lora", "lantern", "lb-", "lb_"))


def _run_async(coro):
    loop = asyncio.new_event_loop()
    try:
        return loop.run_until_complete(coro)
    finally:
        loop.close()


async def _scan_ble_devices() -> list[dict[str, Any]]:
    assert BleakScanner is not None
    devices = await BleakScanner.discover(timeout=BLE_SCAN_TIMEOUT_SECONDS)
    results = []
    for device in devices:
        name = str(getattr(device, "name", "") or "")
        address = str(getattr(device, "address", "") or "")
        lowered = f"{name} {address}".lower()
        if not address:
            continue
        results.append({
            "device": address,
            "description": name or "BLE Device",
            "hwid": address,
            "transport": "ble_uart",
            "transport_label": _transport_label("ble_uart"),
            "likely_lora": _is_ble_lora_candidate(lowered),
            "rssi": getattr(device, "rssi", None),
        })
    results.sort(key=lambda item: (0 if item["likely_lora"] else 1, item["description"], item["device"]))
    return results


def _handle_ble_notification(port: str, data: bytearray) -> None:
    global _BLE_RX_BUFFER

    decoded = bytes(data or b"").decode("utf-8", errors="replace")
    if not decoded:
        return

    lines: list[str] = []
    with _LOCK:
        _BLE_RX_BUFFER += decoded
        while "\n" in _BLE_RX_BUFFER:
            line, _BLE_RX_BUFFER = _BLE_RX_BUFFER.split("\n", 1)
            lines.append(line.strip("\r"))
        if len(_BLE_RX_BUFFER) > 4096:
            lines.append(_BLE_RX_BUFFER)
            _BLE_RX_BUFFER = ""

    for line in lines:
        _record_inbound_line(line, "ble_uart", port)


async def _ble_client_loop(port: str, stop_event: threading.Event, ready_event: threading.Event) -> None:
    global _BLE_CLIENT

    client = BleakClient(port)
    try:
        await client.connect()
        await client.start_notify(
            BLE_NUS_TX_CHAR_UUID,
            lambda _sender, data: _handle_ble_notification(port, data),
        )
        with _LOCK:
            _BLE_CLIENT = client
            _STATE.connected = True
            _STATE.transport = "ble_uart"
            _STATE.port = port
            _STATE.baud = 0
            _STATE.status = "connected"
            _STATE.error = ""
            _STATE.connected_at = _now()
            _STATE.updated_at = _STATE.connected_at
        ready_event.set()

        while not stop_event.is_set() and client.is_connected:
            await asyncio.sleep(0.2)
    except Exception as exc:
        with _LOCK:
            _STATE.connected = False
            _STATE.status = "error"
            _STATE.error = str(exc)
            _STATE.updated_at = _now()
        ready_event.set()
    finally:
        try:
            if client.is_connected:
                await client.stop_notify(BLE_NUS_TX_CHAR_UUID)
        except Exception:
            pass
        try:
            if client.is_connected:
                await client.disconnect()
        except Exception:
            pass
        with _LOCK:
            if _BLE_CLIENT is client:
                _BLE_CLIENT = None


def _run_ble_thread(port: str, stop_event: threading.Event, ready_event: threading.Event) -> None:
    global _BLE_LOOP

    loop = asyncio.new_event_loop()
    with _LOCK:
        _BLE_LOOP = loop
    try:
        loop.run_until_complete(_ble_client_loop(port, stop_event, ready_event))
    finally:
        with _LOCK:
            if _BLE_LOOP is loop:
                _BLE_LOOP = None
        loop.close()


def _reader_loop(stop_event: threading.Event) -> None:
    global _SERIAL

    while not stop_event.is_set():
        with _LOCK:
            ser = _SERIAL
            transport = _STATE.transport
            port = _STATE.port

        if ser is None:
            time.sleep(0.1)
            continue

        try:
            data = ser.readline()
        except Exception as exc:
            if stop_event.is_set():
                break
            with _LOCK:
                _STATE.connected = False
                _STATE.status = "error"
                _STATE.error = str(exc)
                _STATE.updated_at = _now()
            break

        if not data:
            continue

        line = _decode_line(data)
        if not line:
            continue

        _record_inbound_line(line, transport, port)


def list_lora_ports(transport: str = "usb_serial") -> dict[str, Any]:
    transport_value = normalize_transport(transport)
    if transport_value == "ble_uart":
        if BleakScanner is None:
            return {
                "ok": False,
                "transport": transport_value,
                "ports": [],
                "message": "bleak is required for Bluetooth BLE. Install with: pip install bleak",
            }
        ports = _run_async(_scan_ble_devices())
        message = ""
        if not ports:
            message = "未发现 BLE 设备。请确认 Heltec 已开机、靠近 Core，并且固件已启用 BLE UART。"
        return {"ok": True, "transport": transport_value, "ports": ports, "message": message}

    if list_ports is None:
        return {
            "ok": False,
            "transport": transport_value,
            "ports": [],
            "message": "pyserial is required. Install with: pip install pyserial",
        }

    ports = []
    for port in list(list_ports.comports()):
        device = str(port.device or "")
        description = str(port.description or "")
        lowered = f"{device} {description}".lower()
        if "debug-console" in lowered or "bluetooth-incoming-port" in lowered:
            continue
        is_bluetooth = _is_bluetooth_port(lowered)
        is_usb_lora = _is_usb_lora_candidate(lowered)
        if transport_value == "bluetooth_serial" and not is_bluetooth:
            continue
        if transport_value == "usb_serial" and is_bluetooth:
            continue
        ports.append({
            "device": device,
            "description": description,
            "hwid": getattr(port, "hwid", ""),
            "transport": transport_value,
            "transport_label": _transport_label(transport_value),
            "likely_lora": is_usb_lora or is_bluetooth,
        })

    ports.sort(key=lambda item: (0 if item["likely_lora"] else 1, item["device"]))
    message = ""
    if transport_value == "bluetooth_serial" and not ports:
        message = "未发现蓝牙串口。请先在系统蓝牙设置中配对 Heltec，并确认固件提供 Bluetooth Serial/SPP。"
    return {"ok": True, "transport": transport_value, "ports": ports, "message": message}


def get_lora_status() -> dict[str, Any]:
    with _LOCK:
        return {"ok": True, "bridge": _state_dict()}


def connect_lora_bridge(
    port: str,
    baud: int = 115200,
    transport: str = "usb_serial",
) -> dict[str, Any]:
    global _SERIAL, _STOP_EVENT, _READER_THREAD

    transport_value = normalize_transport(transport)
    port_value = str(port or "").strip()
    if not port_value:
        raise LoraBridgeError("port is required")

    disconnect_lora_bridge()

    if transport_value == "ble_uart":
        if BleakClient is None:
            raise LoraBridgeError("bleak is required for Bluetooth BLE. Install with: pip install bleak")
        stop_event = threading.Event()
        ready_event = threading.Event()
        reader = threading.Thread(
            target=_run_ble_thread,
            args=(port_value, stop_event, ready_event),
            daemon=True,
        )
        with _LOCK:
            _STOP_EVENT = stop_event
            _READER_THREAD = reader
            _STATE.transport = "ble_uart"
            _STATE.port = port_value
            _STATE.baud = 0
            _STATE.status = "connecting"
            _STATE.error = ""
            _STATE.updated_at = _now()
        reader.start()
        ready_event.wait(timeout=12.0)
        with _LOCK:
            connected = _STATE.connected
            error = _STATE.error
        if not connected:
            disconnect_lora_bridge()
            raise LoraBridgeError(error or "failed to connect BLE UART device")
        return get_lora_status()

    if serial is None:
        raise LoraBridgeError("pyserial is required. Install with: pip install pyserial")

    bootstrap = bootstrap_meshtastic_nodes(port_value, transport=transport_value)
    with _LOCK:
        _STATE.node_bootstrap_at = _now()
        _STATE.node_bootstrap_status = "ok" if bootstrap.get("ok") else "failed"
        _STATE.node_bootstrap_count = int(bootstrap.get("count") or 0)
        _STATE.node_bootstrap_error = "" if bootstrap.get("ok") else str(bootstrap.get("message") or "")

    try:
        ser = serial.Serial(
            port=port_value,
            baudrate=int(baud or 115200),
            timeout=SERIAL_READ_TIMEOUT_SECONDS,
            write_timeout=2,
        )
    except Exception as exc:
        raise LoraBridgeError(f"failed to open serial port: {exc}")

    stop_event = threading.Event()
    reader = threading.Thread(target=_reader_loop, args=(stop_event,), daemon=True)

    with _LOCK:
        _SERIAL = ser
        _STOP_EVENT = stop_event
        _READER_THREAD = reader
        _STATE.connected = True
        _STATE.transport = transport_value
        _STATE.port = port_value
        _STATE.baud = int(baud or 115200)
        _STATE.status = "connected"
        _STATE.error = ""
        _STATE.connected_at = _now()
        _STATE.updated_at = _STATE.connected_at

    reader.start()
    return get_lora_status()


def _pause_serial_reader() -> tuple[Any, threading.Event | None, threading.Thread | None]:
    global _SERIAL, _STOP_EVENT, _READER_THREAD

    with _LOCK:
        ser = _SERIAL
        stop_event = _STOP_EVENT
        reader = _READER_THREAD
        _SERIAL = None
        _STOP_EVENT = None
        _READER_THREAD = None

    if stop_event is not None:
        stop_event.set()
    if ser is not None:
        try:
            ser.close()
        except Exception:
            pass
    if reader is not None and reader.is_alive():
        reader.join(timeout=0.8)
    return ser, stop_event, reader


def _resume_serial_reader(port: str, baud: int, transport: str) -> None:
    global _SERIAL, _STOP_EVENT, _READER_THREAD

    if serial is None:
        raise LoraBridgeError("pyserial is required. Install with: pip install pyserial")

    ser = serial.Serial(
        port=port,
        baudrate=int(baud or 115200),
        timeout=SERIAL_READ_TIMEOUT_SECONDS,
        write_timeout=2,
    )
    stop_event = threading.Event()
    reader = threading.Thread(target=_reader_loop, args=(stop_event,), daemon=True)
    with _LOCK:
        _SERIAL = ser
        _STOP_EVENT = stop_event
        _READER_THREAD = reader
        _STATE.connected = True
        _STATE.transport = transport
        _STATE.port = port
        _STATE.baud = int(baud or 115200)
        _STATE.status = "connected"
        _STATE.error = ""
        _STATE.updated_at = _now()
    reader.start()


def _send_meshtastic_text(port: str, text: str, targets: list[str]) -> None:
    try:
        from meshtastic.serial_interface import SerialInterface
    except Exception as exc:
        raise LoraBridgeError(f"meshtastic package is not available: {exc}") from exc

    interface = None
    try:
        interface = SerialInterface(
            devPath=port,
            debugOut=None,
            noProto=False,
            connectNow=True,
            noNodes=True,
            timeout=MESHTASTIC_SEND_TIMEOUT_SECONDS,
        )
        destinations = targets or ["^all"]
        for destination in destinations:
            interface.sendText(text, destinationId=destination)
    finally:
        if interface is not None:
            try:
                interface.close()
            except Exception:
                pass


def disconnect_lora_bridge() -> dict[str, Any]:
    global _SERIAL, _STOP_EVENT, _READER_THREAD, _BLE_RX_BUFFER

    with _LOCK:
        ser = _SERIAL
        stop_event = _STOP_EVENT
        reader = _READER_THREAD
        _SERIAL = None
        _STOP_EVENT = None
        _READER_THREAD = None
        _BLE_RX_BUFFER = ""

    if stop_event is not None:
        stop_event.set()
    if ser is not None:
        try:
            ser.close()
        except Exception:
            pass
    if reader is not None and reader.is_alive():
        reader.join(timeout=0.8)

    with _LOCK:
        _STATE.connected = False
        _STATE.status = "idle"
        _STATE.error = ""
        _STATE.transport = "usb_serial"
        _STATE.port = ""
        _STATE.rx_count = 0
        _STATE.tx_count = 0
        _STATE.last_rx_at = ""
        _STATE.last_tx_at = ""
        _STATE.updated_at = _now()

    return get_lora_status()


def send_lora_message(text: str, target: Any = "") -> dict[str, Any]:
    value = str(text or "").strip()
    if not value:
        raise LoraBridgeError("message text is required")
    if len(value) > MAX_MESSAGE_CHARS:
        raise LoraBridgeError(f"message is too long; keep it under {MAX_MESSAGE_CHARS} characters")
    target_values = _normalize_message_targets(target)
    send_count = max(1, len(target_values))

    with _LOCK:
        ser = _SERIAL
        ble_client = _BLE_CLIENT
        ble_loop = _BLE_LOOP
        transport = _STATE.transport
        port = _STATE.port
        baud = _STATE.baud
        connected = _STATE.connected

    if not connected:
        raise LoraBridgeError("LoRa bridge is not connected")

    wire_texts = _format_outbound_payloads(value, target_values)
    sent_by = "serial_bridge"
    if transport == "usb_serial" and _meshtastic_available():
        _pause_serial_reader()
        send_error: Exception | None = None
        resume_error: Exception | None = None
        try:
            _send_meshtastic_text(port, value, target_values)
            sent_by = "meshtastic_api"
        except Exception as exc:
            send_error = exc
        finally:
            try:
                _resume_serial_reader(port, baud, transport)
            except Exception as exc:
                resume_error = exc

        if send_error is not None or resume_error is not None:
            error_parts = []
            if send_error is not None:
                error_parts.append(f"failed to send Meshtastic message: {send_error}")
            if resume_error is not None:
                error_parts.append(f"failed to resume serial reader: {resume_error}")
            with _LOCK:
                _STATE.connected = resume_error is None
                _STATE.status = "error"
                _STATE.error = "; ".join(error_parts)
                _STATE.updated_at = _now()
            raise LoraBridgeError(_STATE.error)
    else:
        payload = ("".join(f"{wire_text}\n" for wire_text in wire_texts)).encode("utf-8")
        if transport == "ble_uart":
            if ble_client is None or ble_loop is None:
                raise LoraBridgeError("Bluetooth BLE bridge is not connected")
            try:
                future = asyncio.run_coroutine_threadsafe(
                    ble_client.write_gatt_char(BLE_NUS_RX_CHAR_UUID, payload, response=False),
                    ble_loop,
                )
                future.result(timeout=5.0)
                sent_by = "ble_uart"
            except Exception as exc:
                with _LOCK:
                    _STATE.status = "error"
                    _STATE.error = str(exc)
                    _STATE.updated_at = _now()
                raise LoraBridgeError(f"failed to write BLE message: {exc}")
        else:
            if ser is None:
                raise LoraBridgeError("Serial bridge is not connected")
            try:
                ser.write(payload)
            except Exception as exc:
                with _LOCK:
                    _STATE.status = "error"
                    _STATE.error = str(exc)
                    _STATE.updated_at = _now()
                raise LoraBridgeError(f"failed to write serial message: {exc}")

    metadata = {
        "target": target_values[0] if len(target_values) == 1 else "",
        "targets": target_values,
        "target_count": len(target_values),
        "target_mode": _target_mode(target_values),
        "sent_by": sent_by,
    }
    message = _append_message(
        direction="tx",
        text=value,
        raw=value if sent_by == "meshtastic_api" else "\n".join(wire_texts),
        transport=transport,
        port=port,
        metadata=metadata,
    )
    with _LOCK:
        _STATE.tx_count += send_count
        _STATE.last_tx_at = _now()
        _STATE.updated_at = _STATE.last_tx_at

    return {"ok": True, "message": message, "bridge": _state_dict()}


def list_lora_messages(limit: int = 120) -> dict[str, Any]:
    safe_limit = max(1, min(int(limit or 120), 500))
    fetch_limit = min(max(safe_limit * 8, 500), 3000)
    conn = get_db_connection()
    rows = conn.execute(
        """
        SELECT * FROM lora_messages
        ORDER BY id DESC
        LIMIT ?
        """,
        (fetch_limit,),
    ).fetchall()
    conn.close()
    messages = [
        message
        for message in (_row_to_message(row) for row in reversed(rows))
        if not _is_lora_log_line(message.get("raw") or message.get("text") or "")
    ][-safe_limit:]
    return {"ok": True, "messages": messages}


def list_lora_nodes(
    limit: int = 120,
    online_window_seconds: int = LORA_NODE_ONLINE_WINDOW_SECONDS,
) -> dict[str, Any]:
    safe_limit = max(1, min(int(limit or 120), 500))
    safe_window = max(30, min(int(online_window_seconds or LORA_NODE_ONLINE_WINDOW_SECONDS), 24 * 60 * 60))
    fetch_limit = max(safe_limit * 4, 500)
    conn = get_db_connection()
    try:
        rows = conn.execute(
            """
            SELECT * FROM lora_nodes
            ORDER BY last_seen_at DESC
            LIMIT ?
            """,
            (fetch_limit,),
        ).fetchall()
    finally:
        conn.close()

    merged_nodes: dict[str, dict[str, Any]] = {}
    for row in rows:
        node = _row_to_node(row, online_window_seconds=safe_window)
        canonical_id = _normalize_node_id(node.get("node_id"))
        if not canonical_id:
            continue
        merged_nodes[canonical_id] = _merge_lora_node(merged_nodes.get(canonical_id, {}), node)

    nodes = sorted(
        merged_nodes.values(),
        key=lambda node: _parse_timestamp(node.get("last_seen_at")),
        reverse=True,
    )[:safe_limit]
    online_count = sum(1 for node in nodes if node["online"])
    return {
        "ok": True,
        "online_window_seconds": safe_window,
        "online_count": online_count,
        "total": len(nodes),
        "nodes": nodes,
    }


def clear_lora_messages() -> dict[str, Any]:
    conn = get_db_connection()
    cursor = conn.execute("DELETE FROM lora_messages")
    conn.commit()
    deleted = cursor.rowcount
    conn.close()
    return {"ok": True, "deleted": deleted}
