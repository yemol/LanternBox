# LanternBox Batch16-D: Cross-Domain Contract Validation Report

生成日期：2026-08-25

输入：

- `data/cross_domain_contracts.json`
- `docs/knowledge/batch16_c_cross_domain_contract_apply_report.md`

本阶段只进行泛化验证。新增 `tests/test_cross_domain_contract_validation.py`，未修改 contract data、Retrieval、profile、ranking、selector、Wiki 或 Guide，也未将契约接入生产。

## 1. Validation Method

验证继续使用 Batch16-C 的声明式 reference evaluation 语义：

1. Active Hazard 先于 Information Object。
2. 按 rule priority 匹配 object、signal 与 all-term groups。
3. 检查 primary owner、allowed secondary、suppressed primary 和 required safety actions。
4. Information Object 检查信息对象与物理对象分类，以及 Records / Storage 主位。

新增 9 个泛化 case，均使用不同于 Batch16-C A01～I04 的自然语言表达。未将完整 query 写入 contract data。

## 2. Result Summary

|Contract|Pass|Expected gap|结论|
|---|---:|---:|---|
|Active Hazard|2|2|Battery abnormal 与 Fire escalation 泛化；Smoke negation 与 Medical injury 不完整|
|Information Object|4|1|跨业务 Records 与 Spare parts 泛化；Seed physical storage 不完整|
|Validation total|6|3|6 passed / 3 strict xfailed|

Batch16-C 原始 contract tests 与本次 validation 合并运行：

- 23 passed
- 3 xfailed
- 0 unexpected fail

三个 xfail 均为本阶段新发现并冻结的 contract gap；使用 `strict=True`，未来 contract 修订后若意外通过，测试会要求移除债务标记并重新确认预期。

## 3. Active Hazard Validation

### V-A01 Battery Abnormal

Query：移动电源外壳突然鼓起并发烫，询问下一步。

|检查项|结果|
|---|---|
|Matched rule|`battery_abnormal`|
|Primary|Energy Safety|
|Suppressed|Energy Deepening、Energy Management、Repair、Storage|
|Safety actions|STOP、disconnect、isolate|
|Verdict|PASS|

结论：Battery abnormal 已从“电池”泛化到“移动电源”，Safety priority 与 suppression 完整。

### V-A02 Smoke Without Fire

Query：充电器持续冒烟并有焦味，但明确“没有看到明火”。

Expected：Energy Safety primary，Fire escalation secondary。

Actual：`active_fire` / Fire primary。

Verdict：XFAIL。

根因：`active_fire.signal_any` 包含“明火”，reference evaluator 以子串匹配读取了否定短语“没有看到明火”。当前 contract 只有整句 prohibition context，没有针对 hazard state 的局部否定语境，因此无法区分“有明火”和“没有明火”。

影响：Safety 仍然优先于 Deepening / Management / Repair，但 Fire 与 Energy Safety 的层级判定错误。该问题会扩大 Fire primary，不是危险漏检，但不满足契约精确 ownership。

### V-A03 Fire Escalation

Query：电源模块已经起火并出现浓烟。

|检查项|结果|
|---|---|
|Matched rule|`active_fire`|
|Primary|Fire|
|Allowed secondary|Energy Safety、Medical|
|Suppressed|Energy Deepening、Energy Management、Repair、Records、Storage|
|Verdict|PASS|

结论：明确明火与浓烟可以稳定提升 Fire primary，Safety priority 与 suppression 正确。

### V-A04 Medical Injury After Electrical Incident

Query：人员被电源线电击后手部烧伤，设备已经断电，询问应先处理什么。

Expected：Medical primary，Repair suppressed。

Actual：无 contract decision。

Verdict：XFAIL。

根因有两层：

1. Active Hazard rules 没有“电击 / 烧伤 / 人员受伤”到 Medical primary 的 decision rule。
2. `resolved_state_terms` 将“已经断电”视为退出 Active Hazard，但危险源解除不代表伤者的医疗紧急性解除。

影响：当前合同无法治理“事故源已安全、人员伤害仍在”的跨域交接。该缺口具有安全意义，是生产集成阻断项。

## 4. Information Object Validation

### V-I01 Energy Records

Query：离网系统全年发电与电池日志建立版本和归档索引。

- Matched：`records_lifecycle`
- Primary：Records
- Suppressed：Storage
- Verdict：PASS

结论：能源主题没有改变日志的信息对象身份。

### V-I02 Manufacturing Repair Logs

Query：工作台设备连续维修失败日志按版本归档。

- Matched：`records_lifecycle`
- Primary：Records
- Suppressed：Storage
- Verdict：PASS

结论：Manufacturing / Repair 主题没有抢占 lifecycle 主位；业务域仍可作为记录内容来源。

### V-I03 Agriculture Experiment Records

Query：三季种植试验数据标记可信度并长期归档。

- Matched：`records_trust`
- Primary：Records
- Suppressed：Storage
- Verdict：PASS

结论：Agriculture 数据的可信度与归档可以稳定归 Records。

### V-I04 Seed Physical Storage

Query：备用种子装袋贴标签后放入干燥区长期存放。

Expected：Storage primary，Records supplemental。

Actual：无 contract decision。

Verdict：XFAIL。

根因：`physical_storage.object_any` 包含物资、材料、设备、电池、燃料、工具、零件和成品，但没有种子或可覆盖生物储备物的对象类别。Storage action 已存在，object classification 仍无法成立。

影响：Information Object Contract 对技术物资具有泛化能力，但尚未覆盖 Agriculture / Seed Library 的物理储藏对象，不能宣称跨域通用。

### V-I05 Spare Parts Storage

Query：备用零件贴标签后入库存放。

- Matched：`physical_storage`
- Primary：Storage
- Supplemental：Records
- Verdict：PASS

结论：Spare parts 的对象分类、Storage 主位与 Records supplemental 均正确。

## 5. Ownership and Suppression Review

|验证维度|结果|说明|
|---|---|---|
|Contract ownership|PARTIAL|6 个泛化 case ownership 正确，3 个无法满足预期|
|Primary domain|PARTIAL|Smoke 被过度提升到 Fire；Medical 与 Seed 无 primary|
|Suppressed domains|PASS on matched rules|所有成功匹配 rule 的 suppression 均符合当前数据声明|
|Safety priority|PARTIAL|危险不会被 Deepening/Management/Repair 抢占，但 Medical injury 未覆盖|
|Object classification|PARTIAL|Records 跨 Energy/Manufacturing/Agriculture 成功；Seed physical object 缺失|

Smoke case 的 Fire over-classification 不会降低停止优先级，但它证明当前否定语境处理不足。Medical case 则可能完全缺失所需主域，风险更高。

## 6. Required Contract Revisions

下一阶段应先修订 contract data，再重新 Validation；不要直接接入 Retrieval。

### Revision 1: Hazard-State Negation

- 为 hazard signal 定义局部否定语境，而不是只识别整句“禁止/不要”。
- 至少区分“有明火 / 已起火”和“没有明火 / 未见明火”。
- 必须保护“冒烟但无明火”仍由 Energy Safety primary，Fire escalation secondary。

### Revision 2: Medical Injury Ownership

- 新增电击、烧伤、烟气吸入、漏液接触等人员伤害语义到 Medical primary 的契约路径。
- 将“危险源已断电/隔离”和“人员伤害已处理”拆成不同退出条件。
- 设备断电后 Repair 仍不得先于正在处理的人员伤害。

### Revision 3: Physical Storage Object Coverage

- 扩展物理 Storage object class，至少覆盖种子。
- 优先采用有边界的对象类别，不用无限追加单题词。
- 必须保护“种子试验数据/发芽记录”仍是 Records，而“种子本体装袋、防潮、入库”是 Storage / Seed Library。

建议新增 revision regression：

1. 冒烟且未见明火：Energy Safety。
2. 明火或浓烟扩大：Fire。
3. 已断电但有人电击烧伤：Medical。
4. 已断电且无人受伤，外部线缆检查：Repair 可进入。
5. 种子本体装袋入库：Storage。
6. 种子试验记录归档：Records。

## 7. No Production Change Confirmation

验证前后对以下文件执行 SHA-256 对比，6 / 6 一致：

- `data/cross_domain_contracts.json`
- `data/retrieval_query_profiles.json`
- `api/retrieval_v2/orchestrator.py`
- `api/retrieval_v2/selector.py`
- `api/retrieval_v2/fetchers.py`
- `data/emergency_guides.json`

Contract data SHA-256 保持：

`87615f34dc342ed8dacf77cd2a0ae4555264499405cf12d71707910f077f79af`

本阶段只新增 validation test 与本报告，未执行生产集成。

## 8. Recommendation

当前 Contract Layer 已证明部分泛化能力：Battery Safety、明确 Fire escalation、跨业务 Records lifecycle/trust 和一般 Spare Parts Storage 均有效。

但以下三项阻止生产集成：

1. hazard-state 否定语境会造成 Fire primary 误判。
2. 电气事故后 Medical injury 没有 ownership 路径。
3. Seed physical storage 没有对象分类。

应先进入一次有限 Contract Revision，只修改契约数据与对应 contract tests；仍不应修改 Retrieval、profile、ranking 或 selector。

最终结论：**NEEDS CONTRACT REVISION**
