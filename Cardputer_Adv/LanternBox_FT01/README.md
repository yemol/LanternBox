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


## v0.1.0a 修复

- 修复拆分后 `UiRecorder.cpp` 中 `drawHelpScreen()` 无法访问 `VERSION` 的编译错误。
- `VERSION` 从主入口暴露为外部符号，供 UI 模块读取。


## v0.1.0b 修复

- 修复拆分后 `CARD_GREEN` 链接失败的问题。
- `CARD_GREEN` 是 `UiRecorder.cpp` 的私有 UI 常量，不再从主入口 `extern` 引用。


## v0.1.5 Navigation Module

新增导航模块：

- `UiNavigation.h`
- `UiNavigation.cpp`

实现内容：

1. 读取 `/lanternbox/tracks/path_points.jsonl`
2. 读取 `/lanternbox/base.json`
3. 导航首页显示轨迹总览
4. 相对位置图：
   - 起点
   - 终点
   - 基地
   - 当前点
   - 选中目标
5. 指南针式导航：
   - 北向固定
   - 显示目标方向
   - 显示距离
   - 显示方位角和八方向

导航页按键：

- `R`：重新读取轨迹
- `O`：轨迹总览
- `M`：相对位置图
- `N`：指南针导航
- `B`：选择基地为目标
- `, / ←`：上一个路径点
- `/ / →`：下一个路径点
- `Esc / Del`：返回首页


## v0.1.6 Nav Session List

导航模块升级：

1. 进入 NAV 后先显示轨迹 session 列表。
2. session 来自 `/lanternbox/tracks/path_points.jsonl` 中的 `session_id` 字段。当前仍是一个总 JSONL 文件，不是多个物理文件；导航模块会按 `session_id` 自动分组。
3. session 列表显示：session_id、最近更新时间、路径点数量。
4. 列表按最后出现顺序倒序排列，也就是最近更新的 session 在最前。
5. H 现在打开导航模块自己的帮助页，不再跳到记录页帮助。

导航按键：

- `<>`：选择 session 或路径点
- `Enter`：打开选中的 session
- `L`：返回 session 列表
- `O`：轨迹总览
- `M`：相对位置图
- `N`：指南针导航
- `B`：选择基地作为导航目标
- `R`：重新读取轨迹
- `H`：导航帮助
- `Esc / Del`：返回首页


## v0.1.6a 修复

- 自动记录时如果临时没有 GNSS FIX：跳过本次写入，不中断 session，不关闭 autoTrack。
- session 列表增加日期显示，优先读取 `device_date`，旧记录回退读取 `gnss_utc_date`。
- 新写入的日志和路径点增加 `device_date` 字段。
- 修复 NAV session 列表中 Enter 识别过窄导致的打开失败。


## v0.1.6b 修复

- 修复 `epochToDateString()` 编译错误。
- 补充 `daysInMonth()` 时间辅助函数。
- 不改变 NAV/session/自动记录逻辑。


## v0.1.7 GNSS Move Heading

新增：

1. 地图页图例改为颜色说明：
   - 绿色：起点
   - 橙色：终点
   - 蓝色：基地
   - 红色：当前
   - 白色：目标
   - 灰色：普通路径点

2. 指南针导航页加入 GNSS 移动方向：
   - 设备没有磁力计，所以不是静止电子指南针。
   - 用户移动约 5 米后，系统用 GNSS 位置变化计算移动方向。
   - 当移动方向锁定后，显示目标相对方向：前方、左前、右前、左侧、右侧、左后、右后、后方。

3. 导航状态：
   - `MOVE LOCK`：移动方向已锁定。
   - `N-UP ONLY`：还没有足够移动距离，暂时仅显示北向固定方位。

后续计划：
- v0.1.8 接入 BMI270 IMU 状态页。
- v0.1.9 做 GNSS + IMU 短时方向保持。


## v0.1.7a 修复

- 修复地图页“红 当前”看不到的问题。
- 目标点不再用白色实心点，而是改成白色圆环。
- 当前点保持红色实心点。
- 当当前位置和目标点重叠时，会显示为红色中心 + 白色外圈。


## v0.1.7b 修复

- 地图页统一参考点样式：
  - 蓝色空心圆：基地
  - 红色空心圆：当前位置 / 我的位置
  - 白色空心圆：目标
- 实心点只用于轨迹点：
  - 绿色实心：起点
  - 橙色实心：终点
  - 灰色小点：普通路径点


## v0.1.7c 调整

- 目标点从白色空心圆改回白色实心点。
- 基地和当前位置仍然是空心圆：
  - 蓝色空心圆：基地
  - 红色空心圆：当前位置 / 我的位置
- 目标点作为“要去的位置”，用白色实心点显示，更醒目。


## v0.1.7d 优化

- 地图页普通轨迹点去重显示：
  - 相邻绘制点距离小于等于 1 米时，只画前一个点。
  - 减少 GNSS 漂移或自动记录过密导致的点团。
  - 起点、终点、基地、当前位置、目标点仍会单独绘制，保证关键点可见。

- 地图页增加当前选中目标点信息：
  - 路径点序号
  - 记录时间
  - 纬度
  - 经度

- 切换路径点时，地图右侧信息会随目标点同步变化。


## v0.1.7e 优化

- 地图普通轨迹点去重阈值从 1 米提升到 2 米。
- 地图页移除颜色图例，只保留当前选中目标点信息。
- 地图底部增加 `H说明` 提示。
- NAV 帮助页改为导航专用说明：
  - 地图颜色含义
  - 基本导航按键
  - 不再重复记录页或其他页面的 Help 内容。


## v0.1.7f Help Compact Layout

- 导航帮助页面调整为双列布局。
- 每行两个提示，减少横向拥挤。
- 保留颜色说明和操作说明。


## v0.1.7g Compass Footer Hint Fix

- 修复指南针导航页面底部提示丢失问题。
- 指南针页面现在显示：
  - <> 选择目标点
  - B 基地导航
  - L 返回轨迹列表
  - M 返回地图
  - H 导航说明


## v0.1.7h Nav Help Footer Stable

- 导航 Help 页面移除底部快捷键提示，只保留说明内容。
- 导航功能页面即使没有 GNSS FIX，也始终保留底部操作提示。
- 避免因为状态异常导致用户不知道如何操作页面。


## v0.1.7i Help Clean

- 修复导航 Help 页面仍显示底部快捷键的问题。
- Help 页面现在只显示说明内容，不显示操作 footer。


## v0.1.7j Help Footer Removed

- 修复导航帮助页底部快捷键残留。
- Help 页面现在完全只显示说明内容。
- 操作提示只存在于功能页面 footer。


## v0.1.7k Help Final Clean

- 全局检查导航 Help 页面残留文本。
- 删除最后残留的快捷键说明。
- Help 页面现在只有颜色说明，不包含任何操作提示。


## v0.1.8 IMU Heading Assist

- 接入 Cardputer-Adv 内置 BMI270 六轴 IMU。
- 指南针页面增加 IMU 短时方向辅助。
- 当前方案：
  - GNSS 提供绝对位置和移动方向校准。
  - IMU 陀螺仪提供短时间转向感知。
  - 不作为长期绝对指南针使用，避免漂移累积。

后续：
- GNSS Course + IMU 姿态融合。
- 增加方向校准流程。


## v0.1.9 Help Manager Module

重构 Help 系统：

- 新增 HelpManager.h / HelpManager.cpp。
- Help 根据调用页面显示不同内容。
- 页面 footer 只显示当前操作。
- Help 页面只负责说明，不再混用快捷键提示。

支持上下文：
- HOME
- RECORDER
- NAVIGATION
- DEVICE
- TASK
- KNOWLEDGE
- COMMUNICATION


## v0.1.9a Help Manager Compile Fix

修复模块拆分后的依赖问题：

- HelpManager.h 增加 Arduino String 类型依赖。
- UiNavigation.cpp 显式引入 HelpManager.h。
- HelpManager.cpp 增加 Arduino 头文件依赖。



## v0.1.9b Help Manager Fix

修复：

- 首页 H 不再触发 Help。
- 状态页 H 不再触发 Help。
- 只有明确提供帮助入口的页面才响应 H。
- Help 页面刷新不再回到旧的 drawHelpScreen。
- 记录页调用 HELP_RECORDER。
- 导航页调用 HELP_NAVIGATION。

HelpManager 成为唯一 Help 渲染入口。


## v0.1.9c Help Navigation Fix

修复：

- 导航 Help 原来只调用 showHelp，但没有切换 SCREEN_HELP。
- 导致 Help 页面显示后下一次刷新又回到导航页面。
- 新增统一 openHelpPage()。
- 所有页面通过统一入口进入 Help。


## v0.1.9d Help Navigation Bridge Fix

修复 C++ 模块隔离问题：

- UiNavigation.cpp 不再直接访问主程序内部的 SCREEN_NAV 枚举。
- 增加 openNavigationHelp() 桥接函数。
- 模块之间通过接口通信，避免跨文件依赖。


## v0.1.9e Help Stable Transition Fix

修复：

- H 打开 Help 后因为键盘重复输入立即关闭的问题。
- Help 页面不再把 H 作为退出键。
- Help 只使用 ESC / DEL / Enter 返回。
- 导航帮助恢复颜色区分显示。


## v0.1.9f Help Color Dependency Fix

- 修复 HelpManager 依赖 UiNavigation 私有颜色常量的问题。
- HelpManager 现在拥有自己的颜色定义，不依赖其他模块。


## v0.1.9g Context Help Navigation Fix

- 轨迹选择页面取消 H 帮助入口。
- 只有地图页和指南针页提供 Help。
- 新增：
  - HELP_NAV_MAP
  - HELP_NAV_COMPASS
- Help 内容根据具体页面变化。


## v0.1.9h Context Help Content Fix

修复：

- 导航 Help 触发页面正确，但内容被 HELP_NAVIGATION 覆盖。
- 修正导航 Help 调用链。
- 地图页显示 HELP_NAV_MAP。
- 指南针页显示 HELP_NAV_COMPASS。


## v0.1.9i Help Bridge Link Fix

修复链接错误：

- UiNavigation.cpp 调用带参数 openNavigationHelp(HelpType)
- 主程序导出同样签名的函数
- 解决 undefined reference


## v0.1.9j Help Bridge Final Link Fix

- 强制统一 openNavigationHelp(HelpType) 函数签名。
- 修复 UiNavigation.cpp 链接不到桥接函数的问题。


## v0.1.9k Help Navigation Return Fix

修复：

- 导航 Help 可以正确进入。
- 原因：
  - openNavigationHelp 已切换 SCREEN_HELP。
  - 但 handleNavKey 继续执行 drawNavScreen 覆盖了 Help。
- 增加 return 防止覆盖。


## v0.2.0a Log Module Skeleton

新增日志模块基础结构：

- UiLog.h
- UiLog.cpp

首页：
- 卡片模块改为日志模块。

当前功能：
- 日志入口
- 独立页面
- 后续接入 AudioLogger

下一阶段：
- AudioLogger
- WAV录音
- GPS绑定
- SD索引
- 回听


## v0.2.0a1 Log Module Exit Fix

修复：
- 日志页面 ESC 无法退出。
- 增加多种 Cardputer ESC/DEL 输入格式兼容。


## v0.2.0a2 Log Exit Fix
- 日志模块退出改为通过主程序状态桥接返回首页。
