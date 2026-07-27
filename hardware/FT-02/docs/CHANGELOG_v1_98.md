# FT-02 v1.98 更新记录

## 存储基线

- 固定为 SDMMC 1-bit、5 MHz。
- CLK 更新为 GPIO35。
- CMD 更新为 GPIO2。
- D0 更新为 GPIO1。
- 保留 Codex 已验证的原生 ESP-IDF VFS SDMMC 后端。

## 清理

- 删除全部 SD 大文件压测与 CRC 测试。
- 删除地图启动探针。
- 删除启动日志测试写入。
- 删除延迟重复存储探针。
- 删除测试专用结果枚举和错误状态。
- 正式固件只挂载一次 SD，地图按需读取。

## 构建

- 仅保留 Codex 实际验证的 pioarduino Arduino 3 环境。
- 移除会触发 Arduino-ESP32 2.0.17 下载的旧 PlatformIO 环境。

## 文档

- 新增 `FT02_hardware_pin_map_v1_0.md`。
- 更新存储架构和地图文档中的 SD 引脚。
