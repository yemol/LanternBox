# LR01 Troubleshooting

## `lora=1` 但没有消息

检查：

- `RADIO_STATE profile=CN35 freq=478.875`
- 天线是否连接正确
- 现有网络是否使用相同频道参数和密钥
- `rx=` 是否增长

## 收到 RF 但文本乱码

如果日志只有原始 RF 数据而无法形成 `MESH_RX`，优先检查 Meshtastic 频道密钥和 protobuf / packet decode 路径，不要把二进制 payload 当普通字符串。

## `gnss=1` 但 `fix=0`

`gnss=1` 表示 K25+ UART 数据流在线，不等于已有卫星定位。

室内可出现：

```text
gnss=1 fix=0 sat_used=0
```

这是正常状态。不要仅凭 `fix=0` 判断 GNSS 断线。

## `compass=1` 但 `compass_q=1`

表示 QMC5883L 正常，但没有有效已保存校准。

运行正式罗盘校准流程并 SAVE 后应变为 `2` 或 `3`。

## 校准一直到不了 READY

检查：

- 是否只做水平旋转，没有覆盖 Z 轴
- 是否完成俯仰 / 滚转 / 8 字动作
- `min/max` 是否在三轴都明显扩展
- `samples` 是否达到数百个

进度不是计时器，设备静置不会自动达到 100%。

## `compass_cal_quality_low`

说明当前 session 尚未达到 `quality>=2`。继续多姿态运动后再 SAVE。

## `compass_cal_busy`

已有 RUNNING / READY session。先 SAVE 或 CANCEL，再 START 新 session。

## Host UART 没有响应

确认：

```text
Core GPIO13 TX -> LR01 GPIO18 RX
Core GPIO7  RX <- LR01 GPIO17 TX
GND 共地
115200 8N1
```

先用：

```text
CORE_PING_1
```

验证最基础双向链路。

## 修改协议时的原则

如果新需求可以通过新增命令或新状态包解决，不要修改已冻结字段名字、单位或语义。
