"""FastAPI 路由层。负责 HTTP 接口、请求校验和调用 Pipeline / Services。"""

import json
import sqlite3
import uuid
from datetime import datetime
from typing import Any
import urllib.request

from fastapi import APIRouter, HTTPException
from fastapi.responses import FileResponse, Response, StreamingResponse

from .services.wiki_service import (
    get_wiki_categories_records,
    get_wiki_articles_by_category_records,
    search_wiki_articles,
)

from .config import (
    APP_DIR,
    BACKUP_DIR,
    DB_PATH,
    EMERGENCY_GUIDES_FILE,
    EMERGENCY_GUIDES_PATH,
    OLLAMA_BASE_URL,
    RESOURCE_CACHE_INFO,
    TTS_OUTPUT_DIR,
    VOICE_SERVICE_URL,
    load_runtime_settings,
    update_runtime_settings,
)

from .db import get_db_connection
from .models import AiAdviceRequest, AiRuntimeSettingsUpdate, InventoryItem, JournalEntry, TtsSpeakRequest
from .resources import load_local_resources
from .tts import cleanup_tts_output, synthesize_tts_to_file
from .utils import get_default_model_for_mode

from .wiki import (
    get_published_wiki_articles,
    get_wiki_article_detail,
)

from .pipeline.dispatcher import run_ai_pipeline, run_ai_stream_pipeline
from .pipeline.builder import build_pipeline_request
from .pipeline.postprocess import (
    build_ai_advice_response,
    build_fallback_pipeline_result,
)
from .pipeline.preload import prepare_ai_pipeline_context
from .llm.client import stream_ollama
from .response.fallback import build_fallback_answer
from .response.safety import sanitize_ai_answer
from .kiwix.fetcher import query_for_lookup as query_kiwix_for_lookup
from .kiwix.zim_client import (
    DEFAULT_MANIFEST_PATH,
    DEFAULT_ZIM_DIR,
    get_article as get_zim_article,
    get_resource as get_zim_resource,
    get_zim_manifest,
)

# 路由控制
router = APIRouter()


def _normalize_kiwix_source(source: str) -> str:
    source = str(source or "").strip()
    if source.lower().endswith(".zim"):
        return source[:-4]
    return source


def build_empty_ai_context(mode: str) -> dict[str, Any]:
    return {
        "context_data": {
            "engine": "companion_light" if mode == "companion" else "retrieval_v2_ai_orchestrated",
            "mode": mode,
            "related_guides": [],
            "related_wikis": [],
            "related_kiwix": [],
            "retrieval_v2": {},
        },
        "detected_domains": [],
        "related_guides": [],
        "related_wikis": [],
        "related_kiwix": [],
        "retrieval_v2": {},
    }


@router.get("/")
def home():
    return FileResponse(APP_DIR / "index.html")


@router.get("/inventory")
def inventory_page():
    return FileResponse(APP_DIR / "inventory.html")


@router.get("/api/status")
def status():
    return {
        "name": "LanternBox",
        "version": "0.4.0",
        "mode": "offline-ready",
        "message": "壳中灯已启动。",
    }




@router.get("/api/settings/ai")
def get_ai_runtime_settings():
    settings = load_runtime_settings()
    return {
        "ok": True,
        "settings": settings,
        "description": {
            "retrieval_engine": "当前主检索引擎为 retrieval_v2_ai_orchestrated",
            "show_retrieval_debug": "是否在接口返回中显示 v2 检索计划、候选来源和选中证据",
            "retrieval_v2_model": "Retrieval v2 规划和证据选择使用的本地模型",
        },
    }


@router.post("/api/settings/ai")
def update_ai_runtime_settings(payload: AiRuntimeSettingsUpdate):
    patch = payload.dict(exclude_unset=True)
    settings = update_runtime_settings(patch)
    return {
        "ok": True,
        "settings": settings,
        "message": "AI 检索设置已更新。",
    }


@router.get("/api/inventory")
def list_inventory():
    conn = get_db_connection()
    rows = conn.execute("SELECT * FROM inventory ORDER BY id DESC").fetchall()
    conn.close()
    return [dict(row) for row in rows]


@router.post("/api/inventory")
def add_inventory(item: InventoryItem):
    conn = get_db_connection()
    cursor = conn.execute(
        """
        INSERT INTO inventory
        (name, category, quantity, unit, expire_date, note)
        VALUES (?, ?, ?, ?, ?, ?)
        """,
        (
            item.name,
            item.category,
            item.quantity,
            item.unit,
            item.expire_date,
            item.note,
        ),
    )

    new_id = cursor.lastrowid
    item_code = f"LB-{new_id:05d}"

    conn.execute(
        "UPDATE inventory SET item_code = ? WHERE id = ?",
        (item_code, new_id),
    )

    conn.commit()
    conn.close()

    return {
        "ok": True,
        "id": new_id,
        "item_code": item_code,
        "message": "物资已记录。",
    }


@router.delete("/api/inventory/{item_id}")
def delete_inventory(item_id: int):
    conn = get_db_connection()
    cursor = conn.execute(
        "DELETE FROM inventory WHERE id = ?",
        (item_id,),
    )
    conn.commit()
    deleted_count = cursor.rowcount
    conn.close()

    if deleted_count == 0:
        return {
            "ok": False,
            "message": "没有找到这条物资记录。",
        }

    return {
        "ok": True,
        "message": "物资已删除。",
    }


@router.post("/api/backup")
def backup_database():
    if not DB_PATH.exists():
        return {
            "ok": False,
            "message": "数据库文件不存在，无法备份。",
        }

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = BACKUP_DIR / f"lanternbox_{timestamp}.db"

    source = sqlite3.connect(DB_PATH)
    target = sqlite3.connect(backup_path)

    try:
        source.backup(target)
    finally:
        target.close()
        source.close()

    return {
        "ok": True,
        "message": "数据库备份成功。",
        "file": backup_path.name,
    }


@router.get("/journal")
def journal_page():
    return FileResponse(APP_DIR / "journal.html")


@router.get("/api/journal")
def list_journal():
    conn = get_db_connection()
    rows = conn.execute("SELECT * FROM journal ORDER BY id DESC").fetchall()
    conn.close()
    return [dict(row) for row in rows]


@router.post("/api/journal")
def add_journal(entry: JournalEntry):
    created_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    conn = get_db_connection()
    cursor = conn.execute(
        """
        INSERT INTO journal
        (entry_type, title, content, created_at)
        VALUES (?, ?, ?, ?)
        """,
        (
            entry.entry_type,
            entry.title,
            entry.content,
            created_at,
        ),
    )
    conn.commit()
    new_id = cursor.lastrowid
    conn.close()

    return {
        "ok": True,
        "id": new_id,
        "created_at": created_at,
        "message": "日记已记录。",
    }


@router.delete("/api/journal/{entry_id}")
def delete_journal(entry_id: int):
    conn = get_db_connection()
    cursor = conn.execute(
        "DELETE FROM journal WHERE id = ?",
        (entry_id,),
    )
    conn.commit()
    deleted_count = cursor.rowcount
    conn.close()

    if deleted_count == 0:
        return {
            "ok": False,
            "message": "没有找到这条日记记录。",
        }

    return {
        "ok": True,
        "message": "日记已删除。",
    }


@router.get("/emergency")
def emergency_page():
    return FileResponse(APP_DIR / "emergency.html")


@router.get("/api/emergency-guides")
def list_emergency_guides():
    if not EMERGENCY_GUIDES_PATH.exists():
        return []

    with open(EMERGENCY_GUIDES_PATH, "r", encoding="utf-8") as file:
        return json.load(file)


@router.get("/api/emergency")
def get_emergency_guides():
    if not EMERGENCY_GUIDES_FILE.exists():
        raise HTTPException(status_code=404, detail="emergency_guides.json not found")

    try:
        with open(EMERGENCY_GUIDES_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        raise HTTPException(status_code=500, detail=f"JSON 格式错误：{e}")


@router.post("/api/ai/advice")
def ai_advice(payload: AiAdviceRequest):
    user_message = payload.message.strip()
    mode = payload.mode or "emergency"
    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    if mode == "companion":
        prepared_ai = build_empty_ai_context(mode)
    else:
        prepared_ai = prepare_ai_pipeline_context(
            user_message=user_message,
            mode=mode,
            history=payload.history,
            conversation_summary=payload.conversation_summary or "",
        )

    context_data = prepared_ai["context_data"]
    detected_domains = prepared_ai["detected_domains"]
    related_guides = prepared_ai["related_guides"]
    related_wikis = prepared_ai["related_wikis"]
    related_kiwix = prepared_ai.get("related_kiwix", [])
    retrieval_v2 = prepared_ai["retrieval_v2"]

    pipeline_result = None

    if payload.metadata_only:
        pipeline_result = build_fallback_pipeline_result(
            mode=mode,
            answer="",
            reason="metadata_only",
        )
    else:
        pipeline_request = build_pipeline_request(
            payload,
            stream=False,
            related_guides=related_guides,
            related_wikis=related_wikis,
            related_kiwix=related_kiwix,
            detected_domains=detected_domains,
            context_data=context_data,
            retrieval_v2=retrieval_v2,
        )

        try:
            pipeline_result = run_ai_pipeline(pipeline_request)
            answer = sanitize_ai_answer(pipeline_result.answer, mode)
        except Exception:
            answer = build_fallback_answer(mode, related_guides)

            pipeline_result = build_fallback_pipeline_result(
                mode=mode,
                answer=answer,
                reason="pipeline_error",
            )


    return build_ai_advice_response(
        result=pipeline_result,
        mode=mode,
        related_guides=related_guides,
        related_wikis=related_wikis,
        related_kiwix=related_kiwix,
        detected_domains=detected_domains,
        context_data=context_data,
        retrieval_v2=retrieval_v2,
    )


@router.post("/api/ai/advice/stream")
def ai_advice_stream(payload: AiAdviceRequest):
    user_message = payload.message.strip()
    mode = payload.mode or "emergency"
    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    if mode == "companion":
        prepared_ai = build_empty_ai_context(mode)
    else:
        prepared_ai = prepare_ai_pipeline_context(
            user_message=user_message,
            mode=mode,
            history=payload.history,
            conversation_summary=payload.conversation_summary or "",
        )

    context_data = prepared_ai["context_data"]
    detected_domains = prepared_ai["detected_domains"]
    related_guides = prepared_ai["related_guides"]
    related_wikis = prepared_ai["related_wikis"]
    related_kiwix = prepared_ai.get("related_kiwix", [])
    retrieval_v2 = prepared_ai["retrieval_v2"]

    pipeline_request = build_pipeline_request(
        payload,
        stream=True,
        related_guides=related_guides,
        related_wikis=related_wikis,
        related_kiwix=related_kiwix,
        detected_domains=detected_domains,
        context_data=context_data,
        retrieval_v2=retrieval_v2,
    )

    pipeline_request.retrieval_v2 = retrieval_v2
    pipeline_request.metadata["retrieval_v2"] = retrieval_v2

    pipeline_stream = run_ai_stream_pipeline(pipeline_request)
    messages = pipeline_stream["messages"]
    def event_generator():
        model = get_default_model_for_mode(mode)
        for chunk in stream_ollama(messages, model=model):
            yield chunk

    return StreamingResponse(event_generator(), media_type="text/plain; charset=utf-8")


@router.get("/assistant.html")
def assistant_page():
    return FileResponse(APP_DIR / "assistant.html")

@router.get("/library.html")
def library_page():
    return FileResponse(APP_DIR / "library.html")

@router.get("/kiwix.html")
def kiwix_page():
    return FileResponse(APP_DIR / "kiwix.html")

@router.get("/status.html")
def status_page():
    return FileResponse(APP_DIR / "status.html")

@router.get("/terminals.html")
def terminals_page():
    return FileResponse(APP_DIR / "terminals.html")

@router.get("/tasks.html")
def tasks_page():
    return FileResponse(APP_DIR / "tasks.html")

@router.post("/api/resources/reload")
def reload_resources():
    load_local_resources()
    return {
        "ok": True,
        "message": "本地资源缓存已刷新",
        "cache": RESOURCE_CACHE_INFO,
    }


@router.get("/api/resources/status")
def resources_status():
    return {
        "ok": True,
        "cache": RESOURCE_CACHE_INFO,
    }


@router.post("/api/tts/speak")
def tts_speak(payload: TtsSpeakRequest):
    """把朗读请求转交给独立语音服务，并缓存生成的音频。"""
    text = (payload.text or "").strip()

    if not text:
        raise HTTPException(status_code=400, detail="请提供需要朗读的文本。")

    TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    cleanup_tts_output()

    output_filename = f"tts_{uuid.uuid4().hex}.wav"
    output_path = TTS_OUTPUT_DIR / output_filename

    synthesize_tts_to_file(
        text=text,
        output_path=output_path,
        mode=payload.mode or "default",
    )

    return {
        "ok": True,
        "engine": "voice_service",
        "audio_url": f"/tts_output/{output_filename}",
        "filename": output_filename,
        "voice_service_url": VOICE_SERVICE_URL,
    }

@router.get("/api/wiki/articles")
def api_wiki_articles():
    articles = get_published_wiki_articles(limit=50)

    return {
        "ok": True,
        "total": len(articles),
        "articles": articles,
    }


@router.get("/api/wiki/articles/{article_id}")
def api_wiki_article_detail(article_id: str):
    detail = get_wiki_article_detail(article_id)

    if not detail:
        return {
            "ok": False,
            "error": "Wiki 文章不存在或读取失败",
        }

    return {
        "ok": True,
        "article": detail["article"],
        "media": detail["media"],
    }


@router.get("/api/wiki/search")
def api_wiki_search(q: str = ""):
    keyword = q.strip()
    articles = search_wiki_articles(keyword, limit=20)

    return {
        "ok": True,
        "query": keyword,
        "total": len(articles),
        "articles": articles,
    }


@router.get("/api/kiwix/status")
def api_kiwix_status():
    zim_files = sorted(DEFAULT_ZIM_DIR.glob("*.zim")) if DEFAULT_ZIM_DIR.exists() else []
    manifest_entries = {item.get("filename"): item for item in get_zim_manifest()}
    normalized_files = [
        {
            "name": path.name,
            "size_bytes": path.stat().st_size,
            "language": manifest_entries.get(path.name, {}).get("language"),
            "topic": manifest_entries.get(path.name, {}).get("topic"),
            "variant": manifest_entries.get(path.name, {}).get("variant"),
            "role": manifest_entries.get(path.name, {}).get("role"),
            "usage_policy": manifest_entries.get(path.name, {}).get("usage_policy"),
        }
        for path in zim_files
    ]
    groups: dict[str, list[dict[str, Any]]] = {}
    for item in normalized_files:
        key = " / ".join(
            part
            for part in [
                item.get("role") or "unknown",
                item.get("language") or "unknown",
                item.get("variant") or "unknown",
            ]
            if part
        )
        groups.setdefault(key, []).append(item)

    return {
        "ok": True,
        "zim_available": bool(zim_files),
        "manifest_available": DEFAULT_MANIFEST_PATH.exists(),
        "summary": {
            "total": len(normalized_files),
            "decision": sum(1 for item in normalized_files if item.get("role") == "decision"),
            "lookup": sum(1 for item in normalized_files if item.get("role") == "lookup"),
            "support": sum(1 for item in normalized_files if item.get("role") == "support"),
            "fallback": sum(1 for item in normalized_files if item.get("role") == "fallback"),
            "zh": sum(1 for item in normalized_files if item.get("language") == "zh"),
            "en": sum(1 for item in normalized_files if item.get("language") == "en"),
        },
        "groups": groups,
        "zim_files": normalized_files,
    }


@router.get("/api/kiwix/search")
def api_kiwix_search(q: str = ""):
    keyword = q.strip()
    zim_hits = query_kiwix_for_lookup(keyword, limit=8) if keyword else []
    results = [
        {
            "title": item.title,
            "snippet": item.snippet,
            "source": item.source,
            "relevance_score": item.relevance_score,
            "topics": item.topics,
            "url": item.url,
            "zim_filename": item.zim_filename,
            "language": item.language,
            "role": item.role,
            "usage_policy": item.usage_policy,
            "article_path": item.article_path,
            "matched_terms": item.matched_terms,
            "matched_terms_count": item.matched_terms_count,
            "content_type": item.content_type,
        }
        for item in zim_hits
    ]

    return {
        "ok": True,
        "query": keyword,
        "total": len(results),
        "results": results,
    }


@router.get("/api/kiwix/article")
def api_kiwix_article(source: str = "", path: str = "", title: str = ""):
    source = _normalize_kiwix_source(source)
    article_key = (path or title).strip()
    if not source or not article_key:
        raise HTTPException(status_code=400, detail="缺少 ZIM 来源或文章路径")

    article = get_zim_article(article_key, source=source)
    if not article:
        raise HTTPException(status_code=404, detail="未能打开该 ZIM 文章")

    return {
        "ok": True,
        "source": source,
        "title": article.get("title"),
        "path": article.get("path"),
        "url": article.get("url"),
        "content_type": article.get("content_type") or "text/html",
        "html": article.get("html") or "",
        "text": article.get("text") or article.get("content") or "",
        # Backward-compatible alias for any old frontend/AI caller still reading content.
        "content": article.get("text") or article.get("content") or "",
    }


@router.get("/api/kiwix/resource")
def api_kiwix_resource(source: str = "", path: str = ""):
    source = _normalize_kiwix_source(source)
    resource_path = path.strip()
    if not source or not resource_path:
        raise HTTPException(status_code=400, detail="缺少 ZIM 来源或资源路径")

    resource = get_zim_resource(resource_path, source=source)
    if not resource:
        raise HTTPException(status_code=404, detail="未能打开该 ZIM 资源")

    return Response(
        content=resource.get("content") or b"",
        media_type=resource.get("content_type") or "application/octet-stream",
    )


@router.get("/api/kiwix/asset")
def api_kiwix_asset(source: str = "", path: str = ""):
    return api_kiwix_resource(source=source, path=path)


# 自检接口
def check_url(url: str, timeout: float = 2.0) -> dict:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            return {
                "ok": True,
                "status": response.status,
                "message": "可连接"
            }
    except Exception as error:
        return {
            "ok": False,
            "status": None,
            "message": str(error)
        }
    
@router.get("/api/system/check")
def system_check():
    checks = []

    checks.append({
        "id": "backend",
        "title": "FastAPI 后端",
        "ok": True,
        "message": "后端服务在线"
    })

    try:
        emergency_data = get_emergency_guides()
        checks.append({
            "id": "emergency",
            "title": "应急指南",
            "ok": True,
            "message": f"可用，当前 {len(emergency_data)} 条指南"
        })
    except Exception as error:
        checks.append({
            "id": "emergency",
            "title": "应急指南",
            "ok": False,
            "message": f"读取失败：{error}"
        })

    try:
        inventory_items = list_inventory()
        checks.append({
            "id": "inventory",
            "title": "物资库存",
            "ok": True,
            "message": f"可用，当前 {len(inventory_items)} 条物资"
        })
    except Exception as error:
        checks.append({
            "id": "inventory",
            "title": "物资库存",
            "ok": False,
            "message": f"读取失败：{error}"
        })

    try:
        journal_entries = list_journal()
        checks.append({
            "id": "journal",
            "title": "日记记录",
            "ok": True,
            "message": f"可用，当前 {len(journal_entries)} 条记录"
        })
    except Exception as error:
        checks.append({
            "id": "journal",
            "title": "日记记录",
            "ok": False,
            "message": f"读取失败：{error}"
        })

    try:
        wiki_articles = get_published_wiki_articles(limit=50)
        checks.append({
            "id": "wiki",
            "title": "精选 Wiki",
            "ok": True,
            "message": f"可用，当前 {len(wiki_articles)} 篇 Wiki"
        })
    except Exception as error:
        checks.append({
            "id": "wiki",
            "title": "精选 Wiki",
            "ok": False,
            "message": f"读取失败：{error}"
        })

    ollama = check_url(f"{OLLAMA_BASE_URL}/api/tags")
    checks.append({
        "id": "ollama",
        "title": "Ollama 本地 AI",
        "ok": ollama["ok"],
        "message": "可用" if ollama["ok"] else f"不可用：{ollama['message']}"
    })

    voice_service = check_url(f"{VOICE_SERVICE_URL}/api/voice/health")
    checks.append({
        "id": "voice_service",
        "title": "独立语音服务",
        "ok": voice_service["ok"],
        "message": "可用" if voice_service["ok"] else f"不可用：{voice_service['message']}"
    })

    checks.append({
        "id": "tts_output",
        "title": "TTS 缓存目录",
        "ok": TTS_OUTPUT_DIR.exists(),
        "message": f"缓存目录可用：{TTS_OUTPUT_DIR}" if TTS_OUTPUT_DIR.exists() else f"缓存目录不存在：{TTS_OUTPUT_DIR}"
    })

    return {
        "ok": all(item["ok"] for item in checks),
        "checked_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "checks": checks
    }

@router.get("/api/wiki/categories")
def get_wiki_categories():
    return get_wiki_categories_records()


@router.get("/api/wiki/categories/{category_id}/articles")
def get_wiki_articles_by_category(
    category_id: str,
    page: int = 1,
    per_page: int = 10,
):
    return get_wiki_articles_by_category_records(
        category_id,
        page=page,
        per_page=per_page,
    )

@router.get("/wiki-categories.html")
def wiki_categories_page():
    return FileResponse(APP_DIR / "wiki_categories.html")

@router.get("/wiki-admin-lite.html")
def wiki_admin_lite_page():
    return FileResponse(APP_DIR / "wiki_admin_lite.html")
