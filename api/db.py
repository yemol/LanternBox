"""SQLite 数据库连接与初始化工具。负责本地结构化数据表创建。"""

import sqlite3
from .config import DB_PATH
from .utils import make_item_code


def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def column_exists(conn, table_name: str, column_name: str) -> bool:
    rows = conn.execute(f"PRAGMA table_info({table_name})").fetchall()
    return any(row["name"] == column_name for row in rows)


def backfill_item_codes(conn):
    rows = conn.execute(
        """
        SELECT id FROM inventory
        WHERE item_code IS NULL OR item_code = ''
        ORDER BY id ASC
        """
    ).fetchall()

    for row in rows:
        item_code = make_item_code(row["id"])
        conn.execute(
            "UPDATE inventory SET item_code = ? WHERE id = ?",
            (item_code, row["id"]),
        )


def add_column_if_missing(conn, table_name: str, column_name: str, column_definition: str):
    if column_exists(conn, table_name, column_name):
        return
    try:
        conn.execute(f"ALTER TABLE {table_name} ADD COLUMN {column_definition}")
    except sqlite3.OperationalError as error:
        if "duplicate column name" not in str(error).lower():
            raise


def init_db():
    conn = get_db_connection()

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS inventory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_code TEXT UNIQUE,
            name TEXT NOT NULL,
            category TEXT,
            quantity REAL DEFAULT 0,
            unit TEXT,
            expire_date TEXT,
            note TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS journal (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            entry_type TEXT DEFAULT '日常记录',
            title TEXT,
            content TEXT NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            metadata_json TEXT
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS terminal_devices (
            device_id TEXT PRIMARY KEY,
            name TEXT,
            role TEXT,
            status TEXT,
            trusted INTEGER,
            created_at TEXT,
            last_seen_at TEXT,
            last_sync_at TEXT,
            firmware_version TEXT,
            notes TEXT
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS terminal_tasks (
            task_id TEXT PRIMARY KEY,
            title TEXT,
            description TEXT,
            priority TEXT,
            status TEXT,
            assigned_to TEXT,
            created_at TEXT,
            updated_at TEXT,
            revision INTEGER,
            target_json TEXT,
            tags_json TEXT,
            created_by TEXT
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS task_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id TEXT,
            event_type TEXT,
            payload_json TEXT,
            created_at TEXT
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS task_reports (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            report_id TEXT,
            task_id TEXT,
            device_id TEXT,
            status TEXT,
            note TEXT,
            device_date TEXT,
            device_time TEXT,
            lat REAL,
            lon REAL,
            source TEXT,
            created_at TEXT
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS lora_messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            direction TEXT NOT NULL,
            text TEXT NOT NULL,
            raw TEXT,
            transport TEXT,
            port TEXT,
            rssi REAL,
            snr REAL,
            metadata_json TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS lora_nodes (
            node_id TEXT PRIMARY KEY,
            name TEXT,
            role TEXT,
            status TEXT,
            transport TEXT,
            port TEXT,
            rssi REAL,
            snr REAL,
            message_count INTEGER DEFAULT 0,
            metadata_json TEXT,
            first_seen_at TEXT,
            last_seen_at TEXT
        )
        """
    )

    add_column_if_missing(conn, "inventory", "item_code", "item_code TEXT")

    add_column_if_missing(conn, "journal", "metadata_json", "metadata_json TEXT")

    add_column_if_missing(conn, "task_reports", "report_id", "report_id TEXT")

    add_column_if_missing(conn, "task_reports", "source", "source TEXT")

    add_column_if_missing(conn, "lora_messages", "transport", "transport TEXT")

    conn.execute(
        """
        CREATE UNIQUE INDEX IF NOT EXISTS idx_task_reports_report_id
        ON task_reports(report_id)
        WHERE report_id IS NOT NULL AND report_id != ''
        """
    )

    backfill_item_codes(conn)
    conn.commit()
    conn.close()
