# LanternBox FT-02 项目说明与提交指南

**当前预览基线：FT-02 v2.75k22 / FieldManual Runtime A3.15 Dynamic Split Visual Preview**  
**日期：2026-08-24**

本文是当前 FT-02 工程的主要说明文件，合并原来的用户操作、编译刷机、FieldManual 部署/媒体规范、发布检查和当前验证记录。历史测试与审计仍保留在 `docs/history/`，但不作为当前操作入口。

---

## 1. 当前架构基线

FT-02 是 LanternBox / 壳中灯的随身终端，采用 Core + LR01 双处理器架构。

### Core ESP32-S3

负责：

- UI 与 800×480 电子纸显示
- CardKB2 输入
- SD 存储
- FieldManual / 知识库
- PBF 离线地图与搜索
- 定位记录、语音日志
- 内部通讯 UI 与应用逻辑

### LR01

负责：

- GNSS
- QMC5883L 罗盘
- Meshtastic / LoRa
- Navigation State
- Radio State

Core 不直接解析 NMEA，不直接访问 QMC5883L，也不直接控制 SX126x。

Core ↔ LR01 固定为：

```text
Core GPIO13 TX -> LR01 GPIO18 RX
Core GPIO7  RX <- LR01 GPIO17 TX
Core GPIO6 RESET_N -> LR01 RST / CHIP_PU / EN
115200 8N1, 3.3 V logic
```

完整协议与硬件约束见：

- `docs/LR01_HOST_PROTOCOL_A2.md`
- `docs/FT02_LR01_HOST_INTERFACE_PIN_LOCK.md`

---

## 2. 当前版本能力


### v2.75k22 / A3.15 Dynamic Split Visual Preview

针对 A3.14 实机出现的“图片完整但未吃满左栏”问题，本版从根上调整：Split 媒体允许等比放大；测试媒体去除无效白边；左右栏按实际媒体宽高比动态分配。旧媒体版式完全兼容。

### v2.75k21 / A3.14 Split Visual Preview

本预览版在不改变 `FT02FieldManualMediaIndex` 84-byte 二进制布局的前提下，新增 media flag bit1。设置后 Reader 使用“左图右文”：左侧位图完整等比 Fit，右侧显示图解页码与 caption。旧 bit0 `caption_below` 和无 flag 媒体继续按原版式显示。预览批次只迁移 6 张真实卡、18 个图解页，用于实机判断空间比例和可读性。

v2.75k20 基于 v2.75k19，修复 FieldManual 搜索连续输入时在电子纸 BUSY 边界上偶发的字符顺序颠倒。Knowledge 不再保留第二套私有 BUSY 输入队列，所有页面统一使用 Global EPD Interactive Runtime A2 的单一严格 FIFO；主循环先消费旧队列事件，再读取新的实时按键。帮助页 CJK 修复、FieldManual v1.3.1 内容、地图、日志、通讯和 LR01 架构保持不变。

主要能力：

- PBF 离线地图、地图搜索、GNSS 跟随和方向箭头
- LR01 单一权威 GNSS / Compass / Meshtastic 状态
- CardKB2 离线拼音输入与本地词频学习
- 语音日志、定位记录、轨迹查看
- Meshtastic 广播、私信、节点列表和可靠投递状态
- 三页系统自检与 LR01 罗盘校准 UI
- FieldManual A3 图文行动卡
- FieldManual 分类、搜索、最近阅读、我的收藏
- Global EPD Interactive Runtime A2：BUSY 期间继续收键、单一严格 FIFO、Hot Partial、日志/搜索/通讯/FieldManual 快速局刷

EPD SPI 继续保持 **4 MHz**。当前提交基线不通过提高总线频率换取速度。

---

## 3. 基本按键

CardKB2 底层方向映射保持：

```text
D = UP
Z = LEFT
X = DOWN
C = RIGHT
```

通用操作：

- `ENTER / SPACE`：确认
- `B / ESC`：返回（文本输入页可能有例外）
- `H`：帮助；文本输入态中的 `h` 优先作为字符
- `DEL`：删除 / 取消（取决于页面）
- `K`：设备状态页进入罗盘校准

---

## 4. FieldManual / 知识库

### 4.1 首页入口

知识库当前包含：

- 快速应急
- 分类浏览
- 搜索知识
- 我的收藏
- 最近阅读
- 设备手册

### 4.2 分类与行动卡列表

分类列表不使用分类图标，以文字为主。

行动卡列表统一导航：

```text
LEFT / Z   = 上一页
RIGHT / C  = 下一页
UP / D     = 上一条
DOWN / X   = 下一条
ENTER      = 阅读
B / ESC    = 返回
```

同页移动使用 micro partial，只刷新旧选中行和新选中行；跨页才刷新列表内容区。

### 4.3 Reader

Reader：

```text
LEFT / Z   = 上一页
RIGHT / C  = 下一页
ENTER      = 下一页
0          = 当前行动卡第 1 页
F          = 收藏 / 取消收藏
B / ESC    = 返回来源列表
```

已收藏时，标题后显示黑底白字 `已收藏` 标签；未收藏时不显示。右侧只保留 Card ID 与页码。SD 持久化失败时，标题标签位置显示 `保存失败`。

### 4.4 我的收藏

收藏记录保存到：

```text
/lanternbox/field_manual/favorites_v1.tsv
```

特点：

- 使用稳定 Card ID，不保存数组位置
- 知识包重新排序后仍能定位原卡片
- 最近新收藏的排在前面
- temp + backup + rename 原子快照
- 写入失败会回滚内存状态
- Runtime 当前最多覆盖 256 张卡

### 4.5 最近阅读

最近阅读保存最多 16 张行动卡以及最后页码：

```text
/lanternbox/field_manual/recent_v1.tsv
```

再次打开会恢复到上次阅读页。记录使用 Card ID，因此知识包重新排序不会串卡。

### 4.6 搜索知识

搜索范围：

- Card ID
- 标题
- 摘要
- 分类名称 / 摘要
- 行动步骤正文
- 经审核的常用说法召回

**标题不要求完全匹配。** 输入标题片段即可命中；完整编号或完整标题只会获得更高排序分数。

多个搜索词用空格分隔时采用 AND 规则。

搜索输入：

- 中文模式复用离线拼音
- `1-5` 选候选
- `6 / 7` 候选翻页
- `` ` `` 切换中 / 英文
- `DEL` 删除
- 有活动拼音时 `ENTER` 先确认输入；无活动拼音时 `ENTER` 执行搜索
- 拼音没有中文候选时，`ENTER` 直接保留当前字母
- 搜索输入态中的 `H` 是普通字母 `h`，不会打开帮助
- `ESC` 返回知识库首页

搜索输入采用 320 ms 合并刷新，避免每输入一个字就触发一次电子纸刷新。

---

## 5. FieldManual 数据包

当前随工程提供：

```text
SD_ROOT/knowledge/field_manual/
```

权威内容版本：**v1.3.1-a3.3**。

当前数据：

- 197 张行动卡
- 9 个分类
- 32 个快速入口
- 57 张卡带图
- 159 个图解页面
- 图解统一 640×110、1-bit 黑白
- 一图一步，图片下方显示短 caption
- Runtime `MAX_CARDS = 256`
- 单卡正文 buffer = 4096 bytes

复制到 SD：

```text
/knowledge/field_manual/
```

运行必需：

```text
manifest.bin
categories.bin
cards.idx
cards.dat
quick.idx
```

图文扩展：

```text
media.idx
media.dat
```

media 缺失时 Runtime 可 fail-soft 回到纯文本；旧 A3/A3.2 图片仍由 A3.3 Fit Renderer 等比缩放并限制在 Reader 内容区内。

当前媒体规范：

- 1-bit 黑白
- MSB-first
- 当前发布画布 640×110
- caption UTF-8 ≤ 63 bytes
- 一个页面尽量只表达一个动作
- Runtime 负责边界 Fit，图片不得越过 Reader 内容区

内容包本身已有双重审核与视觉审计，相关记录位于 `SD_ROOT/knowledge/field_manual/`。

---

## 6. 地图

地图使用 SD 卡中的离线 PBF 数据。

- 首次获得有效 GNSS Fix 后可跟随当前位置
- 罗盘有效时显示方向箭头
- 手动浏览后显示中心十字
- 正常导航使用 1-bit 黑白渲染
- 地图页拥有自己的刷新调度，后台状态栏局刷不会插入地图提交过程

地图搜索输入等待已从 650 ms 降至 320 ms；结果同一窗口移动采用 micro partial。

---

## 7. 日志与记录

### 定位记录

显示 Fix、经纬度、卫星、HDOP、速度和时间，并可保存轨迹。

定位日志列表同页上下移动只刷新旧/新选中行，跨页才刷新内容区。

### 语音日志

支持录音、播放、列表浏览和删除。语音日志列表同样采用旧/新行 micro partial。

---

## 8. 内部通讯

Meshtastic 由 LR01 运行，Core 只消费 Host Protocol 状态并提供 UI。

支持：

- 广播消息
- 私信
- 节点列表
- 可靠投递状态
- ACK / TIMEOUT

语义：

- `SENT`：LR01 已完成本地 RF 发送
- `DELIVERY ACK`：匹配的可靠送达 ACK 已收到
- `TIMEOUT`：可靠送达等待超时

通讯输入采用 320 ms 合并刷新；收件箱、发件箱和节点浏览已改为内容区局刷。

---

## 9. 设备状态与罗盘校准

设备状态页可检查：

- ESP32 / PSRAM / SD / CardKB2 / Audio
- LR01 Host UART
- GNSS / Fix / Compass
- LoRa / 节点网络
- 地图 / FieldManual / 拼音
- 存储空间 / 内存 / 通讯错误 / 启动状态

左右方向键翻自检页，`ENTER` 重跑自检。

设备状态页按 `K` 进入罗盘校准：

```text
COMPASS_CAL_START
COMPASS_CAL_STATUS?
COMPASS_CAL_SAVE
COMPASS_CAL_CANCEL
COMPASS_CAL_RESET
```

校准参数只由 LR01 计算并保存。Core 不保存 offset / scale。

---

## 10. Global EPD Interactive Runtime A2

v2.75k17 已将 FieldManual 中验证有效的快速交互机制扩展到第一批高频页面：

- 首页
- 定位日志
- 语音日志
- 地图搜索
- 内部通讯
- 顶部 GNSS / LoRa / SD / 时钟状态栏

核心机制：

1. GxEPD2 BUSY 期间继续扫描 CardKB2。
2. 公共 BUSY 队列容量 24 项。
3. 触发刷新按键必须先真实松开，避免误判成长按重复。
4. 局刷后保持短时 Hot，通用交互页空闲约 350 ms 后再 `powerOff()`；Field Library 阅读页沿用专用 Hot Reader 策略。
5. 高频列表同页移动优先 micro partial。
6. 高交互页面使用 5 ms 主循环节奏；静态页面保持 20 ms。

2026-08-23 已完成实机体验验证：相较旧刷新策略，按键响应和连续导航体感有显著提升。v2.75k18 完成提交收口；随后实机发现帮助页旧 UI 子集字体对 `常用快捷键`、`状态页罗盘` 缺字，v2.75k19 仅修复该显示问题，不修改已验证交互行为。

v2.75k20 起 FieldManual 不再保留自己的 A3.9 私有队列。实机发现双队列会在连续文本输入时产生时间顺序竞争，因此 Knowledge 改为直接读取公共 FIFO；搜索字符严格按 FIFO 消费，而首页/分类/列表/Reader 仍在 Knowledge 层对公共 FIFO 中连续方向键做最终位置合并。这样既保留快速导航，也消除双队列排序风险。

---

## 11. 编译与刷机

工程使用 PlatformIO。

当前环境：

```text
env: ft02-pbf-a1-n16r8-spi40
board: esp32-s3-devkitc1-n16r8
flash: 16 MB QIO
PSRAM: 8 MB OPI
```

工程自带正确 fallback board manifest：

```text
boards/esp32-s3-devkitc1-n16r8.json
```

旧项目专用 Board ID `ft02-esp32-s3-n16r8` 已废弃，不应恢复。

编译：

```bash
pio run
```

上传：

```bash
pio run -t upload
```

版本一致性检查：

```bash
python3 tools/check_firmware_version.py
```

固件版本唯一权威源：

```text
src/FT02_BuildInfo.h
```

---

## 12. 提交 / 发布检查清单

- [ ] `src/FT02_BuildInfo.h` 已更新
- [ ] `VERSION.txt` 与 BuildInfo 一致
- [ ] `python3 tools/check_firmware_version.py` PASS
- [ ] `pio run` 编译通过
- [ ] `boards/esp32-s3-devkitc1-n16r8.json` 存在
- [ ] 未恢复旧 Board ID
- [ ] 未重新引入 Core 直接解析 GNSS/NMEA
- [ ] 未重新引入 Core QMC5883L / SX126x 硬件访问
- [ ] LR01 Host Protocol 如有变化已同步协议文档
- [ ] 设备内帮助和实际快捷键一致
- [ ] 地图、定位、语音日志、通讯、知识库无明显回归
- [ ] 收藏、最近阅读、搜索的 SD 持久化路径正常
- [ ] 重新生成 `SHA256SUMS.txt`
- [ ] 完整 ZIP 解包后再次校验 SHA256

建议 Git 提交格式：

```text
FT02 vX.XX: <主要功能/修复>
```

---

## 13. v2.75k19 帮助页 CJK 字形修复

实机照片确认帮助页仍有旧子集字库缺字：

- `常用快捷键`：旧 `ft02_menu_28m` 缺 `常 / 用 / 快 / 捷`，只剩 `键` 可显示。
- `状态页罗盘`：旧字库缺 `页 / 罗 / 盘`。

修复原则：

- 帮助页章节标题改用全量 `ft02_cjk_24b`。
- 中文操作说明与底栏改用全量 `ft02_cjk_20r`。
- `ENTER / SPACE` 等纯 ASCII 键帽继续使用原 UI 字体。
- 右侧键帽宽度由 200 px 收到 190 px，操作说明起点前移，`状态页罗盘` 在 20px CJK 字体下实测文本宽 105 px，可完整落在 420–760 px 右面板内。
- 不修改 InputManager、EPD 4 MHz、Global EPD Runtime、FieldManual 数据或 LR01 协议。

## 14. v2.75k18 提交收口审计

本版不增加用户功能，重点清理已经失去调用路径的历史代码和重复说明。

已清理：

- 删除没有任何调用者的旧四灰地图提交链路，以及对应 `Gray4Panel / GrayMapUI / GrayMapFontData` 源码和生成工具。当前地图生产路径早已固定为 1-bit PBF。
- 删除分类浏览取消图标后遗留的分类图标枚举、绘制分支和分类→图标映射函数。
- 删除 `FT02_IconRenderer` 中没有调用者的原尺寸 / 整数倍缩放接口，只保留当前实际使用的 `FT02_DrawIconSize()`。
- 删除无任何 include / 调用路径的孤立字体头 `FT02_GrayFontPackTypes.h`、旧 `FT02_MapFontData.h`、`FT02_StatusFont.h`；保留当前实际使用的 RoadName / MapStatus / StatusBar 字体数据。
- 删除若干只有声明和实现、当前工程没有调用者的简单 Runtime getter/reset/introspection API。
- 删除废弃的 `boards/ft02-esp32-s3-n16r8.json`。
- 合并重复/过期说明文件；FieldManual 当前规范统一以 v1.3.1-a3.3 / 197 张行动卡 / 9 分类 / 640×110 / 159 页为准。
- 工程内部 checksum 统一只保留 `SHA256SUMS.txt`。

明确保留：

- FieldManual A3/A3.2 兼容 Fit 路径，因为它承担旧媒体包兼容，不是死代码。
- `FT02_Gnss` facade，因为它现在承载 LR01 Navigation State 快照兼容接口，并不直接解析 NMEA。
- FieldManual 的方向键合并语义继续保留，但事件来源统一为 Global EPD A2 单一 FIFO；不再保留私有 BUSY 队列。
- `docs/history/`，用于版本追溯，不参与当前功能入口。

### 当前验证状态

- v2.75k17 Global EPD Interactive Runtime A1：2026-08-23 实机体验验证通过，用户确认按键响应显著改善。
- v2.75k18：功能逻辑沿用 k17，仅做提交清理；版本集中化检查、Python tools 语法、JSON 解析、工程内 FT02 include 完整性、删除符号残留、修改 C++ 文件分隔符结构检查均已 PASS。
- v2.75k19：帮助页改用全量 CJK 字库并校验页面全部中文字符均存在；其余冻结边界保持 k18。
- v2.75k20：FieldManual 与其他交互页统一到 Global EPD A2 单一 FIFO，修复连续搜索输入的字符顺序竞争；待本机编译与实机验证。
- `FT02_InputManager.cpp/.h` 与 k17 逐字节一致，`SD_ROOT` 与 k17 逐文件一致，EPD SPI 仍为 4 MHz，Board ID 仍为 `esp32-s3-devkitc1-n16r8`。
- 当前打包环境不具备 PlatformIO，因此此环境未执行 ESP32-S3 `pio run` / upload。提交前仍应在本机执行一次 `pio run`，随后做最小烟雾测试。

---

## 15. 不要做的事

- 不要删除 `boards/esp32-s3-devkitc1-n16r8.json`
- 不要恢复废弃 Board ID
- 不要把 Core 的 GNSS / QMC5883L / SX126x 旧驱动重新接回生产路径
- 不要恢复 Meshtastic protobuf full-sync / Core NodeDB 权威逻辑
- 不要在 Core 保存罗盘校准 offset / scale
- 不要基于旧工程覆盖当前 `src/`
- 不要为了新功能回退已经实机验证的 EPD 快速交互、FieldManual Fit、PSRAM/SD 分块读取和收藏/搜索/最近阅读能力
