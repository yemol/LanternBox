import json
import sqlite3
import uuid
from datetime import datetime
from typing import Any
import urllib.request
import urllib.error

from fastapi import APIRouter, HTTPException
from fastapi.responses import FileResponse, StreamingResponse

from .ai import (
    build_ai_messages,
    build_fallback_answer,
    call_ollama,
    sanitize_ai_answer,
    filter_and_rank_ai_references,
    rerank_candidates_with_local_ai,
    stream_ollama,
)
from .config import (
    APP_DIR,
    BACKUP_DIR,
    DB_PATH,
    EMERGENCY_GUIDES_FILE,
    EMERGENCY_GUIDES_PATH,
    RESOURCE_CACHE_INFO,
    TTS_OUTPUT_DIR,
    VOICE_SERVICE_URL,
    load_runtime_settings,
    update_runtime_settings,
)
from .db import get_db_connection
from .models import AiAdviceRequest, AiRuntimeSettingsUpdate, InventoryItem, JournalEntry, TtsSpeakRequest
from .resources import load_local_resources, prepare_ai_context, serialize_related_guides
from .tts import cleanup_tts_output, synthesize_tts_to_file
from .utils import get_default_model_for_mode

from .wiki import (
    get_published_wiki_articles,
    get_wiki_article_detail,
    search_wiki_articles,
    search_wiki_for_ai,
    pb_get,
)

router = APIRouter()


def filter_related_wikis_for_query(related_wikis, query_profile):
    """v0.7：按 query intent 轻量过滤 Wiki 噪声。
    这里只做标题级别收敛，不影响指南召回。
    """
    if not related_wikis:
        return related_wikis

    intents = set(query_profile.get("intents", []) if query_profile else [])

    def title_of(item):
        return str(item.get("title", ""))

    if "medical_dehydration_diarrhea" in intents:
        bad = ["停水后的用水优先级", "简易过滤", "煮沸饮水", "水质异常", "储水容器"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    if "power_water_electrical_safety" in intents:
        bad = ["冰箱食物", "食物变质", "干粮", "罐头", "雨水收集", "饮用水"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    if "shelter_damp_mold" in intents:
        bad = ["洪水", "撤离", "撤高", "干粮", "罐头"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    return related_wikis


def apply_ai_rerank_if_enabled(user_message, context_data, related_guides):
    """根据运行时设置决定是否让本地 AI 重排候选来源。

    注意：环境变量只适合作为启动默认值；界面开关写入 data/runtime_settings.json，
    这里每次请求读取运行时设置，因而无需重启后端。
    """
    runtime_settings = load_runtime_settings()
    enable_ai_rerank = bool(runtime_settings.get("ai_rerank_enabled"))
    rerank_model = runtime_settings.get("ai_rerank_model") or "qwen2.5:3b"

    candidates = context_data.get("candidate_sources") or []
    rerank_result = rerank_candidates_with_local_ai(
        user_message=user_message,
        candidates=candidates,
        query_profile=context_data.get("query_profile", {}),
        model=rerank_model,
        enable_ai_rerank=enable_ai_rerank,
        max_selected=6,
    )

    selected_sources = rerank_result.get("selected_sources") or context_data.get("selected_sources") or []
    excluded_sources = rerank_result.get("excluded_sources") or context_data.get("excluded_sources") or []

    def source_raw_key(source):
        # v0.6.1 的 candidate_source 使用 source_id / candidate_id，不再提供 raw_id。
        # 这里兼容旧字段，避免 AI 重排成功后因为字段不匹配而无法重排 related_guides。
        if not isinstance(source, dict):
            return None
        return (
            source.get("raw_id")
            or source.get("source_id")
            or str(source.get("candidate_id") or "").split(":", 1)[-1]
            or source.get("title")
        )

    selected_guide_ids = {
        source_raw_key(item)
        for item in selected_sources
        if item.get("source_type") == "guide" and source_raw_key(item)
    }

    if selected_guide_ids:
        guide_by_id = {
            (guide.get("id") or guide.get("title")): guide
            for guide in related_guides
        }
        ordered_guides = []
        seen = set()
        for source in selected_sources:
            if source.get("source_type") != "guide":
                continue
            raw_id = source_raw_key(source)
            guide = guide_by_id.get(raw_id)
            if guide and raw_id not in seen:
                # 把 AI 重排原因带回前端，来源卡片可直接显示。
                if source.get("rerank_reason") or source.get("reason"):
                    guide = dict(guide)
                    guide["_ai_rerank_reason"] = source.get("rerank_reason") or source.get("reason")
                    guide["_ai_rerank_mode"] = source.get("rerank_mode") or rerank_result.get("mode")
                ordered_guides.append(guide)
                seen.add(raw_id)
        # 如果 AI 只选中了部分指南，保留规则排序中的其他指南作为兜底尾巴。
        for guide in related_guides:
            raw_id = guide.get("id") or guide.get("title")
            if raw_id not in seen:
                ordered_guides.append(guide)
                seen.add(raw_id)
        related_guides = ordered_guides[:10]

    retrieval_decision = dict(context_data.get("retrieval_decision") or {})
    retrieval_decision.update({
        "runtime_settings": runtime_settings,
        "rerank_result": {
            "version": rerank_result.get("version"),
            "mode": rerank_result.get("mode"),
            "used_ai": rerank_result.get("used_ai", False),
            "fallback_reason": rerank_result.get("fallback_reason"),
            "intent_summary": rerank_result.get("intent_summary"),
            "answer_focus": rerank_result.get("answer_focus", []),
            "rejected_candidate_ids": rerank_result.get("rejected_candidate_ids", []),
            "error": rerank_result.get("error"),
            "selected_candidate_ids": rerank_result.get("selected_candidate_ids", []),
        },
    })

    return {
        "related_guides": related_guides,
        "selected_sources": selected_sources,
        "excluded_sources": excluded_sources,
        "retrieval_decision": retrieval_decision,
        "runtime_settings": runtime_settings,
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
            "ai_rerank_enabled": "是否启用本地 AI 对候选来源进行重排",
            "ai_rerank_model": "用于重排的 Ollama 模型",
            "retrieval_mode": "rule=规则召回，hybrid=规则召回+AI重排，enhanced=预留增强模式",
            "show_retrieval_debug": "是否在接口返回中显示检索判断、选中来源和排除来源",
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
    model = payload.model or get_default_model_for_mode(mode)

    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    context_data = prepare_ai_context(user_message, mode)
    detected_domains = context_data["detected_domains"]
    matched_triggers = context_data["matched_triggers"]
    related_guides = context_data["related_guides"]

    related_wikis = search_wiki_for_ai(
        payload.message,
        detected_domains=context_data.get("detected_domains", []),
        limit=6,
    )

    # print("AI ADVICE DEBUG user_message:", user_message)
    # print("AI ADVICE DEBUG detected_domains:", detected_domains)
    # print("AI ADVICE DEBUG raw guides:", [g.get("title") for g in related_guides])

    ranked_references = filter_and_rank_ai_references(
        user_message=user_message,
        related_guides=related_guides,
        related_wikis=related_wikis,
        detected_domains=detected_domains,
    )
    related_guides = ranked_references["guides"]
    related_wikis = ranked_references["wikis"]
    related_wikis = filter_related_wikis_for_query(
        related_wikis,
        context_data.get("query_profile", {}),
    )

    rerank_state = apply_ai_rerank_if_enabled(user_message, context_data, related_guides)
    related_guides = rerank_state["related_guides"]

    # print("AI ADVICE DEBUG filtered guides:", [g.get("title") for g in related_guides])

    if payload.metadata_only:
        answer = ""
    else:
        messages = build_ai_messages(
            user_message=user_message,
            mode=mode,
            matched_triggers=matched_triggers,
            related_guides=related_guides,
            detected_domains=detected_domains,
            history=payload.history,
            related_wikis=related_wikis,
        )

        try:
            answer = call_ollama(messages=messages, model=model)
            answer = sanitize_ai_answer(answer, mode)
        except Exception as e:
            print("Ollama 调用失败，返回本地规则建议：", e)
            answer = sanitize_ai_answer(build_fallback_answer(mode, matched_triggers, related_guides), mode)

    return {
        "ok": True,
        "mode": mode,
        "message": user_message,
        "detected_domains": detected_domains,
        "answer": answer,
        "matched_triggers": [
            {
                "id": t.get("id"),
                "title": t.get("title"),
                "category": t.get("category"),
                "severity": t.get("severity"),
                "reminder_type": t.get("reminder_type"),
                "matchScore": t.get("matchScore"),
                "should_log": t.get("should_log"),
                "should_create_task": t.get("should_create_task"),
            }
            for t in matched_triggers
        ],
        "related_guides": serialize_related_guides(related_guides),
        "related_wikis": related_wikis,
        "candidate_sources": context_data.get("candidate_sources", []),
        "selected_sources": rerank_state.get("selected_sources", []),
        "excluded_sources": rerank_state.get("excluded_sources", []),
        "retrieval_decision": rerank_state.get("retrieval_decision", {}),
        "runtime_settings": rerank_state.get("runtime_settings", {}),
    }


@router.post("/api/ai/advice/stream")
def ai_advice_stream(payload: AiAdviceRequest):
    user_message = payload.message.strip()
    mode = payload.mode or "emergency"
    model = payload.model or get_default_model_for_mode(mode)

    if mode not in {"emergency", "companion"}:
        mode = "emergency"

    if not user_message:
        raise HTTPException(status_code=400, detail="请提供需要 AI 判断的情况描述。")

    context_data = prepare_ai_context(user_message, mode)
    detected_domains = context_data["detected_domains"]
    related_guides = context_data["related_guides"]

    related_wikis = search_wiki_for_ai(
        payload.message,
        detected_domains=detected_domains,
        limit=6,
    )

    ranked_references = filter_and_rank_ai_references(
        user_message=user_message,
        related_guides=related_guides,
        related_wikis=related_wikis,
        detected_domains=detected_domains,
    )
    related_guides = ranked_references["guides"]
    related_wikis = ranked_references["wikis"]
    related_wikis = filter_related_wikis_for_query(
        related_wikis,
        context_data.get("query_profile", {}),
    )

    rerank_state = apply_ai_rerank_if_enabled(user_message, context_data, related_guides)
    related_guides = rerank_state["related_guides"]

    messages = build_ai_messages(
        user_message=user_message,
        mode=mode,
        matched_triggers=context_data["matched_triggers"],
        related_guides=related_guides,
        detected_domains=detected_domains,
        history=payload.history,
        related_wikis=related_wikis,
    )

    def event_generator():
        for chunk in stream_ollama(messages, model=model):
            yield chunk

    return StreamingResponse(event_generator(), media_type="text/plain; charset=utf-8")


@router.get("/assistant.html")
def assistant_page():
    return FileResponse(APP_DIR / "assistant.html")

@router.get("/library.html")
def library_page():
    return FileResponse(APP_DIR / "library.html")

@router.get("/status.html")
def status_page():
    return FileResponse(APP_DIR / "status.html")

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


# 这里开始的都是自检用到的函数和借口
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

    ollama = check_url("http://127.0.0.1:11434/api/tags")
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
    params = {
        "page": 1,
        "perPage": 100,
        "sort": "sort_order,name"
    }

    return pb_get("/api/collections/wiki_categories/records", params=params)


@router.get("/api/wiki/categories/{category_id}/articles")
def get_wiki_articles_by_category(
    category_id: str,
    page: int = 1,
    per_page: int = 10
):
    page = max(page, 1)
    per_page = min(max(per_page, 1), 50)

    params = {
        "page": page,
        "perPage": per_page,
        "sort": "title",
        "filter": f'status = "published" && category = "{category_id}"'
    }

    return pb_get("/api/collections/wiki_articles/records", params=params)

@router.get("/wiki-categories.html")
def wiki_categories_page():
    return FileResponse(APP_DIR / "wiki_categories.html")

@router.get("/wiki-admin-lite.html")
def wiki_admin_lite_page():
    return FileResponse(APP_DIR / "wiki_admin_lite.html")