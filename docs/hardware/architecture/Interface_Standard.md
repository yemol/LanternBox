# FT-02 Interface Standard

Version: v0.1  
Status: Draft Baseline

## Internal UART

### UART-GNSS

建议最小信号：

- 3V3
- GND
- TX
- RX
- PPS
- RESET
- POWER ENABLE

### UART-RADIO

建议最小信号：

- 3V3 或 5V
- GND
- TX
- RX
- IRQ
- RESET
- POWER ENABLE
- MODULE DETECT

## Internal SPI

### SPI-DISPLAY

- 3V3
- GND
- SCLK
- MOSI
- CS
- DC
- RST
- BUSY
- POWER ENABLE

### SPI-STORAGE

- 3V3
- GND
- SCLK
- MOSI
- MISO
- CS
- CARD DETECT
- POWER ENABLE

Display 与 Storage 是否共享 SPI 总线，待实验验证后决定。

## Internal I²C

建议设备：

- RTC
- Fuel Gauge
- IMU
- BME280 或同类环境传感器

建议：

- 预留上拉电阻配置
- 保留地址冲突处理方案
- 必要时加入 I²C Multiplexer

## USB-C System Port

职责：

- 充电
- 固件烧录
- 调试
- 文件同步
- 与 Core 有线连接

## USB-C Expansion Port

职责：

- USB Host
- 键盘
- U 盘
- 外部通信设备
- 其他标准 USB 外设

## CAN Port

职责：

- 能源箱
- 车载系统
- 外部传感器
- 工业扩展模块

实验阶段：

- 3.3 V CAN 控制器侧
- SN65HVD230 或同级收发器
- 终端电阻与供电规则必须明确

## External SPI

默认不开放。

仅在实验版、开发版或维护接口中保留。
