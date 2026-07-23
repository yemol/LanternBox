# FT-02 Hardware Pin Map v0.4

## 修正记录

本版本修正 CardKB2 接线记录。

之前版本错误地把 ESP32-S3 GPIO 编号和 CardKB2 标注混淆。

实际测试接线：

  CardKB2 标注   ESP32-S3 GPIO
  -------------- ---------------
  G25            GPIO5
  G26            GPIO4

------------------------------------------------------------------------

# 当前已验证系统

## ESP32-S3

### USB限制

  功能     GPIO
  -------- --------
  USB D-   GPIO19
  USB D+   GPIO20

GPIO19/GPIO20 不用于普通外设。

------------------------------------------------------------------------

# 墨水屏 ePaper（已验证）

  信号   ESP32-S3 GPIO
  ------ ---------------
  PWR    GPIO18
  BUSY   GPIO3
  RST    GPIO8
  DC     GPIO9
  CS     GPIO10
  MOSI   GPIO11
  SCK    GPIO12

状态：

-   驱动成功
-   刷新成功

------------------------------------------------------------------------

# CardKB2（已验证）

设备：

M5Stack Unit CardKB2

通信：

I2C

地址：

    0x5F

实际连接：

  CardKB2   ESP32-S3
  --------- ----------
  GND       GND
  VCC       3.3V
  G25       GPIO5
  G26       GPIO4

ESP32-S3代码：

``` cpp
Wire.begin(4,5);
```

测试结果：

    FT-02 CardKB2 I2C Scan Start
    I2C Init OK
    Found Device: 0x5F
    Device Count: 1
    Scan Finished

------------------------------------------------------------------------

# 当前GPIO规划

  ESP32-S3 GPIO   功能                    状态
  --------------- ----------------------- ----------
  GPIO3           ePaper BUSY             已验证
  GPIO4           I2C SDA / CardKB2 G26   已验证
  GPIO5           I2C SCL / CardKB2 G25   已验证
  GPIO8           ePaper RST              已验证
  GPIO9           ePaper DC               已验证
  GPIO10          ePaper CS               已验证
  GPIO11          ePaper MOSI             已验证
  GPIO12          ePaper SCK              已验证
  GPIO18          ePaper PWR              已验证
  GPIO19          USB D-                  禁止占用
  GPIO20          USB D+                  禁止占用

------------------------------------------------------------------------

# 当前FT-02状态

    ESP32-S3
     |
     +-- ePaper SPI
     |
     +-- CardKB2 I2C
          CardKB2 G26 -> ESP GPIO4
          CardKB2 G25 -> ESP GPIO5
          Address 0x5F

已完成：

-   显示输出
-   串口调试
-   CardKB2通信

下一步：

-   CardKB2按键读取
-   输入事件处理
-   GNSS
-   LoRa
-   SD存储
