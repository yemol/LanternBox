"""LLM 客户端。只负责 Ollama 调用与流式输出，不承载业务逻辑。"""

import json
import requests
from typing import Any, Dict, List, Optional, Tuple

from ..config import OLLAMA_BASE_URL, OLLAMA_MODEL, SCENARIO_PROFILE


def call_ollama(
    messages: List[Dict[str, str]],
    model: Optional[str] = None,
    *,
    force_json: bool = False,
    temperature: float = 0.2,
    num_predict: int = 4000,
) -> str:
    try:
        model = model or OLLAMA_MODEL
        payload = {
            "model": model,
            "messages": messages,
            "stream": False,
            "options": {
                "temperature": temperature,
                "num_predict": num_predict,
            },
        }

        # Ollama 支持 format=json，可以显著降低“模型返回了说明文字而非 JSON”的概率。
        # 只在重排这类结构化任务中开启，普通聊天不受影响。
        if force_json:
            payload["format"] = "json"

        response = requests.post(
            f"{OLLAMA_BASE_URL}/api/chat",
            json=payload,
            timeout=120,
        )
        response.raise_for_status()
        data = response.json()
        return data.get("message", {}).get("content", "")
    except Exception as e:
        raise RuntimeError(f"Ollama 调用失败：{e}")


def stream_ollama(
    messages: List[Dict[str, str]],
    model: Optional[str] = None,
    *,
    temperature: float = 0.2,
    num_predict: int = 4000,
):
    try:
        model = model or OLLAMA_MODEL
        response = requests.post(
            f"{OLLAMA_BASE_URL}/api/chat",
            json={
                "model": model,
                "messages": messages,
                "stream": True,
                "options": {
                    "temperature": temperature,
                    "num_predict": num_predict,
                },
            },
            timeout=120,
            stream=True,
        )

        response.raise_for_status()

        for line in response.iter_lines():
            if not line:
                continue

            try:
                data = json.loads(line.decode("utf-8"))
                content = data.get("message", {}).get("content", "")
                if content:
                    yield content
                if data.get("done"):
                    break
            except Exception:
                continue
    except Exception as e:
        yield f"\n\n[本地 AI 流式输出失败：{e}]"
