# FT-02 Hardware Pin Map v0.5

## 修正记录

### v0.5

本版本基于 `FT02_hardware_pin_map_v0_4.md` 更新，保留 v0.4 已修正的 CardKB2 接线记录，并新增已经实机验证通过的 SD 存储接线基线。

新增内容：

- 增加 Waveshare Micro SD Storage Board 接线记录
- 锁定 SD 存储为 `SPI Mode3 + 独立 SPIClass(FSPI)` 路线
- 锁定 SD D3 / CS 从 GPIO14 调整为 GPIO6
- 增加 SD CD 插卡检测 GPIO7
- 增加 SD 容量读取验证记录
- 更新当前 GPIO 总表
- 更新当前 FT-02 状态
- 更新后续模块预留说明

已验证 SD 测试结果：

```text
Status: SD INFO OK
CD:0 Cap:29823MB
Sig:55 AA
```

结论：

```text
GPIO7 的 CD 检测有效，当前板子 CD=0 表示已插卡。
32G SD 卡容量可从 CSD 读取，显示约 29823MB 属正常换算结果。
block0 读取成功，签名 55 AA 正常。
```

### v0.4

本版本修正 CardKB2 接线记录。

之前版本错误地把 ESP32-S3 GPIO 编号和 CardKB2 标注混淆。

实际测试接线：

| CardKB2 标注 | ESP32-S3 GPIO |
|---|---:|
| G25 | GPIO5 |
| G26 | GPIO4 |

------------------------------------------------------------------------

# 当前已验证系统

## ESP32-S3

### USB限制

| 功能 | GPIO |
|---|---:|
| USB D- | GPIO19 |
| USB D+ | GPIO20 |

GPIO19 / GPIO20 不用于普通外设。

说明：

```text
GPIO19 / GPIO20 与 ESP32-S3 原生 USB 相关。
在 FT-02 原型中禁止作为普通 I2C / SPI / UART / GPIO 外设引脚使用。
```

------------------------------------------------------------------------

# 墨水屏 ePaper（已验证 / 锁定）

模块：

```text
Waveshare 4.26 英寸黑白墨水屏 HAT
GxEPD2 驱动：GxEPD2_426_GDEQ0426T82
分辨率：800 × 480
```

接线：

| 信号 | ESP32-S3 GPIO | 状态 | 说明 |
|---|---:|---|---|
| PWR | GPIO18 | 已验证 / 锁定 | 墨水屏供电控制，高电平开启 |
| BUSY | GPIO3 | 已验证 / 锁定 | 墨水屏忙碌检测 |
| RST | GPIO8 | 已验证 / 锁定 | 墨水屏复位 |
| DC | GPIO9 | 已验证 / 锁定 | 数据 / 命令选择 |
| CS | GPIO10 | 已验证 / 锁定 | 墨水屏片选 |
| MOSI / DIN | GPIO11 | 已验证 / 锁定 | 墨水屏 SPI MOSI |
| SCK / CLK | GPIO12 | 已验证 / 锁定 | 墨水屏 SPI SCK |

固件初始化基线：

```cpp
pinMode(18, OUTPUT);
digitalWrite(18, HIGH);

SPI.begin(12, -1, 11, 10);
display.init(115200);
display.setRotation(2);
```

状态：

- 驱动成功
- 刷新成功
- 顶部状态栏 / 首页 / 底部状态栏均已基于该屏幕稳定显示

------------------------------------------------------------------------

# CardKB2（已验证 / 锁定）

设备：

```text
M5Stack Unit CardKB2
```

通信：

```text
I2C
```

地址：

```text
0x5F
```

实际连接：

| CardKB2 | ESP32-S3 |
|---|---:|
| GND | GND |
| VCC | 3.3V |
| G25 | GPIO5 |
| G26 | GPIO4 |

ESP32-S3 代码：

```cpp
Wire.begin(4, 5);
```

即：

| 功能 | ESP32-S3 GPIO |
|---|---:|
| I2C SDA / CardKB2 G26 | GPIO4 |
| I2C SCL / CardKB2 G25 | GPIO5 |

测试结果：

```text
FT-02 CardKB2 I2C Scan Start
I2C Init OK
Found Device: 0x5F
Device Count: 1
Scan Finished
```

当前输入映射：

| CardKB2 按键 | FT-02 输入 |
|---|---|
| D / d | 上 |
| Z / z | 左 |
| X / x | 下 |
| C / c | 右 |
| Enter / Space | 确认 |
| H / h | 帮助 |
| Esc / Backspace / B / b | 返回 |

重要规则：

```text
不要再改回 SDA=GPIO5 / SCL=GPIO4。
不要默认开启自动扫引脚。
CardKB2 固定使用 GPIO4 / GPIO5，地址 0x5F。
```

------------------------------------------------------------------------

# microSD / TF 存储模块（已验证 / 锁定）

设备：

```text
Waveshare Micro SD Storage Board
```

当前接线模式：

```text
SPI 模式
独立 SPIClass(FSPI)
SPI Mode3
```

接线：

| SD 模块信号 | ESP32-S3 GPIO | SPI 含义 | 状态 |
|---|---:|---|---|
| 3.3V | 3V3_EXT | 电源 | 已验证 |
| GND | GND | 共地 | 已验证 |
| CD | GPIO7 | 插卡检测 | 已验证 / 锁定 |
| CLK | GPIO15 | SCK | 已验证 / 锁定 |
| CMD | GPIO16 | MOSI | 已验证 / 锁定 |
| D0 | GPIO17 | MISO | 已验证 / 锁定 |
| D3 | GPIO6 | CS | 已验证 / 锁定 |
| D1 | 不接 | - | SPI 模式不使用 |
| D2 | 不接 | - | SPI 模式不使用 |

最终接线基线：

```text
SD 3.3V -> 3V3_EXT
SD GND  -> GND
SD CD   -> GPIO7

SD CLK -> GPIO15
SD CMD -> GPIO16
SD D0  -> GPIO17
SD D3  -> GPIO6

SD D1 -> 不接
SD D2 -> 不接
```

已验证结果：

```text
Status: SD INFO OK
CD:0 Cap:29823MB
Sig:55 AA
```

说明：

```text
CD:0 表示当前板子检测到已插卡。
Cap:29823MB 来自 CSD 容量解析，对 32G 卡属于正常显示。
Sig:55 AA 表示 block0 读取成功。
```

协议与固件基线：

```text
使用独立 SPIClass(FSPI)
使用 SPI Mode3
不使用 SD_MMC
不使用 SD.begin()
当前阶段只读：CD + CSD 容量 + block0 签名
```

重要规则：

```text
SPI 模式下，SD D3 是 CS。
SD D1 / D2 不参与 SPI 模式。
GPIO14 已释放，不再作为 SD CS。
不要回退到 SDMMC 4-bit 作为短期主线。
```

已废弃 / 暂不采用路线：

| 路线 | 结果 | 结论 |
|---|---|---|
| SDMMC 1-bit | 失败 | 不作为当前主线 |
| SDMMC 4-bit | SDMMC_MOUNT_FAILED | 不作为当前主线 |
| 全局 SPI 对象 | 曾卡在 PROBING | 不作为当前主线 |
| bitbang SPI Mode3 | 成功 | 可保留为诊断底座 |
| 独立 SPIClass(FSPI) Mode3 | 成功 | 当前正式主线 |

------------------------------------------------------------------------

# 当前 GPIO 规划总表

| ESP32-S3 GPIO | 功能 | 状态 |
|---:|---|---|
| GPIO3 | ePaper BUSY | 已验证 / 锁定 |
| GPIO4 | I2C SDA / CardKB2 G26 | 已验证 / 锁定 |
| GPIO5 | I2C SCL / CardKB2 G25 | 已验证 / 锁定 |
| GPIO6 | SD D3 / CS | 已验证 / 锁定 |
| GPIO7 | SD CD | 已验证 / 锁定 |
| GPIO8 | ePaper RST | 已验证 / 锁定 |
| GPIO9 | ePaper DC | 已验证 / 锁定 |
| GPIO10 | ePaper CS | 已验证 / 锁定 |
| GPIO11 | ePaper MOSI | 已验证 / 锁定 |
| GPIO12 | ePaper SCK | 已验证 / 锁定 |
| GPIO13 | 空闲 | 可候选普通 GPIO |
| GPIO14 | 空闲 | 已从 SD CS 释放，可候选普通 GPIO |
| GPIO15 | SD CLK / SCK | 已验证 / 锁定 |
| GPIO16 | SD CMD / MOSI | 已验证 / 锁定 |
| GPIO17 | SD D0 / MISO | 已验证 / 锁定 |
| GPIO18 | ePaper PWR | 已验证 / 锁定 |
| GPIO19 | USB D- | 禁止占用 |
| GPIO20 | USB D+ | 禁止占用 |

------------------------------------------------------------------------

# 谨慎 / 禁用 GPIO

| GPIO | 原因 | 规则 |
|---:|---|---|
| GPIO19 | USB D- 相关 | 不用于普通外设 |
| GPIO20 | USB D+ 相关 | 不用于普通外设 |
| GPIO43 | UART0 TX 常用 | 谨慎使用，避免影响烧录 / 串口 |
| GPIO44 | UART0 RX 常用 | 谨慎使用，避免影响烧录 / 串口 |
| GPIO0 | Boot / 下载模式相关 | 谨慎使用 |
| GPIO45 | 启动配置相关 | 谨慎使用 |
| GPIO46 | 启动配置相关 | 谨慎使用 |

说明：

```text
可用 GPIO 必须结合实际 ESP32-S3 DevKit N16R8 板子丝印确认。
不要只根据芯片资料表分配引脚。
```

------------------------------------------------------------------------

# 供电基线

当前原则：

```text
ESP32-S3 DevKit -> USB-C 供电
外设 3.3V       -> 外部 3.3V DC-DC 稳压模块
所有 GND        -> 共地
```

重要规则：

```text
不要在 ESP32-S3 通过 USB 供电时，把外部 3.3V 直接反灌到 ESP32-S3 的 3V3 引脚。
外部 3.3V 只作为外设供电总线。
所有模块必须共地。
```

建议电容：

```text
3V3_EXT 总线：100uF ~ 470uF
SD 模块附近：100nF + 10uF / 47uF
```

------------------------------------------------------------------------

# 当前 FT-02 状态

```text
ESP32-S3
 |
 +-- ePaper SPI
 |    PWR  -> GPIO18
 |    BUSY -> GPIO3
 |    RST  -> GPIO8
 |    DC   -> GPIO9
 |    CS   -> GPIO10
 |    MOSI -> GPIO11
 |    SCK  -> GPIO12
 |
 +-- CardKB2 I2C
 |    CardKB2 G26 / SDA -> GPIO4
 |    CardKB2 G25 / SCL -> GPIO5
 |    Address 0x5F
 |
 +-- microSD SPI Mode3
      CD        -> GPIO7
      D3 / CS   -> GPIO6
      CLK / SCK -> GPIO15
      CMD/MOSI  -> GPIO16
      D0 / MISO -> GPIO17
      D1 / D2   -> NC
```

已完成：

- 显示输出
- 串口调试
- CardKB2 I2C 通信
- CardKB2 按键读取
- 首页卡片切换
- SD 插卡检测
- SD 容量读取
- SD block0 读取
- SD `55 AA` 签名验证

下一步：

- 将 SD 状态模块接入首页顶部状态栏
- 继续推进 FAT32 / 文件系统读取
- 后续再做日志落盘与剩余容量统计
- GNSS
- LoRa
- 音频

------------------------------------------------------------------------

# 后续模块预留登记区

| 模块 | 计划信号 | 候选 GPIO | 状态 | 备注 |
|---|---|---:|---|---|
| GNSS | UART TX/RX | 待定 | 未分配 | 避免 GPIO43 / GPIO44 |
| LoRa | SPI / UART | 待定 | 未分配 | 不与 ePaper / SD SPI 混用，除非明确设计总线仲裁 |
| Audio | I2S / DAC / AMP | 待定 | 未分配 | 需单独评估 I2S 引脚 |
| Camera | DVP / SPI | 待定 | 未分配 | 引脚需求多，需单独规划 |
| Sensor | I2C | 可共用 GPIO4/5 或新 I2C | 未定 | 若共用 I2C，需确认地址不冲突 |
| Buzzer / Vibration | GPIO / PWM | GPIO13 / GPIO14 可候选 | 未分配 | 需要实物测试确认 |

------------------------------------------------------------------------

# 调试原则

1. 先验证 GPIO，再连接模块。
2. ESP32-S3 USB 相关 GPIO 不能复用。
3. 每次只增加一个硬件变量。
4. 保留已验证硬件基线。
5. 新增模块前必须先更新本引脚表。
6. 修改固件引脚常量时，必须同步更新本文档。
7. SD / ePaper / CardKB2 当前引脚除非进入明确的硬件重排阶段，否则视为冻结。

------------------------------------------------------------------------

# 文档维护规则

任何后续引脚修改必须同步更新：

```text
1. 本文档
2. 固件中的 pin 常量
3. 对应版本 README / 接线说明
4. 实物接线标签
5. 验证记录
```

不得只改代码不改文档，也不得只改文档不验证硬件。
