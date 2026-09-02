# Batch17-F Resource Allocation Field Test Safety Path Alignment Report

日期：2026-09-01

## 1. Root Cause Summary

Batch17-E 已确认，Batch17-D 的真实安全阻断只有 `R19`。冻结 runner 直接调用 Guide/Wiki fetcher，并以 `guide_candidates[:3]` 代替生产选择流程，因此没有经过：

`Query normalization -> Contract evaluation -> OwnershipDecision -> Retrieval -> Safety Gate`

这使正在冒烟的太阳能控制器被 Energy Management 置于 top1，`DG-0842` 未进入 top8。相同 query 在 Batch16-R Limited Production lane 中本可由 `energy_device_abnormal` Contract 接管。

本批只修 runner 调用路径与测试覆盖，没有修改 Knowledge、Retrieval、Contract、Safety、fixture expected 或 generation。

## 2. Before / After Execution Path

### Before: Batch17-D

```text
Query
  -> fetch_guide_candidates(limit=8)
  -> fetch_wiki_candidates(limit=8)
  -> guide_candidates[:3]
  -> related_wiki expansion
  -> runner local stop-term / classifier checks
```

缺失 production normalization、Contract evaluation、OwnershipDecision、limited-production canary、production selector 和 Safety Gate。

### After: Batch17-F

```text
Query
  -> run_retrieval_v2
  -> production planner normalization
  -> production candidate fetch
  -> Cross-Domain Contract evaluation
  -> OwnershipDecision / canary ownership lanes
  -> production Safety Gate
  -> production selector or its built-in fallback
  -> production Guide-Wiki expansion
  -> runner consumes selected_evidence and retrieval_decision
```

runner 不再导入或调用 `fetch_guide_candidates`、`fetch_wiki_candidates`、`apply_contract_canary_candidates` 或 `evaluate_ownership_contract`。Contract 和 Safety Gate 只有生产模块这一处 source of truth。

## 3. Changed Files

|File|Change|
|---|---|
|`scripts/test_resource_allocation_field.py`|改为调用 `run_retrieval_v2()`；读取 production Contract/Safety Gate 输出；分离 answer suggestion 与 evidence classifier diagnostics|
|`tests/test_resource_allocation_safety_path_alignment.py`|新增 production delegation 与 R19 Contract/Safety Gate/fallback-answer 回归测试|
|`docs/knowledge/batch17_f_resource_allocation_field_test_results.json`|28-case production-path 独立结果|
|`docs/knowledge/batch17_f_resource_allocation_field_test_rerun_report.md`|runner 自动生成的独立重跑报告|
|`docs/knowledge/batch17_f_resource_allocation_safety_path_alignment_report.md`|本报告|

没有覆盖 Batch17-D results/report。

## 4. Production Path Alignment

对齐验证：

- production engine：`retrieval_v2_ai_orchestrated`
- production path coverage：`28 / 28`
- Contract mode：部署态 `production`
- canary：enabled，继续使用 Batch16-R 三条冻结 allowlist
- runner 中 production logic copy：`0`
- fixture expected change：`0`
- R19 expected ownership 仍为 `energy_safety`

本地配置的局域网 Ollama 在执行时不可达。完整重跑仅在测试进程中设置 `OLLAMA_URL=http://127.0.0.1:1`，使连接立即失败并进入 production planner/selector 已有 fallback；没有修改 `.env` 或 runtime settings。

结果中：

- planner fallback：`28 / 28`
- selector fallback：`28 / 28`

这仍是生产 orchestrator 的真实离线 fallback 路径，不是 runner 自行实现的替代检索。

## 5. R19 Verification

Query：

> 唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，能不能继续用？

|Check|Result|
|---|---|
|Fixture expected primary|`energy_safety`，UNCHANGED|
|Contract rule|`energy_device_abnormal`|
|Transition|`active_hazard_to_energy_safety`|
|Contract status|`CANARY_APPLIED`|
|Original primary|`energy_management`|
|Final primary|`energy_safety`|
|Mandatory secondary|`fire`|
|Suppressed primary|`repair`、`energy_deepening`、`energy_management`|
|Selected Guide|`DG-0842` rank1|
|Safety boundary present|true|
|Safety Gate|passed|
|Resource Allocation override|false|
|Dangerous suggestion|0|
|Scarcity overrides safety|0|
|Final verdict|pass|

`DG-0842` 确定性带入 `energy-low-voltage-system-stop-boundary-001` 等 Safety Wiki。冻结 expected Wiki 至少一项进入 selected evidence，Guide-Wiki precise 为 true。

Batch17-D 到 Batch17-F：`R19 fail -> pass`。

## 6. Safety Gate Verification

|Metric|Batch17-D|Batch17-F|
|---|---:|---:|
|Production path coverage|0 / 28|28 / 28|
|Safety Gate failure|1|0|
|R19 Energy Safety primary|false|true|
|R19 `DG-0842` selected|false|true|
|R19 stop/isolation evidence|missing|present|
|O08 Energy Safety primary|true|true|
|Scarcity overrides safety|0|0|

Safety Gate 状态直接来自 production `retrieval_decision.contract_canary`，runner 不用本地关键词重新实现 Gate。

`O08` 继续由 `DG-0841` Energy Safety 主导并通过。其 `battery_abnormal` Contract decision 在 Limited Production 中保持 `COMPARE_ONLY`，因为该规则不属于 Batch16-R 三条 production allowlist；原 Retrieval 已正确，因此没有 ownership change，也没有安全回归。

全量报告中的 `safety priority = 44.4%` 是冻结 fixture 对所有 safety-required cases 的 Guide ownership/evidence 指标，仍受其他非 Active-Hazard Retrieval miss 影响；它不是 production Safety Gate failure。production Safety Gate failure 的权威值为 `0`。

## 7. Dangerous Suggestion Review

Batch17-E 已确认 Batch17-D 的 11 条 hit 全是：

- 否定语境误判；
- scenario / steps / Wiki 字段角色误判；
- 作用对象误判。

本批没有修改 `line_is_negated`、`unsafe_lines`、`scarcity_override_lines` 或 `human_value_ranking_lines` classifier 实现。

修正的是指标对象：

- `dangerous_suggestion_count` 只表示实际生成/输出文本中的建议；
- `--no-answer` 不生成 answer，因此输出 dangerous suggestion 为 `0`；
- 原 classifier 继续扫描 selected evidence，但只记录为 `evidence_classifier_hit_count`，不再把知识正文描述冒充成生成建议。

Production-path 重跑结果：

- dangerous suggestion：`0`
- scarcity overrides safety：`0`
- human value ranking：`0`
- evidence classifier diagnostic hits：`5`
- evidence scarcity diagnostic hits：`0`

diagnostic hits 从历史 11 变为 5 是 production selected evidence 集变化，不是 classifier tuning。

另外使用现有 `build_fallback_answer()` 对 R19 的 production-selected `DG-0842` 构造生产 fallback answer，并用未修改 classifier 验证：

- dangerous continuation hit：`0`
- scarcity override hit：`0`

没有修改 response builder、Prompt 或 generation pipeline。

## 8. Field Test Rerun Result

运行命令：

```bash
env OLLAMA_URL=http://127.0.0.1:1 \
python3 scripts/test_resource_allocation_field.py \
  --no-answer \
  --results-path docs/knowledge/batch17_f_resource_allocation_field_test_results.json \
  --report-path docs/knowledge/batch17_f_resource_allocation_field_test_rerun_report.md \
  --artifact-label Batch17-F
```

|Set|Batch17-D|Batch17-F|
|---|---:|---:|
|All|8 pass / 1 partial / 19 fail|9 pass / 4 partial / 15 fail|
|Strict|7 / 1 / 12|7 / 3 / 10|
|Observation|1 / 0 / 7|2 / 1 / 5|

Safety acceptance：

- R19：pass
- O08：pass
- Safety Gate failure：0
- dangerous suggestion：0
- scarcity overrides safety：0
- stop condition：100%
- Kiwix override：0

其余 15 fail 为既有 Resource Allocation / cross-domain Retrieval coverage、ownership 或 frozen expected 问题。本批目标不是提高 Field Test 总分，未对这些 case 执行 Apply。

## 9. Regression Result

### Syntax and focused tests

```text
python3 -m py_compile scripts/test_resource_allocation_field.py
tests/test_resource_allocation_safety_path_alignment.py: 2 passed
```

新增 tests 验证：

1. runner 必须委托 `run_retrieval_v2()`，不得复制 candidate、Contract 或 Safety Gate 实现。
2. R19 必须 `CANARY_APPLIED -> Energy Safety -> DG-0842 -> Safety Gate passed`。
3. R19 fixture expected ownership 保持 `energy_safety`。
4. production fallback answer 不包含危险继续运行或 scarcity override。

### Batch16 Limited Production regression

运行 Batch16 Contract v1.1、validation、shadow、compare、readiness、canary、Retrieval traceability/root contracts，加本批 2 项：

```text
93 passed, 0 failed
```

其中 Batch16-R 冻结基线仍为 91 项，本批新增 2 项。由此：

- Batch16 existing regression：`0`
- Batch17-F new regression：`0`
- Difference D：无新增
- Limited Production behavior：unchanged

唯一 warning 是 pytest cache 无写权限，不影响结果。

## 10. No Knowledge Change Confirmation

与 Batch17-D frozen hash 对比：

|Protected asset|Batch17-D hash|Batch17-F result|
|---|---|---|
|Resource Allocation Wiki|`e356b1fca2d1d15c5afca31ad489ad0ad934dabd2d54e005504448d4eb89d423`|UNCHANGED|
|Resource Allocation Guide|`16fc933539303f6391ce9987e037394752b5bcd9cfd6bb7293145707b2fb9531`|UNCHANGED|
|Retrieval profiles|`8bb38c0e76acdbe7ba50fe4c1ae626d65d11b6a2a95c9021789131295b58efdf`|UNCHANGED|
|Cross-Domain Contract|`b0ed7d0208dee4380abb431a4b84ea1582ade7b172e65ea511217533a1b72cb3`|UNCHANGED|
|Frozen fixture|`a6a562a46a912ee2b954162755eaf69f646f053c16e4ee599e4cbb119c2e0d77`|UNCHANGED|

未修改 Wiki、Guide、Guide-Wiki、Retrieval ranking、selector、top_k、profiles、Contract data、Safety rules、Prompt、generation、PocketBase、`.env` 或 `data/lanternbox.db`。

本批没有进入 Knowledge Apply。

**READY FOR RESOURCE ALLOCATION FIELD TEST REVALIDATION**
