#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
svc=(root/'src/FT02_MeshService.cpp').read_text()
proto=(root/'src/FT02_MeshProtocol.cpp').read_text()
protoh=(root/'src/FT02_MeshProtocol.h').read_text()
checks={
  'stable_logical_node_id': 'FT02_MESH_LOCAL_NODE = 0x4C423002u' in protoh,
  'nodeinfo_long_name': '"LanternBox FT02"' in proto,
  'nodeinfo_short_name': '"FT02"' in proto,
  'nodeinfo_names_without_pki': 'if (!out || maxLen == 0u)' in proto and 'if (publicKey && !appendBytesField' in proto,
  'boot_announce_not_pki_gated': 'g_nodeInfoBootPending = true' in svc and 'if (!g_serviceReady) return false;' in svc,
  'boot_reannounce': 'NODEINFO_BOOT_REANNOUNCE_MS' in svc and 'g_nodeInfoBootReannouncePending' in svc,
  'periodic_announce': 'NODEINFO_PERIODIC_MS' in svc,
  'pretraffic_announce': 'announceIdentityBeforeFirstTraffic();' in svc,
  'manual_refresh_not_pki_gated': 'bool FT02_MeshServiceRequestNodeInfo()' in svc and 'if (!FT02_MeshServiceReady()) return false;' in svc,
}
failed=[]
for k,v in checks.items():
    print(('PASS' if v else 'FAIL'), k)
    if not v: failed.append(k)
raise SystemExit(1 if failed else 0)
