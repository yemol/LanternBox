# FT-02 Changelog

## v2.75k - Documentation & Help Cleanup

- 整理工程根目录，历史验收文档归档至 `docs/history/`。
- 重写主 `README.md`，只保留当前架构、功能、编译和文档入口。
- 新增用户说明、编译刷机说明、发布检查清单。
- Host Protocol A2 文档同步 QMC5883L Calibration Extension。
- 设备内帮助页增加“状态页 K：罗盘校准”提示。
- 保持 Core / LR01 单一硬件权威架构不变。

## v2.75j1 - Compass Calibration A1 Compile Fix

- 修复 `FT02_OpenCompassCalibrationPage()` 缺少前置声明导致的编译错误。

## v2.75j - Compass Calibration A1

- Core 增加罗盘校准 UI。
- 接入 LR01 `COMPASS_CAL_*` Host Protocol 扩展。
- 支持 START / STATUS / SAVE / CANCEL / RESET。
- 重置已保存校准必须二次确认。
- 系统自检增加罗盘校准质量提示。

## v2.75i2 - Version Sync Fix

- 修复固件权威版本号未随包版本更新的问题。

## v2.75i - Map Status Sync A2

- GNSS 状态栏与实时 Fix 状态同步。
- GNSS 跟随时隐藏地图中心十字，只保留真实定位标记。

## v2.75h - Map Position Arrow A1

- 地图位置标记升级为方向箭头。
- 支持原地 heading 变化局部刷新。

## v2.75g - System Self-Test A2

- 系统自检扩展为 3 页。
- 增加 Host UART、存储、内存、通讯错误和启动状态检查。

更早版本的详细验收记录见 `docs/history/`。
