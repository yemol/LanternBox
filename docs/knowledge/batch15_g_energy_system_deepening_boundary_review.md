# LanternBox Batch15-G: Energy System Deepening Stable Boundary Review

生成日期：2026-08-25

输入：

- `docs/knowledge/batch15_e_energy_system_deepening_root_cause_review.md`
- `docs/knowledge/batch15_f_energy_system_deepening_minimal_apply_report.md`
- Batch15-F 冻结 Field Test 与 contract test 结果

本阶段只做边界审查。未执行 Apply，未修改 Wiki、Guide、profile、Retrieval、ranking、selector、stable domains、Safety、Management、Records、Repair 或 fixture。

## Current Status

Batch15-F 已完成其允许范围内的最小修复：DG-0919 长期容量 candidate recall 恢复，O03/O04/X-B 测试 ownership 得到校正，dangerous suggestion 否定语境误判清零。

当前证据支持冻结 Energy Deepening 的知识范围和新增 profile，不支持宣称所有跨域 ownership 已闭合：

- Wiki audit：0 / 0 / 0。
- Guide audit：0 / 0 / 0。
- Batch15-F contract + retrieval contracts：29 passed。
- Field Test：16 pass / 2 partial / 10 fail。
- dangerous suggestion：0；真实危险允许语句仍能被识别。
- E07/E08、E13/E14/E15、E18/E19/E20 均通过。
- O01/O05/O07/O08 仍为跨域 ownership / candidate recall debt。

判断口径：Energy Deepening 的 Knowledge 与 Guide 集合已经足以承担 v0.1 职责；end-to-end Retrieval 仍有已知选择和跨域归属债务，因此状态是 stable candidate，而不是无债务 stable。

## Stable Capabilities

|Guide|能力|当前证据|审查判断|
|---|---|---|---|
|DG-0919|小型离网能源容量盘点|E07、E08 均为 top1 / PASS；长期容量 profile 已恢复 candidate recall|核心能力稳定|
|DG-0920|连续阴天与长期发电不足降级运行|E02 top1，E01 top2；目标 Guide 可稳定进入，但 low-voltage evidence 仍有选择缺口|Guide 能力稳定，保留 retrieval evidence debt|
|DG-0921|电池实际容量下降复查|E04、E05、E06 PASS；非侵入式、重复样本和单次测量边界有效|安全前提内稳定；O01/O07 另有 Safety ownership debt|
|DG-0922|太阳能实际产出与预期偏差检查|E10 有目标 Guide/Wiki，E11/E12 的正确 Guide 在 candidate 但未进 selected top3|Knowledge/Guide 稳定，选择竞争未闭合|
|DG-0923|单点故障、备用路径与关键负载切换|E13、E14、E15 PASS，低压、并联/反送与切换边界有效|核心能力稳定；E16 保留 selected-set debt|
|DG-0924|低压能源系统周期维护|E17 中进入 top2，维护与 Repair 边界有效；目标 Wiki 受 top8 evidence starvation 影响|Guide 边界稳定，evidence selection 未闭合|
|DG-0925|低压现场测量|E18、E19、E20 PASS，电流档并接禁止线有效|核心能力稳定；E09 的转换损耗召回仍是 selection debt|

七个 Guide 均不需要新增或改写正文，也没有证据要求增加第 2 个 Energy Deepening profile。现有 E01/E02/E09/E10/E11/E12/E16/E17 结果应保留为 retrieval selection / evidence debt，不通过扩写 Deepening Knowledge 处理。

## Remaining Failures

|Case|实际 top1|应有主域|判断|
|---|---|---|---|
|O01|DG-0921|Energy Safety|鼓包电池查询被容量复查入口压过；Safety Guide 已在 top2，停止与隔离 evidence 存在|
|O05|DG-0564|Energy Safety / Fire|正在冒烟时旧 Solar Management 入口占 top1；正确 Safety Guide 未进入 candidate|
|O07|DG-0921|Energy Safety|拆包换芯查询被容量复查入口压过；禁止拆包/换芯 evidence 存在，但 Safety 未获主位|
|O08|DG-0900|Records|长期归档与可信度查询被 Long-Term Storage 入口占据；Records Guide 未进入 candidate|

四项全部属于跨域 ownership / candidate recall debt，不是 Energy Deepening Wiki 或 Guide 缺失：

- O01、O07 是 Deepening 对 Battery Safety 的 top1 吞域。
- O05 是 Solar Management 对 Safety / Fire 的吞域，不是 Deepening profile 造成。
- O08 是 Storage 对 Records 的吞域，不是 Deepening profile 造成。

这些债务必须由对应跨域审查处理，不应继续扩展 Energy Deepening 来吸收。

## Ownership Debt

Energy Deepening 当前存在两项直接 ownership debt：O01 与 O07。两者都由 DG-0921 的容量语义压过更高优先级的异常电池或侵入式操作语义。

该问题是主位选择错误，不是危险正文输出：两条 case 的 battery safety、鼓包停用或拆包换芯禁止 evidence 均已命中，dangerous suggestion 仍为 0。因此它们不否定 DG-0921 在安全、成品、非侵入式容量复查场景中的稳定性，但阻止将跨域边界标记为完全闭合。

O05、O08 是相邻 stable domain 之间的召回与主位债务。它们应分别进入 Energy Safety / Solar Management 和 Records / Long-Term Storage 的 cross-domain review。

## Safety Boundary

Batch15-F 的 classifier 修复只收窄误报，没有放松真实危险规则：

- “不与”“避免”“不提供”“禁止”“不要”等否定/禁止语境不再触发 dangerous suggestion。
- 单字“接”不再误命中名词“接头”。
- “允许未知源并联”“允许市电反送”“可以高压接线”等真实危险允许语句仍被 contract tests 检出。
- E13、E14、E15 从 classifier false positive 导致的 fail 恢复为 PASS。
- After Field Test 的 dangerous suggestion 与各项零容忍危险计数均为 0。

结论：false positive 修复没有显示真实安全检测回退。O01/O07 是 Safety ownership debt，不能解释为 classifier regression；O05 仍需 Safety candidate recall 的跨域后续审查。

## Management Boundary

- E03 继续由 DG-0845 Energy Management top1，证明当天设备关闭/分配没有被新 profile 吞掉。
- O03 由 DG-0859 Communication / Energy Management co-primary 合理主导。
- 新 profile 对“每日能源预算”“充电队列”等 Management 表达设置负向保护。
- O05 的 Solar Management 抢位是旧入口与 Safety 的边界债务，不是新 Deepening profile 的扩张结果。

结论：Energy Deepening 没有吞 Energy Management；Management / Safety 之间仍有 O05 债务。

## Records Boundary

- 新 profile 不覆盖长期归档、可信度、Records lifecycle 或趋势分析。
- O08 中实际触发的是 `storage_energy_fuel_storage`，不是 `energy_long_term_capacity_planning`。
- X-B 的测试 ownership 已恢复为 Records / Repair / Risk Decision co-domain，但 Records trend evidence 仍为 supplemental debt。

结论：Energy Deepening 没有吞 Records。O08 是 Records candidate recall 与 Storage 竞争问题，应留给 Records / Storage cross-domain review。

## Repair Boundary

- O04 已由 DG-0563 Repair / Tools co-primary 主导并 PASS。
- 新 profile 对“维修”设置负向保护，USB 线缆检查与修理不会被长期容量入口吸收。
- DG-0924 的周期维护继续限定在断电后、低风险、可视可触达范围，具体修理由 Repair 承担。

结论：Energy Deepening 没有吞 Repair；当前没有需要通过 Energy profile 修复的 Repair blocker。

## Profile Review

Batch15-F 只新增 `energy_long_term_capacity_planning`，审查结果为足够窄：

- 正向范围限定于跨月/长期容量、持续与峰值负载、Generation / Storage / Conversion / Load 保守模型。
- 目标只指向 DG-0919 candidate recall；E07、E08 均恢复为 DG-0919 top1 / PASS。
- 负向保护包含每日能源预算、充电队列、电池异常、低压异常、Records 归档和维修。
- contract tests 证明当天电量分配保持 Management，鼓包电池隔离保持 Safety，火灾、维修、市电和 Records 查询不触发该 profile。
- O01/O07 失败并非该 profile 引入：O01 触发旧 `energy_battery_abnormal_isolation`，O07 无该新 profile。

结论：该 profile 没有证据显示抢占 Safety、Management、Records 或 Repair；无需扩大触发词、增加第二个 Deepening profile或继续 tuning。

## Recommendation

冻结建议：

1. 冻结 DG-0919～DG-0925 的 v0.1 Knowledge / Guide 范围。
2. 冻结 `energy_long_term_capacity_planning`，不继续增加 Energy Deepening profile。
3. 不以提高 Field Test 分数为理由修改 Wiki、Guide、Guide-Wiki、ranking、selector 或 stable domain profiles。
4. 将 O01/O07 登记为 Battery Safety-over-Deepening ownership debt。
5. 将 O05 登记为 Energy Safety / Solar Management candidate ownership debt。
6. 将 O08 登记为 Records / Long-Term Storage candidate ownership debt。
7. 保留 E01/E02/E09～E12/E16/E17 的 retrieval selection / evidence debt，不把它们误判为 Knowledge 缺失。

最终结论：**B. STABLE CANDIDATE BUT CROSS-DOMAIN DEBT REMAINS**。
