# LanternBox Batch16-O: Cross-Domain Contract Production Enable Re-Review

生成日期：2026-08-29

权威输入：

- `docs/knowledge/batch16_m_cross_domain_contract_production_enable_final_review.md`
- `docs/knowledge/batch16_n_stable_domain_regression_root_cause_review.md`
- `docs/knowledge/batch16_l_cross_domain_contract_production_readiness_report.md`
- `docs/knowledge/batch16_j_cross_domain_contract_compare_report.md`

本批严格遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`，只重新审查 Production Enable 条件。没有修改 Contract、Retrieval、profiles、ranking、selector、Prompt、Wiki、Guide、tests、fixtures、database 或 runtime settings；没有启用 production。

## 1. Previous Batch16-M Blocker

Batch16-M 已确认 Contract Layer 本身通过以下审查：

- v1.1 单一数据源。
- off / shadow / compare / production 四态框架。
- ownership lanes。
- Active Hazard / Human Harm 只读 Safety Gate。
- 脱敏 audit。
- 无 migration rollback。

Batch16-M 的 blocker 是 stable-domain suite 仅有 **91 / 96 passed**。当时五个失败虽然都在 `contract_mode=off` 下出现，但缺少足够历史证据区分“Batch16 新增 regression”和“此前已存在的稳定域债务”，因此报告采用保守结论 `BLOCKED`。

冻结的五个失败为：

|ID|Domain|Failure|
|---|---|---|
|ES-1|Energy Safety|备用电池长期存放后发热：DG-0841 top1，但 Wiki candidates 为空|
|ES-2|Energy Safety|电池漏液：expected DG-0841，actual DG-0112|
|MFG-1|Manufacturing|成品架子承重：DG-0877 为 rank 3，未进入 top2|
|AGR-1|Agriculture|受潮霉味种子：expected DG-0879，actual DG-0897|
|AGR-2|Agriculture|留种/食用批次分离：expected DG-0885/DG-0879，actual DG-0897|

本次不撤销 Batch16-M 当时的保守判断，只使用 Batch16-N 新增的历史因果证据重新评审 gate。

## 2. Batch16-N RCA Result

Batch16-N 使用 pre-Batch16 reference commit `a796331` 建立 frozen baseline，并完成 off、runtime bypass、shadow、compare 四组隔离验证。

|RCA question|Result|
|---|---|
|五项能否在 Batch16 前复现|YES，5 / 5|
|`a796331` 是否包含 Contract runtime|NO|
|`a796331` 是否包含 Contract data|NO|
|当前 off 是否复现相同结果|YES，5 / 5|
|Contract runtime 完全 bypass 后是否仍复现|YES，5 / 5|
|shadow / compare 是否改变 candidate snapshot|NO|
|Batch16 introduced regression|**0**|

分类冻结为：

|Classification|Count|Cases|
|---|---:|---|
|PRE-EXISTING STABLE-DOMAIN DEBT|3|ES-1、ES-2、MFG-1|
|PRE-EXISTING TEST / FIXTURE DEBT|2|AGR-1、AGR-2|
|Batch16 regression|0|无|

本报告接受 Batch16-N 的根因结论，不再将这五项计为 Batch16 新 regression，也不把它们伪装为通过。

## 3. Re-Run Scope And Results

### 3.1 Contract v1.1 Fixed Matrix

运行：

```text
venv/bin/python scripts/validate_cross_domain_contract_v1_1.py
```

结果：

|Metric|Result|
|---|---:|
|Contract version|1.1.0|
|Schema version|1.1|
|Contract source|`data/cross_domain_contracts.json`|
|Contract checksum|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|
|Fixed matrix|16 / 16|
|Fire boundary|5 / 5|
|Human Harm ownership|5 / 5|
|Information Object|6 / 6|
|重点 regression|5 / 5|
|False positive|0|
|False negative|0|

### 3.2 Contract And Retrieval Contracts

运行五个 Contract modules 加 Retrieval traceability/root contracts：

```text
tests/test_cross_domain_ownership_contracts.py
tests/test_cross_domain_contract_validation.py
tests/test_cross_domain_contract_shadow_integration.py
tests/test_cross_domain_contract_compare_mode.py
tests/test_cross_domain_contract_production_readiness.py
tests/test_retrieval_traceability.py
tests/test_retrieval_root_contract.py
```

结果：**71 passed / 0 failed / 0 xfailed**。

### 3.3 Rollback Tests

运行：

```text
tests/test_cross_domain_contract_production_readiness.py
```

结果：**10 passed / 0 failed**。

验证链：

`production simulation -> compare -> shadow -> off`

确认不写 runtime settings、不修改 Contract data 或 database、不需要 schema/data migration。

### 3.4 Shadow And Compare Validation

重新运行 shadow 与 compare validators：

|Metric|Shadow|Compare|
|---|---:|---:|
|Total cases|12|12|
|Contract accuracy|100%|100%|
|Active Hazard accuracy|100%|包含在 12 / 12|
|Information Object accuracy|100%|包含在 12 / 12|
|Energy Deepening protection|100%|包含在 12 / 12|
|A|3|3|
|B|0|0|
|C / accepted non-error|2 + 7 exact match|9|
|D|0|0|

两个 validator 的分类表示方式不同：shadow 输出 `MATCH=7, A=3, C=2, D=0`；compare 将 MATCH 与 acceptable cases 统一记为 C，输出 `A=3, C=9, D=0`。两者 Contract accuracy 都是 100%，且 Contract error D 都为 0。

## 4. Absolute Regression Result

重新运行八个 stable-domain retrieval profile modules。为避免 field runner import 时重复 exec，运行环境明确设置 `LANTERNBOX_ENERGY_DEEPENING_FIELD_TEST_VENV=1`；没有修改测试或 expected。

|Domain|Result|
|---|---:|
|Energy Management|9 / 9 passed|
|Energy Safety|6 / 8 passed，2 failed|
|Energy Deepening|20 / 20 passed|
|Medical|8 / 8 passed|
|Long-Term Records|22 / 22 passed|
|Long-Term Storage|12 / 12 passed|
|Manufacturing|7 / 8 passed，1 failed|
|Agriculture Second Stage|7 / 9 passed，2 failed|
|Absolute total|**91 / 96 passed，5 failed**|

Absolute result 不是全绿。五个失败与 Batch16-M / Batch16-N 的冻结清单逐项一致，没有新增第六项失败，也没有已有失败改变 failure signature。

## 5. Baseline-Relative Regression Result

Baseline：pre-Batch16 committed reference `a796331`。

|Baseline parity check|Result|
|---|---|
|Frozen baseline failures|5|
|Current failures|5|
|Same test nodes|5 / 5|
|Same failure behavior|5 / 5|
|Resolved baseline failures|0|
|New failures|**0**|
|Batch16 NEW regression|**0**|

结论：Absolute result 为 91 / 96，但 baseline-relative result 为 **NEW regression = 0**。两项结果同时保留，不互相替代。

## 6. Contract Integrity Gate

Gate A：Contract / Integration Integrity。

|Requirement|Result|
|---|---|
|Contract v1.1 matrix = 16 / 16|PASS|
|Contract/Retrieval contracts all pass|PASS，71 / 71|
|Rollback tests all pass|PASS，10 / 10|
|Shadow Contract accuracy = 100%|PASS|
|Compare Contract accuracy = 100%|PASS|
|Difference D = 0|PASS|
|off Retrieval output invariance|PASS|
|shadow vs off complete `model_dump()` equality|PASS|
|compare vs off complete `model_dump()` equality|PASS|
|production framework vs off complete `model_dump()` equality|PASS|
|False positive / false negative|PASS，0 / 0|

Gate A final：**PASS**。

## 7. Integration Readiness

### 7.1 Source Of Truth

- 唯一权威 source：`data/cross_domain_contracts.json`。
- schema version：1.1。
- contract version：1.1.0。
- runtime、validator 与 tests 使用同一 loader/source。
- 当前 checksum 与 Batch16-M/L 冻结值一致。
- 没有启用隐藏 v1.0 classifier 或第二份领域词表。

### 7.2 Four Modes

框架支持：

- `off`
- `shadow`
- `compare`
- `production`

Contract source 的 `default_mode` 为 `off`。当前 runtime settings 规范化结果为 `off`，`production_enabled=False`。production 只在 tests 中模拟，本批没有实际切换。

### 7.3 Ownership Lanes

`OwnershipDecision` 已提供：

- `primary_domain`
- `mandatory_secondary_domains`
- `suppressed_domains`
- `ownership_lanes.primary`
- `ownership_lanes.mandatory_secondary`
- `ownership_lanes.suppressed`
- `contract_reason`

这些 lane 当前只生成 decision，不直接改写 Wiki/Guide ranking、candidate score/order、selector、Prompt 或 answer。

### 7.4 Safety Gate

Safety Gate 覆盖：

- Active Hazard。
- Human Harm。

Gate 保持 `observe_only`，只输出 required ownership、mandatory secondary 和 prohibited action labels；没有设备控制、自动断电、自动隔离、自动维修或危险动作执行器。

### 7.5 Audit

结构化 audit 已验证：

|Field|Result|
|---|---|
|event version|存在|
|contract version|1.1.0|
|timestamp|UTC ISO-8601|
|full query saved|false|
|query identifier|SHA-256|
|retention days|30|
|retention class|`local_rotating_jsonl`|

Integration readiness final：**PASS**。

## 8. Rollback Readiness

Rollback 顺序：

`production -> compare -> shadow -> off`

重新运行结果为 10 / 10 passed，并确认：

- 四种 mode 可以直接规范化和切换。
- 切换不要求 database migration。
- 切换不要求 Contract source migration。
- 测试不写入 runtime settings。
- `off` 不加载或评估 Contract source。
- production simulation 与 off 的最终 Retrieval output 相等。

Rollback readiness final：**PASS**。

## 9. Historical Debt Register

这些项目保持公开失败状态，但均标记为 PRE-EXISTING，不在 Batch16 中修复：

|Debt ID|Domain|Type|Current failure|Disposition|
|---|---|---|---|---|
|ES-1|Energy Safety|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0841 top1 后 Wiki recall 为空|后续 Stable Domain Debt Remediation|
|ES-2|Energy Safety|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0112 压过 DG-0841|后续 Energy Safety / Medical ownership review|
|MFG-1|Manufacturing|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0877 rank 3|后续 Manufacturing debt remediation|
|AGR-1|Agriculture|PRE-EXISTING TEST / FIXTURE DEBT|旧 Agriculture top1 expected 与 Storage stable boundary 冲突|后续 fixture/ownership review|
|AGR-2|Agriculture|PRE-EXISTING TEST / FIXTURE DEBT|旧 Agriculture top1 expected 与 Storage stable boundary 冲突|后续 fixture/ownership review|

Debt totals：Energy Safety x2、Manufacturing x1、Agriculture x2。

这些债务没有被标记为 PASS，不计入 Batch16 regression，也不得与 Cross-Domain Contract canary enable 混合处理。

## 10. Three Production Benefit Cases

Batch16-J 三个 A 类 case 重新验证如下：

|ID|Query focus|Existing Retrieval primary|Contract primary|Difference|Judgement|
|---|---|---|---|---|---|
|A1 / E03|控制器冒烟、无明火|Energy Management / DG-0617|Energy Safety；Fire mandatory secondary|A|Contract identifies existing Retrieval ownership debt|
|A2 / E04|太阳能跨季趋势|Energy Management / DG-0617|Energy Deepening|A|Contract identifies existing Retrieval ownership debt|
|A3 / W01|维修日志按版本归档|Repair / Tools / DG-0839|Records|A|Contract identifies existing Retrieval ownership debt|

三项 Contract decision 均符合 v1.1 ownership contract。Compare validator 仍输出 A case IDs `E03, E04, W01`，Contract accuracy 为 100%，D=0。因此它们是现有 Retrieval ownership debt，不是 Contract error。

## 11. Gate Summary

Gate B：Baseline Parity。

|Requirement|Result|
|---|---|
|Reference baseline 明确为 `a796331`|PASS|
|五个历史失败有 pre-Batch16 reproduction|PASS|
|当前 failure set 与 frozen baseline 相同|PASS|
|Batch16 NEW regression = 0|PASS|
|Historical failures entered in debt register|PASS|
|Historical failures not represented as pass|PASS|
|No in-batch stable-domain repair|PASS|

Gate B final：**PASS**。

综合 Go / No-Go：

|Condition|Result|
|---|---|
|Contract / Integration Gate|PASS|
|Batch16 NEW regression|0|
|Rollback ready|PASS|
|Difference D|0|
|Baseline certainty|PASS|
|Current actual mode|off|
|Production actually enabled|NO|

## 12. Final Production Judgement

Batch16-O 的 go 条件全部满足：Contract/Integration Gate PASS、Baseline Parity PASS、Batch16 NEW regression=0、rollback ready、D=0，且历史基线没有矛盾。

该结论只授权进入受控 canary production enable 阶段，不代表本批已经切换 mode，也不授权顺带修复五项稳定域债务。实际启用前仍应保持可立即回退至 compare / shadow / off，并持续记录 Contract audit 与 canary 差异。

当前实际 `contract_mode`：**off**。

最终状态：**READY FOR CANARY PRODUCTION ENABLE**
