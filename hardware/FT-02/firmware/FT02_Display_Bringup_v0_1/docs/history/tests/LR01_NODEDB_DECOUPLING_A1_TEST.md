# LR01 NodeDB Decoupling A1 Test

1. 启动后确认 `[LR01] READY protocol=2`。
2. 进入内部通讯 -> 网络终端，应看到 Core 发送 `MESH_NODES?`。
3. LR01 返回 `MESH_NODE ...` 和 `MESH_NODE_END count=n` 后，节点应立即显示。
4. 选择非本机节点并 ENTER，应直接进入私信编辑，不应再出现“等待 NodeDB”。
5. 从收件箱选择一条有 from NodeID 的消息回复，应直接进入私信编辑。
6. 若 LR01 未学习 PKI，由 LR01 返回 `MESH_TX_FAILED ... reason=pki_not_ready`，Core 不做旧 NodeDB 拦截。
7. 节点页按 R，应发送 `CORE_STATUS?` + `MESH_NODES?`，不应触发旧 LoRa reset/full-sync。
