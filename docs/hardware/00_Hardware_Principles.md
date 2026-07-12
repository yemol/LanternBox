# LanternBox Hardware Principles

Version: v0.1  
Status: Baseline  
Date: 2026-07-12

## 目的

本文档定义 LanternBox / 壳中灯随身终端硬件研发的基本原则。

后续采购、能力验证、实验原型、接口设计、PCB 设计与结构设计，均应遵循本文档。

## 1. 开发板只用于能力验证

开发板、成品模块与现成终端仅用于验证能力，不默认进入最终产品。

当前验证设备包括：

- Cardputer ADV
- Heltec WiFi LoRa 32 V3
- ESP32-S3 DevKit
- M5Stack GNSS 模块
- SPI 墨水屏模块
- 电源与传感器实验模块

## 2. 先验证能力，再确定硬件

开发顺序固定为：

```text
Capability Verification
        ↓
Modular Prototype
        ↓
Carrier Board
        ↓
Module-level Mainboard
        ↓
Production Prototype
```

不得围绕某一块开发板持续堆叠功能。

## 3. 每项采购必须对应一项长期能力

采购前必须明确：

1. 该硬件验证什么能力。
2. 验证完成后的输出是什么。
3. 能否继续用于后续实验或作为对照平台。
4. 是否会影响既定接口与模块边界。

无法回答以上问题的设备原则上不采购。

## 4. FT-01 定位

FT-01（Cardputer ADV）定位为软件与交互验证平台。

负责继续验证：

- Record
- Log
- Navigation
- Task
- Sync
- Communication UI
- Manual UI
- System / Settings UI

FT-01 不再作为 FT-02 最终硬件架构的约束来源。

## 5. FT-02 定位

FT-02 定位为模块化实验终端。

核心组成：

- Main Controller
- Display Module
- Radio Module
- GNSS Module
- Power Module
- Storage Module
- Audio Module
- Sensor Module

## 6. 固定能力，模块化实现

以下能力属于终端固定能力，但模块本身应可维护、可更换：

- Radio / Meshtastic
- GNSS
- Display
- Storage
- Audio
- Power
- Basic Sensors

“固定”不等于永久焊死。

## 7. 内部接口封装

内部优先使用：

- UART：GNSS、Radio 协处理器
- SPI：Display、Storage
- I²C：RTC、电量计、IMU、环境传感器
- GPIO：IRQ、RESET、POWER ENABLE、MODULE DETECT

内部接口不直接暴露给日常用户。

## 8. 外部接口简化

FT-02 外部接口基线：

- USB-C ×2
  - System / Charge / Debug / Sync
  - Expansion / Host
- CAN ×1
  - 车载、能源、传感器与工业扩展
- 可选维护级扩展口
  - 仅在实验版或开发版开放

SPI 不作为默认外部用户接口。

## 9. 显示基线

FT-02 个人随身终端显示基线：

- 4.5 英寸黑白墨水屏
- SPI 接口
- 支持局部刷新
- 适合低功耗常显
- UI 与整机结构围绕 4.5 英寸尺寸设计

## 10. Mesh 验证基线

第一阶段 Mesh Lab 使用：

- Heltec WiFi LoRa 32 V3
- SX1262
- 433 MHz
- Meshtastic
- 3 个节点

测试顺序：

1. 两节点点对点。
2. 三节点多跳组网。
3. 距离、高度、天线、RSSI、SNR 与功耗测试。

## 11. 第一阶段不设计正式 PCB

第一阶段目标是完成能力验证和模块边界确认。

在以下条件满足前，不进入正式 PCB：

- 关键能力完成验证。
- 接口定义稳定。
- 功耗数据可追溯。
- 模块之间不存在结构性冲突。
- 实验记录完整。

## 12. 所有实验必须留档

每项能力验证应至少输出：

- 实验目标
- 硬件清单
- 接线方式
- 固件版本
- 参数配置
- 测试步骤
- 结果数据
- 问题与结论
- 是否通过
- 后续动作
