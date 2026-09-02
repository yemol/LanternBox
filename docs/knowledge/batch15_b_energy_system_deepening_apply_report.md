# LanternBox Batch15-B: Energy System Deepening Knowledge Apply Report

生成日期：2026-08-25

权威依据：

- docs/engineering/ROOT_CAUSE_FIX_POLICY.md
- docs/knowledge/batch15_a_energy_system_deepening_plan.md

## 0. Apply 结论

Batch15-B 已按冻结范围建立 Energy System Deepening v0.1 的 Knowledge + Guide + Evidence 基础层：

- 新增 18 篇 Energy Deepening Wiki，没有扩到第 19 篇。
- 复用 14 篇既有 Wiki，只更新精确 guide_links，没有复制正文。
- 新增 DG-0919 至 DG-0925 共 7 个 Guide。
- 新 Guide 共建立 47 条精准 Guide-Wiki evidence。
- 全库 Guide-Wiki 前向和反向 pair 均为 2434，单边关系和无效引用均为 0。
- Markdown Wiki 与 PocketBase wiki_articles 均为 1157。
- Guide 源文件、data/emergency_guides.json、data/guide_index.json 均为 851。
- Wiki audit 与 Guide audit 均为 errors=0 warnings=0 advisories=0。
- 未新增或修改 query profile；Energy Deepening profile 新增数为 0。
- Retrieval Pipeline、Prompt、ranking、top_k、selector、fallback、schema、FT-02、Core 和 data/lanternbox.db 均未修改。

本阶段没有进行 Retrieval Field Test 或 tuning。

## 1. 新增 18 篇 Wiki

全部位于 wiki_import/power/，category 复用既有“能源”。

|序号|slug|title|Priority|Risk|
|---:|---|---|---|---|
|1|energy-offgrid-capacity-inventory-001|小型离网能源容量盘点表|P1|caution|
|2|energy-load-continuous-peak-profile-001|持续负载与峰值负载档案|P1|caution|
|3|energy-solar-seasonal-output-baseline-001|太阳能跨季产出基线|P1|caution|
|4|energy-solar-multi-day-cloudy-degraded-mode-001|连续阴天多日降级运行|P0|caution|
|5|energy-blackout-degraded-mode-card-001|离网能源 blackout / degraded 模式卡|P0|caution|
|6|energy-battery-capacity-retest-cycle-001|电池实际容量周期复测|P1|high|
|7|energy-battery-health-grade-replacement-line-001|电池健康分级与替换线|P0|high|
|8|energy-spare-battery-readiness-check-001|备用电池就绪检查|P1|high|
|9|energy-solar-shade-soiling-maintenance-001|太阳能板遮挡、污染与低风险维护|P1|caution|
|10|energy-solar-expected-actual-output-check-001|太阳能预期与实际产出偏差检查|P1|caution|
|11|energy-low-voltage-power-path-map-001|5V / USB / 低压 DC 供电路径图|P1|caution|
|12|energy-low-voltage-branch-protection-label-001|低压支路、保险与极性标签|P0|high|
|13|energy-single-point-failure-register-001|能源系统单点故障清单|P1|caution|
|14|energy-backup-power-path-switch-check-001|备用供电路径切换检查|P0|high|
|15|energy-low-voltage-maintenance-calendar-001|低压能源系统周期维护表|P1|caution|
|16|energy-generation-component-failure-degraded-path-001|发电部件故障后的隔离与降级路径|P0|high|
|17|energy-usb-dc-conversion-loss-001|USB 与低压 DC 转换损耗检查|P1|caution|
|18|energy-low-voltage-field-measurement-runtime-verify-001|低压电压、电流、功率、Wh/Ah 与运行时长验证|P0|high|

风险分布：high 7 篇，caution 11 篇。

所有条目都限定 LOW VOLTAGE、MODULAR、ISOLATE、REPLACE、DEGRADE、CONSERVATIVE ESTIMATE，并包含停止条件、替代方案、记录字段和相邻领域边界。

## 2. 复用 14 篇 Wiki

以下文件没有复制或重写业务正文，只新增本批 Guide 的 frontmatter evidence：

|复用 Wiki|本批新增 Guide 关系|
|---|---|
|energy-battery-capacity-001|DG-0919, DG-0921, DG-0925|
|energy-battery-capacity-002|DG-0919|
|energy-battery-aging-capacity-note-001|DG-0921|
|energy-backup-battery-rotation-ledger-001|DG-0921, DG-0923|
|energy-battery-003|DG-0921|
|energy-cloudy-solar-downgrade-001|DG-0920|
|energy-solar-daytime-charge-schedule-001|DG-0920, DG-0922|
|energy-essential-load-reserve-001|DG-0920|
|energy-low-battery-load-disconnect-order-001|DG-0920|
|energy-wh-mah-local-estimate-001|DG-0919, DG-0925|
|energy-device-consumption-baseline-001|DG-0919, DG-0925|
|energy-solar-weather-pattern-log-001|DG-0922|
|energy-charge-discharge-handover-log-001|DG-0921|
|records-energy-capacity-trend-001|DG-0919, DG-0921, DG-0922|

energy-wh-mah-local-estimate-001 已从无 Guide evidence 变为与容量盘点、现场测量精确关联。其他 9 个旧无 evidence Energy Guide 未处理。

## 3. 新增 7 个 Guide

|Guide|标题|Priority|Risk|主要职责|
|---|---|---|---|---|
|DG-0919|小型离网能源容量盘点|P1|caution|Generation / Storage / Conversion / Load 保守容量模型|
|DG-0920|连续阴天与长期发电不足降级运行|P0|caution|多日低产出、blackout、degraded、minimum 与恢复条件|
|DG-0921|电池实际容量下降复查|P0|high|非侵入式 runtime / Wh 趋势、健康等级、替换线|
|DG-0922|太阳能实际产出与预期偏差检查|P1|caution|同类天气/时段下检查遮挡、污染、摆放、接头和控制器|
|DG-0923|单点故障、备用路径与关键负载切换|P0|high|低压单点识别与断开后单路备用切换|
|DG-0924|低压能源系统周期维护|P1|caution|断电后的低风险状态检查和维护日历|
|DG-0925|低压电压、电流、功率与运行时长现场测量|P0|high|5V / USB / 已知低压 DC 的 V/A/W/Wh/Ah/runtime 测量|

每个 Guide 均包含 scenario、goal、steps、check、fallback、stop_or_escalate、notes 和 related_wiki。没有添加 Retrieval profile、ranking metadata 补丁或 query 特判。

## 4. Guide-Wiki Evidence

|Guide|related_wiki 数|
|---|---:|
|DG-0919|9|
|DG-0920|7|
|DG-0921|9|
|DG-0922|6|
|DG-0923|7|
|DG-0924|4|
|DG-0925|5|
|合计|47|

其中新 18 Wiki 提供 25 条 pair，14 篇复用 Wiki 提供 22 条 pair。没有为了增加 edge 数量关联无关 Safety、Repair、Storage 或 Records 内容。

## 5. 双向关系审计

|项目|结果|
|---|---:|
|Wiki guide_links 前向 pair|2434|
|Guide related_wiki 反向 pair|2434|
|Batch15-B 新 pair|47|
|single_forward|0|
|single_reverse|0|
|invalid Guide ID|0|
|invalid Wiki slug|0|

scripts/audit_guides.py 同时确认所有 related_wiki 均为真实 slug，关系完全对称。

## 6. PocketBase 同步

- 同步对象：18 个新增 energy-* slug。
- 使用既有“能源” category，category id 为 ra6ebcc87e673f4。
- 在单一 SQLite 事务中只执行 18 条新增，不删除、不重建、不修改 schema。
- 同步字段为 title、slug、category、summary、content、tags、risk_level、status 和 source。
- 14 篇复用 Wiki 没有创建副本；其正文和 PocketBase article 字段没有改写。

|项目|同步前|同步后|
|---|---:|---:|
|Markdown Wiki|1139|1157|
|PocketBase wiki_articles|1139|1157|
|本批目标 slug|0|18|

tools/audit_wiki.py 已逐 slug 比较 Markdown 与 PocketBase 字段和正文，结果一致。

## 7. Guide Build

运行 python3 tools/build_guides.py：

- Guide 源 JSON：851
- data/emergency_guides.json：851
- data/guide_index.json：851
- 增量：844 -> 851

两份构建输出数量一致。

## 8. Audit 结果

运行：

- python3 tools/audit_wiki.py
- python3 tools/build_guides.py
- python3 scripts/audit_guides.py

|Audit|对象数|errors|warnings|advisories|
|---|---:|---:|---:|---:|
|Wiki audit|1157 Markdown / 1157 PocketBase|0|0|0|
|Guide audit|851|0|0|0|

审计报告：

- docs/knowledge/wiki_audit_2026-08-25.md
- docs/knowledge/guide_audit_2026-08-25.md

没有降低审计严格度。

## 9. Safety Stop-Line Review

### 9.1 电池

- 仅允许成品低压模块的非侵入式 runtime / Wh 验证。
- 鼓包、漏液、异常热、异味、腐蚀、摔裂、冒烟时立即 STOP / isolate。
- 禁止拆包、换芯、电芯重组、点焊、BMS 改造和修理损坏锂电池。
- 异常对象没有“降级继续使用”选项。

### 9.2 备用切换

DG-0923 明确采用：

    断开
    -> 检查
    -> 单一路径接入
    -> 验证
    -> 再接负载

只允许已知低压、电压匹配、极性确认、成品模块、可整体断开的路径。带电改线、未知电源并联、多源直接并联、市电反送、绕过保险、未知极性和高压 DC 均为停止线。

### 9.3 现场测量

- 只允许 5V、USB 和已知低压 DC。
- USB Power Meter 可记录 V/A/W/Wh/Ah。
- multimeter 电压测量与电流测量接法明确区分。
- 电流档不得直接并接电源两端。
- 未知电压、破损表笔、短接电池、市电、110V/220V、高压 DC、潮湿和裸露高风险导体均立即停止。

### 9.4 太阳能与维护

- 单次低读数不直接判定面板故障。
- 破裂面板、裸露导体、控制器冒烟、火花、积水、高压串联和屋顶高处作业全部停止。
- 周期维护只做安全可达、断电后的低风险状态检查；拆壳和具体维修交 Repair。

## 10. Ownership Boundaries

### Energy Safety

当前鼓包、漏液、过热、冒烟、火花、短路、潮湿和危险隔离继续由 Energy Safety 主导。Deepening 只处理安全部件的长期容量、冗余和降级状态。

### Energy Management

今天哪些设备先开、每日预算、充电队列和短期负载排序继续由 DG-0844、DG-0845、DG-0846 等入口主导。Deepening 只处理连续多日、跨周/月/季的产能和系统韧性。

### Records

Records 保存 raw V/A/W/Wh/Ah、runtime、instrument、device、time、load mode、conditions、来源、重复样本和趋势。Energy 只基于可追溯、可比较样本解释保守容量、健康等级、降级模式和冗余。

### Repair

Repair 负责线缆、接头、控制器、面板和仪表的具体检查、修理与替换方法。Energy 只识别系统影响、隔离范围、备用要求和恢复证据。

### Storage

Storage 继续负责电池和备件的长期存放条件、位置与取用。Energy 只判断备用源是否就绪及其对系统冗余的贡献。

## 11. High-Voltage / Mains Professional Boundary

本批不提供 110V/220V 配线或测量、配电箱施工、并网、市电反送、高压太阳能串、大型逆变器工程、工业 UPS、微电网、大型柴油发电设计、电池拆包、电芯重组、点焊、BMS 改造或自制危险充电器。

遇到这些主题统一执行 STOP + isolate + professional boundary，不把它们包装成“小型离网能源”继续操作。

## 12. Retrieval Freeze Confirmation

本阶段没有修改：

- data/retrieval_query_profiles.json
- Retrieval Pipeline / Prompt
- ranking / top_k
- selector / selector limit
- fallback / evidence cap
- schema / FastAPI / 应用代码

当前 profile 文件共有 79 个 profile，没有 energy_deepening、offgrid_capacity 或 long_term_energy profile。

query profile 新增数 = 0。

新 Guide 当前是否容易检索不属于本阶段判断；下一步先做 Apply Verification / Field Test Preparation。

## 13. 未处理旧无 Evidence Guide

以下 9 个旧 Guide 保持原状：

DG-0107、DG-0108、DG-0110、DG-0113、DG-0114、DG-0330、DG-0331、DG-0337、DG-0343。

“无 evidence”不等于 Batch15-B 缺口，本批没有顺手清理或补链。

## 14. FT-02 / Core Confirmation

本批只在知识内容中声明三层数据边界：

- Field Measurement：raw V/A/W/Wh/Ah、runtime、instrument、device、time、load mode、conditions。
- Records：source、repeated sample、history、trend。
- Energy：conservative usable capacity、health interpretation、degraded mode、redundancy planning。

未修改 FT-02 battery display、Fuel Gauge 接入、FT-02 firmware / hardware、Core、schema 或同步协议。

当前固定 UI 百分比没有被写成可信 Fuel Gauge 数据；单次读数不能自动成为健康结论。

## 15. Dirty Files

### 本批有意新增或修改

- 18 篇 wiki_import/power/energy-*.md
- 14 篇复用 Wiki 的 guide_links
- data/guides/power/DG-0919.json 至 DG-0925.json
- data/emergency_guides.json
- data/guide_index.json
- pocketbase/pb_data/data.db 的本地 Wiki 数据及其 SQLite journal/WAL 状态
- docs/knowledge/wiki_audit_2026-08-25.md
- docs/knowledge/guide_audit_2026-08-25.md
- 本报告

### 既有 dirty，未处理

- data/lanternbox.db：本批前后 SHA-256 均为 aeca9af6579dbe97656b0968c6b6a9b20152bee5b90e2b9cb2c860bff3202e49，没有写入、清理或提交。
- data/retrieval_query_profiles.json：本批开始前已 dirty，本批未修改。
- Batch13 / Batch14 未跟踪 Wiki、Guide、测试和报告。
- FT-02 firmware / FieldManual 的既有 dirty 文件。
- 其他与 Batch15-B 无关的工作树改动。

data/emergency_guides.json 和 data/guide_index.json 在本批开始前已有改动；本阶段按允许范围使用现有全部 Guide 源重新构建到 851，没有回退其中既有内容。

## 16. Batch15-C 推荐

下一步只进入 Batch15-C Apply Verification / Field Test Preparation：

1. 再确认 18 Wiki、7 Guide 和 47 条 evidence。
2. 固定 Safety、Energy Management、Records、Repair 与专业边界 ownership。
3. 设计 strict / observation fixture，保护当前鼓包、电气火灾、今天分配、USB 线维修、控制器冒烟和住宅配电场景。
4. 然后再进入正式 Retrieval Field Test。

不要在 Batch15-C 提前新增 profile、调整 ranking、扩大 top_k / selector 或修改 fallback。

## 17. Final Status

Energy System Deepening v0.1 的 Knowledge + Guide + Evidence 基础层已完成：

    18 new Wiki
    + 14 reused Wiki
    + 7 new Guide
    + 47 precise bidirectional evidence pairs
    + PocketBase 1157 / 1157
    + Guide build 851 / 851
    + Wiki audit 0 / 0 / 0
    + Guide audit 0 / 0 / 0
    + query profile 0

本阶段到此停止，不继续 Retrieval tuning。
