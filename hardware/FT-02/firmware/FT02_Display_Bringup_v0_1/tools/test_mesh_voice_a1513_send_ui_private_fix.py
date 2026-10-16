from pathlib import Path
root = Path(__file__).resolve().parents[1]
ui = (root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
proto = (root/'src/FT02_MeshVoiceProtocol.h').read_text(errors='ignore')
transport = (root/'src/FT02_MeshVoiceTransport.cpp').read_text(errors='ignore')
transport_h = (root/'src/FT02_MeshVoiceTransport.h').read_text(errors='ignore')
service = (root/'src/FT02_MeshService.cpp').read_text(errors='ignore')
service_h = (root/'src/FT02_MeshService.h').read_text(errors='ignore')

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL ' + msg)

# A1.5.12 failure was 204-byte voice chunk -> 216-byte voice wire ->
# Data protobuf + 12-byte PKI + 16-byte Mesh header = 250-byte RF frame.
req('FT02_VOICE_CHUNK_BYTES = 192u' in proto, 'voice chunk must be restored to 192 B')
req('FT02_VOICE_WIRE_MAX_BYTES = 204u' in proto, 'wire max must be 204 B')
req('MESH_PRIVATE_APP_MAX_PAYLOAD = 206u' in service, 'private app envelope ceiling missing')
req('FT02_MeshServiceMaxPrivateAppPayload' in service_h and 'FT02_MeshServiceMaxPrivateAppPayload()' in service,
    'private envelope API missing')
req('FT02_VOICE_WIRE_MAX_BYTES > FT02_MeshServiceMaxPrivateAppPayload()' in transport,
    'voice private preflight missing')
# Exact worst-case private RF size at current port 256 / chunk packet format.
chunk = 192
voice_wire = 12 + chunk
protobuf = voice_wire + 6
pki = protobuf + 12
radio = pki + 16
req(radio == 238 and radio <= 240, f'private RF frame budget unsafe: {radio}')

# Failed lower-layer sends must not retry forever.
req('TX_MAX_CONSECUTIVE_FAILURES = 5u' in transport, 'bounded TX failure retry missing')
req('TX aborted after %u consecutive failures' in transport, 'bounded failure abort log missing')
req('FT02_MESH_STATUS_FAILED' in transport, 'failed voice history state missing')
req('FT02_MESH_VOICE_TX_FAILED' in transport_h and 'FT02_MeshVoiceTransportLastOutcome' in transport_h,
    'transport failure outcome API missing')
req('私聊语音发送失败，已停止重试' in ui, 'UI failure closeout missing')

# New sending must be explicit, not inferred from whichever history row happens to be selected.
for text in ('广播文字', '广播语音', '私聊文字', '私聊语音'):
    req(text in ui, f'new-message option missing: {text}')
req('COMM_SEND_MENU' in ui, 'explicit send menu missing')
req("g_mode == COMM_INBOX && cmd == 'v'" not in ui, 'Inbox global V shortcut should remain removed after A1.5.14a')
req('case 1u:' in ui and 'return beginVoice(FT02_MESH_BROADCAST_NODE, COMM_INBOX);' in ui, 'T New broadcast voice behavior missing')
req('return beginVoice(FT02_MESH_BROADCAST_NODE, COMM_INBOX);' in ui, 'direct new broadcast voice shortcut missing')
req('NODE_ACTION_PRIVATE_VOICE' in ui and 'ENTER确认私聊语音' in ui, 'explicit private voice picker missing')

# Footer/status help must match actual state.
for text in ('ENTER回复/播放 · T新建', '正在保存/压缩语音', '看到提示后说话'):
    req(text in ui, f'communication footer/status missing: {text}')
req('FT02_MeshVoiceTransportCancel();' in ui and '{nullptr, "B取消"}' in ui, 'voice TX cancel action/footer missing')

print('PASS A1.5.13 explicit send UI + PKI-safe private voice + bounded failure closeout')
