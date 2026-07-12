# FT-02 Capability List

Version: v0.1

| Capability | 验证目标 | 当前/计划硬件 | 状态 |
|---|---|---|---|
| Record | 文字、语音、现场记录 | Cardputer ADV | Completed |
| Log | 日志管理、回听、删除、索引 | Cardputer ADV | Completed |
| Navigation | 轨迹、地图、导航交互 | Cardputer ADV | Completed |
| Task | 任务创建、查看、完成、同步准备 | Cardputer ADV | In Progress |
| Sync | 与 Core 的任务包和数据同步 | Cardputer ADV | In Progress |
| Communication UI | 节点、频道、消息、状态界面 | Cardputer ADV | Planned |
| Manual UI | 手册浏览、搜索与离线查看 | Cardputer ADV | Planned |
| Mesh Point-to-Point | 两节点稳定收发 | Heltec WiFi LoRa 32 V3 ×2 | Planned |
| Mesh Multi-hop | 三节点转发与恢复 | Heltec WiFi LoRa 32 V3 ×3 | Planned |
| GNSS | 定位、时间、轨迹与漂移测试 | UART GNSS 模块 | Planned |
| Main Controller | UART / SPI / I²C / USB / 睡眠 | ESP32-S3 N16R8 DevKit | Planned |
| E-paper Display | 4.5 英寸显示、局刷、功耗 | 4.5 英寸 SPI 黑白墨水屏 | Planned |
| Storage | 日志、地图、手册与录音存储 | SPI microSD 模块 | Planned |
| Audio | 录音、播放、音量与功耗 | I²S 麦克风与功放模块 | Planned |
| Power | 充电、供电、续航与功耗数据库 | USB-C 功率计、电池、电源模块 | Planned |
| Sensors | 环境与姿态数据采集 | I²C 传感器模块 | Planned |
| CAN | 外部工业与能源扩展 | CAN 收发模块 | Planned |
| USB Expansion | 外设、存储、键盘等扩展 | USB Host 实验 | Planned |

## Capability 完成标准

每项能力必须满足：

- 功能验证通过
- 硬件名称与版本已记录
- 接线与接口已记录
- 固件版本已记录
- 功耗数据已记录
- 已知问题已记录
- 结论可复现
- 输出接口定义或驱动抽象
