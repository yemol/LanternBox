from pathlib import Path
root=Path(__file__).resolve().parents[1]
radio=(root/'src/FT02_MeshRadioBackend.cpp').read_text()
probe=(root/'src/FT02_LoRaHwProbe.cpp').read_text()
svc=(root/'src/FT02_MeshService.cpp').read_text()
comm=(root/'src/FT02_LoRaCommunicationRuntime.cpp').read_text()
proto=(root/'src/FT02_MeshProtocol.h').read_text()
checks={
 'local_node_ft02':'0x4C423002u' in proto,
 'broadcast_service':'FT02_MeshServiceSendBroadcastText' in svc,
 'flags_hop_3':'FT02_MESH_HOP_START << 5u' in svc and 'FT02_MESH_HOP_LIMIT' in svc,
 'channel_a4':'FT02_MESH_CHANNEL_HASH' in svc,
 'aes_ctr':'aesCtrCrypt(dataProto, dataLen, FT02_MESH_LOCAL_NODE, packetId)' in svc,
 'radio_transmit':'g_radio->transmit(data, len)' in radio,
 'tx_path_before_rf':'FT02_LoRaHwProbeSetRfTransmit()' in radio,
 'rx_path_restore':'FT02_LoRaHwProbeSetRfReceive()' in radio and 'startListening()' in radio,
 'aw_tx_pattern':'AW_RADIO_TX_ACTIVE = 0x05' in probe,
 'aw_rx_pattern':'AW_RADIO_RX_STANDBY = 0x03' in probe,
 'ui_broadcast_mesh':'FT02_MeshServiceSendBroadcastText(text, packetId)' in comm,
 'private_meshservice':'FT02_MeshServiceSendPrivateText(destination, text, packetId)' in comm and 'meshPrivate' in comm,
 'outbox_broadcast_sent':'meshBroadcastReady ? FT02_DELIVERY_SENT' in comm,
 'outbox_private_waits_ack':'meshPrivateReady' in comm and 'FT02_DELIVERY_WAITING_ACK' in comm,
 'no_auto_tx':'FT02_MeshServiceSendBroadcastText(' not in (root/'src/main.cpp').read_text(),
}
failed=[]
for k,v in checks.items():
 print(('PASS' if v else 'FAIL'),k)
 if not v: failed.append(k)
raise SystemExit(1 if failed else 0)
