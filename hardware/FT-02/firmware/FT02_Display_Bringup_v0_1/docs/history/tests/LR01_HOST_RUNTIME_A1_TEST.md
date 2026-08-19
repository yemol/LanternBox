> SUPERSEDED by `LR01_HOST_RUNTIME_A2_TEST.md`. Retained only as A1 history.

# FT-02 v2.74x LR01 Host Runtime A1 Test

## 固定接口

- Core GPIO13 TX -> LR01 GPIO18 RX
- Core GPIO7 RX <- LR01 GPIO17 TX
- GND <-> GND
- 115200 8N1, 3.3V TTL

## 启动预期

Core:

```text
[LR01-HOST-A1] ready RX=7 TX=13 baud=115200 protocol=ASCII-LF
```

LR01启动后应收到并解析：

```text
LR01_BOOT ...
LR01_READY
SYSTEM_STATE ...
NAV_STATE ...
RADIO_STATE ...
```

Core每秒发送：

```text
CORE_PING_<n>
```

并应收到：

```text
LR01_PONG_<n>
```

## 消息

广播发送：

```text
MESH_TX <UTF-8 text>
```

私信：

```text
MESH_PRIVATE !4c423001 <UTF-8 text>
```

接收：

```text
MESH_RX from=... name="..." kind=MESH_TEXT ... text="..."
MESH_RX from=... name="..." kind=MESH_PKI_TEXT ... text="..."
```

## 重要架构变化

Core不再发送旧 Meshtastic protobuf `want_config/full-sync`，不再复位LoRa协处理器。
Core不再直接打开GNSS UART或解析NMEA。导航状态由LR01 `NAV_STATE/SYSTEM_STATE`注入现有FT02 GNSS兼容状态层。

LR01 A1 当前只返回本地发送结果 `MESH_TX_RESULT`，没有端到端投递ACK。因此旧可靠/持久消息UI仍可排队，但在本协议版本中以LR01提交成功作为“已发送”，不再由Core自行重试空口发送。
