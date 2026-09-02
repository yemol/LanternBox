# LanternBox Batch14-C: Long-Term Records / Knowledge Transfer Knowledge Apply Report

生成日期：2026-08-24

依据：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch14_b_long_term_records_knowledge_transfer_plan.md`

## 0. Apply 结论

Batch14-C 已建立 Long-Term Records / Knowledge Transfer v0.1 的 Knowledge + Evidence 基础层：

- 新增 32 篇 Records Wiki，未复制 13 篇复用 Wiki。
- 新增 `DG-0911` 至 `DG-0918` 共 8 个 Guide。
- 只为允许处理的 `DG-0144`、`DG-0401`、`DG-0529` 补充旧 Guide evidence。
- Guide-Wiki 前向和反向关系完全对称。
- Markdown Wiki 与 PocketBase `wiki_articles` 均为 1139。
- Guide 源文件、`emergency_guides.json`、`guide_index.json` 均为 844。
- Wiki audit 与 Guide audit 均为 `errors=0 warnings=0 advisories=0`。
- Retrieval、Prompt、query profile、参数、tests、fixture、应用代码、FT-02 firmware 和 `data/lanternbox.db` 均未修改。

本阶段未运行 Retrieval Field Test，也未因为未来可检索性提前新增 profile 或调整 ranking。

## 1. 新增 32 篇 Wiki

### 1.1 Record Lifecycle / Trust

1. `records-lifecycle-state-model-001` - 记录生命周期状态模型
2. `records-raw-record-minimum-fields-001` - 原始现场记录最小字段
3. `records-review-verification-checklist-001` - 记录复核与验证检查表
4. `records-source-direct-reported-boundary-001` - 亲历、转述与设备来源边界
5. `records-applicable-conditions-context-001` - 经验适用条件与上下文字段
6. `records-contradictory-evidence-handling-001` - 冲突记录保留与复核

### 1.2 Review / Learning / Promotion

1. `records-success-review-minimum-001` - 成功做法复盘最小字段
2. `records-failure-cause-classification-001` - 失败原因分类与边界
3. `records-repeated-problem-trend-review-001` - 重复问题与趋势复查
4. `records-pending-hypothesis-next-test-001` - 待验证假设与下一次测试
5. `records-corrective-action-verification-001` - 整改项验证与关闭
6. `records-knowledge-promotion-gate-001` - 经验晋升正式知识门槛

### 1.3 SOP Version / Archive

1. `records-sop-version-minimum-fields-001` - SOP 与知识卡版本最小字段
2. `records-current-version-selection-001` - CURRENT 版本选择规则
3. `records-superseded-version-isolation-001` - SUPERSEDED 旧版隔离
4. `records-rollback-revalidation-001` - 回滚与重新验证
5. `records-high-risk-knowledge-review-gate-001` - 高风险知识审核门禁
6. `records-version-change-log-001` - 知识版本变更日志
7. `records-retention-access-disposal-001` - 记录保留、访问与到期处理

### 1.4 Cross-Domain Review

1. `records-daily-cross-domain-summary-001` - 每日跨域运行摘要
2. `records-incident-near-miss-review-template-001` - 事故与 near miss 上层复盘
3. `records-maintenance-history-trend-001` - 维修履历与重复故障趋势
4. `records-production-batch-long-term-review-001` - 生产批次长期回顾
5. `records-agriculture-season-comparison-001` - 农业跨季对比
6. `records-energy-capacity-trend-001` - 能源容量与产出趋势
7. `records-medical-observation-handover-boundary-001` - 医疗观察交接与隐私边界

### 1.5 Knowledge / Skill Transfer

1. `records-skill-authorization-minimum-001` - 技能授权记录最小字段
2. `records-skill-level-boundaries-001` - OBSERVE 到 INDEPENDENT 等级边界
3. `records-supervised-practice-evidence-001` - 监督练习证据记录
4. `records-skill-expiry-refresher-001` - 技能过期、降级与复训
5. `records-new-member-learning-path-001` - 新成员低风险学习路径
6. `records-knowledge-transfer-handover-summary-001` - 知识与技能交接摘要

所有新 Wiki 位于 `wiki_import/records/`。只在 `tools/audit_wiki.py` 的合法 domain 集合中最小新增 `records`，未修改审计阈值或降低检查严格度。

## 2. 复用 13 篇 Wiki

以下文件未复制、未重写业务正文，只按精准 evidence 增补 `guide_links`：

|复用 Wiki|本批新增 Guide 关系|
|---|---|
|`general-event-log-001`|`DG-0911`|
|`general-record-trust-label-001`|`DG-0912`|
|`education-paper-notebook-001`|`DG-0911`, `DG-0917`|
|`general-failure-lesson-record-001`|`DG-0913`|
|`general-knowledge-card-archive-001`|`DG-0915`, `DG-0916`|
|`general-long-term-archive-check-001`|`DG-0917`|
|`general-offline-directory-index-001`|`DG-0916`, `DG-0917`|
|`education-offline-data-backup-001`|`DG-0917`|
|`general-member-info-backup-001`|`DG-0144`, `DG-0529`, `DG-0917`|
|`general-medical-record-archive-001`|`DG-0529`|
|`general-small-team-handoff-001`|`DG-0918`|
|`organization-onboarding-001`|`DG-0918`|
|`organization-skill-redundancy-001`|`DG-0918`|

既有 Guide ID 均保留；表中只列本批新增的关系。

## 3. 新增 8 个 Guide

|Guide|标题|Priority|Risk|
|---|---|---|---|
|`DG-0911`|每日跨域运行摘要|P0|normal|
|`DG-0912`|原始记录可信度检查|P0|caution|
|`DG-0913`|失败 / 事故记录复盘|P0|high|
|`DG-0914`|重复问题与经验提取|P0|caution|
|`DG-0915`|经验升级为 SOP 候选|P0|high|
|`DG-0916`|SOP / Knowledge Card 版本更新与废止|P0|high|
|`DG-0917`|长期记录归档与索引|P1|caution|
|`DG-0918`|新成员知识与技能交接|P0|caution|

每个 Guide 均包含离线、纸面可执行的 `steps`、`check`、`fallback`、`stop_or_escalate` 和相邻域边界。Guide 不提供医疗处置、制造、维修、当前能源调度、当前库存、当前农业或 Production 班次操作。

## 4. 新 Guide-Wiki Evidence

8 个新 Guide 共建立 57 条精准 `related_wiki` evidence：

|Guide|related_wiki|
|---|---|
|`DG-0911`|`records-daily-cross-domain-summary-001`, `records-raw-record-minimum-fields-001`, `general-event-log-001`, `education-paper-notebook-001`|
|`DG-0912`|`records-lifecycle-state-model-001`, `records-raw-record-minimum-fields-001`, `records-review-verification-checklist-001`, `records-source-direct-reported-boundary-001`, `records-applicable-conditions-context-001`, `records-contradictory-evidence-handling-001`, `general-record-trust-label-001`, `records-medical-observation-handover-boundary-001`|
|`DG-0913`|`records-contradictory-evidence-handling-001`, `records-failure-cause-classification-001`, `records-corrective-action-verification-001`, `records-incident-near-miss-review-template-001`, `general-failure-lesson-record-001`|
|`DG-0914`|`records-applicable-conditions-context-001`, `records-success-review-minimum-001`, `records-repeated-problem-trend-review-001`, `records-pending-hypothesis-next-test-001`, `records-corrective-action-verification-001`, `records-maintenance-history-trend-001`, `records-production-batch-long-term-review-001`, `records-agriculture-season-comparison-001`, `records-energy-capacity-trend-001`|
|`DG-0915`|`records-lifecycle-state-model-001`, `records-applicable-conditions-context-001`, `records-knowledge-promotion-gate-001`, `records-high-risk-knowledge-review-gate-001`, `general-knowledge-card-archive-001`|
|`DG-0916`|`records-lifecycle-state-model-001`, `records-sop-version-minimum-fields-001`, `records-current-version-selection-001`, `records-superseded-version-isolation-001`, `records-rollback-revalidation-001`, `records-high-risk-knowledge-review-gate-001`, `records-version-change-log-001`, `general-knowledge-card-archive-001`, `general-offline-directory-index-001`|
|`DG-0917`|`records-superseded-version-isolation-001`, `records-retention-access-disposal-001`, `records-medical-observation-handover-boundary-001`, `education-paper-notebook-001`, `general-long-term-archive-check-001`, `general-offline-directory-index-001`, `education-offline-data-backup-001`, `general-member-info-backup-001`|
|`DG-0918`|`records-skill-authorization-minimum-001`, `records-skill-level-boundaries-001`, `records-supervised-practice-evidence-001`, `records-skill-expiry-refresher-001`, `records-new-member-learning-path-001`, `records-knowledge-transfer-handover-summary-001`, `general-small-team-handoff-001`, `organization-onboarding-001`, `organization-skill-redundancy-001`|

## 5. 旧 Guide Evidence 补充

只处理了规划允许的 3 个旧 Guide：

|Guide|新增 related_wiki|
|---|---|
|`DG-0144` 家庭成员信息包|`general-member-info-backup-001`, `records-retention-access-disposal-001`|
|`DG-0401` 五分钟技能教学|`records-skill-authorization-minimum-001`, `records-skill-level-boundaries-001`, `records-supervised-practice-evidence-001`, `records-skill-expiry-refresher-001`|
|`DG-0529` 成员档案权限分级|`records-retention-access-disposal-001`, `general-member-info-backup-001`, `general-medical-record-archive-001`|

没有修改 `DG-0145` 或其他旧 Guide。医疗观察边界的新 Wiki 只关联 Records 新 Guide；现有医疗记录 Wiki 继续保留 Medical Guide 主关系。

## 6. 双向关系审计

对全部 1139 篇 Wiki 与 844 个 Guide 独立计数：

|项目|结果|
|---|---:|
|Wiki `guide_links` 前向 pair|2387|
|Guide `related_wiki` 反向 pair|2387|
|`single_forward`|0|
|`single_reverse`|0|
|invalid Guide ID|0|
|invalid Wiki slug|0|

`scripts/audit_guides.py` 同时确认所有 `related_wiki` 均为真实 slug，关系完全对称。

## 7. PocketBase 同步

- 同步对象：32 个新增 `records-*` slug。
- 使用既有分类 `信息保存与长期重建`，category id 为 `r889fe40e3fb86b`。
- 每条同步 title、slug、category、summary、content、tags、risk_level、status 和 source。
- 13 篇复用 Wiki 没有创建副本；其正文和 PocketBase 内容字段无需重写。
- 未删除旧记录、未重建数据库、未修改 PocketBase schema。
- `tools/audit_wiki.py` 已逐 slug 比较 Markdown 与 PocketBase 的字段及正文，结果无差异。

|项目|同步前|同步后|
|---|---:|---:|
|Markdown Wiki|1107|1139|
|PocketBase `wiki_articles`|1107|1139|

`pocketbase/pb_data/data.db` 因本批 32 篇内容同步产生预期本地变动，保留原状，不主动清理或提交。

## 8. Guide Build

运行 `python3 tools/build_guides.py`：

- Guide 源 JSON：844
- `data/emergency_guides.json`：844
- `data/guide_index.json`：844
- 增量：836 -> 844

## 9. Audit 结果

运行：

```text
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

最终结果：

|Audit|对象数|errors|warnings|advisories|
|---|---:|---:|---:|---:|
|Wiki audit|1139 Markdown / 1139 PocketBase|0|0|0|
|Guide audit|844|0|0|0|

报告文件：

- `docs/knowledge/wiki_audit_2026-08-24.md`
- `docs/knowledge/guide_audit_2026-08-24.md`

## 10. Record Lifecycle 两轴确认

记录成熟度轴已实现为：

```text
RAW -> REVIEWED -> VERIFIED -> LESSON -> SOP_CANDIDATE
```

正式知识版本轴已实现为：

```text
CANDIDATE -> CURRENT -> SUPERSEDED / ARCHIVED
```

`APPROVED` 只保存为审核决定事件，不作为长期状态。系统可以检查字段、提供索引和提示复核，但不能自动把 RAW 提升为 VERIFIED，也不能自动批准或修改正式 Wiki / Guide。

## 11. 版本规则确认

- 只有 `CURRENT` 可以默认指导行动。
- `SUPERSEDED` 保留审计和学习价值，但从默认行动入口隔离。
- `ARCHIVED` 只用于历史查询，不参与默认行动。
- 回滚不把旧版原样改回 CURRENT；必须创建新的 CURRENT version，并记录 rollback reason、evidence、reviewer 和 effective date。
- 当前版无法唯一确认、旧版混入工作点或关键字段缺失时，Guide 明确要求停止执行相关高风险 SOP。

## 12. 高风险 Knowledge Promotion Gate

Medical、Fire、Energy、Chemical、Food Safety 和 Structural Safety 的现场经验不得直接成为正式知识。进入 CURRENT 前至少需要：

- 原始 evidence；
- 对应 domain reviewer；
- 可靠来源或 CURRENT 对照；
- 风险等级与适用条件；
- 明确 `stop_or_escalate`；
- 实际复现或 Field Test。

缺少任一门槛时维持 `SOP_CANDIDATE`、`LESSON` 或 `VERIFIED`，不得进入 CURRENT。

## 13. FT-02 / Core 与 Audio 边界

- FT-02 只作为 RAW note、observation、task result、audio evidence、GNSS metadata 和 device metadata 的采集入口。
- Core 只承担 review queue、cross-domain index、version index、archive 和 verification support。
- 自动时间、GNSS、device id 和文件 hash 只说明来源，不证明内容真实。
- Audio Log 是 RAW evidence；转写属于 derived record，必须保留 audio id、source、transcription version 和 uncertainty。
- FT-02 不自动验证事实，Core 不自动批准知识，语音中的“确认”不会自动形成 VERIFIED。

本批仅在 Wiki / Guide 中声明这些信息边界，没有修改 FT-02 firmware、Core application、sync protocol 或 UI。

## 14. Retrieval 冻结确认

本批未修改：

- Retrieval Pipeline 与 selector 行为；
- Prompt；
- query profile；
- `top_k`、selector limit、ranking、fallback；
- tests 与 Field Test fixture。

本批未运行 Retrieval Field Test，也未因 `DG-091x` 的未来命中情况增加 workaround。流程仍保持 `Knowledge -> Evidence -> Field Test -> Root Cause Review -> Minimal Apply`。

## 15. 未处理旧 Guide

按规划保留以下无 `related_wiki` 的跨域旧 Guide，不顺手补链：

- `DG-0123`, `DG-0125`, `DG-0126`, `DG-0128`
- `DG-0164`, `DG-0327`
- `DG-0391`, `DG-0395`, `DG-0396`
- `DG-0437`
- `DG-0513`, `DG-0515`

这些条目分别属于 Inventory / Storage、Medical / Care、Organization / Task allocation、Outdoor / Safety、Weather / Navigation、Agriculture / Storage，不属于 Records v0.1 的补链范围。

## 16. Dirty 文件说明

本批开始前工作树已存在与当前任务无关的 dirty / untracked 内容，包括：

- `data/lanternbox.db`
- `data/retrieval_query_profiles.json`
- `hardware/FT-02/**`
- Batch13 报告、Production Wiki / Guide、Retrieval tests 与 fixture
- 既有 coverage map、audit 报告及其他无关文件

上述既有内容均未清理、回退或顺手修复。`data/lanternbox.db` 未被本批访问或修改。`pocketbase/pb_data/data.db` 的 32 条新增文章属于本批允许且预期的同步变动；不主动清理或提交。

## 17. Batch14-D Field Test 建议

Batch14-D 应保持只测不调，先验证以下入口与边界：

1. RAW 记录最小字段、转述/亲历/设备来源和冲突证据处理。
2. 一次日志不能直接成为 VERIFIED、LESSON 或 CURRENT。
3. Audio Log 与转写只能作为 RAW / derived evidence。
4. 每日摘要、失败复盘、重复问题趋势和整改关闭。
5. CURRENT 选择、SUPERSEDED 隔离、回滚重新验证和 ARCHIVED 历史查询。
6. 记录访问分级、长期索引和纸面归档。
7. OBSERVE / ASSIST / SUPERVISED / INDEPENDENT 技能边界、过期降级和交接。
8. Medical 当前伤情、Production 当前 WIP、Repair 当前破损、Energy 当前电池、Agriculture 当前处置仍由相邻 domain 主导。

Field Test 应记录 expected Guide、Wiki evidence、主导 domain、安全停止线和 fallback。Batch14-D 不新增 Records profile；如出现检索问题，先留给后续 Root Cause Review 分类，不在 Field Test 阶段调参。

## 18. 阶段状态

Long-Term Records / Knowledge Transfer v0.1 Knowledge + Evidence 基础层已完成。Retrieval 状态尚未验证，等待 Batch14-D。
