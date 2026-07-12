# FT-02 Module Definition

Version: v0.1

## Main Controller Module

验证硬件：

- ESP32-S3 N16R8 DevKit

最终方向：

- 成熟 ESP32-S3-WROOM 类模组
- 自定义载板与主板

## Radio Module

固定能力：

- Meshtastic
- 433 MHz LoRa
- 独立网络保持
- 可替换

验证平台：

- Heltec WiFi LoRa 32 V3

最终模块方向：

- 独立 MCU
- SX1262
- UART
- IRQ / RESET / POWER ENABLE
- 天线接口
- 无独立 UI 和屏幕

## GNSS Module

固定能力：

- GPS / 北斗
- UART
- PPS
- 外接天线优先
- 可替换

## Display Module

固定基线：

- 4.5 英寸黑白墨水屏
- SPI
- 3.3 V
- 局部刷新

## Power Module

第一阶段：

- 使用现成实验模块
- 建立功耗数据库

最终方向：

- 独立充电与保护
- 电量计
- 电源域控制
- 可换电池
- 支持外部能源输入

## Storage Module

- microSD
- SPI
- 可拆卸
- 高耐久存储优先

## Audio Module

- I²S 麦克风
- I²S 功放
- 扬声器
- 录音与播放低功耗控制

## Sensor Module

- I²C
- 环境、姿态、时间和电量相关传感器
