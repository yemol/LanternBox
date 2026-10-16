#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
svc=(root/'src/FT02_MeshService.cpp').read_text()
protoh=(root/'src/FT02_MeshProtocol.h').read_text()
protoc=(root/'src/FT02_MeshProtocol.cpp').read_text()
nodeh=(root/'src/FT02_LoRaNodeRuntime.h').read_text()
nodec=(root/'src/FT02_LoRaNodeRuntime.cpp').read_text()
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text()
comm=(root/'src/FT02_LoRaCommunicationRuntime.cpp').read_text()
main=(root/'src/main.cpp').read_text()
pki=(root/'src/FT02_MeshPki.cpp').read_text()
checks={
 'nodeinfo_parser': 'FT02_MeshParseUserProto' in protoc and 'fieldNo == 2u' in protoc and 'fieldNo == 3u' in protoc and 'fieldNo == 8u' in protoc,
 'nodeinfo_builder': 'FT02_MeshBuildUserProto' in protoc and 'LanternBox FT02' in protoc and 'FT02' in protoc,
 'identity_guard': 'identityMatch = identityNode == header.from' in svc,
 'nodedb_names_keys': 'FT02_LoRaNodeRuntimeHostLearnIdentity' in svc and 'memcpy(v.publicKey' in nodec,
 'ui_mesh_refresh': 'FT02_MeshServiceRequestNodeInfo()' in ui and 'Mesh：%s' in ui,
 'main_node_revision_redraw': 'FT02_LoRaNodeRuntimeRevision() != g_ft02LastLoRaNodeRevision' in main,
 'pki_persistent_key': 'NVS_NAMESPACE = "ft02pki"' in pki and 'NVS_PRIVATE_KEY = "private"' in pki,
 'pki_x25519_aesccm': 'x25519' in pki and 'mbedtls_sha256' in pki and 'mbedtls_ccm_encrypt_and_tag' in pki and 'mbedtls_ccm_auth_decrypt' in pki,
 'private_channel_zero': 'transmitPrepared(destination, 0u' in svc,
 'private_want_ack': 'true, packetId, "PKI_TEXT"' in svc,
 'private_runtime_route': 'FT02_MeshServiceSendPrivateText(destination, text, packetId)' in comm,
 'private_rx': 'header.channel == 0u && header.to == FT02_MESH_LOCAL_NODE' in svc and 'decryptFromPeer' in svc,
 'routing_ack_tx': 'sendRoutingAck(header.from, header.id)' in svc,
 'routing_ack_rx': 'FT02_LoRaCommunicationHostDelivery(data.requestId, header.from, error == 0u)' in svc,
 'nodeinfo_auto_request': 'g_nodeInfoBootPending = true' in svc and 'sendNodeInfo(true)' in svc,
 'pki_unknown_pending': 'rememberPendingPki' in svc and 'retryPendingForPeer' in svc,
 'no_unsupported_peer_pollution': 'opaque reason=unsupported_channel_or_target' in svc,
}
failed=[]
for k,v in checks.items():
    print(('PASS' if v else 'FAIL'), k)
    if not v: failed.append(k)
raise SystemExit(1 if failed else 0)
