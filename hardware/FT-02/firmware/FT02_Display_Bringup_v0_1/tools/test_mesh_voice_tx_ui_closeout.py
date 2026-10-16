from pathlib import Path

root = Path(__file__).resolve().parents[1]
ui = (root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
uh = (root/'src/FT02_CommunicationNodeUI.h').read_text(errors='ignore')
vt = (root/'src/FT02_MeshVoiceTransport.cpp').read_text(errors='ignore')
vh = (root/'src/FT02_MeshVoiceTransport.h').read_text(errors='ignore')
main = (root/'src/main.cpp').read_text(errors='ignore')

checks = [
    ('UI includes voice transport API header', '#include "FT02_MeshVoiceTransport.h"' in ui),
    ('voice tx service declared', 'FT02_CommunicationUIServiceVoiceTransport' in uh),
    ('voice tx service implemented', 'bool FT02_CommunicationUIServiceVoiceTransport()' in ui),
    ('UI shows RF sending state', '正在发送语音' in ui),
    ('UI shows air progress', '分片 %03u / %03u' in ui and 'snprintf(pct, sizeof(pct), "%03u%%"' in ui),
    ('broadcast completion notice', '广播语音已发送' in ui),
    ('direct waits for real END->ACK phase', 'FT02_MeshVoiceTransportAwaitingAck()' in ui),
    ('awaiting ack API declared', 'FT02_MeshVoiceTransportAwaitingAck' in vh),
    ('awaiting ack API checks TX_WAIT_ACK', 'g_tx.phase == TX_WAIT_ACK' in vt),
    ('main services tx closeout', 'FT02_CommunicationUIServiceVoiceTransport()' in main),
    ('RF per-chunk telemetry', '[MESH-VOICE-TX] RF OK CHUNK' in vt),
    ('RF END telemetry', '[MESH-VOICE-TX] RF OK END' in vt),
]
failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(('PASS' if ok else 'FAIL'), name)
if failed:
    raise SystemExit('FAIL: ' + ', '.join(failed))
print('PASS mesh voice TX UI closeout guard')
