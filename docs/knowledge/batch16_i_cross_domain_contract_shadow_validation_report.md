# LanternBox Batch16-I: Cross-Domain Contract Shadow Validation Report

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_h_cross_domain_contract_shadow_integration_report.md`
- `docs/knowledge/batch16_g_cross_domain_contract_production_integration_plan.md`

本批只验证 Contract Layer 在现有 Guide 候选检索路径中的 shadow decision。没有启用 production mode，也没有把 OwnershipDecision 写入 candidate、ranking、selector、Prompt 或 answer。

## 1. Test Scope

新增只读验证资产：

- `tests/fixtures/cross_domain_contract_shadow_validation_cases.json`
- `scripts/validate_cross_domain_contract_shadow.py`
- `docs/knowledge/batch16_i_cross_domain_contract_shadow_validation_report.md`

fixture 在首次执行前冻结，共 12 条，覆盖：

- Energy：容量下降、鼓包电池、控制器冒烟、太阳能长期趋势。
- Medical：设备事故导致人员受伤、普通医疗问题。
- Records：实验数据归档、设备历史记录。
- Storage：种子保存、备件保存。
- Manufacturing / Workshop：维修日志归档、材料保存。

验证链路：

```text
frozen query
  -> evaluate_ownership_contract(query)
  -> OwnershipDecision（仅观测）

same frozen query
  -> fetch_guide_candidates(limit=8)
  -> existing Retrieval top Guide
  -> test-only ownership mapping

OwnershipDecision primary <-> Retrieval primary comparison
```

runner 没有设置全局 `contract_mode`，而是直接调用 shadow evaluator；现有 Retrieval 继续独立运行。这样可验证 Contract 判断，同时保证它不能影响候选结果。

## 2. Classification Rules

|标记|含义|计入有效冲突|
|---|---|---|
|MATCH|Contract primary 与 Retrieval primary 精确一致|否|
|A|Contract 正确，Retrieval primary 不符合冻结 ownership|是|
|B|Retrieval 正确，Contract primary 不符合冻结 ownership|是|
|C|双方均可接受，或 Contract 明确 `unassigned` 并由 stable domain 正常接管|否|
|D|Contract 错误，且 Retrieval 也不符合冻结 ownership|是|

`agreement rate` 使用 `MATCH + C`；`conflict rate` 使用 `A + B + D`。这避免把 ordinary-domain passthrough 和已设计的共域边界误算为 Contract 冲突。

## 3. Metrics

|Metric|Result|
|---|---:|
|Total cases|12|
|Contract expectation accuracy|12 / 12 = **100%**|
|Retrieval fixture ownership accuracy|9 / 12 = **75%**|
|Exact agreement|7 / 12 = **58.33%**|
|Accepted agreement|9 / 12 = **75%**|
|Effective conflict|3 / 12 = **25%**|
|MATCH|7|
|A: Contract correct|3|
|B: Retrieval correct|0|
|C: Both acceptable|2|
|D: Contract wrong|0|

Contract primary 分布：

- Energy Deepening：2
- Energy Safety：2
- Medical：1
- Records：3
- Storage：3
- Unassigned passthrough：1

Retrieval top ownership 分布：

- Energy Deepening：1
- Energy Safety：1
- Energy Management：2
- Medical：2
- Records：2
- Storage：2
- Agriculture：1
- Repair / Tools：1 个 co-domain top Guide

## 4. Case Results

|ID|Scenario|Contract primary|Retrieval primary / top Guide|Class|
|---|---|---|---|---|
|E01|容量下降|Energy Deepening|Energy Deepening / DG-0921 电池实际容量下降复查|MATCH|
|E02|鼓包电池|Energy Safety|Energy Safety / DG-0841 电池异常隔离|MATCH|
|E03|控制器冒烟、无明火|Energy Safety + Fire secondary|Energy Management / DG-0617 太阳能充电白天排程|A|
|E04|太阳能跨季长期趋势|Energy Deepening|Energy Management / DG-0617 太阳能充电白天排程|A|
|M01|电击后烧伤|Medical + Energy Safety secondary|Medical / DG-0560 烧烫伤冷却与覆盖|MATCH|
|M02|普通发热咳嗽|Unassigned|Medical / DG-0558 发热观察、补水和记录|C|
|R01|种植实验数据归档|Records + Agriculture secondary|Records / DG-0707 判断外部信息可信度|MATCH|
|R02|设备历史与版本归档|Records|Records / DG-0914 重复问题与经验提取|MATCH|
|S01|种子本体保存|Storage + Records secondary|Agriculture / DG-0885 收获后晾晒防霉与储藏|C|
|S02|备件保存|Storage + Records secondary|Storage / DG-0895 储藏区基础分区与标签|MATCH|
|W01|维修日志版本归档|Records|Repair / Tools / DG-0839 刀锯作业前清场|A|
|W02|工坊材料入库|Storage + Records secondary|Storage / DG-0899 工具和材料防潮防锈储藏|MATCH|

## 5. Conflict Review

### E03: Active Hazard vs Energy Management

Contract 正确识别“控制器 + 冒烟 + 没有明火”：

- primary：Energy Safety。
- mandatory secondary：Fire。
- Fire negative evidence 只阻止 Fire primary，不压制设备异常。

现有 Retrieval top1 为 DG-0617 当天太阳能充电排程，属于 Energy Management 抢 Active Hazard 主位。分类为 A，不是 Contract false positive。

### E04: Energy Deepening vs Energy Management

Contract 正确识别“冬天 + 跨季 + 长期基线”为 Energy Deepening。现有 Retrieval top1 仍为 DG-0617 当天排程。分类为 A，说明正常长期趋势已被 Contract 保护，但 baseline Retrieval 仍存在 Management ownership competition。

### W01: Records vs Repair / Tools

Contract 正确识别“维修日志 + 版本 + 归档”为 Information Object lifecycle，primary 为 Records。现有 Retrieval top1 为 DG-0839 刀锯作业前清场。分类为 A，属于 Repair / Tools 对记录对象主位的旧入口偏移。

三条 A 类均适合进入 compare mode 继续观测。它们不能在本批通过 ranking、profile、fixture 或 Contract runtime 调整来消除。

## 6. Active Hazard Validation

Active Hazard / Human Harm 三条 Contract 判断全部正确：**3 / 3 = 100%**。

- 鼓包发烫电池：Energy Safety primary，Fire secondary。
- 控制器冒烟且无明火：Energy Safety primary，Fire secondary。
- 电击并烧伤：Medical primary，Energy Safety secondary。

边界结论：

- Safety、Fire、Medical precedence 在 Contract side 稳定。
- Fire negative evidence 没有把“冒烟但无明火”错误降为普通 Energy Management。
- Human Harm override 正常。
- Retrieval 对 E03 仍有一个 A 类 ownership debt，但不构成 Contract revision 理由。

## 7. Information Object Validation

Records / Storage 六条 Contract 判断全部正确：**6 / 6 = 100%**。

- 实验数据、历史记录、维修日志的归档/版本/可信度意图：Records primary。
- 种子本体、备用零件、工坊材料的保存/入库意图：Storage primary。
- S01 由 Agriculture top1 处理收获后种子储藏，属于 Storage / Seed Library / Agriculture 可接受共域，分类 C。
- W01 的 Repair / Tools top1 不满足信息对象主位，分类 A。

Information Object Contract 能稳定区分“信息生命周期”和“物理对象 custody”，未发现 Contract 把物资保存误吞为 Records，或把归档任务误吞为 Storage。

## 8. Energy Deepening Protection

Energy Deepening Contract 判断：**2 / 2 = 100%**。

- 正常容量下降由 DG-0921 保持 Deepening top1，MATCH。
- 太阳能跨季基线由 Contract 判为 Deepening，但 baseline Retrieval 为 Management，A。

鼓包、冒烟没有被 Deepening 吞掉；普通容量下降没有被 Safety 吞掉。Energy Deepening 的 Contract 边界成立。

## 9. Regression Check

执行：

```text
python3 -m py_compile scripts/validate_cross_domain_contract_shadow.py
python3 scripts/validate_cross_domain_contract_shadow.py
```

runner 编译通过，12 条 case 完整执行。

执行现有 Contract 与 Retrieval contracts：

```text
tests/test_cross_domain_ownership_contracts.py
tests/test_cross_domain_contract_validation.py
tests/test_cross_domain_contract_shadow_integration.py
tests/test_retrieval_traceability.py
tests/test_retrieval_root_contract.py
```

结果：**50 passed / 3 xfailed / 0 unexpected fail**。

3 条 xfail 是 Batch16-H 已记录、未修改的 v1.0 contract-data baseline；v1.1-shadow runtime tests 保持通过。pytest cache 目录产生权限 warning，不影响测试结果。

本批未修改：

- `api/retrieval_v2/contract_runtime.py`
- Retrieval output logic、fetchers、profiles、ranking、selector、top_k、fallback
- Prompt、Wiki、Guide、Guide-Wiki
- contract data、PocketBase、database

未启用 production mode，未改变全局 runtime mode。

## 10. Recommendation

Contract side：

- 12 / 12 符合冻结 ownership 预期。
- Active Hazard 3 / 3。
- Information Object 6 / 6。
- Energy Deepening protection 2 / 2。
- B = 0，D = 0，没有发现 Contract wrong 或 stable-domain false takeover。

Baseline Retrieval side 的 3 条 A 类分歧正是 compare mode 需要持续收集的数据。下一阶段可在仍不改变输出的前提下，把 Contract proposed ownership 与 baseline candidate / selected ownership 做结构化 compare；不得据此直接进入 production filtering。

最终状态：**READY FOR COMPARE MODE**
