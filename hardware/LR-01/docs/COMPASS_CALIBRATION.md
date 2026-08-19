# QMC5883L Compass Calibration

## 1. 设计原则

QMC5883L 的访问、原始磁场采样、校准计算、持久化和 heading 计算全部由 LR01 完成。

Core：

- 不访问 QMC5883L I2C
- 不接收原始磁场流
- 不计算 offset / scale
- 不自行计算 heading

Core 只通过 HOST_PROTOCOL_A2 控制校准流程和显示状态。

## 2. 命令

```text
COMPASS_CAL_START
COMPASS_CAL_STATUS?
COMPASS_CAL_SAVE
COMPASS_CAL_CANCEL
COMPASS_CAL_RESET
```

### START

清空本次会话的 min/max、samples 和空间覆盖统计，但**不修改已保存校准**。

### STATUS?

返回完整 `COMPASS_CAL_STATE`。

### SAVE

仅在 `quality >= 2` 时允许。保存验证成功后，新校准才立即成为 active calibration。

### CANCEL

结束本次会话，丢弃本次结果，旧校准保持不变。

### RESET

删除 LR01 NVS 中的持久化校准，是显式破坏性操作。Core UI 应二次确认。

## 3. 校准数学

硬铁偏置：

```text
offsetX = (maxX + minX) / 2
offsetY = (maxY + minY) / 2
offsetZ = (maxZ + minZ) / 2
```

基础轴向软铁比例：

```text
rangeX = (maxX - minX) / 2
rangeY = (maxY - minY) / 2
rangeZ = (maxZ - minZ) / 2

avgRange = (rangeX + rangeY + rangeZ) / 3

scaleX = avgRange / rangeX
scaleY = avgRange / rangeY
scaleZ = avgRange / rangeZ
```

运行时：

```text
correctedX = (rawX - offsetX) * scaleX
correctedY = (rawY - offsetY) * scaleY
correctedZ = (rawZ - offsetZ) * scaleZ
```

当前 LR01 没有加速度计用于姿态倾斜补偿，因此航向仍使用：

```text
atan2(correctedY, correctedX)
```

Z 轴继续参与校准覆盖、质量判断与软铁修正，并为未来倾斜补偿保留。

## 4. 进度与质量

进度不是时间进度，而是综合：

- 有效样本数量
- X / Y / Z 三轴跨度
- 最小轴 / 最大轴跨度平衡度
- 围绕运行中硬铁中心的 3D octant 覆盖

当前门槛：

| Quality | 条件 |
|---|---|
| 0 | 数据不足 / 无效 |
| 1 | ≥100 samples，最小轴跨度 ≥200 counts |
| 2 | ≥400 samples，最小轴跨度 ≥600，balance ≥0.25，≥3 octants |
| 3 | ≥800 samples，最小轴跨度 ≥900，balance ≥0.50，≥6 octants |

只有 `quality >= 2` 才会进入 `READY`，并报告 `progress=100`。

这些阈值属于实现参数，可以根据后续 K25+/QMC5883L 实机采样微调，但不改变 Host 协议。

## 5. 持久化

使用 ESP32 Preferences / NVS：

```text
namespace: qmc_cal
keys: cal0, cal1
```

每个 slot 包含：

- magic
- version
- generation
- offsetX/Y/Z
- scaleX/Y/Z
- quality
- valid
- CRC32

保存采用双槽策略：

```text
旧有效槽保持不动
        ↓
写入另一槽
        ↓
readback
        ↓
CRC / generation 验证
        ↓
验证成功后才切换 active calibration
```

这样 SAVE 过程中异常断电时，旧有效校准仍有机会作为回滚版本。

启动时加载 generation 最新且 CRC 有效的 slot。

## 6. 运行时行为

校准过程为 non-blocking。

期间必须继续运行：

- Meshtastic / LoRa
- GNSS
- Host UART
- PING / STATUS

`RUNNING` / `READY` 期间约每秒主动上报一次 `COMPASS_CAL_STATE`。

若已有旧校准：

- 新校准采样期间 heading 继续使用旧校准
- SAVE 成功后立即使用新校准

若无任何已保存校准：

- heading 仍可输出原始二维磁航向
- `compass_q=1`

## 7. compass_q

```text
0 = 传感器 / heading 无效
1 = QMC5883L 正常，但没有有效已保存校准
2 = 当前 active calibration 可用
3 = 当前 active calibration 质量良好
```
