# LanternBox Batch16-F: Cross-Domain Contract Revalidation Report

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_c_cross_domain_contract_apply_report.md`
- `docs/knowledge/batch16_d_cross_domain_contract_validation_report.md`
- `docs/knowledge/batch16_e_cross_domain_contract_revision.md`

本阶段只验证 Batch16-E v1.1 Contract Revision。新增独立 runner `scripts/validate_cross_domain_contract_v1_1.py`，未修改 contract data、既有 tests、Retrieval、profiles、ranking、selector、top_k、fallback、Wiki、Guide、Guide-Wiki、database 或 PocketBase。

## 1. Test Scope

验证分为三层：

1. 重跑 Batch16-C / Batch16-D 既有 contract tests，确认冻结 baseline 未回退。
2. 用独立 v1.1 design-reference runner 执行 16 个新验证 case：Fire 5、Human Harm 5、Information Object 6。
3. 执行 5 个重点回归：Battery abnormal、正常容量下降、未知电源并联、Energy records、Spare parts。

所有 expected 在 runner 执行前固定。runner 不读取或写入 `data/cross_domain_contracts.json`，不调用 Retrieval，也不根据实际结果调整 expected。

## 2. Contract Version

|项目|状态|
|---|---|
|当前 contract data|v1.0.0 / `contract_only`|
|本次验证目标|Batch16-E v1.1 design revision|
|验证实现|独立 `v1.1-design-reference` runner|
|runtime integration|false|
|生产行为变化|无|

本报告中的 READY 表示 v1.1 语义设计已经通过独立验证，可以进入下一阶段的受控生产集成 Apply；不表示 v1.1 已写入 contract data 或已接入 Retrieval。

## 3. Batch16-C Regression

既有测试文件未修改并重新运行：

|范围|结果|
|---|---|
|Batch16-C 原始 contract tests|17 passed|
|Batch16-D 既有 validation non-gap cases|6 passed|
|Batch16-D 冻结 gap markers|3 xfailed|
|合并 baseline|23 passed / 3 xfailed|

三条 xfail 仍然存在是预期行为：生产 contract data 按本阶段禁止项保持 v1.0，因此旧 evaluator 仍应暴露 Fire negative、Medical injury 和 Seed storage 缺口。本阶段没有修改 expected 或移除 xfail。

重点回归由 v1.1 runner 重新验证：

|边界|Expected|Result|
|---|---|---|
|Battery abnormal|Energy Safety|PASS|
|Normal capacity degradation|Energy Deepening|PASS|
|Unknown power parallel|Energy Safety|PASS|
|Energy records|Records|PASS|
|Spare parts storage|Storage + Records supplemental|PASS|

Regression count：**0**；5 / 5 通过。

## 4. Batch16-D Fixed Cases

|Batch16-D gap|v1.1 rule|Revalidation|
|---|---|---|
|“没有看到明火”误触发 Fire|Negative Fire Evidence masking|PASS：Energy Safety primary，Fire secondary|
|电击烧伤且设备已断电无 Medical owner|Human Harm Priority|PASS：Medical primary，Energy Safety secondary|
|种子本体无法分类|Biological Material storage|PASS：Seed Library primary，Records supplemental|

三个 strict xfail 对应的 v1.1 reference case 全部通过，没有通过修改旧测试或 contract data 掩盖问题。

## 5. Fire Validation

|Case|Expected primary|Expected secondary|Actual|Result|
|---|---|---|---|---|
|F-N01 充电器冒烟但没有明火|Energy Safety|Fire|Energy Safety / Fire|PASS|
|F-N02 接头异常热、无烟无燃烧|Energy Safety|无强制 Fire|Energy Safety|PASS|
|F-N03 历史起火已熄灭，现场安全后复盘|Records|Fire、Energy Safety|Records / Fire / Energy Safety|PASS|
|F-P01 设备起火并出现浓烟|Fire|Energy Safety|Fire / Energy Safety|PASS|
|F-P02 之前无明火、现在已起火|Fire|Energy Safety|Fire / Energy Safety|PASS|

Fire boundary accuracy：**100.0%（5 / 5）**。

验证确认：

- negative fire phrase 只屏蔽其局部 signal。
- 冒烟、焦味与异常热仍保持 Energy Safety priority。
- 历史且已解除的火情可转 Records。
- 新的当前 positive evidence 可以覆盖此前 negative evidence。

## 6. Human Harm Validation

|Case|Expected primary|Expected secondary|Actual|Result|
|---|---|---|---|---|
|H-01 电击烧伤，设备已断电|Medical|Energy Safety|Medical / Energy Safety|PASS|
|H-02 吸入烟气后咳嗽头晕|Medical|Fire、Energy Safety|Medical / Fire / Energy Safety|PASS|
|H-03 设备火花但无人受伤|Energy Safety|无强制 Medical|Energy Safety|PASS|
|H-04 有人受伤且仍有明火，询问伤者|Medical|Fire、Energy Safety|Medical / Fire / Energy Safety|PASS|
|H-05 无人受伤、设备已断电，检查线缆|Repair 可进入|Energy Safety|Repair / Energy Safety|PASS|

Human Harm ownership accuracy：**100.0%（5 / 5）**。

验证确认：

- 人员当前受伤时 Medical override 设备 Safety primary。
- 设备已断电不会错误结束 Medical urgency。
- 无人受伤时不会误触发 Medical。
- 仍有明火时 Fire 保持 mandatory secondary 和现场撤离边界。
- 无伤且已断电后 Repair 可以在 Safety 确认下进入。

## 7. Information Object Validation

|Case|Expected|Actual|Result|
|---|---|---|---|
|P-01 种子本体装袋、防潮、入库|Seed Library / Storage + Records|Seed Library + Records|PASS|
|P-02 种植试验日志与发芽率数据归档|Records|Records + Agriculture|PASS|
|P-03 医疗样本试管低温保存与交接|Medical / Bio Safety custody|Medical Bio Safety + Records|PASS|
|P-04 医疗样本检测结果可信度与归档|Records|Records + Medical|PASS|
|P-05 卫生耗材入库存放|Medical Storage|Medical Storage + Records|PASS|
|P-06 卫生耗材消耗日志长期趋势|Records|Records + Medical|PASS|

Information Object accuracy：**100.0%（6 / 6）**。

验证确认：

- Biological Material 的本体与实验记录可以分离。
- Medical Sample 的样本本体 custody 与检测结果 lifecycle 可以分离。
- Consumable 的实物入库与消耗日志趋势可以分离。
- Records supplemental 不会抢占物理 custody，业务域 supplemental 也不会抢占 Records lifecycle。

## 8. Remaining Gaps

本次指定验证矩阵没有剩余语义失败：

- Contract pass：16 / 16。
- Fire、Human Harm、Information Object 均为 100%。
- False positive：0。
- False negative：0。
- Regression：0。

仍有一个阶段性实现差距，但不是设计失败：`data/cross_domain_contracts.json` 仍为 v1.0，Batch16-E v1.1 尚未 Apply，生产 Retrieval 也未接入 contract layer。因此：

- 旧 Batch16-D 三项 xfail 必须保留到下一阶段 Apply。
- 生产集成时必须把本 runner 的固定矩阵转换为正式 contract-data / integration tests。
- 集成不得改写 expected、增加 global ranking、扩大 top_k 或借 selector 调整掩盖 ownership。
- Apply 后必须重跑 23 passed baseline、16 个 v1.1 case、5 个重点回归以及既有 Retrieval contracts。

## 9. Whether Contract Is Ready for Production Integration

|指标|结果|
|---|---:|
|Contract pass rate|100.0%（16 / 16）|
|Fire boundary accuracy|100.0%（5 / 5）|
|Human Harm ownership accuracy|100.0%（5 / 5）|
|Information Object accuracy|100.0%（6 / 6）|
|Regression count|0（5 / 5 passed）|
|False positive count|0|
|False negative count|0|

验证前后对 contract data、两个既有 contract test、Retrieval profile、orchestrator、selector 与 fetchers 执行 SHA-256 对比，7 / 7 一致。当前工作区生产行为未变化。

Batch16-E v1.1 设计已通过固定 case、负向边界、跨域 ownership、suppression、Safety priority 与对象分类验证，可以进入单独的 Production Integration Apply 阶段。

最终状态：**READY FOR PRODUCTION INTEGRATION**
