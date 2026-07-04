"""Kiwix client interface stub.

This module intentionally does not connect to real ZIM files or any external
service. It exists as the future adapter boundary for a real Kiwix backend.
"""

from typing import Dict, List, Optional

from api.kiwix.schema import KiwixResult


class KiwixClient:
    """Placeholder for a future Kiwix/ZIM-backed client."""

    def __init__(self, zim_path: Optional[str] = None) -> None:
        self.zim_path = zim_path

    def search(self, query: str, context: Optional[Dict] = None) -> List[KiwixResult]:
        """Return no remote/ZIM results in the stub implementation."""
        return []
