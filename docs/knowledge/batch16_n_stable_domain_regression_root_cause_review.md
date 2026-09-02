# Batch16-N Cross-Domain Contract Stable-Domain Regression Root Cause Review

## 1. Review Scope

本报告严格按 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md` 执行，只审查 Batch16-M 冻结的 5 个 stable-domain test failure，不实施修复，不修改测试、fixture、ownership、Retrieval、Contract、profile、Wiki、Guide 或运行配置。

当前实际 `contract_mode` 保持 `off`。

本轮结论摘要：

|项目|结果|
|---|---|
|Batch16-M stable-domain suite|91 / 96 passed，5 failed|
|冻结失败数|5|
|可在 pre-Batch16 committed baseline 复现|5 / 5|
|Contract runtime 完全 bypass 后仍复现|5 / 5|
|off / shadow / compare 结果一致|5 / 5|
|Batch16 新增 regression|0|
|分类 A|3|
|分类 B|2|
|分类 C-G|0|

## 2. Frozen Failure List

以下清单是本报告唯一审查范围。没有增加、删除或改写 case。

|ID|Domain|Test|Query|Expected|Actual|Failure point|
|---|---|---|---|---|---|---|
|ES-1|Energy Safety|`test_spare_battery_hot_after_storage_prefers_battery_abnormal_isolation`|备用电池放了半年，现在发热还有必要测试吗？|DG-0841 top1；Wiki candidates 非空|DG-0841 top1；Wiki candidates `[]`|Wiki recall 为空|
|ES-2|Energy Safety|`test_battery_leak_prefers_battery_abnormal_isolation`|电池漏液了怎么处理？|DG-0841 top1|DG-0112 top1，DG-0841 top2|Medical / Energy Guide ownership competition|
|MFG-1|Manufacturing|`test_finished_shelf_heavy_load_prefers_load_check`|做好的架子能不能直接放重物？|DG-0877 top2 内；top1 为 DG-0877 或 DG-0874|DG-0874、DG-0875、DG-0877|DG-0877 为 rank 3|
|AGR-1|Agriculture|`test_moldy_wet_seed_stays_in_seed_library_boundary`|种子受潮有霉味，能不能混回种子库？|DG-0879 top1|DG-0897 top1，DG-0879 top2|旧 Agriculture ownership expectation|
|AGR-2|Agriculture|`test_seed_food_batch_separation_prefers_postharvest_or_seed_library`|留种和食用批次怎么分开？|DG-0885 或 DG-0879 top1|DG-0897 top1；DG-0885 top2；DG-0879 top4|旧 Agriculture ownership expectation|

### 2.1 Current Retrieval Evidence

|ID|Triggered profiles|Guide evidence|Wiki evidence|Selector note|
|---|---|---|---|---|
|ES-1|`energy_battery_abnormal_isolation`；同时误触发 `medical_infection_dehydration_fever`|DG-0841、DG-0558、DG-0921、DG-0559、DG-0021、DG-0020、DG-0087、DG-0112|独立 Wiki candidates 为空|测试直接断言 fetcher candidates；没有调用 selector|
|ES-2|`energy_battery_abnormal_isolation`、`medical_poisoning_chemical_exposure`|DG-0112、DG-0841、DG-0921、DG-0064、DG-0901、DG-0924、DG-0108、DG-0210|top1 为 `energy-battery-leak-corrosion-isolation-001`|Guide top1 在 selector 前已经偏向 Medical|
|MFG-1|`manufacturing_connection_load_check`|DG-0874、DG-0875、DG-0877|独立 Wiki candidates 为空|测试在 Guide rank 断言处结束|
|AGR-1|`agriculture_seed_library`、`storage_seed_storage`|DG-0897、DG-0879、DG-0886、DG-0232|top1 为 `agriculture-storage-seed-moisture-quarantine-001`|Storage Guide 与正确 seed-storage Wiki 一致|
|AGR-2|`agriculture_seed_library`、`agriculture_postharvest_storage`、`storage_seed_storage`|DG-0897、DG-0885、DG-0886、DG-0879、DG-0902、DG-0832|top1 为 `agriculture-storage-seed-food-separation-shelf-001`|Storage Guide 与物理储藏对象一致|

“selected Guide / Wiki”在这些 unit tests 中并不等同于生产 selector 输出：测试直接调用 fetchers 并断言候选顺序。上表因此区分 candidate rank 与真正 selector result；未调用 selector 的位置记为不适用，而不是推断一个不存在的 selected result。

## 3. Historical Baseline

### 3.1 Timeline

|Point|Commit / evidence|Meaning|
|---|---|---|
|Energy Safety stable|`c6df74b`，2026-07-14|Energy Safety v0.1 冻结点|
|Manufacturing stable|`4eb8dc6`，2026-07-18|Manufacturing Retrieval v0.1 冻结点|
|Agriculture stable|`baeeab1`，2026-07-18|Agriculture Second Stage v0.1 冻结点|
|Last shared pre-Batch16 baseline|`a796331`，2026-07-18|Long-Term Storage v0.1 stable；晚于上述三个域，早于 Batch16|
|Batch15-F report|pre-Batch16 report|已明确复现 AGR-1：expected DG-0879，actual DG-0897|
|Batch16-C report|Contract data/test apply|记录 Agriculture DG-0879 -> DG-0897 为 existing failure|
|Batch16-H/L|runtime/readiness implementation|引入只读 runtime 与四态 framework，默认 off|
|Current Batch16-M state|working tree|91 / 96 passed，5 failed|

### 3.2 Decisive Pre-Batch16 Reproduction

从 committed tree `a796331` 创建只读临时归档，并用当前 Python test runtime 运行相同 5 个 node。结果为 **5 failed**，且每项 actual 与当前完全一致：

|ID|`a796331` result|Current result|Parity|
|---|---|---|---|
|ES-1|DG-0841 top1，Wiki `[]`|相同|YES|
|ES-2|DG-0112 top1，DG-0841 top2|相同|YES|
|MFG-1|DG-0874、DG-0875；DG-0877 不在 top2|相同|YES|
|AGR-1|DG-0897 top1|相同|YES|
|AGR-2|DG-0897 top1|相同|YES|

`a796331` 中不存在：

- `api/retrieval_v2/contract_runtime.py`
- `data/cross_domain_contracts.json`
- Contract runtime hook

因此，这 5 个失败在 Contract Layer 存在之前已经成立。这个证据比“当前 mode=off”更强，因为历史执行环境根本没有 Contract implementation 可进入。

### 3.3 Original Stable-Point Evidence

在各域自己的初始 stable commit 上，旧 Guide expectation 曾经成立：

|Domain|Stable-point observation|Historical limitation|
|---|---|---|
|Energy Safety|ES-2 到达 DG-0841 Guide expectation；ES-1 也到达 DG-0841|历史 PocketBase/Wiki snapshot 不完整，完整 Wiki 断言为 UNKNOWN|
|Manufacturing|MFG-1 的 DG-0877 top2 Guide assertion 通过|后续 related Wiki 阶段受历史 PocketBase snapshot 限制|
|Agriculture|AGR-1/AGR-2 的旧 DG-0879 / DG-0885 ownership assertion 通过|后续 related Wiki 阶段受历史 PocketBase snapshot 限制|

这说明失败并非这些测试自创建以来从未通过，而是后续 stable-domain expansion 之间发生了语义竞争或 ownership 边界演进。是否 pre-Batch16 则不依赖这些不完整 snapshot：`a796331` 的完整五项复现已经给出确定答案。

## 4. T1-T4 Mode Isolation

### 4.1 Test Matrix

|Run|Setup|Result|Difference from T1|
|---|---|---|---|
|T1|当前实际配置，normalized mode=`off`|0 passed / 5 failed；冻结 actual 全部复现|baseline|
|T2|安装 in-memory import blocker；任何 `api.retrieval_v2.contract_runtime` import 立即报错|0 passed / 5 failed；`contract_runtime_imported=False`|无|
|T3|独立进程内仅将 runtime settings monkeypatch 为 `shadow`|0 passed / 5 failed|无|
|T4|独立进程内仅将 runtime settings monkeypatch 为 `compare`|0 passed / 5 failed|无|

没有写入 runtime settings，实际模式始终为 `off`。

### 4.2 Candidate Snapshot Equality

对五个 query 的 profile、Guide candidate 与 Wiki candidate snapshot 计算摘要：

|Mode|SHA-256|
|---|---|
|off|`2bd9c22d943681b6af947af800accfeb436b4738c4449de28892ed6242e4eeda`|
|shadow|`2bd9c22d943681b6af947af800accfeb436b4738c4449de28892ed6242e4eeda`|
|compare|`2bd9c22d943681b6af947af800accfeb436b4738c4449de28892ed6242e4eeda`|

结论：T1-T4 完全一致。没有 mode-dependent result、order instability 或 cache-dependent drift。

## 5. Batch16 Diff And Off-Mode Side-Effect Audit

### 5.1 Production Path Audit

|Layer|Batch16 behavior|Can explain the five failures?|
|---|---|---|
|Contract source|Batch16-C/L data source；off path 不加载|NO|
|Query normalization|Contract evaluator 内部使用；不回写 Retrieval query|NO|
|OwnershipDecision|只生成 decision/audit|NO|
|Orchestrator shadow hook|plan normalization 后调用，返回值被忽略|NO|
|Compare evaluator|candidate fetch 后读取结果并记录；不改 candidate|NO|
|Fetcher|五个测试直接调用的实现无 Batch16 diff|NO|
|Profiles|Batch16-C 保护 hash 与当前一致；Batch16 未新增 stable-domain profile 变更|NO|
|Ranking / selector / Prompt|Batch16-C-M 报告及 diff 均无变更|NO|

### 5.2 Import And Cache Audit

- `contract_runtime` 模块 import 只定义类型和函数，不在 import time 预加载 ContractSet。
- `off` 路径在 evaluator/source load/audit 前返回。
- 五个失败测试直接调用 fetchers，不经过 orchestrator Contract hooks。
- T2 在 Contract runtime 完全不可 import 时仍得到同一结果。
- T1-T4 candidate snapshot 完全相同。

结论：没有发现 off-mode import side effect、shared dictionary mutation、normalization 回写、cache 污染、test order dependency 或 shadow/compare 干预。

## 6. Energy Safety Root Cause

### 6.1 ES-1 Spare Battery Hot After Storage

根因链：

1. `energy_battery_abnormal_isolation` 正确触发。
2. DG-0841 正确位于 Guide top1。
3. Wiki fetcher 返回空列表，失败发生在 Guide 已正确 ownership 之后。
4. “发热”同时触发 Medical fever profile，说明 generic state token 仍有跨域歧义，但它没有改变本 case 的 Guide top1。
5. 相同空 Wiki 结果在 `a796331`、当前 off、runtime bypass、shadow、compare 全部复现。

判定：这是 pre-Batch16 Energy Safety Wiki recall/evidence production debt，不是 Batch16 test-only failure，也不是 Contract regression。

### 6.2 ES-2 Battery Leak

根因链：

1. Energy abnormal battery profile 正确触发。
2. Medical chemical exposure profile 也因“电池 + 漏液”触发。
3. DG-0112“电池漏液处理”压过 DG-0841“电池异常隔离”。
4. Query 没有皮肤接触、误食、吸入或人员症状，按现有 Active Hazard / Information ownership 语义，Energy Safety 应为 primary，Medical 只应在人身暴露时上升。
5. 目标 Energy Wiki 已在 top1，问题集中于 Guide ownership competition，而不是知识缺失。
6. Batch5 历史报告已记录 DG-0841 在漏液场景的排序不稳定；`a796331` 又完整复现当前 DG-0112 top1。

判定：这是 pre-Batch16 Medical / Energy Safety ownership competition，是 stable-domain production debt。

## 7. Manufacturing Root Cause

### 7.1 MFG-1 Finished Shelf Heavy Load

根因链：

1. `manufacturing_connection_load_check` 正确触发。
2. 正确 Guide DG-0877 已召回，但排在 DG-0874、DG-0875 后，为 rank 3。
3. Batch9-E 已明确记录“制作后承重”场景中 DG-0874 会挤压 DG-0877，并将制作中结构检查与制作后承重检查定义为需要保持的边界。
4. Batch9-F 的目标是 DG-0877 进入前二；当前结果说明该局部排序在之后的共享 baseline 中漂移。
5. `a796331`、runtime bypass 与三种 mode 的结果完全相同。

判定：这是 pre-Batch16 Manufacturing 内部 Guide ranking/evidence debt。测试 expectation 仍符合 Manufacturing 已声明的生产语义，因此不是 fixture 错误。

## 8. Agriculture Root Cause

### 8.1 AGR-1 Moldy Wet Seed

Query 的核心动作是受潮、有霉味、是否混回种子库，属于物理对象隔离、储藏复查与 custody。Long-Term Storage v0.1 已明确：

- 播种可行性、发芽率和生产计划由 Agriculture 主导。
- 种子保存、受潮复查、种食分离和轮换记录由 Storage 主导。

Batch12-D 对同类受潮隔离 query 的 expected 与 actual 都是 DG-0897 primary、DG-0879 secondary。当前结果正好符合后来冻结的 Storage boundary。

判定：生产 Retrieval behavior 符合较新的 stable-domain contract；Agriculture test 仍要求旧 DG-0879 top1，是 pre-Batch16 fixture/ownership expectation debt。

### 8.2 AGR-2 Seed / Food Batch Separation

Query 询问留种与食用批次如何分开，是物理种子分架、标签和储藏 custody。Batch12-F 明确把“种食分离”归入 `storage_seed_storage` / DG-0897，并保留 Agriculture Guides 为 supplemental。当前 DG-0897 top1、DG-0885/DG-0879 后续进入，与这一边界一致。

判定：生产 Retrieval behavior 符合较新的 Long-Term Storage stable boundary；旧 Agriculture top1 assertion 是 pre-Batch16 fixture/ownership expectation debt。

## 9. Per-Failure Answers And Classification

|ID|Pre-Batch16?|Present in committed baseline before C/H/L?|Fails with mode off?|Fails with runtime bypass?|Shadow/compare changes result?|Production behavior or test debt?|Class|Should block Contract production?*|
|---|---|---|---|---|---|---|---|---|
|ES-1|YES|YES，`a796331`|YES|YES|NO|Production Wiki recall debt|A|NO|
|ES-2|YES|YES，`a796331`|YES|YES|NO|Production cross-domain ownership debt|A|NO|
|MFG-1|YES|YES，`a796331`|YES|YES|NO|Production ranking/evidence debt|A|NO|
|AGR-1|YES|YES，`a796331`|YES|YES|NO|Test/fixture stale against Storage stable boundary|B|NO|
|AGR-2|YES|YES，`a796331`|YES|YES|NO|Test/fixture stale against Storage stable boundary|B|NO|

\* “NO”基于 Baseline Parity Gate，含义是不能把它归因于 Batch16 或作为 Batch16 integration regression 阻断项；Absolute Green Gate 仍然失败，见下一节。

分类汇总：

|Classification|Count|Cases|
|---|---:|---|
|A PRE-EXISTING STABLE-DOMAIN DEBT|3|ES-1、ES-2、MFG-1|
|B PRE-EXISTING TEST / FIXTURE DEBT|2|AGR-1、AGR-2|
|C BATCH16 TEST-ONLY REGRESSION|0|无|
|D BATCH16 OFF-MODE RUNTIME SIDE EFFECT|0|无|
|E BATCH16 CONTRACT / NORMALIZATION REGRESSION|0|无|
|F ENVIRONMENT / ORDER / CACHE INSTABILITY|0|无|
|G UNKNOWN — INSUFFICIENT HISTORICAL EVIDENCE|0|无|

## 10. Batch16 Regression Decision

Batch16 新增 regression 数：**0**。

证据链满足 ROOT_CAUSE_FIX_POLICY 的因果要求：

1. 五项在 Contract-free、pre-Batch16 committed tree `a796331` 上逐项复现。
2. 当前 `contract_mode=off` 时逐项复现。
3. Contract runtime 被 import blocker 完全 bypass 时逐项复现。
4. shadow / compare 不改变任何 candidate snapshot。
5. 五个测试直接调用的 fetcher、三个 stable-domain test 文件没有 Batch16 diff。
6. Batch16-C 和 Batch15-F 历史报告已经记录 Agriculture 的相同旧失败。

因此不能用 Contract、normalization、mode framework、audit、compare evaluator 或 Batch16 fixture change 解释这五项失败。

## 11. Absolute Green Gate Versus Baseline Parity Gate

### 11.1 Gate A: Absolute Green

定义：stable-domain suite 必须 96 / 96 passed。

当前结果：**FAIL，91 / 96 passed**。

该 gate 能表达 repository 当前是否绝对全绿，但不能区分 release candidate 引入的新 regression 与已经存在的跨域/fixture debt。

### 11.2 Gate B: Baseline Parity

定义：相对于冻结的 pre-change committed baseline，不能新增 failure、改变 candidate result 或引入 mode-dependent behavior。

当前结果：**PASS**。

|Parity check|Result|
|---|---|
|Pre-Batch16 same five failures|PASS|
|Current off equals Contract-free baseline|PASS|
|Runtime bypass equals off|PASS|
|Shadow equals off|PASS|
|Compare equals off|PASS|
|Batch16 Contract/Retrieval contracts|71 passed|
|Batch16-F matrix|16 passed|
|Rollback drill|10 passed|

### 11.3 Recommendation Without Changing The Gate

本报告不修改 Batch16-M 的 gate。建议后续 Production Enable Review 同时、分别记录两个 gate：

- Absolute Green 保持为 repository health / debt closure 指标，状态仍为 FAIL。
- Baseline Parity 作为判断“Batch16 是否引入 regression”的 release evidence，状态为 PASS。

如果治理规则明确要求 Absolute Green 才能 production enable，则仍需由审查者维持 blocked；但不能把这五项标记为 Batch16 regression，也不应为了开启 Contract 而修改 stable domains。若治理目标是阻止本批新增回归，则证据支持继续 Production Enable Review。

## 12. Production Blocker Judgment

|Question|Decision|
|---|---|
|Contract v1.1 是否造成五项失败？|NO|
|off-mode runtime 是否产生 side effect？|NO|
|shadow/compare 是否改变 Retrieval？|NO|
|是否存在 Batch16 true regression？|NO|
|五项是否都早于 Batch16？|YES|
|是否应在本批修 Energy/Manufacturing/Agriculture？|NO|
|是否可以进入下一次 Production Enable Review？|YES|

五项应登记为 existing debt，而不是在 Batch16-N 执行 Apply：

|Debt register|Cases|Future owner|
|---|---|---|
|Energy Safety Wiki recall / Medical ownership competition|ES-1、ES-2|Energy Safety + Medical cross-domain review|
|Manufacturing finished-product load-check ranking|MFG-1|Manufacturing stable-domain review|
|Agriculture tests stale against Long-Term Storage ownership|AGR-1、AGR-2|Stable-domain fixture/ownership review|

## 13. Unique Next Step

下一步只做一次新的 **Production Enable Review**：以 `a796331` 的五项 frozen failure 和本报告的 A/B 分类作为 pre-existing debt baseline，继续保持实际 `contract_mode=off`，重新裁定 Batch16 production enable。该审查不得顺带修 stable domains、改 fixture 或扩大 Contract behavior。

最终状态：**READY FOR PRODUCTION ENABLE REVIEW**
