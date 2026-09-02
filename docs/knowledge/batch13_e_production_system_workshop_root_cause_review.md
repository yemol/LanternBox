# LanternBox Batch13-E: Production System / Workshop Retrieval Root Cause Review

生成日期：2026-08-24

本阶段只做根因审查。参考 Batch13-B/C/D 规划、Apply 与 Field Test 结果；未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、索引、tests、`.env`、`data/lanternbox.db` 或其他 dirty 文件。

## 1. Field Test 总结

|指标|结果|
|---|---:|
|total|26|
|strict / observation|20 / 6|
|pass / partial / fail|7 / 5 / 14|
|strict pass / partial / fail|6 / 3 / 11|
|observation pass / partial / fail|1 / 2 / 3|
|Production strict 主位率|35%|
|expected Guide hit|45%|
|Wiki any / full hit|35% / 20%|
|Guide-Wiki precise|30%|
|dangerous suggestion / Kiwix 越权|0 / 0|
|safety / fallback / record-check|100% / 100% / 100%|

### 总体判断

|问题|判断|
|---|---|
|知识缺失？|否。41 篇 Production Wiki 与 8 个 Guide 已覆盖全部 strict 主题；不存在 Q 类缺口。|
|Guide-Wiki 缺链？|否。Batch13-C 双向关系为 57 条，single forward/reverse 均为 0。问题是正确 Guide 未选中，或 top1 旧 Guide 的 related Wiki 先占满前 8。|
|Production query profile 缺失？|是，属于主因。DG-0909 在 3 个目标 case 中候选命中为 0；DG-0905/0907/0908 也在多个明确 Production query 中不进前 8。|
|selected evidence priority / top8 截断？|是。`production_daily_closeout` 与 `production_shift_handover` 中正确 Guide 已在 top3，但 top1 的 DG-0871 / DG-0902 related Wiki 先占满前 8。|
|相邻 stable domain 抢主位？|是。Manufacturing、Waste、Storage、Records/Organization 在其旧 profile 触发时具有显著优势；部分合理，部分与 Production 流程语义冲突。|
|fixture expected 过窄？|仅 `production_unfinished_item_tag` 明显过窄；DG-0905 对 WIP 标签和流转是合理 top1。其他 strict expected 不建议放宽。|
|observation fail 含义？|O3、O5 是相邻域自身入口偏移；O4 是 Production 吞掉 Storage。都不应通过扩大 Production 覆盖解决。|
|安全或 Kiwix 问题？|否。safety/fallback/record-check 100%，dangerous suggestion 与 Kiwix override 均为 0。|

根因优先级：**profile/主位选择 > top1 related Wiki 截断 > 少量 related_wiki 顺序 > fixture 一处过窄**。不应新增知识，也不应扩大 top_k 或 selector limit。

## 2. fail / partial 逐条根因

分类代码：
- **A**：Production profile 缺失
- **B**：correct Production Guide exists but not selected
- **C**：correct Production Guide in candidates but not top3
- **D**：correct Production Guide in top3 but not top1
- **E**：correct Production Wiki exists but not selected
- **F**：correct Production Wiki blocked by old Guide related_wiki
- **G**：Guide-Wiki related_wiki 顺序问题
- **H**：Manufacturing 抢主位
- **I**：Waste / Recycling 抢主位
- **J**：Long-Term Storage 抢主位
- **K**：Tools / Repair 抢主位
- **L**：Records / Organization 抢主位
- **M**：Safety / Fire / Energy 抢主位
- **N**：Medical 应主导但未主导
- **O**：Observation expected 过窄
- **P**：Fixture query 语义过宽或歧义
- **Q**：确实缺 Wiki / Guide
- **R**：合理 observation，不修

### production_daily_start_check

- query：今天工坊准备开工，应该先检查什么？
- expected Guide：DG-0903
- expected Wiki：production-workshop-daily-start-check-001, production-workshop-material-count-before-work-001, production-workshop-risk-source-scan-001, production-workshop-task-board-minimum-001, production-workshop-tool-count-before-work-001
- actual top1 / top3：DG-0871 / DG-0871, DG-0904, DG-0878
- actual selected Wiki：repair-manufacturing-zone-layout-001, repair-manufacturing-workbench-start-check-001, repair-manufacturing-dust-spark-stop-line-001, repair-manufacturing-cut-drill-clear-zone-001, repair-manufacturing-ppe-minimum-001, repair-manufacturing-bystander-exclusion-001, repair-manufacturing-damaged-tool-stop-001, repair-manufacturing-raw-finished-zones-001
- Guide candidates 中 Production：DG-0904@2
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_workspace_safety
- root cause：A, H, E, F
- 最小修复建议：新增 start/close 复合 profile，使 DG-0903 主导；不改 Wiki 顺序。

### production_daily_closeout

- query：工坊今天做不完了，收工前怎么封存？
- expected Guide：DG-0904
- expected Wiki：production-workshop-daily-stop-shutdown-001, production-workshop-end-of-day-status-log-001, production-workshop-unfinished-item-tag-001
- actual top1 / top3：DG-0871 / DG-0871, DG-0904, DG-0878
- actual selected Wiki：repair-manufacturing-zone-layout-001, repair-manufacturing-workbench-start-check-001, repair-manufacturing-dust-spark-stop-line-001, repair-manufacturing-cut-drill-clear-zone-001, repair-manufacturing-ppe-minimum-001, repair-manufacturing-bystander-exclusion-001, repair-manufacturing-damaged-tool-stop-001, repair-manufacturing-raw-finished-zones-001
- Guide candidates 中 Production：DG-0904@2
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_workspace_safety
- root cause：D, F, H
- 最小修复建议：同一 start/close profile 提升 DG-0904 为 top1；其 3 个 related Wiki 已按核心顺序。

### production_unfinished_item_tag

- query：半成品还没做完，明天继续，标签最少要写什么？
- expected Guide：DG-0904, DG-0910
- expected Wiki：production-workshop-unfinished-item-tag-001, production-workshop-wip-handover-tag-001
- actual top1 / top3：DG-0905 / DG-0905, DG-0878, DG-0904
- actual selected Wiki：production-workshop-material-count-before-work-001, production-workshop-raw-material-intake-card-001, production-workshop-material-reservation-line-001, production-workshop-wip-zone-layout-001, production-workshop-wip-handover-tag-001, production-workshop-finished-goods-hold-zone-001, production-workshop-finished-goods-storage-transfer-001, production-workshop-waste-transfer-card-001
- Guide candidates 中 Production：DG-0905@1, DG-0904@3, DG-0907@4, DG-0908@7
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：D, O
- 最小修复建议：fixture 将 DG-0905 加入 acceptable expected；当前 WIP 标签已进入，不做检索修复。

### production_raw_material_intake

- query：从材料池拿来的原料进工坊前要怎么登记？
- expected Guide：DG-0905
- expected Wiki：production-workshop-batch-label-minimum-001, production-workshop-raw-material-intake-card-001
- actual top1 / top3：DG-0894 / DG-0894, DG-0892, DG-0893
- actual selected Wiki：general-recycling-material-pool-ledger-001, general-waste-reuse-failure-record-001, general-waste-disposal-handover-card-001, general-waste-source-hazard-log-001, general-waste-source-label-minimum-001, hygiene-waste-organic-pest-odor-daily-check-001, repair-recycling-fasteners-small-parts-sort-001, repair-recycling-material-downgrade-label-001
- Guide candidates 中 Production：DG-0905@4, DG-0904@5, DG-0903@6, DG-0909@7, DG-0910@8
- Wiki candidates 中 Production：无
- 已触发 profile：waste_material_pool_reuse
- root cause：A, C, I, E, F
- 最小修复建议：新增 material/WIP flow profile，DG-0905 主导、DG-0894/0892 secondary；把 raw intake 与 batch label 提前。

### production_finished_goods_hold

- query：一批东西刚做完，还没检查，能不能直接入库？
- expected Guide：DG-0905, DG-0907
- expected Wiki：production-workshop-finished-goods-hold-zone-001, production-workshop-finished-goods-storage-transfer-001, production-workshop-function-check-before-release-001
- actual top1 / top3：DG-0058 / DG-0058, DG-0865, DG-0556
- actual selected Wiki：food-post-flood-food-high-risk-001, navigation-route-risk-score-001, navigation-flood-terrain-risk-001, navigation-alternate-route-selection-001, navigation-route-closure-recheck-001, navigation-flood-road-stop-line-001, navigation-night-route-risk-001, navigation-night-movement-stop-line-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, E, P
- 最小修复建议：新增 quality release profile，以“未检查/待检/入库前”为复合触发；Storage secondary。

### production_waste_transfer

- query：工坊产生的边角料和废料要怎么交给废料区？
- expected Guide：DG-0905, DG-0908
- expected Wiki：production-workshop-nonconforming-item-quarantine-001, production-workshop-waste-transfer-card-001
- actual top1 / top3：DG-0888 / DG-0888, DG-0878, DG-0904
- actual selected Wiki：hygiene-waste-sharp-glass-temporary-container-001, hygiene-waste-metal-edge-scrap-isolation-001, hygiene-waste-child-access-control-001, repair-recycling-metal-sheet-intake-check-001, repair-manufacturing-end-clean-count-001, repair-manufacturing-damaged-tool-stop-001, repair-manufacturing-bystander-exclusion-001, repair-manufacturing-raw-finished-zones-001
- Guide candidates 中 Production：DG-0904@3, DG-0903@4, DG-0909@5, DG-0910@6
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, I, E, F
- 最小修复建议：material/WIP flow profile 覆盖“工坊产生+转交”，Waste 继续负责分类并作为 secondary。

### production_first_article

- query：要连续做十个同样的小支架，为什么要先做首件确认？
- expected Guide：DG-0906
- expected Wiki：production-workshop-first-article-check-001, production-workshop-repeat-step-card-001, production-workshop-template-size-standard-001
- actual top1 / top3：DG-0876 / DG-0876, DG-0877, DG-0801
- actual selected Wiki：repair-manufacturing-metal-scrap-check-001, repair-manufacturing-metal-edge-safe-001, repair-manufacturing-thin-metal-drill-001, repair-manufacturing-corroded-metal-reuse-001, repair-manufacturing-material-prep-001, repair-manufacturing-metal-bracket-check-001, repair-manufacturing-thin-metal-bend-001, repair-manufacturing-metal-join-choice-001
- Guide candidates 中 Production：DG-0906@4
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, C, H, E, F
- 最小修复建议：新增 batch repetition profile；DG-0906 主导，Manufacturing secondary。

### production_batch_consistency

- query：一批零件做出来大小不太一样，应该怎么复查？
- expected Guide：DG-0906, DG-0907
- expected Wiki：production-workshop-batch-consistency-check-001, production-workshop-size-check-sheet-001
- actual top1 / top3：DG-0309 / DG-0309, DG-0051, DG-0052
- actual selected Wiki：无
- Guide candidates 中 Production：DG-0906@6
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, C, E, P
- 最小修复建议：batch repetition profile 覆盖“一批+尺寸不一致/复查”；不扩大 top_k。

### production_edge_burr_release

- query：做好的东西边缘有毛刺和突出的钉子，能不能先放行使用？
- expected Guide：DG-0907, DG-0908
- expected Wiki：production-workshop-edge-burr-check-001, production-workshop-rework-decision-line-001
- actual top1 / top3：DG-0058 / DG-0058, DG-0376, DG-0865
- actual selected Wiki：food-post-flood-food-high-risk-001, repair-fastener-washer-storage-001, repair-screws-and-nuts-001, navigation-route-risk-score-001, navigation-flood-terrain-risk-001, navigation-alternate-route-selection-001, navigation-route-closure-recheck-001, navigation-flood-road-stop-line-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, E, P
- 最小修复建议：quality release profile 覆盖“成品+毛刺/突钉+放行”；DG-0908 secondary。

### production_loose_connection

- query：成品螺丝和扎带有松动，入库前怎么办？
- expected Guide：DG-0907, DG-0908
- expected Wiki：production-workshop-finished-goods-storage-transfer-001, production-workshop-loose-fastener-check-001, production-workshop-rework-decision-line-001
- actual top1 / top3：DG-0875 / DG-0875, DG-0874, DG-0877
- actual selected Wiki：repair-manufacturing-connection-choice-001, repair-manufacturing-wood-screw-join-001, repair-manufacturing-fastener-reuse-boundary-001, repair-manufacturing-metal-join-choice-001, repair-manufacturing-fabric-rope-leather-check-001, repair-manufacturing-wood-selection-001, repair-manufacturing-wood-drill-position-001, repair-manufacturing-frame-square-check-001
- Guide candidates 中 Production：DG-0907@4
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_connection_load_check
- root cause：A, C, H, E, F
- 最小修复建议：quality release profile 覆盖“成品+松动+入库前”；Manufacturing 连接方法 secondary。

### production_low_light_stop

- query：工坊灯光很暗，但还想继续切割和钻孔，可以吗？
- expected Guide：DG-0909
- expected Wiki：production-workshop-low-light-stop-line-001, production-workshop-risk-source-scan-001
- actual top1 / top3：DG-0873 / DG-0873, DG-0871
- actual selected Wiki：repair-manufacturing-clamp-before-processing-001, repair-manufacturing-wood-saw-support-001, repair-manufacturing-wood-drill-position-001, repair-manufacturing-wood-measure-mark-001, repair-manufacturing-cut-drill-clear-zone-001, repair-manufacturing-wood-selection-001, repair-manufacturing-zone-layout-001, repair-manufacturing-workbench-start-check-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_workspace_safety, manufacturing_cutting_clamping
- root cause：A, H, M, E, F
- 最小修复建议：新增 workshop safety incident profile；低照明加工由 DG-0909 触发停工，Manufacturing/Safety secondary。

### production_fatigue_stop

- query：连续做了很久开始手抖，还能继续做高风险加工吗？
- expected Guide：DG-0909, DG-0910
- expected Wiki：production-workshop-fatigue-stop-line-001, production-workshop-incident-review-card-001
- actual top1 / top3：DG-0689 / DG-0689, DG-0394, DG-0136
- actual selected Wiki：organization-night-safety-001, organization-outside-movement-001, organization-return-check-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：A, L, E, F
- 最小修复建议：同一 safety incident profile 覆盖“工坊/加工+手抖/疲劳”，DG-0689 作为组织层补充。

### production_near_miss_review

- query：刚才打磨时火花和碎屑差点伤到人，复工前要记录什么？
- expected Guide：DG-0909
- expected Wiki：production-workshop-dust-spark-near-miss-log-001, production-workshop-incident-review-card-001, production-workshop-ppe-check-before-task-001
- actual top1 / top3：DG-0871 / DG-0871
- actual selected Wiki：repair-manufacturing-zone-layout-001, repair-manufacturing-workbench-start-check-001, repair-manufacturing-dust-spark-stop-line-001, repair-manufacturing-cut-drill-clear-zone-001, repair-manufacturing-ppe-minimum-001, repair-manufacturing-bystander-exclusion-001, repair-manufacturing-damaged-tool-stop-001, repair-manufacturing-raw-finished-zones-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_workspace_safety
- root cause：A, H, E, F
- 最小修复建议：safety incident profile 覆盖“工坊+火花/碎屑+差点伤人+复工记录”。

### production_shift_handover

- query：下一班要接手未完成的工坊任务，交接卡最少写什么？
- expected Guide：DG-0910
- expected Wiki：production-workshop-next-action-handover-001, production-workshop-shift-handover-card-001, production-workshop-tool-state-handover-001, production-workshop-wip-handover-tag-001
- actual top1 / top3：DG-0902 / DG-0902, DG-0910, DG-0904
- actual selected Wiki：general-storage-handover-card-001, general-storage-inventory-card-minimum-001, general-storage-check-calendar-001, general-storage-issue-log-001, general-storage-fifo-shelf-rule-001, general-storage-label-minimum-fields-001, agriculture-storage-seed-library-rotation-card-001, agriculture-storage-seed-bag-label-001
- Guide candidates 中 Production：DG-0910@2, DG-0904@3
- Wiki candidates 中 Production：无
- 已触发 profile：inventory_consumption, storage_inventory_handover
- root cause：D, F, J, L
- 最小修复建议：新增 shift handover profile；DG-0910 top1，DG-0902/Records secondary；重排 DG-0910 核心四项。

### observation_make_shelf

- query：我想做一个结实的架子，具体怎么切木板和连接？
- expected Guide：无（observation）
- expected Wiki：production-workshop-first-article-check-001, production-workshop-function-check-before-release-001
- actual top1 / top3：DG-0873 / DG-0873
- actual selected Wiki：repair-manufacturing-clamp-before-processing-001, repair-manufacturing-wood-saw-support-001, repair-manufacturing-wood-drill-position-001, repair-manufacturing-wood-measure-mark-001, repair-manufacturing-cut-drill-clear-zone-001, repair-manufacturing-wood-selection-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：manufacturing_cutting_clamping
- root cause：R
- 最小修复建议：合理 observation；保持 Manufacturing top1，不为 Production 补充 evidence 做 profile。

### observation_broken_saw

- query：锯子坏了还能不能继续用，怎么修？
- expected Guide：无（observation）
- expected Wiki：production-workshop-tool-count-before-work-001, production-workshop-tool-state-handover-001
- actual top1 / top3：DG-0837 / DG-0837, DG-0840, DG-0873
- actual selected Wiki：repair-dangerous-tool-temporary-isolation-001, repair-dangerous-tool-borrow-permission-001, repair-sharp-blunt-tool-zone-storage-001, repair-damaged-tool-disable-tag-001, repair-damaged-tool-report-isolation-001, repair-manufacturing-clamp-before-processing-001, repair-manufacturing-wood-saw-support-001, repair-manufacturing-wood-drill-position-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：repair_tool_damage_isolation, manufacturing_cutting_clamping
- root cause：R
- 最小修复建议：合理 observation；保持 Repair top1，不为 Production 补充 evidence 做 profile。

### observation_salvaged_wood

- query：这块旧木板从废料堆拿来，能不能拿去做东西？
- expected Guide：无（observation）
- expected Wiki：production-workshop-raw-material-intake-card-001
- actual top1 / top3：DG-0830 / DG-0830, DG-0315, DG-0058
- actual selected Wiki：agriculture-wood-ash-fertilizer-boundary-001, food-post-flood-food-high-risk-001
- Guide candidates 中 Production：无
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：C, P
- 最小修复建议：记录为 Waste/Manufacturing 旧入口语义漂移，移出 Batch13-F Production 修复范围。

### observation_long_term_finished_storage

- query：检查合格的成品要长期放在哪里，怎么保存？
- expected Guide：无（observation）
- expected Wiki：production-workshop-batch-label-minimum-001, production-workshop-finished-goods-storage-transfer-001
- actual top1 / top3：DG-0908 / DG-0908, DG-0905, DG-0907
- actual selected Wiki：production-workshop-nonconforming-item-quarantine-001, production-workshop-waste-transfer-card-001, production-workshop-edge-burr-check-001, production-workshop-rework-decision-line-001, production-workshop-scrap-decision-line-001, production-workshop-quality-record-minimum-001, production-workshop-failed-batch-isolation-001, production-workshop-material-count-before-work-001
- Guide candidates 中 Production：DG-0908@1, DG-0905@2, DG-0907@3, DG-0904@4
- Wiki candidates 中 Production：无
- 已触发 profile：无
- root cause：J（反向边界：Storage 应主导但未进入）
- 最小修复建议：保持 fail；Storage 必须主导。新 Production profiles 明确排除“检查合格+长期保存”，另记 Storage 边界回归。

### observation_workshop_injury

- query：工坊切割时有人被划伤流血了，应该先做什么？
- expected Guide：无（observation）
- expected Wiki：production-workshop-daily-stop-shutdown-001, production-workshop-incident-review-card-001
- actual top1 / top3：DG-0839 / DG-0839, DG-0873, DG-0871
- actual selected Wiki：repair-knife-saw-clear-zone-001, repair-bystander-position-boundary-001, repair-floor-clutter-trip-check-001, repair-manufacturing-clamp-before-processing-001, repair-manufacturing-wood-saw-support-001, repair-manufacturing-wood-drill-position-001, repair-manufacturing-wood-measure-mark-001, repair-manufacturing-cut-drill-clear-zone-001
- Guide candidates 中 Production：DG-0903@5, DG-0909@6, DG-0905@8
- Wiki candidates 中 Production：无
- 已触发 profile：repair_site_clearance_boundary
- root cause：N, K
- 最小修复建议：保持 fail；Medical 应主导。另记 Medical/Workshop injury cross-domain gap，不由 Production 修。

### Observation pass 补充

- `observation_low_energy_schedule`：Production/Energy 共主合理，判定为 R，不修。

## 3. DG-0903 至 DG-0910 Guide 稳定性审查

|Guide|strict expected cases|候选 / top3 / top1|旧域竞争|related Wiki evidence|顺序调整|profile 保护|边界判断|
|---|---:|---|---|---|---|---|---|
|DG-0903|3|2 / 2 / 2|DG-0871 + manufacturing workspace safety|top1 时 6 篇全部进入；daily start 时 Guide 本身缺席|否，现序已突出 start/task/tool/material/risk|必要：start_close|不替代具体制造安全|
|DG-0904|2|2 / 2 / 0|DG-0871、DG-0905|closeout 中被 DG-0871 的前 8 条挤出；自身仅 3 篇|否，3 篇顺序合理|必要：start_close|DG-0905 对 WIP 流转可合理共主|
|DG-0905|5|2 / 1 / 1|DG-0894/0892、DG-0888、异常泛域|top1 时前 8 进入，batch label 位于第 9|是：raw/WIP/hold/waste/batch label 前置|必要：material_wip_flow|材料可用性仍归 Waste/Manufacturing|
|DG-0906|3|3 / 1 / 1|DG-0876/0877、泛小零件/农业|top1 时 7 篇全部进入|可重排语义但不影响 top8；非关键|必要：batch_repetition|具体切割连接归 Manufacturing|
|DG-0907|5|2 / 1 / 1|DG-0875、异常食物/导航漂移|top1 时核心 quality Wiki 均在前 8；each-piece 第 9|非必要；核心已在前 8|必要：quality_release|长期保存归 Storage|
|DG-0908|5|1 / 1 / 1|Waste 与 Manufacturing；部分 query 完全漂移|top1 时 7 篇全部进入|否|独立 profile 暂缓；作 quality/flow secondary|Waste 负责最终分类与污染判断|
|DG-0909|3|0 / 0 / 0|DG-0873/0871、DG-0689|Guide 未选中，顺序调整没有作用|否；7 篇均可在 top1 时进入|强必要：safety_incident|已受伤/火灾/电气由相邻安全域主导|
|DG-0910|3|1 / 1 / 0|DG-0902、DG-0905、DG-0689|shift case 被 DG-0902 的前 8 条挤出；核心 handover 位于 6-8|是：shift/tool/next/WIP 前置|必要：shift_handover|储藏交接和长期归档不由 Production 主导|

结论：Guide 内容与安全字段本身稳定。DG-0909 的问题完全发生在候选/选择之前；DG-0904 和 DG-0910 的问题是 top1 竞争导致 evidence 截断；只有 DG-0905、DG-0910 的顺序调整具有直接价值。

## 4. Production Wiki evidence 审查

|Wiki|关联 Guide（顺位）|是否进入非 pass evidence|主要挤出者|顺序调整|需要 profile|
|---|---|---|---|---|---|
|`production-workshop-daily-start-check-001`|DG-0903#1|否|DG-0871|否；profile/top1 优先|start_close|
|`production-workshop-task-board-minimum-001`|DG-0910#1, DG-0903#2|否|DG-0871|否；profile/top1 优先|shift_handover / start_close|
|`production-workshop-tool-count-before-work-001`|DG-0903#3|否|DG-0871, DG-0837|否；profile/top1 优先|start_close|
|`production-workshop-material-count-before-work-001`|DG-0905#1, DG-0903#4|production_unfinished_item_tag, observation_long_term_finished_storage|DG-0871|否；profile/top1 优先|material_wip_flow / start_close|
|`production-workshop-risk-source-scan-001`|DG-0909#1, DG-0903#5|否|DG-0871, DG-0873|否；profile/top1 优先|safety_incident / start_close|
|`production-workshop-daily-stop-shutdown-001`|DG-0904#2|否|DG-0871, DG-0839|否；profile/top1 优先|start_close|
|`production-workshop-unfinished-item-tag-001`|DG-0910#2, DG-0904#1|否|DG-0871, DG-0905|否；profile/top1 优先|shift_handover / start_close|
|`production-workshop-end-of-day-status-log-001`|DG-0910#3, DG-0904#3|否|DG-0871|否；profile/top1 优先|shift_handover / start_close|
|`production-workshop-raw-material-intake-card-001`|DG-0905#2|production_unfinished_item_tag|DG-0894, DG-0830|小范围前置|material_wip_flow|
|`production-workshop-material-reservation-line-001`|DG-0905#3|production_unfinished_item_tag|无直接失败|否；profile/top1 优先|material_wip_flow|
|`production-workshop-wip-zone-layout-001`|DG-0905#4|production_unfinished_item_tag|无直接失败|小范围前置|material_wip_flow|
|`production-workshop-wip-handover-tag-001`|DG-0910#4, DG-0905#5|production_unfinished_item_tag|DG-0902|小范围前置|shift_handover / material_wip_flow|
|`production-workshop-finished-goods-hold-zone-001`|DG-0907#1, DG-0905#6|production_unfinished_item_tag|DG-0058|小范围前置|quality_release / material_wip_flow|
|`production-workshop-finished-goods-storage-transfer-001`|DG-0907#2, DG-0905#7|production_unfinished_item_tag|DG-0058, DG-0875, DG-0908|否；profile/top1 优先|quality_release / material_wip_flow|
|`production-workshop-waste-transfer-card-001`|DG-0908#2, DG-0905#8|production_unfinished_item_tag, observation_long_term_finished_storage|DG-0888|小范围前置|quality_release / nonconforming secondary / material_wip_flow|
|`production-workshop-batch-label-minimum-001`|DG-0906#1, DG-0905#9|否|DG-0894, DG-0908|小范围前置|batch_repetition / material_wip_flow|
|`production-workshop-first-article-check-001`|DG-0906#3|否|DG-0876, DG-0873|否；profile/top1 优先|batch_repetition|
|`production-workshop-batch-consistency-check-001`|DG-0906#6|否|DG-0309|否；profile/top1 优先|batch_repetition|
|`production-workshop-size-check-sheet-001`|DG-0907#3|否|DG-0309|否；profile/top1 优先|quality_release|
|`production-workshop-edge-burr-check-001`|DG-0907#5, DG-0908#3|observation_long_term_finished_storage|DG-0058|否；profile/top1 优先|quality_release / quality_release / nonconforming secondary|
|`production-workshop-loose-fastener-check-001`|DG-0907#6|否|DG-0875|否；profile/top1 优先|quality_release|
|`production-workshop-rework-decision-line-001`|DG-0908#4|observation_long_term_finished_storage|DG-0058, DG-0875|否；profile/top1 优先|quality_release / nonconforming secondary|
|`production-workshop-scrap-decision-line-001`|DG-0908#5|observation_long_term_finished_storage|无直接失败|否；profile/top1 优先|quality_release / nonconforming secondary|
|`production-workshop-low-light-stop-line-001`|DG-0909#4|否|DG-0873|否；profile/top1 优先|safety_incident|
|`production-workshop-fatigue-stop-line-001`|DG-0910#5, DG-0909#5|否|DG-0689|否；profile/top1 优先|shift_handover / safety_incident|
|`production-workshop-dust-spark-near-miss-log-001`|DG-0909#3|否|DG-0871|否；profile/top1 优先|safety_incident|
|`production-workshop-incident-review-card-001`|DG-0909#7|否|DG-0689, DG-0871, DG-0839|否；profile/top1 优先|safety_incident|
|`production-workshop-shift-handover-card-001`|DG-0910#6|否|DG-0902|小范围前置|shift_handover|
|`production-workshop-tool-state-handover-001`|DG-0910#7|否|DG-0902, DG-0837|小范围前置|shift_handover|
|`production-workshop-next-action-handover-001`|DG-0910#8|否|DG-0902|小范围前置|shift_handover|

说明：`是否进入非 pass evidence` 只表示它曾出现在某个非 pass case，不代表命中了该 case 的目标语义。大量目标 Wiki 缺失，是因为其 Production Guide 未入 top3；对这些条目单独调顺序没有作用。

## 5. 是否需要 query profile

|候选 profile|必要性|覆盖失败 case|可能误伤 observation|secondary 与边界|Batch13-F|
|---|---|---|---|---|---|
|`production_workshop_start_close`|必要|daily start、daily closeout|O1 制作架子、O5 已受伤|DG-0871/0878、Repair、Safety secondary；要求“工坊 + 开工检查/收工封存/任务状态”等复合触发|建议进入|
|`production_material_wip_flow`|必要|raw intake、waste transfer；巩固 WIP|O3 废料能否使用、O4 长期保存|DG-0894/0892/0888、Storage/Waste secondary；材料能否用不归 Production|建议进入|
|`production_batch_repetition`|必要|first article、batch consistency；巩固 failed batch|O1 具体切割连接|DG-0907/0908、Manufacturing secondary；要求“批次/首件/多个一样 + 检查”|建议进入|
|`production_quality_release`|必要|finished hold、edge burr、loose connection|O4 检查合格后长期保存|DG-0908/0905、Manufacturing/Storage/Safety secondary；只匹配未检查、待检、放行前、入库前缺陷|建议进入|
|`production_nonconforming_rework_scrap`|当前非必要|rework/scrap 与 failed batch 已 pass；可补 waste transfer/edge|O3 Waste 判断|作为 DG-0908 secondary 逻辑已够；先看其他 profile 复测|暂缓|
|`production_workshop_safety_incident`|强必要|low light、fatigue、near miss|O5 已受伤、真实火灾/漏电|Medical/Fire/Energy/Safety/Manufacturing secondary；“已受伤/流血/火已起/漏电”必须让相邻域主导|建议进入|
|`production_shift_handover`|必要|shift handover；辅助 unfinished tag|Storage 交接、Records 长期归档|DG-0904/0902、Records/Organization/Storage secondary；要求工坊/半成品/工具状态与下一班组合|建议进入|

建议 Batch13-F 新增 6 个 profile：1、2、3、4、6、7；暂缓 profile 5。每个 profile 都应使用对象 + 状态复合触发、target category/anchor 约束和相邻域 secondary，不用单题句子硬编码。

## 6. 是否需要 Guide-Wiki 顺序调整

|Guide|判断|Batch13-F 建议|
|---|---|---|
|DG-0903|6 篇全可进入；daily start 失败时 Guide 不在候选，顺序无意义。|不调。|
|DG-0904|仅 3 篇，顺序已是 unfinished / shutdown / status。|不调；先让 Guide top1。|
|DG-0905|9 篇会发生第 9 条截断，raw intake、WIP、hold、waste、batch label 是主要查询入口。|小调：将 raw intake、WIP zone/tag、finished hold、waste transfer、batch label 放前；材料清点/保留线仍保留。|
|DG-0906|7 篇均能进入 top8。|不为测试调序；可保持现状。|
|DG-0907|9 篇中核心 hold/size/stability/edge/loose/function 已在前 7。|不调。|
|DG-0908|7 篇全部可进入。|不调。|
|DG-0909|7 篇全部可进入，但三个失败 case 中 Guide 从未入候选。|不调；profile 是唯一有意义的先手。|
|DG-0910|9 篇且 shift/tool/next 位于 6-8；成为 top1 后虽能进入，但核心语义靠后。|小调：shift handover、tool state、next action、WIP tag 前置。|

DG-0904 / DG-0910 在 top3 但 evidence 缺失，根因首先是 top1 Guide 的 related Wiki 整批占位；不能用扩大 top8 修复。

## 7. 是否需要新增 Wiki / Guide

|主题|现有入口|判断|
|---|---|---|
|开工 / 收工|DG-0903/0904 + 8 篇 Wiki|完整，不新增。|
|原料 / WIP / 成品流转|DG-0905 + 9 篇 Wiki|完整，不新增。|
|小批量 / 首件 / 一致性|DG-0906 + 6 篇核心 Wiki|完整，不新增。|
|质量 / 锐边 / 松动 / 放行|DG-0907 + 对应检查 Wiki|完整，不新增。|
|不合格 / 返工 / 报废|DG-0908 + 隔离/返工/报废 Wiki|完整，不新增。|
|安全记录 / near miss / 低照明 / 疲劳|DG-0909 + 7 篇 Wiki|完整；问题是候选命中为 0，不新增。|
|工坊交接|DG-0910 + 交接/工具/下一步 Wiki|完整，不新增。|

Q 类根因计数为 0。新增知识只会制造重复，不会解决当前主位问题。

## 8. 是否需要修正 fixture

|case|当前判定|是否调整 fixture|理由|
|---|---|---|---|
|`production_unfinished_item_tag`|partial；DG-0905 top1，DG-0904 top3，WIP tag 已进入|是|DG-0905 本就负责原料/WIP/成品流转，应把 expected Guide 扩为 DG-0904 / DG-0905 / DG-0910。|
|`observation_long_term_finished_storage`|fail；Production top1|否|“检查合格 + 长期放哪里/怎么保存”明确属于 Storage。Production top1 必须继续判 fail。|
|`observation_workshop_injury`|fail；Repair/Safety top1，Medical 未主导|否|已经划伤流血应由 Medical 主导；这是 Production 修复范围外的 Medical cross-domain gap。|
|`observation_salvaged_wood`|fail；草木灰入口漂移|否|旧木板来自废料堆，应由 Waste/Manufacturing 判断；expected 不窄，属于既有语义漂移。|
|`observation_make_shelf`|partial；Manufacturing 主导但无 Production 补充|否|R 类合理 observation；保留 partial 作为补充 evidence 观察，不据此新增广泛 profile。|
|`observation_broken_saw`|partial；Repair 主导但无 Production 补充|否|R 类合理 observation；不让 Production 抢 Repair top1。|

fixture 只建议修改 1 条 strict expected；observation 的 partial/fail 保留，用于防止新 profile 吞域。

## 9. Batch13-F Minimal Apply 建议

1. **新增 6 个 Production query profiles**：start_close、material_wip_flow、batch_repetition、quality_release、workshop_safety_incident、shift_handover。暂缓独立 nonconforming profile。
2. **只调整 DG-0905 与 DG-0910 的 related_wiki 顺序**；不调整其他 6 个 Guide。
3. **只修正 1 条 fixture**：`production_unfinished_item_tag` 接受 DG-0905。
4. **不新增 Wiki / Guide**，不修改双向关系集合，只改变两个 Guide 内既有关联的顺序。
5. **复跑同一 26 条 Field Test 和两个 Retrieval contracts**；重点要求 strict 主位/any hit 提升，同时 O1/O2/O4/O5 不被 Production 吞掉。
6. **observation 不修项**：O1 Manufacturing、O2 Repair、O6 Energy/Production 共主；O3 另记 Waste/Manufacturing 语义漂移，O5 另记 Medical gap；O4 保持 Storage 边界回归。

## 10. 不建议修改项

- 不扩大 top_k，不提高 selector limit。
- 不修改 Retrieval Pipeline、Prompt、ranking、fallback 或 schema。
- 不修改 old stable domains 来迁就 Production；只在新 Production profiles 中设置明确 secondary 和排除边界。
- 不为单条 query 写精确句子或 case ID 硬编码。
- 不让 Production 吞掉具体 Manufacturing、长期 Storage、Waste 材料判断、Repair、Medical、Energy/Fire/Safety 或 Records。
- 不把 safety/fallback/record-check 当作根因；三项当前均为 100%。
- 不新增 Wiki/Guide，也不重建已有 stable domain 内容。

结论：建议进入 Batch13-F Minimal Apply，但范围严格限定为 **6 个复合 Production profiles + 2 个 Guide 的小范围 related_wiki 顺序调整 + 1 条 fixture expected 修正**。除此之外不做生产改动。
