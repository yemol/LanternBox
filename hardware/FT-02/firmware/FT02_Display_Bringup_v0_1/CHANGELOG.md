## v2.75k19 - Submission Hotfix A1.1 / Help CJK Font Fix

- 根据实机照片修复帮助页中文缺字方框。根因是帮助页仍使用早期 `ft02_menu_28m` UI 子集：`常用快捷键` 缺 `常/用/快/捷`，`状态页罗盘` 缺 `页/罗/盘`。
- 帮助页章节标题统一改用全量 `ft02_cjk_24b`，中文操作说明和底栏统一改用全量 `ft02_cjk_20r`；纯 ASCII 键帽继续使用原 UI 字体。
- 右侧快捷键键帽宽度由 200 px 调整为 190 px，中文操作说明起点前移到 x=650；`状态页罗盘` 在 `ft02_cjk_20r` 下宽度 105 px，可完整落在右侧面板边界内。
- 保持 v2.75k18 的 Global EPD Interactive Runtime A1、FieldManual A3.12、InputManager、EPD SPI 4 MHz、SD_ROOT 数据与 LR01 Single Authority 不变。

## v2.75k18 - Commit Baseline A1 / Submission Cleanup

- 以已完成实机快速交互验证的 v2.75k17 为功能基线；本版不新增用户功能，不修改已经验证的按键映射、EPD 4 MHz SPI、FieldManual 数据或 LR01 单一权威架构。
- 合并用户操作、编译刷机、FieldManual 部署/媒体规范、发布检查和当前验证记录到 `docs/FT02_PROJECT_GUIDE.md`；协议与硬件 Pin Lock 继续独立保留。
- 删除已经没有任何调用者的旧四灰地图提交链路，以及 `FT02_Gray4Panel`、`FT02_GrayMapUI`、`FT02_GrayMapFontData` 和对应生成工具；当前生产地图继续固定为 1-bit PBF 快速导航。
- 清理分类浏览取消图标后遗留的分类图标枚举、绘制分支和分类→图标映射函数。
- `FT02_IconRenderer` 删除未使用的原尺寸 / 整数倍缩放接口，仅保留当前实际调用的 `FT02_DrawIconSize()`。
- 删除 3 个确认无任何 include / 调用路径的孤立字体头：`FT02_GrayFontPackTypes.h`、旧 `FT02_MapFontData.h`、`FT02_StatusFont.h`；当前地图继续使用 `FT02_RoadNameFont.h` / `FT02_MapStatusFontData.h`，状态栏继续使用 `FT02_StatusFontData.h`。
- 删除若干只有声明/实现但无当前调用者的简单 Runtime getter/reset/introspection API。
- 删除废弃的 `boards/ft02-esp32-s3-n16r8.json`，只保留正确的 `boards/esp32-s3-devkitc1-n16r8.json`。
- 工程 checksum 统一为单一 `SHA256SUMS.txt`，移除重复 `FILE_CHECKSUMS.sha256`。
- FieldManual 当前说明统一为 Content v1.0.5-a3.3：100 卡、32 快速入口、57 张卡带图、141 图解页、640×110 1-bit 安全画布。
- 明确保留 FieldManual A3.9 专用 BUSY 队列/方向合并、旧 A3/A3.2 媒体 Fit 兼容路径和 `FT02_Gnss` LR01 snapshot facade；这些仍承担当前运行语义，不作为死代码删除。

## v2.75k17 - Global EPD Interactive Runtime A1 + Fast Navigation Phase 1

- 将 FieldManual A3.9 已验证的 BUSY-time CardKB 扫描与短时 Hot Partial 机制抽为公共 `FT02_EpdInteractiveRuntime`，供非知识库页面复用。
- 新增 24 项公共 BUSY 输入队列；电子纸刷新和延迟 `powerOff()` 期间继续轮询 CardKB2，刷新结束后由主循环重放按键。
- 首页卡片选择继续使用原有局刷区域，但改为 BUSY 收键 + 2.5 秒 Hot Partial，不再每移动一次就立即 `powerOff()`。
- 定位记录列表同页移动改为只刷新旧选中行与新选中行；跨页才刷新列表内容区，并接入公共 Hot Partial。
- 语音日志列表同页移动改为旧/新两行 micro partial；跨页继续使用内容区局刷，并接入公共 Hot Partial。
- 地图搜索输入合并等待由 650 ms 降至 320 ms；停顿后只刷新搜索编辑区。搜索结果同一可视窗口内移动只刷新旧/新结果行，滚动窗口变化时才刷新结果内容区。
- 内部通讯输入合并等待由 650 ms 降至 320 ms；收件箱/发件箱/节点浏览改为内容区局刷，输入编辑区使用独立局刷，不再每次都 800×480 全刷。
- 顶部 LoRa/GNSS/SD/时钟局刷接入 BUSY 输入捕获；后台状态变化发生时也能继续接住短按键，避免偶发“这一键特别慢”。
- 首页、地图搜索、通讯、定位日志、语音日志与知识库主循环轮询统一使用 5 ms 快速交互节奏；静态/诊断页仍保持 20 ms。
- 保持 EPD SPI 4 MHz，不在本版提高总线频率；保持 `FT02_InputManager.cpp/.h`、CardKB2 D/Z/X/C 底层映射、FieldManual `SD_ROOT` 数据、LR01 架构不变。

## v2.75k16 - FieldManual Runtime A3.12 Inline Favorite Badge + Logical Navigation

- Reader 不再使用右侧独立收藏提示条，避免覆盖 Card ID / 页码并消除局刷残字。
- 当前卡已收藏时，在标题后紧跟黑底白字 `已收藏` 标签；未收藏时完全不显示标签。
- 取消收藏后标签直接消失，收藏后标签直接出现，收藏状态本身即为操作反馈。
- SD 持久化失败时只在同一标题标签位显示 `保存失败`，不占用右侧分页区域。
- 右侧始终只显示 `<Card ID>  <当前页>/<总页数>`。
- 保留 A3.11 逻辑导航、A3.10 收藏持久化、A3.9 BUSY 输入队列 / micro partial、4 MHz EPD SPI、InputManager 与知识包不变。

## v2.75k15 - FieldManual Runtime A3.11 Favorite Feedback + Logical Navigation

- Reader 收藏/取消收藏新增明确黑底反馈：`已收藏` / `已取消收藏`；持久化失败显示 `收藏保存失败`。
- 收藏提示不使用阻塞 delay，也不安排额外的“提示消失”墨水刷新；下一次正常 Reader 重绘自然清除。
- 行动卡列表导航改为：`LEFT/RIGHT` 上一页/下一页，`UP/DOWN` 上一条/下一条。
- Reader 导航统一为：`LEFT/RIGHT` 上一页/下一页；`UP/DOWN` 不再承担翻页。
- BUSY 输入队列中的列表/Reader 合并导航同步采用新键位，避免主路径与排队路径行为不一致。
- CardKB2 底层 `D/Z/X/C = UP/LEFT/DOWN/RIGHT` 映射、InputManager、EPD 4 MHz SPI、FieldManual 数据与 A3.3/A3.9 优化全部保持不变。

## v2.75k14 - FieldManual Runtime A3.10 Favorites

- 完成知识库“我的收藏”入口，不再使用占位页。
- Reader 新增 `F` 收藏 / 取消收藏；收藏成功后 Reader 右上角显示“已收藏”。
- Reader 底栏将原来容易歧义的 `0首页` 明确改为 `0第一页`，并加入 `F收藏/取消` 提示。
- 收藏持久化文件为 `/lanternbox/field_manual/favorites_v1.tsv`，只保存稳定 Card ID，不保存数组序号；未来知识包重新排序不会把收藏指到错误卡片。
- 收藏写入使用 temp + backup + rename 原子快照；写 SD 失败时回滚内存状态，不虚假显示收藏成功。
- 收藏列表按最近新增顺序排列；取消后立即从“我的收藏”列表移除，重新收藏会回到列表最前。
- 收藏运行时只保存 `uint16_t` 卡片索引顺序，最多覆盖 Runtime 当前 256 张卡，不复制标题/正文，避免重新引入大块 DRAM。
- 知识包升级后若收藏文件存在已删除/改名 Card ID，会在加载时忽略并清理陈旧项。
- 保留 A3.9 BUSY 输入队列 / micro partial、A3.8 搜索召回、A3.7 最近阅读、A3.3 图片 Fit 与 PSRAM/SD 分块读取路径。
- 保持 InputManager、CardKB2 D/Z/X/C 底层映射、EPD SPI 4 MHz、LR01 单一硬件权威架构不变。

## v2.75k13 - FieldManual Runtime A3.9 Busy Input Queue + Micro Partial Navigation

- 在 FieldManual 的电子纸刷新期间启用 GxEPD2 BUSY callback，BUSY 等待阶段继续轮询 CardKB2，不再让约半秒的局刷窗口完全停止键盘服务。
- 新增 16 项 FieldManual BUSY 输入队列；刷新期间收到的新按键在当前提交结束后继续处理。
- 对首页、分类、行动卡列表和 Reader 的连续方向键做合并：同一 BUSY 周期内多次导航只计算最终位置，并只触发一次后续刷新，避免“按三次就再刷三次”。
- 增加初始按键释放门控：触发当前刷新的那次物理按键不会因为 BUSY 轮询而被误算成长按重复。
- 行动卡列表同页 LEFT/RIGHT 移动改为 micro partial，仅重绘旧/新选择覆盖到的列表行；跨页仍刷新知识内容区。
- 列表右上角改为页级信息 `x/y页  共N条`，当前条目由黑色高亮行明确表示，因此同页移动无需为逐条序号刷新 Header。
- 搜索输入合并刷新等待由 650 ms 缩短为 320 ms；BUSY 队列负责接住刷新期间的新输入。
- FieldManual 页面主循环轮询间隔由 20 ms 缩短为 5 ms；其他 FT-02 页面保持原有节奏。
- FieldManual 空闲 `powerOff()` 阶段也启用 BUSY 输入捕获，避免关高压期间形成新的输入盲区。
- 保持 EPD SPI 4 MHz，不在本版提高总线频率；保持 InputManager 文件和底层按键映射不变。
- 保持 A3.8 搜索、A3.7 最近阅读、Reader `0` 回当前卡第 1 页、A3.3 图片 Fit、PSRAM 索引/图片缓冲和 SD 分块读取不变。

## v2.75k12 - FieldManual Runtime A3.8 Search Input Priority + Common-Term Recall

- 修复知识库搜索中文输入时字母 `H` 被主事件分发层提前解释为全局 HELP 的问题；搜索输入态现在拥有 raw 字母优先权，`H` 正常进入拼音/英文搜索。
- 保持 InputManager 底层全局 `H=HELP` 映射不变；只在 FieldManual 搜索输入态阻止主循环提前打开帮助页，离开搜索后 H 仍按原逻辑打开帮助。
- 搜索页移除含糊的“关键词”文案，改为“标题片段、编号或内容词”，并明确标题支持部分匹配。
- 拼音无中文候选时，ENTER 自动把当前字母加入搜索词；旧 `0` 原样字母功能继续兼容，但不再作为用户必须理解的操作提示。
- 搜索新增安全的常用说法召回层；首批加入“陷阱 / 营地陷阱 / 野外陷阱 / 捕兽陷阱 / 警戒线 / 报警线 / 警报线”→ `CMP003 无伤害外围声响报警`。这只改善召回，不新增伤害性陷阱知识。
- 原有字面全文搜索继续支持：卡片编号、标题、摘要、分类、正文；完整 ID/标题得分更高，标题/正文部分匹配仍有效，多搜索词仍为 AND。
- A3.7 最近阅读、搜索结果列表、650 ms 输入合并局刷、A3.6 快速局刷与 A3.3 图像 Fit 保持不变。

## v2.75k11 - FieldManual Runtime A3.7 Recent Reading + Full Search

- 完成知识库“最近阅读”：最多保存 16 张最近打开的行动卡，按最近使用顺序排列。
- 最近阅读持久化保存 `cardId + page + lastUsedEpoch` 到 `/lanternbox/field_manual/recent_v1.tsv`，使用临时文件 + 备份文件原子替换，避免掉电写坏主记录。
- 最近记录以卡片 ID 为权威，不持久化数组序号；知识包未来重新排序时会重新解析 ID，已删除/改名卡片会自动忽略。
- Reader 打开、翻页和 `0` 返回当前卡第 1 页都会更新最近阅读页码；退出 Reader 时立即 flush，停留阅读时也会在空闲 5 秒后自动持久化。
- 完成“搜索知识”：支持卡片编号、标题、摘要、分类名称/摘要以及 `cards.dat` 行动步骤正文。
- 搜索采用“索引字段优先、正文按需读取”策略；只有关键词未在索引命中时才加载对应卡片正文，减少无意义 SD 读取。
- 多关键词搜索采用 AND 语义，结果按编号精确命中、标题、摘要、分类、正文的相关度评分排序，最多保留 64 条结果。
- 搜索输入复用已验证的离线拼音 A4、词频学习与共享 CN/EN 模式；ENTER 在存在活动拼音时先上屏，再次 ENTER 执行搜索。
- 搜索输入采用 650 ms idle 合并局刷，按键先进入 RAM，不再每个字符都触发一次约 0.5 秒电子纸刷新。
- 搜索结果、最近阅读、快速应急和分类列表统一使用 `FT02KnowledgeListSource` 列表来源状态，不再依赖旧 `g_quickMode` 特例，为后续“我的收藏”保留同一扩展路径。
- A3.6 快速局刷、Reader `0` 回当前卡第 1 页、A3.3 图片 Fit、PSRAM 索引/图片缓冲、SD 分块读取和 LR01 单一权威架构保持不变。

## v2.75k10 - FieldManual Runtime A3.6 Reader First Page + Hot Partial Navigation

- 修正 v2.75k9 对“0首页”的语义误解：`0` 现在只在行动卡 Reader 内生效，跳回**当前行动卡第 1 页**，不再返回 FT-02 系统首页。
- Reader 底栏保留 `0首页`；FieldManual 首页、分类页、卡片列表和预留页移除错误的 `0首页` 提示。
- 保持 InputManager 底层按键映射不变，`0` 仍是普通 `FT02_KEY_CHAR`，仅由 Reader 层解释。
- 继续优化按键体感：FieldManual 局部刷新后不再每次立即 `powerOff()`；连续浏览期间保持短时热状态，空闲 2.5 秒后自动关闭面板驱动高压。
- 新增 `[KNOWLEDGE-EPD-A3.6]` 串口耗时日志，直接记录每次知识库局刷耗时和延迟关电耗时，便于实机确认瓶颈。
- GDEQ0426T82/GxEPD2 的快速局刷本身仍有约 0.5 秒级物理刷新时间；本版删除的是额外的重复关电开销，不虚构 LCD 级响应。
- EPD SPI 仍保持当前工程已使用的 4 MHz，不在未验证硬件信号完整性的情况下贸然提高全局 SPI 时钟。
- A3.3 图片 Fit、PSRAM 索引/图片缓冲、SD 分块读取、LR01 单一权威架构保持不变。

## v2.75k9 - FieldManual Runtime A3.5 Fast Partial Navigation + Zero Home

- FieldManual 任意子页面新增 `0` 直接返回 FT-02 系统首页；不修改 InputManager 底层按键映射。
- 知识库底栏同步加入 `0首页` 提示；列表底栏明确保留 `ENTER阅读`、`B返回`。
- 修复知识库方向键体感迟钝的主要原因：v2.75k8 每次选择移动都会执行 800×480 整屏刷新。
- 首页/分类同页选中移动改为旧/新卡片联合区域局部刷新。
- 分类跨页、行动卡列表移动/翻页、Reader 翻页改为知识内容区局部刷新。
- 页面进入/退出仍保留完整刷新，避免破坏现有电子纸清屏/残影控制逻辑。
- CardKB2 首次按键仍即时读取；180 ms 仅用于同键长按重复节流，没有增加首击延迟。
- FieldManual v1.0.5-a3.3 数据包、PSRAM 索引/图片缓冲、SD 分块读取和 A3.3 图片 Fit Renderer 全部保持不变。

## v2.75k8 - FieldManual Runtime A3.4 Category/List Navigation

- 分类浏览卡片移除分类图标，释放横向空间给分类名称和摘要。
- 行动卡列表改为 `LEFT/RIGHT` 上一条/下一条。
- 行动卡列表改为 `UP/DOWN` 上一页/下一页，每页 4 条。
- 列表页头同时显示当前条目位置与页码，例如 `5/24  2/6页`。
- 列表底栏移除“确认阅读”，改为 `左右 上/下条`、`上下 上/下页`、`返回(B)`。
- `ENTER/SPACE` 仍用于打开当前选中的行动卡。
- FieldManual v1.0.5-a3.3 数据包与 A3.3 图解格式保持不变。

## v2.75k7 - FieldManual Runtime A3.3 Visual Fit + Single-Action Pages

- 修复 A3.2 captionBelow 图解高度超过阅读区域时越界的问题。
- 新增 1-bit 图片等比 Fit Renderer：旧 A3/A3.2 大图也会自动缩放在 Reader 边界内。
- 配套 FieldManual v1.0.5-a3.3：所有图解改为 640×110 安全画布；旧多动作总览拆成单动作页并逐页配短说明。
- 保留 v2.75k5 PSRAM 动态索引/图片缓冲与 v2.75k3 PlatformIO board 修复。

## v2.75k6 - FieldManual Runtime A3.2 Step Diagrams

- 绳结 TOL001-TOL004 改成一页一步的大图，不再把多个动作挤在同一张 688×184 总览图。
- 分步图页面增加 `图解 n/N` 标题，操作说明直接绘制在图片下方。
- 使用 media flags bit0 保持 A3 media 格式兼容：旧图仍按 A3 总览布局显示。
- 平结 4 页、称人结 5 页、双套结 4 页、卡车结 5 页，共 18 张分步图；其他工具 6 张总览图保留。
- 绳结步骤重新对照 Animated Knots by Grog 的标准顺序核查，仅用于卡片标注的低风险用途边界。
- 保留 v2.75k5 的 PSRAM 动态索引/图片缓冲修复与 v2.75k3 PlatformIO board 修复。

# FT-02 Changelog

## v2.75k5 - FieldManual Runtime A3.1 PSRAM DRAM Fix

- 修复 A3 图文 Runtime 链接阶段 `.dram0.bss` 溢出。
- FieldManual 卡片索引从固定 256×316B 内部 DRAM 数组改为按实际 cardCount 在 PSRAM 动态分配。
- 图像索引从固定 256×84B 内部 DRAM 数组改为按实际 mediaCount 在 PSRAM 动态分配。
- 图像数据缓冲从固定 24KB 内部 DRAM 改为打开图片时按 dataLength 在 PSRAM 分配，退出图片页立即释放。
- SD→PSRAM 读取增加 512B 内部 RAM 分段 staging，避免大块 PSRAM 指针直接交给存储 VFS。
- A3 图文包格式保持不变，v1.0.2 图文知识包无需重建。

- 在 A2 文本行动卡基础上增加可选 `media.idx / media.dat` 图片页。
- 图片采用预处理 1-bit 黑白位图，最大 688×184，不在 ESP32 上做 PNG/JPEG 解码。
- 图片按需读取并进行单图 CRC32 校验。
- 图片扩展 fail-soft：图片缺失或异常不会阻断文本手册。
- 阅读器把图解作为独立分页并沿用 Z/C 翻页。
- 首批为 TOL001-TOL010 十张工具卡加入图解。
- 保留 v2.75k3 PlatformIO board 修复与全部既有 Core/LR01 架构。

## v2.75k3 - PlatformIO Board ID Fix

- Fixed PlatformIO `UnknownBoard` by switching the project board ID to `esp32-s3-devkitc1-n16r8`.
- Added explicit `boards_dir = boards`.
- Bundled `boards/esp32-s3-devkitc1-n16r8.json` as a project-local fallback.
- Preserved N16R8 settings: 16 MB QIO flash, 8 MB OPI PSRAM, existing partition table and upload limit.
- FieldManual Runtime A2 and the double-checked 100-card SD pack are unchanged.

## v2.75k3 - FieldManual Runtime A2 Release Verification

- FieldManual Runtime 最大卡片数保持为 256。
- 修复 v2.75k1 包中的版本元数据不同步问题。
- 修复 `FT02_FIRMWARE_BUILD_LABEL` 重复拼接版本号的问题。
- 重新生成并验证工程 SHA-256 清单。
- 附带当前双重审核通过的 FieldManual v1.0.1 SD 部署目录。
- 不改变 Core/LR01、地图、通讯、罗盘校准等已验证运行逻辑。

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


## v2.75k1 FieldManual Runtime Expansion
- Expand FieldManual card capacity from 64 to 256 for external manual packs.
