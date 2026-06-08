import json
import sqlite3
import subprocess
import uuid
from datetime import datetime
from typing import Any

from fastapi import APIRouter, HTTPException
from fastapi.responses import FileResponse, StreamingResponse

from .ai import (
    build_ai_messages,
    build_fallback_answer,
    call_ollama,
    stream_ollama,
)
from .config import (
    APP_DIR,
    BACKUP_DIR,
    DB_PATH,
    EMERGENCY_GUIDES_FILE,
    EMERGENCY_GUIDES_PATH,
    PIPER_MODEL_PATH,
    RESOURCE_CACHE_INFO,
    TTS_OUTPUT_DIR,
)
from .db import get_db_connection
from .models import AiAdviceRequest, InventoryItem, JournalEntry, TtsSpeakRequest
from .resources import load_local_resources, prepare_ai_context, serialize_related_guides
from .tts import cleanup_tts_output, run_melotts_tts, run_piper_tts
from .utils import get_default_model_for_mode, get_tts_engine

router = APIRouter()


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
        )

        try:
            answer = call_ollama(messages=messages, model=model)
        except Exception as e:
            print("Ollama 调用失败，返回本地规则建议：", e)
            answer = build_fallback_answer(mode, matched_triggers, related_guides)

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
    messages = build_ai_messages(
        user_message=user_message,
        mode=mode,
        matched_triggers=context_data["matched_triggers"],
        related_guides=context_data["related_guides"],
        detected_domains=context_data["detected_domains"],
        history=payload.history,
    )

    def event_generator():
        for chunk in stream_ollama(messages, model=model):
            yield chunk

    return StreamingResponse(event_generator(), media_type="text/plain; charset=utf-8")


@router.get("/assistant.html")
def assistant_page():
    return FileResponse(APP_DIR / "assistant.html")


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
    text = payload.text.strip()

    if not text:
        raise HTTPException(status_code=400, detail="请提供需要朗读的文本。")

    if not PIPER_MODEL_PATH.exists():
        raise HTTPException(
            status_code=500,
            detail=f"Piper 语音模型不存在：{PIPER_MODEL_PATH}",
        )

    TTS_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    cleanup_tts_output()

    output_filename = f"tts_{uuid.uuid4().hex}.wav"
    output_path = TTS_OUTPUT_DIR / output_filename

    try:
        selected_engine = get_tts_engine(payload.mode, payload.engine)

        if selected_engine == "melotts":
            run_melotts_tts(text, output_path)
        else:
            run_piper_tts(text, output_path)
    except subprocess.CalledProcessError as e:
        raise HTTPException(
            status_code=500,
            detail=f"TTS 生成语音失败：{e.stderr or e.stdout or str(e)}"
        )

    return {
        "ok": True,
        "engine": selected_engine,
        "audio_url": f"/tts_output/{output_filename}",
        "filename": output_filename,
    }
