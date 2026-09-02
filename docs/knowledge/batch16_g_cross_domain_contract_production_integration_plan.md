# LanternBox Batch16-G: Cross-Domain Contract Production Integration Plan

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_f_cross_domain_contract_revalidation_report.md`
- `docs/knowledge/batch16_e_cross_domain_contract_revision.md`

本阶段只生成生产接入规划。未修改 contract data、Retrieval、profiles、ranking、selector、Wiki、Guide、tests 或 database，也未执行 Integration。

## 1. Contract Layer Architecture Position

### 1.1 Proposed Flow

建议生产链路：

```text
Raw Query
  -> existing query normalization
  -> Cross-Domain Contract Evaluator
  -> OwnershipDecision
  -> existing Retrieval Planner with ownership lanes
  -> Guide candidate fetch/filter
  -> existing Guide ranking inside each eligible lane
  -> existing selector
  -> deterministic related Wiki loading
  -> existing response generation
```

用户提出的：

```text
Query -> Contract -> Domain Ownership -> Retrieval
```

总体合理，但 Contract 应放在现有 query normalization 之后、Retrieval planning 之前。原因：

- evaluator 需要稳定的规范化 query，但不应读取候选分数或生成内容。
- ownership 必须在候选集合形成前确定，才能通过 candidate eligibility 实现，而不是事后修改 ranking。
- Contract 只决定哪些领域有资格进入 primary lane、哪些必须作为 secondary、哪些不得成为 primary。
- Retrieval 仍负责实际 Guide 召回、评分、选择、related Wiki 和 traceability。

### 1.2 Proposed Components

后续 Integration Apply 建议采用最小组件，不引入独立服务：

|组件|职责|不得承担|
|---|---|---|
|Contract loader|只读加载、版本检查、启动时校验 contract data|数据库同步、热写入、业务召回|
|Contract evaluator|根据 query 产生确定性 ownership decision|Guide/Wiki 搜索、分数计算、Prompt 生成|
|Ownership adapter|把 contract owner ID 映射到现有 Guide domain/category metadata|按 Guide ID 硬编码、修改 Guide metadata|
|Candidate lane policy|形成 primary、mandatory secondary、suppressed-primary lanes|修改 rank score、重排同 lane Guide|
|Safety gate|验证 Active Hazard / Human Harm 必需 owner 是否进入候选/选择|生成安全内容、修改 fallback 文本|
|Contract trace|记录 mode、version、matched rule、decision 与 suppression reason|写入 Prompt、改变 answer wording|

优先使用一个小型 Retrieval 内部模块，例如 `api/retrieval_v2/contracts.py`，避免建立新的服务或持久化 schema。`OwnershipDecision` 应为每次请求的内存对象，不写 PocketBase 或 database。

### 1.3 OwnershipDecision Contract

建议最小字段：

- `contract_version`
- `matched_contract`
- `matched_rule`
- `primary_owners`
- `mandatory_secondary_owners`
- `suppressed_primary_owners`
- `required_safety_actions`
- `decision_status`: matched / not_matched / invalid / unsatisfied
- `trace_reasons`: positive、negative、historical、human-harm 或 object-class evidence

禁止放入：

- Guide ID allowlist
- Wiki slug allowlist
- ranking boost / penalty
- top_k 或 selector limit
- Prompt 片段
- 生成答案模板

### 1.4 Domain Metadata Prerequisite

Ownership adapter 只能使用现有 domain/category metadata。Integration 前必须确认 Medical、Energy Safety、Fire、Energy Deepening、Records、Storage、Repair、Manufacturing、Agriculture / Seed Library 的 metadata 能稳定映射。

若必须通过 case ID、完整 query 或 Guide ID 才能区分 owner，说明 metadata contract 不足，应停止 Integration 并回到 architecture review，不能用硬编码绕过。

## 2. Integration Boundary

### 2.1 Contract May Affect

#### Domain Ownership

Contract 可以：

- 指定本次 query 的 primary owner domain。
- 指定必须进入的 secondary owner domain。
- 指定哪些 domain 不得成为 primary。
- 在 Human Harm、Active Fire、设备异常、信息对象和物理对象之间执行已验证的 ownership precedence。

Contract 不改变 stable domain 的内部知识职责，只处理跨域主位。

#### Candidate Filtering

Contract 可以在 Guide ranking 前形成三个 lane：

1. `primary`: 只包含 primary owner 的合格 Guide candidates。
2. `mandatory_secondary`: 包含 Fire、Safety、Medical、Records 等契约要求的辅助领域。
3. `suppressed_primary`: 候选可保留作背景，但不得占 primary slot。

过滤规则：

- 不修改候选原始分数。
- 不改变同一 lane 内相对顺序。
- 不扩大 fetch limit、top_k 或 selector limit。
- 不删除 trace；被限制的 candidate 必须记录 `contract_suppressed_primary` reason。
- suppression 默认限制 primary eligibility，不应把仍有价值的 secondary evidence 全部丢弃。

#### Safety Gating

Contract 可以：

- 对 Active Hazard / Human Harm 要求至少一个正确 Safety / Fire / Medical primary candidate。
- 要求 mandatory secondary owner 进入可选择集合。
- 在必需 owner 缺失时将 decision 标记为 `unsatisfied`，阻止 unrelated diagnosis / optimization / repair Guide 成为 primary。
- 调用现有安全 fallback 路径，但不得修改 fallback 内容或生成新操作建议。

错误处理必须区分：

- evaluator 解析失败：关闭本次 Contract decision，回到现有 Retrieval 与现有 safety protection，并记录 error。
- evaluator 成功但 safety owner 缺失：fail closed，不能让被 suppression 的领域取得 primary；使用现有 fallback。

### 2.2 Contract Must Not Affect

#### Wiki Ranking

- Contract 不读取、过滤、boost、penalize 或重排 Wiki。
- Guide 选中后仍按既有规则确定性加载 related Wiki。
- independent Wiki ranking 和 cap 保持不变。
- Contract trace 不写入 Wiki query。

#### Guide Ranking

- 不修改任何 score、boost、penalty、tie-break 或 ranking function。
- Contract 只决定 candidate 的 lane / primary eligibility。
- 合格 candidates 在每个 lane 内保持现有 ranking 的相对顺序。
- 不用“Safety 一律加分”或 risk_level 全局调权实现 ownership。

#### Content Generation

- Contract data、matched terms、suppression reason 不进入 Prompt。
- Contract 不生成 STOP、Medical、Fire、Storage 或 Records 文本。
- 生成层继续只使用选中的 Guide/Wiki evidence 和现有 safety sanitizer。
- Contract 不覆盖 Kiwix policy、fallback 文本或 source priority。

### 2.3 Integration Invariants

1. `mode=off` 时请求结果与接入前字节级/结构级一致。
2. `not_matched` query 的 candidates、scores、order、selected evidence 完全一致。
3. `matched` query 只允许 primary eligibility 和 mandatory secondary membership 发生变化。
4. retained candidates 的 score 与相对顺序不变。
5. selected Guide 的 related Wiki 加载行为不变。
6. Contract decision 不出现在 Prompt 或最终回答中。
7. 所有 ownership 变化都能追溯到 contract version、rule 和 evidence。

## 3. Backward Compatibility

### 3.1 Stable-Domain Rule

Contract 不是新的全局 domain router。只有满足已验证复合条件时才产生 matched decision；其他 query 必须走原有 Retrieval。

“不会被重新排序”的验收定义：

- 非 contract query：所有 Guide candidate ID、score、order 与 top1 不变。
- contract query：同一 owner lane 内 ID、score、relative order 不变。
- stable top1 只在它违反明确 ownership contract 时失去 primary eligibility。
- Wiki ranking、Guide ranking function 和 selector function 均不变。

### 3.2 Domain-by-Domain Protection

|Stable domain|保持不变的主位|Contract 允许改变的边界|
|---|---|---|
|Medical|当前伤病、治疗与观察|设备事故造成人员伤害时提升 Medical；无人受伤不触发|
|Energy|正常容量、产出、Management、Deepening|鼓包、冒烟、火花、未知并联等当前危险转 Safety / Fire|
|Records|归档、可信度、版本、趋势|当前危险先由 Medical/Safety/Fire；物理对象 custody 不由 Records primary|
|Storage|物资、设备、种子、耗材本体保存|记录/日志/检测结果 lifecycle 转 Records；异常物品先转 Safety|
|Manufacturing|正常制作、工艺与当前生产|当前人身/火灾/电气危险转 Safety；维修日志归档转 Records|
|Agriculture|当前种植、田间判断与农业动作|种子本体入库转 Seed Library/Storage；实验记录归档转 Records|

### 3.3 Frozen Regression Set

Integration 前后必须保持：

- Battery abnormal -> Energy Safety。
- Normal capacity degradation -> Energy Deepening。
- Unknown power parallel -> Energy Safety。
- Energy records lifecycle -> Records。
- Spare parts physical storage -> Storage。
- 当天能源分配 -> Energy Management。
- 正常 Manufacturing 制作 -> Manufacturing。
- 当前 Agriculture 种植 -> Agriculture。
- 当前 Medical injury -> Medical。

不得为保护 stable domain 扩大 accepted owner；必须比较真实 primary、candidate order 和 evidence。

## 4. Rollout Plan

### Phase 0: Integration Preconditions

- 将 Batch16-E v1.1 语义按独立 Apply 写入 contract data。
- 通过 parser/schema-in-memory 校验；不建立 database schema。
- 建立 owner ID 到现有 metadata 的完整映射检查。
- 把 Batch16-F 16 个 case、5 个 regression 和 Batch16-C baseline 固化为 integration contracts。
- mode 默认 `off`。

### Phase 1: Shadow Mode

行为：

- 对每个 query 运行 Contract evaluator。
- 记录 OwnershipDecision 和 latency。
- Retrieval、candidate、selector、Prompt 与答案完全使用 baseline。
- 不执行 candidate filtering 或 safety gate。

必须观测：

- match / not_match / invalid 比率。
- rule 分布与 owner 分布。
- positive / negative / historical evidence。
- evaluator latency 与 parse error。
- fixed validation case 的 false positive / false negative。

进入 compare mode 的门槛：

- 16 / 16 v1.1 case pass。
- 5 / 5 regression pass。
- false positive=0，false negative=0。
- stable-domain non-trigger decision rate=0。
- parser / owner mapping error=0。

### Phase 2: Compare Mode

行为：

- baseline Retrieval 正常运行并返回用户结果。
- 并行计算 proposed ownership lanes、candidate filtering 与 safety gate。
- 不让 proposed 结果进入 selector、Prompt 或最终回答。
- 输出 baseline/proposed trace diff。

比较项目：

- candidate IDs、scores、order。
- baseline top owner 与 proposed top owner。
- suppressed-primary candidates 与 reason。
- mandatory secondary 是否存在。
- related Wiki 集合是否仅因 proposed Guide primary 变化而变化；Wiki ranking 不得变化。
- stable-domain non-trigger query 是否保持完全一致。

进入 production mode 的门槛：

- 所有 contract case ownership diff 与 expected 一致。
- 所有 non-trigger query diff=0。
- retained candidate score/order diff=0。
- dangerous suggestion=0，Kiwix override=0。
- Safety / fallback / record-check=100%。
- 无 mandatory owner missing。
- latency 在预先冻结预算内。

### Phase 3: Production Mode

采用可回退 feature mode，默认从小范围 rule family 启用：

1. Information Object：Records / physical custody。
2. Active Hazard：Battery / low-voltage device / Fire negative evidence。
3. Human Harm Priority。

每个 family 独立启用、独立记录，不用一次开关隐藏具体回归。返回路径只使用 ownership lanes；ranking、selector、Wiki 和 generation 保持原实现。

生产门槛持续监控：

- contract match count。
- owner-change count。
- unsatisfied safety gate count。
- fallback count。
- stable-domain top1 diff。
- false positive / false negative review。
- latency 与 exception count。

## 5. Rollback Plan

### 5.1 Runtime Modes

使用单一运行模式配置：

- `off`: evaluator 不参与请求。
- `shadow`: 只计算 decision。
- `compare`: 计算 proposed Retrieval diff，但返回 baseline。
- `production`: ownership lanes 与 safety gate 生效。

配置使用项目现有 runtime configuration 机制；不要求 database、PocketBase 或 schema migration。具体配置载体在 Integration Apply 前冻结，不在本阶段选择或修改 `.env`。

### 5.2 Rollback Levels

|当前模式|回退目标|影响|
|---|---|---|
|production|compare|立即停止影响用户结果，继续保留 proposed diff|
|compare|shadow|停止模拟 candidate filtering，只保留 decision trace|
|shadow|off|完全绕过 evaluator|

Contract data 可以保留在磁盘；`off` 模式不得加载或应用其 decision。没有数据库迁移，因此无需数据回滚或清理。

### 5.3 Automatic Rollback Triggers

任一条件触发 production -> compare：

- false negative > 0，尤其 Medical / Fire / Safety owner 缺失。
- non-trigger stable-domain top1 diff > 0。
- mandatory owner missing > 0。
- retained candidate score/order 发生变化。
- dangerous suggestion > 0 或 Kiwix override > 0。
- Contract parse/version/owner mapping error > 0。
- fallback 或 latency 超过冻结阈值。

回退后保留 trace 和失败 query 的脱敏 case ID，不自动修改 contract data、profile、ranking 或 expected。

### 5.4 Rollback Verification

回退到 `off` 后必须确认：

- baseline Retrieval contracts 全部通过。
- candidate IDs、scores、order 恢复 baseline。
- Prompt 与 selected evidence 不含 Contract trace。
- Contract loader/evaluator 不在 request path。
- 不需要 database cleanup。

## 6. Test Plan

### 6.1 Contract Unit Tests

- parser、version、required fields、duplicate rule、unknown owner。
- Negative Fire Evidence 的局部否定范围。
- historical -> current positive transition。
- Human Harm positive / negative / exit conditions。
- Biological Material、Consumable、Sample 的 object/action classification。
- invalid contract 必须 fail startup validation 或保持 mode off，不能半加载。

### 6.2 Fixed Semantic Matrix

必须全部运行：

- Batch16-C 原始 17 tests。
- Batch16-D 9 validation cases；v1.1 Apply 后原 3 xfail 必须转 pass，并移除 xfail 标记。
- Batch16-F 16 revalidation cases。
- Batch16-F 5 regression cases。
- Fire negative/positive pair、Human Harm pair、physical object/information object pair。

Expected 必须在 Integration 前冻结，不根据结果调整。

### 6.3 Retrieval Integration Tests

- `mode=off` 与未接入 baseline 的 retrieval trace 完全一致。
- shadow mode 不改变 candidates、selected evidence、Prompt 或 answer。
- compare mode 返回 baseline，只生成 proposed diff。
- production mode 只改变 matched case 的 primary eligibility。
- retained Guide scores 与 lane 内顺序不变。
- suppressed domain 可作为允许的 secondary，但不能占 primary。
- mandatory secondary 可被选择，且不扩大 selector limit。
- Guide selected 后 related Wiki 仍确定性加载。
- Contract 不进入 Wiki ranking、Prompt、Kiwix 或 content sanitizer。

### 6.4 Stable-Domain Regression

至少覆盖：

- Medical：当前伤病、历史伤病记录、设备事故伤害。
- Energy：Safety、Management、Deepening、Fire negative/positive。
- Records：archive、trust、trend、当前业务操作保护。
- Storage：physical device、seed、consumable、sample custody。
- Manufacturing：正常制作、当前危险、repair logs。
- Agriculture：当前种植、seed storage、experiment records。

每个领域都需要 positive + negative boundary，不接受只验证 ownership change 的测试。

### 6.5 Safety and Failure Tests

- Battery disassembly、cell replacement、BMS、unknown parallel、backfeed、mains/high-voltage。
- Fire negative evidence 不得降低设备 Safety gate。
- Human injury 必须 Medical primary，场景危险仍保留 Safety / Fire secondary。
- mandatory owner absent、unknown owner、invalid version、malformed JSON。
- evaluator exception 时回到 baseline；successful safety decision unsatisfied 时 fail closed 到现有 fallback。
- dangerous suggestion=0，Kiwix override=0，Safety/fallback/record-check=100%。

### 6.6 Rollout and Rollback Tests

- off / shadow / compare / production 状态转换。
- 每个 rule family 独立 enable/disable。
- production -> compare 不重启数据库、不改 schema。
- off 模式不加载 Contract decision。
- rollback 后 baseline hashes、trace 与 selected evidence 恢复。

### 6.7 Acceptance Gate

Production Integration Apply 只有同时满足以下条件才能进入 production mode：

- Contract / integration tests 全绿。
- 既有 Retrieval contracts 无 unexpected fail。
- 16 / 16 v1.1 validation，5 / 5 regression。
- Fire / Human Harm / Information Object=100%。
- false positive=0，false negative=0，regression=0。
- non-trigger stable-domain diff=0。
- ranking score/order diff=0。
- dangerous suggestion=0，Kiwix override=0。
- Safety/fallback/record-check=100%。
- rollback drill 通过。

## 7. Recommendation

架构结论：Query normalization 后、Retrieval planning 前增加纯 Contract evaluator，并通过 ownership lanes 与 safety gate 约束 primary eligibility，是合理且可回滚的接入位置。

该方案：

- 能治理跨域 ownership，而不修改全局 ranking。
- 能在 candidate 层实施 suppression，而不触碰 Wiki ranking。
- 能执行 Safety / Medical fail-closed gate，而不生成内容或修改 fallback。
- 能通过 off / shadow / compare / production 四态渐进上线。
- 能在不迁移 database、不改 profile 的情况下立即回滚。

下一阶段应先执行独立 Integration Apply：升级 contract data 到 v1.1、实现纯 evaluator/adapter/lane policy/trace、保持默认 mode=off，并完成 shadow/compare 验证后再申请 production mode。

最终状态：**READY FOR INTEGRATION**
