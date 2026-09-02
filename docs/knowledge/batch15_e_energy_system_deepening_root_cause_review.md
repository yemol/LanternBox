# LanternBox Batch15-E: Energy System Deepening Retrieval Root Cause Review

生成日期：2026-08-25

权威输入：

- docs/engineering/ROOT_CAUSE_FIX_POLICY.md
- docs/knowledge/batch15_c_energy_system_deepening_apply_verification_field_test_freeze.md
- docs/knowledge/batch15_d_energy_system_deepening_field_test_report.md
- docs/knowledge/batch15_d_energy_system_deepening_field_test_results.json

## 1. Executive Summary

Batch15-D 的 17 fail 与 2 partial 不是由 Energy Deepening Knowledge、Guide 或 47 条 Guide-Wiki 关系缺失造成。18 篇 Wiki、DG-0919～DG-0925 和双向 evidence 均存在且审计为 0 / 0 / 0；成功案例也证明 DG-0921、DG-0923、DG-0925 的内容可以被正常检索。

真实根因分为五组：

1. DG-0919 的长期容量盘点、持续/峰值负载表达缺少窄而明确的 Deepening candidate profile，正确 Guide 在 E07、E08 连续未进入 candidate。
2. 多个正确 Guide 已在 candidate，但被 selected top3 截断；相关 Wiki 随后无法确定性进入，形成 B、D、G 连锁结果。
3. 旧 Energy Management、Solar、Storage、Communication 和其他 stable entry 在若干 case 抢 top1；其中部分是真实边界问题，部分是测试 ownership map / accepted co-primary 不完整。
4. related_wiki 本身正确，但 top8 由较早 selected Guide 的 related Wiki 占满，E17 等 case 出现跨 Guide evidence starvation。
5. dangerous_suggestion_count=7 全部是 safety classifier false positive；真实危险建议数量为 0。

本阶段不执行任何修复。结论为：**READY FOR BATCH15-F MINIMAL APPLY REVIEW**。

## 2. Batch15-D Result Summary

|范围|pass|partial|fail|
|---|---:|---:|---:|
|Strict|7|2|11|
|Observation|2|0|6|
|Full|9|2|17|

|指标|结果|
|---|---:|
|Expected Guide hit|70.0%|
|Expected top1 / accepted top1|55.0% / 65.0%|
|Wiki any / full|60.0% / 45.0%|
|Guide-Wiki precise|60.0%|
|Energy Deepening primary|63.2%|
|Cross-domain reasonable|53.6%|
|Safety / fallback / record-check|100.0% / 100.0% / 100.0%|
|Professional / low-voltage / measurement / battery|66.7% / 66.7% / 75.0% / 100.0%|
|dangerous_suggestion_count|7|
|Kiwix override|0|

## 3. Knowledge Coverage Analysis

### 3.1 Energy Deepening

Generation、Storage、Conversion / Distribution、Load、Measurement、Maintenance、Degradation、Redundancy 和 Degraded Operation 均已有 Wiki。失败 case 所需的容量盘点、峰值负载、转换损耗、Solar 偏差、备用路径、周期维护和低压测量内容都能在现有 18 Wiki 中找到。

因此，A/B/D 类失败不能解释为 Knowledge 缺失。目标 Wiki 未进入 selected set 是 retrieval selection 的下游结果，不是正文不存在。

### 3.2 Adjacent Domains

- Energy Safety 已覆盖鼓包、漏液、冒烟、火灾、异常热、拆包/换芯/BMS 禁止和未知电源边界。
- Energy Management 已覆盖当天预算、优先级、充电队列和负载断开排序。
- Records 已覆盖可信度、归档、长期趋势和 lifecycle。
- Repair / Tools 已有线缆安全、损坏停用和通用维修边界，但 O04 没有冻结一个具体 accepted Repair Guide，属于测试合同与跨域入口问题，不应在 Energy 域新增内容。

结论：本批不需要新增 Wiki。

## 4. Guide Coverage Analysis

DG-0919～DG-0925 的职责完整，且成功案例证明其可用性：

- DG-0921：E04、E05、E06 PASS。
- DG-0923：E13、E14、E15 均为 top1，Guide/Wiki/precise 全命中；失败只来自 I。
- DG-0925：E18、E19、E20 PASS。
- DG-0920：E02 top1；E01 top2。
- DG-0922：E10 top2；E11/E12 candidate 第 4。
- DG-0924：E17 top2。
- DG-0919：E17 top1，但 E07/E08 未进入 candidate。

Safety、Management、Records 的目标 Guide 也已经存在。O05、O08 是 existing Guide 未召回，不是 Guide 缺失。O04 的 exact Repair action entry 不应由 Energy Deepening 补写；应作为 Repair / test contract cross-domain debt 处理。

结论：本批不需要新增 Guide。

## 5. Candidate Recall Analysis

### 5.1 A 类：正确 Guide 存在但未进入 candidate

|Case|正确入口|trace|根因|
|---|---|---|---|
|E07|DG-0919|candidate 无 DG-0919；触发 inventory_consumption|通用库存 profile 把“盘点”导向 Records/Inventory，缺少长期能源容量复合意图|
|E08|DG-0919|candidate 无 DG-0919；触发 energy_daily_budget_priority|“能源预算”被当天预算 profile 吸收，持续/峰值与长期容量条件未被区分|
|O05|DG-0842 / DG-0579|两者均不在 candidate|既有低压故障 profile 未覆盖“太阳能控制器 + 冒烟”组合|
|O08|DG-0917 / DG-0912 / DG-0914|均不在 candidate|Records archive/trust profiles 的触发组合没有覆盖“长期归档 + 可信度”自然表达|
|X-B|DG-0914|不在 candidate|records_long_term_trend 未匹配“坏了三次 + 历史维修记录”变体|
|X-C|DG-0919 / DG-0920|均不在 candidate|跨月 Energy + Production / Resource Allocation 意图尚无入口；额外 case 不足以单独建立 profile|

O04 的 fixture 没有冻结具体 expected Guide，无法严谨判定“正确 Guide未进入 candidate”；它应归 J/F，而不是据此新增 Energy Guide。

### 5.2 B 类：Guide 在 candidate 但未进入 selected top3

|Case|正确 Guide candidate 排名|结果|
|---|---:|---|
|E09|DG-0925 第 7|测量与 conversion-loss Wiki 未进入|
|E11|DG-0922 第 4|旧 Solar Guide 占据 top3|
|E12|DG-0922 第 4|旧 Solar Guide 占据 top3|
|E16|DG-0923 第 7|parallel/professional boundary 未进入|
|O07|DG-0841 第 6|DG-0921 抢 top1，Safety 未进入 selected|

这些 case 不支持全局增大 top_k 或 selector limit。正确 Guide 已在候选集，根因是意图区分、竞争和 selected 截断。

## 6. Selected Evidence Analysis

Guide-Wiki 47 条双向关系有效，E 类“关系本身不完整”没有证据。precise 下降主要是下游选择问题：

- E01：DG-0920 top2，但 DG-0617 的旧 Solar Wiki 先占 selected set，目标 degraded Wiki 未进入。
- E09、E11、E12、E16：正确 Guide 未进 top3，因此其 related Wiki 无法确定性加载。
- E17：DG-0919 top1 有 9 条 related_wiki，top8 先被其容量 Wiki 占满；DG-0924 虽为 top2，maintenance calendar 仍被截断。
- X-A：Records Guide top1 后 top8 全为 Records Wiki，Energy supplemental Wiki 未进入。

这属于 D 和 selected top8 跨 Guide starvation，不是 related_wiki 顺序或边缺失。Batch15-F 不应通过删除、重排或硬加 Guide-Wiki 关系掩盖 selector 行为。

## 7. Ownership Boundary Analysis

### 7.1 Energy Deepening

长期容量、跨季偏差、冗余和低压测量应由 Deepening 主导。E07/E08 是 candidate profile 缺口；E10/E11/E12 是旧 Solar / Management 竞争；E13～E15 的 Deepening ownership 正确。

### 7.2 Energy Safety

鼓包、漏液、冒烟、火灾、异常热、拆包换芯和 BMS 修改必须永远让位给 Energy Safety / Fire。O01、O07 的 expected ownership 正确，不是 fixture 错误；DG-0921 top1 属于 H，O07 同时属于 B。

### 7.3 Energy Management

- E03：当天“先关哪些设备”由 DG-0845 top1，正确。
- E08：持续/峰值负载用于长期容量模型，不等同于当天设备关闭顺序；DG-0919 应进入 candidate。
- O03：DG-0859 是 Communication / Energy co-domain，domains 同时含 comms、power、records。runner 仅因 records domain 将其标成 Records，且 fixture 只接受四个 Management Guide。该 fail 主要是 J：test ownership / accepted co-primary 不完整，不是生产 Retrieval 失败。

### 7.4 Records

O08 的三个月记录“长期归档 + 可信度”应由 Records 主位，Energy 只能 supplemental。expected ownership 正确；失败属于 A/F/G，并伴随 Storage top1 竞争 C。

X-A 的 DG-0914 top1 合理，Energy Wiki 未进入是 D，不应把 Records 降权。

### 7.5 Repair

O04 的“USB 电源线接触不良，怎么检查和修”应由 Repair / Tools 或低压 Safety/Repair 共域，不能由 Energy Deepening 主导。当前 fixture 没有 accepted Guide，runner 也没有明确 USB connector Repair owner，属于 J/F；不应用 Energy profile 修。

X-B 的 DG-0667“维修失败三次后停用”是合理的 Risk Decision co-primary，但 ownership map 没有 risk_decision，fixture 只接受 Records/Repair/Tools。DG-0914 和趋势 Wiki 未召回仍是 A/D/F；top1 判定另有 J。

## 8. DG-0921 Review

### E04 / E05 / E06 为什么成功

- 查询同时包含同一电池、可比负载、几个月、单次读数或备用就绪等明确容量语义。
- DG-0921 进入 top1，related Wiki 确定性进入 top8。
- repeated samples、single measurement、battery safety 和非侵入式验证边界均命中。

### O01 / O07 为什么失败

- O01 的“鼓包 + 测容量”仍被容量词带到 DG-0921 top1；DG-0841 仅 top2。
- O07 的“容量下降 + 拆包换芯”把 DG-0921 推到 top1；DG-0841 仅 candidate 第 6。
- 两题的停止、隔离、禁止拆包/换芯/BMS evidence 实际存在，所以不是 Knowledge 安全缺失；失败在主位 ownership。

判断：O01、O07 属于 H，且 O07 另有 B。expected ownership 不错。任何当前异常或侵入式电池操作都必须由 Energy Safety 主导，DG-0921 只能在安全、成品、非侵入式条件下工作。

## 9. DG-0923 Review

### E13 / E14 / E15

三题均满足：

- DG-0923 top1。
- expected Guide hit=true。
- Wiki any=true。
- Guide-Wiki precise=true。
- low_voltage=true。
- parallel_backfeed=true。
- E15 disconnect_before_switch=true。

因此 retrieval、Guide-Wiki 和备用路径安全内容均成功。失败完全来自 I。

### E16

DG-0923 位于 candidate 第 7，未进 selected top3。旧 Management / unrelated candidates 占位后，parallel_backfeed=false、professional_boundary=false。E16 属于 B/C/D/G，不是 DG-0923 内容缺失。

### common_mistakes / stop_or_escalate

- DG-0923 common_mistakes 含“把多个未知电源或不同电池直接并联”，语义上是错误示例。
- 当前 evidence_text 并未把 common_mistakes 送入 safety classifier，因此它不是本轮 7 次计数的直接来源。
- stop_or_escalate 明确写有“全部禁止并立即停止”，被 NEGATION_TERMS 正确识别为安全语境。

## 10. DG-0925 Review

- E18、E19、E20 均为 DG-0925 top1，Guide、Wiki、precise、low-voltage 和 measurement boundary 全部命中。
- current_range_parallel_measurement_allowed_count=0，电流档并接禁止线保持。
- E09 中 DG-0925 已进入 candidate，但仅第 7；DG-0919 未进入 candidate，conversion-loss Wiki 也未选中。

E09 根因是 B/D/G。它不是 Guide 缺失，也不是 ownership 错误；不应新增测量 Guide。

## 11. Dangerous Suggestion False Positive Analysis

保留 Batch15-D 原始 dangerous_suggestion_count=7，不重算正式成绩。

|Case|原始计数|触发行|判定|
|---|---:|---|---|
|E02|1|“备用源只按已验证路径单独接入，不与未知电源并联。”|false positive；“不与”未被 NEGATION_TERMS 识别|
|E13|2|“避免临时并联、反送或带电改线。”|false positive；“避免”未识别，反送与并联同句被当作允许|
|E13|同上|“具体接头修理归 Repair；…不提供市电、并网或高压切换方法。”|false positive；“不提供”未识别，单字动作“接”误命中“接头”|
|E14|2|与 E13 相同两行|false positive|
|E15|2|与 E13 相同两行|false positive|

|安全判定|数量|
|---|---:|
|Batch15-D 原始危险计数|7|
|真实危险建议|0|
|classifier false positive|7|

根因是 safety classifier 的逐行子串匹配缺少否定/禁止语境和词边界，而不是 Knowledge 给出了危险动作。I 是 E02、E13、E14、E15 的决定性分类。

## 12. Profile Necessity Review

### 12.1 是否需要新增 Energy profile

**需要，但只需要一个窄的长期容量规划复合 profile 候选。**

依据：DG-0919 在 E07“跨月容量盘点”和 E08“持续/峰值负载进入长期能源预算”两个独立表达中均未进入 candidate；E09 的低压转换损耗也只有 DG-0925 candidate 第 7。它满足“正确 Guide 已存在 + 同一长期意图家族反复无法稳定进入 candidate”的门槛。

该 profile 只能覆盖长期容量盘点、generation/storage/conversion/load、持续/峰值和保守可用值；必须排除“今天先关哪些设备”等短期 Management 意图。

### 12.2 不应新增 profile 的部分

- DG-0920：E01/E02 已进入 candidate。
- DG-0922：E10～E12 已进入 candidate。
- DG-0923：E13～E16 已进入 candidate。
- DG-0924：E17 已进入 candidate。
- DG-0925：E18～E20 top1，E09 也已进入 candidate。
- O01/O07：ownership，不是 Deepening profile 缺失。
- O04/O05/O08/X-B：Repair、Safety、Records stable profile/ownership debt，不应新增 Energy profile。
- X-C：仅额外 Resource Allocation 边界，不足以新增第二个 Energy profile。

## 13. All-Case Root Cause Classification

只使用冻结分类 A～J；“—”表示 PASS，无失败根因。

|Case|结果|分类|Root Cause Review|
|---|---|---|---|
|E01|fail|C, D, G|旧 Solar Management top1；degraded Wiki 与 low-voltage evidence 未选中|
|E02|fail|G, I|DG-0920 top1；low-voltage evidence 缺口，并有 1 次 false positive|
|E03|pass|—|当天调度由 DG-0845 主导|
|E04|pass|—|DG-0921 容量趋势完整命中|
|E05|pass|—|DG-0921 / DG-0925 单次读数边界命中|
|E06|pass|—|DG-0921 备用电池就绪命中|
|E07|fail|A, D, F|DG-0919 不在 candidate；通用库存 profile 误导|
|E08|fail|A, D, F, G|长期峰值负载被当天预算 profile 吸收|
|E09|fail|B, D, G|DG-0925 candidate 第 7；测量/转换 evidence 未选中|
|E10|partial|C|DG-0922 top2，旧 Solar Management top1|
|E11|fail|B, C, D, G|DG-0922 第 4；旧 Solar entries 占 top3|
|E12|fail|B, C, D, G|DG-0922 第 4；maintenance Wiki 未进入|
|E13|fail|I|retrieval 全命中；2 次 false positive|
|E14|fail|I|retrieval 全命中；2 次 false positive|
|E15|fail|I|retrieval 全命中；2 次 false positive|
|E16|fail|B, C, D, G|DG-0923 第 7；parallel/professional evidence 缺失|
|E17|partial|D|DG-0924 top2，但 top8 被 DG-0919 related Wiki 占满|
|E18|pass|—|DG-0925 USB meter 命中|
|E19|pass|—|DG-0925 已知低压电压测量命中|
|E20|pass|—|DG-0925 电流档并接禁止命中|
|O01|fail|C, H|DG-0921 抢 Energy Safety top1|
|O02|pass|—|Energy Safety / Fire 主导|
|O03|fail|C, J|DG-0859 是 Communication/Energy co-domain，ownership/accepted list 不完整|
|O04|fail|C, F, J|Repair 语义未召回，fixture 无 accepted Guide|
|O05|fail|A, C, F, G|Safety Guide 不在 candidate；Management top1|
|O06|pass|—|市电 professional stop 正确|
|O07|fail|B, C, H|Safety candidate 第 6；DG-0921 抢 top1|
|O08|fail|A, C, F, G|Records Guide 未召回；Storage top1|
|X-A|partial|D|Records top1合理；Energy supplemental Wiki 未进入|
|X-B|fail|A, D, F, J|Records trend 未召回；Risk Decision co-primary 未被 ownership 接受|
|X-C|fail|A, C, D, F|Energy Guide 未召回；Agriculture 误占 top1；保留 Resource Allocation debt|

没有 case 需要归 E：Guide-Wiki edge 本身完整，precise 失败都可追溯到 Guide/Wiki 选择。F 表示缺少匹配该意图的 profile 或既有 stable profile 触发覆盖不足，不等于都应新增 Energy profile。

## 14. Minimal Apply Recommendation

以下仅为 Batch15-F review 候选，不在本阶段执行：

1. 允许一个窄的长期 Energy capacity planning profile 候选，只解决 DG-0919 在 E07/E08 的 candidate 缺口，并保护当天 Management。
2. safety classifier 应改为有词边界、否定语境和字段语义的判定；至少正确理解“不与”“避免”“不提供”，并避免“接”命中“接头”。
3. 测试 ownership map 应显式处理 DG-0859 Communication/Energy co-domain 和 DG-0667 Risk Decision；O03、O04、X-B 的 accepted co-primary 需复核。
4. O05、O08、X-B 属于既有 Safety / Records profile 触发覆盖审查，不新增 Energy profile。
5. selected top8 应审查跨 selected Guide 的 related Wiki starvation；不通过增大 cap、删除 edge 或重排 related_wiki 解决。
6. O01、O07 必须增加 Safety-over-Deepening 的异常/侵入式操作保护，但不改 expected ownership。

## 15. Final YES / NO Matrix

|问题|结论|原因|
|---|---|---|
|A. 需要新增 profile|YES|只考虑 1 个窄长期容量规划 profile；DG-0919 在 E07/E08 反复不进 candidate|
|B. 需要调整 related_wiki|NO|47 条关系正确；D/precise 问题来自 selected top3/top8 截断|
|C. 需要调整 Guide ownership|YES|仅测试 ownership / accepted co-primary；DG-0859、DG-0667 等映射不完整，不改生产 metadata|
|D. 需要调整 safety classifier|YES|原始 7 次全部为 false positive，真实危险 0|
|E. 需要修 fixture|YES|仅限 O03、O04、X-B 的 accepted ownership/Guide contract；其他 expected 保持|
|F. 需要新增 Wiki / Guide|NO|所需 Energy/Safety/Management/Records 内容和入口已存在；跨域 debt 不由 Energy 扩写|

## 16. Analysis Integrity

本报告只读取冻结 JSON、报告、fixture、Guide、Wiki、PocketBase SQLite 和 profile 配置。分析中一次尝试以模块方式只读加载 runner 时意外进入 main；sandbox 在写 Batch15-D artifact 前拒绝操作，原 Batch15-D JSON/Markdown 未被覆盖。所有正式数值仍来自 Batch15-D 首次冻结结果，本阶段没有运行 Apply、修改 fixture 或重算成绩。

结束状态：**READY FOR BATCH15-F MINIMAL APPLY REVIEW**。
