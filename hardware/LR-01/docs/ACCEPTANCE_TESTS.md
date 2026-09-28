# LR01 Acceptance Tests

## A. 基础 Runtime

### 1. Host UART

Core 连续发送：

```text
CORE_PING_100
```

预期：

```text
LR01_PONG_100
```

### 2. 状态查询

```text
CORE_STATUS? id=42
```

预期依次看到：

```text
SYSTEM_STATE ...
NAV_STATE ...
RADIO_STATE ...
STATUS_END id=42
```

### 3. Mesh 接收

从 FT01 / 已有网络节点广播：

```text
test
```

LR01 应上报类似：

```text
MESH_RX id=... from=4C423001 to=FFFFFFFF name="FT01" kind=MESH_TEXT rssi=... snr=... text="test"
```

### 4. Mesh 广播发送

Core：

```text
MESH_TX id=1001 TF02 test
```

至少应看到：

```text
MESH_TX_ACCEPTED id=1001
MESH_TX_SENT id=1001
MESH_TX_RESULT id=1001 type=TEXT ok=1
```

并由现有网络节点确认收到。

### 5. PKI 私信

```text
MESH_PRIVATE id=1002 !4c423001 private-test
```

应看到发送生命周期，并在远端 ACK 可用时最终出现：

```text
MESH_DELIVERY id=1002 node=4C423001 status=ACK
```

## B. QMC5883L 校准

LR01 USB Serial Monitor 设为 `115200`。固件允许 USB 控制台输入同样的校准命令。

### 1. 初始状态

```text
COMPASS_CAL_STATUS?
```

从未校准时：

```text
COMPASS_CAL_STATE state=IDLE ... calibrated=0 ...
```

已有校准时 `calibrated=1` 属于正常。

### 2. 开始

```text
COMPASS_CAL_START
```

预期：

```text
COMPASS_CAL_STATE state=RUNNING progress=0 quality=0 samples=0 calibrated=<old> ...
```

### 3. 水平旋转

整机保持水平，缓慢执行：

- 水平 360° 旋转
- 重复 2~3 圈
- 不做俯仰翻转
- 不做左右滚转
- 不做 8 字运动

观察：

- `samples` 持续增长
- X/Y min/max 范围持续扩大
- 四象限覆盖逐渐完整
- `progress` 根据覆盖增加
- `quality` 逐渐从 0 -> 1 -> 2/3
- Z min/max 仅作为诊断显示，不参与质量判断
- LoRa、GNSS、PING、STATUS 同时继续运行

### 4. 保存

当：

```text
state=READY
quality>=2
```

发送：

```text
COMPASS_CAL_SAVE
```

预期：

```text
COMPASS_CAL_STATE state=SAVED progress=100 quality=2|3 ... calibrated=1 ...
```

随后 `NAV_STATE compass_q=2|3`。

### 5. 重启持久化

重启 LR01，再查询：

```text
COMPASS_CAL_STATUS?
```

必须满足：

```text
calibrated=1
```

同时 `NAV_STATE compass_q=2|3`。

### 6. CANCEL 保留旧校准

```text
COMPASS_CAL_START
# 做少量运动
COMPASS_CAL_CANCEL
```

如果此前已有旧校准，应仍：

```text
calibrated=1
```

重启后依旧存在。

### 7. 低质量 SAVE 保护

```text
COMPASS_CAL_START
# 只采少量样本
COMPASS_CAL_SAVE
```

预期：

```text
LR01_ERR code=22 message="compass_cal_quality_low"
```

旧 calibration 不得被覆盖。

### 8. RESET

仅在明确需要删除校准时执行：

```text
COMPASS_CAL_RESET
```

预期：

```text
COMPASS_CAL_STATE state=IDLE ... calibrated=0 ...
```

重启后仍为 `calibrated=0`。

## C. GNSS Robust Recovery A1

### 1. 基础诊断

发送：

```text
CORE_STATUS? id=9001
```

确认 `SYSTEM_STATE` 至少满足：

```text
gnss_rxbuf=4096
gnss_nmea_ok=<持续增长>
gnss_nmea_bad=<正常情况下应远低于 ok>
```

### 2. 室内 -> 室外恢复（本版核心回归）

1. 在室内开机，保持 GNSS 无 Fix 2~5 分钟。
2. 不重启 LR01，不重新插拔，不进入任何手动“重连”。
3. 把整机直接移动到室外开阔天空。
4. 观察 `NAV_STATE`。

通过条件：

- `sat_visible` 只能反映 5 秒内的新鲜 GSV；旧值会自动归零。
- 获取有效定位后必须自动从 `fix=0` 转为 `fix=1`。
- 不允许要求人工重连 LR01。
- `gnss_nmea_ok` 应持续增长。
- 正常情况下 `gnss_recoveries` 可以保持 0；若链路异常触发恢复，应自动恢复后继续增长 `gnss_bytes` / `gnss_nmea_ok`。

### 3. GNSS 串口异常恢复

测试环境允许时，短暂中断 GNSS TX -> LR01 RX 后恢复。

预期：

- 超过约 8 秒无 GNSS 字节时进入 `stream_stall`。
- 只重建 GNSS UART receive path，不重启 LoRa、罗盘或 Host UART。
- 恢复接线后 GNSS 数据自动重新进入。

### 4. NMEA 数据健康

长期观察：

```text
gnss_nmea_ok
gnss_nmea_bad
```

若 `gnss_nmea_bad` 快速连续增长，优先检查 GNSS UART 数据完整性、波特率、供电和接线。4096-byte RX buffer 用于降低主循环忙碌时的溢出风险。

