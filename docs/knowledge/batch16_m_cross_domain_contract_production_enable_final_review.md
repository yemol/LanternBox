# LanternBox Batch16-M: Cross-Domain Contract Production Enable Final Review

生成日期：2026-08-29

输入：

- `docs/knowledge/batch16_l_cross_domain_contract_production_readiness_report.md`
- `docs/knowledge/batch16_k_cross_domain_contract_production_enable_review.md`

本批只执行最终审查与既有测试，不修改 Contract、Retrieval、profiles、Wiki、Guide、tests、fixture、database 或 runtime settings。production 未启用，当前实际模式保持 `off`。

## 1. Review Decision

Contract layer 本身通过单一来源、四态框架、ownership lane、只读 safety gate、audit 与 rollback 审查。

Production Enable 总门槛未通过：六个稳定域的既有 retrieval profile tests 共 **91 passed / 5 failed**。失败集中在 Energy Safety、Manufacturing 与 Agriculture，无法确认“无 unexpected regression”。

这些失败发生在 `contract_mode=off` 下，未发现 Contract runtime 直接改写 Retrieval 的证据；但当前 baseline 不干净，production enable 后将无法可靠区分既有漂移与 Contract 引入的回归。因此本批不得授权开启 production。

## 2. Contract Source

当前唯一权威来源：

`data/cross_domain_contracts.json`

|Check|Result|
|---|---|
|schema version|1.1|
|contract version|1.1.0|
|status|production_ready_not_enabled|
|runtime integration|true|
|default mode|off|
|rule count|15|
|SHA-256|18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2|

审查确认：

- runtime loader 与 tests 均读取同一 JSON source。
- Batch16-F validator 通过 `load_contract_source()` 与 `evaluate_ownership_contract()` 调用相同 runtime。
- runtime 只保留通用 schema、normalization、declarative matching、priority 与 decision assembly。
- Contract tests 明确检查 runtime 不包含领域词表副本。
- 在 Contract runtime、validator、Contract tests 与 contract data 中未发现 `1.0.0`、`1.1-shadow`、`design-reference` 或旧独立分类器残留。

结论：**PASS，v1.1 单一数据源成立，无隐藏旧 source。**

## 3. Production Mode

`contract_mode` 支持：

- `off`
- `shadow`
- `compare`
- `production`

默认配置为 `off`。当前 `data/runtime_settings.json` 未写入 `contract_mode`，运行时规范化结果为 `off`。

各模式当前边界：

|Mode|Behavior|
|---|---|
|off|不加载、不评估、不记录|
|shadow|只生成 OwnershipDecision 与 audit|
|compare|只记录 Retrieval / Contract ownership 差异|
|production|仅 readiness simulation：生成 decision 与 audit，不改变最终 Retrieval|

结论：**PASS，四态框架存在，默认和当前模式均为 off；production 未开启。**

## 4. Ownership Lane

`OwnershipDecision` 已输出：

- `primary_domain`
- `mandatory_secondary_domains`
- `suppressed_domains`
- `contract_reason`
- `ownership_lanes.primary`
- `ownership_lanes.mandatory_secondary`
- `ownership_lanes.suppressed`

同时包含 contract version/checksum、matched rule、decision status、confidence 与 timestamp。

代码路径与 invariance tests 确认 lane 只生成 proposed decision：

- 不修改 Wiki ranking。
- 不修改 Guide ranking。
- 不修改 candidate score/order。
- 不修改 selector result。
- 不写入 Prompt 或 answer。
- production simulation 与 off 的完整 `RetrievalV2Result.model_dump()` 相等。

结论：**PASS，ownership lane 为只读 decision layer。**

## 5. Safety Gate

验证范围：

- Active Hazard：battery abnormal、device smoke/overheat、active fire、unknown power action。
- Human Harm：电击伤、烧伤、烟气吸入、设备事故人员受伤。

`SafetyGateDecision` 属性：

- `mode=observe_only`
- 输出 required primary / mandatory secondary
- 输出 prohibited action labels
- 进入结构化 audit event
- 不包含执行器、设备控制、断电、隔离、撤离或维修调用
- 不生成或自动执行危险动作

Batch16-F matrix 结果：

|Metric|Result|
|---|---:|
|Contract cases|16 / 16|
|Fire boundary accuracy|5 / 5 = 100%|
|Human Harm ownership accuracy|5 / 5 = 100%|
|Information Object accuracy|6 / 6 = 100%|
|重点 regression|5 / 5|
|False positive|0|
|False negative|0|

结论：**PASS，Safety Gate 只读、可审计、不可自动执行危险动作。**

## 6. Contract Regression

运行：

`scripts/validate_cross_domain_contract_v1_1.py`

结果：

- contract version：1.1.0
- schema version：1.1
- checksum 与权威 source 一致
- 16 / 16 Contract matrix passed
- 5 / 5重点回归 passed
- false positive = 0
- false negative = 0

运行 5 个 Contract test modules，加 Retrieval traceability/root contracts：

**71 passed / 0 failed / 0 xfailed**

结论：**PASS，Contract 与基础 Retrieval contracts 无 unexpected fail。**

## 7. Stable Domain Regression

运行既有 stable domain profile tests：

|Domain|Test file|Result|
|---|---|---:|
|Energy Management|`test_energy_management_retrieval_profiles.py`|9 / 9 passed|
|Energy Safety|`test_energy_safety_retrieval_profiles.py`|6 / 8 passed，2 failed|
|Energy Deepening|`test_energy_system_deepening_retrieval_profiles.py`|20 / 20 passed|
|Medical|`test_medical_retrieval_profiles.py`|8 / 8 passed|
|Records|`test_long_term_records_knowledge_transfer_retrieval_profiles.py`|22 / 22 passed|
|Storage|`test_long_term_storage_retrieval_profiles.py`|12 / 12 passed|
|Manufacturing|`test_manufacturing_retrieval_profiles.py`|7 / 8 passed，1 failed|
|Agriculture|`test_agriculture_second_stage_retrieval_profiles.py`|7 / 9 passed，2 failed|
|Total|8 modules|**91 / 96 passed，5 failed**|

### 7.1 Energy Safety Failures

1. `test_spare_battery_hot_after_storage_prefers_battery_abnormal_isolation`
   - Guide top1 正确为 DG-0841。
   - Wiki candidates 为空，违反 stable evidence 预期。

2. `test_battery_leak_prefers_battery_abnormal_isolation`
   - Expected Guide top1：DG-0841。
   - Actual Guide top1：DG-0112。

### 7.2 Manufacturing Failure

`test_finished_shelf_heavy_load_prefers_load_check`

- Expected DG-0877 进入 Guide top2。
- Actual top2：DG-0874、DG-0875。

### 7.3 Agriculture Failures

1. `test_moldy_wet_seed_stays_in_seed_library_boundary`
   - Expected top1：DG-0879。
   - Actual top1：DG-0897。

2. `test_seed_food_batch_separation_prefers_postharvest_or_seed_library`
   - Expected top1：DG-0885 或 DG-0879。
   - Actual top1：DG-0897。

### 7.4 Regression Assessment

- Energy Management、Energy Deepening、Medical、Records、Storage 全部通过。
- Energy Safety、Manufacturing、Agriculture 未达到 clean baseline。
- 所有失败均在实际 `contract_mode=off` 时复现。
- 本批没有修改 failing tests、fixtures、profiles、Guide、Wiki 或 Retrieval。
- 本阶段只审查，不对这些失败执行 Apply。

结论：**FAIL，无法确认所有 stable domains 无 unexpected regression。**

## 8. Rollback

`tests/test_cross_domain_contract_production_readiness.py`：

**10 passed**

验证链：

`production simulation -> compare -> shadow -> off`

确认：

- mode normalization 顺序正确。
- 不写入 runtime settings。
- 不需要 database 或 Contract data migration。
- 切换前后 `data/cross_domain_contracts.json` 与 `data/lanternbox.db` 的 size、mtime 不变。
- `off` 不加载 Contract source。
- production simulation 不改变 Retrieval output。

结论：**PASS，rollback 无 migration。**

## 9. Audit

结构化事件前缀：

`cross_domain_contract_audit`

实测字段：

|Field|Result|
|---|---|
|contains_full_query|false|
|raw query field|不存在|
|query identifier|SHA-256，64 hex chars|
|retention_days|30|
|retention_class|local_rotating_jsonl|
|event_version|1.0|
|contract_version|1.1.0|
|timestamp|UTC ISO-8601|
|Safety Gate|gate type + observe_only|

包含姓名的测试 query 不会在序列化 audit event 中保存原文或姓名。compare event 可记录 baseline ownership 与 difference type，但不记录 Prompt 或 answer。

结论：**PASS，audit 满足脱敏、retention、version 与 timestamp 要求。**

## 10. Enable Gate Summary

|Gate|Result|
|---|---|
|v1.1 single source|PASS|
|runtime/test version consistency|PASS|
|no hidden old source|PASS|
|four production modes|PASS|
|default/current mode off|PASS|
|ownership lane fields|PASS|
|no ranking/Prompt mutation|PASS|
|Active Hazard / Human Harm gate|PASS|
|Batch16-F matrix|PASS|
|Contract tests|PASS|
|stable domain regression|**FAIL**|
|rollback without migration|PASS|
|audit privacy/retention/version/time|PASS|

## 11. Required Before Re-review

本批不修复。再次 Production Enable Review 前必须：

1. 在独立 stable-domain 修复批次中调查并处理或正式基线化上述 5 个失败。
2. 不得用 Cross-Domain Contract、全局 ranking、selector、top_k、Prompt 或 fixture 放宽来掩盖失败。
3. 重跑本报告中的 96 项 stable-domain suite，目标 96 / 96 passed。
4. 重跑 Batch16-F matrix、71 项 Contract/Retrieval contracts 与 rollback drill。
5. 继续保持实际 `contract_mode=off`，直到新的最终审查明确通过。

最终状态：**BLOCKED**
