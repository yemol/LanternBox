# LanternBox Batch16-C: Cross-Domain Ownership Contract Minimal Apply Report

生成日期：2026-08-25

依据：

- `docs/knowledge/batch16_b_cross_domain_contract_design.md`
- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

## 1. Changed Files

本批新增：

- `data/cross_domain_contracts.json`
- `tests/test_cross_domain_ownership_contracts.py`
- `docs/knowledge/batch16_c_cross_domain_contract_apply_report.md`

按要求运行审计时刷新了当日生成 artifact：

- `docs/knowledge/wiki_audit_2026-08-25.md`
- `docs/knowledge/guide_audit_2026-08-25.md`

未修改 Wiki、Guide、Guide-Wiki、related_wiki、guide_links、Retrieval profile、Prompt、ranking、top_k、selector、selector limit、fallback、schema、PocketBase、FT-02、Core、firmware、hardware 或 database。

## 2. Contract Implementation

`data/cross_domain_contracts.json` 是独立声明式 Contract Layer，版本为 `1.0.0`：

- `status=contract_only`
- `runtime_integration=false`
- contract precedence：Active Hazard 在 Information Object 之前
- 包含 ownership priority、primary owner、allowed secondary owners、suppressed primary owners 和 required actions
- 不含 ranking 分数、top_k、selector 数量或生产 Guide ID override

本批没有新增生产 evaluator。`tests/test_cross_domain_ownership_contracts.py` 内提供最小 reference evaluator，只用于验证 JSON 契约是否能够产生确定 ownership 决策。未来 domain expansion 可以复用 contract data，在独立 Apply 经过边界验证后再选择接入方式。

### Active Hazard Contract

声明五类规则：

1. `active_fire`
2. `battery_invasive_operation`
3. `battery_abnormal`
4. `energy_device_abnormal`
5. `unknown_power_action`

规则要求当前 Fire / Energy Safety 优先，并抑制 Energy Deepening、Energy Management、Repair、Records、Storage 或 Manufacturing 的不当 primary。数据同时声明 resolved-state、prohibition-context 和 Deepening negative boundaries。

### Information Object Contract

声明四类规则：

1. `records_trust`
2. `records_lifecycle`
3. `records_trend`
4. `physical_storage`

信息对象的可信度、归档、版本和趋势由 Records 主导；物资、设备和电池本体的入库、存放与物理保存由 Storage 主导。Physical Storage 保留 Records supplemental ownership。

## 3. Active Hazard Tests

|Case|Expected|Result|Matched rule|
|---|---|---|---|
|A01 鼓包电池仍想测容量|Energy Safety|PASS|`battery_abnormal`|
|A02 太阳能控制器正在冒烟|Energy Safety / Fire|PASS|`energy_device_abnormal`；Fire escalation 保留|
|A03 未知来源电源直接并联|Safety|PASS|`unknown_power_action`|
|A04 旧锂电池包拆开换芯|Energy Safety|PASS|`battery_invasive_operation`|
|N01 正常容量下降复查|Energy Deepening|PASS|Active Hazard 不触发；preserve Deepening|
|N02 Solar 跨季基线|Energy Deepening|PASS|Active Hazard 不触发；preserve Deepening|

额外边界通过：

- Active Hazard 优先于记录归档请求。
- 已停用隔离且冷却稳定后可退出 Active Hazard，转 Records。
- “禁止把未知来源电源直接并联”不会被解释为请求执行危险动作。

## 4. Information Object Tests

|Case|Expected|Result|Matched rule|
|---|---|---|---|
|I01 三个月能源记录长期归档|Records|PASS|`records_lifecycle`|
|I02 备用电池长期不用如何保存|Storage|PASS|`physical_storage`|
|I03 电池测试数据标记可信度|Records|PASS|`records_trust`|
|I04 设备贴标签后入库存放|Storage primary + Records supplemental|PASS|`physical_storage`|

对象主题与对象类型已分离：记录内容出现“太阳能/电池”不会把信息归档改判为物理 Storage；设备标签服务入库存放时仍由 Storage 主导。

## 5. No Retrieval Change Confirmation

本批开始与验证结束后，对以下受保护文件执行 SHA-256 对比，结果 10 / 10 一致：

- `api/retrieval_v2/orchestrator.py`
- `api/retrieval_v2/schemas.py`
- `api/retrieval_v2/__init__.py`
- `api/retrieval_v2/policy.py`
- `api/retrieval_v2/selector.py`
- `api/retrieval_v2/fetchers.py`
- `api/retrieval_v2/planner.py`
- `data/retrieval_query_profiles.json`
- `tests/test_retrieval_traceability.py`
- `tests/test_retrieval_root_contract.py`

新增测试扫描 `api/**/*.py`，确认没有生产模块引用 `cross_domain_contracts.json`。因此现有 candidate recall、ranking、selector 与最终 Retrieval 行为不受本批影响。

## 6. No Profile Change Confirmation

`data/retrieval_query_profiles.json` 在本批前后 SHA-256 均为：

`b6c7f7877114f8c0520d2835054508b4e81ce80c36b7e2492534aceea97e06bc`

没有新增 Safety、Records 或 Storage profile，也没有修改任何既有 profile。工作区中该文件原有 dirty 状态未清理、未提交，本批只用哈希确认内容没有发生变化。

## 7. Regression Result

### Required Audits

|验证|结果|
|---|---|
|Wiki audit|PASS：1157 Markdown / 1157 PocketBase；errors=0, warnings=0, advisories=0|
|Guide audit|PASS：851 Guides；errors=0, warnings=0, advisories=0|

### Contract and Retrieval Regression

|范围|结果|
|---|---|
|新增 cross-domain contract tests|17 passed|
|Contract + retrieval traceability/root + Energy Deepening profile tests|46 passed|
|既有 Retrieval 测试文件内容|哈希未变|

pytest cache 因工作区权限产生 warning，不影响测试结论。

### Full Pytest Status

全量 pytest 已执行，但仓库既有问题使其不能形成全绿结果：

1. 原始全量 collection 在 `tests/test_planting_retrieval_field.py` 被同名 `scripts/test_planting_retrieval_field.py` 模块冲突阻断。
2. 跳过该单一 collection 冲突后，32 passed，随后 `scripts/test_kiwix_retrieval_pipeline.py::test_api_response_shape` 因既有 `source_payload` fixture 缺失而 error。
3. 仅运行 `tests/` 并跳过同名冲突后，复现既有 Agriculture 边界失败：期望 DG-0879，实际 DG-0897。

这些失败均不读取新增 contract data，也不是本批引入。按禁止范围未修改 planting、Kiwix 或 Agriculture 实现与测试。

## 8. Recommendation

Contract Layer 已满足本阶段目标：

- 两个契约均为声明式、可复用、可版本化的数据。
- 必需 positive / negative ownership cases 全部通过。
- Active Hazard 与 Information Object 的冲突顺序已验证。
- 生产 Retrieval 与 profile 保持字节级不变。
- 当前实现不会提高命中率、改变 ranking 或修复 O01/O05/O07/O08。

下一阶段应只做 Contract Validation：审查术语覆盖、resolved-state 退出条件、Fire / Energy Safety 升级边界、Records / Storage object classification，以及对未来 domain expansion 的复用方式。运行时接入必须另立 Apply，不应在 validation 阶段隐式启用。

结束状态：**READY FOR CONTRACT VALIDATION**
