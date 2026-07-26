# FT02 v1.90 SDMMC 1-bit File R/W From v1.87 NoRetry

基于 `FT02_v1_87_SDMMC1Bit_TopStatus.zip`，不是基于 v1.88。

## 为什么回到 v1.87

v1.88 引入了多次 `SD_MMC.end()` / `setPins()` / `begin()` 重试。  
实际测试表明这反而可能让 SDMMC 状态更不稳定，导致 `SD ERR`。

所以 v1.90 的原则是：

```text
保留 v1.87 已经成功的单次 SDMMC 1-bit mount 路径
只在 mount 成功之后增加文件写入 / 读取烟测
不继承 v1.88 的重试逻辑
```

## 接线

```text
SD 3.3V -> 3V3_EXT
SD GND  -> GND
SD CD   -> GPIO7

SD CLK -> GPIO15
SD CMD -> GPIO16
SD D0  -> GPIO17

SD D3 -> 不接
SD D1 -> 不接
SD D2 -> 不接
```

## 本版测试内容

5 秒后自动：

```text
1. SD_MMC 1-bit 挂载
2. 读取 card size / total / used
3. 写入 /ft02_rw_test.txt
4. 重新读取 /ft02_rw_test.txt
5. 校验内容是否包含 FT02_RW_TEST_V1_90
6. 成功后顶部显示 剩余容量 / RW OK
```

## 顶部成功显示

```text
29G
RW OK
```

## 串口成功关键行

```text
SD_MMC.begin OK
File R/W smoke OK
SD file R/W result: RW_OK
FT02 SDMMC 1-bit READY + FILE RW OK
```

## 注意

本版会在 SD 卡根目录创建或覆盖：

```text
/ft02_rw_test.txt
```

---

# v1.91 Help Restored

本版本在 v1.90 基础上恢复被遗漏的 Help 页面，不改变现有硬件引脚、CardKB2 接线、首页布局或 SD 存储实现。

## 修复内容

```text
H / h              -> 从首页进入 Help 页面
B / b              -> 从 Help 返回首页
Esc                 -> 从 Help 返回首页
Backspace           -> 从 Help 返回首页
```

新增模块：

```text
src/FT02_PageState.h
src/FT02_HelpUI.h
src/FT02_HelpUI.cpp
```

页面切换结构：

```text
HOME --H--> HELP
HELP --B / ESC / Backspace--> HOME
```

返回首页后会立即恢复：

- 当前时间与日期
- 当前 SD 顶部状态
- 原来的首页卡片选中位置

## 未修改内容

- ePaper 引脚
- CardKB2 SDA=GPIO4 / SCL=GPIO5 / 0x5F
- 首页卡片布局与局部刷新
- SD 模块实现与接线
- 顶部状态栏布局
- 底部状态栏几何布局
