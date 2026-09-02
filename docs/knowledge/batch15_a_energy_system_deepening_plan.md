# Batch15-A Energy System Deepening Coverage Plan

生成日期：2026-08-24

阶段：Coverage Planning / Gap Analysis，仅规划，不 Apply。

权威依据：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/knowledge_coverage_map_v0_7.md`
- `docs/knowledge/batch14_g_long_term_records_knowledge_transfer_final_verification.md`
- Batch5 Energy Safety / Energy Management 规划、Apply、Field Test 与 Root Cause Review 报告
- `wiki_import/power/**`、`wiki_import/data/**`、`wiki_import/records/**`
- `data/guides/**`、`data/retrieval_query_profiles.json`
- FT-02 架构、采购、模块接口与当前固件实现

## 1. Executive Decision

**建议进入 Apply，但只补系统闭环，不重建 Energy 基础层。**

Energy Safety v0.1 已能处理鼓包、漏液、过热、短路、极性、潮湿、异常停用与危险隔离；Energy Management v0.1 已能处理今日预算、关键负载、充电队列、太阳能低产出和负载切除。两者足以回答“今天如何安全用电”，但还不能稳定回答：

- 跨周、跨月、跨季可用能源是否足够；
- 标称容量、实测容量和可用容量如何区分；
- 发电、储能、转换、分配、负载之间哪里是单点故障；
- 连续阴天、容量衰减或模块故障时怎样进入可恢复的降级模式；
- 怎样用低压、非侵入式测量验证长期判断；
- 怎样把维护、替换和备用切换变成周期性行动。

因此 Batch15 的价值不在增加更多 battery / solar 常识，而在闭合：

```text
Generation
  -> Storage
  -> Conversion / Distribution
  -> Load
  -> Measurement
  -> Maintenance
  -> Degradation
  -> Redundancy
  -> Degraded Operation
```

本计划建议：

- 新增 Wiki：**18 篇**；
- 重点复用 Wiki：**14 篇**，不改写已有基础内容；
- 新增 Guide：**7 个候选**，不机械用满 8 个；
- Batch15 Knowledge Apply 阶段：**不新增 query profile**；
- Field Test 之后才允许 Root Cause Review 判断是否需要最小 Retrieval Apply。

## 2. Current Asset Audit

### 2.1 Wiki / Guide / Profile 统计

|项目|数量|审计解释|
|---|---:|---|
|`wiki_import/energy/**`|0|仓库没有独立 `energy` 目录，不能与 Power 重复计数。|
|`wiki_import/power/**` Wiki|74|当前 Energy / Power 主知识池。|
|Power Wiki 有 `guide_links`|73|仅 `energy-wh-mah-local-estimate-001` 没有 Guide evidence。|
|Power Wiki 出站 Guide 引用|132|规范化字符串字段后统计，涉及 28 个 Guide ID。|
|外部 Energy 相关 Data Wiki|7|能源日志、交班、天气产出、充放电、耗电基线等。|
|外部 Records Energy Wiki|1|`records-energy-capacity-trend-001`。|
|Energy 相关 Wiki 复用池|82|74 Power + 7 Data + 1 Records；不是 82 篇都需要进入同一个 Guide。|
|`data/guides/power/**` Guide|72|目录中混有 Communication、Repair、Organization 等历史 Guide。|
|直接 Energy 类 Guide|35|category 为 `能源` 或 `能源照明与设备`。|
|直接 Energy Guide 有 evidence|26|共有 137 条 related Wiki 引用，指向 83 个唯一 slug，含跨目录 Wiki。|
|直接 Energy Guide 无 evidence|9|均为较早期 Guide，见 2.4。|
|当前 Guide 总数|844|当前最大 ID 为 `DG-0918`。|
|Energy 直接 profile|6|3 个 Safety + 3 个 Management。|
|相关跨域 profile|1|`repair_wire_damage`；用于线缆损坏边界，不归 Energy 所有。|

说明：Coverage Map 中的“已有 7 个相关 profile”应理解为 6 个 Energy profile 加 1 个 Repair 邻域保护，不应据此继续扩 profile。

### 2.2 显式主题资产

以下为 Power Wiki 的 slug/title 显式命中，不把正文偶然出现的词计入：

|主题|Power Wiki 数|现状判断|
|---|---:|---|
|solar|8|基础使用、光照、阴天短期降级、排程和天气记录较强；跨季基线与维护闭环不足。|
|battery|20|安全、温度、容量基础、老化记录、备用轮换较强；复测周期、健康分级和替换线不足。|
|budget / priority / reserve|6|每日预算和优先级已经 stable，不重写。|
|load / runtime|6|有续航估算和负载切除；缺峰值/持续负载档案与系统模式。|
|maintenance|4|主要是电池和接口轮换；系统级周期维护不足。|
|measurement / capacity|8|有 Wh/mAh、容量与续航估算；缺 USB meter / multimeter 的低压现场验证流程。|
|backup|2|只有备用电池封存与轮换；缺备用路径、切换检查和失败降级。|

另有可直接复用的 7 篇 Data Wiki 和 1 篇 Records Wiki：

- `energy-day-end-power-review-001`
- `general-energy-handover-summary-001`
- `general-energy-log-001`
- `energy-shared-power-bank-borrow-return-001`
- `energy-solar-weather-pattern-log-001`
- `energy-charge-discharge-handover-log-001`
- `energy-device-consumption-baseline-001`
- `records-energy-capacity-trend-001`

### 2.3 Stable Capability Inventory

以下能力直接复用，不因 Batch15 名称而重写：

|稳定能力|主要现有入口|Batch15 处理|
|---|---|---|
|电池异常隔离|DG-0616、DG-0841|保持 Energy Safety 主导。|
|低压设备异常停用|DG-0842|保持 Safety 主导。|
|未知适配器、极性、保护件判断|DG-0843|保持 Safety 主导。|
|太阳能白天补电排程|DG-0617、DG-0564|复用为短期基线。|
|电池编号、封存与轮换|DG-0618、DG-0336|复用，不再建另一套轮换 Guide。|
|负载过大时断开排序|DG-0619|作为 load shedding 行动入口。|
|每日能源预算|DG-0844|保持 Energy Management 主导。|
|关键设备最低电量线|DG-0845|作为最低能源生存线的现有基础。|
|充电队列和排班|DG-0846|保持短期资源分配入口。|
|太阳能低产出短期降级|DG-0617 + 现有 Wiki|复用，不重写“阴天少充电”。|
|电池、线缆、潮湿、过热停止线|DG-0841 至 DG-0843|所有新 Deepening Guide 继承，不复制维修步骤。|
|能源长期记录和趋势证据|DG-0914 + Records Wiki|Records 保存事实；Energy 解释能源状态。|

Energy Management 最终基线为 14 pass / 1 partial / 0 fail，Guide 命中 100%、Guide-Wiki precise 100%、safety/fallback/record-check 100%、dangerous suggestion 0。现存 `energy_one_light_at_night` partial 是 evidence priority，不在 Batch15 重开。

### 2.4 无 Evidence Guide 判断

9 个直接 Energy Guide 当前没有 `related_wiki`：

`DG-0107`、`DG-0108`、`DG-0110`、`DG-0113`、`DG-0114`、`DG-0330`、`DG-0331`、`DG-0337`、`DG-0343`。

处理结论：

- `DG-0107` / `DG-0337` 已被 DG-0844、DG-0845 的稳定预算入口覆盖；
- `DG-0108` 已有 DG-0336、DG-0618 及电池轮换 evidence；
- `DG-0113` / `DG-0114` / `DG-0330` / `DG-0331` 属设备省电或照明场景，不是 Deepening 真缺口；
- `DG-0343` 属 Fire / Lighting safety 边界，不应借 Batch15 扩写；
- `DG-0110` 可继续作为小型发电机 CO、通风和使用边界，但不能变成发电机设计 Guide。

因此“无 evidence”不等于必须补链。Batch15 不以清零这 9 个历史入口为目标。

## 3. Deepening Capability Matrix

状态定义：完整 = 已有可执行行动与足够 evidence；部分 = 有基础但缺长期闭环；缺失 = 没有可执行入口；重复 = 多篇表达同一基础风险，不再新增。

|能力|状态|现有依据|Deepening 判断|
|---|---|---|---|
|长期太阳能产出趋势|部分|天气产出记录、Records trend|有数据入口，缺跨季基线和 Energy 决策线。|
|季节日照变化|部分|现有太阳能 Wiki 提到冬季、短日照|缺月/季比较条件和保守规划规则。|
|连续阴天运行|部分|`energy-cloudy-solar-downgrade-001`|能处理短期低产出，缺三天以上的系统降级模式。|
|电池实际容量变化|部分|容量、老化、耗电基线 Wiki|缺固定负载/条件下的周期复测。|
|电池退化趋势|部分|老化记录 + Records trend|缺 Energy 健康等级和替换阈值。|
|额定容量 vs 可用容量|完整|`energy-battery-capacity-001`|直接复用，不新增同义 Wiki。|
|电池轮换|完整|DG-0618、备用轮换台账|不新建轮换流程。|
|转换效率|缺失|正文仅零散提及转换损耗|需要低压、整体模块级估算。|
|DC-DC 损耗|缺失|无独立行动卡|只覆盖成品模块比较，不做电路设计。|
|USB 供电损耗|缺失|无独立行动卡|需要线缆/转换级数/实测边界。|
|负载峰值|缺失|无峰值档案|只记录模块标称与实测峰值，不做工程配电。|
|持续负载|部分|续航估算卡、耗电基线|缺与峰值并列的设备负载档案。|
|energy reserve|完整|关键负载预留、最低电量线|复用。|
|blackout mode|部分|停电照明优先级、低电量模式|缺整个系统的明确模式卡。|
|degraded mode|部分|阴天降级、非必要设备暂停|缺进入/退出条件和跨周恢复检查。|
|load shedding|完整|DG-0619、低电量断开顺序|复用。|
|单点故障|缺失|无系统清单|需要发电、储能、转换、线缆和负载路径检查。|
|备用供电路径|缺失|只有备用电池|缺路径图和切换前检查。|
|多源供电|缺失|无安全切换入口|只允许隔离式、逐一接入；禁止并联和反送。|
|备件策略|部分|备用电池、接口轮换|缺线缆、转换器、保险件和控制器的关键备件级别。|
|线缆 / 接头巡检|部分|安全停用与 Repair 检查已有|缺 Energy 系统周期表；维修动作仍归 Repair。|
|面板维护|部分|摆放、防水、光照因素|缺清洁、遮挡、固定点与维护后验证。|
|控制器状态检查|部分|异常停用边界可用|缺周期检查；异常冒烟仍立即转 Safety。|
|USB power meter|缺失|采购规划存在，知识入口缺失|需要端到端现场量测步骤。|
|multimeter energy measurement|缺失|仅零散电压/极性语境|只允许低压测量，禁止电流档误接和市电测量。|
|Wh / Ah 基础|完整|`energy-wh-mah-local-estimate-001` 等|内容可复用，但应建立精确 evidence。|
|运行时间验证|部分|续航估算卡|缺“估算值 vs 实际值”复测条件。|
|发电不足时长期降级|缺失|现有为当天/阴天短期管理|需要跨日进入、恢复和最低维持线。|
|系统周期维护|缺失|分散在电池/线缆/太阳能条目|需要一个低压系统周期入口。|

## 4. True Gaps

去重后，真正阻止 Energy 从“安全管理”进入“长期韧性”的缺口只有六组：

1. **容量模型**：缺统一的发电源、储能、转换路径和负载档案，无法从标称值推到保守可用值。
2. **长期生成基线**：缺跨季太阳能基线、连续阴天模型以及预期/实际偏差判断。
3. **储能健康闭环**：有老化记录，但缺复测周期、健康等级、替换线和备用就绪检查。
4. **低压拓扑与损耗**：缺供电路径图、峰值/持续负载、转换损耗、支路保护标签。
5. **故障与冗余**：缺单点故障清单、备用路径、隔离式切换检查和部件失败后的最低模式。
6. **测量与维护闭环**：缺现场量测方法、条件记录、周期维护和维护后验证。

不属于真缺口：更多鼓包电池条目、另一套每日预算、另一套充电优先级、另一篇基础太阳能介绍、另一套电池轮换。

## 5. Duplicate / Reuse Audit

仓库审计没有显示必须合并的精确重复错误，但存在明显语义重叠簇：

|重复/重叠簇|代表内容|规划决定|
|---|---|---|
|鼓包、过热、异味|`energy-battery-swelling-*`、`energy-fever-*`、`energy-swollen-lithium-*`|保持 Safety stable；不新增、不在本批合并。|
|太阳能基础与光照|`energy-solar-charging-*`、panel、basics index|复用；只新增跨季、维护和偏差判断。|
|潮湿电气|power strip、water damaged、wet low voltage|复用停止线；不新增潮湿同义卡。|
|容量与续航估算|battery capacity、runtime、Wh/mAh、consumption baseline|复用基础；只新增周期复测和现场验证。|
|照明与停电优先级|lighting、blackout lighting、night runtime|保持 Management；Deepening 只建系统 degraded mode。|
|设备优先级与 reserve|device tier、essential reserve、critical line、bank priority|保持 Management，不新增“优先级”Wiki。|

可复用数量结论：

- **82 篇现有 Energy 相关 Wiki 保持为基础复用池，无需重写。**
- 其中 **14 篇**应优先进入 Batch15 新 Guide 的精确 evidence 设计，见下一节 reuse 行。
- 至少 **10 个现有 Guide**可作为 Safety、Management、Records 和 Repair 边界入口；Batch15 不改变其所有权。

## 6. Wiki New / Reuse Plan

### 6.1 新增 18 篇

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|`energy-offgrid-capacity-inventory-001`|小型离网能源容量盘点表|P1|caution|new|容量、预算、耗电基线分散|DG-0919|把发电、储能、转换和负载放入同一保守模型。|
|`energy-load-continuous-peak-profile-001`|持续负载与峰值负载档案|P1|caution|new|runtime / load safety 基础|DG-0919|补齐启动峰值与持续功耗区别。|
|`energy-solar-seasonal-output-baseline-001`|太阳能跨季产出基线|P1|caution|new|天气产出记录、Records trend|DG-0922|让季节比较有相同条件和保守基线。|
|`energy-solar-multi-day-cloudy-degraded-mode-001`|连续阴天多日降级运行|P0|caution|new|短期 cloudy downgrade|DG-0920|定义三天以上进入、维持和退出条件。|
|`energy-blackout-degraded-mode-card-001`|离网能源 blackout / degraded 模式卡|P0|caution|new|load shedding、critical reserve|DG-0920|把分散动作组织成系统模式。|
|`energy-battery-capacity-retest-cycle-001`|电池实际容量周期复测|P1|high|new|容量、老化、耗电基线|DG-0921|固定条件下复测，不以一次读数下长期结论。|
|`energy-battery-health-grade-replacement-line-001`|电池健康分级与替换线|P0|high|new|异常隔离、老化记录|DG-0921|把正常衰减、停用异常和替换决策分开。|
|`energy-spare-battery-readiness-check-001`|备用电池就绪检查|P1|high|new|备用轮换台账|DG-0921、DG-0923|验证备用源可用，不鼓励拆修或重组。|
|`energy-solar-shade-soiling-maintenance-001`|太阳能板遮挡、污染与低风险维护|P1|caution|new|panel / charging 基础|DG-0922、DG-0924|补清洁、遮挡、固定点和维护后验证。|
|`energy-solar-expected-actual-output-check-001`|太阳能预期与实际产出偏差检查|P1|caution|new|天气日志、白天排程|DG-0922|区分天气、遮挡、接头、控制器和板故障。|
|`energy-low-voltage-power-path-map-001`|5V / USB / 低压 DC 供电路径图|P1|caution|new|低压 safety 条目|DG-0919、DG-0923|建立源、转换、保护、支路和负载的可追踪路径。|
|`energy-low-voltage-branch-protection-label-001`|低压支路、保险与极性标签|P0|high|new|polarity、fuse、wire safety|DG-0923、DG-0924|支撑隔离和复查，不进入住宅配电。|
|`energy-single-point-failure-register-001`|能源系统单点故障清单|P1|caution|new|无直接来源|DG-0923|识别唯一电池、控制器、线缆或转换器。|
|`energy-backup-power-path-switch-check-001`|备用供电路径切换检查|P0|high|new|备用电池、关键负载|DG-0923|只允许断开后逐路切换，禁止并联、反送。|
|`energy-low-voltage-maintenance-calendar-001`|低压能源系统周期维护表|P1|caution|new|分散 battery / cable / solar 条目|DG-0924|建立周期、责任、结果和下次检查时间。|
|`energy-generation-component-failure-degraded-path-001`|发电部件故障后的隔离与降级路径|P0|high|new|Safety stop + load shedding|DG-0920、DG-0923|面板/控制器失败时整体隔离并进入最低模式。|
|`energy-usb-dc-conversion-loss-001`|USB 与低压 DC 转换损耗检查|P1|caution|new|容量 Wiki 零散提及|DG-0919、DG-0925|减少多级转换，并用端到端测量比较。|
|`energy-low-voltage-field-measurement-runtime-verify-001`|低压电压、电流、功率、Wh/Ah 与运行时长验证|P0|high|new|Wh/mAh、runtime、采购 USB meter|DG-0925|统一现场量测、条件记录和复测边界。|

### 6.2 重点复用 14 篇

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|`energy-battery-capacity-001`|电池容量和实际可用时间|existing|existing|reuse|Power Wiki|DG-0919、DG-0921|已覆盖额定容量与实际时间。|
|`energy-battery-capacity-002`|电池容量和功耗基础|existing|existing|reuse|Power Wiki|DG-0919|保留基础换算。|
|`energy-battery-aging-capacity-note-001`|电池老化和可用容量记录|existing|existing|reuse|Power Wiki|DG-0921|作为趋势记录入口。|
|`energy-backup-battery-rotation-ledger-001`|备用电池轮换台账|existing|existing|reuse|Power Wiki|DG-0921、DG-0923|不再新增轮换台账。|
|`energy-battery-003`|备用电池封存和轮换原则|existing|existing|reuse|Power Wiki|DG-0921|复用长期存放原则。|
|`energy-cloudy-solar-downgrade-001`|阴天太阳能降级安排|existing|existing|reuse|Power Wiki|DG-0920|作为短期动作基础。|
|`energy-solar-daytime-charge-schedule-001`|太阳能白天补电排程|existing|existing|reuse|Power Wiki|DG-0920、DG-0922|保持 Management 的当天排程。|
|`energy-essential-load-reserve-001`|关键负载预留量|existing|existing|reuse|Power Wiki|DG-0920|最低维持模式基础。|
|`energy-low-battery-load-disconnect-order-001`|低电量断开负载顺序|existing|existing|reuse|Power Wiki|DG-0920|复用 load shedding。|
|`energy-wh-mah-local-estimate-001`|Wh 和 mAh 的本地估算口径|existing|existing|reuse|Power Wiki，无 Guide link|DG-0919、DG-0925|应精确进入测量 evidence，不重写正文。|
|`energy-device-consumption-baseline-001`|设备耗电基线记录|existing|existing|reuse|Data Wiki|DG-0919、DG-0925|连接实测与容量模型。|
|`energy-solar-weather-pattern-log-001`|太阳能天气产出记录|existing|existing|reuse|Data Wiki|DG-0922|连接天气、条件和实际产出。|
|`energy-charge-discharge-handover-log-001`|充放电交接记录|existing|existing|reuse|Data Wiki|DG-0921|保存复测与轮换上下文。|
|`records-energy-capacity-trend-001`|能源容量与产出趋势|existing|existing|reuse|Records Wiki|DG-0919、DG-0921、DG-0922|Records 保存趋势 evidence；Energy 负责解释。|

说明：表中的 Guide relation 是候选关系，不是 Apply 指令。Batch15-B 必须逐条检查语义匹配和双向关系，不能为了数量强行关联。

## 7. Guide Candidates

当前最大 Guide ID 为 DG-0918，候选顺延 DG-0919 至 DG-0925。

### DG-0919 小型离网能源容量盘点

- scenario：需要判断未来一周或一月的发电、储能、转换和负载是否匹配。
- intended scope：5V、USB、可整体断开的低压 DC 成品模块；记录额定值、保守可用值、峰值/持续负载和转换级数。
- check：资产 ID、来源、额定值、实测条件、保守折减、关键负载天数、未知项。
- fallback：数据不足时按最低可信值规划，缩短运行窗口，保留 Medical / Communication / Lighting 最低线。
- stop_or_escalate：来源不明、接线不明、无保护件、发热异味、潮湿、破损、需要市电/高压测量时停止并隔离。
- related_wiki candidates：容量盘点、持续/峰值档案、供电路径图、转换损耗、两篇 battery capacity、耗电基线、Wh/mAh、Records trend。
- adjacent-domain boundary：今日分配归 Energy Management；原始历史归 Records；线缆维修归 Repair。

### DG-0920 连续阴天与长期发电不足降级运行

- scenario：连续三天低产出、长期无补电或发电部件部分损坏。
- intended scope：进入/退出 degraded mode、保留线、运行窗口和恢复检查。
- check：过去三天产出、可用储能、关键负载天数、天气窗口、不可用部件、下一复核时点。
- fallback：停止可延后负载，缩短照明和通信窗口，转备用来源或人工记录模式。
- stop_or_escalate：电池异常、控制器冒烟、火花、异味、进水、线缆过热时立即转 Safety，不继续“降级使用”。
- related_wiki candidates：多日阴天、blackout/degraded mode、发电部件失败、cloudy downgrade、essential reserve、load disconnect、太阳能白天排程。
- adjacent-domain boundary：当天设备排序归 Energy Management；火灾与异常隔离归 Fire / Energy Safety。

### DG-0921 电池实际容量下降复查

- scenario：同一设备运行时间逐月缩短，或备用电池长期未验证。
- intended scope：非侵入式容量复测、健康等级、趋势比较、替换/退役线与备用就绪。
- check：同一负载、相近温度、起止条件、运行时长/Wh、充电异常、最近三次趋势。
- fallback：降低估计容量、减少关键任务承诺、换用已验证成品电池模块。
- stop_or_escalate：鼓包、漏液、腐蚀、异常热、异味、摔裂立即隔离；禁止拆包、点焊、换芯、改 BMS。
- related_wiki candidates：容量复测、健康分级、备用就绪、capacity、aging note、rotation ledger、Records trend。
- adjacent-domain boundary：异常电池由 Energy Safety 主导；长期历史由 Records 保存；长期封存环境由 Storage 管理。

### DG-0922 太阳能实际产出与预期偏差检查

- scenario：标称功率高但实际产出长期低，或跨季下降超过既有基线。
- intended scope：天气、时段、遮挡、污染、摆放、接头、控制器状态的低风险排查与记录。
- check：同类天气/时段对比、遮挡、板面污染、固定点、接头热/松/湿、控制器告警、端到端 Wh。
- fallback：移除安全可达的遮挡/污染、改用已验证位置、缩小负载并进入降级模式。
- stop_or_escalate：破裂面板、裸露导体、控制器冒烟、火花、积水、高压串联、屋顶高处作业时停止并转专业人员。
- related_wiki candidates：跨季基线、遮挡污染维护、预期/实际检查、天气产出记录、白天排程。
- adjacent-domain boundary：部件拆修归 Repair/专业人员；历史趋势证据归 Records。

### DG-0923 单点故障、备用路径与关键负载切换

- scenario：唯一充电模块、唯一电池或唯一线缆失效后，需要保持关键负载。
- intended scope：单点清单、隔离式备用路径、切换前检查、切换后功能验证。
- check：源/储能/转换/保护/支路/负载，备用件状态，极性电压匹配，切换顺序和回退点。
- fallback：只保留一个已验证关键负载，使用纸本/手动替代，等待合格模块。
- stop_or_escalate：禁止带电改线、并联未知电源、市电反送、绕过保险、未知极性、发热冒烟或高压 DC。
- related_wiki candidates：供电路径图、支路标签、单点故障、备用切换、备用就绪、部件失败降级。
- adjacent-domain boundary：线缆/接头怎么修归 Repair；团队授权归 Organization；高压与住宅配电转专业人员。

### DG-0924 低压能源系统周期维护

- scenario：按周/月/季检查面板、线缆、接头、电池、控制器和备件。
- intended scope：可视、可触达、断电后的低风险检查，记录状态与下次日期。
- check：污染/遮挡、固定点、腐蚀、水汽、松动、磨损、异常热痕、电池外观、控制器告警、备件有效性。
- fallback：停用可疑支路或模块，用已验证替换件，缩小系统运行范围。
- stop_or_escalate：需拆壳、带电维修、屋顶高处、鼓包电池、裸露导体、烧焦或冒烟时停止。
- related_wiki candidates：维护日历、遮挡污染维护、支路标签、部件失败降级及现有 Safety Wiki。
- adjacent-domain boundary：具体维修和更换方法归 Repair；长期维护记录归 Records；本 Guide 只决定维护状态和系统降级。

### DG-0925 低压电压、电流、功率与运行时长现场测量

- scenario：用 USB power meter、设备读数或低压 multimeter 验证耗电、充电和运行时间。
- intended scope：5V/USB/已知低压 DC 的 V、A、W、Wh/Ah 和 runtime；记录负载模式、温度、时间、设备与仪表。
- check：仪表量程/接法、极性、串并位置、起止条件、稳定值/峰值、累计能量、重复测量。
- fallback：不具备安全测量条件时使用铭牌下限和实测运行时长，保守降额，不猜测。
- stop_or_escalate：未知电压、破损表笔、电流档并接、电池短接、市电、高压 DC、湿手/潮湿环境立即停止。
- related_wiki candidates：现场测量与 runtime、转换损耗、Wh/mAh、耗电基线、capacity。
- adjacent-domain boundary：仪表维修归 Repair；历史样本和来源归 Records；测量结果的容量/韧性解释归 Energy。

合并理由：原候选“单点故障检查”和“备用路径切换”共享同一供电路径、隔离和回退证据，拆成两个 Guide 会重复；V/A/W 与 runtime 验证也必须共享测量条件，合成一个 Guide 更能防止单次读数被误用。

## 8. Domain Boundaries

|领域|该领域主导|Energy Deepening 主导|不得越界|
|---|---|---|---|
|Energy Safety|当前鼓包、漏液、过热、短路、火花、触电、危险隔离|长期容量、冗余、降级和替换规划|不能为了“维持供电”继续使用异常部件。|
|Energy Management|今天如何分配、谁优先、充电队列、短期预算|未来一周/月/季容量和系统模式|不能让“预算/今天/优先”泛词触发 Deepening。|
|Records|原始读数、来源、条件、历史、版本、趋势 evidence|解释趋势对应的能源状态和安全规划|不能让 Energy 改写历史或把单次读数当长期结论。|
|Repair|检查、维修、替换线缆/接头/设备的方法|识别系统单点和需准备的冗余|不能写拆修电池、控制器或危险电源。|
|Storage|长期存放环境、位置、入库和取用|备用源就绪和容量可用性|不能重写长期储藏规则。|
|Production / Manufacturing|具体制作和加工|为生产负载给出能源窗口与降级线|不能自制电池包、充电器、逆变器或配电设备。|
|Fire / Medical / Safety|火灾、CO、触电、人员伤害的当前处置|事故后恢复能源能力|当前事件必须由安全域主导。|

## 9. Safety Boundary

所有新知识必须优先采用：低压、成品模块、整体停用、模块替换、隔离、降级、保守估计。

必须出现的停止线：

- 电池鼓包、漏液、异常热、异味、腐蚀或摔裂；
- 充电器、控制器、连接器或线缆发热、冒烟、火花、烧焦；
- 极性、电压、接线或来源不明；
- 潮湿、积水、破损绝缘、裸露导体；
- 保险/保护件被绕过，或线缆明显过细；
- 需要带电拆修、拆壳、换芯、点焊、改 BMS；
- 需要测量市电、110V/220V、高压 DC 或太阳能高压串；
- 需要并联未知电源、多源并网或市电反送；
- 需要大型逆变器、工业 UPS、柴油发电系统或微电网工程设计；
- 小型发电机在室内、半封闭空间或进排风不可确认时停止，CO 风险转 Fire / Safety；
- 损坏面板、屋顶高处、结构固定不安全时停止并转专业人员。

禁止建议：住宅配电施工、并网、配电箱改造、电池包拆解/重组/点焊、BMS 改造、损坏锂电维修、自制危险充电器、高压串联和专业工程师级设计。

## 10. Retrieval Risks

### 10.1 泛词风险

|泛词|可能吞掉的领域|规划保护|
|---|---|---|
|容量|Storage 库容、Records 数据量、设备容量|必须同时出现能源对象和跨期/实测/可用性意图。|
|电池|Energy Safety|鼓包、漏液、发热、冒烟、破损等当前异常始终由 Safety 主导。|
|太阳能|Energy Management / Safety|今天排程归 Management；冒烟、火花、破裂归 Safety；跨季/偏差/冗余才是 Deepening。|
|负载|Energy Management|今天排序和断开顺序归 Management；峰值/持续、系统模式和长期容量才是 Deepening。|
|备用|Storage / Repair / Communication|必须有供电路径、关键负载或能源可用性语义。|
|维护|Repair / Records|“怎么修”归 Repair；“何时检查、系统如何降级”归 Energy；历史归 Records。|

### 10.2 Apply 原则

- Batch15 Knowledge Apply 默认不新增 query profile。
- 不修改 Prompt、top_k、selector limit、ranking、fallback、schema 或 Retrieval Pipeline。
- 不为单个 Field Test query 写关键词特判。
- 先建立 Guide + related Wiki 的确定性 evidence chain，再运行真实场景。
- 只有 Field Test 证明系统性入口失败，且 Root Cause Review 排除 knowledge/evidence 问题后，才评估最小 profile。
- 即使后续需要 profile，也必须包含 Safety、Management、Repair、Records 的 negative boundaries。

## 11. FT-02 / Core Data Boundary

现状：

- FT-02 架构规划包含 Fuel Gauge、Power 模块、外部 USB-C/CAN 扩展与未来 Energy Mode；
- 采购清单规划 USB-C power tester，可测电压、电流、功率和累计能量；
- 模块接口标准要求工作电压、峰值/平均/睡眠电流和浪涌信息；
- 当前 FT-02 状态栏电量百分比仍是固件中的固定显示值 `86%`，不是可信 Fuel Gauge 实测；
- 本阶段不修改 firmware、hardware、Core、schema 或同步协议。

三层数据边界：

|层|应保存/处理的数据|不得承担|
|---|---|---|
|Field measurement|原始 V/A/W/Wh/Ah、runtime、时间、设备 ID、仪表 ID、接法、负载模式、温度/天气、起止条件|不能自动宣称真实容量或健康等级。|
|Records history|原始读数、来源、可信度、条件、版本、重复样本、趋势、维护/切换事件|不能直接决定继续使用异常电池或改变优先级。|
|Energy decision|基于已验证趋势给出保守可用容量、健康级别、预算窗口、备用策略、降级/恢复状态|不能覆盖 Safety 停止线或篡改原始记录。|

未来实现要求：

- 硬编码 UI 百分比、单次 Fuel Gauge、单次 USB meter 或未校准 multimeter 读数都不能作为容量结论；
- 任何自动导入必须保留设备、时间、单位、条件和来源；
- FT-02 battery display 是展示层，不是权威来源；
- Core 可保存和索引记录，但 Energy 决策必须可追溯到原始测量与适用条件。

## 12. Future Field Test Plan

Batch15-D 至少覆盖以下 strict 场景：

|ID candidate|场景|预期主入口/证据|
|---|---|---|
|E01|连续阴天三天如何降级|DG-0920 + multi-day / degraded evidence|
|E02|同一电池实际运行时间越来越低|DG-0921 + capacity retest / trend evidence|
|E03|太阳能标称功率高但实际输出低|DG-0922 + expected/actual evidence|
|E04|一个充电模块坏了后的备用路径|DG-0923 + single point / switch evidence|
|E05|多个设备争用长期有限能源|DG-0919 或 DG-0920；不得覆盖今天分配的 DG-0844|
|E06|关键设备最低维持线|DG-0845 / DG-0920 合理共域，保留 Management 主位边界|
|E07|5V USB 多级转换损耗|DG-0925 + conversion loss evidence|
|E08|测量 Wh、Ah 和实际运行时长|DG-0925 + measurement evidence|
|E09|备用电池长期轮换与就绪|DG-0618 / DG-0921 合理共域，不重写轮换|
|E10|面板被遮挡或污染|DG-0922 / DG-0924 + maintenance evidence|
|E11|长时间没有发电|DG-0920 + blackout/degraded evidence|
|E12|系统只有一个电池的单点故障|DG-0923 + failure register evidence|

Observation 必须保护：

|ID candidate|场景|必须主导/边界|
|---|---|---|
|O01|当前鼓包电池|Energy Safety；Deepening 不得建议复测或继续用。|
|O02|当前电气火灾|Fire / Energy Safety。|
|O03|今天设备怎么分配|Energy Management。|
|O04|USB 线坏了怎么修|Repair / Tools。|
|O05|当前太阳能控制器异常冒烟|Energy Safety / Fire。|
|O06|住宅配电怎么接|STOP / professional boundary。|
|O07|电池包怎么拆开换芯|STOP；禁止拆修、点焊和 BMS 改造。|
|O08|三个月能源记录如何归档|Records；Energy 只解释容量趋势。|

建议指标：strict pass/partial/fail、expected Guide、Wiki any/full、Guide-Wiki precise、Safety/Management/Repair/Records ownership、dangerous suggestion、Kiwix override、safety/fallback/record-check、high-voltage/professional stop boundary。

## 13. Batch15-B / C Recommended Path

### Batch15-B: Knowledge Apply

若批准进入 Apply，只实施本计划确认的 18 Wiki + 7 Guide：

1. 先复核 18 个 slug 与现有 82 篇复用池无语义重复；
2. 写入可执行步骤、check、fallback、stop_or_escalate 和相邻域边界；
3. 只建立语义精确的双向 Guide-Wiki evidence；
4. 同步 PocketBase 并运行 Wiki / Guide audit；
5. 不新增 profile，不修改 Retrieval、Prompt、ranking、top_k、selector 或 fallback；
6. 不修改 firmware、hardware、Core、schema 或 FT-02 数据模型。

### Batch15-C: Apply Verification / Field Test Preparation

- 确认 Wiki/Guide 数量、双向 evidence、ID/slug、停止线和跨域 ownership；
- 建立 Field Test fixture 和指标时，先固定 strict 与 observation 边界；
- 默认仍不新增 profile，不以人工排序或 fixture 特判制造 pass；
- 若项目阶段命名将 Batch15-C 直接用于 Field Test，则只测试，不在同批修 Retrieval。

### 后续固定顺序

```text
Knowledge + Evidence
  -> Field Test
  -> Root Cause Review
  -> Minimal Retrieval Apply（仅在证据证明需要时）
  -> Final Verification / Freeze
```

## 14. Apply Readiness

结论：**值得进入 Apply，范围已可冻结。**

理由：

- 缺口跨越 Generation、Storage、Conversion、Load、Measurement、Maintenance、Redundancy 和 Degraded Operation，不能靠现有单篇基础卡自然闭合；
- Records v0.1 已 stable，可保存跨季产出、容量复测和维护历史；
- 18 篇新增低于参考范围下限但有充分依据，因为已有 82 篇相关 Wiki 可复用；
- 7 个 Guide 已通过合并去重，没有把每个 Wiki 都做成 Guide；
- 所有候选均可限定在低压、成品模块、隔离与降级范围；
- 不需要先改 Retrieval，也不需要先猜 query profile。

Apply 阻断条件：若 Batch15-B 无法为 USB/multimeter 测量写出明确低压范围和误接停止线，或备用切换无法明确禁止并联、反送和带电改线，则对应高风险条目应暂缓，而不是降低风险描述后上线。

## 15. Final Recommendation

|项目|最终建议|
|---|---|
|新增 Wiki|18|
|重点复用 Wiki|14（从 82 篇相关复用池中选取）|
|新增 Guide|7（DG-0919 至 DG-0925 候选）|
|新增 query profile|0|
|修改 Retrieval / Prompt / ranking / top_k / selector / fallback|0|
|修改 firmware / hardware / Core / schema|0|
|进入 Apply|是，按本计划冻结范围|

本阶段只完成 Coverage Planning / Gap Analysis，没有创建 Wiki、Guide、profile、fixture，没有同步 PocketBase，也没有修改数据库。

## 16. Dirty Files Boundary

工作树在本阶段开始前已有大量 dirty / untracked 文件，包括但不限于：

- `data/lanternbox.db`
- `data/retrieval_query_profiles.json`
- `data/emergency_guides.json`
- `data/guide_index.json`
- Batch13 / Batch14 的 Wiki、Guide、测试与报告
- FT-02 firmware / FieldManual 相关文件
- `tools/audit_wiki.py`

本阶段不清理、不回退、不提交、不解释这些既有改动，也不运行会同步 PocketBase 或重建 Guide 索引的命令。Batch15-A 的唯一计划变更应为本报告。
