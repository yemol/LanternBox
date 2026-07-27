# FT-02 v1.98 干净主线固件

本版本直接基于用户最后提供、已由 Codex 编译并完成硬件验证的工程整理。

## 稳定 SD 基线

```text
SDMMC 1-bit
频率：5 MHz
CLK -> GPIO35
CMD -> GPIO2
D0  -> GPIO1
CD / D1 / D2 / D3 -> 不接
```

SD 底层使用原生 `esp_vfs_fat_sdmmc_mount()`，保持 Codex 已验证的实现，不改回 Arduino `SD_MMC.begin()`。

## 构建环境

本包只保留 Codex 实际使用的 Arduino 3 / pioarduino 环境，避免 PlatformIO 再去下载旧的 Arduino-ESP32 2.0.17。

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip
board = esp32-s3-devkitc-1
framework = arduino
```

在项目根目录执行：

```bash
~/.platformio/penv/bin/pio run -e esp32-s3-devkitc-1 -t clean
~/.platformio/penv/bin/pio run -e esp32-s3-devkitc-1
~/.platformio/penv/bin/pio run -e esp32-s3-devkitc-1 -t upload
```

## 已移除的测试代码

- SD 32 MiB / 64 MiB 读写压测
- CRC 与测试数据生成
- 启动 R/W Probe
- 地图启动探针
- 启动日志测试写入
- 延迟重复挂载与重复探针
- 测试文件创建和清理

正式固件只执行一次 SD 挂载。地图页面打开时按需读取 `/maps/current/map.cfg` 和 FTM 图块。

## 其他固定引脚

```text
墨水屏：PWR18 BUSY3 RST8 DC9 CS10 MOSI11 SCK12
CardKB2：SDA4 SCL5 地址0x5F
```

地图数据不包含在本固件包内，继续保留 SD 卡现有 `/maps/current/` 目录。
