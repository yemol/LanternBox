# LanternBox FT-02 Firmware

FT-02 是 LanternBox 的随身终端固件工程。当前生产架构采用 **Core + LR01 双处理器**：

- **Core**：UI、电子纸显示、输入、SD、地图、日志、知识库。
- **LR01**：GNSS、QMC5883L 罗盘、Meshtastic/LoRa。
- Core 与 LR01 通过 Host UART A2 通讯。
- Core 不直接访问 GNSS、罗盘或 LoRa 硬件。

## 当前版本

见 [`VERSION.txt`](VERSION.txt)。

当前主线：**v2.75k Documentation & Help Cleanup**。

本版不改变已验证的通讯/导航架构，主要整理项目说明、帮助、协议和验收文件，同时更新设备内帮助页。

## 编译

工程使用 PlatformIO。

```bash
pio run
```

当前环境：

```text
ft02-pbf-a1-n16r8-spi40
custom board: boards/ft02-esp32-s3-n16r8.json
```

不要删除 `boards/`，否则 PlatformIO 会报 `UnknownBoard`。

更完整步骤见：

- [`docs/BUILD_AND_FLASH.md`](docs/BUILD_AND_FLASH.md)
- [`docs/USER_GUIDE.md`](docs/USER_GUIDE.md)

## 当前关键功能

- PBF 离线地图、搜索、区域缓存与黑白快速导航
- LR01 实时 GNSS 定位同步
- 地图当前位置方向箭头
- CardKB2 离线拼音输入
- 语音日志与定位记录
- Meshtastic 广播、私信、可靠投递状态
- 三页系统自检
- LR01 QMC5883L 罗盘校准 UI
- 离线应急手册 / 知识库

## 文档入口

- [`docs/README.md`](docs/README.md) 文档索引
- [`docs/USER_GUIDE.md`](docs/USER_GUIDE.md) 用户操作说明
- [`docs/BUILD_AND_FLASH.md`](docs/BUILD_AND_FLASH.md) 编译与刷机
- [`docs/LR01_HOST_PROTOCOL_A2.md`](docs/LR01_HOST_PROTOCOL_A2.md) Core ↔ LR01 协议
- [`docs/FT02_LR01_HOST_INTERFACE_PIN_LOCK.md`](docs/FT02_LR01_HOST_INTERFACE_PIN_LOCK.md) 硬件接口锁定
- [`docs/RELEASE_CHECKLIST.md`](docs/RELEASE_CHECKLIST.md) 提交/发布前检查
- [`CHANGELOG.md`](CHANGELOG.md) 版本变更记录

历史测试和审计记录已归档到 `docs/history/`，不再堆在工程根目录。

## 架构硬约束

1. Core 不解析 NMEA。
2. Core 不访问 QMC5883L I2C。
3. Core 不控制 SX126x。
4. Core 不运行 Meshtastic protobuf full-sync / NodeDB 权威逻辑。
5. 罗盘校准参数只由 LR01 计算并持久化。
6. 所有导航、罗盘与 LoRa 运行状态以 LR01 Host Protocol 为权威。

