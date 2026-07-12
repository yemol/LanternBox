# FT-02 Procurement List

Version: v0.1  
Status: Initial Verification Stage

## A. Mesh Communication Capability

### Heltec WiFi LoRa 32 V3

- 完整型号：Heltec WiFi LoRa 32 V3
- MCU：ESP32-S3
- LoRa：SX1262
- 频率：433 MHz
- 数量：3
- 用途：
  - 两节点点对点通信
  - 三节点 Meshtastic 多跳组网
  - RSSI / SNR / 距离 / 功耗验证
- 淘宝搜索关键词：
  - `Heltec WiFi LoRa 32 V3 SX1262 433MHz`
  - `Heltec LoRa32 V3 433`

### 433 MHz 天线

- 数量建议：
  - 3 dBi 胶棒天线 ×3
  - 5 dBi 长棒天线 ×2
- 要求：
  - 明确标注 433 MHz
  - 与所购转接头接口匹配
- 搜索关键词：
  - `433MHz SMA 3dBi 胶棒天线`
  - `433MHz SMA 5dBi 全向天线`

### 天线转接线

- 数量：3 至 5
- 常见规格：IPEX / U.FL 转 SMA
- 搜索关键词：
  - `IPEX转SMA 1.13线`
  - `UFL转SMA 433`

## B. Main Controller Capability

### ESP32-S3 DevKit

- 推荐规格：N16R8
- Flash：16 MB
- PSRAM：8 MB
- 引脚：优先 44Pin
- 数量：1
- 用途：
  - FT-02A 主控验证
  - UART / SPI / I²C
  - USB Host
  - 睡眠与功耗
- 搜索关键词：
  - `ESP32-S3 N16R8 44脚 Type-C 开发板`
  - `ESP32-S3 DevKitC-1 N16R8`

## C. Display Capability

### 4.5 英寸黑白 SPI 墨水屏

- 数量：1
- 关键要求：
  - 4.5 英寸
  - 黑白
  - SPI
  - 3.3 V 逻辑
  - 支持局部刷新
  - 提供 ESP32 示例或公开驱动资料
  - 带 BUSY / RST / DC / CS 引脚
- 搜索关键词：
  - `4.5寸 墨水屏 SPI ESP32`
  - `4.5英寸 电子纸模块 黑白 SPI`
  - `4.5inch e-paper SPI`

## D. GNSS Capability

### UART GNSS 模块

第一阶段要求：

- UART 输出
- 支持 GPS 与北斗
- 有公开协议或 NMEA 输出
- 支持外接天线更佳

候选搜索关键词：

- `M5Stack GPS Unit v1.1`
- `AT6668 GPS 北斗 UART 模块`
- `u-blox M8N UART GPS 模块`

数量：1

## E. Storage Capability

### SPI microSD 模块

- 数量：1
- 要求：
  - 3.3 V 逻辑
  - SPI 接口
  - 电平转换设计清晰
- 搜索关键词：
  - `SPI Micro SD模块 3.3V`
  - `ESP32 SD卡模块 SPI`

### microSD 卡

- 数量：1 至 2
- 建议：32 GB 或 64 GB，高耐久型号优先

## F. Power Capability

### USB-C 功率测试仪

- 数量：1
- 用途：
  - 电压、电流、功率与累计电量
  - 对比显示、GNSS、LoRa、录音、睡眠状态
- 搜索关键词：
  - `USB-C 功率测试仪 电流表`
  - `Type-C 电压电流功率计`

### 实验电源模块

第一阶段不锁定最终电源架构，优先选择具备以下能力的模块：

- 单节锂电池充电
- 过充、过放、过流保护
- USB-C 输入
- 稳定 5 V 或 3.3 V 输出
- 支持边充边用时必须明确说明

搜索关键词：

- `单节锂电 UPS模块 Type-C 5V`
- `锂电池 充放电保护 升压 5V Type-C`
- `ESP32 UPS 电源模块 Type-C`

注意：

- IP5306 可用于早期实验，但不作为最终电源方案基线。
- 最终电源设计必须单独评审。

### 电池与电池座

- 18650 电池 ×2
- 单节 18650 电池座 ×2
- 使用可靠品牌与正规保护方案
- 不采购来源不明的高容量虚标电池

## G. CAN Capability

### CAN 收发模块

- 数量：2
- 推荐芯片方向：
  - SN65HVD230，3.3 V
- 搜索关键词：
  - `SN65HVD230 CAN模块 3.3V`
- 用途：
  - 终端与电源箱、车载、传感器模块之间的总线验证

## H. Audio Capability

第一阶段可后购。

候选：

- I²S 麦克风：INMP441
- I²S 功放：MAX98357A
- 小型扬声器：4 Ω / 3 W

搜索关键词：

- `INMP441 I2S 麦克风模块`
- `MAX98357A I2S 功放模块`
- `4欧3瓦 小喇叭`

## I. Sensors Capability

第一阶段可后购。

候选：

- BME280：温度、湿度、气压
- IMU：根据后续导航需求选型
- RTC：低功耗独立时钟
- 电量计：后续单独选型

## J. 连接与实验材料

建议采购：

- 2.54 mm 杜邦线
- JST-GH 插头与线束
- USB-C 数据线
- 铜柱、M2 / M2.5 螺丝
- 亚克力或 3D 打印实验底板
- 焊锡、热缩管、标签纸
- 万用表
- 逻辑分析仪，建议 8 通道基础款

## 第一批优先采购

| 硬件 | 数量 | 对应能力 |
|---|---:|---|
| Heltec WiFi LoRa 32 V3，433 MHz | 3 | Mesh |
| 433 MHz 天线 | 5 | Mesh / 天线 |
| IPEX / U.FL 转 SMA 线 | 3-5 | Mesh / 天线 |
| ESP32-S3 N16R8 开发板 | 1 | Main Controller |
| 4.5 英寸 SPI 黑白墨水屏 | 1 | Display |
| UART GNSS 模块 | 1 | GNSS |
| SPI microSD 模块 | 1 | Storage |
| USB-C 功率测试仪 | 1 | Power |
| 实验电源模块 | 1-2 | Power |
| 18650 电池与电池座 | 各2 | Power |
| SN65HVD230 CAN 模块 | 2 | CAN |
