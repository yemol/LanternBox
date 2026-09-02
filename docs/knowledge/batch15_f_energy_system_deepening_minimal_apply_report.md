# LanternBox Batch15-F: Energy System Deepening Minimal Apply Report

生成日期：2026-08-25

依据：

- docs/engineering/ROOT_CAUSE_FIX_POLICY.md
- docs/knowledge/batch15_c_energy_system_deepening_apply_verification_field_test_freeze.md
- docs/knowledge/batch15_d_energy_system_deepening_field_test_report.md
- docs/knowledge/batch15_e_energy_system_deepening_root_cause_review.md

## 0. Apply Decision

Batch15-F 已完成允许范围内的最小 Apply：一个长期容量 profile、O03/O04/X-B 测试 ownership、三条 fixture accepted contract、safety classifier 否定语境和 contract tests。

本批直接目标均生效，但完整验收尚未通过：O01、O05、O07、O08 仍未保持要求的 Safety / Records ownership。它们需要修改既有 Safety/Records profile 或 production candidate/ranking 行为，而 Batch15-F 明确禁止这些改动。因此当前状态为：

**BLOCKED FOR BATCH15-G FINAL VERIFICATION**

不进行第二轮 tuning，不越权修改 stable profiles。

## 1. Changed Files

本批有意修改：

- data/retrieval_query_profiles.json
- scripts/test_energy_system_deepening_field.py
- tests/fixtures/energy_system_deepening_field_cases.json

本批新增：

- tests/test_energy_system_deepening_retrieval_profiles.py
- docs/knowledge/batch15_f_energy_system_deepening_field_test_results.json
- docs/knowledge/batch15_f_energy_system_deepening_field_test_report.md
- docs/knowledge/batch15_f_energy_system_deepening_minimal_apply_report.md

必需审计刷新了当日 Wiki / Guide audit artifact。未修改 Wiki、Guide、Guide-Wiki、related_wiki、guide_links、Retrieval Pipeline、Prompt、top_k、selector limit、global ranking、fallback、schema、PocketBase、FT-02、Core、firmware 或 hardware。

受保护哈希确认：Energy Wiki/Guide 99/99 未变；emergency_guides、guide_index、data/lanternbox.db、PocketBase DB 和 Batch15-C freeze report 5/5 未变。

## 2. Profile Change

新增且仅新增一个 profile：

    energy_long_term_capacity_planning

职责：

- 长期或跨月容量盘点。
- Generation / Storage / Conversion / Load 保守模型。
- 持续负载与峰值负载档案。
- 只解决 DG-0919 candidate recall。

保护：

- “今天先关哪些设备”不触发，E03 继续由 DG-0845 主导。
- 鼓包电池、冒烟火灾、USB 维修、220V 配电和 Records 归档均不触发。
- 不新增 Solar、Backup、Measurement 或 Safety profile。

结果：E07、E08 均从 DG-0919 不在 candidate 变为 DG-0919 top1 / PASS。

## 3. Ownership Changes

只增加三条测试侧 override，不修改生产 Guide metadata：

|Case|Guide|测试 ownership|
|---|---|---|
|O03|DG-0859|Communication / Energy Management|
|O04|DG-0563|Repair / Tools co-primary|
|X-B|DG-0667|Records / Repair / Risk Decision|

O03、O04 从 fail 变为 pass。X-B 从 fail 变为 partial；top1 ownership 已合理，但 Records trend Wiki 仍未进入，不在本批继续调整。

## 4. Safety Classifier Changes

否定语境新增：

- 不与
- 避免
- 不提供

原有“禁止”“不要”等规则保留。没有删除或放松任何 stop_or_escalate。

单字动作“接”改为带词边界检查，不再误命中“接头”，仍可命中接线、接入、接在和并接等真实动作。

## 5. Fixture Changes

fixture 保持 28 formal + 3 extra，query 数量和其他 case 均不变。只修改：

- O03：允许 DG-0859，增加 Communication / Energy co-primary。
- O04：冻结 DG-0563 为测试侧 Repair / Tools co-primary。
- X-B：允许 DG-0667，增加 Risk Decision co-primary。

未修改 O01、O05、O07、O08 或任何 strict expected。

## 6. Contract Tests

新增 tests/test_energy_system_deepening_retrieval_profiles.py，共 20 个测试实例，覆盖：

1. 只注册一个 long-term capacity profile。
2. E07/E08 型 query 必须由 DG-0919 top1。
3. 当天分配保持 Energy Management。
4. 明确鼓包隔离 query 保持 DG-0841 Safety top1。
5. profile 不触发电池异常、火灾、维修、市电或 Records。
6. 不与 / 避免 / 不提供 / 禁止 / 不要不触发危险计数。
7. 未知源并联、市电反送和高压接线的真实允许语句仍触发危险计数。
8. ownership override 只覆盖 O03/O04/X-B。

验证：

- Batch15-F contract + retrieval traceability/root contract：29 passed。
- py_compile：通过。
- 仓库全量 pytest 已运行，但未通过；首个可复现失败为 tests/test_agriculture_second_stage_retrieval_profiles.py::test_moldy_wet_seed_stays_in_seed_library_boundary，期望 DG-0879、实际 DG-0897。新 profile 不触发该 Agriculture query，本批未处理该无关既有失败。

## 7. Before / After Field Test

为保留 Batch15-D 基线，本次使用独立 Batch15-F artifact 路径运行同一 runner。

|指标|Batch15-D Before|Batch15-F After|
|---|---:|---:|
|Full pass / partial / fail|9 / 2 / 17|16 / 2 / 10|
|Strict pass / partial / fail|7 / 2 / 11|12 / 2 / 6|
|Observation pass / partial / fail|2 / 0 / 6|4 / 0 / 4|
|Expected Guide hit|70.0%|80.0%|
|Expected top1|55.0%|65.0%|
|Accepted top1|65.0%|70.0%|
|Wiki any / full|60.0% / 45.0%|70.0% / 55.0%|
|Guide-Wiki precise|60.0%|70.0%|
|Energy Deepening primary|63.2%|68.4%|
|Cross-domain reasonable|53.6%|64.3%|
|Energy Management ownership|50.0%|100.0%|
|Repair ownership|0.0%|100.0%|
|Safety ownership|28.6%|28.6%|
|Records ownership|0.0%|0.0%|
|Dangerous suggestion|7|0|
|Kiwix override|0|0|
|Safety / fallback / record-check|100 / 100 / 100%|100 / 100 / 100%|
|Professional / low-voltage / measurement / battery|66.7 / 66.7 / 75 / 100%|66.7 / 75 / 75 / 100%|

成功变化：

- E07、E08：fail -> pass。
- E13、E14、E15：fail -> pass。
- O03、O04：fail -> pass。
- X-B：fail -> partial，ownership 已恢复。

## 8. Dangerous False Positive Resolution

Batch15-E 识别的 7 个 false positive 已全部清除：

- E02：不与未知电源并联，不再误判。
- E13/E14/E15：“避免临时并联、反送”和“不提供高压切换方法 / 接头修理”不再误判。
- E13、E14、E15 现均为 PASS。

After Field Test 的所有零容忍计数均为 0：high voltage、mains wiring、battery disassembly、cell replacement、BMS modification、unknown-source parallel、backfeed、current-range parallel 和 swollen-battery continue-test。

真实危险保护未下降：contract tests 中“可以未知源并联”“允许市电反送”“可以高压接线操作”全部仍被 dangerous_suggestion 检出。

## 9. Safety Regression Check

|验收项|结果|说明|
|---|---|---|
|Wiki audit 0 / 0 / 0|PASS|1157 Markdown / 1157 PocketBase|
|Guide audit 0 / 0 / 0|PASS|851 Guides|
|Dangerous suggestion=0|PASS|false positive 已清除|
|真实危险仍 fail|PASS|3 个正向危险 contract 全部命中|
|E13/E14/E15 pass|PASS|DG-0923、安全边界和 precise evidence 保持|
|E03 Energy Management|PASS|DG-0845 top1|
|O03 Energy Management / Communication|PASS|DG-0859 accepted co-primary|
|O04 Repair|PASS|测试侧 DG-0563 Repair / Tools co-primary|
|O02/O06 Safety|PASS|保持原结果|
|O01/O07 Safety ownership|FAIL|DG-0921 仍 top1；需既有 battery safety profile/ranking 调整，当前禁止|
|O05 Safety ownership|FAIL|DG-0564 仍 top1；需既有 low-voltage/fire profile trigger 调整，当前禁止|
|O08 Records ownership|FAIL|DG-0900 仍 top1；需既有 Records profile trigger 调整，当前禁止|

## 10. Remaining Blockers

未通过项都不属于本批允许的四个 Apply 点：

- O01/O07：Energy Deepening 吞 Battery Safety。
- O05：当前冒烟被 Solar Management 吞域。
- O08：长期归档与可信度未进入 Records candidate。
- 全量 pytest 存在无关 Agriculture 既有失败。

在“不修改既有 stable profiles、不修改 production ranking、不修改其他 fixture case”的约束下，无法合法达到这些验收项。本批不扩大范围、不继续 tuning。

结束状态：**BLOCKED FOR BATCH15-G FINAL VERIFICATION**。
