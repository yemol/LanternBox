# LanternBox Batch16-H: Cross-Domain Contract Shadow Integration Report

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_g_cross_domain_contract_production_integration_plan.md`
- `docs/knowledge/batch16_f_cross_domain_contract_revalidation_report.md`

## 1. Changed Files

本批新增：

- `api/retrieval_v2/contract_runtime.py`
- `tests/test_cross_domain_contract_shadow_integration.py`
- `docs/knowledge/batch16_h_cross_domain_contract_shadow_integration_report.md`

本批修改：

- `api/config.py`
- `api/models.py`
- `api/routes.py`
- `api/retrieval_v2/orchestrator.py`

未修改：

- `data/cross_domain_contracts.json`
- Retrieval profiles、planner、fetchers、ranking、selector、top_k、fallback
- Prompt 与 generation pipeline
- Wiki、Guide、Guide-Wiki
- PocketBase、database schema、`data/lanternbox.db`

工作区中既有 `data/lanternbox.db`、`data/retrieval_query_profiles.json` 等 dirty 状态未清理、未提交。本批对 contract data、profile、selector、fetchers、planner、Prompt、Guide 和 guide index 只做只读哈希确认。

## 2. Runtime Architecture

实现链路：

```text
user query
  -> existing Retrieval plan build
  -> existing plan/query normalization boundary
  -> record_shadow_decision(query)
       -> normalize_contract_query
       -> evaluate_ownership_contract
       -> OwnershipDecision
       -> structured process log
  -> existing candidate fetch
  -> existing ranking / selector
  -> existing Retrieval result
```

shadow hook 位于 `plan_normalization` 成功后、`fetch_candidates_from_plan` 之前。调用返回值被刻意忽略，不写入 `RetrievalPlan`、candidate、selection、selected evidence、Prompt 或 answer。

### OwnershipDecision

`OwnershipDecision` 是 Pydantic 内存模型，包含全部要求字段：

- `query`
- `normalized_query`
- `contract_version`
- `primary_domain`
- `mandatory_secondary_domains`
- `suppressed_domains`
- `trigger_reason`
- `confidence`
- `timestamp`

query normalization 使用 NFKC、空白折叠和首尾清理。timestamp 使用 UTC ISO-8601。

### Runtime Modes

runtime settings 新增：

```json
{
  "contract_mode": "off"
}
```

只允许：

- `off`: 不计算、不记录，返回 `None`。
- `shadow`: 计算并记录 OwnershipDecision，但不进入 Retrieval state。

当前实际设置为 `off`。非法值由 runtime normalization 降级为 `off`；API request model 只接受 `off|shadow`，`production` 会被 Pydantic 拒绝。本批不存在 production mode 分支。

### Contract Version Boundary

runtime 输出 `contract_version=1.1-shadow`，实现 Batch16-E/F 已复验的 reference semantics，包括：

- Fire negative evidence。
- Human Harm priority。
- Biological Material、Consumable 与 Sample 边界。
- 原有 Battery / unknown power / Records / Storage / Repair 边界。

`data/cross_domain_contracts.json` 按禁止范围仍为 v1.0.0，未被 runtime 修改或加载。生产启用前必须另立 Apply，把 v1.1 语义迁移到权威 contract data 并消除代码/数据双源；shadow 阶段允许用 reference semantics 观测，不得据此进入 production。

## 3. Shadow Output Example

输入：

```text
充电器冒烟但没有看到明火
```

结构化日志中的 OwnershipDecision：

```json
{
  "query": "充电器冒烟但没有看到明火",
  "normalized_query": "充电器冒烟但没有看到明火",
  "contract_version": "1.1-shadow",
  "primary_domain": "energy_safety",
  "mandatory_secondary_domains": ["fire"],
  "suppressed_domains": ["repair", "energy_deepening", "energy_management"],
  "trigger_reason": "energy_device_abnormal: device hazard without unnegated active fire",
  "confidence": 0.98,
  "timestamp": "<UTC ISO-8601>"
}
```

日志 logger：`lanternbox.cross_domain_contract`。

日志只写进现有进程日志；不写 database、PocketBase、Retrieval result 或最终回答。未命中契约时也会记录 `primary_domain=unassigned`、`confidence=0.0`，方便 shadow coverage 统计。

## 4. Test Result

### Compile

以下文件 `py_compile` 通过：

- `api/config.py`
- `api/models.py`
- `api/routes.py`
- `api/retrieval_v2/contract_runtime.py`
- `api/retrieval_v2/orchestrator.py`
- `tests/test_cross_domain_contract_shadow_integration.py`

### Shadow Integration Tests

`tests/test_cross_domain_contract_shadow_integration.py`：**18 passed**。

覆盖：

- Active Hazard：鼓包电池、冒烟设备、明火、电击伤害。
- Information Object：记录归档、物资保存、种子保存、实验数据。
- Regression：Energy Deepening、Energy Safety、Records、Storage、Repair。
- OwnershipDecision 九个字段与 query normalization。
- off 不计算、不记录。
- shadow 输出可解析 JSON 日志。
- `contract_mode` 只接受 off / shadow。
- production 配置在 API model 层被拒绝。
- off / shadow Retrieval result 完全相等。

### Combined Contract and Retrieval Tests

运行：

- Batch16-C contract tests。
- Batch16-D validation tests。
- Batch16-H shadow integration tests。
- retrieval traceability / root contracts。
- Energy Deepening retrieval profile contracts。

结果：**70 passed / 3 xfailed / 0 unexpected fail**。

三条 xfail 是未修改的 v1.0 contract-data baseline：Fire negative、Medical injury、Seed storage。对应 v1.1-shadow integration tests 均已通过；本批没有修改旧 expected 或 xfail。

pytest cache 因工作区权限产生 warning，不影响测试结论。

## 5. Regression Result

|边界|Shadow ownership|Result|
|---|---|---|
|Battery abnormal|Energy Safety|PASS|
|Smoke without fire|Energy Safety + Fire secondary|PASS|
|Active fire|Fire + Energy Safety secondary|PASS|
|Electrical injury|Medical + Energy Safety secondary|PASS|
|Normal capacity degradation|Energy Deepening|PASS|
|Unknown power parallel|Energy Safety|PASS|
|Energy records|Records|PASS|
|Spare parts storage|Storage + Records secondary|PASS|
|Safe de-energized cable inspection|Repair + Energy Safety secondary|PASS|

Retrieval output regression：

- integration test 对同一 mocked Retrieval 分别运行 `off` 和 `shadow`。
- `plan`、candidates、selection、selected evidence、debug、source outputs 和 retrieval decision 的完整 `model_dump()` 相等。
- shadow decision 没有进入 Retrieval output。

受保护内容确认：

- `cross_domain_contracts.json` SHA-256 仍为 `87615f34dc342ed8dacf77cd2a0ae4555264499405cf12d71707910f077f79af`。
- Retrieval profile、selector、fetchers、planner 保持 Batch16-F/G 已冻结哈希。
- Prompt、Guide、guide index 未修改。
- 没有新增 ranking、candidate filter、safety gate、compare 或 production 行为。

## 6. Production Enable Conditions

本批只达到 shadow validation 入口。Production enable 前必须全部满足：

1. 收集 shadow mode 的真实离线 query 样本，确认 match / unassigned / error / latency 分布。
2. shadow validation 中 false positive=0、false negative=0、stable-domain unexpected match=0。
3. 将 v1.1 语义迁移到权威 `cross_domain_contracts.json`，runtime 改为单一数据源。
4. 原 Batch16-D 三条 xfail 在 contract-data tests 中转为 pass，并移除 xfail 标记。
5. 实现独立 compare mode；仍返回 baseline，只比较 proposed ownership 与 candidates。
6. compare mode non-trigger Retrieval diff=0，retained candidate score/order diff=0。
7. 完成 ownership metadata adapter；不得按 Guide ID 或 case ID 硬编码。
8. 实现 candidate lanes 与 safety gate，但保持默认关闭，并通过 integration review。
9. dangerous suggestion=0，Kiwix override=0，Safety/fallback/record-check=100%。
10. rollback drill 能从未来 production 降为 compare、shadow、off。
11. 另行批准 `production` 配置；当前 API model 明确拒绝该值。

在上述条件满足前：

- 不得让 OwnershipDecision 进入 candidate filtering。
- 不得改变 ranking、selector、top_k 或 fallback。
- 不得把 contract trace 放入 Prompt 或 answer。
- 不得把当前 shadow runtime 宣称为 production-integrated contract data。

最终状态：**READY FOR SHADOW VALIDATION**
