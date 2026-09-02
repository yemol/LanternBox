# Batch16-P Cross-Domain Contract Canary Production Report

日期：2026-08-31

范围：首次允许 Cross-Domain Contract 对生产 Retrieval 产生受控 ownership 影响，但仅限三个冻结 transition 的显式 canary。仓库默认仍为 contract_mode=off、contract_canary_enabled=false；本批没有启动持续运行的 production session，也没有进入 Full Production。

## 1. Changed Files

|File|Batch16-P change|
|---|---|
|api/config.py|增加 canary 开关和三个结构化 transition ID；默认 mode 保持 off、canary 保持 false|
|api/models.py|运行设置 API 增加 canary enabled 与 transition scope 字段|
|api/routes.py|公开 canary 配置说明，不改变默认值|
|api/retrieval_v2/contract_runtime.py|扩展结构化 audit event，区分 COMPARE_ONLY 与 CANARY_APPLIED|
|api/retrieval_v2/contract_compare.py|compare audit 记录原 Retrieval primary 与 COMPARE_ONLY|
|api/retrieval_v2/contract_canary.py|新增三个 production ownership lane、Safety Gate fail-closed、selection 约束与审计|
|api/retrieval_v2/fetchers.py|为独立 ownership candidate lane 增加可选 Guide corpus filter；既有调用默认行为不变|
|api/retrieval_v2/orchestrator.py|在候选获取后接入 canary lane，在 selection 后落实 primary/mandatory-secondary；非 canary 路径不变|
|tests/test_cross_domain_contract_canary_production.py|新增 18 项 canary、负边界、安全、审计与 rollback tests|
|scripts/validate_cross_domain_contract_canary.py|新增只读冻结 fixture validator，不写 runtime settings|
|docs/knowledge/batch16_p_cross_domain_contract_canary_production_report.md|本报告|

未修改 Wiki、Guide、Guide-Wiki、Contract JSON、profiles、Prompt、generation、top_k、selector limit、ranking formula、既有 fixture、historical expected、schema 或 PocketBase。

data/lanternbox.db 的既有 dirty 状态未处理、未清理、未同步。data/cross_domain_contracts.json 是此前 Batch16 的 v1.1 单一来源，本批只读使用，没有修改。

## 2. Canary Architecture

运行链路：

Query normalization
-> Contract v1.1 evaluator
-> OwnershipDecision
-> structured transition eligibility
-> primary / mandatory-secondary / suppressed-primary candidate lanes
-> existing selector
-> canary selection ownership constraint
-> related Wiki expansion
-> answer pipeline

只有 contract_mode=production 且 contract_canary_enabled=true 时，eligible transition 才能改变 ownership。其他 production Contract decision 只生成 COMPARE_ONLY decision/audit，不改变 candidate、selection 或 answer。

Contract 只改变 domain ownership 与 candidate lane：

- 未修改 Guide/Wiki score formula。
- 未修改全局 Guide/Wiki ranking algorithm。
- 未修改 Prompt、generation、top_k 或 selector。
- 既有 Retrieval 路径继续使用原 query profiles；canary primary lane 仅在已判定 owner 的过滤 corpus 内执行基础相关性召回，不修改任何 profile 定义或评分公式。

## 3. Canary Eligibility

一次 decision 必须同时满足：

1. 全局 mode 为 production。
2. canary 开关为 true。
3. transition ID 位于结构化 allowlist。
4. Contract matched_rule 属于该 transition 的冻结 rule set。
5. Contract primary_domain 等于 transition target。
6. 原 Retrieval primary 属于该 transition 允许纠正的 source domains。
7. primary owner lane 能召回候选。
8. 若 safety_gate.required=true，至少一个 primary candidate 含 STOP、停用、断开、隔离、撤离、远离、禁止或不可等停止边界。

任一条件不满足均 fail closed 为 COMPARE_ONLY，原 candidates 保持不变。实现中没有 exact query、fixture/test case ID、Guide ID、Wiki slug 或完整字符串 allowlist。

## 4. Three Ownership Transitions

|Transition ID|Contract rule|Target primary|Protected boundary|
|---|---|---|---|
|active_hazard_to_energy_safety|energy_device_abnormal|Energy Safety|Human Harm 不扩入本 canary；无未否定明火时 Fire 不得成为 primary；Safety Gate 必须通过|
|long_term_energy_to_energy_deepening|solar_seasonal_baseline|Energy Deepening|当前危险、当天负载、普通 Energy Management、Records lifecycle 不进入此 transition|
|information_object_to_records|information_lifecycle|Records|当前维修、实体设备、零件、材料与实物 Storage 不进入此 transition|

Suppressed domain 只被移出 primary lane。原候选全部保留并去重，mandatory-secondary 候选优先进入 secondary lane，suppressed evidence 仍可供解释使用。

## 5. A-Class Before / After Comparison

冻结 Batch16-I fixture 共 12 条，结果 12 / 12 passed：3 条 CANARY_APPLIED，9 条 COMPARE_ONLY。

|Case|Before|Canary after|Evidence change|Result|
|---|---|---|---|---|
|E03 控制器冒烟但无明火|Energy Management / DG-0617|Energy Safety / DG-0842；Fire mandatory-secondary / DG-0547|原 DG-0617 仍保留为 suppressed-primary evidence|PASS|
|E04 太阳能跨季趋势|Energy Management / DG-0617|Energy Deepening / DG-0922|原 Management evidence 保留|PASS|
|W01 维修日志按版本归档|Repair / Tools / DG-0839|Records / DG-0916，DG-0917 同域候选|原 Repair evidence 保留|PASS|

三项 ownership_changed=true。冻结 fixture 中其余 E01、E02、M01、M02、R01、R02、S01、S02、W02 没有无理由改变。

## 6. Negative-Boundary Results

|Boundary|Final ownership|Canary behavior|Result|
|---|---|---|---|
|正常电池容量下降|Energy Deepening|未被 Safety 接管|PASS|
|异常热、没有烟、无燃烧|Energy Safety|C1 可应用；Fire 不为 primary|PASS|
|当天太阳能不足并调整负载|Energy Management|不进入 Deepening|PASS|
|设备已断电后检查维修接头|Repair|不进入 Records|PASS|
|备件实体长期保存|Storage|不进入 Records|PASS|
|种植实验记录与发芽率归档|Records|Information Object 正确归 Records|PASS|
|种子本体装袋防潮入库|Agriculture custody，可接受 Physical Storage 共域|不进入 Records|PASS|
|历史设备事故已安全，仅归档|Records|不被 Active Hazard 抢占|PASS|

负边界与配置、审计、回滚测试合计：18 / 18 passed。

## 7. Contract Regression

|Gate|Result|
|---|---:|
|Contract v1.1 fixed matrix|16 / 16 passed|
|Fire boundary accuracy|100%|
|Human Harm ownership accuracy|100%|
|Information Object accuracy|100%|
|Contract regression cases|5 / 5 passed|
|False positive|0|
|False negative|0|
|Shadow Contract accuracy|100%|
|Compare Contract accuracy|100%|
|Difference D|0|
|Contract + Retrieval contract suite|89 passed|

Compare 结果仍为 A=3、B=0、C=9、D=0；A cases 仍是 E03、E04、W01。Canary validator 将这三项转为 CANARY_APPLIED，没有产生新的 D。

## 8. Stable-Domain Absolute Result

按 Batch16-O 相同的八个 stable-domain profile modules 重跑：

|Domain|Absolute result|
|---|---:|
|Energy Management|9 / 9 passed|
|Energy Safety|6 / 8 passed，2 failed|
|Energy Deepening|20 / 20 passed|
|Medical|8 / 8 passed|
|Long-Term Records|22 / 22 passed|
|Long-Term Storage|12 / 12 passed|
|Manufacturing|7 / 8 passed，1 failed|
|Agriculture Second Stage|7 / 9 passed，2 failed|
|Total|91 / 96 passed，5 failed|

Absolute result 没有伪装为全绿。五项 failure node 与 failure signature 均和 Batch16-N/O 冻结清单一致。

## 9. Baseline-Relative Regression

Reference baseline：pre-Batch16 commit a796331。

|Parity check|Result|
|---|---:|
|Frozen baseline failures|5|
|Current failures|5|
|Same failure nodes|5 / 5|
|Same failure behavior|5 / 5|
|New failures|0|
|Batch16-P NEW regression|0|

Production Canary 硬门禁 NEW regression=0：PASS。

默认 off 路径未启用 canary；stable-domain suite 验证了仓库默认行为与冻结 baseline parity。Canary 的 12 条显式模拟验证则单独确认三个受控 production effect。

## 10. Safety Gate Validation

E03 的 Contract decision 为 Energy Safety primary、Fire mandatory-secondary、energy_management/repair/energy_deepening suppressed-primary。

验证结果：

- selected primary 为含明确停用/隔离边界的 Energy Safety Guide。
- safety_boundary_present=true。
- 没有明火不会让 Fire 成为 primary。
- 原 Energy Management evidence 未删除。
- 若 owner lane 无任何含停止边界的候选，canary fail closed 为 COMPARE_ONLY。
- Safety Gate 保持 observe_only，不执行设备控制、自动断电、自动隔离、维修或任何危险动作。
- Human Harm Contract v1.1 matrix 仍为 Medical primary、Safety mandatory-secondary；本批没有把 Human Harm 扩为新的 canary transition。

Safety Gate：PASS。

## 11. Audit Validation

CANARY_APPLIED 与 COMPARE_ONLY 均写入结构化 local audit event。已验证字段：

- event_version
- contract_version=1.1.0
- contract_checksum
- contract_mode
- canary 与 canary_status
- matched_rule / contract_reason
- original_primary_domain
- contract_primary_domain
- mandatory_secondary_domains
- suppressed_primary_domains
- safety_gate
- ownership_changed
- UTC timestamp
- query SHA-256 与 query length
- contains_full_query=false
- retention_days=30
- retention_class=local_rotating_jsonl

日志不保存全文 query。测试确认原 query 不出现在 audit payload。

## 12. Rollback Drill

实际执行了测试内的显式状态序列：

production + canary
-> compare
-> shadow
-> off

|Step|Behavior|Audit / output|
|---|---|---|
|production + canary|E03 ownership 被受控应用|CANARY_APPLIED|
|compare|并行比较，不改 candidate/selection|COMPARE_ONLY|
|shadow|只生成 OwnershipDecision/audit|shadow event|
|off|Contract 不影响 Retrieval|NOT_APPLICABLE|

readiness + canary rollback 子集为 28 / 28 passed。演练前后 cross_domain_contracts.json、runtime_settings.json 与 lanternbox.db 的 size/mtime 快照一致；无需 database migration 或 schema migration。最终运行状态再次读取为：

- contract_mode=off
- contract_canary_enabled=false
- production actually enabled=NO

Rollback：PASS。

## 13. Remaining Historical Debts

|Debt|Classification|Current failure|Disposition|
|---|---|---|---|
|Energy Safety ES-1|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0841 top1 后 Wiki recall 为空|后续 Stable Domain Debt Remediation|
|Energy Safety ES-2|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0112 压过 DG-0841|后续 Energy Safety / Medical ownership review|
|Manufacturing MFG-1|PRE-EXISTING STABLE-DOMAIN DEBT|DG-0877 rank 3|后续 Manufacturing debt remediation|
|Agriculture AGR-1|PRE-EXISTING TEST / FIXTURE DEBT|旧 Agriculture top1 expected 与 Storage stable boundary 冲突|后续 fixture/ownership review|
|Agriculture AGR-2|PRE-EXISTING TEST / FIXTURE DEBT|旧 Agriculture top1 expected 与 Storage stable boundary 冲突|后续 fixture/ownership review|

这些债务没有修改、隐藏、改 expected 或标记为 PASS，也没有归因给 Batch16-P。

## 14. Production Risk Assessment

硬门禁全部满足：

|Condition|Result|
|---|---|
|Only three structured transitions can apply|PASS|
|Outside scope remains compare-only|PASS|
|A-class benefit cases applied|3 / 3|
|Frozen fixture|12 / 12|
|Negative boundaries|8 / 8|
|Safety Gate|PASS|
|Suppressed evidence retained|PASS|
|Contract D / FP / FN|0 / 0 / 0|
|Stable-domain NEW regression|0|
|Rollback|PASS|
|Repository default mode|off|
|Full Production enabled|NO|

剩余生产风险是受控的：

- transition eligibility 依赖 Contract v1.1 rule 与当前 Retrieval ownership metadata；Full Production 前仍应评审真实 canary audit 分布。
- 本批只证明三个冻结 transition，不代表其余 Contract rule 可直接产生 production effect。
- 五项历史 stable-domain debt 仍公开存在，但与本 canary 没有因果新增关系。
- canary 开启必须同时显式设置 production mode、canary enabled 与 transition scope；仓库默认不会自行开启。

结论：三个 canary ownership transition 已满足完整性、安全、基线与回滚门禁，可以进入 Full Production Review，但本批不启用 Full Production。

READY FOR FULL PRODUCTION REVIEW
