# Batch16-R Cross-Domain Contract Limited Production Final Report

日期：2026-09-01

## 1. Final Production Scope

本批正式启用并冻结 **Cross-Domain Contract Limited Production v1.0**。运行表达保持既有四模式模型：`contract_mode=production`，并通过 validated canary allowlist 仅开放三条结构化 transition。没有创建第五种 mode，也没有启用 Full Production。

部署配置最终状态：

- `contract_mode=production`
- `contract_canary_enabled=true`
- allowlist 仅含 LP1、LP2、LP3 对应的三个 transition ID
- 仓库代码安全默认仍为 `contract_mode=off`、canary disabled

## 2. Changed Files / Configuration

|File|Batch16-R change|
|---|---|
|`api/retrieval_v2/contract_runtime.py`|Limited Production audit event 增加 `limited_scope`、`rule_family`、`fallback_used`|
|`api/retrieval_v2/contract_canary.py`|CANARY_APPLIED / COMPARE_ONLY 写入上述审计字段|
|`tests/test_cross_domain_contract_canary_production.py`|新增 Full Expansion 防逃逸测试；补充审计断言；测试不再假设部署配置必须为 off|
|`tests/test_cross_domain_contract_production_readiness.py`|明确验证代码默认 off，而不覆盖 deployment runtime state|
|`data/runtime_settings.json`|部署态启用 production + validated limited scope；保留既有未知配置字段|
|`docs/knowledge/batch16_r_cross_domain_contract_limited_production_final_report.md`|本冻结报告|

未修改 Wiki、Guide、Guide-Wiki、profiles、ranking、selector、top_k、Prompt、generation、Contract JSON、PocketBase 或数据库 schema。`data/lanternbox.db` 的既有 dirty 状态未处理。

## 3. Three Frozen Production Transitions

|ID|Transition ID|Structured rule|Production effect|
|---|---|---|---|
|LP1|`active_hazard_to_energy_safety`|`energy_device_abnormal` / active_hazard family|Active Hazard -> Energy Safety|
|LP2|`long_term_energy_to_energy_deepening`|`solar_seasonal_baseline` / long_term_energy family|Long-Term Energy -> Energy Deepening|
|LP3|`information_object_to_records`|`information_lifecycle` / information_object family|Information Object -> Records|

Eligibility 继续由 rule ID、rule family、原 ownership 与目标 ownership 共同判定。实现中没有 exact query、fixture ID、Guide ID、Wiki slug 或 case-specific production hardcode。

## 4. Non-Production v1.1 Rules

除 LP1-LP3 外，其余 v1.1 rules 仅允许 evaluator、OwnershipDecision、compare 与 audit，不得产生 production ownership effect。Human Harm expansion、Fire full expansion、Biological Material、Consumable、Sample、未完整验证的 cross-contract precedence 与 candidate readiness 均保持非生产范围。

Limited runtime 对未获 allowlist 的 decision 统一保持 `COMPARE_ONLY` 和原 Retrieval candidates。

## 5. Production Ownership Results

最终 deployment 配置下重新执行：

|Case|Before|After|Status|Evidence loss|
|---|---|---|---|---:|
|控制器冒烟但无明火|Energy Management|Energy Safety|CANARY_APPLIED|0|
|太阳能跨季长期基线|Energy Management|Energy Deepening|CANARY_APPLIED|0|
|维修日志按版本归档|Repair|Records|CANARY_APPLIED|0|

三项均 `ownership_changed=true`。Batch16-I limited fixture 结果为 12 / 12：3 CANARY_APPLIED、9 COMPARE_ONLY；Difference D=0。

## 6. Negative Boundaries

|Boundary|Final ownership|Limited behavior|Result|
|---|---|---|---|
|正常容量下降|Energy Deepening|不被 Safety 接管|PASS|
|当天太阳能不足 / 调负载|Energy Management|不进入 Deepening|PASS|
|设备断电后正在维修|Repair|不进入 Records|PASS|
|备件本体保存|Storage|不进入 Records|PASS|
|种子本体防潮保存|Agriculture custody|不进入 Records|PASS|
|历史事故已安全、仅归档|Records|不被当前 Hazard 抢占|PASS|
|设备异常热、无烟、无燃烧|Energy Safety|Fire 不为 primary|PASS|

补充生产验证“控制器冒烟但无明火”为 Energy Safety primary、Fire mandatory-secondary，不发生 Fire primary 误升级。全部边界 evidence loss=0。

## 7. Full-Production Escape Test

新增独立回归证明 Batch16-Q 两个 Full Expansion D case 无法逃出 limited allowlist：

|Case|Contract proposal|Runtime result|Existing Retrieval preserved|
|---|---|---|---|
|历史起火已熄灭、现场安全、询问重新通电|Fire|COMPARE_ONLY|Energy Safety，YES|
|农业样本低温保存与交接|unassigned|COMPARE_ONLY|Records，YES|

两项均 `ownership_changed=false`、candidate sequence 不变、evidence loss=0。它们没有被修复或改写 expected。

## 8. Safety Gate

LP1 的 `active_hazard` Safety Gate 保持生产级约束：Energy Safety primary lane 必须召回含停用、断开、隔离、撤离、远离、禁止或不可等停止边界的 evidence；否则 fail closed 为 COMPARE_ONLY。控制器冒烟案例 `safety_boundary_present=true`。

普通 Energy 优化不得抢 primary；negative fire evidence 不会将 Fire 提升为 primary。Gate 仍为只读 `observe_only`，不会执行断电、隔离、维修或其他设备动作。

## 9. Evidence Preservation

Suppressed-primary 只禁止对应 domain 成为 primary，不删除其 evidence。LP1-LP3 及所有负边界、防逃逸案例的原 candidate ID 集均被保留：

- evidence loss count：0
- LP1 原 Energy Management evidence：保留
- LP1 Fire mandatory-secondary lane：保留
- LP2 原 Energy Management evidence：保留
- LP3 原 Repair evidence：保留

## 10. Audit Validation

Limited ownership change 会生成现有脱敏结构化 audit event。实测字段：

|Field|Validated value / behavior|
|---|---|
|`contract_version`|1.1.0|
|`contract_mode`|production|
|`limited_scope`|true for CANARY_APPLIED|
|`rule_family`|由单一 Contract JSON 的 matched rule 导出|
|`original_primary_domain` / `contract_primary_domain`|记录 before / after|
|`mandatory_secondary_domains` / `suppressed_primary_domains`|完整记录 ownership lanes|
|`safety_gate`|完整只读 gate decision|
|`fallback_used`|false for applied；true for compare-only fallback|
|`ownership_changed`|true only for allowed production effect|
|`timestamp`|UTC timestamp|

Audit 仅保存 SHA-256 query identifier 与长度，`contains_full_query=false`，不保存全文 query；retention 与 event version 保持 v1.1 单一来源定义。

## 11. Rollback Drill

真实修改 deployment runtime configuration 并逐阶段验证：

`Limited Production -> compare -> shadow -> off -> Limited Production`

|Stage|Observed behavior|
|---|---|
|Limited Production|三项 allowlisted ownership effect 生效|
|compare|只记录 difference，candidates 不变|
|shadow|只生成 OwnershipDecision / audit|
|off|不生成 Contract runtime decision，Retrieval 恢复原路径|
|Limited Production re-enable|三条 transition 再次生效|

演练无需 database migration、schema migration、knowledge rebuild 或状态迁移。`data/cross_domain_contracts.json` 与 `data/lanternbox.db` 的 size、mtime、SHA-256 均在演练前后不变。最终部署配置已重新启用 Limited Production。

## 12. Contract Regression

|Gate|Result|
|---|---:|
|Contract v1.1 fixed matrix|16 / 16 passed|
|Fire boundary accuracy|100%|
|Human Harm ownership accuracy|100%|
|Information Object accuracy|100%|
|Contract regression subset|5 / 5 passed|
|False positive / false negative|0 / 0|
|Limited fixture|12 / 12 passed|
|Focused Limited / readiness tests|30 passed|
|Contract + Retrieval contract suite|91 passed|
|Difference D in Limited scope|0|

最终回归在 deployment `production + limited allowlist` 状态下完成。唯一 warning 是 pytest cache 无写权限，不影响测试结果。

## 13. Stable-Domain Absolute Result

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

Absolute result 未伪装为全绿。

## 14. Baseline-Relative Regression

Reference：pre-Batch16 commit `a796331`。

|Metric|Result|
|---|---:|
|Frozen baseline failures|5|
|Current failures|5|
|Same failure nodes / signatures|5 / 5|
|Batch16-R NEW regression|**0**|

Baseline parity：PASS。Limited Production 没有引入新的 stable-domain regression。

## 15. Historical Debt Register

|Debt|Classification|Status|
|---|---|---|
|ES-1：DG-0841 top1 后 Wiki recall 为空|PRE-EXISTING STABLE-DOMAIN DEBT|保留，未修|
|ES-2：DG-0112 压过 DG-0841|PRE-EXISTING STABLE-DOMAIN DEBT|保留，未修|
|MFG-1：DG-0877 rank 3|PRE-EXISTING STABLE-DOMAIN DEBT|保留，未修|
|AGR-1：旧 expected 与 Storage boundary 冲突|PRE-EXISTING TEST / FIXTURE DEBT|保留，未改 expected|
|AGR-2：旧 expected 与 Storage boundary 冲突|PRE-EXISTING TEST / FIXTURE DEBT|保留，未改 expected|

Totals：Energy Safety x2、Manufacturing x1、Agriculture x2。

## 16. Full-Production Debt Register

Batch16-Q 的 Full Expansion D=2 继续登记：historical fire + re-energize，以及 agriculture sample custody。另保留以下 readiness debt：

- full candidate readiness
- mandatory-secondary readiness
- fallback completeness
- full audit coverage
- full rollback coverage
- 未完整验证的 Human Harm / Fire / Biological Material / Consumable / Sample expansion 与 precedence

这些均属于 **Cross-Domain Contract Full Production Expansion Debt**，不是 Limited Production blocker，本批未修。

## 17. Freeze Manifest

正式冻结：

- LP1 Active Hazard -> Energy Safety
- LP2 Long-Term Energy -> Energy Deepening
- LP3 Information Object -> Records
- v1.1 Contract source schema 与 checksum `18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`
- OwnershipDecision / ownership lane model
- read-only Safety Gate behavior
- redacted audit schema
- `production -> compare -> shadow -> off` rollback behavior
- limited production allowlist definition
- limited fixture、negative boundaries 与 Full escape regression tests

后续 Full Production 债务不得顺手修改以上三条 transition；只有新的真实错误证据才能重新开启根因审查。

## 18. Final Status

所有 Limited Production hard gates 已通过：D=0、NEW regression=0、Safety Gate PASS、evidence loss=0、scope escape=0、rollback PASS。最终 deployment configuration 保持 Limited Production enabled，仓库代码安全默认保持 off。

**LIMITED PRODUCTION v1.0 ENABLED AND FROZEN**
