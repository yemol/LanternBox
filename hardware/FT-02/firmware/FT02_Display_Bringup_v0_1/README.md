# LanternBox FT-02 Firmware

FT-02 是 LanternBox / 壳中灯的随身终端固件工程。当前采用 **Core ESP32-S3 + LR01** 双处理器架构：Core 负责 UI、电子纸、输入、SD、地图、日志和 FieldManual；LR01 负责 GNSS、QMC5883L 罗盘和 Meshtastic/LoRa。

## 当前提交基线

**FT-02 v2.75k19 / Submission Hotfix A1.1**

本版以已经完成实机快速交互验证并收口的 v2.75k18 为提交基线，只修复帮助页旧 UI 子集字库缺字导致的方框乱码，不改变已验证功能行为：

- `常用快捷键`、`状态页罗盘` 等帮助页中文改用全量 CJK 字库，消除缺字方框
- 右侧快捷键说明重新预留完整 5 个中文字宽度，避免修复字形后越过边框
- 帮助页底栏中文也统一使用全量 CJK 20px 字库
- 保持 Global EPD Interactive Runtime A1、FieldManual A3.12、收藏/最近阅读/搜索、LR01 Single Authority 和全部已验证按键行为不变

完整操作、部署、编译、验证与发布说明：

- [`docs/FT02_PROJECT_GUIDE.md`](docs/FT02_PROJECT_GUIDE.md)

接口权威：

- [`docs/LR01_HOST_PROTOCOL_A2.md`](docs/LR01_HOST_PROTOCOL_A2.md)
- [`docs/FT02_LR01_HOST_INTERFACE_PIN_LOCK.md`](docs/FT02_LR01_HOST_INTERFACE_PIN_LOCK.md)

版本历史：

- [`CHANGELOG.md`](CHANGELOG.md)

## 编译

```bash
pio run
```

当前 PlatformIO 环境：

```text
ft02-pbf-a1-n16r8-spi40
board = esp32-s3-devkitc1-n16r8
```

必须保留：

```text
boards/esp32-s3-devkitc1-n16r8.json
```

废弃的 `ft02-esp32-s3-n16r8` Board ID 不再包含在提交包中，也不要恢复。

## 架构硬约束

1. Core 不解析 NMEA。
2. Core 不直接访问 QMC5883L。
3. Core 不直接控制 SX126x。
4. Core 不承担 Meshtastic protobuf full-sync / NodeDB 权威逻辑。
5. 罗盘校准参数由 LR01 计算并持久化。
6. 导航、罗盘和 LoRa 状态以 LR01 Host Protocol 为权威。
