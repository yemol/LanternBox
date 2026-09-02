# LanternBox Batch16-J: Cross-Domain Contract Compare Mode Report

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_i_cross_domain_contract_shadow_validation_report.md`
- `docs/knowledge/batch16_g_cross_domain_contract_production_integration_plan.md`

目标：实现 Contract decision 与现有 Retrieval ownership 的只读 Compare Mode。Compare 只记录差异，不改变 candidate、ranking、selector、Prompt 或 answer；本批不进入 production。

## 1. Changed Files

新增：

- `api/retrieval_v2/contract_compare.py`
- `tests/test_cross_domain_contract_compare_mode.py`
- `scripts/validate_cross_domain_contract_compare.py`
- `docs/knowledge/batch16_j_cross_domain_contract_compare_report.md`

修改：

- `api/retrieval_v2/contract_runtime.py`
- `api/retrieval_v2/orchestrator.py`
- `api/config.py`
- `api/models.py`
- `api/routes.py`

复用且未修改：

- `tests/fixtures/cross_domain_contract_shadow_validation_cases.json`

未修改：

- Retrieval fetcher、candidate scoring、ranking、selector、top_k、fallback
- Retrieval profiles、Prompt、generation pipeline
- Wiki、Guide、Guide-Wiki、contract data
- PocketBase、database schema、`data/lanternbox.db`
- `data/runtime_settings.json`

工作区既有 dirty 文件未清理、未提交；本批没有处理 `data/retrieval_query_profiles.json` 或其他无关 dirty 文件。

## 2. Runtime Architecture

实现链路：

```text
existing query normalization
  -> existing Contract evaluator
  -> existing candidate fetch and order
  -> Compare evaluator
       -> Original Retrieval Ownership
       -> Contract OwnershipDecision
       -> Difference Type
       -> structured compare log
  -> existing selector
  -> existing evidence assembly
  -> existing Prompt / answer
```

Compare hook 位于现有 candidate fetch 完成后、selector 调用前。它读取候选顺序中的首个 Guide，产生 ownership observation，但不修改候选对象或把 comparison 写入 Retrieval state。

运行模式现在允许：

- `off`
- `shadow`
- `compare`

仍然拒绝 `production`。默认值保持 `off`，本批没有写入或切换实际 runtime settings。

## 3. Compare Evaluator

### Original Retrieval Ownership

`RetrievalOwnership` 输出：

- `primary_domain`
- `co_domains`
- `source_type`
- `source_id`
- `source_title`
- `source_category`

ownership adapter 只读取现有 Guide 的 title、category 和 domains metadata，不按 fixture case ID 或 Guide ID 改写生产 metadata。

### Contract OwnershipDecision

完整保留 Batch16-H 的字段：

- query / normalized_query
- contract_version
- primary_domain
- mandatory_secondary_domains
- suppressed_domains
- trigger_reason
- confidence
- timestamp

### OwnershipComparison

Compare 输出：

- `original_retrieval_ownership`
- `contract_ownership_decision`
- `difference_type`
- `difference_reason`
- `compare_version`
- query / timestamp

结构化日志前缀：`cross_domain_contract_compare`；logger 仍为 `lanternbox.cross_domain_contract`。

## 4. Difference Types

|Type|Meaning|
|---|---|
|A|Contract identifies existing Retrieval debt|
|B|Retrieval better than Contract|
|C|Both acceptable，包括精确一致、合法 secondary、stable-domain passthrough|
|D|Contract error|

运行时无 ground truth 时，Compare 依据已验证 contract precedence 和兼容边界产生 provisional type。冻结 fixture 验证时，使用 `expected_contract_primary` 与 `accepted_retrieval_primary` 做 adjudication；不修改 expected，也不把 fixture 数据载入生产请求。

## 5. Compare Output Example

query：`太阳能控制器正在冒烟但没有明火，还能继续用吗？`

```json
{
  "original_retrieval_ownership": {
    "primary_domain": "energy_management",
    "source_id": "DG-0617",
    "source_title": "太阳能充电白天排程"
  },
  "contract_ownership_decision": {
    "primary_domain": "energy_safety",
    "mandatory_secondary_domains": ["fire"],
    "trigger_reason": "energy_device_abnormal: device hazard without unnegated active fire"
  },
  "difference_type": "A"
}
```

该 comparison 只进入日志；DG-0617 仍按 baseline candidate / selector 流程处理，用户输出不变。

## 6. Batch16-I Fixture Validation

运行：

```text
python3 scripts/validate_cross_domain_contract_compare.py
```

结果：

|Metric|Result|
|---|---:|
|Total cases|12|
|Contract accuracy|12 / 12 = **100%**|
|A|3|
|B|0|
|C|9|
|D|**0**|

无 fixture adjudication 的 runtime provisional compare 同样得到：

- A = 3
- C = 9
- A case IDs = E03、E04、W01

因此三条 A 不是测试层强行分类，runtime compare 本身能稳定记录相同 old vs Contract 差异。

## 7. Three Existing Retrieval Debts

|Case|Original Retrieval Ownership|Contract OwnershipDecision|Type|
|---|---|---|---|
|E03 控制器冒烟、无明火|Energy Management / DG-0617|Energy Safety + Fire secondary|A|
|E04 太阳能跨季趋势|Energy Management / DG-0617|Energy Deepening|A|
|W01 维修日志归档|Repair / Tools / DG-0839|Records|A|

Compare 正确记录：

- E03：`energy_management -> energy_safety`
- E04：`energy_management -> energy_deepening`
- W01：`repair -> records`

没有调整 DG-0617、DG-0839、profile、ranking 或 selector。本批只把差异变成结构化 observation。

## 8. C Boundaries

九条 C 包括：

- 正常容量下降：Energy Deepening 一致。
- 鼓包电池：Energy Safety 一致。
- 电击烧伤：Medical 一致。
- 普通医疗：Contract unassigned，由 Medical stable domain 正常接管。
- Records 归档与可信度：Records 一致。
- 种子本体保存：Contract Storage，现有 Agriculture Guide；属于 Agriculture / Seed Library / Storage 可接受共域。
- 备件和工坊材料保存：Storage 一致。

Compare 没有把 ordinary-domain passthrough 或合法共域误记成 Contract debt。

## 9. Tests

新增 Compare Mode tests：**7 passed**。

覆盖：

- `compare` mode 可用，`production` 继续被拒绝。
- Batch16-I fixture Contract accuracy 100%。
- A=3、B=0、C=9、D=0。
- 三条 A 的 original / Contract owner 精确匹配。
- Compare output 必需字段。
- structured logging。
- off / shadow 不产生 compare record。
- compare / off Retrieval 完整 `model_dump()` 相等。

运行全部 Contract + Retrieval contracts：

```text
tests/test_cross_domain_ownership_contracts.py
tests/test_cross_domain_contract_validation.py
tests/test_cross_domain_contract_shadow_integration.py
tests/test_cross_domain_contract_compare_mode.py
tests/test_retrieval_traceability.py
tests/test_retrieval_root_contract.py
```

结果：**57 passed / 3 xfailed / 0 unexpected fail**。

3 条 xfail 是既有 v1.0 contract-data baseline，未修改 expected 或 xfail。pytest cache 写入权限 warning 不影响测试结果。

## 10. Retrieval Output Invariance

集成测试分别以 `off` 和 `compare` 运行同一 query、plan、candidate 和 selector result，并比较完整 `RetrievalV2Result.model_dump()`：完全相等。

确认 Compare Mode 没有改变：

- candidate ID、内容、顺序或分数
- selector 输入或输出
- selected evidence / related Wiki
- retrieval decision / debug
- Prompt 或 answer

Compare result 没有加入 API response model；只存在于独立结构化日志。

## 11. Production Boundary

本批没有：

- candidate filtering
- ownership lane
- suppression enforcement
- safety gate enforcement
- ranking / selector change
- production mode 配置或分支

下一阶段 Production Review 应评估：metadata adapter 的覆盖率、compare 日志采样、B/D 人工 adjudication 流程、日志保留与脱敏、延迟预算和 rollback gate。未经 review，不得把 A 类 observation 用于改变 Retrieval 输出。

最终状态：**READY FOR PRODUCTION REVIEW**
