# LanternBox FT-01 v0.1.0 Modular Base

这是从 v0.0.9p 拆分出来的第一个模块化基线。

## 文件结构

- `LanternBox_FT01.ino`
  - 主入口
  - SD 初始化
  - GNSS / NMEA 解析
  - session / 自动记录 / 路径点写入
  - 键盘输入和页面调度

- `UiRecorder.h`
  - 路径记录页面 UI 模块头文件

- `UiRecorder.cpp`
  - 路径记录页面绘制逻辑
  - 包含四张中文信息卡片、顶部 AUTO / SD / 电量状态、底部坐标和时间

## 当前首页模块调整

- `记录 / LOG` 已改为 `路径 / LOG`
- `电量 / BAT` 已改为 `导航 / NAV`

## 重要硬件规则

LoRa/GNSS Cap 的 LoRa 与 SD 共享 SPI 总线。  
初始化 SD 前必须保持 LoRa NSS 高电平，否则 SD 可能失败。

## 后续开发规则

从本版本开始，后续优先按模块推进：

1. 已拆出的模块，除非修改该模块本身，不再随意改动。
2. 下一步可以拆 `UiHome`，或者继续实现 `NAV / TRACK`。
3. SD / GNSS / session 逻辑暂时仍在主入口内，等其稳定后再拆。
