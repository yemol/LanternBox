# FT02-LR01 HOST_PROTOCOL_A2

状态：**冻结基线 + Compass Calibration 增量扩展**  
协议版本：`2`

## 1. 设备身份

```text
owner=LanternBox FT-02
owner_short=FT02
node=!4c423002
```

启动：

```text
LR01_BOOT version=MESH_NATIVE_A2 protocol=2 node=FT02 id=!4c423002
LR01_READY protocol=2
```

## 2. 物理 Host UART

```text
Core GPIO13 TX -> LR01 GPIO18 RX
Core GPIO7  RX <- LR01 GPIO17 TX
115200 8N1
ASCII / UTF-8 + LF
```

Host 输入单行最大 256 bytes。超长行丢弃至下一次 LF，并返回：

```text
LR01_ERR code=8 message="line_too_long"
```

用户消息正文最大 120 UTF-8 bytes。

## 3. Core -> LR01

### 链路与状态

```text
CORE_PING_<seq>
CORE_STATUS?
CORE_STATUS? id=<uint32>
```

### Mesh

```text
MESH_NODEINFO
MESH_NODES?
MESH_TX <text>
MESH_TX id=<uint32> <text>
MESH_PRIVATE <node> <text>
MESH_PRIVATE id=<uint32> <node> <text>
```

### Compass Calibration

```text
COMPASS_CAL_START
COMPASS_CAL_STATUS?
COMPASS_CAL_SAVE
COMPASS_CAL_CANCEL
COMPASS_CAL_RESET
```

## 4. LR01 -> Core 基础状态

### PING

```text
LR01_PONG_<seq>
```

### NAV_STATE

完整格式：

```text
NAV_STATE fix=<0|1> fix_type=<0|2|3> lat=<e7> lon=<e7> alt=<dm> sat=<n> sat_used=<n> sat_visible=<n> hdop=<x100> speed=<cm/s> heading=<x10deg> compass=<0|1> compass_q=<0..3> unix=<sec> time_valid=<0|1>
```

示例：

```text
NAV_STATE fix=1 fix_type=3 lat=312138835 lon=1214956818 alt=286 sat=14 sat_used=14 sat_visible=27 hdop=98 speed=135 heading=968 compass=1 compass_q=2 unix=1787053355 time_valid=1
```

单位：

- `lat/lon`：degree × 1e7
- `alt`：0.1 m
- `hdop`：HDOP × 100
- `speed`：cm/s
- `heading`：degree × 10
- `unix`：UTC Unix seconds
- `sat`：A1 兼容字段，始终等于 `sat_used`

`fix_type`：

- `0`：No Fix
- `2`：2D Fix
- `3`：3D Fix

`compass_q`：

- `0`：无效
- `1`：传感器正常但没有有效已保存校准
- `2`：已保存可用校准
- `3`：已保存高质量校准

### RADIO_STATE

```text
RADIO_STATE ready=<0|1> profile=<name> freq=<MHz> rx=<n> nodes=<n> pki=<n> dup=<n> tx_queue=<n>
```

示例：

```text
RADIO_STATE ready=1 profile=CN35 freq=478.875 rx=25 nodes=3 pki=2 dup=1 tx_queue=0
```

### SYSTEM_STATE

```text
SYSTEM_STATE gnss=<0|1> compass=<0|1> lora=<0|1> uptime=<sec> gnss_bytes=<n> heap=<bytes> psram=<bytes> rx_errors=<n> uart_errors=<n> radio_resets=<n>
```

示例：

```text
SYSTEM_STATE gnss=1 compass=1 lora=1 uptime=1234 gnss_bytes=974815 heap=185000 psram=8123456 rx_errors=0 uart_errors=0 radio_resets=0
```

### 强制状态同步结束

```text
STATUS_END
STATUS_END id=<uint32>
```

## 5. Mesh 发送生命周期

Core 生成 `id=<uint32>`，LR01 必须原样贯穿整个 Host 生命周期。

```text
MESH_TX_ACCEPTED id=<uint32>
MESH_TX_SENT id=<uint32>
MESH_TX_FAILED id=<uint32> reason=<reason>
MESH_TX_RESULT id=<uint32> type=TEXT|PRIVATE ok=0|1 [reason=...]
MESH_DELIVERY id=<uint32> node=<hex> status=ACK|TIMEOUT
```

语义：

- `ACCEPTED`：LR01 已接受 Host 请求。
- `SENT`：LR01 已完成本地 Meshtastic 编码和 RF 发射。
- `DELIVERY ... ACK`：LR01 收到与该空口包匹配的 Meshtastic Routing ACK。
- `DELIVERY ... TIMEOUT`：等待窗口内没有收到匹配 Delivery ACK。

`SENT` 不能解释为已送达。Core 的可靠消息“已送达”状态只能由 `MESH_DELIVERY ... ACK` 驱动。

Core ID 与 Meshtastic 空口 `MeshPacket.id` 是两套 ID。LR01 内部维护映射，Core 不需要管理空口 packet ID。

## 6. Mesh 接收

```text
MESH_RX id=<air_packet_id> from=<hex> to=<hex> name="..." kind=... rssi=... snr=... text="..."
```

Core 可使用 `id/from/to` 做收件箱去重以及广播 / 私信分类。

## 7. NodeDB 查询

Core：

```text
MESH_NODES?
```

LR01：

```text
MESH_NODE id=!4C423001 long="LanternBox FT01" short="FT01" online=1 hops=0 rssi=-63.0 snr=6.0 pki=1 last=3
MESH_NODE_END count=1
```

## 8. Compass Calibration

### 状态包

```text
COMPASS_CAL_STATE state=<STATE> progress=<0..100> quality=<0..3> samples=<n> calibrated=<0|1> min_x=<n> max_x=<n> min_y=<n> max_y=<n> min_z=<n> max_z=<n>
```

`STATE`：

```text
IDLE
RUNNING
READY
SAVED
CANCELED
FAILED
```

示例：

```text
COMPASS_CAL_STATE state=RUNNING progress=42 quality=1 samples=386 calibrated=1 min_x=-1240 max_x=822 min_y=-956 max_y=1105 min_z=-701 max_z=593
```

`calibrated=1` 表示 LR01 已存在持久化有效校准。因此重新校准时允许出现：

```text
state=RUNNING calibrated=1
```

此时实时 heading 仍继续使用旧校准，直到新的 `COMPASS_CAL_SAVE` 成功。

仅 `quality >= 2` 允许 SAVE。

详细算法、质量门槛和持久化规则见 [`COMPASS_CALIBRATION.md`](COMPASS_CALIBRATION.md)。

## 9. 错误码

通用错误：

```text
1  unknown_command
2  invalid_argument
3  payload_too_large
4  node_not_found
5  pki_not_ready
6  radio_not_ready
7  queue_full
8  line_too_long
```

罗盘校准扩展：

```text
20 compass_cal_busy
21 compass_cal_not_running
22 compass_cal_quality_low
23 compass_cal_save_failed
24 compass_cal_reset_failed
25 compass_not_ready
```

统一格式：

```text
LR01_ERR code=<n> message="<text>"
```

## 10. 兼容性规则

- A2 已冻结字段名称、单位和语义不得修改。
- 后续优先新增独立命令和状态包。
- `NAV_STATE` 现有字段顺序和含义保持不变。
- A1 无 ID 的 `MESH_TX` / `MESH_PRIVATE` 仍保留兼容入口。
