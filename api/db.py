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
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )

    if not column_exists(conn, "inventory", "item_code"):
        conn.execute("ALTER TABLE inventory ADD COLUMN item_code TEXT")

    backfill_item_codes(conn)
    conn.commit()
    conn.close()
