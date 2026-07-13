"""Terminal device registry service.

This module owns terminal device persistence and keeps route handlers thin.
"""

from datetime import datetime, timezone
from typing import Optional

from ..db import get_db_connection
from ..models import TerminalDevice, TerminalRegisterRequest


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _row_to_terminal_device(row) -> TerminalDevice:
    return TerminalDevice(
        device_id=row["device_id"],
        name=row["name"] or "",
        role=row["role"] or "",
        status=row["status"] or "active",
        trusted=bool(row["trusted"]),
        created_at=row["created_at"] or "",
        last_seen_at=row["last_seen_at"] or "",
        last_sync_at=row["last_sync_at"],
        firmware_version=row["firmware_version"] or "",
        notes=row["notes"] or "",
    )


def list_terminal_devices() -> list[TerminalDevice]:
    conn = get_db_connection()
    try:
        rows = conn.execute(
            """
            SELECT * FROM terminal_devices
            ORDER BY last_seen_at DESC, created_at DESC
            """
        ).fetchall()
        return [_row_to_terminal_device(row) for row in rows]
    finally:
        conn.close()


def get_terminal_device(device_id: str) -> Optional[TerminalDevice]:
    conn = get_db_connection()
    try:
        row = conn.execute(
            "SELECT * FROM terminal_devices WHERE device_id = ?",
            (device_id,),
        ).fetchone()
        if not row:
            return None
        return _row_to_terminal_device(row)
    finally:
        conn.close()


def register_terminal_device(payload: TerminalRegisterRequest) -> TerminalDevice:
    device_id = payload.device_id.strip()
    now = _now_iso()

    conn = get_db_connection()
    try:
        existing = conn.execute(
            "SELECT * FROM terminal_devices WHERE device_id = ?",
            (device_id,),
        ).fetchone()

        if existing:
            conn.execute(
                """
                UPDATE terminal_devices
                SET name = ?,
                    role = ?,
                    firmware_version = ?,
                    last_seen_at = ?,
                    notes = COALESCE(?, notes)
                WHERE device_id = ?
                """,
                (
                    payload.name,
                    payload.role,
                    payload.firmware_version,
                    now,
                    payload.notes,
                    device_id,
                ),
            )
        else:
            conn.execute(
                """
                INSERT INTO terminal_devices
                (
                    device_id,
                    name,
                    role,
                    status,
                    trusted,
                    created_at,
                    last_seen_at,
                    last_sync_at,
                    firmware_version,
                    notes
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    device_id,
                    payload.name,
                    payload.role,
                    "active",
                    1,
                    now,
                    now,
                    None,
                    payload.firmware_version,
                    payload.notes or "",
                ),
            )

        conn.commit()
    finally:
        conn.close()

    device = get_terminal_device(device_id)
    if device is None:
        raise RuntimeError("terminal device registration failed")
    return device


def set_terminal_trust(device_id: str, trusted: bool) -> Optional[TerminalDevice]:
    conn = get_db_connection()
    try:
        cursor = conn.execute(
            """
            UPDATE terminal_devices
            SET trusted = ?
            WHERE device_id = ?
            """,
            (1 if trusted else 0, device_id),
        )
        conn.commit()
        if cursor.rowcount == 0:
            return None
    finally:
        conn.close()

    return get_terminal_device(device_id)


def update_terminal_last_seen(device_id: str) -> Optional[TerminalDevice]:
    now = _now_iso()
    conn = get_db_connection()
    try:
        cursor = conn.execute(
            """
            UPDATE terminal_devices
            SET last_seen_at = ?
            WHERE device_id = ?
            """,
            (now, device_id),
        )
        conn.commit()
        if cursor.rowcount == 0:
            return None
    finally:
        conn.close()

    return get_terminal_device(device_id)
