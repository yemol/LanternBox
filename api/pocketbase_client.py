import requests
from typing import Any, Dict, Optional, List
from .config import POCKETBASE_URL

from typing import Any, Dict, Optional
import requests
from fastapi import HTTPException

from .config import POCKETBASE_URL


def pb_get(path: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    url = f"{POCKETBASE_URL}{path}"

    try:
        response = requests.get(url, params=params or {}, timeout=10)
        response.raise_for_status()
        return response.json()
    except requests.RequestException as exc:
        raise HTTPException(
            status_code=502,
            detail=f"PocketBase 请求失败：{exc}",
        )


def pocketbase_get_records(
    collection: str,
    params: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    return pb_get(
        f"/api/collections/{collection}/records",
        params=params,
    )


def pocketbase_get_record(
    collection: str,
    record_id: str,
) -> Dict[str, Any]:
    return pb_get(
        f"/api/collections/{collection}/records/{record_id}",
    )