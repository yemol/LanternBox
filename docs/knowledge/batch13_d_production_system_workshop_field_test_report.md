# Batch13-D Production System / Workshop Retrieval Field Test Report

生成时间：2026-08-24T06:23:45.632265+00:00

## 1. 测试范围

本阶段只测试 Production System / Workshop v0.1 的本地 Retrieval 表现。使用现有 guide/wiki fetcher，选取 top 3 Guide，并按既有 related_wiki 规则与独立 Wiki 候选合并前 8 条 evidence；不调用 LLM，不生成最终回答。

覆盖开工、收工、原料/WIP/成品流转、质检、小批量、不合格品、安全记录和班次交接；observation 观察 Manufacturing、Repair、Waste、Storage、Medical、Energy 是否合理主导。

## 2. 测试资产

- fixture：`tests/fixtures/production_system_workshop_field_cases.json`
- 脚本：`scripts/test_production_system_workshop_field.py`
- 结果：`docs/knowledge/batch13_d_production_system_workshop_field_test_results.json`
- 报告：`docs/knowledge/batch13_d_production_system_workshop_field_test_report.md`

## 3. 基础审计结果

- `python3 tools/audit_wiki.py`：errors=0，warnings=0，advisories=0；Markdown/PocketBase=1107/1107。
- `python3 scripts/audit_guides.py`：errors=0，warnings=0，advisories=0；Guides=836。

## 4. Retrieval contract 结果

- `tests/test_retrieval_traceability.py` + `tests/test_retrieval_root_contract.py`：本轮独立命令验证：9 passed。
- 未修改现有 contract tests，未新增 profile contract tests。

## 5. Field Test 总表

- total：26；strict / observation：20 / 6
- pass / partial / fail：21 / 3 / 2

|case|type|verdict|top1 Guide|Wiki any|Production primary|cross-domain|reason|
|---|---|---|---|---|---|---|---|
|production_daily_start_check|strict|pass|DG-0903|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_tool_count_before_work|strict|pass|DG-0903|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_material_count_reserve|strict|pass|DG-0903|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_daily_closeout|strict|pass|DG-0904|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_unfinished_item_tag|strict|pass|DG-0905|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_raw_material_intake|strict|pass|DG-0905|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_wip_zone|strict|pass|DG-0905|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_finished_goods_hold|strict|pass|DG-0907|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_waste_transfer|strict|pass|DG-0905|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_first_article|strict|pass|DG-0906|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_batch_consistency|strict|pass|DG-0906|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_failed_batch|strict|pass|DG-0906|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_finished_quality_review|strict|pass|DG-0907|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_edge_burr_release|strict|pass|DG-0907|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_loose_connection|strict|pass|DG-0907|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_rework_scrap|strict|pass|DG-0908|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_low_light_stop|strict|pass|DG-0909|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_fatigue_stop|strict|pass|DG-0909|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_near_miss_review|strict|pass|DG-0909|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|production_shift_handover|strict|pass|DG-0910|True|True|True|预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。|
|observation_make_shelf|observation|partial|DG-0873|False|False|True|相邻领域主导合理，但 Production 补充 evidence 未进入。|
|observation_broken_saw|observation|partial|DG-0837|False|False|True|相邻领域主导合理，但 Production 补充 evidence 未进入。|
|observation_salvaged_wood|observation|fail|DG-0830|False|False|False|top1 未落在允许的相邻主域，cross-domain 主位不合理。|
|observation_long_term_finished_storage|observation|partial|DG-0895|False|False|True|相邻领域主导合理，但 Production 补充 evidence 未进入。|
|observation_workshop_injury|observation|fail|DG-0839|False|False|False|top1 未落在允许的相邻主域，cross-domain 主位不合理。|
|observation_low_energy_schedule|observation|pass|DG-0910|True|True|True|相邻领域主导合理，且 Production Guide/Wiki 作为补充进入。|

## 6. strict / observation 结果

- strict pass / partial / fail：20 / 0 / 0
- observation pass / partial / fail：1 / 3 / 2
- cross-domain 合理：24 / 26

## 7. Guide hit / Wiki hit / precise / Production primary rate

- Production Guide hit（strict）：100.0%
- expected Guide hit（strict）：100.0%
- Wiki full hit（strict）：65.0%
- Wiki any hit（strict）：100.0%
- Guide-Wiki precise（strict）：100.0%
- Production primary rate（strict）：100.0%

## 8. dangerous suggestion

- dangerous suggestion count：0。

## 9. Kiwix 越权

- Kiwix override count：0。本测试路径只选择 Guide/Wiki；若 Kiwix 出现即判 fail。

## 10. safety / fallback / record-check

- safety：100.0%
- fallback：100.0%
- record/check：100.0%

## 11. 主要失败模式

- Production 补充 evidence 未进入：5
- observation 主域偏离：2

失败只记录，不在 Batch13-D 修改 Wiki、Guide、profile、selector、ranking 或 fallback。

## 12. 是否建议进入 Batch13-E Root Cause Review

建议进入 Batch13-E Root Cause Review。存在 strict partial/fail 或 observation 主域错误，需要区分 Production 检索入口缺口、selector/ranking、evidence 进入不足和相邻领域抢主位；本阶段不新增 profile。

## 13. 逐条结果

### production_daily_start_check

- type：strict
- query：今天工坊准备开工，应该先检查什么？
- expected Guide：DG-0903
- expected Wiki any：production-workshop-daily-start-check-001、production-workshop-material-count-before-work-001、production-workshop-risk-source-scan-001、production-workshop-task-board-minimum-001、production-workshop-tool-count-before-work-001
- actual top Guides：DG-0903、DG-0871、DG-0904
- actual selected Wiki：production-workshop-daily-start-check-001、production-workshop-task-board-minimum-001、production-workshop-tool-count-before-work-001、production-workshop-material-count-before-work-001、production-workshop-risk-source-scan-001、production-workshop-daily-resource-budget-001、repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_tool_count_before_work

- type：strict
- query：开工前发现几个关键工具状态不清，要怎么清点？
- expected Guide：DG-0903
- expected Wiki any：production-workshop-tool-count-before-work-001
- actual top Guides：DG-0903、DG-0904、DG-0871
- actual selected Wiki：production-workshop-daily-start-check-001、production-workshop-task-board-minimum-001、production-workshop-tool-count-before-work-001、production-workshop-material-count-before-work-001、production-workshop-risk-source-scan-001、production-workshop-daily-resource-budget-001、production-workshop-unfinished-item-tag-001、production-workshop-daily-stop-shutdown-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_material_count_reserve

- type：strict
- query：今天生产前怎么确认材料够用，又不要把关键材料用光？
- expected Guide：DG-0903、DG-0905
- expected Wiki any：production-workshop-material-count-before-work-001、production-workshop-material-reservation-line-001
- actual top Guides：DG-0903、DG-0904
- actual selected Wiki：production-workshop-daily-start-check-001、production-workshop-task-board-minimum-001、production-workshop-tool-count-before-work-001、production-workshop-material-count-before-work-001、production-workshop-risk-source-scan-001、production-workshop-daily-resource-budget-001、production-workshop-unfinished-item-tag-001、production-workshop-daily-stop-shutdown-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_daily_closeout

- type：strict
- query：工坊今天做不完了，收工前怎么封存？
- expected Guide：DG-0904
- expected Wiki any：production-workshop-daily-stop-shutdown-001、production-workshop-end-of-day-status-log-001、production-workshop-unfinished-item-tag-001
- actual top Guides：DG-0904、DG-0903、DG-0871
- actual selected Wiki：production-workshop-unfinished-item-tag-001、production-workshop-daily-stop-shutdown-001、production-workshop-end-of-day-status-log-001、production-workshop-daily-start-check-001、production-workshop-task-board-minimum-001、production-workshop-tool-count-before-work-001、production-workshop-material-count-before-work-001、production-workshop-risk-source-scan-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_unfinished_item_tag

- type：strict
- query：半成品还没做完，明天继续，标签最少要写什么？
- expected Guide：DG-0904、DG-0905、DG-0910
- expected Wiki any：production-workshop-unfinished-item-tag-001、production-workshop-wip-handover-tag-001
- actual top Guides：DG-0905
- actual selected Wiki：production-workshop-raw-material-intake-card-001、production-workshop-wip-zone-layout-001、production-workshop-wip-handover-tag-001、production-workshop-finished-goods-hold-zone-001、production-workshop-waste-transfer-card-001、production-workshop-batch-label-minimum-001、production-workshop-material-count-before-work-001、production-workshop-material-reservation-line-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_raw_material_intake

- type：strict
- query：从材料池拿来的原料进工坊前要怎么登记？
- expected Guide：DG-0905
- expected Wiki any：production-workshop-batch-label-minimum-001、production-workshop-raw-material-intake-card-001
- actual top Guides：DG-0905、DG-0894、DG-0892
- actual selected Wiki：production-workshop-raw-material-intake-card-001、production-workshop-wip-zone-layout-001、production-workshop-wip-handover-tag-001、production-workshop-finished-goods-hold-zone-001、production-workshop-waste-transfer-card-001、production-workshop-batch-label-minimum-001、production-workshop-material-count-before-work-001、production-workshop-material-reservation-line-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_wip_zone

- type：strict
- query：半成品应该放在哪里，怎么避免和成品混在一起？
- expected Guide：DG-0905
- expected Wiki any：production-workshop-finished-goods-hold-zone-001、production-workshop-wip-handover-tag-001、production-workshop-wip-zone-layout-001
- actual top Guides：DG-0905
- actual selected Wiki：production-workshop-raw-material-intake-card-001、production-workshop-wip-zone-layout-001、production-workshop-wip-handover-tag-001、production-workshop-finished-goods-hold-zone-001、production-workshop-waste-transfer-card-001、production-workshop-batch-label-minimum-001、production-workshop-material-count-before-work-001、production-workshop-material-reservation-line-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_finished_goods_hold

- type：strict
- query：一批东西刚做完，还没检查，能不能直接入库？
- expected Guide：DG-0905、DG-0907
- expected Wiki any：production-workshop-finished-goods-hold-zone-001、production-workshop-finished-goods-storage-transfer-001、production-workshop-function-check-before-release-001
- actual top Guides：DG-0907、DG-0908、DG-0899
- actual selected Wiki：production-workshop-finished-goods-hold-zone-001、production-workshop-finished-goods-storage-transfer-001、production-workshop-size-check-sheet-001、production-workshop-stability-check-sheet-001、production-workshop-edge-burr-check-001、production-workshop-loose-fastener-check-001、production-workshop-function-check-before-release-001、production-workshop-quality-record-minimum-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_waste_transfer

- type：strict
- query：工坊产生的边角料和废料要怎么交给废料区？
- expected Guide：DG-0905、DG-0908
- expected Wiki any：production-workshop-nonconforming-item-quarantine-001、production-workshop-waste-transfer-card-001
- actual top Guides：DG-0905
- actual selected Wiki：production-workshop-raw-material-intake-card-001、production-workshop-wip-zone-layout-001、production-workshop-wip-handover-tag-001、production-workshop-finished-goods-hold-zone-001、production-workshop-waste-transfer-card-001、production-workshop-batch-label-minimum-001、production-workshop-material-count-before-work-001、production-workshop-material-reservation-line-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_first_article

- type：strict
- query：要连续做十个同样的小支架，为什么要先做首件确认？
- expected Guide：DG-0906
- expected Wiki any：production-workshop-first-article-check-001、production-workshop-repeat-step-card-001、production-workshop-template-size-standard-001
- actual top Guides：DG-0906
- actual selected Wiki：production-workshop-batch-label-minimum-001、production-workshop-template-size-standard-001、production-workshop-first-article-check-001、production-workshop-repeat-step-card-001、production-workshop-each-piece-check-001、production-workshop-batch-consistency-check-001、production-workshop-failed-batch-isolation-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_batch_consistency

- type：strict
- query：一批零件做出来大小不太一样，应该怎么复查？
- expected Guide：DG-0906、DG-0907
- expected Wiki any：production-workshop-batch-consistency-check-001、production-workshop-size-check-sheet-001
- actual top Guides：DG-0906
- actual selected Wiki：production-workshop-batch-label-minimum-001、production-workshop-template-size-standard-001、production-workshop-first-article-check-001、production-workshop-repeat-step-card-001、production-workshop-each-piece-check-001、production-workshop-batch-consistency-check-001、production-workshop-failed-batch-isolation-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_failed_batch

- type：strict
- query：批量制作中发现前三个都有同样问题，整批要怎么处理？
- expected Guide：DG-0906、DG-0908
- expected Wiki any：production-workshop-failed-batch-isolation-001、production-workshop-nonconforming-item-quarantine-001
- actual top Guides：DG-0906
- actual selected Wiki：production-workshop-batch-label-minimum-001、production-workshop-template-size-standard-001、production-workshop-first-article-check-001、production-workshop-repeat-step-card-001、production-workshop-each-piece-check-001、production-workshop-batch-consistency-check-001、production-workshop-failed-batch-isolation-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_finished_quality_review

- type：strict
- query：成品准备使用前，质量复查最少看哪些项目？
- expected Guide：DG-0907
- expected Wiki any：production-workshop-edge-burr-check-001、production-workshop-function-check-before-release-001、production-workshop-loose-fastener-check-001、production-workshop-size-check-sheet-001、production-workshop-stability-check-sheet-001
- actual top Guides：DG-0907、DG-0908、DG-0895
- actual selected Wiki：production-workshop-finished-goods-hold-zone-001、production-workshop-finished-goods-storage-transfer-001、production-workshop-size-check-sheet-001、production-workshop-stability-check-sheet-001、production-workshop-edge-burr-check-001、production-workshop-loose-fastener-check-001、production-workshop-function-check-before-release-001、production-workshop-quality-record-minimum-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_edge_burr_release

- type：strict
- query：做好的东西边缘有毛刺和突出的钉子，能不能先放行使用？
- expected Guide：DG-0907、DG-0908
- expected Wiki any：production-workshop-edge-burr-check-001、production-workshop-rework-decision-line-001
- actual top Guides：DG-0907、DG-0908、DG-0895
- actual selected Wiki：production-workshop-finished-goods-hold-zone-001、production-workshop-finished-goods-storage-transfer-001、production-workshop-size-check-sheet-001、production-workshop-stability-check-sheet-001、production-workshop-edge-burr-check-001、production-workshop-loose-fastener-check-001、production-workshop-function-check-before-release-001、production-workshop-quality-record-minimum-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_loose_connection

- type：strict
- query：成品螺丝和扎带有松动，入库前怎么办？
- expected Guide：DG-0907、DG-0908
- expected Wiki any：production-workshop-finished-goods-storage-transfer-001、production-workshop-loose-fastener-check-001、production-workshop-rework-decision-line-001
- actual top Guides：DG-0907、DG-0908、DG-0899
- actual selected Wiki：production-workshop-finished-goods-hold-zone-001、production-workshop-finished-goods-storage-transfer-001、production-workshop-size-check-sheet-001、production-workshop-stability-check-sheet-001、production-workshop-edge-burr-check-001、production-workshop-loose-fastener-check-001、production-workshop-function-check-before-release-001、production-workshop-quality-record-minimum-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_rework_scrap

- type：strict
- query：不合格的半成品是返工、降级还是报废，怎么判断？
- expected Guide：DG-0908
- expected Wiki any：production-workshop-nonconforming-item-quarantine-001、production-workshop-rework-decision-line-001、production-workshop-scrap-decision-line-001
- actual top Guides：DG-0908、DG-0907、DG-0895
- actual selected Wiki：production-workshop-nonconforming-item-quarantine-001、production-workshop-waste-transfer-card-001、production-workshop-edge-burr-check-001、production-workshop-rework-decision-line-001、production-workshop-scrap-decision-line-001、production-workshop-quality-record-minimum-001、production-workshop-failed-batch-isolation-001、production-workshop-finished-goods-hold-zone-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_low_light_stop

- type：strict
- query：工坊灯光很暗，但还想继续切割和钻孔，可以吗？
- expected Guide：DG-0909
- expected Wiki any：production-workshop-low-light-stop-line-001、production-workshop-risk-source-scan-001
- actual top Guides：DG-0909
- actual selected Wiki：production-workshop-risk-source-scan-001、production-workshop-ppe-check-before-task-001、production-workshop-dust-spark-near-miss-log-001、production-workshop-low-light-stop-line-001、production-workshop-fatigue-stop-line-001、production-workshop-bystander-exclusion-log-001、production-workshop-incident-review-card-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_fatigue_stop

- type：strict
- query：连续做了很久开始手抖，还能继续做高风险加工吗？
- expected Guide：DG-0909、DG-0910
- expected Wiki any：production-workshop-fatigue-stop-line-001、production-workshop-incident-review-card-001
- actual top Guides：DG-0909
- actual selected Wiki：production-workshop-risk-source-scan-001、production-workshop-ppe-check-before-task-001、production-workshop-dust-spark-near-miss-log-001、production-workshop-low-light-stop-line-001、production-workshop-fatigue-stop-line-001、production-workshop-bystander-exclusion-log-001、production-workshop-incident-review-card-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_near_miss_review

- type：strict
- query：刚才打磨时火花和碎屑差点伤到人，复工前要记录什么？
- expected Guide：DG-0909
- expected Wiki any：production-workshop-dust-spark-near-miss-log-001、production-workshop-incident-review-card-001、production-workshop-ppe-check-before-task-001
- actual top Guides：DG-0909、DG-0871
- actual selected Wiki：production-workshop-risk-source-scan-001、production-workshop-ppe-check-before-task-001、production-workshop-dust-spark-near-miss-log-001、production-workshop-low-light-stop-line-001、production-workshop-fatigue-stop-line-001、production-workshop-bystander-exclusion-log-001、production-workshop-incident-review-card-001、repair-manufacturing-zone-layout-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### production_shift_handover

- type：strict
- query：下一班要接手未完成的工坊任务，交接卡最少写什么？
- expected Guide：DG-0910
- expected Wiki any：production-workshop-next-action-handover-001、production-workshop-shift-handover-card-001、production-workshop-tool-state-handover-001、production-workshop-wip-handover-tag-001
- actual top Guides：DG-0910
- actual selected Wiki：production-workshop-shift-handover-card-001、production-workshop-tool-state-handover-001、production-workshop-next-action-handover-001、production-workshop-wip-handover-tag-001、production-workshop-unfinished-item-tag-001、production-workshop-task-board-minimum-001、production-workshop-end-of-day-status-log-001、production-workshop-fatigue-stop-line-001
- verdict / reason：pass / 预期 Production Guide 为 top1，且至少一篇预期 Wiki 进入 evidence。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：Production 主导合理。
- root cause：无

### observation_make_shelf

- type：observation
- query：我想做一个结实的架子，具体怎么切木板和连接？
- expected Guide：无
- expected Wiki any：production-workshop-first-article-check-001、production-workshop-function-check-before-release-001
- actual top Guides：DG-0873
- actual selected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-wood-selection-001
- verdict / reason：partial / 相邻领域主导合理，但 Production 补充 evidence 未进入。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=manufacturing；实际=manufacturing,repair,tools；Production补充=无。
- root cause：Production 补充 evidence 未进入

### observation_broken_saw

- type：observation
- query：锯子坏了还能不能继续用，怎么修？
- expected Guide：无
- expected Wiki any：production-workshop-tool-count-before-work-001、production-workshop-tool-state-handover-001
- actual top Guides：DG-0837、DG-0840、DG-0873
- actual selected Wiki：repair-dangerous-tool-temporary-isolation-001、repair-dangerous-tool-borrow-permission-001、repair-sharp-blunt-tool-zone-storage-001、repair-damaged-tool-disable-tag-001、repair-damaged-tool-report-isolation-001、repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001
- verdict / reason：partial / 相邻领域主导合理，但 Production 补充 evidence 未进入。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=repair,tools；实际=repair,safety,tools；Production补充=无。
- root cause：Production 补充 evidence 未进入

### observation_salvaged_wood

- type：observation
- query：这块旧木板从废料堆拿来，能不能拿去做东西？
- expected Guide：无
- expected Wiki any：production-workshop-raw-material-intake-card-001
- actual top Guides：DG-0830、DG-0315、DG-0058
- actual selected Wiki：agriculture-wood-ash-fertilizer-boundary-001、food-post-flood-food-high-risk-001
- verdict / reason：fail / top1 未落在允许的相邻主域，cross-domain 主位不合理。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=manufacturing,waste；实际=other；Production补充=无。
- root cause：observation 主域偏离、Production 补充 evidence 未进入

### observation_long_term_finished_storage

- type：observation
- query：检查合格的成品要长期放在哪里，怎么保存？
- expected Guide：无
- expected Wiki any：production-workshop-batch-label-minimum-001、production-workshop-finished-goods-storage-transfer-001
- actual top Guides：DG-0895、DG-0899、DG-0857
- actual selected Wiki：general-storage-zone-basic-layout-001、general-storage-label-minimum-fields-001、general-storage-moisture-daily-check-001、general-storage-pest-rodent-check-001、general-storage-fifo-shelf-rule-001、repair-storage-shelf-overload-warning-001、repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001
- verdict / reason：partial / 相邻领域主导合理，但 Production 补充 evidence 未进入。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=storage；实际=records,repair,storage；Production补充=无。
- root cause：Production 补充 evidence 未进入

### observation_workshop_injury

- type：observation
- query：工坊切割时有人被划伤流血了，应该先做什么？
- expected Guide：无
- expected Wiki any：production-workshop-daily-stop-shutdown-001、production-workshop-incident-review-card-001
- actual top Guides：DG-0839、DG-0873、DG-0871
- actual selected Wiki：repair-knife-saw-clear-zone-001、repair-bystander-position-boundary-001、repair-floor-clutter-trip-check-001、repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001
- verdict / reason：fail / top1 未落在允许的相邻主域，cross-domain 主位不合理。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=medical；实际=repair,safety,tools；Production补充=无。
- root cause：observation 主域偏离、Production 补充 evidence 未进入

### observation_low_energy_schedule

- type：observation
- query：今天电量不够，工坊任务怎么安排？
- expected Guide：无
- expected Wiki any：production-workshop-daily-resource-budget-001、production-workshop-task-board-minimum-001
- actual top Guides：DG-0910、DG-0904、DG-0903
- actual selected Wiki：production-workshop-shift-handover-card-001、production-workshop-tool-state-handover-001、production-workshop-next-action-handover-001、production-workshop-wip-handover-tag-001、production-workshop-unfinished-item-tag-001、production-workshop-task-board-minimum-001、production-workshop-end-of-day-status-log-001、production-workshop-fatigue-stop-line-001
- verdict / reason：pass / 相邻领域主导合理，且 Production Guide/Wiki 作为补充进入。
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=energy,production；实际=production,records,repair,tools；Production补充=有。
- root cause：无

## 14. 边界声明

本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、`emergency_guides.json`、`guide_index.json`、`.env` 或 `data/lanternbox.db`；未运行答案生成。
