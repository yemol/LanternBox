# FT-02 v1.91 Help Restored 验证说明

## 已完成检查

- `H / h` 仍由 CardKB2 输入层映射为 `FT02_KEY_HELP`
- 首页收到 `FT02_KEY_HELP` 后切换到 Help 页面
- Help 页面收到 `B / b`、Esc 或 Backspace 后返回首页
- 返回首页后保留原首页卡片选中位置
- 页面完整刷新后强制恢复当前时间、日期与 SD 顶部状态
- Help 页面所用中英文字形均已存在于当前字体包
- 全部 `src/*.cpp` 已通过主机端 C++17 语法检查

## 未改变

- CardKB2 SDA=GPIO4 / SCL=GPIO5 / 地址 0x5F
- ePaper 引脚与驱动
- 首页冻结布局
- 首页卡片局部刷新逻辑
- SD 存储模块及其接线
- 顶部和底部状态栏几何布局

## 实机验收

1. 烧录后首页正常出现。
2. 按 `H`，显示 Help 页面。
3. Help 页面显示 D/Z/X/C、Enter/Space、H、B/Esc 对应说明。
4. 按 `B` 返回首页。
5. 再次按 `H` 进入 Help，然后按 Esc 返回首页。
6. 返回首页后卡片选中位置不变。
7. 顶部时间和 SD 状态未恢复成占位值。

> 当前环境没有安装 PlatformIO ESP32 工具链，因此完成的是全源文件主机端语法检查，不等同于真实 ESP32-S3 编译与实机验收。
