from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(errors='ignore')
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
uh=(root/'src/FT02_CommunicationNodeUI.h').read_text(errors='ignore')
vt=(root/'src/FT02_MeshVoiceTransport.cpp').read_text(errors='ignore')
vh=(root/'src/FT02_MeshVoiceTransport.h').read_text(errors='ignore')
checks=[
 ('main includes transport API', '#include "FT02_MeshVoiceTransport.h"' in main),
 ('transport revision api', 'FT02_MeshVoiceTransportRevision()' in vt and 'FT02_MeshVoiceTransportRevision();' in vh),
 ('chunk count api', 'FT02_MeshVoiceTransportChunksSent()' in vt and 'FT02_MeshVoiceTransportTotalChunks()' in vt),
 ('revision advances per chunk', '++g_tx.nextChunk;\n                ++g_txRevision;' in vt),
 ('voice tx partial declared', 'FT02_DrawCommunicationVoiceTxPartial' in uh),
 ('voice tx partial implemented', 'void FT02_DrawCommunicationVoiceTxPartial' in ui),
 ('ui shows chunk counts', 'FT02_MeshVoiceTransportChunksSent()' in ui and 'FT02_MeshVoiceTransportTotalChunks()' in ui),
 ('main 10pct bucket', 'pct / 10u' in main and 'g_ft02MeshVoiceLastShownTxBucket' in main),
 ('main partial refresh', 'FT02_DrawCommunicationVoiceTxPartial(display)' in main),
 ('bar uses rf pct', 'static_cast<uint32_t>(txPct) * static_cast<uint32_t>(innerW)' in ui),
 ('partial covers progress block', 'FT02_CommunicationCommitPartial(display, 56, 204, 688, 208);' in ui),
 ('stable tx state label', 'state = "正在发送语音"' in ui),
 ('fixed width chunk columns', '分片 %03u / %03u' in ui and 'snprintf(pct, sizeof(pct), "%03u%%"' in ui),
 ('private voice picker retained', 'ENTER确认私聊语音' in ui),
 ('inbox new-send discoverability', 'ENTER回复/播放 · T新建' in ui and 'V广播' not in ui),
]
failed=[]
for name,ok in checks:
 print(('PASS' if ok else 'FAIL'),name)
 if not ok: failed.append(name)
if failed: raise SystemExit('FAIL: '+', '.join(failed))
print('PASS Mesh voice real RF progress + graphical bar partial-refresh guard')
