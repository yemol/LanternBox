# LanternBox Batch13-C: Production System / Workshop Knowledge Apply Report

生成日期：2026-08-24

本阶段完成 Production System / Workshop v0.1 第一阶段 Apply，把 Manufacturing Foundation、Waste / Recycling、Long-Term Storage、Agriculture Second Stage 连接为可重复、可检查、可交接、可复盘的小规模生产流程。全程遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`；未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema、`.env` 或 Retrieval Field Test。

## 1. 新增 Wiki 清单

新增 41 篇，存放于 `wiki_import/production/`。每篇包含离线执行步骤、判断标准、停止线、替代方案、记录交接和相邻领域边界。

|slug|title|priority|risk_level|guide_links|
|---|---|---|---|---|
|`production-workshop-batch-consistency-check-001`|批次一致性复查|P1|normal|`[DG-0906]`|
|`production-workshop-batch-label-minimum-001`|生产批次标签最小字段|P0|normal|`[DG-0905, DG-0906]`|
|`production-workshop-bystander-exclusion-log-001`|非操作人员隔离记录|P1|high|`[DG-0909]`|
|`production-workshop-daily-resource-budget-001`|当日生产资源预算|P2|normal|`[DG-0903, DG-0910]`|
|`production-workshop-daily-start-check-001`|工坊每日开工总检查|P0|high|`[DG-0903]`|
|`production-workshop-daily-stop-shutdown-001`|工坊收工断电与封存|P0|high|`[DG-0904]`|
|`production-workshop-dust-spark-near-miss-log-001`|粉尘火花 near miss 记录|P1|high|`[DG-0909]`|
|`production-workshop-each-piece-check-001`|每件检查规则|P1|caution|`[DG-0906, DG-0907]`|
|`production-workshop-edge-burr-check-001`|锐边毛刺检查|P0|high|`[DG-0907, DG-0908]`|
|`production-workshop-end-of-day-status-log-001`|当日任务状态记录|P0|normal|`[DG-0904, DG-0910]`|
|`production-workshop-failed-batch-isolation-001`|失败批次隔离|P1|high|`[DG-0906, DG-0908]`|
|`production-workshop-fatigue-stop-line-001`|疲劳生产停止线|P1|high|`[DG-0909, DG-0910]`|
|`production-workshop-finished-goods-hold-zone-001`|成品待检区|P0|caution|`[DG-0905, DG-0907]`|
|`production-workshop-finished-goods-storage-transfer-001`|成品入库转交|P0|caution|`[DG-0905, DG-0907]`|
|`production-workshop-first-article-check-001`|首件确认|P1|caution|`[DG-0906]`|
|`production-workshop-function-check-before-release-001`|成品放行前功能检查|P0|caution|`[DG-0907]`|
|`production-workshop-incident-review-card-001`|工坊伤害与 near miss 复盘卡|P1|high|`[DG-0909]`|
|`production-workshop-loose-fastener-check-001`|松动连接复查|P0|high|`[DG-0907]`|
|`production-workshop-low-light-stop-line-001`|低照明生产停止线|P1|high|`[DG-0909]`|
|`production-workshop-material-count-before-work-001`|开工前材料清点|P0|caution|`[DG-0903, DG-0905]`|
|`production-workshop-material-reservation-line-001`|生产材料保留线|P0|high|`[DG-0905]`|
|`production-workshop-next-action-handover-001`|下一步动作交接|P1|normal|`[DG-0910]`|
|`production-workshop-nonconforming-item-quarantine-001`|不合格品隔离区|P0|high|`[DG-0908]`|
|`production-workshop-ppe-check-before-task-001`|生产防护用品检查|P1|high|`[DG-0909]`|
|`production-workshop-quality-record-minimum-001`|质量检查记录最小字段|P1|normal|`[DG-0907, DG-0908]`|
|`production-workshop-raw-material-intake-card-001`|原料入工坊卡|P0|caution|`[DG-0905]`|
|`production-workshop-repeat-step-card-001`|重复制作步骤卡|P1|normal|`[DG-0906]`|
|`production-workshop-rework-decision-line-001`|返工判断线|P0|caution|`[DG-0908]`|
|`production-workshop-risk-source-scan-001`|工坊风险源开工扫描|P0|high|`[DG-0903, DG-0909]`|
|`production-workshop-scrap-decision-line-001`|报废判断线|P0|high|`[DG-0908]`|
|`production-workshop-shift-handover-card-001`|工坊班次交接卡|P1|normal|`[DG-0910]`|
|`production-workshop-size-check-sheet-001`|尺寸复查表|P0|normal|`[DG-0907]`|
|`production-workshop-stability-check-sheet-001`|稳定性复查表|P0|high|`[DG-0907]`|
|`production-workshop-task-board-minimum-001`|工坊任务板最小字段|P0|normal|`[DG-0903, DG-0910]`|
|`production-workshop-template-size-standard-001`|模板与尺寸标准卡|P1|normal|`[DG-0906]`|
|`production-workshop-tool-count-before-work-001`|开工前工具清点|P0|high|`[DG-0903]`|
|`production-workshop-tool-state-handover-001`|工具状态交接|P1|caution|`[DG-0910]`|
|`production-workshop-unfinished-item-tag-001`|未完成品状态标签|P0|caution|`[DG-0904, DG-0910]`|
|`production-workshop-waste-transfer-card-001`|生产废料转 Waste 交接卡|P0|high|`[DG-0905, DG-0908]`|
|`production-workshop-wip-handover-tag-001`|半成品交接标签|P0|caution|`[DG-0905, DG-0910]`|
|`production-workshop-wip-zone-layout-001`|半成品暂存区布局|P0|caution|`[DG-0905]`|

覆盖分布：开工/收工 8 篇，原料/半成品/成品流转 9 篇，质量复查 8 篇，小批量重复制作 6 篇，安全记录 6 篇，任务交接 3 篇，资源预算 1 篇。

## 2. 新增 Guide 清单

新增 8 个 Guide，存放于 `data/guides/production/`。ID 从原最高 `DG-0902` 顺延至 `DG-0910`。

|Guide|title|priority|risk_level|related_wiki 数|
|---|---|---|---|---:|
|`DG-0903`|工坊开工前检查|P0|high|6|
|`DG-0904`|工坊收工与未完成品封存|P0|high|3|
|`DG-0905`|原料 / 半成品 / 成品流转|P0|high|9|
|`DG-0906`|小批量重复制作流程|P1|caution|7|
|`DG-0907`|成品质量复查|P0|high|9|
|`DG-0908`|不合格品 / 返工 / 报废判断|P0|high|7|
|`DG-0909`|工坊安全记录与 near miss 复盘|P1|high|7|
|`DG-0910`|工坊任务交接|P1|caution|9|

P0/high 入口均含明确停止、禁用、隔离或转相邻安全域条件。Guide 不包含具体制作、废弃物危险判断、长期储藏、工具维修、团队制度或长期知识治理步骤。

## 3. Guide-Wiki 双向关系

本批建立 57 条新增 Guide-Wiki evidence 边。Wiki `guide_links` 与 Guide `related_wiki` 逐条对称。

|Guide|双向关联 Wiki|
|---|---|
|`DG-0903`|`production-workshop-daily-start-check-001`, `production-workshop-task-board-minimum-001`, `production-workshop-tool-count-before-work-001`, `production-workshop-material-count-before-work-001`, `production-workshop-risk-source-scan-001`, `production-workshop-daily-resource-budget-001`|
|`DG-0904`|`production-workshop-unfinished-item-tag-001`, `production-workshop-daily-stop-shutdown-001`, `production-workshop-end-of-day-status-log-001`|
|`DG-0905`|`production-workshop-material-count-before-work-001`, `production-workshop-raw-material-intake-card-001`, `production-workshop-material-reservation-line-001`, `production-workshop-wip-zone-layout-001`, `production-workshop-wip-handover-tag-001`, `production-workshop-finished-goods-hold-zone-001`, `production-workshop-finished-goods-storage-transfer-001`, `production-workshop-waste-transfer-card-001`, `production-workshop-batch-label-minimum-001`|
|`DG-0906`|`production-workshop-batch-label-minimum-001`, `production-workshop-template-size-standard-001`, `production-workshop-first-article-check-001`, `production-workshop-repeat-step-card-001`, `production-workshop-each-piece-check-001`, `production-workshop-batch-consistency-check-001`, `production-workshop-failed-batch-isolation-001`|
|`DG-0907`|`production-workshop-finished-goods-hold-zone-001`, `production-workshop-finished-goods-storage-transfer-001`, `production-workshop-size-check-sheet-001`, `production-workshop-stability-check-sheet-001`, `production-workshop-edge-burr-check-001`, `production-workshop-loose-fastener-check-001`, `production-workshop-function-check-before-release-001`, `production-workshop-quality-record-minimum-001`, `production-workshop-each-piece-check-001`|
|`DG-0908`|`production-workshop-nonconforming-item-quarantine-001`, `production-workshop-waste-transfer-card-001`, `production-workshop-edge-burr-check-001`, `production-workshop-rework-decision-line-001`, `production-workshop-scrap-decision-line-001`, `production-workshop-quality-record-minimum-001`, `production-workshop-failed-batch-isolation-001`|
|`DG-0909`|`production-workshop-risk-source-scan-001`, `production-workshop-ppe-check-before-task-001`, `production-workshop-dust-spark-near-miss-log-001`, `production-workshop-low-light-stop-line-001`, `production-workshop-fatigue-stop-line-001`, `production-workshop-bystander-exclusion-log-001`, `production-workshop-incident-review-card-001`|
|`DG-0910`|`production-workshop-task-board-minimum-001`, `production-workshop-unfinished-item-tag-001`, `production-workshop-end-of-day-status-log-001`, `production-workshop-wip-handover-tag-001`, `production-workshop-fatigue-stop-line-001`, `production-workshop-shift-handover-card-001`, `production-workshop-tool-state-handover-001`, `production-workshop-next-action-handover-001`, `production-workshop-daily-resource-budget-001`|

独立关系校验：`single_forward=0`，`single_reverse=0`，`invalid Guide ID=0`，`invalid Wiki slug=0`。未为补数量强行连接相邻域 Wiki。

## 4. PocketBase 同步结果

- 已在单一事务中把 41 篇新增 Wiki 写入本地 `pocketbase/pb_data/data.db` 的 `wiki_articles`。
- 复用现有 `团队轮值与任务管理` category；未新增或修改 PocketBase schema。
- 同步字段：title、slug、category、summary、content、tags、risk_level、status、source。
- Markdown Wiki：`1107`；PocketBase `wiki_articles`：`1107`；逐字段审计一致。
- 本地 PocketBase 服务当时未运行，因此使用本地数据库事务同步；首次插入 41 条，内容增强后原位更新 41 条。

## 5. Audit 结果

|命令|结果|
|---|---|
|`python3 tools/audit_wiki.py`|markdown=1107，pocketbase=1107；errors=0，warnings=0，advisories=0|
|`python3 tools/build_guides.py`|Generated 836 Guides；Generated 836 Guide Index Items|
|`python3 scripts/audit_guides.py`|Guides=836；errors=0，warnings=0，advisories=0|

- `data/emergency_guides.json`：836 条。
- `data/guide_index.json`：836 条。
- 审计器合法 slug domain 原缺少规划要求的 `production`；按根因原则仅在 `tools/audit_wiki.py` 注册该知识域。未修改审计阈值、测试或数据库 schema。

## 6. 未覆盖内容

- 未实现大型工厂排产、自动化生产线、专业质量认证、危险化学品生产、武器制造或高压电气作业。
- 未引入 ERP、网络或云端库存依赖。
- 未扩写 Manufacturing 的具体制作步骤、Waste 的危险废弃物判断、Storage 的长期保存规则、Repair 的维修步骤、Records 的长期沉淀或 Organization 的人员制度。
- 未做 Retrieval Field Test，也未提前调整检索参数或新增 query profile。

## 7. 下一步 Field Test 建议

建议 Batch13-D 只做 Production System / Workshop Retrieval Field Test，先复现实际主位竞争，再决定是否需要最小检索入口修复。重点覆盖：

- “一批成品怎么检查和入库”：Production 主导复查、放行和转交，Storage 补长期入库。
- “半成品放哪里”：Production 主导 WIP 状态区与下一步，避免被 Storage 抢主位。
- “不合格品怎么处理”：Production 主导隔离、返工/降级/报废入口，Waste 补材料去向。
- “怎么做一个架子”和“工具坏了怎么办”：分别保持 Manufacturing、Repair 主导。
- “工坊交接怎么写”：Production 主导班次级实物交接，Records/Organization 不应抢主位。
- “低照明还要不要继续干活”：Production 触发停工，Safety/Fire/Energy 保留危险判断边界。
- 额外测试首件失败、整批隔离、材料保留线、疲劳停止、near miss 复工条件和成品未经检查等严格场景。

## 8. Dirty 文件说明

- `data/lanternbox.db` 在 Batch13-C 开始前已经 dirty；本阶段未写入、未清理、未提交。
- `pocketbase/pb_data/data.db` 因本阶段同步 41 篇 Wiki 发生预期本地数据变动；该路径未出现在 Git tracked diff 中，不主动提交或清理。
- `docs/knowledge/knowledge_coverage_map_v0_6.md` 与 `docs/knowledge/batch13_b_production_system_workshop_plan.md` 是前序批次已生成的未跟踪报告，本阶段未改写。
- 本阶段生成的审计报告为 `docs/knowledge/wiki_audit_2026-08-24.md` 与 `docs/knowledge/guide_audit_2026-08-24.md`。

## 9. 结论

Batch13-C 已完成 Production System / Workshop v0.1：41 篇 Wiki、8 个 Guide、57 条双向 evidence 关系、PocketBase 同步、`emergency_guides` / `guide_index` 更新及全量审计清零。下一阶段进入 Field Test，不提前修改 Retrieval。
