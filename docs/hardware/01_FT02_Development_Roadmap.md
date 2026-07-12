# FT-02 Development Roadmap

Version: v0.1  
Status: Baseline

## FT-01：软件验证平台

当前已完成：

- Record
- Log
- Navigation

继续推进：

- Task
- Sync
- Communication UI
- Manual UI
- Settings
- System

FT-01 继续承担软件、交互与操作逻辑验证，不再承担 FT-02 最终硬件形态。

## Mesh Lab：通信能力验证

目标：

- Meshtastic 固件与节点配置
- 两节点点对点通信
- 三节点多跳组网
- 节点发现
- 消息收发
- Hop 验证
- 中继恢复
- RSSI / SNR 记录
- 天线对比
- 功耗测试

验证硬件：

- Heltec WiFi LoRa 32 V3，433 MHz，SX1262 ×3

## FT-02A：桌面能力实验台

形态：

- ESP32-S3 开发板
- 4.5 英寸 SPI 墨水屏
- 独立 GNSS 模块
- 独立 Radio 验证节点
- SD 存储
- 音频模块
- 传感器模块
- 电源与功耗测试工具

目标：

- 验证接口分配
- 验证驱动与模块协同
- 验证功耗和睡眠
- 验证 UI 在 4.5 英寸墨水屏上的可用性

## FT-02B：模块化载板原型

形态：

- 现成开发板和模块插接
- 自定义载板或背板
- 统一连接器
- 统一供电
- 可装入实验外壳

目标：

- 消除面包板和散乱线束
- 验证布局、装配、维修和携带
- 验证双 USB-C、CAN 与内部模块接口

## FT-02C：模组级主板

形态：

- ESP32-S3-WROOM 等成熟模组
- 可插拔 GNSS Module
- 可插拔 Radio Module
- 集成显示、存储、音频、电源与接口

目标：

- 第一块真正的 FT-02 自研主板
- 不从裸芯片与自研射频开始
- 由工厂完成 PCB 与 SMT

## FT-03：正式工程原型

前提：

- FT-02C 完成长周期使用验证
- 功耗、结构、接口与固件稳定
- 关键电源与射频设计经过专业复核

目标：

- 正式结构件
- 正式主板
- 长期户外与离网环境测试
