# FT-02 Hardware Pin Map v1.1

## 版本更新

2026-07-29：CardKB2 新引脚已完成实机验证，正式由 `SDA=GPIO4 / SCL=GPIO5` 迁移为 `SDA=GPIO47 / SCL=GPIO21`。

## 当前稳定生产基线

### SD 卡：SDMMC 1-bit，5 MHz

| SD 信号 | ESP32-S3 GPIO | 状态 |
|---|---:|---|
| CLK | GPIO35 | 当前稳定基线 |
| CMD | GPIO2 | 当前稳定基线 |
| D0 | GPIO1 | 当前稳定基线 |
| CD | NC | 当前不接 |
| D1 | NC | 当前不接 |
| D2 | NC | 当前不接 |
| D3 | NC | 当前不接 |

说明：该组合已经完成稳定读写验证。正式固件不得从地图、UI、日志模块中自行修改这些引脚或重复初始化 SD。

### 4.26 英寸墨水屏

| 信号 | GPIO |
|---|---:|
| PWR | GPIO18 |
| BUSY | GPIO3 |
| RST | GPIO8 |
| DC | GPIO9 |
| CS | GPIO10 |
| MOSI | GPIO11 |
| SCK | GPIO12 |

### CardKB2

| 信号 | GPIO / 地址 |
|---|---:|
| SDA | GPIO47 |
| SCL | GPIO21 |
| I2C 地址 | 0x5F |
| I2C 频率 | 100 kHz |

## 已释放或未分配

GPIO4、GPIO5 已从 CardKB2 输入总线释放。

GPIO14、GPIO15、GPIO16、GPIO17 不再属于当前 SD 主线。未来分配前应重新检查全部外设冲突。

CardKB2 当前锁定规则：

```text
SDA = GPIO47
SCL = GPIO21
ADDR = 0x5F
I2C = 100 kHz
```

该组合已经完成实机按键验证，不得无记录地改回 GPIO4 / GPIO5。

## 未来 4-bit 实验规则

4-bit 只能作为独立实验 profile。新转接卡和新连接线到货后，先在纯 SD 诊断工程中验证，再决定是否进入主固件。不得覆盖当前 1-bit 稳定基线。
