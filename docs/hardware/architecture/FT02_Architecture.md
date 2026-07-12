# FT-02 Architecture

Version: v0.1

## 总体框图

```text
┌──────────────────────────────────────┐
│              FT-02 Main              │
│                                      │
│  UI / Task / Log / Navigation        │
│  Communication / Manual / Sync       │
│                                      │
│          Main Controller             │
└───────┬────────┬────────┬────────────┘
        │        │        │
      UART1    UART2      SPI
        │        │        ├── 4.5" E-paper
        │        │        └── microSD
        │        │
      GNSS     Radio
        │        │
        └────┬───┘
             │
             I²C
             ├── RTC
             ├── Fuel Gauge
             ├── IMU
             └── Environment Sensors

External:
- USB-C System / Charge / Debug / Sync
- USB-C Expansion / Host
- CAN
```

## 模块角色

### Main Controller

负责：

- UI 与状态机
- 任务、日志、导航与手册
- 本地文件系统
- 与 Core 同步
- 控制显示、音频、存储和传感器
- 与 Radio / GNSS 模块通信

### Radio Module

负责：

- Meshtastic 节点能力
- LoRa 收发
- 网络保持与转发
- 节点和频道状态
- 与主控通过 UART 或后续确定的内部协议连接

### GNSS Module

负责：

- 定位
- 时间
- 速度与方向
- 轨迹原始数据
- 通过 UART 输出 NMEA 或结构化数据

### Display Module

基线：

- 4.5 英寸
- 黑白墨水屏
- SPI
- 支持局部刷新

### Power Module

负责：

- 充电
- 电池保护
- 电量测量
- 电源域控制
- 睡眠与唤醒
- 外设按需上电

### External Expansion

- USB-C：通用外设
- CAN：能源、车载、传感器与工业扩展
