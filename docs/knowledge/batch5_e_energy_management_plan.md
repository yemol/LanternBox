# Batch5-E Energy Management Expansion Plan

生成时间：2026-07-14

## 1. 当前能源管理覆盖

本阶段只做规划，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或测试。

已检查：

- `wiki_import/power/`
- `wiki_import/comms/`（项目中通信目录实际为 `comms`，不是 `communication`）
- `wiki_import/data/`
- `wiki_import/records/`
- `wiki_import/repair/`
- `data/guides/power/`
- `data/guides/comms/`

### 已有能源管理条目

已有能源管理相关 Wiki：

- `energy-task-scheduling-001`：低电量任务排程，已关联 DG-0339。
- `energy-bank-battery-priority-001`：移动电源低电量使用优先级，已关联 DG-0563。
- `energy-power-outage-002`：停电后的关键设备分级，已关联 DG-0563。
- `energy-power-outage-001`：断电后第一小时能源盘点。
- `energy-battery-capacity-001` / `energy-battery-capacity-002`：电池容量和功耗基础。
- `energy-charge-log-001`：充放电记录必要性。
- `energy-core-shutdown-loss-001`：Core 设备断电前处理。
- `energy-radio-power-001`、`energy-phone-002`：通信设备和手机低电量相关条目。

已有能源管理相关 Guide：

- DG-0107：设备充电优先级：先保命，再保信息。
- DG-0339：低电量工作模式。
- DG-0563：移动电源：关键设备优先级。
- DG-0336：电池轮换使用。
- DG-0618：电池混放后的编号轮换。
- DG-0619：负载过大时断开排序。

判断：

- 已有内容能处理“低电量要省电”和“关键设备优先”，但粒度偏粗。
- 缺少可复用的每日预算表、最低电量线、充电队列、共享电源借还、用电复盘和交班摘要。

### 已有太阳能管理条目

已有太阳能 Wiki：

- `energy-solar-panel-001`：太阳能板基础使用。
- `energy-solar-charging-001`：太阳能充电受光照和温度影响。
- `energy-solar-charging-002`：太阳能板防水和线头保护。
- `energy-solar-charging-003`：太阳能充电的光照因素。
- `energy-solar-system-basics-index-001`：太阳能系统基础索引。

已有太阳能 Guide：

- DG-0109：太阳能板应急充电。
- DG-0564：太阳能充电：排程和保护。
- DG-0617：太阳能充电白天排程。

判断：

- 已有内容偏“基础使用、排程和安全保护”。
- 缺少阴天降级、日照窗口估算、今日先充谁、傍晚复盘和多日天气下的保守策略。

### 已有通信省电条目

已有通信省电 Wiki：

- `communication-power-saving-001`：低电量通信优先级。
- `communication-communication-knowledge-002`：电量和通信窗口的协调。
- `communication-contact-window-001`：固定开机窗口节省电量的原理。
- `communication-contact-window-002`：固定开机窗口的作用。
- `communication-device-daily-check-001`：通讯设备每日检查。
- `communication-sms-002`：短信电话和耗电差异。

已有通信省电 Guide：

- DG-0582：手机低电量：固定时间开机联系。
- DG-0636：断网家庭固定开机窗口。
- DG-0639：LoRa 节点每日状态记录。

判断：

- 通信省电已经较完整。
- Batch5-E 不应重复通信纪律，而应定义“能源管理主导、通信作为负载”的边界。

### 已有能源记录条目

已有记录 Wiki：

- `general-energy-log-001`：能源日志。
- `general-inventory-consumption-log-001`：库存消耗记录。
- `general-small-team-handoff-001`：小团队知识交接。
- `organization-decision-log-001`：决策记录基础。

判断：

- `general-energy-log-001` 已经覆盖能源日志总原则。
- 缺少更细的“充放电交接记录、移动电源借还、备用电池轮换、每日预算复盘、太阳能产出记录”。
- 这些应作为 P1/P2 操作条目补齐，不应重写 `general-energy-log-001`。

### 重复或近似条目

不建议在 Batch5-E 重复以下主题：

- 电池鼓包/发热/停用：`energy-battery-swelling-001`、`energy-battery-swelling-overheat-001`、`energy-fever-001`、`energy-fever-003`、`energy-swollen-lithium-battery-stop-use-001`、DG-0841。
- 低压异常/接线安全：DG-0842、DG-0843 及 Batch5-A1 的 18 篇 P0 安全 Wiki。
- 太阳能基础百科：`energy-solar-panel-001`、`energy-solar-system-basics-index-001`。
- 通信窗口：DG-0582、DG-0636、`communication-contact-window-001`。
- 通信省电：`communication-power-saving-001`、`communication-sms-002`。

### 不应改动的旧条目

本批只做能源管理扩容，不改动：

- Energy Safety Retrieval v0.1 stable 相关 Wiki/Guide/profile。
- DG-0841、DG-0842、DG-0843。
- 旧通信窗口 Guide：DG-0582、DG-0636。
- 现有太阳能安全和排程 Guide：DG-0109、DG-0564、DG-0617。
- 现有记录总纲：`general-energy-log-001`。
- 任何 Retrieval profile、Prompt、Pipeline 或 selector 参数。

## 2. 缺口分析

当前最明显缺口不是“危险停用”，而是长期断电时的日常能源治理：

1. 每天有多少能源可用、预留多少、谁能用。
2. 哪些设备有最低电量线，低于线后自动停用非关键任务。
3. 多设备、多成员、多电源时如何排队充电。
4. 太阳能产出不稳定时如何保守安排。
5. 夜间照明如何按续航和安全风险分配。
6. 备用电池、移动电源如何轮换、借还、交接。
7. 低电量时按什么顺序断开负载，而不是等设备突然断电。
8. Core、Field Terminal、通信设备、照明之间的边界需要明确。

## 3. 新增 Wiki 规划清单

建议规划 27 篇，其中 P1 21 篇、P2 6 篇、P0 0 篇。  
分类只使用现有正式分类：`能源`、`信息保存与长期重建`、`通讯`、`风险决策`。

| slug | title | category | priority | risk_level | 用途一句话 | 是否需要 Guide | 可能关联 Guide |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `energy-daily-energy-budget-001` | 每日能源预算表 | 能源 | P1 | caution | 把当天可用电量、预留量、充电来源和关键负载写成一页预算。 | 是 | DG-0844 |
| `energy-critical-device-minimum-line-001` | 关键设备最低电量线 | 能源 | P1 | caution | 为通信、照明、记录、医疗照护设备设定低于即收紧用电的电量线。 | 是 | DG-0845 |
| `energy-device-power-priority-tier-001` | 设备供电优先级分层 | 能源 | P1 | caution | 将设备按保命、通信、照明、记录、效率、舒适分层。 | 是 | DG-0844 / DG-0845 |
| `energy-charging-queue-schedule-001` | 充电队列和排班 | 能源 | P1 | caution | 给多个待充设备安排顺序、时段、负责人和停止时间。 | 是 | DG-0846 |
| `energy-low-battery-power-window-001` | 低电量设备开机窗口 | 能源 | P1 | caution | 把低电量设备的开机、查询、记录和关机压缩到短窗口。 | 是 | DG-0845 / DG-0846 |
| `energy-solar-daytime-charge-schedule-001` | 太阳能白天补电排程 | 能源 | P1 | caution | 按日照窗口安排早、中、晚的充电目标和面板调整。 | 是 | DG-0847 |
| `energy-cloudy-solar-downgrade-001` | 阴天太阳能降级安排 | 能源 | P1 | caution | 云多、输出弱时只保关键设备，不安排低优先设备补电。 | 是 | DG-0847 |
| `energy-night-lighting-runtime-plan-001` | 夜间照明续航安排 | 能源 | P1 | caution | 用低亮点位、轮换灯具和关灯窗口保证整夜最低照明。 | 是 | DG-0845 |
| `energy-backup-battery-rotation-ledger-001` | 备用电池轮换台账 | 能源 | P1 | normal | 记录备用电池编号、估计电量、最近使用和下次复查。 | 可选 | DG-0336 / DG-0618 |
| `energy-charge-discharge-handover-log-001` | 充放电交接记录 | 信息保存与长期重建 | P1 | normal | 记录每次充电、放电、借出、归还、异常和责任人。 | 可选 | DG-0846 |
| `energy-low-battery-load-disconnect-order-001` | 低电量断开负载顺序 | 能源 | P1 | caution | 在电量不足但未出现故障时，按优先级断开非关键负载。 | 是 | DG-0848 |
| `energy-shared-power-bank-borrow-return-001` | 共享移动电源借用归还 | 信息保存与长期重建 | P1 | normal | 管理共用移动电源的借用人、用途、归还电量和异常状态。 | 可选 | DG-0846 |
| `energy-device-runtime-estimate-card-001` | 设备续航估算卡 | 能源 | P1 | normal | 用历史使用时长和掉电速度估算设备还能支撑多久。 | 否 | DG-0844 |
| `energy-essential-load-reserve-001` | 关键负载预留量 | 能源 | P1 | caution | 给照明、通信、记录和医疗照护保留不可挪用电量。 | 是 | DG-0845 |
| `energy-nonessential-device-suspend-list-001` | 非必要设备停用清单 | 能源 | P1 | normal | 明确娱乐、重复照明、长时间开屏、非关键拍摄等暂停项。 | 否 | DG-0848 |
| `energy-day-end-power-review-001` | 每日用电复盘 | 信息保存与长期重建 | P1 | normal | 每晚复盘今日用电、异常掉电、未完成充电和次日调整。 | 可选 | DG-0844 |
| `energy-solar-charge-target-selection-001` | 日照不足时充电目标选择 | 能源 | P1 | caution | 输出不足时在通信、照明、记录、移动电源之间选择目标。 | 是 | DG-0847 |
| `energy-charging-port-rotation-rule-001` | 充电口轮换规则 | 能源 | P1 | normal | 接口数量有限时，用轮换表防止单设备长期占用。 | 可选 | DG-0846 |
| `energy-power-source-status-board-001` | 电源状态看板 | 信息保存与长期重建 | P1 | normal | 把移动电源、电池、太阳能、充电口状态集中展示。 | 否 | DG-0844 |
| `energy-low-power-core-terminal-window-001` | Core 和终端低电量窗口 | 能源 | P1 | caution | 为 Core、FT-01、记录设备设置短开机、同步预览和关机窗口。 | 可选 | DG-0845 |
| `energy-cold-weather-runtime-adjustment-001` | 低温下电量估算修正 | 能源 | P1 | normal | 低温时按更保守续航估算安排照明、通信和备用电池。 | 否 | DG-0844 |
| `energy-wh-mah-local-estimate-001` | Wh 和 mAh 的本地估算口径 | 能源 | P2 | normal | 用低资源方式把容量标称转成可比较的粗略电量。 | 否 | 无 |
| `energy-device-consumption-baseline-001` | 设备耗电基线记录 | 信息保存与长期重建 | P2 | normal | 记录每台设备在常用模式下的实际耗电经验。 | 否 | 无 |
| `energy-solar-weather-pattern-log-001` | 太阳能天气产出记录 | 信息保存与长期重建 | P2 | normal | 记录晴天、阴天、遮挡、时间段和充入效果，用于后续排程。 | 否 | DG-0847 |
| `energy-battery-aging-capacity-note-001` | 电池老化和可用容量记录 | 能源 | P2 | normal | 记录掉电快、充不满、低温衰减等老化迹象，不再按标称容量安排。 | 否 | DG-0336 / DG-0618 |
| `energy-load-category-vocabulary-001` | 负载分类术语卡 | 能源 | P2 | normal | 统一关键负载、普通负载、可暂停负载、备用负载的团队用语。 | 否 | DG-0844 |
| `general-energy-handover-summary-001` | 能源交班摘要 | 信息保存与长期重建 | P2 | normal | 用一页摘要交接剩余电量、禁用设备、明日优先级和复查点。 | 可选 | DG-0844 |

## 4. Guide 候选

最多新增 5 个 Guide。以下 ID 为建议，占用需在 Apply 前再检查最新 Guide 序号。

### DG-0844 每日能源预算

scenario：长期断电或离网供电不稳定，团队每天需要决定电量怎么分配。  
steps：

1. 统计可用电源、估计余量和今日可能补电来源。
2. 列出关键负载：通信、照明、记录、医疗照护、取水/保温。
3. 写出不可挪用预留量，再分配普通任务窗口。
4. 标记今日暂停设备和可手工替代任务。
5. 晚间复盘实际用电和次日调整。

check：

- 预算表包含电源、负载、预留量、负责人和复查时间。
- 关键负载没有被娱乐、重复照明或长时间开屏占用。
- 当日补电失败时有降级方案。

stop_or_escalate：

- 关键通信、照明或医疗照护低于最低线时，暂停所有非关键用电。
- 能源记录与实物不符时，先盘点再继续分配。

fallback：

- 没有电量百分比时，用满/半/低/空和使用时长估算。
- 没有纸表时，用胶带标签或木板表格。

related_wiki：

- `energy-daily-energy-budget-001`
- `energy-device-power-priority-tier-001`
- `energy-device-runtime-estimate-card-001`
- `energy-day-end-power-review-001`
- `energy-power-source-status-board-001`
- `general-energy-handover-summary-001`

### DG-0845 关键设备最低电量线

scenario：通信、照明、Core/终端、记录或医疗照护设备电量下降，需要决定何时停止非关键任务。  
steps：

1. 给每类关键设备设最低电量线。
2. 低于线时取消非关键任务和普通充电请求。
3. 将设备改为短窗口使用或交由负责人保管。
4. 确认纸质替代、低亮照明或离线记录路径。
5. 每晚复查最低线是否过高或过低。

check：

- 每台关键设备有最低线和负责人。
- 低于最低线后的动作明确。
- 替代方案可执行。

stop_or_escalate：

- 所有关键设备同时低于最低线时，进入低电量紧急模式。
- 设备异常发热、鼓包、进水时转入 DG-0841/DG-0842，不因最低线继续使用。

fallback：

- 无法读百分比时，用开机次数、亮度变化、掉电速度和历史记录估算。

related_wiki：

- `energy-critical-device-minimum-line-001`
- `energy-essential-load-reserve-001`
- `energy-low-battery-power-window-001`
- `energy-night-lighting-runtime-plan-001`
- `energy-low-power-core-terminal-window-001`

### DG-0846 充电队列和排班

scenario：移动电源、太阳能板或充电口有限，多台设备同时要求补电。  
steps：

1. 收集待充设备编号、电量、用途和截止时间。
2. 按关键等级排序，先通信、照明、记录/医疗照护，再普通工具。
3. 给每个设备安排开始、结束、负责人和目标电量。
4. 充电结束后记录实际变化，归还设备和线材。
5. 排队冲突时由能源负责人按预算表裁决。

check：

- 队列中没有未编号设备。
- 每个充电窗口有结束条件。
- 共享移动电源借还状态清楚。

stop_or_escalate：

- 设备、线缆、接口发热或异味时停止该路充电并转入安全 Guide。
- 队列无法满足关键设备最低线时，取消非关键设备。

fallback：

- 接口不足时只保留一个关键充电口，其他设备改到下一窗口。

related_wiki：

- `energy-charging-queue-schedule-001`
- `energy-charge-discharge-handover-log-001`
- `energy-shared-power-bank-borrow-return-001`
- `energy-charging-port-rotation-rule-001`

### DG-0847 太阳能白天补电排程

scenario：太阳能输出不稳定，需要在白天把有限日照转成关键设备电量。  
steps：

1. 早晨记录天气、遮挡、预计日照窗口和待充设备。
2. 上午先充关键通信/照明或移动电源缓冲。
3. 中午复查面板方向、阴影、温升、线缆和接口。
4. 阴天或输出弱时改为短时补关键设备，不排普通设备。
5. 傍晚记录产出、未完成设备和夜间节电安排。

check：

- 今日目标设备不超过日照能力。
- 阴天有降级目标。
- 傍晚复盘进入次日预算。

stop_or_escalate：

- 面板、线缆、接口发热、进水或打火时停止该路充电。
- 连续两天低产出时收紧通信窗口和夜间照明。

fallback：

- 无法估算输出时，先给移动电源或最低电量关键设备短时补电。

related_wiki：

- `energy-solar-daytime-charge-schedule-001`
- `energy-cloudy-solar-downgrade-001`
- `energy-solar-charge-target-selection-001`
- `energy-solar-weather-pattern-log-001`

### DG-0848 低电量断开负载顺序

scenario：电量不足但未出现电气故障，需要主动断开非关键负载以保留关键功能。  
steps：

1. 列出当前所有用电设备和用途。
2. 先断开娱乐、重复照明、长时间开屏、非必要拍摄。
3. 再断开普通工具、备用设备和可纸质替代任务。
4. 保留最低照明、一次通信窗口、必要记录和医疗照护。
5. 记录断开原因和恢复条件。

check：

- 非关键负载已断开。
- 关键负载仍在最低线以上。
- 团队知道恢复使用条件。

stop_or_escalate：

- 若出现发热、异味、火花或异常掉电，转入 DG-0842，不按普通低电量处理。
- 若断开后仍无法守住关键最低线，进入 DG-0844/DG-0845 的紧急预算。

fallback：

- 没有完整负载清单时，先断开肉眼可见最大耗电和可人工替代设备。

related_wiki：

- `energy-low-battery-load-disconnect-order-001`
- `energy-nonessential-device-suspend-list-001`
- `energy-essential-load-reserve-001`

## 5. Retrieval 风险预测

本阶段只预测，不修改 retrieval。

### power / communication 混淆

问题例：

- “停电三天优先给什么设备充电？”
- “手机快没电怎么安排联系？”
- “对讲机和照明谁优先？”

判断：

- 如果核心问题是“有限电量怎么分配给多个设备”，应由能源管理 Guide 主导，例如 DG-0844 / DG-0845 / DG-0846。
- 如果核心问题是“什么时候开机、发什么消息、怎么避免失联”，应由通信 Guide 主导，例如 DG-0582 / DG-0636。
- “对讲机和照明谁优先”应能源主导，通信作为负载类型；回答要保留通信最低窗口，不让通信 Guide 直接覆盖能源预算。

风险：

- 现有 DG-0582、DG-0636 的关键词“低电量、开机窗口”很强，可能抢走能源预算类问题。
- 后续 Field Test 应包含跨域问题，检查能源 Guide 是否作为主入口、通信 Wiki 是否作为补充。

### power / computer 混淆

问题例：

- “Core 快没电了先关什么？”
- “终端低电量怎么办？”
- “离线设备怎么省电？”

判断：

- 如果问题是 Core/终端作为用电负载的开机窗口、最低电量线和关机顺序，应能源管理主导。
- 如果问题是数据保存、同步流程、任务系统或终端固件行为，应由数据/终端系统文档或未来终端 Guide 主导。
- Batch5-E 可以写 `energy-low-power-core-terminal-window-001`，但不要引入 Terminal Sync、任务系统或固件逻辑。

风险：

- “Core、终端、同步”可能召回 Terminal Sync 或任务系统内容。
- Apply 阶段应避免写“上传、拉取、commit、固件”细节，只写电量窗口和手工记录边界。

### power / safety 混淆

问题例：

- “电量不够要不要关照明？”
- “晚上只留一盏灯可以吗？”

判断：

- 如果问题是跌倒、外部暴露、夜间巡查，应由安全/照明 Guide 补充。
- 如果问题是有限电量下如何保证最低夜间照明续航，应能源管理主导。
- 回答应同时保留“低亮不等于无灯”：通道、厕所、水源、照护点必须有最低照明。

风险：

- 低电量策略可能误导为过度关灯，损害夜间安全。
- `energy-night-lighting-runtime-plan-001` 必须包含安全最低点位和停止条件。

### power / solar 混淆

问题例：

- “阴天太阳能板还要展开吗？”
- “今天云很多先充哪个？”
- “太阳能板中午很热怎么办？”

判断：

- “阴天是否展开、先充谁、日照窗口怎么排”应由太阳能能源管理 Guide 主导。
- “中午很热、进水、接口发热、打火”应转入 P0 安全边界或 DG-0842/DG-0843。
- 不应让太阳能百科索引覆盖行动建议。

风险：

- 现有 DG-0564、DG-0617 已强，新增 DG-0847 可能与旧太阳能排程竞争。
- Apply 阶段可以复用 DG-0617 而不一定新增 DG-0847；若新增，需要明确 DG-0847 偏“阴天/多日低产出/目标选择”。

## 6. 不建议加入内容

Batch5-E 不建议加入：

- P0 电气安全重复扩展。
- 发电机深度维护。
- 逆变器改装。
- USB PD / 快充协议百科。
- 终端同步代码。
- Core 任务系统。
- 通信协议细节。
- 太阳能控制器参数大全。
- 高压电、市电维修。
- 电池拆解、电芯焊接、BMS 改装。
- 太阳能系统设计公式大全。
- 云服务、在线电价、购买推荐。

## 7. Batch5-E Apply 范围

建议下一阶段 Apply 先做 20 到 25 篇，不一次性写满 27 篇。

优先 Apply 第一批 23 篇：

- P1：18 篇
  - `energy-daily-energy-budget-001`
  - `energy-critical-device-minimum-line-001`
  - `energy-device-power-priority-tier-001`
  - `energy-charging-queue-schedule-001`
  - `energy-low-battery-power-window-001`
  - `energy-solar-daytime-charge-schedule-001`
  - `energy-cloudy-solar-downgrade-001`
  - `energy-night-lighting-runtime-plan-001`
  - `energy-backup-battery-rotation-ledger-001`
  - `energy-charge-discharge-handover-log-001`
  - `energy-low-battery-load-disconnect-order-001`
  - `energy-shared-power-bank-borrow-return-001`
  - `energy-device-runtime-estimate-card-001`
  - `energy-essential-load-reserve-001`
  - `energy-nonessential-device-suspend-list-001`
  - `energy-day-end-power-review-001`
  - `energy-solar-charge-target-selection-001`
  - `energy-charging-port-rotation-rule-001`
- P2：5 篇
  - `energy-wh-mah-local-estimate-001`
  - `energy-device-consumption-baseline-001`
  - `energy-solar-weather-pattern-log-001`
  - `energy-battery-aging-capacity-note-001`
  - `general-energy-handover-summary-001`

暂缓：

- `energy-power-source-status-board-001`
- `energy-low-power-core-terminal-window-001`
- `energy-cold-weather-runtime-adjustment-001`
- `energy-load-category-vocabulary-001`

Guide：

- 最多新增 5 个。
- 若要更小批量，优先新增 DG-0844、DG-0845、DG-0846；DG-0847 可复用/增强现有 DG-0617 的关联，DG-0848 可视是否与 DG-0619 重叠后决定。

禁止：

- 不改 Retrieval。
- 不改 Prompt。
- 不改 query profile。
- 不改 top_k。
- 不改 selector limit。
- 不新增 category。
- 不进入 stable，Apply 后必须做 Field Test。

## 8. 验收建议

Batch5-E Apply 后建议验收：

- Wiki audit：errors=0 warnings=0 advisories=0
- Guide audit：errors=0 warnings=0 advisories=0
- Guide-Wiki 单边关系：0
- 无效 Guide ID：0
- 无效 Wiki slug：0
- 不新增 category
- 不修改 Retrieval Pipeline
- 不修改 Prompt
- 不修改 query profile
- 不修改 top_k
- 不修改 selector limit
- 不修改 fallback 架构
- PocketBase wiki_articles 与 Markdown 同步
- 后续必须进入 Energy Management Field Test，不直接宣布 stable

Field Test 建议覆盖：

- 停电三天优先给什么设备充电。
- 手机快没电但还要保持联系。
- 对讲机和照明谁优先。
- 阴天太阳能还要不要展开。
- 云很多时先充移动电源还是手机。
- 晚上只留一盏灯是否安全。
- 共享移动电源借走后剩余电量不清。
- 备用电池轮换记录丢失。
- Core 或终端低电量时如何安排短窗口。
- 低电量时按什么顺序断开负载。

目标不是一次全 pass，而是确认能源管理知识能进入 evidence，并清楚区分能源预算、通信窗口、夜间安全和太阳能排程。
