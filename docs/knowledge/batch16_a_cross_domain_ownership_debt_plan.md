# LanternBox Batch16-A: Cross-Domain Ownership Debt Planning

生成日期：2026-08-25

权威输入：

- `docs/knowledge/batch15_g_energy_system_deepening_boundary_review.md`
- Batch15-F 冻结 Field Test trace
- 现有 Guide 与 query profile 配置（只读）

本阶段只建立跨域债务路线图。未修改 Wiki、Guide、Retrieval、profile、ranking、selector、tests、fixture、PocketBase 或 database。

## 1. Current Stable Domains

本计划涉及的现有稳定或冻结边界如下：

|领域|当前入口|本计划中的状态|
|---|---|---|
|Energy Deepening|DG-0919～DG-0925；`energy_long_term_capacity_planning`|v0.1 stable candidate，Knowledge / Guide / profile 范围冻结|
|Energy Safety|DG-0616、DG-0841、DG-0842；既有 battery / low-voltage safety profiles|安全内容已存在，召回与主位仍有跨域债务|
|Fire Safety|DG-0579|当前冒烟/火灾的 critical owner|
|Energy Management|DG-0564、DG-0617 等|当天调度与 Solar 排程保持稳定，但不得压过当前危险|
|Long-Term Records|DG-0912、DG-0914、DG-0917；Records v0.1 profiles|v0.1 stable / frozen；O08 暴露自然表达召回缺口|
|Long-Term Storage|DG-0900 与 `storage_energy_fuel_storage`|物理能源物资储藏 owner，不负责记录归档与可信度|
|Repair / Tools|DG-0563 等|O04 已恢复正确主位，不在本轮债务范围|

当前问题不是 Wiki 或 Guide 缺失。三类债务都发生在 candidate recall、top1 competition 或跨域语义主位。

## 2. Energy Deepening Frozen Boundary

Energy Deepening 冻结范围保持 Batch15-G 结论：

- DG-0919：长期容量盘点。
- DG-0920：长期发电不足与降级运行。
- DG-0921：安全、成品、非侵入式电池容量复查。
- DG-0922：太阳能实际产出偏差检查。
- DG-0923：单点故障与已验证低压备用路径。
- DG-0924：断电后低风险周期维护。
- DG-0925：已知低压范围的现场测量。

冻结约束：

- 不扩大 `energy_long_term_capacity_planning`。
- 不让 DG-0921 承担鼓包电池处置、拆包、换芯、点焊或 BMS 操作。
- 不让 DG-0922 承担正在冒烟、起火或异常热设备的处置。
- 不让 Energy Deepening 承担 Records 归档、Storage 物理保存或 Repair 具体修理。

Batch16 后续若进入 Apply，应只调整相邻域入口与跨域主位，不修改上述 Deepening 职责。

## 3. Battery Safety Debt

范围：O01、O07。

### O01: 鼓包后先测容量

|检查项|结果|
|---|---|
|当前 Safety Guide|DG-0841 candidate 第 2；DG-0616 candidate 第 5|
|当前 Deepening Guide|DG-0921 top1|
|profile|`energy_battery_abnormal_isolation` 已触发|
|candidate recall|Safety recall 成功|
|top1 competition|失败；“测容量”把 DG-0921 推到 Safety 之前|
|安全 evidence|鼓包停用、隔离、battery safety 均已命中|

根因：不是 Safety profile 缺失，而是异常状态与容量诊断同时出现时缺少 Safety-over-Deepening 主位契约。

规划建议：O01 不新增 profile。未来应验证“明确鼓包/漏液/冒烟/异常热 + 继续测试或容量复查”时，现有 Safety candidate 是否必须排在 DG-0921 之前。

### O07: 容量下降后拆包换芯

|检查项|结果|
|---|---|
|当前 Safety Guide|DG-0841 candidate 第 6；DG-0616 未进入当前 candidate top8|
|当前 Deepening Guide|DG-0921 top1|
|profile|没有 Safety profile 触发|
|candidate recall|Safety recall 弱；正确 Guide 在 candidate 但不在 selected top3|
|top1 competition|失败；容量下降语义压过拆包/换芯禁止语义|
|安全 evidence|DG-0921 自身含禁止拆包、换芯、点焊、BMS 改造停止线|

根因：candidate recall 与 ownership precedence 两层同时存在。`energy_battery_abnormal_isolation` 覆盖鼓包、发热、漏液等异常状态，但未覆盖拆包、换芯、点焊、BMS 改造等侵入式操作请求。

规划建议：先审查是否可在现有 Battery Safety 入口内增加“电池包 + 侵入式动作”窄触发，并建立正反边界；只有现有 profile 扩展会破坏异常隔离语义时，才考虑一个专用 invasive-operation-stop profile。不得使用“电池”“维修”“容量下降”单词独立触发。

### Battery 路线

1. 先冻结正向意图族：鼓包后继续测试、拆包、换芯、点焊、电芯重组、BMS 改造。
2. 冻结负向边界：正常成品电池的非侵入式容量复查继续由 DG-0921 主导。
3. 验证现有 Battery Safety profile 的最小扩展能否同时解决 O01/O07。
4. candidate 稳定后再验证 Safety top1；不要用 fixture ownership override 代替生产归属。

## 4. Solar Safety Debt

范围：O05。

### 当前 trace

- query：太阳能控制器正在冒烟但仍有输出，询问能否继续用。
- actual top3：DG-0564、DG-0617、DG-0922。
- 正确 Safety Guide：DG-0579、DG-0842，均未进入 candidate。
- 没有 profile 触发。
- fire stop evidence 最终存在，但 Safety / Fire 没有取得主位。

Solar Management 抢位的原因不是它应处理冒烟，而是对象“太阳能”和“还有输出/继续用”召回了 Solar 排程、充电与产出偏差入口；当前 profile 没有把“太阳能控制器 + 冒烟”识别为低压设备当前异常。

现有 Safety 内容已经足够：

- DG-0579 是浓烟/初起火情的 critical 撤离 owner。
- DG-0842 明确要求冒烟、火花、焦味、烫手时立即停用并隔离。
- `energy_low_voltage_fault_stop` 已负责低压设备异常，但 object triggers 未覆盖太阳能/充电控制器，state triggers 未覆盖“冒烟”。

规划建议：优先对现有 `energy_low_voltage_fault_stop` 做窄范围 cross-domain adjustment 评审，要求“太阳能控制器/充电控制器 + 冒烟/焦味/火花/异常热”召回 DG-0842，并在真实烟火升级时保留 DG-0579。当前证据不支持新建宽泛 Solar Safety profile。

必须保护：正常 Solar 排程、连续阴天、跨季产出偏差、清洁遮挡检查仍由 Management / Deepening 主导；“太阳能”或“输出低”不能单独触发 Safety。

## 5. Records Debt

范围：O08。

### 当前 trace

- query：已经保存三个月太阳能和电池记录，询问长期归档和可信度标记。
- Records expected：DG-0912、DG-0914、DG-0917。
- Records Guide 均未进入 candidate。
- `storage_energy_fuel_storage` 被触发，DG-0900 取得 top1。
- 实际主位将“电池 + 保存 + 标签”理解成物理能源物资储藏，而没有把“记录 + 归档 + 可信度”识别成信息对象 lifecycle。

O08 属于 Records profile 与 Storage object-boundary 问题：

- `records_archive_index` 已覆盖长期索引、离线归档、多介质备份和归档动作，但 object 侧没有稳定覆盖自然语言“记录/日志/数据”。
- `records_capture_trust_summary` 覆盖来源与 verification，但 O08 的“标记可信度”组合没有触发。
- `storage_energy_fuel_storage` 合理覆盖电池、保存和标签，但缺少“记录/日志/数据/归档/可信度是信息对象”时的排除边界。

规划建议：需要 Records / Long-Term Storage boundary review，但不新增宽泛 Records profile。优先评审：

1. 让现有 Records archive/trust profiles 识别“记录或日志 + 长期归档/索引/可信度”的复合意图。
2. 为 Storage profile 增加信息对象保护，只在保存的是电池、燃料等实物时主导。
3. 保留“电池长期不用怎么保存、如何贴物理状态标签”由 DG-0900 主导。
4. 保留 Records 只负责记录生命周期，不接管物理电池安全储藏。

## 6. 是否需要新增 Profile

|债务|规划结论|理由|
|---|---|---|
|O01 Battery abnormal + capacity|NO|现有 Safety profile 已触发，问题是 top1 ownership|
|O07 invasive battery operation|CONDITIONAL|先评审现有 Battery Safety profile 的窄扩展；仅在语义无法安全容纳时新增一个专用 stop profile|
|O05 solar controller smoke|NO|现有 `energy_low_voltage_fault_stop` 是正确语义归属，缺少对象/状态覆盖|
|O08 records archive/trust|NO|现有 `records_archive_index` 与 trust profile 已有职责，只需复合触发与 Storage 保护|

总体建议：本路线图不预批准新增 profile。后续 Root Cause Review 若证明 O07 无法由现有 Safety profile 在不扩域的前提下解决，最多评审一个窄的 battery invasive-operation-stop profile；其余债务不应新增 profile。

## 7. 是否需要调整 Ownership

需要，但调整应是生产语义主位，不是测试侧 owner map：

|意图|应有主位|可保留的 secondary|
|---|---|---|
|电池鼓包、漏液、冒烟、异常热后仍想测试|Energy Safety|DG-0921 只能在停止与隔离之后作为非操作性背景，不得 top1|
|拆包、换芯、点焊、电芯重组、BMS 改造|Energy Safety|Repair / Deepening 不得输出操作入口|
|太阳能控制器正在冒烟、起火、焦味、火花|Energy Safety / Fire|Solar Management / Deepening 只能在安全解除后处理恢复|
|记录/日志/数据的长期归档、索引、可信度|Records|Energy / Storage 可作为记录所属业务域，不得主导 lifecycle|
|电池、燃料等实物的长期保存|Long-Term Storage / Energy Safety|Records 只记录位置、状态与复查结果|

不建议按 Guide ID 硬编码 ownership，也不建议通过 fixture 扩大 accepted top1。应由查询中“当前危险状态、侵入式动作、信息对象或物理对象”的语义组合决定主位。

## 8. 是否需要新的 Cross-Domain Rule

需要两个窄、可测试的跨域语义契约；它们不是全局 ranking 加权规则：

### Rule A: Active Hazard / Invasive Action Precedence

当查询同时包含明确对象和当前危险状态或禁止性侵入动作时：

- Battery / low-voltage Safety 必须先进入 candidate，并优先于容量诊断、产出分析、排程和维修。
- 适用：鼓包、漏液、冒烟、异常热、火花、拆包、换芯、点焊、BMS 改造。
- 不适用：正常成品电池容量下降复查、无异常的太阳能产出偏差、已断电后的低风险维护。

### Rule B: Information Object Lifecycle Precedence

当被保存、归档、索引或标记可信度的对象明确是记录、日志、数据或档案时：

- Records 优先于物理 Storage。
- 当对象是电池、燃料、火柴、设备等实物时，Storage / Safety 保持主位。
- 业务域名词“太阳能”“电池”只说明记录主题，不能把记录对象重新解释成实物。

两个契约都必须先有 positive / negative boundary contracts，再进入任何 Minimal Apply。禁止建立“high risk Guide 永远 top1”或“Records 词出现就 top1”这类宽泛规则。

## 9. 不建议处理方向

- 不修改 DG-0919～DG-0925 或扩大 Energy Deepening profile。
- 不新增 Wiki、Guide 或 Guide-Wiki evidence。
- 不用全局 ranking、top_k、selector limit 或 evidence cap 掩盖 candidate root cause。
- 不以 Guide `risk_level` 单字段决定所有 top1；DG-0921 本身也是 high risk，无法表达 Safety 主位。
- 不为 O01/O07/O05/O08 写单题关键词或按 case ID 硬编码。
- 不修改 fixture accepted ownership 来制造通过。
- 不让 Solar Management 处理当前烟火，也不让 Repair 处理锂电池拆包换芯。
- 不让 Records 接管物理电池/燃料储藏，也不让 Storage 接管信息归档和可信度。
- 不在同一 Apply 中同时调整 Battery、Solar Safety 和 Records/Storage；应按安全优先级分批验证，避免无法归因。

建议后续顺序：

1. O05 Active Smoke Safety candidate recall。
2. O01/O07 Battery Safety candidate + ownership precedence。
3. O08 Records / Storage information-object boundary。
4. 每组独立 Root Cause Review、Minimal Apply、contract regression 和 stable-domain boundary verification。

最终规划建议：**NO APPLY**。
