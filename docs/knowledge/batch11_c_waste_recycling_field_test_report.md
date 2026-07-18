# Batch11-C Waste / Recycling Retrieval Field Test Report

生成时间：2026-07-18T10:50:16.491895+00:00

## 1. 测试范围

本阶段只测试 Batch11-B 新增 Waste / Recycling Guide/Wiki 是否稳定进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。

覆盖：废弃物分类、临时隔离、锋利/破碎物、电池漏液物、病人垃圾、不明污染物、厨余分流、灰烬炭渣、材料池准入、容器再利用、材料池台账和交接记录。

重点观察：Fire、Agriculture、WASH/Hygiene、Manufacturing、Energy/Contamination、Medical/Safety 是否抢走 Waste / Recycling 主行动链。

## 2. Case 结果

- 用例总数：24
- strict / observation：18 / 6
- pass / partial / fail：24 / 0 / 0
- strict pass / partial / fail：18 / 0 / 0

## 3. Guide / Wiki 命中

- Guide 命中率（strict，含 allowed secondary）：100.0%
- 主 Guide 命中率（strict，仅 expected）：100.0%
- Wiki 全量命中率（strict，全部 expected Wiki）：100.0%
- Wiki 任一命中率（strict，至少一个 expected Wiki）：100.0%
- Guide-Wiki 精准组合率（strict）：100.0%
- Waste / Recycling 主 Guide 进入率（全部 cases）：91.7%

安全指标：

- safety boundary：100.0%
- fallback：100.0%
- record/check：100.0%
- dangerous suggestion：0
- Kiwix 越权：0

## 4. Case 明细

|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|
|---|---|---|---|---|---|---|---|
|waste_mixed_trash_sort_isolate|strict|pass|DG-0887 废弃物基础分类与临时隔离、DG-0889 病人垃圾与污染物分流、DG-0550 湿垃圾和食物残渣：密封、干湿分离与防虫处理|hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-temporary-overflow-plan-001 废弃物临时满溢处理|waste_basic_sorting|无|无|
|waste_broken_glass_collect_mark|strict|pass|DG-0888 锋利 / 破碎 / 金属边角废物处理、DG-0086 垃圾分区：普通、污染、尖锐、DG-0887 废弃物基础分类与临时隔离|hygiene-waste-sharp-glass-temporary-container-001 碎玻璃和锋利物临时容器、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、repair-recycling-metal-sheet-intake-check-001 金属片入池判断、hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界|waste_sharp_broken_material|无|无|
|waste_metal_edge_nail_puncture|strict|pass|DG-0888 锋利 / 破碎 / 金属边角废物处理、DG-0878 工坊收工与工具封存、DG-0838 儿童远离工具区|hygiene-waste-sharp-glass-temporary-container-001 碎玻璃和锋利物临时容器、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、repair-recycling-metal-sheet-intake-check-001 金属片入池判断、repair-manufacturing-end-clean-count-001 收工清点与危险工具封存、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|waste_sharp_broken_material|无|无|
|waste_battery_leak_common_trash|strict|pass|DG-0887 废弃物基础分类与临时隔离、DG-0112 电池漏液处理、DG-0841 电池异常隔离|hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-temporary-overflow-plan-001 废弃物临时满溢处理|energy_battery_abnormal_isolation、medical_poisoning_chemical_exposure、waste_contaminated_trash|无|无|
|waste_patient_tissue_common_trash|strict|pass|DG-0889 病人垃圾与污染物分流、DG-0887 废弃物基础分类与临时隔离、DG-0854 病人用品与厨房污染隔离|hygiene-waste-patient-trash-double-bag-zone-001 病人垃圾双层封存区、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-contaminated-container-downgrade-001 污染容器降级和禁用、hygiene-waste-patient-leftover-no-compost-001 病人剩饭不直接进堆肥、hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线|waste_contaminated_trash|无|无|
|waste_unknown_pungent_trash_hold|strict|pass|DG-0887 废弃物基础分类与临时隔离、DG-0894 废弃物与材料池记录交接、DG-0847 临时住所选址与防雨防潮|hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-temporary-overflow-plan-001 废弃物临时满溢处理|waste_basic_sorting|无|无|
|waste_rotten_kitchen_scrap_leachate_compost|strict|pass|DG-0890 厨余和有机物进入堆肥前判断、DG-0882 堆肥成熟和未腐熟风险判断、DG-0883 病虫害早期隔离与工具分流|hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-patient-leftover-no-compost-001 病人剩饭不直接进堆肥、agriculture-waste-kitchen-scrap-before-compost-001 厨余进堆肥前分拣、hygiene-waste-oil-meat-odor-organic-boundary-001 油脂肉类和异味厨余边界、agriculture-waste-compost-waiting-bin-distance-001 堆肥等待桶距离边界、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线|agriculture_contaminated_plot_boundary、waste_kitchen_scrap_boundary|无|无|
|waste_patient_leftover_compost_bin|strict|pass|DG-0890 厨余和有机物进入堆肥前判断、DG-0882 堆肥成熟和未腐熟风险判断、DG-0889 病人垃圾与污染物分流|hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-patient-leftover-no-compost-001 病人剩饭不直接进堆肥、agriculture-waste-kitchen-scrap-before-compost-001 厨余进堆肥前分拣、hygiene-waste-oil-meat-odor-organic-boundary-001 油脂肉类和异味厨余边界、agriculture-waste-compost-waiting-bin-distance-001 堆肥等待桶距离边界、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线|waste_kitchen_scrap_boundary|无|无|
|waste_hot_ash_direct_trash_bin|strict|pass|DG-0891 灰烬与炭渣冷却后分流、DG-0851 灰烬与余火处理、DG-0550 湿垃圾和食物残渣：密封、干湿分离与防虫处理|fire-waste-hot-ash-not-trash-001 热灰不得进入垃圾袋、fire-waste-cold-ash-storage-boundary-001 冷灰保存和丢弃边界、hygiene-waste-ash-trash-mixing-ban-001 灰烬混入普通垃圾风险、fire-waste-charcoal-residue-reuse-check-001 炭渣再利用前检查、agriculture-waste-ash-soil-use-interface-001 灰烬进入土壤前转交判断、fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理、fire-night-final-extinguish-log-001 夜间火源熄灭记录、fire-small-fire-stop-001 初起小火处置停止线|waste_ash_char_boundary|无|无|
|waste_cold_ash_store_later_use|strict|pass|DG-0891 灰烬与炭渣冷却后分流、DG-0851 灰烬与余火处理、DG-0879 种子保存与发芽率复测|fire-waste-hot-ash-not-trash-001 热灰不得进入垃圾袋、fire-waste-cold-ash-storage-boundary-001 冷灰保存和丢弃边界、hygiene-waste-ash-trash-mixing-ban-001 灰烬混入普通垃圾风险、fire-waste-charcoal-residue-reuse-check-001 炭渣再利用前检查、agriculture-waste-ash-soil-use-interface-001 灰烬进入土壤前转交判断、fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理、fire-night-final-extinguish-log-001 夜间火源熄灭记录、fire-small-fire-stop-001 初起小火处置停止线|waste_ash_char_boundary|无|无|
|waste_salvaged_wood_material_pool|strict|pass|DG-0892 可再利用材料进入材料池前检查、DG-0894 废弃物与材料池记录交接、DG-0893 塑料桶 / 容器再利用前判断|repair-recycling-salvaged-wood-intake-check-001 废木板入池判断、repair-recycling-metal-sheet-intake-check-001 金属片入池判断、repair-recycling-material-intake-checklist-001 材料入池前检查清单、repair-recycling-material-downgrade-label-001 材料降级用途标签、repair-recycling-material-pool-zone-layout-001 可再利用材料池分区、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离、repair-recycling-fabric-rope-intake-check-001 布料和绳索入池判断、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类|waste_material_pool_reuse|无|无|
|waste_salvaged_metal_sheet_pool|strict|pass|DG-0892 可再利用材料进入材料池前检查、DG-0876 废旧金属材料安全处理、DG-0872 材料再利用前安全判断|repair-recycling-salvaged-wood-intake-check-001 废木板入池判断、repair-recycling-metal-sheet-intake-check-001 金属片入池判断、repair-recycling-material-intake-checklist-001 材料入池前检查清单、repair-recycling-material-downgrade-label-001 材料降级用途标签、repair-recycling-material-pool-zone-layout-001 可再利用材料池分区、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离、repair-recycling-fabric-rope-intake-check-001 布料和绳索入池判断、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类|manufacturing_material_reuse|无|无|
|waste_plastic_bucket_odor_reuse|strict|pass|DG-0893 塑料桶 / 容器再利用前判断、DG-0874 简易结构件制作检查、DG-0892 可再利用材料进入材料池前检查|repair-recycling-plastic-container-intake-check-001 塑料容器入池判断、hygiene-waste-contaminated-container-downgrade-001 污染容器降级和禁用、repair-recycling-cleanable-noncleanable-material-001 可清洁材料和不可再用材料分界、repair-recycling-material-downgrade-label-001 材料降级用途标签、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-frame-square-check-001 简易框架方正检查|manufacturing_material_reuse、waste_material_pool_reuse|无|无|
|waste_container_unknown_residue_reuse|strict|pass|DG-0893 塑料桶 / 容器再利用前判断、DG-0894 废弃物与材料池记录交接、DG-0892 可再利用材料进入材料池前检查|repair-recycling-plastic-container-intake-check-001 塑料容器入池判断、hygiene-waste-contaminated-container-downgrade-001 污染容器降级和禁用、repair-recycling-cleanable-noncleanable-material-001 可清洁材料和不可再用材料分界、repair-recycling-material-downgrade-label-001 材料降级用途标签、general-recycling-material-pool-ledger-001 材料池台账最小字段、general-waste-reuse-failure-record-001 回收再利用失败记录、general-waste-disposal-handover-card-001 废弃物处理交接卡、general-waste-source-hazard-log-001 废弃物来源和风险记录|waste_material_pool_reuse|无|无|
|waste_material_pool_ledger|strict|pass|DG-0894 废弃物与材料池记录交接、DG-0892 可再利用材料进入材料池前检查、DG-0893 塑料桶 / 容器再利用前判断|general-recycling-material-pool-ledger-001 材料池台账最小字段、general-waste-reuse-failure-record-001 回收再利用失败记录、general-waste-disposal-handover-card-001 废弃物处理交接卡、general-waste-source-hazard-log-001 废弃物来源和风险记录、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类、repair-recycling-material-downgrade-label-001 材料降级用途标签|waste_material_pool_reuse|无|无|
|waste_reuse_failure_record|strict|pass|DG-0894 废弃物与材料池记录交接、DG-0299 可回收材料分类、DG-0667 维修失败三次后停用|general-recycling-material-pool-ledger-001 材料池台账最小字段、general-waste-reuse-failure-record-001 回收再利用失败记录、general-waste-disposal-handover-card-001 废弃物处理交接卡、general-waste-source-hazard-log-001 废弃物来源和风险记录、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类、repair-recycling-material-downgrade-label-001 材料降级用途标签|无|无|无|
|waste_contaminated_bin_placement|strict|pass|DG-0887 废弃物基础分类与临时隔离、DG-0889 病人垃圾与污染物分流、DG-0086 垃圾分区：普通、污染、尖锐|hygiene-waste-basic-sorting-isolation-001 废弃物基础分类与临时隔离、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-mixed-trash-stop-line-001 混合垃圾禁止线、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-child-access-control-001 儿童远离废弃物和材料池、hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-temporary-overflow-plan-001 废弃物临时满溢处理|waste_basic_sorting|无|无|
|waste_handover_minimum_fields|strict|pass|DG-0894 废弃物与材料池记录交接、DG-0893 塑料桶 / 容器再利用前判断、DG-0892 可再利用材料进入材料池前检查|general-recycling-material-pool-ledger-001 材料池台账最小字段、general-waste-reuse-failure-record-001 回收再利用失败记录、general-waste-disposal-handover-card-001 废弃物处理交接卡、general-waste-source-hazard-log-001 废弃物来源和风险记录、general-waste-source-label-minimum-001 废弃物来源最小标签、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类、repair-recycling-material-downgrade-label-001 材料降级用途标签|inventory_consumption、waste_material_pool_reuse|无|无|
|waste_observe_kitchen_scrap_to_compost|observation|pass|DG-0890 厨余和有机物进入堆肥前判断、DG-0882 堆肥成熟和未腐熟风险判断、DG-0054 堆肥：厨余不是随便堆|hygiene-waste-food-rot-wet-isolation-001 腐败食物和湿垃圾隔离、hygiene-waste-patient-leftover-no-compost-001 病人剩饭不直接进堆肥、agriculture-waste-kitchen-scrap-before-compost-001 厨余进堆肥前分拣、hygiene-waste-oil-meat-odor-organic-boundary-001 油脂肉类和异味厨余边界、agriculture-waste-compost-waiting-bin-distance-001 堆肥等待桶距离边界、hygiene-waste-organic-pest-odor-daily-check-001 有机废弃物虫害异味日查、agriculture-compost-maturity-second-check-001 堆肥成熟二次确认、agriculture-immature-compost-stop-line-001 未腐熟肥进入食用区停止线|无|无|无|
|waste_observe_hot_ash_rekindle|observation|pass|DG-0891 灰烬与炭渣冷却后分流、DG-0851 灰烬与余火处理、DG-0646 室内取暖睡前火源复查|fire-waste-hot-ash-not-trash-001 热灰不得进入垃圾袋、fire-waste-cold-ash-storage-boundary-001 冷灰保存和丢弃边界、hygiene-waste-ash-trash-mixing-ban-001 灰烬混入普通垃圾风险、fire-waste-charcoal-residue-reuse-check-001 炭渣再利用前检查、agriculture-waste-ash-soil-use-interface-001 灰烬进入土壤前转交判断、fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理、fire-night-final-extinguish-log-001 夜间火源熄灭记录、fire-small-fire-stop-001 初起小火处置停止线|waste_ash_char_boundary|无|无|
|waste_observe_wood_to_shelf|observation|pass|DG-0892 可再利用材料进入材料池前检查、DG-0304 木板和木棍再利用、DG-0830 草木灰来源审查和禁用|repair-recycling-salvaged-wood-intake-check-001 废木板入池判断、repair-recycling-metal-sheet-intake-check-001 金属片入池判断、repair-recycling-material-intake-checklist-001 材料入池前检查清单、repair-recycling-material-downgrade-label-001 材料降级用途标签、repair-recycling-material-pool-zone-layout-001 可再利用材料池分区、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离、repair-recycling-fabric-rope-intake-check-001 布料和绳索入池判断、repair-recycling-fasteners-small-parts-sort-001 旧螺丝钉和小零件分类|无|无|无|
|waste_observe_patient_trash_contamination|observation|pass|DG-0889 病人垃圾与污染物分流、DG-0477 病人垃圾处理、DG-0854 病人用品与厨房污染隔离|hygiene-waste-patient-trash-double-bag-zone-001 病人垃圾双层封存区、hygiene-waste-unknown-chemical-item-hold-001 不明化学污染物暂存、hygiene-waste-battery-leakage-boundary-001 电池漏液废弃物边界、hygiene-waste-contaminated-container-downgrade-001 污染容器降级和禁用、hygiene-waste-patient-leftover-no-compost-001 病人剩饭不直接进堆肥、hygiene-patient-cup-towel-isolation-001 病人杯子毛巾餐具隔离、hygiene-kitchen-raw-cooked-contamination-line-001 厨房生熟和污染线、hygiene-wash-abnormal-record-001 卫生异常记录和追溯|无|无|无|
|waste_observe_battery_leak_skin|observation|pass|DG-0112 电池漏液处理、DG-0841 电池异常隔离、DG-0064 疑似化学污染：远离、脱外层、冲洗|medical-chemical-skin-eye-exposure-001 化学物接触皮肤和眼睛的冲洗边界、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁、hygiene-contamination-log-001 污染记录帮助追溯来源、hygiene-isolation-supplies-001 隔离用品不足时的替代、energy-battery-parallel-series-boundary-001 电池串并联禁用边界、energy-battery-chemistry-mix-stop-001 不同电池类型混用停用边界、energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离|energy_battery_abnormal_isolation、medical_poisoning_chemical_exposure|off_domain_primary、waste_recycling_vs_medical、waste_recycling_vs_energy|无|
|waste_observe_glass_cut_hand|observation|pass|DG-0632 门口玻璃碎裂隔离、DG-0888 锋利 / 破碎 / 金属边角废物处理、DG-0625 拆解旧物前割伤防护|fire-small-fire-stop-001 初起小火处置停止线、safety-barter-002 物资交换前的对象风险判断、safety-buddy-system-001 双人行动和返回时间的意义、safety-children-003 儿童靠近危险点的隔离原则、safety-conflict-002 物资分配冲突的安全风险、safety-conflict-003 冲突降温中的暂停口令、safety-door-window-001 门窗破损后的可见暴露风险、safety-electric-shock-001 漏电风险中的水和金属接触|无|off_domain_primary、waste_recycling_vs_safety|无|

## 5. Cross Domain 分析

- fire 抢主位观察：0
- agriculture 抢主位观察：0
- hygiene 抢主位观察：1
- contamination 抢主位观察：1
- manufacturing 抢主位观察：0
- energy 抢主位观察：1
- medical 抢主位观察：2
- safety 抢主位观察：1
- food 抢主位观察：0
- records 抢主位观察：0
- repair 抢主位观察：0
- shelter 抢主位观察：0

Cross domain labels：
- off_domain_primary：2
- waste_recycling_vs_medical：1
- waste_recycling_vs_energy：1
- waste_recycling_vs_safety：1

### 重点抢主位分析

- Fire 抢热灰 / 炭渣主位：热灰和防复燃语义应由 Fire 停止线主导，但 Waste / Recycling 需要进入冷却后分流、垃圾桶禁止和记录证据。
- Agriculture 抢厨余 / 堆肥主位：堆肥成熟和入土应由 Agriculture 主导，厨余进入堆肥前的病人剩饭、腐败渗液、异味分流应保留 Waste evidence。
- WASH / Hygiene 抢病人垃圾 / 污染物主位：人体卫生和清洁分区可主导，但病人垃圾、污染容器和临时桶分流应作为 Waste 主链或补充链进入。
- Manufacturing 抢废木板 / 容器 / 材料池主位：制作、切割、承重归 Manufacturing；材料池准入、降级标签和来源记录归 Waste / Recycling。
- Energy / Contamination 抢电池漏液主位：人体接触和电池安全可主导，但漏液物作为废弃物不得混入普通垃圾或材料池。
- Medical / Safety 抢碎玻璃 / 锋利物主位：割伤后 Medical 主导；未受伤时碎玻璃、金属边角和钉子应由 Waste / Recycling 的锋利物分流主导。

## 6. Waste / Recycling Evidence 稳定性

- Waste / Recycling Guide 作为 top evidence 的比例：91.7%。
- Strict cases 中 Guide 命中率：100.0%。
- Strict cases 中 Wiki 全量命中率：100.0%。
- Guide-Wiki 精准组合率：100.0%。
- 本阶段只记录命中表现，不根据结果调整 profile、selector、ranking 或知识内容。

## 7. 是否需要 profile

暂不建议新增 profile；可将 Waste / Recycling v0.1 作为 stable candidate 继续观察。

## 8. 是否进入 Waste / Recycling Retrieval Root Cause Review

建议进入 Batch11-D Waste / Recycling Retrieval Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。

## 9. 逐条复盘

### waste_mixed_trash_sort_isolate

- query：家里产生一堆混合垃圾，应该先怎么分类和临时隔离？
- 类型：strict
- focus：Waste 主行动链起点：混合垃圾先分类、临时隔离、标记来源，不应被泛卫生建议替代。
- verdict：pass
- expected Guide：DG-0887
- allowed secondary：无
- selected Guide：DG-0887、DG-0889、DG-0550
- expected Wiki：hygiene-waste-basic-sorting-isolation-001
- selected Wiki：hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-child-access-control-001、hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-temporary-overflow-plan-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_broken_glass_collect_mark

- query：碎玻璃散在地上，怎么收集和标记才安全？
- 类型：strict
- focus：碎玻璃作为 waste sharp item 应由锋利/破碎废物处理入口主导，medical 只能在已受伤时补充。
- verdict：pass
- expected Guide：DG-0888
- allowed secondary：无
- selected Guide：DG-0888、DG-0086、DG-0887
- expected Wiki：hygiene-waste-sharp-glass-temporary-container-001
- selected Wiki：hygiene-waste-sharp-glass-temporary-container-001、hygiene-waste-metal-edge-scrap-isolation-001、hygiene-waste-child-access-control-001、repair-recycling-metal-sheet-intake-check-001、hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001、hygiene-waste-battery-leakage-boundary-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_metal_edge_nail_puncture

- query：废金属边角和钉子怎么处理，避免扎伤？
- 类型：strict
- focus：金属边角和钉子先做废弃物隔离和标记，再判断是否进入材料池。
- verdict：pass
- expected Guide：DG-0888
- allowed secondary：DG-0892
- selected Guide：DG-0888、DG-0878、DG-0838
- expected Wiki：hygiene-waste-metal-edge-scrap-isolation-001
- selected Wiki：hygiene-waste-sharp-glass-temporary-container-001、hygiene-waste-metal-edge-scrap-isolation-001、hygiene-waste-child-access-control-001、repair-recycling-metal-sheet-intake-check-001、repair-manufacturing-end-clean-count-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_battery_leak_common_trash

- query：电池漏液的垃圾能不能和普通垃圾放一起？
- 类型：strict
- focus：漏液物作为废弃物应先隔离并禁入普通垃圾；Energy/Medical 可补充人体或电池安全边界。
- verdict：pass
- expected Guide：DG-0887、DG-0889
- allowed secondary：DG-0112
- selected Guide：DG-0887、DG-0112、DG-0841
- expected Wiki：hygiene-waste-battery-leakage-boundary-001
- selected Wiki：hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-child-access-control-001、hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-temporary-overflow-plan-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_patient_tissue_common_trash

- query：病人用过的纸巾和垃圾能不能混进普通垃圾？
- 类型：strict
- focus：病人垃圾应进入污染物分流，不应混入普通垃圾；WASH 可作为清洁和照护补充。
- verdict：pass
- expected Guide：DG-0889
- allowed secondary：DG-0854
- selected Guide：DG-0889、DG-0887、DG-0854
- expected Wiki：hygiene-waste-patient-trash-double-bag-zone-001
- selected Wiki：hygiene-waste-patient-trash-double-bag-zone-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-contaminated-container-downgrade-001、hygiene-waste-patient-leftover-no-compost-001、hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_unknown_pungent_trash_hold

- query：不明刺鼻味的废弃物应该放在哪里？
- 类型：strict
- focus：不明刺鼻味废弃物应暂存、标记、隔离，不能进入普通垃圾或材料池。
- verdict：pass
- expected Guide：DG-0887、DG-0889
- allowed secondary：DG-0064
- selected Guide：DG-0887、DG-0894、DG-0847
- expected Wiki：hygiene-waste-unknown-chemical-item-hold-001
- selected Wiki：hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-child-access-control-001、hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-temporary-overflow-plan-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_rotten_kitchen_scrap_leachate_compost

- query：厨余已经发臭渗液，还能不能进堆肥？
- 类型：strict
- focus：Waste 应主导厨余进入 Agriculture 前分流；Agriculture 判断堆肥成熟和入土。
- verdict：pass
- expected Guide：DG-0890
- allowed secondary：DG-0882
- selected Guide：DG-0890、DG-0882、DG-0883
- expected Wiki：hygiene-waste-food-rot-wet-isolation-001
- selected Wiki：hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-patient-leftover-no-compost-001、agriculture-waste-kitchen-scrap-before-compost-001、hygiene-waste-oil-meat-odor-organic-boundary-001、agriculture-waste-compost-waiting-bin-distance-001、hygiene-waste-organic-pest-odor-daily-check-001、agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_patient_leftover_compost_bin

- query：病人剩饭能不能倒进厨余堆肥桶？
- 类型：strict
- focus：病人剩饭不直接进入堆肥，Waste 分流和 WASH 污染边界应协同。
- verdict：pass
- expected Guide：DG-0889、DG-0890
- allowed secondary：DG-0854
- selected Guide：DG-0890、DG-0882、DG-0889
- expected Wiki：hygiene-waste-patient-leftover-no-compost-001
- selected Wiki：hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-patient-leftover-no-compost-001、agriculture-waste-kitchen-scrap-before-compost-001、hygiene-waste-oil-meat-odor-organic-boundary-001、agriculture-waste-compost-waiting-bin-distance-001、hygiene-waste-organic-pest-odor-daily-check-001、agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_hot_ash_direct_trash_bin

- query：火堆刚灭的热灰能不能直接倒进垃圾桶？
- 类型：strict
- focus：热灰未冷却前由 Fire 停止线主导，同时 Waste 应承接不得进垃圾桶的分流边界。
- verdict：pass
- expected Guide：DG-0891
- allowed secondary：DG-0851
- selected Guide：DG-0891、DG-0851、DG-0550
- expected Wiki：fire-waste-hot-ash-not-trash-001
- selected Wiki：fire-waste-hot-ash-not-trash-001、fire-waste-cold-ash-storage-boundary-001、hygiene-waste-ash-trash-mixing-ban-001、fire-waste-charcoal-residue-reuse-check-001、agriculture-waste-ash-soil-use-interface-001、fire-ash-ember-cooling-disposal-001、fire-night-final-extinguish-log-001、fire-small-fire-stop-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_cold_ash_store_later_use

- query：冷灰能不能保存起来以后再用？
- 类型：strict
- focus：冷灰保存应进入灰烬与炭渣冷却后分流，不直接进入 Agriculture 施用判断。
- verdict：pass
- expected Guide：DG-0891
- allowed secondary：DG-0851
- selected Guide：DG-0891、DG-0851、DG-0879
- expected Wiki：fire-waste-cold-ash-storage-boundary-001
- selected Wiki：fire-waste-hot-ash-not-trash-001、fire-waste-cold-ash-storage-boundary-001、hygiene-waste-ash-trash-mixing-ban-001、fire-waste-charcoal-residue-reuse-check-001、agriculture-waste-ash-soil-use-interface-001、fire-ash-ember-cooling-disposal-001、fire-night-final-extinguish-log-001、fire-small-fire-stop-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_salvaged_wood_material_pool

- query：废木板能不能直接放进材料池？
- 类型：strict
- focus：废木板先做材料池准入判断，Manufacturing 只在后续加工制作时主导。
- verdict：pass
- expected Guide：DG-0892
- allowed secondary：DG-0872
- selected Guide：DG-0892、DG-0894、DG-0893
- expected Wiki：repair-recycling-salvaged-wood-intake-check-001
- selected Wiki：repair-recycling-salvaged-wood-intake-check-001、repair-recycling-metal-sheet-intake-check-001、repair-recycling-material-intake-checklist-001、repair-recycling-material-downgrade-label-001、repair-recycling-material-pool-zone-layout-001、hygiene-waste-metal-edge-scrap-isolation-001、repair-recycling-fabric-rope-intake-check-001、repair-recycling-fasteners-small-parts-sort-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_salvaged_metal_sheet_pool

- query：旧金属片能不能收进可再利用材料池？
- 类型：strict
- focus：旧金属片进入材料池前应检查锐边、锈蚀、油污和用途限制。
- verdict：pass
- expected Guide：DG-0892
- allowed secondary：DG-0876、DG-0888
- selected Guide：DG-0892、DG-0876、DG-0872
- expected Wiki：repair-recycling-metal-sheet-intake-check-001
- selected Wiki：repair-recycling-salvaged-wood-intake-check-001、repair-recycling-metal-sheet-intake-check-001、repair-recycling-material-intake-checklist-001、repair-recycling-material-downgrade-label-001、repair-recycling-material-pool-zone-layout-001、hygiene-waste-metal-edge-scrap-isolation-001、repair-recycling-fabric-rope-intake-check-001、repair-recycling-fasteners-small-parts-sort-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_plastic_bucket_odor_reuse

- query：旧塑料桶有异味，还能不能拿来装东西？
- 类型：strict
- focus：旧塑料桶应按来源、异味、残留和用途降级判断，不进入食物饮水链。
- verdict：pass
- expected Guide：DG-0893
- allowed secondary：DG-0892
- selected Guide：DG-0893、DG-0874、DG-0892
- expected Wiki：repair-recycling-plastic-container-intake-check-001
- selected Wiki：repair-recycling-plastic-container-intake-check-001、hygiene-waste-contaminated-container-downgrade-001、repair-recycling-cleanable-noncleanable-material-001、repair-recycling-material-downgrade-label-001、repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-frame-square-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_container_unknown_residue_reuse

- query：旧容器有不明残留，清一清能不能继续用？
- 类型：strict
- focus：不明残留容器应禁入饮水食物用途，不能用清洗动作掩盖来源不明。
- verdict：pass
- expected Guide：DG-0893
- allowed secondary：DG-0892
- selected Guide：DG-0893、DG-0894、DG-0892
- expected Wiki：hygiene-waste-contaminated-container-downgrade-001
- selected Wiki：repair-recycling-plastic-container-intake-check-001、hygiene-waste-contaminated-container-downgrade-001、repair-recycling-cleanable-noncleanable-material-001、repair-recycling-material-downgrade-label-001、general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001、general-waste-source-hazard-log-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_material_pool_ledger

- query：可用材料池怎么登记，避免以后没人知道能不能用？
- 类型：strict
- focus：材料池台账应记录来源、等级、用途限制、复查日期和交接信息。
- verdict：pass
- expected Guide：DG-0894
- allowed secondary：无
- selected Guide：DG-0894、DG-0892、DG-0893
- expected Wiki：general-recycling-material-pool-ledger-001
- selected Wiki：general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001、general-waste-source-hazard-log-001、general-waste-source-label-minimum-001、hygiene-waste-organic-pest-odor-daily-check-001、repair-recycling-fasteners-small-parts-sort-001、repair-recycling-material-downgrade-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_reuse_failure_record

- query：回收失败的材料要不要记录？
- 类型：strict
- focus：回收失败原因应记录，避免下一班重复误用或升级用途。
- verdict：pass
- expected Guide：DG-0894
- allowed secondary：DG-0892
- selected Guide：DG-0894、DG-0299、DG-0667
- expected Wiki：general-waste-reuse-failure-record-001
- selected Wiki：general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001、general-waste-source-hazard-log-001、general-waste-source-label-minimum-001、hygiene-waste-organic-pest-odor-daily-check-001、repair-recycling-fasteners-small-parts-sort-001、repair-recycling-material-downgrade-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_contaminated_bin_placement

- query：污染垃圾桶应该放在哪里，不能靠近哪些区域？
- 类型：strict
- focus：污染垃圾桶位置应远离饮水、食物、睡眠区和通道，并需要标记和复查。
- verdict：pass
- expected Guide：DG-0887、DG-0889
- allowed secondary：DG-0853
- selected Guide：DG-0887、DG-0889、DG-0086
- expected Wiki：hygiene-waste-basic-sorting-isolation-001
- selected Wiki：hygiene-waste-basic-sorting-isolation-001、general-waste-source-label-minimum-001、hygiene-waste-mixed-trash-stop-line-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-child-access-control-001、hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-temporary-overflow-plan-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_handover_minimum_fields

- query：废弃物交接时要告诉下一班什么？
- 类型：strict
- focus：废弃物交接应覆盖来源、风险、位置、下一步和处理人，形成记录闭环。
- verdict：pass
- expected Guide：DG-0894
- allowed secondary：无
- selected Guide：DG-0894、DG-0893、DG-0892
- expected Wiki：general-waste-disposal-handover-card-001
- selected Wiki：general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001、general-waste-source-hazard-log-001、general-waste-source-label-minimum-001、hygiene-waste-organic-pest-odor-daily-check-001、repair-recycling-fasteners-small-parts-sort-001、repair-recycling-material-downgrade-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_observe_kitchen_scrap_to_compost

- query：厨余能不能直接拿去堆肥？
- 类型：observation
- focus：观察 Agriculture 是否主导成熟和土壤使用，Waste 是否主导进入堆肥前分流。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0890、DG-0882、DG-0054
- expected Wiki：无
- selected Wiki：hygiene-waste-food-rot-wet-isolation-001、hygiene-waste-patient-leftover-no-compost-001、agriculture-waste-kitchen-scrap-before-compost-001、hygiene-waste-oil-meat-odor-organic-boundary-001、agriculture-waste-compost-waiting-bin-distance-001、hygiene-waste-organic-pest-odor-daily-check-001、agriculture-compost-maturity-second-check-001、agriculture-immature-compost-stop-line-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_observe_hot_ash_rekindle

- query：热灰怎么处理，防止复燃？
- 类型：observation
- focus：观察 Fire 是否主导火源安全，Waste 是否补充分流和记录。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0891、DG-0851、DG-0646
- expected Wiki：无
- selected Wiki：fire-waste-hot-ash-not-trash-001、fire-waste-cold-ash-storage-boundary-001、hygiene-waste-ash-trash-mixing-ban-001、fire-waste-charcoal-residue-reuse-check-001、agriculture-waste-ash-soil-use-interface-001、fire-ash-ember-cooling-disposal-001、fire-night-final-extinguish-log-001、fire-small-fire-stop-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_observe_wood_to_shelf

- query：废木板怎么做成架子？
- 类型：observation
- focus：观察 Manufacturing 是否主导制作，Waste 是否补充材料池判断。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0892、DG-0304、DG-0830
- expected Wiki：无
- selected Wiki：repair-recycling-salvaged-wood-intake-check-001、repair-recycling-metal-sheet-intake-check-001、repair-recycling-material-intake-checklist-001、repair-recycling-material-downgrade-label-001、repair-recycling-material-pool-zone-layout-001、hygiene-waste-metal-edge-scrap-isolation-001、repair-recycling-fabric-rope-intake-check-001、repair-recycling-fasteners-small-parts-sort-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_observe_patient_trash_contamination

- query：病人垃圾怎么处理才不会污染别人？
- 类型：observation
- focus：观察 WASH / Hygiene 是否主导人体卫生，Waste 是否补充分流和临时隔离。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0889、DG-0477、DG-0854
- expected Wiki：无
- selected Wiki：hygiene-waste-patient-trash-double-bag-zone-001、hygiene-waste-unknown-chemical-item-hold-001、hygiene-waste-battery-leakage-boundary-001、hygiene-waste-contaminated-container-downgrade-001、hygiene-waste-patient-leftover-no-compost-001、hygiene-patient-cup-towel-isolation-001、hygiene-kitchen-raw-cooked-contamination-line-001、hygiene-wash-abnormal-record-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### waste_observe_battery_leak_skin

- query：电池漏液碰到手怎么办？
- 类型：observation
- focus：观察 Medical / Contamination 是否主导人体接触，Waste 是否补充漏液物隔离。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0112、DG-0841、DG-0064
- expected Wiki：无
- selected Wiki：medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001、hygiene-isolation-supplies-001、energy-battery-parallel-series-boundary-001、energy-battery-chemistry-mix-stop-001、energy-battery-leak-corrosion-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、waste_recycling_vs_medical、waste_recycling_vs_energy
- root cause：无
- failure reasons：无

### waste_observe_glass_cut_hand

- query：碎玻璃割伤了手怎么办？
- 类型：observation
- focus：观察 Medical 是否主导伤口处理，Waste 是否补充剩余碎玻璃隔离。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0632、DG-0888、DG-0625
- expected Wiki：无
- selected Wiki：fire-small-fire-stop-001、safety-barter-002、safety-buddy-system-001、safety-children-003、safety-conflict-002、safety-conflict-003、safety-door-window-001、safety-electric-shock-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、waste_recycling_vs_safety
- root cause：无
- failure reasons：无

## 10. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_waste_recycling_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_waste_recycling_field.py --no-answer
```

边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。
