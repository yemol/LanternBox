# FT02_HomeScreen_v1.51_HomeFinalLocked

基于 v1.50_PageIndicatorBigger。

本版为首页样式锁定后的整理版，主要做代码清理与模块拆分，不改变最终视觉布局。

## 首页视觉冻结

顶部状态栏：
- 已抽象为 FT02_StatusBar
- 保持当前布局不变。

底部状态栏：
- 已抽象为 FT02_BottomBar
- 内容锁定为：
  - 编号：FT-02A
  - 版本：v1.30
  - 帮助(H)
- 保持当前布局不变。

首页中间区域：
- 主标题：
  - 壳中灯智能随身终端
  - x=40
  - baselineY=140
- 右侧提示：
  - 方向键选择 确认键进入
  - x=W-310
  - baselineY=140
- card：
  - tileX=32
  - tileY=170
  - tileW=230
  - tileH=100
  - gapX=24
  - gapY=24
- card 文字：
  - ft02_menu_28m
  - x=cardX+90
  - baselineY=cardY+60
- card 图标：
  - 6 个首页 card 均使用 SVG 转换后的 48x48 bitmap 图标
- 分页标记：
  - 当前页 0
  - 总页数 2
  - y=420
  - dotRadius=7
  - dotGap=26

## 本版代码整理

新增：
- src/FT02_HomeContent.h
- src/FT02_HomeContent.cpp
- src/FT02_HomeCards.h
- src/FT02_HomeCards.cpp

清理：
- src/FT02_HomeUI.cpp 现在只负责页面编排：
  - 清屏
  - 顶部状态栏
  - 首页标题区域
  - 首页 card 区域
  - 底部状态栏
- 移除 HomeUI.cpp 中已经不用的旧手绘图标函数：
  - drawBookIcon
  - drawMapIcon
  - drawLogIcon
  - drawSearchIcon
  - drawDeviceIcon
  - drawCommIcon
- 移除 HomeUI.cpp 中的 card 绘制细节，统一放到 FT02_HomeCards.cpp。
- 移除 HomeUI.cpp 中的标题绘制细节，统一放到 FT02_HomeContent.cpp。

保持：
- src/FT02_HomeCardIconData.h 保留 6 个首页图标 bitmap 数据。
- 源 SVG / TTF / OTF 文件未包含在工程包内。
- 顶部状态栏、底部状态栏、字体、图标数据、时间局部刷新逻辑不做额外改动。

## 提交建议

Summary：
锁定 FT-02 首页样式并整理 Home UI 模块

Description：
- 锁定首页顶部、底部、中间 card 区域视觉布局
- 抽出 FT02_HomeContent 负责首页标题区域
- 抽出 FT02_HomeCards 负责 card 网格、图标、分页标记
- 清理 HomeUI.cpp 中旧的手绘图标函数和页面细节
- 保留 6 个 SVG 转换后的首页 card bitmap 图标
- 保持顶部状态栏和底部状态栏已锁定布局不变
