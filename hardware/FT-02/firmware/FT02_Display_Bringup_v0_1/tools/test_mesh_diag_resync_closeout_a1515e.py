from pathlib import Path

ui = Path("src/FT02_CommunicationNodeUI.cpp").read_text()
uih = Path("src/FT02_CommunicationNodeUI.h").read_text()
svc = Path("src/FT02_MeshService.cpp").read_text()
svch = Path("src/FT02_MeshService.h").read_text()
main = Path("src/main.cpp").read_text()

def req(cond, msg):
    if not cond:
        raise AssertionError(msg)

req("MESH_SYNC_NOTICE_TIMEOUT_MS = 6000u" in ui, "finite sync timeout missing")
req("g_syncNoticeNodeInfoRxBaseline = FT02_MeshServiceNodeInfoRxCount()" in ui, "NodeInfo RX baseline missing")
req("FT02_MeshServiceNodeInfoRxCount() != g_syncNoticeNodeInfoRxBaseline" in ui, "NodeInfo response completion missing")
req("Mesh刷新完成，暂无新身份响应" in ui, "successful timeout closeout text missing")
req("Mesh同步超时，请稍后重试" in ui, "offline timeout closeout text missing")
req("if(!FT02_MeshServiceRequestNodeInfo())" in ui, "immediate request failure closeout missing")
req("bool FT02_CommunicationUIServiceSyncNotice(uint32_t nowMs);" in uih, "sync service API missing")
req("uint32_t FT02_MeshServiceNodeInfoRxCount();" in svch, "NodeInfo RX count API missing")
req("uint32_t FT02_MeshServiceNodeInfoRxCount() { return g_nodeInfoRx; }" in svc, "NodeInfo RX accessor missing")
req("FT02_CommunicationUIServiceSyncNotice(millis())" in main, "main loop sync closeout service missing")
req("Manual NodeInfo refresh itself does not toggle Mesh/NodeRuntime ready" in main, "stale resync semantics comment remains")
print("PASS A1.5.15e diagnostics resync finite closeout")
