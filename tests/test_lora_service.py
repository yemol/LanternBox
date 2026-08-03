import json

from api import db
from api.services import lora_service


def setup_lora_db(monkeypatch, tmp_path):
    monkeypatch.setattr(db, "DB_PATH", tmp_path / "lanternbox-lora.db")
    db.init_db()
    lora_service._RECENT_MESHTASTIC_PACKETS = {}
    lora_service._SELF_NODE_ID = ""
    lora_service._STATE.rx_count = 0
    lora_service._STATE.last_rx_at = ""
    lora_service._STATE.updated_at = ""


def test_meshtastic_text_packet_is_recorded_as_rx_message(monkeypatch, tmp_path):
    setup_lora_db(monkeypatch, tmp_path)
    lora_service._STATE.port = "/dev/ttyUSB0"

    packet = {
        "from": 0x4C423001,
        "to": 0xFFFFFFFF,
        "id": 123,
        "rxRssi": -91,
        "rxSnr": 7.5,
        "decoded": {
            "portnum": "TEXT_MESSAGE_APP",
            "text": "别人发来的消息",
        },
    }

    lora_service._record_meshtastic_packet(packet)
    lora_service._record_meshtastic_packet(packet)

    conn = db.get_db_connection()
    try:
        rows = conn.execute("SELECT * FROM lora_messages").fetchall()
        node = conn.execute("SELECT * FROM lora_nodes WHERE node_id = ?", ("!4c423001",)).fetchone()
    finally:
        conn.close()

    assert len(rows) == 1
    assert rows[0]["direction"] == "rx"
    assert rows[0]["text"] == "别人发来的消息"
    assert rows[0]["rssi"] == -91
    assert rows[0]["snr"] == 7.5
    metadata = json.loads(rows[0]["metadata_json"])
    assert metadata["source"] == "meshtastic_api_packet"
    assert metadata["sender_node_id"] == "!4c423001"
    assert node is not None
    assert lora_service._STATE.rx_count == 1


def test_meshtastic_text_packet_can_decode_payload_bytes():
    packet = {
        "decoded": {
            "portnum": "TEXT_MESSAGE_APP",
            "payload": list("hello".encode("utf-8")),
        }
    }

    assert lora_service._meshtastic_packet_text(packet) == "hello"


def test_meshtastic_self_node_is_marked_in_node_list(monkeypatch, tmp_path):
    setup_lora_db(monkeypatch, tmp_path)

    lora_service._SELF_NODE_ID = "!09d3a9a0"
    lora_service._record_meshtastic_node(
        "self",
        {
            "num": 0x09D3A9A0,
            "user": {
                "id": "!09d3a9a0",
                "longName": "LanternBox Core",
                "shortName": "Core",
            },
        },
        transport="usb_serial",
        port="/dev/ttyUSB0",
        is_self=True,
    )

    result = lora_service.list_lora_nodes()
    node = next(item for item in result["nodes"] if item["node_id"] == "!09d3a9a0")

    assert node["metadata"]["is_self"] is True
    assert node["long_name"] == "LanternBox Core"
    assert node["short_name"] == "Core"
