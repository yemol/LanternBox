import requests
from typing import Any, Dict, Optional, List
from .config import POCKETBASE_URL


def pocketbase_get_records(collection: str, params: Optional[dict] = None) -> dict:
    url = f"{POCKETBASE_URL}/api/collections/{collection}/records"

    try:
        response = requests.get(url, params=params or {}, timeout=8)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"PocketBase 读取失败: {collection}", e)
        return {
            "page": 1,
            "perPage": 0,
            "totalItems": 0,
            "totalPages": 0,
            "items": []
        }
