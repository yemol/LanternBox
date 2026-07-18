# Batch10-C Agriculture Second Stage Retrieval Field Test Report

生成时间：2026-07-18T04:03:02.542176+00:00

## 1. 测试范围

本阶段只测试 Batch10-B 新增 Agriculture Second Stage Guide/Wiki/evidence chain 是否稳定进入本地 Retrieval selected evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。

覆盖：种子复测、连续育苗、苗期腐烂隔离、土壤恢复、堆肥/粪肥/厨余边界、病虫害和工具分流、污染地块停用、轮作、多季计划、收获后晾晒防霉、储藏容器、小规模粮食优先级。

## 2. strict / observation 数量

- 用例总数：24
- strict / observation：18 / 6
- pass / partial / fail：24 / 0 / 0
- strict pass / partial / fail：18 / 0 / 0

## 3. Guide / Wiki 命中

- Guide 命中率（strict，含 allowed secondary）：100.0%
- 主 Guide 命中率（strict，仅 expected）：94.4%
- Wiki 全量命中率（strict，全部 expected Wiki）：100.0%
- Wiki 任一命中率（strict，至少一个 expected Wiki）：100.0%
- Guide-Wiki 精准组合率（strict）：100.0%
- Agriculture Second Stage 主 Guide 进入率（全部 cases）：95.8%

## 4. 安全边界

- safety boundary：100.0%
- fallback：100.0%
- record/check：100.0%
- dangerous suggestion：0
- Kiwix 越权：0

## 5. Case 明细

|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|
|---|---|---|---|---|---|---|---|
|agri_seed_low_germination_large_sowing|strict|pass|DG-0879 种子保存与发芽率复测、DG-0053 种子发芽测试：先测一小撮、DG-0886 小规模粮食生产优先级|agriculture-seed-batch-viability-ledger-001 种子批次与发芽率复测台账、agriculture-seed-drying-recheck-001 种子干燥后的复查、agriculture-seed-storage-moisture-failure-001 种子受潮失效判断、agriculture-seed-reserve-use-line-001 留种和食用种子的保底线、agriculture-seed-library-box-index-001 小型种子库盒索引、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-seed-storage-001 种子保存的干燥和温度、agriculture-seed-germination-test-001 种子发芽率小样测试|planting_seed_germination、agriculture_seed_library|无|无|
|agri_seed_mold_back_to_library|strict|pass|DG-0879 种子保存与发芽率复测、DG-0886 小规模粮食生产优先级、DG-0232 种子库存管理|agriculture-seed-batch-viability-ledger-001 种子批次与发芽率复测台账、agriculture-seed-drying-recheck-001 种子干燥后的复查、agriculture-seed-storage-moisture-failure-001 种子受潮失效判断、agriculture-seed-reserve-use-line-001 留种和食用种子的保底线、agriculture-seed-library-box-index-001 小型种子库盒索引、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-seed-storage-001 种子保存的干燥和温度、agriculture-seed-germination-test-001 种子发芽率小样测试|agriculture_seed_library|无|无|
|agri_continuous_nursery_no_gap|strict|pass|DG-0880 连续育苗与失败复盘、DG-0728 安排轮休避免疲劳失控、DG-0236 简易育苗流程|agriculture-continuous-nursery-schedule-001 连续育苗排期、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-transplant-hardening-off-001 移栽前炼苗和缓苗风险、agriculture-nursery-capacity-limit-001 育苗数量和照护上限、agriculture-seedling-001 育苗失败的常见原因、agriculture-seedling-transplant-001 幼苗移栽和缓苗检查、agriculture-seedling-thinning-001 幼苗间苗与保留株选择、agriculture-planting-failure-review-001 种植失败现场复盘|无|无|无|
|agri_seedling_damping_off|strict|pass|DG-0880 连续育苗与失败复盘、DG-0883 病虫害早期隔离与工具分流、DG-0234 雨季种植防烂根|agriculture-continuous-nursery-schedule-001 连续育苗排期、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-transplant-hardening-off-001 移栽前炼苗和缓苗风险、agriculture-nursery-capacity-limit-001 育苗数量和照护上限、agriculture-seedling-001 育苗失败的常见原因、agriculture-seedling-transplant-001 幼苗移栽和缓苗检查、agriculture-seedling-thinning-001 幼苗间苗与保留株选择、agriculture-planting-failure-review-001 种植失败现场复盘|无|无|无|
|agri_compacted_soil_recovery|strict|pass|DG-0881 土壤贫瘠与板结恢复判断、DG-0727 短期可完成任务恢复秩序、DG-0275 备份恢复演练|agriculture-soil-poor-fertility-signs-001 土壤贫瘠的现场信号、agriculture-soil-compaction-recovery-001 土壤板结恢复、agriculture-salinity-crust-stop-line-001 土表盐壳和盐分风险、agriculture-raised-bed-drainage-recovery-001 积水地块的高畦恢复、agriculture-soil-recovery-log-001 土壤恢复记录、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-soil-texture-field-check-001 土壤质地手感判断、agriculture-soil-drainage-test-001 种植地排水下渗测试|无|无|无|
|agri_salt_crust_continue_fertilizer|strict|pass|DG-0881 土壤贫瘠与板结恢复判断、DG-0111 电线和插线板安全检查、DG-0112 电池漏液处理|agriculture-soil-poor-fertility-signs-001 土壤贫瘠的现场信号、agriculture-soil-compaction-recovery-001 土壤板结恢复、agriculture-salinity-crust-stop-line-001 土表盐壳和盐分风险、agriculture-raised-bed-drainage-recovery-001 积水地块的高畦恢复、agriculture-soil-recovery-log-001 土壤恢复记录、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-soil-texture-field-check-001 土壤质地手感判断、agriculture-soil-drainage-test-001 种植地排水下渗测试|无|无|无|
|agri_stinky_kitchen_compost_leafy_greens|strict|pass|DG-0882 堆肥成熟和未腐熟风险判断、DG-0883 病虫害早期隔离与工具分流、DG-0235 堆肥成熟判断|agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-kitchen-waste-compost-boundary-001 厨余堆肥进入种植区边界、agriculture-compost-001 堆肥成熟和病原风险、agriculture-compost-pile-setup-001 小型厨余堆肥建堆、agriculture-manure-growing-zone-separation-001 粪污与食用种植区隔离、agriculture-pest-early-scouting-routine-001 虫害早期巡查流程|agriculture_contaminated_plot_boundary|无|无|
|agri_manure_direct_edible_plot|strict|pass|DG-0882 堆肥成熟和未腐熟风险判断、DG-0883 病虫害早期隔离与工具分流、DG-0235 堆肥成熟判断|agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-kitchen-waste-compost-boundary-001 厨余堆肥进入种植区边界、agriculture-compost-001 堆肥成熟和病原风险、agriculture-compost-pile-setup-001 小型厨余堆肥建堆、agriculture-manure-growing-zone-separation-001 粪污与食用种植区隔离、agriculture-pest-early-scouting-routine-001 虫害早期巡查流程|agriculture_contaminated_plot_boundary|无|无|
|agri_leaf_back_many_eggs|strict|pass|DG-0883 病虫害早期隔离与工具分流、DG-0831 作物病害隔离与工具分流、DG-0002 伤者初筛：意识、呼吸、大出血|agriculture-pest-early-scouting-routine-001 虫害早期巡查流程、agriculture-egg-larvae-leaf-back-check-001 叶背虫卵和幼虫检查、agriculture-manual-pest-removal-record-001 人工除虫后的复查记录、agriculture-diseased-tool-zone-separation-001 带病害工具分区、agriculture-unknown-chemical-plot-stop-001 不明化学污染地块停用、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-plant-pests-001 病虫害早期观察|无|无|无|
|agri_diseased_pruner_cross_use|strict|pass|DG-0883 病虫害早期隔离与工具分流、DG-0831 作物病害隔离与工具分流、DG-0675 采收后留种标记|agriculture-pest-early-scouting-routine-001 虫害早期巡查流程、agriculture-egg-larvae-leaf-back-check-001 叶背虫卵和幼虫检查、agriculture-manual-pest-removal-record-001 人工除虫后的复查记录、agriculture-diseased-tool-zone-separation-001 带病害工具分区、agriculture-unknown-chemical-plot-stop-001 不明化学污染地块停用、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-plant-pests-001 病虫害早期观察|agriculture_pest_disease_control|无|无|
|agri_unknown_chemical_plot_food|strict|pass|DG-0883 病虫害早期隔离与工具分流、DG-0882 堆肥成熟和未腐熟风险判断、DG-0235 堆肥成熟判断|agriculture-pest-early-scouting-routine-001 虫害早期巡查流程、agriculture-egg-larvae-leaf-back-check-001 叶背虫卵和幼虫检查、agriculture-manual-pest-removal-record-001 人工除虫后的复查记录、agriculture-diseased-tool-zone-separation-001 带病害工具分区、agriculture-unknown-chemical-plot-stop-001 不明化学污染地块停用、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-plant-pests-001 病虫害早期观察|agriculture_contaminated_plot_boundary|无|无|
|agri_next_season_same_small_plot|strict|pass|DG-0884 轮作和多季种植计划、DG-0552 停水后第一小时：容器收集和分区、DG-0055 地震后第一小时：先稳住，不急着乱跑|agriculture-crop-family-rotation-card-001 作物家族轮作卡、agriculture-heavy-light-feeder-rotation-001 高耗肥和低耗肥作物轮作、agriculture-quick-leaf-long-cycle-balance-001 快收叶菜和长周期作物搭配、agriculture-seasonal-planting-calendar-001 多季种植日历、agriculture-production-failure-backup-plan-001 生产失败备用计划、agriculture-annual-garden-plan-card-001 家庭菜地年度计划卡、agriculture-yield-record-minimum-fields-001 产量记录最小字段、agriculture-team-garden-task-handover-001 菜地任务交接|无|无|无|
|agri_leafy_quick_vs_long_cycle_staple|strict|pass|DG-0886 小规模粮食生产优先级、DG-0608 霉变粮食和坚果停用、DG-0229 叶菜轮播计划|agriculture-staple-crop-small-plot-priority-001 小地块粮食作物优先级、agriculture-seed-reserve-use-line-001 留种和食用种子的保底线、agriculture-quick-leaf-long-cycle-balance-001 快收叶菜和长周期作物搭配、agriculture-production-failure-backup-plan-001 生产失败备用计划、agriculture-yield-record-minimum-fields-001 产量记录最小字段、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-local-crop-profile-001 本地作物行动档案、agriculture-planting-inventory-link-001 种植计划与库存消耗联动|无|无|无|
|agri_harvested_beans_dry_mold_prevention|strict|pass|DG-0885 收获后晾晒防霉与储藏、DG-0832 采收后分级防霉、DG-0306 简易晾晒绳|agriculture-harvest-drying-rack-check-001 收获晾晒架检查、agriculture-postharvest-drydown-record-001 收获后干燥记录、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-storage-container-dry-clean-check-001 储藏容器干燥清洁检查、agriculture-rodent-insect-storage-barrier-001 储粮鼠虫防护、agriculture-mold-batch-discard-line-001 霉变批次丢弃边界、agriculture-root-crop-curing-basic-001 根茎作物短期熟化和储藏、agriculture-harvest-maturity-check-001 小规模作物采收成熟判断|agriculture_postharvest_storage|无|无|
|agri_seed_food_batch_separation|strict|pass|DG-0885 收获后晾晒防霉与储藏、DG-0879 种子保存与发芽率复测、DG-0886 小规模粮食生产优先级|agriculture-harvest-drying-rack-check-001 收获晾晒架检查、agriculture-postharvest-drydown-record-001 收获后干燥记录、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-storage-container-dry-clean-check-001 储藏容器干燥清洁检查、agriculture-rodent-insect-storage-barrier-001 储粮鼠虫防护、agriculture-mold-batch-discard-line-001 霉变批次丢弃边界、agriculture-root-crop-curing-basic-001 根茎作物短期熟化和储藏、agriculture-harvest-maturity-check-001 小规模作物采收成熟判断|agriculture_seed_library、agriculture_postharvest_storage|无|无|
|agri_storage_container_moisture_grain|strict|pass|DG-0885 收获后晾晒防霉与储藏、DG-0832 采收后分级防霉、DG-0673 容器种植排水孔检查|agriculture-harvest-drying-rack-check-001 收获晾晒架检查、agriculture-postharvest-drydown-record-001 收获后干燥记录、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-storage-container-dry-clean-check-001 储藏容器干燥清洁检查、agriculture-rodent-insect-storage-barrier-001 储粮鼠虫防护、agriculture-mold-batch-discard-line-001 霉变批次丢弃边界、agriculture-root-crop-curing-basic-001 根茎作物短期熟化和储藏、agriculture-harvest-maturity-check-001 小规模作物采收成熟判断|agriculture_postharvest_storage|无|无|
|agri_small_plot_leafy_or_staple_first|strict|pass|DG-0886 小规模粮食生产优先级、DG-0608 霉变粮食和坚果停用、DG-0884 轮作和多季种植计划|agriculture-staple-crop-small-plot-priority-001 小地块粮食作物优先级、agriculture-seed-reserve-use-line-001 留种和食用种子的保底线、agriculture-quick-leaf-long-cycle-balance-001 快收叶菜和长周期作物搭配、agriculture-production-failure-backup-plan-001 生产失败备用计划、agriculture-yield-record-minimum-fields-001 产量记录最小字段、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-local-crop-profile-001 本地作物行动档案、agriculture-planting-inventory-link-001 种植计划与库存消耗联动|无|无|无|
|agri_repeated_production_failure_next_batch|strict|pass|DG-0884 轮作和多季种植计划、DG-0880 连续育苗与失败复盘、DG-0886 小规模粮食生产优先级|agriculture-crop-family-rotation-card-001 作物家族轮作卡、agriculture-heavy-light-feeder-rotation-001 高耗肥和低耗肥作物轮作、agriculture-quick-leaf-long-cycle-balance-001 快收叶菜和长周期作物搭配、agriculture-seasonal-planting-calendar-001 多季种植日历、agriculture-production-failure-backup-plan-001 生产失败备用计划、agriculture-annual-garden-plan-card-001 家庭菜地年度计划卡、agriculture-yield-record-minimum-fields-001 产量记录最小字段、agriculture-team-garden-task-handover-001 菜地任务交接|无|无|无|
|agri_observe_moldy_grain_eat|observation|pass|DG-0608 霉变粮食和坚果停用、DG-0628 霉味衣物和睡眠区分离、DG-0886 小规模粮食生产优先级|food-canned-food-001 干粮和罐头三天配给原则、food-canned-food-002 干货罐头轮换和先开先用、food-children-001 儿童老人食物优先级判断、food-cooking-001 低燃料烹饪对食物安全的影响、food-food-knowledge-001 即食食品和需煮食品的排序、food-food-knowledge-002 不可食残渣和可食剩余的分界、food-food-knowledge-003 断供期间食物心理安慰与纪律、food-food-poisoning-001 多人共餐时食物中毒追溯|无|off_domain_primary、agriculture_vs_food|无|
|agri_observe_drying_rack_make|observation|pass|DG-0885 收获后晾晒防霉与储藏、DG-0832 采收后分级防霉、DG-0306 简易晾晒绳|agriculture-harvest-drying-rack-check-001 收获晾晒架检查、agriculture-postharvest-drydown-record-001 收获后干燥记录、agriculture-seed-food-harvest-separation-001 留种批次与食用批次分离、agriculture-storage-container-dry-clean-check-001 储藏容器干燥清洁检查、agriculture-rodent-insect-storage-barrier-001 储粮鼠虫防护、agriculture-mold-batch-discard-line-001 霉变批次丢弃边界、agriculture-root-crop-curing-basic-001 根茎作物短期熟化和储藏、agriculture-harvest-maturity-check-001 小规模作物采收成熟判断|agriculture_postharvest_storage|无|无|
|agri_observe_waterlogged_soil_domain|observation|pass|DG-0881 土壤贫瘠与板结恢复判断、DG-0673 容器种植排水孔检查、DG-0234 雨季种植防烂根|agriculture-soil-poor-fertility-signs-001 土壤贫瘠的现场信号、agriculture-soil-compaction-recovery-001 土壤板结恢复、agriculture-salinity-crust-stop-line-001 土表盐壳和盐分风险、agriculture-raised-bed-drainage-recovery-001 积水地块的高畦恢复、agriculture-soil-recovery-log-001 土壤恢复记录、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-soil-texture-field-check-001 土壤质地手感判断、agriculture-soil-drainage-test-001 种植地排水下渗测试|无|无|无|
|agri_observe_tool_disease_clean_or_repair|observation|pass|DG-0883 病虫害早期隔离与工具分流、DG-0831 作物病害隔离与工具分流、DG-0675 采收后留种标记|agriculture-pest-early-scouting-routine-001 虫害早期巡查流程、agriculture-egg-larvae-leaf-back-check-001 叶背虫卵和幼虫检查、agriculture-manual-pest-removal-record-001 人工除虫后的复查记录、agriculture-diseased-tool-zone-separation-001 带病害工具分区、agriculture-unknown-chemical-plot-stop-001 不明化学污染地块停用、agriculture-seedling-damping-off-isolation-001 苗期猝倒和隔离、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-plant-pests-001 病虫害早期观察|agriculture_pest_disease_control|无|无|
|agri_observe_patient_leftovers_compost|observation|pass|DG-0882 堆肥成熟和未腐熟风险判断、DG-0607 剩饭过夜无冷藏禁食判断、DG-0054 堆肥：厨余不是随便堆|agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线、agriculture-manure-compost-food-zone-boundary-001 粪肥与食用地块边界、agriculture-kitchen-waste-compost-boundary-001 厨余堆肥进入种植区边界、agriculture-compost-001 堆肥成熟和病原风险、agriculture-compost-pile-setup-001 小型厨余堆肥建堆、agriculture-manure-growing-zone-separation-001 粪污与食用种植区隔离、food-canned-food-001 干粮和罐头三天配给原则|无|无|无|
|agri_observe_eat_seed_reserve|observation|pass|DG-0886 小规模粮食生产优先级、DG-0879 种子保存与发芽率复测、DG-0232 种子库存管理|agriculture-staple-crop-small-plot-priority-001 小地块粮食作物优先级、agriculture-seed-reserve-use-line-001 留种和食用种子的保底线、agriculture-quick-leaf-long-cycle-balance-001 快收叶菜和长周期作物搭配、agriculture-production-failure-backup-plan-001 生产失败备用计划、agriculture-yield-record-minimum-fields-001 产量记录最小字段、agriculture-water-fertility-budget-card-001 水肥预算卡、agriculture-local-crop-profile-001 本地作物行动档案、agriculture-planting-inventory-link-001 种植计划与库存消耗联动|agriculture_seed_library|无|无|

## 6. Cross Domain 统计

- food 抢主位观察：1
- hygiene 抢主位观察：0
- contamination 抢主位观察：0
- water 抢主位观察：0
- manufacturing 抢主位观察：0
- tools 抢主位观察：0
- repair 抢主位观察：0

Cross domain labels：
- off_domain_primary：1
- agriculture_vs_food：1

## 7. 重点分析

- food 抢主位：重点观察霉味粮食、收获后晾晒、储藏容器和吃掉留种场景。Food safety 主导已霉变食用判断是合理的，但农业 evidence 应补充批次隔离和种源保护。
- hygiene / WASH 抢主位：重点观察厨余、病人剩饭、粪肥进入堆肥或食用地块场景。人身污染可由 hygiene 主导，但农业应保留堆肥成熟和食用区禁入边界。
- contamination 抢主位：不明化学污染地块可以由 contamination 协同，但是否继续种食用作物应有 Agriculture evidence。
- water 抢主位：饮水和洪水问题由 water 主导；土壤积水、根区恢复和高畦处理应由 Agriculture 主导或进入 evidence。
- manufacturing 抢主位：晾晒架制作可由 manufacturing 主导，但收获后晾晒质量、防霉和批次记录应进入 Agriculture evidence。
- tools / repair 抢主位：剪过病株的工具不是维修问题，应优先体现病害传播、分区停用和记录。

## 8. 是否需要 profile

暂不建议新增 profile；可将 Agriculture Second Stage v0.1 作为 stable candidate 继续观察。

## 9. 是否进入 Root Cause Review

建议进入 Batch10-D Agriculture Second Stage Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。

## 10. 逐条复盘

### agri_seed_low_germination_large_sowing

- query：旧种子发芽率很低，还能不能大面积播种？
- 类型：strict
- focus：种子批次和发芽率复测应主导，避免低发芽率种子直接占用主地块。
- verdict：pass
- expected Guide：DG-0879
- allowed secondary：无
- selected Guide：DG-0879、DG-0053、DG-0886
- expected Wiki：agriculture-seed-batch-viability-ledger-001
- selected Wiki：agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001、agriculture-seed-reserve-use-line-001、agriculture-seed-library-box-index-001、agriculture-seed-food-harvest-separation-001、agriculture-seed-storage-001、agriculture-seed-germination-test-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_seed_mold_back_to_library

- query：种子受潮有霉味，能不能混回种子库？
- 类型：strict
- focus：种子受潮和霉味应触发隔离、停用主种源和记录。
- verdict：pass
- expected Guide：DG-0879
- allowed secondary：无
- selected Guide：DG-0879、DG-0886、DG-0232
- expected Wiki：agriculture-seed-storage-moisture-failure-001
- selected Wiki：agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001、agriculture-seed-reserve-use-line-001、agriculture-seed-library-box-index-001、agriculture-seed-food-harvest-separation-001、agriculture-seed-storage-001、agriculture-seed-germination-test-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_continuous_nursery_no_gap

- query：连续育苗怎么安排，避免菜地断档？
- 类型：strict
- focus：连续育苗排期和资源上限应进入 selected evidence。
- verdict：pass
- expected Guide：DG-0880
- allowed secondary：无
- selected Guide：DG-0880、DG-0728、DG-0236
- expected Wiki：agriculture-continuous-nursery-schedule-001
- selected Wiki：agriculture-continuous-nursery-schedule-001、agriculture-seedling-damping-off-isolation-001、agriculture-transplant-hardening-off-001、agriculture-nursery-capacity-limit-001、agriculture-seedling-001、agriculture-seedling-transplant-001、agriculture-seedling-thinning-001、agriculture-planting-failure-review-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_seedling_damping_off

- query：苗盘里有幼苗倒伏腐烂，怎么办？
- 类型：strict
- focus：苗期猝倒、腐烂和苗盘隔离应主导，不应泛化成普通卫生或污染。
- verdict：pass
- expected Guide：DG-0880
- allowed secondary：DG-0883
- selected Guide：DG-0880、DG-0883、DG-0234
- expected Wiki：agriculture-seedling-damping-off-isolation-001
- selected Wiki：agriculture-continuous-nursery-schedule-001、agriculture-seedling-damping-off-isolation-001、agriculture-transplant-hardening-off-001、agriculture-nursery-capacity-limit-001、agriculture-seedling-001、agriculture-seedling-transplant-001、agriculture-seedling-thinning-001、agriculture-planting-failure-review-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_compacted_soil_recovery

- query：土壤板结，菜长得很差，怎么恢复？
- 类型：strict
- focus：土壤贫瘠和板结恢复应主导，避免被水或泛种植旧 Guide 完全吸走。
- verdict：pass
- expected Guide：DG-0881
- allowed secondary：无
- selected Guide：DG-0881、DG-0727、DG-0275
- expected Wiki：agriculture-soil-compaction-recovery-001、agriculture-soil-poor-fertility-signs-001
- selected Wiki：agriculture-soil-poor-fertility-signs-001、agriculture-soil-compaction-recovery-001、agriculture-salinity-crust-stop-line-001、agriculture-raised-bed-drainage-recovery-001、agriculture-soil-recovery-log-001、agriculture-water-fertility-budget-card-001、agriculture-soil-texture-field-check-001、agriculture-soil-drainage-test-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_salt_crust_continue_fertilizer

- query：土表有白色盐壳，还能继续施肥吗？
- 类型：strict
- focus：土表盐壳应触发停止继续施肥和水源/盐分记录。
- verdict：pass
- expected Guide：DG-0881
- allowed secondary：无
- selected Guide：DG-0881、DG-0111、DG-0112
- expected Wiki：agriculture-salinity-crust-stop-line-001
- selected Wiki：agriculture-soil-poor-fertility-signs-001、agriculture-soil-compaction-recovery-001、agriculture-salinity-crust-stop-line-001、agriculture-raised-bed-drainage-recovery-001、agriculture-soil-recovery-log-001、agriculture-water-fertility-budget-card-001、agriculture-soil-texture-field-check-001、agriculture-soil-drainage-test-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_stinky_kitchen_compost_leafy_greens

- query：厨余堆肥还发臭，能不能倒在叶菜旁边？
- 类型：strict
- focus：发臭厨余和未腐熟肥应触发食用区禁入边界。
- verdict：pass
- expected Guide：DG-0882
- allowed secondary：无
- selected Guide：DG-0882、DG-0883、DG-0235
- expected Wiki：agriculture-immature-compost-stop-line-001、agriculture-kitchen-waste-compost-boundary-001
- selected Wiki：agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-kitchen-waste-compost-boundary-001、agriculture-compost-001、agriculture-compost-pile-setup-001、agriculture-manure-growing-zone-separation-001、agriculture-pest-early-scouting-routine-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_manure_direct_edible_plot

- query：粪肥能不能直接用在食用菜地？
- 类型：strict
- focus：粪肥与食用区边界应主导，WASH/污染控制可作为 secondary。
- verdict：pass
- expected Guide：DG-0882
- allowed secondary：无
- selected Guide：DG-0882、DG-0883、DG-0235
- expected Wiki：agriculture-manure-compost-food-zone-boundary-001
- selected Wiki：agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-kitchen-waste-compost-boundary-001、agriculture-compost-001、agriculture-compost-pile-setup-001、agriculture-manure-growing-zone-separation-001、agriculture-pest-early-scouting-routine-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_leaf_back_many_eggs

- query：叶背发现很多虫卵，应该怎么处理？
- 类型：strict
- focus：虫卵、幼虫和叶背巡查应触发农业病虫害早期隔离。
- verdict：pass
- expected Guide：DG-0883
- allowed secondary：无
- selected Guide：DG-0883、DG-0831、DG-0002
- expected Wiki：agriculture-egg-larvae-leaf-back-check-001、agriculture-pest-early-scouting-routine-001
- selected Wiki：agriculture-pest-early-scouting-routine-001、agriculture-egg-larvae-leaf-back-check-001、agriculture-manual-pest-removal-record-001、agriculture-diseased-tool-zone-separation-001、agriculture-unknown-chemical-plot-stop-001、agriculture-seedling-damping-off-isolation-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-plant-pests-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_diseased_pruner_cross_use

- query：剪过病株的剪刀还能继续剪别的菜吗？
- 类型：strict
- focus：病害工具分流应主导，tools/repair 不应只按工具清洁或维修回答。
- verdict：pass
- expected Guide：DG-0883
- allowed secondary：无
- selected Guide：DG-0883、DG-0831、DG-0675
- expected Wiki：agriculture-diseased-tool-zone-separation-001
- selected Wiki：agriculture-pest-early-scouting-routine-001、agriculture-egg-larvae-leaf-back-check-001、agriculture-manual-pest-removal-record-001、agriculture-diseased-tool-zone-separation-001、agriculture-unknown-chemical-plot-stop-001、agriculture-seedling-damping-off-isolation-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-plant-pests-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_unknown_chemical_plot_food

- query：这块地疑似有化学污染，还能种吃的吗？
- 类型：strict
- focus：不明化学污染地块应触发食用作物停用和边界标记。
- verdict：pass
- expected Guide：DG-0883
- allowed secondary：无
- selected Guide：DG-0883、DG-0882、DG-0235
- expected Wiki：agriculture-unknown-chemical-plot-stop-001
- selected Wiki：agriculture-pest-early-scouting-routine-001、agriculture-egg-larvae-leaf-back-check-001、agriculture-manual-pest-removal-record-001、agriculture-diseased-tool-zone-separation-001、agriculture-unknown-chemical-plot-stop-001、agriculture-seedling-damping-off-isolation-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-plant-pests-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_next_season_same_small_plot

- query：同一小地块下一季应该种什么？
- 类型：strict
- focus：轮作和多季计划应主导，而不是普通食物或泛种植建议。
- verdict：pass
- expected Guide：DG-0884
- allowed secondary：无
- selected Guide：DG-0884、DG-0552、DG-0055
- expected Wiki：agriculture-crop-family-rotation-card-001、agriculture-heavy-light-feeder-rotation-001
- selected Wiki：agriculture-crop-family-rotation-card-001、agriculture-heavy-light-feeder-rotation-001、agriculture-quick-leaf-long-cycle-balance-001、agriculture-seasonal-planting-calendar-001、agriculture-production-failure-backup-plan-001、agriculture-annual-garden-plan-card-001、agriculture-yield-record-minimum-fields-001、agriculture-team-garden-task-handover-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_leafy_quick_vs_long_cycle_staple

- query：快收叶菜和长周期粮食怎么搭配？
- 类型：strict
- focus：快收叶菜和长周期粮食搭配可由轮作或小规模粮食优先级主导。
- verdict：pass
- expected Guide：DG-0884
- allowed secondary：DG-0886
- selected Guide：DG-0886、DG-0608、DG-0229
- expected Wiki：agriculture-quick-leaf-long-cycle-balance-001
- selected Wiki：agriculture-staple-crop-small-plot-priority-001、agriculture-seed-reserve-use-line-001、agriculture-quick-leaf-long-cycle-balance-001、agriculture-production-failure-backup-plan-001、agriculture-yield-record-minimum-fields-001、agriculture-water-fertility-budget-card-001、agriculture-local-crop-profile-001、agriculture-planting-inventory-link-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_harvested_beans_dry_mold_prevention

- query：收获后的豆子怎么晾晒，避免发霉？
- 类型：strict
- focus：采收后干燥记录和晾晒架质量应主导，food safety 可补充但不应完全抢主位。
- verdict：pass
- expected Guide：DG-0885
- allowed secondary：无
- selected Guide：DG-0885、DG-0832、DG-0306
- expected Wiki：agriculture-harvest-drying-rack-check-001、agriculture-postharvest-drydown-record-001
- selected Wiki：agriculture-harvest-drying-rack-check-001、agriculture-postharvest-drydown-record-001、agriculture-seed-food-harvest-separation-001、agriculture-storage-container-dry-clean-check-001、agriculture-rodent-insect-storage-barrier-001、agriculture-mold-batch-discard-line-001、agriculture-root-crop-curing-basic-001、agriculture-harvest-maturity-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_seed_food_batch_separation

- query：留种和食用批次怎么分开？
- 类型：strict
- focus：留种、食用、待观察批次分离应进入 selected evidence。
- verdict：pass
- expected Guide：DG-0885
- allowed secondary：DG-0879
- selected Guide：DG-0885、DG-0879、DG-0886
- expected Wiki：agriculture-seed-food-harvest-separation-001
- selected Wiki：agriculture-harvest-drying-rack-check-001、agriculture-postharvest-drydown-record-001、agriculture-seed-food-harvest-separation-001、agriculture-storage-container-dry-clean-check-001、agriculture-rodent-insect-storage-barrier-001、agriculture-mold-batch-discard-line-001、agriculture-root-crop-curing-basic-001、agriculture-harvest-maturity-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_storage_container_moisture_grain

- query：储藏容器有潮气，还能不能装粮？
- 类型：strict
- focus：储藏容器干燥清洁检查应主导，manufacturing 只能补充容器制作/修复。
- verdict：pass
- expected Guide：DG-0885
- allowed secondary：无
- selected Guide：DG-0885、DG-0832、DG-0673
- expected Wiki：agriculture-storage-container-dry-clean-check-001
- selected Wiki：agriculture-harvest-drying-rack-check-001、agriculture-postharvest-drydown-record-001、agriculture-seed-food-harvest-separation-001、agriculture-storage-container-dry-clean-check-001、agriculture-rodent-insect-storage-barrier-001、agriculture-mold-batch-discard-line-001、agriculture-root-crop-curing-basic-001、agriculture-harvest-maturity-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_small_plot_leafy_or_staple_first

- query：小地块先种叶菜还是粮食作物？
- 类型：strict
- focus：小规模粮食生产优先级应主导，平衡短期补给和长期试种。
- verdict：pass
- expected Guide：DG-0886
- allowed secondary：DG-0884
- selected Guide：DG-0886、DG-0608、DG-0884
- expected Wiki：agriculture-staple-crop-small-plot-priority-001
- selected Wiki：agriculture-staple-crop-small-plot-priority-001、agriculture-seed-reserve-use-line-001、agriculture-quick-leaf-long-cycle-balance-001、agriculture-production-failure-backup-plan-001、agriculture-yield-record-minimum-fields-001、agriculture-water-fertility-budget-card-001、agriculture-local-crop-profile-001、agriculture-planting-inventory-link-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_repeated_production_failure_next_batch

- query：连续几次生产失败，下一批怎么调整？
- 类型：strict
- focus：生产失败备用计划和多季计划应进入 evidence。
- verdict：pass
- expected Guide：DG-0884
- allowed secondary：DG-0886
- selected Guide：DG-0884、DG-0880、DG-0886
- expected Wiki：agriculture-production-failure-backup-plan-001
- selected Wiki：agriculture-crop-family-rotation-card-001、agriculture-heavy-light-feeder-rotation-001、agriculture-quick-leaf-long-cycle-balance-001、agriculture-seasonal-planting-calendar-001、agriculture-production-failure-backup-plan-001、agriculture-annual-garden-plan-card-001、agriculture-yield-record-minimum-fields-001、agriculture-team-garden-task-handover-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_observe_moldy_grain_eat

- query：粮食有霉味还能不能吃？
- 类型：observation
- focus：观察 food safety 是否主导，Agriculture 是否补充批次隔离和储藏记录。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0608、DG-0628、DG-0886
- expected Wiki：无
- selected Wiki：food-canned-food-001、food-canned-food-002、food-children-001、food-cooking-001、food-food-knowledge-001、food-food-knowledge-002、food-food-knowledge-003、food-food-poisoning-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、agriculture_vs_food
- root cause：无
- failure reasons：无

### agri_observe_drying_rack_make

- query：晾晒架怎么做？
- 类型：observation
- focus：观察 manufacturing 是否主导，Agriculture 是否补充晾晒质量要求。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0885、DG-0832、DG-0306
- expected Wiki：无
- selected Wiki：agriculture-harvest-drying-rack-check-001、agriculture-postharvest-drydown-record-001、agriculture-seed-food-harvest-separation-001、agriculture-storage-container-dry-clean-check-001、agriculture-rodent-insect-storage-barrier-001、agriculture-mold-batch-discard-line-001、agriculture-root-crop-curing-basic-001、agriculture-harvest-maturity-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_observe_waterlogged_soil_domain

- query：土壤积水严重，是水的问题还是种植问题？
- 类型：observation
- focus：观察 water / agriculture 边界：饮水和洪水归 water，根区恢复归 agriculture。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0881、DG-0673、DG-0234
- expected Wiki：无
- selected Wiki：agriculture-soil-poor-fertility-signs-001、agriculture-soil-compaction-recovery-001、agriculture-salinity-crust-stop-line-001、agriculture-raised-bed-drainage-recovery-001、agriculture-soil-recovery-log-001、agriculture-water-fertility-budget-card-001、agriculture-soil-texture-field-check-001、agriculture-soil-drainage-test-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_observe_tool_disease_clean_or_repair

- query：工具上可能带病害，需要清洗还是修理？
- 类型：observation
- focus：观察 tools / repair / agriculture 边界：病害传播应有 agriculture evidence。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0883、DG-0831、DG-0675
- expected Wiki：无
- selected Wiki：agriculture-pest-early-scouting-routine-001、agriculture-egg-larvae-leaf-back-check-001、agriculture-manual-pest-removal-record-001、agriculture-diseased-tool-zone-separation-001、agriculture-unknown-chemical-plot-stop-001、agriculture-seedling-damping-off-isolation-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-plant-pests-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_observe_patient_leftovers_compost

- query：厨余和病人剩饭能不能进堆肥？
- 类型：observation
- focus：观察 hygiene / contamination / agriculture 边界：病人剩饭应触发卫生污染边界，农业补充堆肥禁入。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0882、DG-0607、DG-0054
- expected Wiki：无
- selected Wiki：agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001、agriculture-manure-compost-food-zone-boundary-001、agriculture-kitchen-waste-compost-boundary-001、agriculture-compost-001、agriculture-compost-pile-setup-001、agriculture-manure-growing-zone-separation-001、food-canned-food-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### agri_observe_eat_seed_reserve

- query：今年收成少，要不要吃掉留种？
- 类型：observation
- focus：观察 food rationing / agriculture seed reserve 边界：口粮分配可补充，但核心留种应进入 evidence。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0886、DG-0879、DG-0232
- expected Wiki：无
- selected Wiki：agriculture-staple-crop-small-plot-priority-001、agriculture-seed-reserve-use-line-001、agriculture-quick-leaf-long-cycle-balance-001、agriculture-production-failure-backup-plan-001、agriculture-yield-record-minimum-fields-001、agriculture-water-fertility-budget-card-001、agriculture-local-crop-profile-001、agriculture-planting-inventory-link-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

## 11. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_agriculture_second_stage_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_agriculture_second_stage_field.py --no-answer
```

边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。
