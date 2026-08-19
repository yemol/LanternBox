# FT-02 v2.74y LR01 Host Runtime A2 Test

基于冻结的 FT02-LR01 HOST_PROTOCOL_A2 Final。

## 1. 物理接口

- Core GPIO13 TX -> LR01 GPIO18 RX
- Core GPIO7 RX <- LR01 GPIO17 TX
- 115200 8N1
- ASCII/UTF-8 + LF

## 2. 启动验收

应看到：

```text
[LR01-HOST-A2] ready RX=7 TX=13 baud=115200 protocol=ASCII/UTF8-LF
[LR01] BOOT ... protocol=2 ...
[LR01] READY protocol=2
[LR01-TX] CORE_STATUS? id=...
[LR01-TX] MESH_NODES?
```

随后应收到 NAV_STATE / RADIO_STATE / SYSTEM_STATE / STATUS_END。

## 3. NAV_STATE

确认日志包含：fix_type、sat_used、sat_visible、speed、compass_q、unix/time_valid。
地图和定位记录继续通过 FT02_GnssSnapshot 使用 LR01 归一化状态。

## 4. 广播

发送一条广播，生命周期应至少出现：

```text
MESH_TX_ACCEPTED id=...
MESH_TX_SENT id=...
MESH_TX_RESULT id=... type=TEXT ok=1
```

## 5. 私信可靠投递

发送私信后应出现：

```text
MESH_TX_ACCEPTED id=...
MESH_TX_SENT id=...
MESH_TX_RESULT id=... type=PRIVATE ok=1
MESH_DELIVERY id=... node=... status=ACK
```

只有 ACK 视为 delivered。TIMEOUT 会把可靠/持久消息重新放回 Core 队列等待重试。

## 6. 接收消息

MESH_RX 的 air packet id 会作为 Core inbox packetId，用于重复包去重；from/to 会保留广播/私信方向。

## 7. 节点列表

READY 后 Core 自动发送 MESH_NODES?。应看到若干 MESH_NODE 行，最后：

```text
[LR01] NODE_LIST complete count=...
```

## 8. 回归

地图、中文输入法、SD、搜索、PBF Source Tiles、Host PBI 不应有行为变化。
