# FT-02 Hardware Pin Map v0.8

## 1. 当前生产候选基线

Codex 隔离测试确认：SDMMC D0 从 GPIO17 改接 GPIO14 后，采用 1-bit 10 MHz 可稳定完成 64 MiB 文件读写。

当前 FT-02 主线固定为：

```text
SD 3.3V -> 3V3_EXT
SD GND  -> GND
SD CLK  -> GPIO15
SD CMD  -> GPIO16
SD D0   -> GPIO14

SD CD   -> NC
SD D1   -> NC
SD D2   -> NC
SD D3   -> NC
```

```text
模式：SDMMC 1-bit
频率：10 MHz
生产 D0：GPIO14
```

GPIO17 已从 SD 主线释放，不得再沿用旧版 `D0=GPIO17` 配置。

## 2. 当前完整 GPIO 分配表

| GPIO | 当前功能 | 状态 | 说明 |
|---:|---|---|---|
| 3 | ePaper BUSY | 锁定 | 墨水屏固定引脚 |
| 4 | CardKB2 SDA | 锁定 | I2C 地址 0x5F |
| 5 | CardKB2 SCL | 锁定 | I2C 地址 0x5F |
| 6 | 空闲 | 可候选 | 当前不参与 SD |
| 7 | 空闲 | 可候选 | 当前不参与 SD |
| 8 | ePaper RST | 锁定 | 墨水屏固定引脚 |
| 9 | ePaper DC | 锁定 | 墨水屏固定引脚 |
| 10 | ePaper CS | 锁定 | 墨水屏固定引脚 |
| 11 | ePaper MOSI | 锁定 | 墨水屏固定引脚 |
| 12 | ePaper SCK | 锁定 | 墨水屏固定引脚 |
| 13 | 空闲 | 可候选 | 当前不参与 SD/CD |
| 14 | SDMMC D0 | 生产候选锁定 | 1-bit 10 MHz 已通过 64 MiB 读写 |
| 15 | SDMMC CLK | 生产候选锁定 | SD 主线 |
| 16 | SDMMC CMD | 生产候选锁定 | SD 主线 |
| 17 | 空闲 | 已释放 | 旧 D0 路径不稳定，不再用于 SD |
| 18 | ePaper PWR | 锁定 | 墨水屏固定引脚 |
| 19 | USB D- | 禁止占用 | USB 固定用途 |
| 20 | USB D+ | 禁止占用 | USB 固定用途 |

## 3. SD 独立模块约束

SD 引脚、模式和频率只允许在以下文件中定义：

```text
lib/FT02Storage/src/FT02_StorageConfig.h
```

地图、UI、日志、录音等业务模块不得直接调用：

```cpp
SD_MMC.setPins(...)
SD_MMC.begin(...)
SD_MMC.open(...)
```

业务代码必须通过 `FT02_Storage` 对外接口访问 SD。

## 4. 未来 4-bit 实验说明

4-bit 当前不属于生产主线。新转接卡和新连接线到货后，建立独立实验 profile，再重新验证：

```text
CLK / CMD / D0 / D1 / D2 / D3
初始化
原始扇区读写
64 MiB 及以上 FAT 读写
冷启动稳定性
```

不得直接覆盖当前已验证的 1-bit 10 MHz 配置，也不得默认复用此前未经最终验证的 D1/D2/D3 排列。
