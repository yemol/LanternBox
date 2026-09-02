# LanternBox Batch16-K: Cross-Domain Contract Production Enable Review

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_j_cross_domain_contract_compare_report.md`
- `docs/knowledge/batch16_h_cross_domain_contract_shadow_integration_report.md`
- `docs/knowledge/batch16_g_cross_domain_contract_production_integration_plan.md`

本批只做 production enable 评审与方案设计。未修改 Retrieval、ranking、selector、profiles、Wiki、Guide 或 contract behavior，未启用 production。

## 1. Current Review Status

Shadow 与 Compare 已证明：

- v1.1 reference semantics 在冻结用例中 Contract accuracy = 100%。
- Batch16-I fixture Compare 为 A=3、B=0、C=9、D=0。
- off / shadow / compare 不改变 Retrieval output。
- 三条 A 能稳定识别 baseline ownership debt。

但当前实现尚未满足 production enable gate：

|Gate|Current status|Review|
|---|---|---|
|Contract v1.1 单一数据源|v1.0 data + v1.1 code 双源|FAIL|
|production mode 开关|只允许 off / shadow / compare|FAIL|
|ownership lanes|未实现|FAIL|
|mandatory secondary enforcement|未实现|FAIL|
|safety gate / unsatisfied handling|未实现|FAIL|
|rollback chain|有设计，无 production drill|PARTIAL|
|OwnershipDecision audit|进程 logger，无冻结保留/脱敏规范|PARTIAL|
|stable-domain compare regression|通过|PASS for compare only|
|stable-domain production regression|无 production path 可测|NOT TESTED|
|三条 A 语义收益|已确认|PASS|
|三条 A 可执行 candidate readiness|存在缺口|FAIL|

因此，Compare readiness 不能直接提升为 Production readiness。

## 2. Contract v1.1 Single Source Review

### Current State

当前存在两个语义来源：

1. `data/cross_domain_contracts.json`
   - `version=1.0.0`
   - `status=contract_only`
   - `runtime_integration=false`
   - 文件明确声明不被 Retrieval runtime 消费。

2. `api/retrieval_v2/contract_runtime.py`
   - `contract_version=1.1-shadow`
   - Fire negative evidence、Human Harm、physical object expansion 等 v1.1 语义硬编码在 Python 中。

原有 3 条 v1.0 contract-data xfail 仍保留。此状态不能作为 production authority：同一 query 可能因 data evaluator 与 runtime evaluator 不同而得到不同 ownership。

### Required Single-Source Design

Production Enable Apply 必须先建立唯一权威源：

```text
data/cross_domain_contracts.json v1.1.0
  -> startup loader
  -> schema/in-memory validation
  -> immutable ContractSet
  -> generic evaluator
  -> OwnershipDecision
```

要求：

- 把 Batch16-E/F 验证过的 v1.1 规则全部迁入 data，包括 Fire negative evidence、Human Harm priority、Biological Material、Consumable、Sample。
- Python evaluator 只实现通用匹配、precedence、negative evidence scope 和 decision assembly，不再保存领域词表的第二份副本。
- ContractSet 启动时校验 version、rule ID 唯一性、owner 合法性、priority 冲突、required fields 和 checksum。
- 每个请求固定使用一次加载后的 immutable version；禁止请求中途热切换。
- invalid / unknown version 时禁止进入 production，并自动保持或回退到 compare。
- v1.0 的 3 条 xfail 必须转 pass 并移除 xfail 标记。
- audit 必须记录 data version 与 checksum，不能只记录代码常量。

在单一数据源完成前，不应增加 production behavior。

## 3. Production Mode Switch Design

### Proposed Modes

继续使用单一 `contract_mode`：

- `off`: 不加载、不评估、不记录。
- `shadow`: 只产生 OwnershipDecision。
- `compare`: baseline 正常返回，只记录 baseline/proposed 差异。
- `production`: Contract 只影响 domain ownership eligibility 与 safety gating。

默认必须保持 `off`。只有 Production Enable Apply、production contracts 和 rollback drill 全部通过后，API model 才允许 `production`。

### Production Effect Boundary

production 只允许：

1. 标记 primary owner lane。
2. 标记 mandatory secondary lane。
3. 禁止 suppressed domain 成为 primary。
4. 在 required safety owner 缺失时 fail closed 到现有 fallback。

production 禁止：

- 修改 candidate 原始 score。
- 修改同一 lane 内相对顺序。
- 修改 ranking function、selector、top_k 或 selector limit。
- 修改 Wiki ranking 或 Guide-Wiki loading。
- 把 Contract trace 写入 Prompt 或 answer。
- 生成新的安全建议或 fallback 文本。

### Enable Guard

建议 production 开关前置 guard：

- loaded contract version 必须为批准的 `1.1.0` checksum。
- owner metadata mapping coverage = 100%。
- matched rule 的 primary candidate 必须存在。
- mandatory secondary candidate 必须存在，或进入明确的 `unsatisfied` fallback。
- retained candidates 的 score/order invariants 必须通过。
- non-trigger query diff 必须为 0。
- invalid ContractSet、unknown owner、missing mandatory owner 均禁止 enable。

建议按 rule family 独立 rollout，但不引入隐藏的全局 ranking 调整：

1. `information_object`
2. `active_hazard`
3. `human_harm_priority`

每个 family 必须能单独回退到 compare。

## 4. Rollback Review

目标链路合理：

```text
production -> compare -> shadow -> off
```

### production -> compare

- 立即停止 ownership eligibility、suppression 和 safety gate 对用户结果的影响。
- 保留 Contract evaluation 和 old/proposed diff。
- 不重写 candidate、database 或 contract data。

触发条件：

- Medical / Fire / Safety false negative > 0。
- mandatory owner missing > 0。
- stable-domain non-trigger top1 diff > 0。
- retained candidate score/order diff > 0。
- dangerous suggestion > 0 或 Kiwix override > 0。
- contract load/version/checksum/owner mapping error > 0。
- fallback、exception 或 latency 超过冻结阈值。

### compare -> shadow

- 停止读取 baseline candidate ownership。
- 只保留 OwnershipDecision、match/not-match、rule 和 latency。

### shadow -> off

- evaluator 不进入请求路径。
- 不产生 Contract 日志。
- Retrieval output 必须恢复并保持 baseline。

### Missing Verification

当前没有 production mode，也没有以下 drill：

- production -> compare 原子切换。
- in-flight request 使用一致 mode/version。
- rollback 后 candidate IDs、scores、order、selection 和 Prompt 恢复 baseline。
- invalid v1.1 data 自动拒绝 production。

这些不是文档声明可以替代的测试，必须在下一次 Integration Apply 中实现并验证。

## 5. OwnershipDecision Audit Log Review

### Current State

当前 shadow / compare 使用 `lanternbox.cross_domain_contract` logger，能输出 decision 和 comparison，但没有冻结：

- 日志持久化载体。
- rotation / retention 周期。
- query 脱敏规则。
- request correlation ID。
- production applied changes。
- rollback epoch 与 mode transition audit。

因此当前日志适合开发观测，不足以支持 production ownership 审计。

### Required Audit Event

Production audit event 至少包含：

- `event_version`
- `request_id`
- `timestamp`
- `contract_mode`
- `contract_version`
- `contract_checksum`
- `rule_family`
- `matched_rule`
- `decision_status`: matched / not_matched / invalid / unsatisfied / applied
- `primary_domain`
- `mandatory_secondary_domains`
- `suppressed_domains`
- `trigger_reason` 与 positive/negative evidence IDs
- baseline top owner / source ID
- applied primary eligibility changes
- mandatory owner presence / missing reason
- fallback used / rollback epoch
- evaluator and total compare latency
- error code

隐私边界：

- 默认记录 query hash 与短脱敏摘要，不长期保存原始 query。
- 不记录 Prompt、answer、医疗身份信息或完整历史。
- 使用本地 append-only rotating JSONL 或等价离线日志；不要求 database schema 或云端服务。
- retention、rotation、磁盘不足降级和人工 review 流程必须冻结。

## 6. Stable Domains Regression Impact

Compare 已确认 baseline output 不变，但 production 尚未实现，不能据此推断 stable domains 无回归。

Production contracts 必须覆盖：

|Stable domain|必须保持|允许改变的窄边界|
|---|---|---|
|Medical|普通医疗、当前伤病 top1|设备事故有人受伤时 Medical override|
|Energy Safety|鼓包、冒烟、未知电源|不得被 Management / Deepening 抢主位|
|Energy Management|当天电量分配、排程|只在 active hazard 或明确长期趋势时让位|
|Energy Deepening|容量下降、跨季基线|active hazard 时让位 Safety / Fire|
|Records|归档、可信度、版本、趋势|当前危险先由 Safety / Medical / Fire|
|Storage|物资、备件、种子本体保存|信息对象 lifecycle 让位 Records|
|Manufacturing / Repair|当前制作与维修动作|日志归档让位 Records；当前危险让位 Safety|
|Agriculture|当前种植与收获处理|种子 custody 允许 Storage 共域|

验收必须比较：

- candidate IDs、scores、order。
- primary eligibility。
- mandatory secondary presence。
- selected Guide 与 related Wiki。
- Prompt / answer 不含 Contract trace。
- dangerous suggestion、Kiwix、Safety / fallback / record-check。

## 7. A-Class Benefit Review

### E03 Controller Smoke

语义收益：高。Contract 将 `energy_management -> energy_safety`，并要求 Fire secondary，符合 Active Hazard Contract。

Production readiness：不足。只读 top8 检查中没有稳定识别出 Energy Safety candidate；出现的 Fire candidate是燃气泄漏 Guide，不能作为控制器冒烟的 mandatory secondary。若直接 suppression Energy Management，可能得到 `unsatisfied` 且缺少正确安全证据。

### E04 Seasonal Solar Trend

语义收益：明确。Contract 将 `energy_management -> energy_deepening`，保护跨季基线不被当天排程吞掉。

Production readiness：部分。DG-0922 已在 top3，但当前 metadata adapter 把它识别为 Repair，而不是稳定的 Energy Deepening owner。这说明 candidate recall 可能存在，owner metadata contract 尚未完成。

### W01 Repair Log Archive

语义收益：明确。Contract 将 `repair -> records`，符合 Information Object Contract。

Production readiness：不足。只读 top8 均为 Repair / Manufacturing Guide，没有 Records candidate。直接禁止 Repair primary 会造成 Records lane 空缺。

### Benefit Conclusion

三条 A 的 ownership correction 方向全部确认；但 A 只证明“Contract 看到了旧债务”，不证明目标 owner candidate 已经可用。Production Apply 前必须解决 metadata mapping 与 mandatory candidate readiness，不能通过调整 fixture、放宽 accepted owner 或改 ranking 掩盖。

## 8. Required Integration Revision

下一阶段最小 Integration Revision 应按顺序完成：

1. 将 v1.1 迁入唯一 contract data，并让 runtime 只消费该数据。
2. 让 v1.0 三条 xfail 转 pass。
3. 冻结 owner taxonomy 与 metadata adapter；禁止 Guide ID / case ID 映射。
4. 在 compare 中模拟 primary、mandatory secondary、suppressed lanes，但仍返回 baseline。
5. 对三条 A 验证目标 owner candidate presence；缺失时只记录 `unsatisfied`，不修改 ranking/profile。
6. 实现 production mode，但默认关闭。
7. 实现 fail-closed safety gate 与现有 fallback 的连接，不修改 fallback 文本。
8. 建立 production audit event、rotation、retention 和脱敏规则。
9. 完成 stable-domain production regression 和 rollback drill。
10. 另行执行 Production Enable Verification 后，才允许切换 production。

明确不建议：

- 直接把 Compare A 变成强制 top1。
- 为三条 A 调整全局 ranking、top_k 或 selector limit。
- 在 runtime 与 JSON 各维护一份 v1.1 词表。
- 用 Guide ID、case ID 或完整 query 硬编码 ownership。
- 在 mandatory Safety / Records candidate 缺失时静默选择其他域。
- 把 audit trace 写入 Prompt 或用户答案。

## 9. Review Decision

当前系统已达到“可观察、可比较”，尚未达到“可安全施加 ownership 变化”。阻断项是 integration completeness，而不是 Contract 语义方向：

- v1.1 未单一数据源。
- production mode 与 lane/safety behavior 不存在。
- rollback 未做 production drill。
- audit logging 未达到 production retention / privacy 要求。
- 三条 A 中两条目标 owner candidate 缺失，一条 owner metadata 不稳定。

最终状态：**NEEDS INTEGRATION REVISION**
