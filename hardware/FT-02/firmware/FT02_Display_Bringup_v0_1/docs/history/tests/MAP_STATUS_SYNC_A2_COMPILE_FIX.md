# v2.75i1 Compile Fix

修复 `FT02_PbfMapUI.cpp` 中 `g_ft02MapOverlayFollowGnss` 在首次使用前未声明导致的编译错误。

修复方式：
- 在文件级静态状态区增加：
  `static bool g_ft02MapOverlayFollowGnss = true;`

功能逻辑保持 v2.75i 不变：
- GNSS 有效且跟随开启时隐藏地图中心十字。
- 手动浏览/关闭跟随时显示地图中心十字。
