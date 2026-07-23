# FT02_HomeScreen_v1.23_RealClockDate

本版本在 v1.22 局部刷新测试成功的基础上，进入真实计时测试。

新增：
- 时间按真实经过时间刷新
- 日期同步刷新
- 日期支持跨日滚动
- 只刷新左上角时间/日期区域，不整屏刷新

注意：
- 当前没有 RTC 硬件，也没有联网校时。
- 当前时间基准来自编译时间：__DATE__ / __TIME__
- 启动后使用 millis() 推进时间。
- 断电重启后会回到固件编译时间附近。
- 后续接入 RTC / GPS / 手动设置时间后，只需要替换时间源，不需要改 UI 坐标。

冻结视觉坐标：
- 时间 x=30, baselineY=44
- 日期 x=135, baselineY=44
- 状态栏底线 y=73~75, thickness=3
- blockStart=225
- blockWidth=150
- iconOffsetX=7
- iconY=16
- iconSize=32
- textOffsetX=52
- line1 baselineY=34
- line2 baselineY=58

局部刷新窗口：
- x=20
- y=8
- w=220
- h=52

模块坐标：
- LoRa: blockX=225, iconX=232, textX=277
- GPS: blockX=375, iconX=382, textX=427
- SD: blockX=525, iconX=532, textX=577
- Battery: blockX=675, iconX=682, textX=727
