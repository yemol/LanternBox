# FT-02 v2.75j Compass Calibration A1

## 入口
设备状态 -> K 罗盘校准。

进入页面后 Core 自动发送：
`COMPASS_CAL_STATUS?`

## 正常流程
1. ENTER 开始：
   `COMPASS_CAL_START`
2. LR01 返回 RUNNING，页面显示 progress / quality / samples / XYZ 范围。
3. 缓慢转动设备并做空间“8”字。
4. READY 后 ENTER：
   `COMPASS_CAL_SAVE`
5. 收到 SAVED 后应显示“当前校准：已保存”。

## 取消
RUNNING / READY 状态按 DEL：
`COMPASS_CAL_CANCEL`
旧持久化校准不得受影响。

## 重置
按 R -> 必须进入二次确认页。
确认后发送：
`COMPASS_CAL_RESET`

## 错误
Core 应显示 code 20..25：
20 busy
21 not_running
22 quality_low
23 save_failed
24 reset_failed
25 compass_not_ready

## 架构验收
Core 不得：
- 访问 QMC5883L I2C
- 处理原始磁场 offset/scale
- 保存罗盘校准参数
- 自行计算 heading

所有校准权威均在 LR01。
