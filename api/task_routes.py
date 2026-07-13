"""Core task system API routes."""

from typing import Optional

from fastapi import APIRouter, HTTPException

from .models import (
    TaskAssignRequest,
    TaskCreateRequest,
    TaskItem,
    TaskMutationResponse,
    TaskReportRequest,
    TaskUpdateRequest,
)
from .services.task_service import (
    DeviceNotFoundError,
    DeviceNotTrustedError,
    TaskNotFoundError,
    assign_task,
    create_task,
    get_task,
    list_tasks,
    pull_tasks_for_device,
    record_task_report,
    update_task,
)

router = APIRouter(prefix="/api/tasks", tags=["tasks"])


def _bad_request(error: Exception) -> HTTPException:
    return HTTPException(status_code=400, detail=str(error))


def _task_not_found() -> HTTPException:
    return HTTPException(status_code=404, detail="task not found")


def _device_not_found(error: DeviceNotFoundError) -> HTTPException:
    return HTTPException(status_code=404, detail=f"terminal device not found: {error}")


@router.post("", response_model=TaskItem)
def create_task_api(payload: TaskCreateRequest):
    try:
        return create_task(payload)
    except DeviceNotFoundError as error:
        raise HTTPException(status_code=400, detail=f"unknown terminal device: {error}")
    except ValueError as error:
        raise _bad_request(error)


@router.get("", response_model=list[TaskItem])
def list_tasks_api(
    status: Optional[str] = None,
    assigned_to: Optional[str] = None,
    priority: Optional[str] = None,
):
    try:
        return list_tasks(status=status, assigned_to=assigned_to, priority=priority)
    except ValueError as error:
        raise _bad_request(error)


@router.get("/pull/{device_id}")
def pull_tasks_api(device_id: str):
    try:
        return {
            "ok": True,
            "device_id": device_id,
            "tasks": pull_tasks_for_device(device_id),
        }
    except DeviceNotFoundError as error:
        raise _device_not_found(error)
    except DeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")


@router.post("/report", response_model=TaskMutationResponse)
def report_task_api(payload: TaskReportRequest):
    try:
        task = record_task_report(payload)
        return {
            "ok": True,
            "task_id": task.task_id,
            "revision": task.revision,
        }
    except DeviceNotFoundError as error:
        raise _device_not_found(error)
    except DeviceNotTrustedError as error:
        raise HTTPException(status_code=403, detail=f"terminal device is not trusted: {error}")
    except TaskNotFoundError:
        raise _task_not_found()
    except ValueError as error:
        raise _bad_request(error)


@router.get("/{task_id}", response_model=TaskItem)
def get_task_api(task_id: str):
    try:
        return get_task(task_id)
    except TaskNotFoundError:
        raise _task_not_found()


@router.patch("/{task_id}", response_model=TaskItem)
def update_task_api(task_id: str, payload: TaskUpdateRequest):
    try:
        return update_task(task_id, payload)
    except DeviceNotFoundError as error:
        raise HTTPException(status_code=400, detail=f"unknown terminal device: {error}")
    except TaskNotFoundError:
        raise _task_not_found()
    except ValueError as error:
        raise _bad_request(error)


@router.post("/{task_id}/assign", response_model=TaskItem)
def assign_task_api(task_id: str, payload: TaskAssignRequest):
    try:
        return assign_task(task_id, payload)
    except DeviceNotFoundError as error:
        raise HTTPException(status_code=400, detail=f"unknown terminal device: {error}")
    except TaskNotFoundError:
        raise _task_not_found()
    except ValueError as error:
        raise _bad_request(error)
