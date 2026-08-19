# LanternBox FT02-LR01 Firmware

FT02-LR01 是 LanternBox / FT-02 的导航与通信协处理器固件。

当前基线：**Meshtastic Native A2 + QMC5883L Compass Calibration A1**

设备身份：

```text
owner: LanternBox FT-02
owner_short: FT02
node: !4c423002
```

## 职责边界

LR01 是以下硬件与协议的唯一权威：

- LoRa / Meshtastic Native
- K25+ GNSS
- QMC5883L 罗盘
- 罗盘校准与持久化
- Core ↔ LR01 Host UART 协议

FT-02 Core 只负责 Host UART 上层业务、UI、存储和应用逻辑，不直接访问 SX1262、GNSS 或 QMC5883L。

## 当前已实现

- CN35 / 478.875 MHz Meshtastic Native 收发
- 广播文本消息
- PKI 私信
- NodeInfo / 节点发现
- Routing ACK / Delivery 状态
- Host Message ID 生命周期
- 节点列表查询
- GNSS 位置、速度、时间、卫星状态
- QMC5883L 航向
- QMC5883L 非阻塞校准
- 校准双槽 NVS 持久化、CRC32、回滚保护
- Host UART 状态与错误上报

## 已实机验证基线

在进入罗盘校准扩展前，下列链路已经实机通过：

- Core ↔ LR01 Host UART
- `CORE_PING` / `LR01_PONG`
- GNSS 数据流
- QMC5883L 读取
- SX1262 初始化与真实 RF 接收
- Meshtastic NodeInfo
- Meshtastic 广播文本接收
- 广播与 PKI 私信收发

罗盘校准扩展需按 `docs/ACCEPTANCE_TESTS.md` 完成最终实机验收后再标记为稳定。

## 硬件连接

| 功能 | LR01 GPIO |
|---|---:|
| QMC5883L SDA | 4 |
| QMC5883L SCL | 5 |
| GNSS RX | 6 |
| GNSS TX | 7 |
| SX1262 NSS | 8 |
| SX1262 SCK | 9 |
| SX1262 MOSI | 10 |
| SX1262 MISO | 11 |
| SX1262 RESET | 12 |
| SX1262 BUSY | 13 |
| SX1262 DIO1 | 14 |
| Host UART TX | 17 |
| Host UART RX | 18 |

Host UART：

```text
Core GPIO13 TX -> LR01 GPIO18 RX
Core GPIO7  RX <- LR01 GPIO17 TX
115200 8N1
ASCII / UTF-8 + LF
```

## 编译与烧录

使用 PlatformIO 打开项目根目录。

```bash
platformio run
```

烧录：

```bash
platformio run -t upload
```

串口监视：

```bash
platformio device monitor -b 115200
```

也可直接使用 VS Code PlatformIO 的 Build / Upload / Monitor。

## 文档

- [`docs/HOST_PROTOCOL_A2.md`](docs/HOST_PROTOCOL_A2.md)：Core ↔ LR01 正式 Host 协议
- [`docs/COMPASS_CALIBRATION.md`](docs/COMPASS_CALIBRATION.md)：QMC5883L 校准设计与语义
- [`docs/ACCEPTANCE_TESTS.md`](docs/ACCEPTANCE_TESTS.md)：烧录后的验收流程
- [`docs/HARDWARE.md`](docs/HARDWARE.md)：硬件角色、引脚与总线说明
- [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md)：常见故障排查
- [`CHANGELOG.md`](CHANGELOG.md)：版本变化记录

## 重要约束

1. 不要让 Core 重新直接访问 QMC5883L、GNSS 或 SX1262。
2. 不要破坏 HOST_PROTOCOL_A2 已冻结字段名称、单位和语义。
3. 新协议能力优先以“新增命令 / 新增独立状态包”扩展。
4. `COMPASS_CAL_START` 不得删除旧校准。
5. 只有 `COMPASS_CAL_SAVE` 成功后，新校准才生效。
6. `COMPASS_CAL_RESET` 是显式破坏性操作。
7. 项目包含私有 Meshtastic 网络材料时，不要上传到公开仓库。
