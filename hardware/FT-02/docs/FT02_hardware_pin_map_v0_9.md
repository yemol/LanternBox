# FT-02 Hardware Pin Map v0.9

## 1. 当前冻结基线

### SD 卡：SDMMC 1-bit 5 MHz

| SD 信号 | ESP32-S3 GPIO | 状态 |
|---|---:|---|
| CLK | GPIO35 | 当前稳定基线 |
| CMD | GPIO2 | 当前稳定基线 |
| D0 | GPIO1 | 当前稳定基线 |
| CD | 不接 | 当前未使用 |
| D1 | 不接 | 1-bit 模式未使用 |
| D2 | 不接 | 1-bit 模式未使用 |
| D3 | 不接 | 1-bit 模式未使用 |
| VCC | 3V3_EXT | 3.3V 供电 |
| GND | GND | 共地 |

配置：

```text
SDMMC 1-bit
5 MHz
CLK35 / CMD2 / D0-1
```

Codex 隔离工程已完成 64 MiB 稳定读写验证。当前主固件必须保持这套配置，不允许地图、UI、日志或录音模块单独改动。

### 4-bit 后续实验

4-bit 当前不属于生产基线。待新转接卡和连接线到货后，单独建立实验 profile，再确定 D1、D2、D3 的最终 GPIO。实验结果未完成全部读写验收前，不得覆盖当前 1-bit 配置。

## 2. 墨水屏：Waveshare 4.26 英寸

| 信号 | ESP32-S3 GPIO |
|---|---:|
| PWR | GPIO18 |
| BUSY | GPIO3 |
| RST | GPIO8 |
| DC | GPIO9 |
| CS | GPIO10 |
| MOSI | GPIO11 |
| SCK | GPIO12 |
| VCC | 3V3 |
| GND | GND |

## 3. CardKB2

| 信号 | ESP32-S3 GPIO |
|---|---:|
| SDA | GPIO4 |
| SCL | GPIO5 |
| I2C 地址 | 0x5F |

## 4. 当前未占用或已释放

下列引脚不再属于当前 SD 主线：

```text
GPIO14
GPIO15
GPIO16
GPIO17
```

它们是否可分配给后续硬件，仍需结合开发板实际引出、启动约束和其他模块规划再次确认。

## 5. 固定规则

- SD 底层配置只允许在 `lib/FT02Storage/src/FT02_StorageConfig.h` 中定义。
- 墨水屏固定引脚不得为 SD 复用。
- CardKB2 固定使用 GPIO4 / GPIO5。
- 不允许业务模块直接调用 SDMMC 初始化接口。
- 更换 SD 引脚后必须同步更新本文件和存储配置文件。
