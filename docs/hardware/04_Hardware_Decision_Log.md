# Hardware Decision Log

Version: v0.1

## HD-001：FT-01 不再作为最终硬件平台

Decision:

FT-01（Cardputer ADV）继续用于软件与交互验证，不再围绕其单一 USB-C、固定屏幕与既有外壳持续扩展最终硬件架构。

Reason:

- 已完成 Record、Log、Navigation 等核心软件能力验证。
- 继续硬件扩展会被现有接口与结构限制。
- FT-02 需要独立定义显示、通信、GNSS、电源和扩展接口。

## HD-002：启动 FT-02 模块化实验终端

Decision:

FT-02 从桌面能力实验台开始，不直接进入正式 PCB。

Reason:

- 降低硬件研发门槛。
- 先验证能力和模块边界。
- 避免围绕开发板形态设计产品。

## HD-003：内部接口与外部接口分离

Decision:

内部使用 UART、SPI、I²C 与 GPIO；外部保留双 USB-C 和 CAN。

Reason:

- 内部接口简单、低功耗、易于模块化。
- USB-C 提供最广泛的外部扩展生态。
- CAN 适合能源、车载、长线与多节点扩展。
- 不将 SPI 作为默认外部用户接口。

## HD-004：Radio 与 GNSS 固定存在但可更换

Decision:

Radio Module 与 GNSS Module 为终端固定能力，但采用可维护、可替换的模块结构。

Reason:

- 终端必须具备通信与定位。
- 射频和 GNSS 技术会升级。
- 单模块损坏不应导致整机报废。

## HD-005：Mesh Lab 使用 Heltec V3

Decision:

第一阶段使用 3 个 Heltec WiFi LoRa 32 V3，433 MHz，SX1262。

Reason:

- Meshtastic 支持成熟。
- 适合验证点对点与三节点多跳。
- OLED 在实验阶段便于现场观察状态。
- Heltec 作为 Mesh Lab 节点，不直接等同于 FT-02 最终 Radio Module。

## HD-006：显示尺寸确定为 4.5 英寸墨水屏

Decision:

FT-02 个人随身终端采用 4.5 英寸黑白墨水屏作为显示基线。

Reason:

- 更符合随身携带和单手使用。
- 满足任务、通讯、导航、日志与手册快速查看。
- 墨水屏适合低功耗常显。
- 不再以 5.83、6、7.5 英寸作为 FT-02 主显示方案。

## HD-007：主控自研采用渐进路线

Decision:

不从裸 ESP32 芯片级主板开始，先开发板验证，再做载板，最后使用成熟模组设计主板。

Reason:

- 避免初期承担射频、Flash、PSRAM、晶振和高速电源设计风险。
- 允许工厂完成 PCB 与 SMT。
- 让自研集中在接口、电源、模块化与整机能力上。
