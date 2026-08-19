# Changelog

## 2026-08-19 - Meshtastic Native A2 + Compass Calibration A1

### Added

- QMC5883L 正式校准流程
- `COMPASS_CAL_START`
- `COMPASS_CAL_STATUS?`
- `COMPASS_CAL_SAVE`
- `COMPASS_CAL_CANCEL`
- `COMPASS_CAL_RESET`
- `COMPASS_CAL_STATE`
- Hard-iron offset 校准
- 基础 per-axis soft-iron scale 修正
- 基于样本、三轴跨度、平衡度和空间覆盖的 progress / quality
- 双槽 NVS + generation + CRC32 校准持久化
- USB Serial bench calibration command input

### Protocol baseline retained

- HOST_PROTOCOL_A2 protocol=2
- `NAV_STATE`
- `RADIO_STATE`
- `SYSTEM_STATE`
- Message ID lifecycle
- Node list
- Meshtastic broadcast / PKI private messaging
- Routing ACK / Delivery status

### Documentation

- 合并重复 Host Protocol 文档
- 新增硬件参考
- 新增罗盘校准说明
- 新增统一验收文档
- 新增故障排查文档

## Previous verified baseline

A2 之前已实机验证：Host UART、GNSS stream、QMC5883L、SX1262 RF、Meshtastic NodeInfo、广播消息和 PKI 私信链路。
