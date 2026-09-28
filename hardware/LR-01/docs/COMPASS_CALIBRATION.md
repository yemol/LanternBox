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

当前 LR01 没有加速度计，运行时 heading 是严格的二维磁航向。因此校准也必须与运行模型一致，只使用设备**保持水平时**的 X/Y 数据。

校准动作：

```text
整机保持水平
-> 缓慢旋转 360°
-> 重复 2~3 圈
-> 不做 8 字
-> 不前后/左右倾斜
```

硬铁偏置：

```text
offsetX = (maxX + minX) / 2
offsetY = (maxY + minY) / 2
```

二维轴向比例：

```text
rangeX = (maxX - minX) / 2
rangeY = (maxY - minY) / 2

A4.6 no longer uses independent scaleX/scaleY as the final correction. It retains the X/Y hard-iron center, computes the full X/Y covariance including the cross term, and derives a symmetric 2x2 inverse-square-root correction matrix.
```

运行时：

```text
correctedX = matrixXX*(rawX-offsetX) + matrixXY*(rawY-offsetY)
correctedY = matrixXY*(rawX-offsetX) + matrixYY*(rawY-offsetY)
correctedZ = rawZ
```

航向使用：

```text
atan2(-correctedY, correctedX)
```

随后应用 FT-02 机身 +90° yaw 安装修正。

Z 轴仍在 `COMPASS_CAL_STATE` 中上报，便于诊断传感器和现场磁环境，但不再参与 heading 校准质量、X/Y scale 或保存参数。旧版把 3D 倾斜采样的 Z 范围纳入 X/Y scale，会在没有倾斜补偿的前提下扭曲水平航向。

## 4. 进度与质量

进度综合：

- 有效样本数量
- X / Y 两轴跨度
- X / Y 跨度平衡度
- 围绕运行中硬铁中心的四象限覆盖

当前门槛：

| Quality | 条件 |
|---|---|
| 0 | 数据不足 / 无效 |
| 1 | 基础样本和 X/Y 跨度已建立 |
| 2 | ≥400 samples，X/Y 最小跨度 ≥600，balance ≥0.25，≥3 quadrants |
| 3 | ≥800 samples，X/Y 最小跨度 ≥900，balance ≥0.50，4 quadrants |

只有 `quality >= 2` 才会进入 `READY` 并允许保存。

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
- matrixXX/matrixXY/matrixYY
- scaleZ
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


## A4.4 installed body-frame correction

Four-point bench measurements on the final FT-02 assembly showed that the published heading rotated in the opposite direction to the physical device heading. This cannot be corrected by a yaw offset alone. A4.4 RC1 therefore applies the body-frame transform **after** hard/soft-iron calibration:

```text
bodyX = correctedX
bodyY = -correctedY
heading = atan2(bodyY, bodyX)
heading += 97 degrees
```

The 97-degree yaw term is the fitted device-front alignment from the four measured pairs 0->278, 90->178, 180->99, 270->12. It should be rechecked after flashing using another four-direction test. Saved calibration offset/scale data remains mathematically valid because the sign transform is applied after calibration. Recalibration is still recommended after final mechanical assembly if residual error varies by direction.
