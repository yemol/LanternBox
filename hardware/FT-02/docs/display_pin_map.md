# FT-02 墨水屏引脚映射文档 v0.1

## 1. 文档说明

本文档定义 FT-02 现场终端主控（ESP32-S3）与 Waveshare 4.26 英寸电子墨水屏模块之间的硬件连接关系。

该引脚映射已经完成实际硬件连接测试，并验证：

- ESP32-S3 启动正常
- SPI 通信正常
- 墨水屏初始化正常
- 全屏刷新正常
- 文本显示正常

**状态：已冻结（FROZEN）**

后续 FT-02 固件开发、PCB设计、结构设计必须以本文档作为显示模块硬件基线。

---

# 2. 墨水屏模块信息

## 显示屏

- 厂商：Waveshare
- 型号：4.26 英寸 e-Paper HAT
- 分辨率：800 × 480
- 驱动芯片：GDEQ0426T82 / SSD1677兼容
- 驱动库：GxEPD2
- 通信方式：
  - SPI
  - GPIO控制信号

---

# 3. 最终固定引脚映射

| 墨水屏接口 | 功能 | ESP32-S3 |
|---|---|---|
| VCC | 电源输入 | 3V3 |
| GND | 电源地 | GND |
| DIN | SPI MOSI 数据输入 | GPIO11 |
| CLK | SPI SCK 时钟 | GPIO12 |
| CS | SPI片选 | GPIO10 |
| DC | 数据/命令切换 | GPIO9 |
| RST | 屏幕复位 | GPIO8 |
| BUSY | 忙状态检测 | GPIO3 |
| PWR | 屏幕电源控制 | GPIO18 |

---

# 4. 固件引脚定义

PlatformIO / Arduino 固件中固定：

```cpp
constexpr int EPD_PWR  = 18;

constexpr int EPD_BUSY = 3;
constexpr int EPD_RST  = 8;
constexpr int EPD_DC   = 9;
constexpr int EPD_CS   = 10;

constexpr int EPD_MOSI = 11;
constexpr int EPD_SCK  = 12;