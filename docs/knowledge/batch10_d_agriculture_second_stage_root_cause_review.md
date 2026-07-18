# LanternBox Batch10-D: Agriculture Second Stage Retrieval Root Cause Review

本阶段只做根因分析，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase 或测试。

参考：

- `docs/knowledge/batch10_a_agriculture_second_stage_plan.md`
- `docs/knowledge/batch10_b_agriculture_second_stage_apply_report.md`
- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_report.md`
- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_results.json`
- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

## 1. Field Test 总结

|指标|结果|
|---|---:|
|total|24|
|strict / observation|18 / 6|
|pass / partial / fail|20 / 0 / 4|
|strict pass / partial / fail|14 / 0 / 4|
|Guide hit|94.4%|
|expected Guide hit|88.9%|
|Wiki hit|94.4%|
|Guide-Wiki precise|94.4%|
|Agriculture Second Stage top1 rate|70.8%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|

总体判断：

1. fail 主要不是知识缺失。新增 Guide / Wiki / evidence chain 大多已进入 selected evidence。
2. fail 主要不是 Guide-Wiki 缺链。4 个 fail 中 3 个已经命中 expected Wiki，且 precise chain 成立。
3. 主要问题是 selected Guide 排序和领域边界：旧 Planting v0.1 Guide、Food / Hygiene / Medical / Water / Contamination Guide 在部分 query 中抢 top1。
4. `agri_unknown_chemical_plot_food` 是唯一真正出现 expected Guide/Wiki 完全未进入 selected 的 strict fail，更像 agriculture profile 缺失 + contamination/medical/water 强势召回。
5. 当前没有 dangerous suggestion 或 Kiwix 越权，安全边界、fallback、record/check 均为 100%。

结论：Batch10-E 不应新增大量知识，也不应改 Prompt/top_k/selector limit。应做最小 profile + 少量 Guide-Wiki evidence priority 调整。

## 2. 4 个 Fail 根因

### 2.1 旧种子发芽率低是否还要大面积播种

|项|内容|
|---|---|
|query|旧种子发芽率很低，还能不能大面积播种？|
|expected Guide|DG-0879|
|expected Wiki|`agriculture-seed-batch-viability-ledger-001`|
|actual selected Guide|DG-0053, DG-0879, DG-0232|
|actual selected Wiki|`agriculture-seed-germination-test-001`, `general-seed-batch-record-001`, `agriculture-seed-batch-viability-ledger-001`, `agriculture-seed-drying-recheck-001`, `agriculture-seed-storage-moisture-failure-001`, `agriculture-seed-reserve-use-line-001`, `agriculture-seed-food-harvest-separation-001`, `agriculture-seed-library-box-index-001`|
|Agriculture Second Stage Guide 是否进入|是，DG-0879 第 2 位|
|Agriculture Second Stage Wiki 是否进入|是，expected Wiki 第 3 位|
|cross domain|legacy_planting_primary, agriculture_vs_food, agriculture_vs_storage|
|root cause|E. 旧 Planting Guide 抢主位；A. agriculture profile 缺失|

分析：DG-0053 “种子发芽测试：先测一小撮”与 query 中“发芽率很低”强匹配，因此旧 Planting v0.1 主导。DG-0879 和 expected Wiki 已进入 selected，说明不是知识缺失，也不是缺链。差异在于 query 同时包含“发芽率”和“大面积播种”，后者属于 Agriculture Second Stage 的批次与规模决策。

最小修复建议：新增 `agriculture_seed_library` 或 `agriculture_seed_batch_viability` profile，覆盖“旧种子、发芽率低、大面积播种、种子库、批次、复测、主种源、保底留种”等触发。DG-0879 优先，DG-0053 可作为 secondary。

### 2.2 剪过病株的剪刀还能继续剪别的菜吗

|项|内容|
|---|---|
|query|剪过病株的剪刀还能继续剪别的菜吗？|
|expected Guide|DG-0883|
|expected Wiki|`agriculture-diseased-tool-zone-separation-001`|
|actual selected Guide|DG-0831, DG-0883, DG-0111|
|actual selected Wiki|`agriculture-crop-disease-isolation-001`, `agriculture-diseased-residue-separation-001`, `agriculture-pest-early-scouting-routine-001`, `agriculture-egg-larvae-leaf-back-check-001`, `agriculture-manual-pest-removal-record-001`, `agriculture-seedling-damping-off-isolation-001`, `agriculture-diseased-tool-zone-separation-001`, `agriculture-unknown-chemical-plot-stop-001`|
|Agriculture Second Stage Guide 是否进入|是，DG-0883 第 2 位|
|Agriculture Second Stage Wiki 是否进入|是，expected Wiki 第 7 位|
|cross domain|legacy_planting_primary, agriculture_vs_hygiene|
|root cause|E. 旧 Planting Guide 抢主位；D. DG-0883 related_wiki 顺序可优化|

分析：DG-0831 是旧 Planting v0.1 的“作物病害隔离与工具分流”，覆盖当前单次病害隔离场景，因此抢 top1。DG-0883 与 expected Wiki 均已进入，说明正确 evidence 存在。问题是 query 指向“剪过病株的剪刀是否继续跨区使用”，应由 Agriculture Second Stage 的病害工具分区和周期管理主导。

最小修复建议：新增 `agriculture_pest_disease_control` profile，触发“病株、剪刀、工具、继续剪、跨区、健康苗、带病害工具”。同时将 DG-0883 的 `agriculture-diseased-tool-zone-separation-001` 前移到前 2 位，避免叶背虫卵类 Wiki 在工具 query 中先占位。

### 2.3 留种和食用批次怎么分开

|项|内容|
|---|---|
|query|留种和食用批次怎么分开？|
|expected Guide|DG-0885|
|allowed secondary|DG-0879|
|expected Wiki|`agriculture-seed-food-harvest-separation-001`|
|actual selected Guide|DG-0675, DG-0885, DG-0879|
|actual selected Wiki|`agriculture-container-002`, `agriculture-seed-selection-for-saving-001`, `agriculture-seed-storage-001`, `agriculture-sprouts-001`, `agriculture-harvest-drying-rack-check-001`, `agriculture-postharvest-drydown-record-001`, `agriculture-seed-food-harvest-separation-001`, `agriculture-storage-container-dry-clean-check-001`|
|Agriculture Second Stage Guide 是否进入|是，DG-0885 第 2 位，DG-0879 第 3 位|
|Agriculture Second Stage Wiki 是否进入|是，expected Wiki 第 7 位|
|cross domain|legacy_planting_primary|
|root cause|E. 旧 Planting Guide 抢主位；D. DG-0885 / DG-0879 related_wiki 顺序可优化；G. fixture expected 可略窄|

分析：DG-0675 “采收后留种标记”对“留种”高度匹配，因此抢 top1。DG-0885 和 DG-0879 都进入 selected，expected Wiki 也进入。query 的语义是“采收后批次分离”，既可由 DG-0885 主导，也可由 DG-0879 主导。现 fixture 只把 DG-0879 作为 allowed secondary，合理但偏窄；旧 DG-0675 也不是错误知识，只是 v0.1 入口压过了 v1.5 的批次管理入口。

最小修复建议：新增或合并到 `agriculture_postharvest_storage` / `agriculture_seed_library` profile，覆盖“留种、食用批次、分开、采收后、批次、待观察”。将 `agriculture-seed-food-harvest-separation-001` 在 DG-0885 保持前 3，DG-0879 可前移到前 3。是否把 DG-0675 作为 allowed secondary 可在 Batch10-E 评估，但不应仅靠 fixture 放宽掩盖 v1.5 top1 不稳定。

### 2.4 疑似化学污染地块还能不能种吃的

|项|内容|
|---|---|
|query|这块地疑似有化学污染，还能种吃的吗？|
|expected Guide|DG-0883|
|expected Wiki|`agriculture-unknown-chemical-plot-stop-001`|
|actual selected Guide|DG-0064, DG-0553, DG-0665|
|actual selected Wiki|`medical-chemical-skin-eye-exposure-001`, `hygiene-contamination-zone-001`, `hygiene-contaminated-clothing-001`, `hygiene-hygiene-knowledge-001`, `hygiene-contamination-log-001`, `hygiene-isolation-supplies-001`, `water-boiling-004`, `water-chemical-risk-011`|
|Agriculture Second Stage Guide 是否进入|否|
|Agriculture Second Stage Wiki 是否进入|否|
|cross domain|medical / hygiene / water / contamination|
|root cause|A. agriculture profile 缺失；F. Contamination / Water / Medical 抢主位；可能存在 C. correct Wiki 未召回|

分析：query 中“化学污染”触发了医疗化学暴露、可疑水源、化学味容器等成熟高风险入口。它们对人体和水容器风险是正确的，但 query 的对象是“地块”和“能不能种吃的”，应让 agriculture evidence 至少进入 selected。当前 DG-0883 和 `agriculture-unknown-chemical-plot-stop-001` 都未进入，因此这是 4 个 fail 中唯一的实质召回缺口。

最小修复建议：新增 `agriculture_contaminated_plot_food_zone` 或并入 `agriculture_pest_disease_control` 的 plot contamination 分支。触发对象包括“地、地块、菜地、土壤、食用作物、种吃的”；状态包括“化学污染、刺鼻味、油膜、不明液体、污染地块”；动作包括“还能不能种、能不能吃、停用、标记”。Contamination / medical / water 应保留 secondary，不应被 Agriculture 吞掉人体接触和饮水风险。

## 3. 新增 8 个 Guide 稳定性审查

|Guide|Field selected|top1|稳定性|问题|建议|
|---|---:|---:|---|---|---|
|DG-0879 种子保存与发芽率复测|4|1|Yellow|低发芽率 query 被 DG-0053 抢 top1；留种/食用 query 被 DG-0675 抢 top1|需要 seed library profile；`seed-food-harvest-separation` 可前移|
|DG-0880 连续育苗与失败复盘|3|2|Green|连续育苗、猝倒隔离均稳定；production failure 作为 secondary 合理|暂不调整|
|DG-0881 土壤贫瘠与板结恢复判断|3|3|Green|板结、盐壳、积水 observation 均稳定|暂不调整|
|DG-0882 堆肥成熟和未腐熟风险判断|3|3|Green|厨余、粪肥、病人剩饭 observation 均稳定|可保持；profile 可加强 compost food-zone 边界但非必需|
|DG-0883 病虫害早期隔离与工具分流|3|1|Yellow|病株剪刀被 DG-0831 抢 top1；化学污染地块未召回|需要 pest/disease profile；工具分区 Wiki 前移；化学污染地块可能需要独立 profile|
|DG-0884 轮作和多季种植计划|3|2|Green|轮作、多季、生产失败进入稳定|暂不调整|
|DG-0885 收获后晾晒防霉与储藏|4|3|Yellow|留种/食用批次被 DG-0675 抢 top1；其他收获后处理稳定|postharvest profile 可补；批次分离 Wiki 保持前排|
|DG-0886 小规模粮食生产优先级|4|2|Green|叶菜 vs 粮食、小地块优先级稳定；霉味粮食 observation 中 Food 主导合理|暂不调整|

总体：8 个 Guide 结构和 evidence chain 成立。需要修的是入口优先级，不是重写 Guide。

## 4. 新增 40 篇 Wiki Evidence 审查

|Wiki|关联 Guide|是否进入 evidence|未进入原因|建议|
|---|---|---|---|---|
|`agriculture-seed-batch-viability-ledger-001`|DG-0879|是|无|保持，profile 触发低发芽率/批次|
|`agriculture-seed-drying-recheck-001`|DG-0879|是|无|保持|
|`agriculture-seed-storage-moisture-failure-001`|DG-0879|是|无|保持|
|`agriculture-seed-reserve-use-line-001`|DG-0879, DG-0886|是|无|保持|
|`agriculture-continuous-nursery-schedule-001`|DG-0880|是|无|保持|
|`agriculture-seedling-damping-off-isolation-001`|DG-0880, DG-0883|是|无|保持|
|`agriculture-transplant-hardening-off-001`|DG-0880|是|未被 strict expected，但随 DG-0880 进入|保持|
|`agriculture-nursery-capacity-limit-001`|DG-0880|是|未被 strict expected，但随 DG-0880 进入|保持|
|`agriculture-soil-poor-fertility-signs-001`|DG-0881|是|无|保持|
|`agriculture-compost-maturity-second-check-001`|DG-0882|是|未被 strict expected，但随 DG-0882 进入|保持|
|`agriculture-immature-compost-stop-line-001`|DG-0882|是|无|保持|
|`agriculture-soil-compaction-recovery-001`|DG-0881|是|无|保持|
|`agriculture-salinity-crust-stop-line-001`|DG-0881|是|无|保持|
|`agriculture-raised-bed-drainage-recovery-001`|DG-0881|是|未被 strict expected，但积水 observation 进入|保持|
|`agriculture-soil-recovery-log-001`|DG-0881|是|未被 strict expected，但随 DG-0881 进入|保持|
|`agriculture-pest-early-scouting-routine-001`|DG-0883|是|无|保持|
|`agriculture-egg-larvae-leaf-back-check-001`|DG-0883|是|无|保持|
|`agriculture-manual-pest-removal-record-001`|DG-0883|是|未被 strict expected，但随 DG-0883 进入|保持|
|`agriculture-diseased-tool-zone-separation-001`|DG-0883|是|进入较靠后；工具 query 中应更靠前|在 DG-0883 前移|
|`agriculture-unknown-chemical-plot-stop-001`|DG-0883|部分|随虫害 query 进入；化学污染地块 query 未进入|需要 profile；可在 DG-0883 前移到工具分区之后|
|`agriculture-manure-compost-food-zone-boundary-001`|DG-0882, DG-0883|是|无|保持|
|`agriculture-kitchen-waste-compost-boundary-001`|DG-0882|是|无|保持|
|`agriculture-crop-family-rotation-card-001`|DG-0884|是|无|保持|
|`agriculture-heavy-light-feeder-rotation-001`|DG-0884|是|无|保持|
|`agriculture-quick-leaf-long-cycle-balance-001`|DG-0884, DG-0886|是|无|保持|
|`agriculture-seasonal-planting-calendar-001`|DG-0884|是|未被 strict expected，但随 DG-0884 进入|保持|
|`agriculture-staple-crop-small-plot-priority-001`|DG-0886|是|无|保持|
|`agriculture-production-failure-backup-plan-001`|DG-0884, DG-0886|是|无|保持|
|`agriculture-harvest-drying-rack-check-001`|DG-0885|是|无|保持|
|`agriculture-postharvest-drydown-record-001`|DG-0885|是|无|保持|
|`agriculture-seed-food-harvest-separation-001`|DG-0879, DG-0885|是|进入但受旧 DG-0675 top1 影响|在 DG-0879 / DG-0885 保持或前移|
|`agriculture-storage-container-dry-clean-check-001`|DG-0885|是|无|保持|
|`agriculture-rodent-insect-storage-barrier-001`|DG-0885|是|未被 strict expected，但随 DG-0885 进入|保持|
|`agriculture-mold-batch-discard-line-001`|DG-0885|是|未被 strict expected，但随 DG-0885 进入|保持|
|`agriculture-root-crop-curing-basic-001`|DG-0885|是|未被 strict expected，但随 DG-0885 进入|保持|
|`agriculture-annual-garden-plan-card-001`|DG-0884|是|未被 strict expected，但随 DG-0884 进入|保持|
|`agriculture-yield-record-minimum-fields-001`|DG-0884, DG-0886|是|未被 strict expected，但随 planning/粮食优先级进入|保持|
|`agriculture-seed-library-box-index-001`|DG-0879|是|未被 strict expected，但种子 case 进入|保持|
|`agriculture-water-fertility-budget-card-001`|DG-0881, DG-0886|是|未被 strict expected，但土壤/粮食优先级进入|保持|
|`agriculture-team-garden-task-handover-001`|DG-0884|是|未被 strict expected，但随 DG-0884 进入|保持|

结论：40 篇 Wiki 中绝大多数已进入 evidence。唯一明显问题是 `agriculture-unknown-chemical-plot-stop-001` 未能在自己的化学污染地块 query 中进入 selected。

## 5. Cross Domain 根因

### Planting v0.1

抢主位来源：

- DG-0053 种子发芽测试：先测一小撮
- DG-0831 作物病害隔离与工具分流
- DG-0675 采收后留种标记

边界判断：

- Planting v0.1 应主导：单次播种、基础育苗、当前地块基础处理、雨后当前菜地复查。
- Agriculture Second Stage 应主导：种子库、批次管理、大面积播种决策、连续育苗、多季计划、病虫害周期管理、长期储藏、生产失败复盘。

根因：旧 Guide 对关键词“发芽测试 / 病害工具 / 留种标记”命中强，但无法表达 v1.5 的长期生产循环意图。应通过 profile 和少量 evidence priority 让 v1.5 query 进入新 Guide，不应压低或删除旧 Guide。

### Food

Food 合理主导：

- 已霉变粮食能否入口。
- 食物中毒风险。
- 立即食用顺序。

Agriculture 应主导：

- 收获后干燥。
- 留种与食用批次分离。
- 储藏前容器检查。
- 种源保底。

当前 Food 不是 strict fail 的主要 root cause，但 observation 中多次作为 top1 或 secondary 出现。应保留协同边界。

### Hygiene / Contamination / Medical

Hygiene / Contamination 应主导：

- 人体接触污染。
- 污染区清理。
- 排泄物卫生处理。
- 化学物接触皮肤/眼睛。

Agriculture 应主导：

- 食用地块是否停用。
- 堆肥成熟。
- 病害工具跨区传播。
- 种植区材料边界。

当前最大缺口是“化学污染地块能否种吃的”：现有检索把“化学污染”直接带向 medical/hygiene/water，但没有识别“地块 / 种吃的 / 食用作物”作为 agriculture 目标。

### Water

Water 应主导：

- 饮水、水源、净水。
- 洪水水质风险。

Agriculture 应主导：

- 作物根区积水。
- 排水恢复。
- 高畦。
- 停止浇水。

Field Test 中土壤积水 observation 已由 DG-0881 主导，暂不需要优先修 Water 冲突。

## 6. 是否需要 Profile

|profile|是否必要|覆盖 triggers|可能误伤|Batch10-E 是否实现|
|---|---|---|---|---|
|`agriculture_seed_library`|必要|旧种子、发芽率低、种子库、批次、复测、主种源、大面积播种、留种保底|可能压过 DG-0053 的单次发芽小样|是|
|`agriculture_nursery_cycle`|暂不必要|连续育苗、苗盘、移栽、缓苗、断档|可能压过基础育苗 DG-0236|暂缓，当前通过|
|`agriculture_soil_recovery`|暂不必要|板结、盐壳、贫瘠、根区积水、高畦、排水恢复|可能压过 water 的洪水/饮水问题|暂缓，当前通过|
|`agriculture_compost_food_zone`|可选|厨余堆肥、粪肥、未腐熟、食用地块、叶菜旁、病人剩饭|可能压过 hygiene 的人身污染和病人用品处理|可暂缓或轻量实现|
|`agriculture_pest_disease_control`|必要|病株、剪刀、工具、继续剪、叶背虫卵、幼虫、病害跨区、健康苗|可能压过 tools/repair 的真实工具损坏问题|是|
|`agriculture_crop_rotation_plan`|暂不必要|下一季、轮作、地块、多季、生产失败|可能压过 organization 记录/计划|暂缓，当前通过|
|`agriculture_postharvest_storage`|必要|收获后、晾晒、防霉、留种/食用批次、储藏容器、入库、返潮|可能压过 Food 的“能不能吃”判断|是，注意 Food safety secondary|

建议额外合并或拆出一个轻量 profile：

- `agriculture_contaminated_plot_food_zone`：针对“化学污染 + 地块/土壤/菜地 + 种吃的/食用作物/停用”。这是唯一 expected Guide/Wiki 完全未进入的 fail，建议 Batch10-E 实现。

## 7. 是否需要 Guide-Wiki 顺序调整

需要少量调整，不需要大批量重排。

|Guide|当前问题|建议|
|---|---|---|
|DG-0879|`agriculture-seed-food-harvest-separation-001` 在第 5，低发芽率 case 已足够；留种/食用可更靠前|将 `agriculture-seed-food-harvest-separation-001` 前移到第 4 左右；核心仍保持批次台账、受潮失效、保底线靠前|
|DG-0880|稳定|不调整|
|DG-0881|稳定|不调整|
|DG-0882|稳定|不调整或将 `kitchen-waste` 与 `manure` 视 query 分支；当前无需|
|DG-0883|工具分流和化学污染地块 Wiki 排位偏后|将 `agriculture-diseased-tool-zone-separation-001` 提到第 1 或第 2；将 `agriculture-unknown-chemical-plot-stop-001` 提到前 4|
|DG-0884|稳定|不调整|
|DG-0885|稳定但留种/食用 case 被旧 Guide 抢主位|`agriculture-seed-food-harvest-separation-001` 保持前 3；可放到第 1 或第 2 之前视 profile 结果|
|DG-0886|稳定|不调整|

注意：Guide-Wiki 顺序只能解决已选中 Guide 后 related_wiki 的展示优先，不能单独解决 DG-0883 在化学污染地块 query 中完全未选中的问题。

## 8. 是否需要新增 Wiki / Guide

默认不新增。逐项判断：

|主题|现有支撑|是否新增|
|---|---|---|
|种子低发芽率|DG-0879 + `agriculture-seed-batch-viability-ledger-001` 已进入|不新增|
|病害工具分流|DG-0883 + `agriculture-diseased-tool-zone-separation-001` 已进入|不新增|
|留种 / 食用批次|DG-0885 / DG-0879 + `agriculture-seed-food-harvest-separation-001` 已进入|不新增|
|化学污染地块停用|DG-0883 / `agriculture-unknown-chemical-plot-stop-001` 存在，但未召回|不新增，先 profile|

结论：当前没有证据表明需要新增 Wiki 或 Guide。化学污染地块是召回/领域入口问题，不是内容缺失。

## 9. Batch10-E 最小 Apply 建议

推荐：C. profile + Guide-Wiki 顺序都需要。

最小范围：

1. 新增少量 Agriculture query profile：
   - `agriculture_seed_library`
   - `agriculture_pest_disease_control`
   - `agriculture_postharvest_storage`
   - `agriculture_contaminated_plot_food_zone`
2. 可暂缓：
   - `agriculture_nursery_cycle`
   - `agriculture_soil_recovery`
   - `agriculture_crop_rotation_plan`
   - `agriculture_compost_food_zone`，除非 Batch10-E 希望顺手降低 hygiene/compost observation 风险。
3. 少量 Guide-Wiki 顺序调整：
   - DG-0883：前置 `agriculture-diseased-tool-zone-separation-001` 与 `agriculture-unknown-chemical-plot-stop-001`。
   - DG-0879：确保种子批次、受潮失效、保底线、留种/食用分离排在前列。
   - DG-0885：确保 `agriculture-seed-food-harvest-separation-001`、晾晒记录、储藏容器检查在前列。
4. 新增 contract tests：
   - 旧种子发芽率低能不能大面积播种 -> DG-0879。
   - 剪过病株的剪刀能不能继续剪 -> DG-0883。
   - 化学污染地块能不能种吃的 -> DG-0883。
   - 留种和食用批次怎么分开 -> DG-0885 / DG-0879。
5. 复跑 Agriculture Field Test。

不建议：

- 不新增大量 Wiki。
- 不新增 Guide。
- 不修改 Prompt。
- 不扩大 top_k。
- 不修改 selector limit。
- 不压低或删除旧 Planting Guide。
- 不让 Agriculture 吞掉 Food / Hygiene / Water / Contamination。
- 不硬编码测试答案。

## 10. 不建议修改项

- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不修改 top_k。
- 不修改 selector limit。
- 不修改 ranking 核心逻辑。
- 不修改 fallback。
- 不新增 Wiki / Guide。
- 不同步 PocketBase。
- 不把 Food safety、Hygiene、Water、Medical 的高风险入口并入 Agriculture。
- 不把旧 Planting v0.1 Guide 删除或压低到不可用；它们仍适合单次播种、基础病害隔离和采收留种标记。

## 结论

Agriculture Second Stage 知识链本身成立：Guide hit、Wiki hit、Guide-Wiki precise 均为 94.4%，安全指标全部 100%，无 dangerous suggestion / Kiwix 越权。当前 4 个 fail 的根因集中在 v1.5 agriculture intent 未被 profile 稳定识别，以及旧 Planting v0.1 / contamination-medical-water 入口抢 top1。Batch10-E 应采用最小 Apply：新增 4 个 agriculture profile，少量调整 DG-0879 / DG-0883 / DG-0885 的 related_wiki 顺序，并加入 contract tests 后复跑 Field Test。
