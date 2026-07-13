"""Core task system service.

This module owns task persistence, assignment checks, terminal pull rules, and
task report recording. It does not implement terminal sync or transport.
"""

import json
from datetime import datetime, timezone
from typing import Any, Optional

from ..db import get_db_connection
from ..models import (
    TaskAssignRequest,
    TaskCreateRequest,
    TaskItem,
    TaskReportRequest,
    TaskUpdateRequest,
)
from .terminal_service import get_terminal_device


TASK_STATUSES = {"open", "assigned", "in_progress", "blocked", "done", "cancelled"}
TASK_PRIORITIES = {"low", "normal", "high", "critical"}
FINAL_TASK_STATUSES = {"done", "cancelled"}


class TaskNotFoundError(Exception):
    pass


class DeviceNotFoundError(Exception):
    pass


class DeviceNotTrustedError(Exception):
    pass


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _json_dumps(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def _json_loads(value: Optional[str], fallback: Any) -> Any:
    if not value:
        return fallback
    try:
        return json.loads(value)
    except json.JSONDecodeError:
        return fallback


def _normalize_string_list(items: Optional[list[str]]) -> list[str]:
    if not items:
        return []
    normalized = []
    for item in items:
        text = str(item or "").strip()
        if text and text not in normalized:
            normalized.append(text)
    return normalized


def _validate_priority(priority: str) -> str:
    normalized = str(priority or "normal").strip() or "normal"
    if normalized not in TASK_PRIORITIES:
        raise ValueError(f"unsupported task priority: {normalized}")
    return normalized


def _validate_status(status: str) -> str:
    normalized = str(status or "open").strip() or "open"
    if normalized not in TASK_STATUSES:
        raise ValueError(f"unsupported task status: {normalized}")
    return normalized


def _ensure_devices_exist(device_ids: list[str]) -> None:
    for device_id in device_ids:
        if get_terminal_device(device_id) is None:
            raise DeviceNotFoundError(device_id)


def _ensure_trusted_device(device_id: str) -> None:
    device = get_terminal_device(device_id)
    if device is None:
        raise DeviceNotFoundError(device_id)
    if not device.trusted:
        raise DeviceNotTrustedError(device_id)


def _row_to_task(row) -> TaskItem:
    return TaskItem(
        task_id=row["task_id"],
        title=row["title"] or "",
        description=row["description"] or "",
        priority=row["priority"] or "normal",
        status=row["status"] or "open",
        assigned_to=_json_loads(row["assigned_to"], []),
        created_at=row["created_at"] or "",
        updated_at=row["updated_at"] or "",
        revision=int(row["revision"] or 1),
        target=_json_loads(row["target_json"], {}),
        tags=_json_loads(row["tags_json"], []),
        created_by=row["created_by"] or "core",
    )


def _insert_task_event(conn, task_id: str, event_type: str, payload: dict[str, Any]) -> None:
    conn.execute(
        """
        INSERT INTO task_events (task_id, event_type, payload_json, created_at)
        VALUES (?, ?, ?, ?)
        """,
        (task_id, event_type, _json_dumps(payload), _now_iso()),
    )


def _generate_task_id(conn) -> str:
    prefix = f"TASK-{datetime.now(timezone.utc).strftime('%Y%m%d')}-"
    row = conn.execute(
        """
        SELECT task_id FROM terminal_tasks
        WHERE task_id LIKE ?
        ORDER BY task_id DESC
        LIMIT 1
        """,
        (f"{prefix}%",),
    ).fetchone()

    next_number = 1
    if row:
        try:
            next_number = int(str(row["task_id"]).rsplit("-", 1)[1]) + 1
        except (IndexError, ValueError):
            next_number = 1

    return f"{prefix}{next_number:04d}"


def create_task(payload: TaskCreateRequest) -> TaskItem:
    title = payload.title.strip()
    if not title:
        raise ValueError("task title is required")

    assigned_to = _normalize_string_list(payload.assigned_to)
    _ensure_devices_exist(assigned_to)

    priority = _validate_priority(payload.priority)
    status = "assigned" if assigned_to else "open"
    created_at = _now_iso()
    target = payload.target or {}
    tags = _normalize_string_list(payload.tags)
    created_by = payload.created_by.strip() or "core"

    conn = get_db_connection()
    try:
        task_id = _generate_task_id(conn)
        conn.execute(
            """
            INSERT INTO terminal_tasks
            (
                task_id,
                title,
                description,
                priority,
                status,
                assigned_to,
                created_at,
                updated_at,
                revision,
                target_json,
                tags_json,
                created_by
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                task_id,
                title,
                payload.description,
                priority,
                status,
                _json_dumps(assigned_to),
                created_at,
                created_at,
                1,
                _json_dumps(target),
                _json_dumps(tags),
                created_by,
            ),
        )
        _insert_task_event(
            conn,
            task_id,
            "create",
            {
                "title": title,
                "priority": priority,
                "status": status,
                "assigned_to": assigned_to,
                "target": target,
                "tags": tags,
                "created_by": created_by,
            },
        )
        conn.commit()
    finally:
        conn.close()

    return get_task(task_id)


def list_tasks(
    status: Optional[str] = None,
    assigned_to: Optional[str] = None,
    priority: Optional[str] = None,
) -> list[TaskItem]:
    clauses = []
    params: list[str] = []

    if status:
        clauses.append("status = ?")
        params.append(_validate_status(status))

    if priority:
        clauses.append("priority = ?")
        params.append(_validate_priority(priority))

    where_clause = f"WHERE {' AND '.join(clauses)}" if clauses else ""
    conn = get_db_connection()
    try:
        rows = conn.execute(
            f"""
            SELECT * FROM terminal_tasks
            {where_clause}
            ORDER BY updated_at DESC, created_at DESC
            """,
            params,
        ).fetchall()
    finally:
        conn.close()

    tasks = [_row_to_task(row) for row in rows]
    if assigned_to:
        device_id = assigned_to.strip()
        tasks = [task for task in tasks if device_id in task.assigned_to]
    return tasks


def get_task(task_id: str) -> TaskItem:
    conn = get_db_connection()
    try:
        row = conn.execute(
            "SELECT * FROM terminal_tasks WHERE task_id = ?",
            (task_id,),
        ).fetchone()
        if not row:
            raise TaskNotFoundError(task_id)
        return _row_to_task(row)
    finally:
        conn.close()


def update_task(task_id: str, payload: TaskUpdateRequest) -> TaskItem:
    existing = get_task(task_id)
    patch = payload.dict(exclude_unset=True)
    if not patch:
        return existing

    next_values = existing.dict()
    if "title" in patch:
        title = str(patch["title"] or "").strip()
        if not title:
            raise ValueError("task title is required")
        next_values["title"] = title

    if "description" in patch:
        next_values["description"] = patch["description"] or ""

    if "priority" in patch:
        next_values["priority"] = _validate_priority(patch["priority"])

    if "status" in patch:
        next_values["status"] = _validate_status(patch["status"])

    if "assigned_to" in patch:
        assigned_to = _normalize_string_list(patch["assigned_to"])
        _ensure_devices_exist(assigned_to)
        next_values["assigned_to"] = assigned_to

    if "target" in patch:
        next_values["target"] = patch["target"] or {}

    if "tags" in patch:
        next_values["tags"] = _normalize_string_list(patch["tags"])

    next_values["revision"] = existing.revision + 1
    next_values["updated_at"] = _now_iso()

    conn = get_db_connection()
    try:
        conn.execute(
            """
            UPDATE terminal_tasks
            SET title = ?,
                description = ?,
                priority = ?,
                status = ?,
                assigned_to = ?,
                updated_at = ?,
                revision = ?,
                target_json = ?,
                tags_json = ?
            WHERE task_id = ?
            """,
            (
                next_values["title"],
                next_values["description"],
                next_values["priority"],
                next_values["status"],
                _json_dumps(next_values["assigned_to"]),
                next_values["updated_at"],
                next_values["revision"],
                _json_dumps(next_values["target"]),
                _json_dumps(next_values["tags"]),
                task_id,
            ),
        )
        _insert_task_event(conn, task_id, "update", patch)
        conn.commit()
    finally:
        conn.close()

    return get_task(task_id)


def assign_task(task_id: str, payload: TaskAssignRequest) -> TaskItem:
    existing = get_task(task_id)
    assigned_to = _normalize_string_list(payload.assigned_to)
    _ensure_devices_exist(assigned_to)

    next_status = existing.status
    if existing.status not in FINAL_TASK_STATUSES:
        next_status = "assigned" if assigned_to else "open"

    updated_at = _now_iso()
    revision = existing.revision + 1

    conn = get_db_connection()
    try:
        conn.execute(
            """
            UPDATE terminal_tasks
            SET assigned_to = ?,
                status = ?,
                updated_at = ?,
                revision = ?
            WHERE task_id = ?
            """,
            (_json_dumps(assigned_to), next_status, updated_at, revision, task_id),
        )
        _insert_task_event(
            conn,
            task_id,
            "assign",
            {
                "assigned_to": assigned_to,
                "status": next_status,
                "previous_status": existing.status,
            },
        )
        conn.commit()
    finally:
        conn.close()

    return get_task(task_id)


def pull_tasks_for_device(device_id: str) -> list[dict[str, Any]]:
    device_id = device_id.strip()
    _ensure_trusted_device(device_id)

    tasks = [
        task for task in list_tasks(assigned_to=device_id)
        if task.status not in FINAL_TASK_STATUSES
    ]

    return [
        {
            "task_id": task.task_id,
            "title": task.title,
            "description": task.description,
            "priority": task.priority,
            "status": task.status,
            "revision": task.revision,
            "target": task.target,
            "tags": task.tags,
            "updated_at": task.updated_at,
        }
        for task in tasks
    ]


def record_task_report(payload: TaskReportRequest) -> TaskItem:
    device_id = payload.device_id.strip()
    task_id = payload.task_id.strip()
    _ensure_trusted_device(device_id)
    existing = get_task(task_id)

    next_status = existing.status
    if payload.status:
        next_status = _validate_status(payload.status)

    updated_at = _now_iso()
    revision = existing.revision + 1

    report_payload = {
        "device_id": device_id,
        "task_id": task_id,
        "status": payload.status,
        "note": payload.note,
        "device_date": payload.device_date,
        "device_time": payload.device_time,
        "lat": payload.lat,
        "lon": payload.lon,
    }

    conn = get_db_connection()
    try:
        conn.execute(
            """
            INSERT INTO task_reports
            (
                task_id,
                device_id,
                status,
                note,
                device_date,
                device_time,
                lat,
                lon,
                created_at
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                task_id,
                device_id,
                payload.status or "",
                payload.note,
                payload.device_date,
                payload.device_time,
                payload.lat,
                payload.lon,
                updated_at,
            ),
        )
        conn.execute(
            """
            UPDATE terminal_tasks
            SET status = ?,
                updated_at = ?,
                revision = ?
            WHERE task_id = ?
            """,
            (next_status, updated_at, revision, task_id),
        )
        _insert_task_event(conn, task_id, "report", report_payload)
        conn.commit()
    finally:
        conn.close()

    return get_task(task_id)
