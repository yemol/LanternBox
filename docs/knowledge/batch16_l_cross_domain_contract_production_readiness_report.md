# LanternBox Batch16-L: Cross-Domain Contract Production Readiness Implementation

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_k_cross_domain_contract_production_enable_review.md`
- `docs/knowledge/batch16_j_cross_domain_contract_compare_report.md`

本批只消除 Production Enable 前置实现阻断项。Contract production 行为没有启用，当前运行配置保持 `off`；OwnershipDecision、ownership lanes 与 safety gate 均未进入 Retrieval candidate、ranking、selector、Prompt 或 answer。

## 1. Current Status

|Readiness item|Result|
|---|---|
|Contract v1.1 单一数据源|PASS|
|schema / contract version 明确|PASS|
|runtime 与 tests 同源|PASS|
|off / shadow / compare / production 框架|PASS|
|默认与当前模式为 off|PASS|
|Ownership lane model|PASS，read-only|
|Active Hazard / Human Harm safety gate|PASS，observe-only|
|结构化脱敏 audit event|PASS|
|production -> compare -> shadow -> off drill|PASS|
|状态切换无数据迁移|PASS|
|Retrieval output invariance|PASS|
|production 实际启用|NO，本批明确未启用|

## 2. Changed Files

Contract source 与 runtime：

- `data/cross_domain_contracts.json`
- `api/retrieval_v2/contract_runtime.py`
- `api/retrieval_v2/contract_compare.py`
- `api/retrieval_v2/orchestrator.py`
- `api/config.py`
- `api/models.py`
- `api/routes.py`

验证器与测试：

- `scripts/validate_cross_domain_contract_v1_1.py`
- `scripts/validate_cross_domain_contract_shadow.py`
- `tests/test_cross_domain_ownership_contracts.py`
- `tests/test_cross_domain_contract_validation.py`
- `tests/test_cross_domain_contract_shadow_integration.py`
- `tests/test_cross_domain_contract_compare_mode.py`
- `tests/test_cross_domain_contract_production_readiness.py`

报告：

- `docs/knowledge/batch16_l_cross_domain_contract_production_readiness_report.md`

未修改 Wiki、Guide、Guide-Wiki、Retrieval profiles、ranking、selector、top_k、fallback、Prompt、PocketBase 或 database schema。

## 3. Contract Source Consolidation

唯一权威来源为：

`data/cross_domain_contracts.json`

冻结属性：

|Field|Value|
|---|---|
|schema_version|1.1|
|contract_version|1.1.0|
|status|production_ready_not_enabled|
|runtime_integration|true|
|default_mode|off|
|rule count|15|
|SHA-256|18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2|

`contract_runtime.py` 现在只保留通用能力：

- JSON schema model 与启动校验
- Unicode / whitespace normalization
- declarative all / none / mask phrase 匹配
- priority evaluation
- contextual secondary assembly
- OwnershipDecision 与 audit event 组装

领域词表、negative evidence、owner、suppression、confidence 和 safety-gate policy 均来自同一 JSON。旧 v1.1 validator 的独立 Python 分类器已删除；runtime、contract tests 与 revalidation runner 都调用同一 loader / evaluator。

loader 会拒绝 unsupported schema、非 runtime source、非 off 默认值、保存全文 query 的 audit policy、空规则、重复 rule ID 与 unknown owner。模块导入不再预加载 ContractSet；`off` 路径不加载、不评估、不记录。

## 4. Production Mode Framework

`contract_mode` 支持：

|Mode|Current behavior|
|---|---|
|off|不加载 Contract，不生成 decision，不记录|
|shadow|生成 decision 与脱敏 audit event，不改变 Retrieval|
|compare|并行记录 baseline ownership 与 Contract 差异，不改变 Retrieval|
|production|生产框架占位：生成 decision 与 audit event，不改变 Retrieval|

默认设置与当前实际设置均为 `off`。现有 `data/runtime_settings.json` 没有写入 `contract_mode`，规范化后仍为 `off`。本批没有切换实际运行模式，也没有实现 candidate filtering、suppression enforcement 或 selector behavior。

## 5. Ownership Lane Model

`OwnershipDecision` 已包含：

- `primary_domain`
- `mandatory_secondary_domains`
- `suppressed_domains`
- `ownership_lanes.primary`
- `ownership_lanes.mandatory_secondary`
- `ownership_lanes.suppressed`
- `safety_gate`
- `contract_reason`

同时保留 version、checksum、matched rule、decision status、confidence 与 timestamp。lane 只表达 proposed ownership，不修改候选、分数、顺序或最终 evidence。

## 6. Safety Gate Framework

`SafetyGateDecision` 为只读 `observe_only`：

- Active Hazard：Battery abnormal、invasive battery work、device smoke/overheat、active fire、unknown power action。
- Human Harm：设备事故导致电击伤、烧伤、烟气吸入或其他人员伤害时，Medical primary，必要的 Fire / Energy Safety 为 secondary。

gate 输出 required primary / secondary 与 prohibited action labels，但不执行断电、隔离、撤离、维修或任何危险动作，也不生成安全建议。

Fire negative evidence 边界保持：

- 冒烟但无明火：Energy Safety primary，Fire mandatory secondary。
- 异常热且明确无烟、无燃烧：Energy Safety primary，不强制 Fire secondary。
- 当前明火：Fire primary。
- 已熄灭、现场安全后的复盘：Records primary。

## 7. Audit Event

统一事件前缀为 `cross_domain_contract_audit`。事件包含：

- event / contract version
- contract checksum
- request ID 与 UTC timestamp
- mode、matched rule、decision status
- primary / mandatory secondary / suppressed domains
- safety gate 与 contract reason
- compare baseline ownership / difference type（适用时）
- `query_sha256` 与 `query_length`
- `retention_days=30`
- `retention_class=local_rotating_jsonl`
- `contains_full_query=false`

日志不保存全文 query、normalized query、Prompt、answer 或医疗身份信息。测试使用包含姓名的 query 验证，序列化事件中不存在原 query 或姓名。

## 8. Rollback Drill

测试按以下顺序模拟：

`production -> compare -> shadow -> off`

结果：

- 四个 mode 均可被规范化，无 schema 或数据迁移。
- production / shadow 只产生 decision 与 audit；compare 只产生 comparison 与 audit；off 无 side effect。
- 切换前后 `data/cross_domain_contracts.json` 与 `data/lanternbox.db` 的 size、mtime 均一致。
- production framework 与 off 的完整 `RetrievalV2Result.model_dump()` 相等。

该 drill 没有写入 runtime settings，不代表 production 已启用。

## 9. Validation Results

### Syntax

`py_compile`：PASS。

### Contract + Retrieval Contracts

合并运行：

- 5 个 Cross-Domain Contract test modules
- `tests/test_retrieval_traceability.py`
- `tests/test_retrieval_root_contract.py`

结果：**71 passed / 0 failed / 0 xfailed**。

覆盖：

- v1.1 single source 与 schema rejection
- Active Hazard、Fire negative evidence、Human Harm
- Information Object 与 physical storage
- off / shadow / compare / production mode
- ownership lanes 与 read-only gate
- audit redaction / retention / version
- rollback no-migration
- shadow / compare / production Retrieval output invariance

### v1.1 Revalidation

|Metric|Result|
|---|---:|
|Contract cases|16 / 16|
|Contract pass rate|100%|
|Fire boundary|5 / 5 = 100%|
|Human Harm ownership|5 / 5 = 100%|
|Information Object|6 / 6 = 100%|
|重点 regression|5 / 5|
|False positive|0|
|False negative|0|

### Shadow / Compare Regression

Batch16-I fixture：

- Contract accuracy：12 / 12 = 100%
- A：3
- B：0
- C：9
- D：0
- A cases：E03 controller smoke、E04 seasonal solar trend、W01 repair log archive

Compare 仍只记录 existing Retrieval debt，Retrieval 输出不变。

## 10. Production Enable Boundary

本批完成的是 production readiness framework，不是 production behavior。以下能力仍未启用：

- Contract 修改 domain eligibility
- suppressed domain candidate filtering
- mandatory secondary enforcement
- safety gate fail-closed fallback
- Contract 驱动的 candidate / ranking / selector 变化

因此不存在对 stable Medical、Energy、Records、Storage、Manufacturing、Agriculture 或 Repair 的生产重排。后续 Production Enable Review 应以本批 checksum、71-test baseline、A=3/C=9/D=0 和 rollback drill 为冻结输入，单独决定是否允许行为接入。

## 11. Dirty Files

工作区原有大量 dirty / untracked 文件，本批未清理、提交或回退。

特别说明：

- `data/lanternbox.db` 在本批开始前已经 dirty。
- 本批没有修改数据库内容或 schema。
- rollback drill 证明测试期间该文件 size 与 mtime 未变化。
- `.env`、Wiki、Guide、profiles、FT-02 与其他无关 dirty 文件均未处理。

最终状态：**READY FOR PRODUCTION ENABLE REVIEW**
