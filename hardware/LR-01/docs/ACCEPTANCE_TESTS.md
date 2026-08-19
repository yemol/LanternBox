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

LR01 USB Serial Monitor 设为 `115200`。测试固件允许 USB 调试口输入同样的校准命令。

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

### 3. 多姿态运动

缓慢执行：

- 水平 360° 旋转
- 俯仰翻转
- 左右滚转
- 8 字运动

观察：

- `samples` 持续增长
- X/Y/Z min/max 范围持续扩大
- `progress` 根据覆盖增加
- `quality` 逐渐从 0 -> 1 -> 2/3
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
