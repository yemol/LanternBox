# Batch12-D Long-Term Storage Retrieval Field Test Report

生成时间：2026-07-18T14:51:11.016617+00:00

## 1. 测试范围

本阶段只测试 Batch12-C 新增 Long-Term Storage Guide/Wiki/evidence chain 是否稳定进入本地 Retrieval selected evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。

覆盖：储藏分区与标签、干粮防潮防虫、鼠咬/霉味批次隔离、先入先出、种子储藏与轮换、急救包和医疗物资复查、工具材料储藏、电池/燃料储藏、污染/漏液/过期物隔离、储藏交接记录。

说明：Batch12-D 请求中的 `storage-*` Wiki 名称为规划期占位；Batch12-C 为遵守现有 slug domain 白名单，实际落地使用 `general-storage-*`、`food-storage-*`、`agriculture-storage-*`、`medical-storage-*`、`repair-storage-*`、`energy-storage-*`、`fire-storage-*`、`hygiene-storage-*`。本 fixture 使用实际有效 slug。

## 2. strict / observation 数量

- 用例总数：26
- strict / observation：20 / 6
- pass / partial / fail：26 / 0 / 0
- strict pass / partial / fail：20 / 0 / 0

## 3. Guide / Wiki 命中

- Guide 命中率（strict，含 allowed secondary）：100.0%
- 主 Guide 命中率（strict，仅 expected）：100.0%
- Wiki 全量命中率（strict，全部 expected Wiki）：100.0%
- Wiki 任一命中率（strict，至少一个 expected Wiki）：100.0%
- Guide-Wiki 精准组合率（strict）：100.0%
- Long-Term Storage 主 Guide 进入率（全部 cases）：88.5%

## 4. 安全边界

- safety boundary：100.0%
- fallback：100.0%
- record/check：100.0%
- dangerous suggestion：0
- Kiwix 越权：0

## 5. Case 明细

|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|
|---|---|---|---|---|---|---|---|
|storage_zone_labeling|strict|pass|DG-0895 储藏区基础分区与标签、DG-0902 储藏记录、先入先出与交接、DG-0896 干粮 / 米面豆类防潮防虫储藏|general-storage-zone-basic-layout-001 长期储藏区基础分区、general-storage-label-minimum-fields-001 储藏标签最小字段、general-storage-moisture-daily-check-001 储藏区潮湿日查、general-storage-pest-rodent-check-001 储藏区虫鼠巡查、general-storage-fifo-shelf-rule-001 先入先出货架规则、repair-storage-shelf-overload-warning-001 储藏架超载警示、general-storage-handover-card-001 储藏区交接卡、general-storage-inventory-card-minimum-001 储藏库存卡最小字段|storage_basic_zone_labeling|无|无|
|storage_dry_grain_moisture_pest|strict|pass|DG-0896 干粮 / 米面豆类防潮防虫储藏、DG-0902 储藏记录、先入先出与交接、DG-0901 霉变 / 漏液 / 过期 / 污染物隔离|food-storage-dry-grain-zone-001 干粮和米面豆类储藏区、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离、general-storage-pest-rodent-check-001 储藏区虫鼠巡查、food-storage-can-jar-monthly-check-001 罐头和密封食品月度复查、food-storage-opened-package-short-use-001 开封食品短期使用标签、food-storage-emergency-ration-reserve-line-001 应急口粮保底线、general-storage-handover-card-001 储藏区交接卡|storage_food_grain_rotation|无|无|
|storage_rodent_food_batch_isolation|strict|pass|DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0896 干粮 / 米面豆类防潮防虫储藏、DG-0902 储藏记录、先入先出与交接|hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡、hygiene-storage-contaminated-container-hold-001 污染容器暂存标签、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离|storage_food_grain_rotation|无|无|
|storage_moldy_dry_food_batch_hold|strict|pass|DG-0896 干粮 / 米面豆类防潮防虫储藏、DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0902 储藏记录、先入先出与交接|food-storage-dry-grain-zone-001 干粮和米面豆类储藏区、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离、general-storage-pest-rodent-check-001 储藏区虫鼠巡查、food-storage-can-jar-monthly-check-001 罐头和密封食品月度复查、food-storage-opened-package-short-use-001 开封食品短期使用标签、food-storage-emergency-ration-reserve-line-001 应急口粮保底线、hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离|storage_food_grain_rotation|无|无|
|storage_fifo_grain_rotation|strict|pass|DG-0902 储藏记录、先入先出与交接、DG-0896 干粮 / 米面豆类防潮防虫储藏、DG-0901 霉变 / 漏液 / 过期 / 污染物隔离|general-storage-handover-card-001 储藏区交接卡、general-storage-inventory-card-minimum-001 储藏库存卡最小字段、general-storage-check-calendar-001 储藏复查日历、general-storage-issue-log-001 储藏异常记录、general-storage-fifo-shelf-rule-001 先入先出货架规则、general-storage-label-minimum-fields-001 储藏标签最小字段、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签|storage_food_grain_rotation、storage_inventory_handover|无|无|
|storage_seed_moisture_recheck|strict|pass|DG-0897 种子储藏与复查、DG-0879 种子保存与发芽率复测|agriculture-storage-seed-moisture-quarantine-001 受潮种子待复测隔离、agriculture-storage-seed-food-separation-shelf-001 留种和食用批次分架、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签、agriculture-storage-seed-box-dry-check-001 种子盒干燥复查、agriculture-seed-batch-viability-ledger-001 种子批次与发芽率复测台账、agriculture-seed-drying-recheck-001 种子干燥后的复查、agriculture-seed-storage-moisture-failure-001 种子受潮失效判断|storage_seed_storage|无|无|
|storage_seed_food_separation|strict|pass|DG-0897 种子储藏与复查、DG-0879 种子保存与发芽率复测、DG-0886 小规模粮食生产优先级|agriculture-storage-seed-moisture-quarantine-001 受潮种子待复测隔离、agriculture-storage-seed-food-separation-shelf-001 留种和食用批次分架、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签、agriculture-storage-seed-box-dry-check-001 种子盒干燥复查、agriculture-seed-batch-viability-ledger-001 种子批次与发芽率复测台账、agriculture-seed-drying-recheck-001 种子干燥后的复查、agriculture-seed-storage-moisture-failure-001 种子受潮失效判断|agriculture_seed_library、agriculture_postharvest_storage、storage_seed_storage|无|无|
|storage_seed_library_rotation_record|strict|pass|DG-0897 种子储藏与复查、DG-0902 储藏记录、先入先出与交接、DG-0232 种子库存管理|agriculture-storage-seed-moisture-quarantine-001 受潮种子待复测隔离、agriculture-storage-seed-food-separation-shelf-001 留种和食用批次分架、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签、agriculture-storage-seed-box-dry-check-001 种子盒干燥复查、general-storage-handover-card-001 储藏区交接卡、general-storage-inventory-card-minimum-001 储藏库存卡最小字段、general-storage-check-calendar-001 储藏复查日历|storage_seed_storage、storage_inventory_handover|无|无|
|storage_first_aid_kit_review|strict|pass|DG-0898 医疗物资与急救包储藏复查、DG-0901 霉变 / 漏液 / 过期 / 污染物隔离|medical-storage-dressing-bandage-dry-pack-001 敷料和绷带干燥封存、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、medical-storage-first-aid-kit-monthly-check-001 急救包月度复查、medical-storage-medicine-dry-dark-box-001 药品避光防潮盒、medical-storage-care-supply-zone-001 照护用品储藏分区、hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡|storage_medical_supply_storage|无|无|
|storage_damaged_medicine_package|strict|pass|DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0898 医疗物资与急救包储藏复查|hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡、hygiene-storage-contaminated-container-hold-001 污染容器暂存标签、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离|storage_medical_supply_storage、storage_suspicious_item_isolation|无|无|
|storage_dressing_bandage_dry|strict|pass|DG-0898 医疗物资与急救包储藏复查、DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0896 干粮 / 米面豆类防潮防虫储藏|medical-storage-dressing-bandage-dry-pack-001 敷料和绷带干燥封存、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、medical-storage-first-aid-kit-monthly-check-001 急救包月度复查、medical-storage-medicine-dry-dark-box-001 药品避光防潮盒、medical-storage-care-supply-zone-001 照护用品储藏分区、hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡|storage_medical_supply_storage|无|无|
|storage_tool_rust_prevention|strict|pass|DG-0899 工具和材料防潮防锈储藏、DG-0895 储藏区基础分区与标签、DG-0373 工具防锈防潮|repair-storage-tool-dry-rust-check-001 工具干燥防锈储藏、repair-storage-sharp-tool-secure-box-001 刃具和锋利工具安全盒、repair-storage-small-parts-bin-label-001 小零件盒分类标签、repair-storage-fabric-rope-dry-check-001 布料和绳索储藏复查、repair-storage-wood-metal-material-stack-001 木材金属片堆放边界、repair-storage-shelf-overload-warning-001 储藏架超载警示、general-storage-zone-basic-layout-001 长期储藏区基础分区、general-storage-label-minimum-fields-001 储藏标签最小字段|storage_tools_materials_storage|无|无|
|storage_sharp_tool_safe_box|strict|pass|DG-0899 工具和材料防潮防锈储藏、DG-0888 锋利 / 破碎 / 金属边角废物处理|repair-storage-tool-dry-rust-check-001 工具干燥防锈储藏、repair-storage-sharp-tool-secure-box-001 刃具和锋利工具安全盒、repair-storage-small-parts-bin-label-001 小零件盒分类标签、repair-storage-fabric-rope-dry-check-001 布料和绳索储藏复查、repair-storage-wood-metal-material-stack-001 木材金属片堆放边界、repair-storage-shelf-overload-warning-001 储藏架超载警示、hygiene-waste-sharp-glass-temporary-container-001 碎玻璃和锋利物临时容器、hygiene-waste-metal-edge-scrap-isolation-001 金属边角废料隔离|storage_tools_materials_storage|无|无|
|storage_small_parts_sorting|strict|pass|DG-0899 工具和材料防潮防锈储藏、DG-0309 临时小零件盒、DG-0272 设备外借和维修前清理|repair-storage-tool-dry-rust-check-001 工具干燥防锈储藏、repair-storage-sharp-tool-secure-box-001 刃具和锋利工具安全盒、repair-storage-small-parts-bin-label-001 小零件盒分类标签、repair-storage-fabric-rope-dry-check-001 布料和绳索储藏复查、repair-storage-wood-metal-material-stack-001 木材金属片堆放边界、repair-storage-shelf-overload-warning-001 储藏架超载警示|storage_tools_materials_storage|无|无|
|storage_battery_power_bank_zone|strict|pass|DG-0900 能源 / 燃料物资安全储藏、DG-0895 储藏区基础分区与标签、DG-0902 储藏记录、先入先出与交接|energy-storage-battery-dry-cool-zone-001 电池干燥阴凉储藏区、energy-storage-power-bank-rotation-label-001 充电宝轮换标签、fire-storage-match-candle-dry-box-001 火柴蜡烛防潮盒、fire-storage-fuel-away-from-living-zone-001 燃料远离生活区储藏线、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、general-storage-zone-basic-layout-001 长期储藏区基础分区、general-storage-label-minimum-fields-001 储藏标签最小字段、general-storage-moisture-daily-check-001 储藏区潮湿日查|storage_basic_zone_labeling、storage_energy_fuel_storage|无|无|
|storage_matches_ignition_dry_check|strict|pass|DG-0900 能源 / 燃料物资安全储藏、DG-0899 工具和材料防潮防锈储藏|energy-storage-battery-dry-cool-zone-001 电池干燥阴凉储藏区、energy-storage-power-bank-rotation-label-001 充电宝轮换标签、fire-storage-match-candle-dry-box-001 火柴蜡烛防潮盒、fire-storage-fuel-away-from-living-zone-001 燃料远离生活区储藏线、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、repair-storage-tool-dry-rust-check-001 工具干燥防锈储藏、repair-storage-sharp-tool-secure-box-001 刃具和锋利工具安全盒、repair-storage-small-parts-bin-label-001 小零件盒分类标签|storage_tools_materials_storage、storage_energy_fuel_storage|无|无|
|storage_fuel_common_goods_boundary|strict|pass|DG-0900 能源 / 燃料物资安全储藏|energy-storage-battery-dry-cool-zone-001 电池干燥阴凉储藏区、energy-storage-power-bank-rotation-label-001 充电宝轮换标签、fire-storage-match-candle-dry-box-001 火柴蜡烛防潮盒、fire-storage-fuel-away-from-living-zone-001 燃料远离生活区储藏线、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒|storage_energy_fuel_storage|无|无|
|storage_leak_odor_item_isolation|strict|pass|DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0895 储藏区基础分区与标签、DG-0112 电池漏液处理|hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡、hygiene-storage-contaminated-container-hold-001 污染容器暂存标签、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离|medical_poisoning_chemical_exposure、storage_basic_zone_labeling、storage_suspicious_item_isolation|无|无|
|storage_suspicious_batch_review_label|strict|pass|DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0895 储藏区基础分区与标签、DG-0902 储藏记录、先入先出与交接|hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡、hygiene-storage-contaminated-container-hold-001 污染容器暂存标签、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离|storage_basic_zone_labeling、storage_suspicious_item_isolation|无|无|
|storage_handover_minimum_fields|strict|pass|DG-0902 储藏记录、先入先出与交接、DG-0895 储藏区基础分区与标签|general-storage-handover-card-001 储藏区交接卡、general-storage-inventory-card-minimum-001 储藏库存卡最小字段、general-storage-check-calendar-001 储藏复查日历、general-storage-issue-log-001 储藏异常记录、general-storage-fifo-shelf-rule-001 先入先出货架规则、general-storage-label-minimum-fields-001 储藏标签最小字段、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签|inventory_consumption、storage_basic_zone_labeling、storage_inventory_handover|无|无|
|obs_storage_moldy_food_eat|observation|pass|DG-0608 霉变粮食和坚果停用、DG-0628 霉味衣物和睡眠区分离、DG-0886 小规模粮食生产优先级|food-canned-food-001 干粮和罐头三天配给原则、food-canned-food-002 干货罐头轮换和先开先用、food-children-001 儿童老人食物优先级判断、food-cooking-001 低燃料烹饪对食物安全的影响、food-food-knowledge-001 即食食品和需煮食品的排序、food-food-knowledge-002 不可食残渣和可食剩余的分界、food-food-knowledge-003 断供期间食物心理安慰与纪律、food-food-poisoning-001 多人共餐时食物中毒追溯|无|off_domain_primary、storage_vs_food|无|
|obs_storage_seed_moisture_sow|observation|pass|DG-0897 种子储藏与复查、DG-0879 种子保存与发芽率复测、DG-0886 小规模粮食生产优先级|agriculture-storage-seed-moisture-quarantine-001 受潮种子待复测隔离、agriculture-storage-seed-food-separation-shelf-001 留种和食用批次分架、agriculture-storage-seed-library-rotation-card-001 小型种子库轮换卡、agriculture-storage-seed-bag-label-001 种子袋批次标签、agriculture-storage-seed-box-dry-check-001 种子盒干燥复查、agriculture-seed-batch-viability-ledger-001 种子批次与发芽率复测台账、agriculture-seed-drying-recheck-001 种子干燥后的复查、agriculture-seed-storage-moisture-failure-001 种子受潮失效判断|agriculture_seed_library、storage_seed_storage|无|无|
|obs_storage_expired_medicine_use|observation|pass|DG-0901 霉变 / 漏液 / 过期 / 污染物隔离、DG-0587 必须转移时：水食物药品证件优先级、DG-0898 医疗物资与急救包储藏复查|hygiene-storage-leak-odor-isolation-001 漏液异味物品隔离、general-storage-suspect-batch-quarantine-001 可疑批次隔离区、general-storage-expired-item-hold-card-001 过期物品暂存卡、hygiene-storage-contaminated-container-hold-001 污染容器暂存标签、medical-storage-expired-unknown-medicine-hold-001 过期和不明药品暂存、energy-storage-battery-leak-suspect-bin-001 漏液疑似电池隔离盒、food-storage-grain-moisture-clump-check-001 米面豆类受潮结块检查、food-storage-rodent-bite-isolation-001 鼠咬食物批次隔离|无|无|无|
|obs_storage_shelf_stronger|observation|pass|DG-0899 工具和材料防潮防锈储藏、DG-0059 火场返回前判断、DG-0079 撤离后回取物资判断|repair-storage-tool-dry-rust-check-001 工具干燥防锈储藏、repair-storage-sharp-tool-secure-box-001 刃具和锋利工具安全盒、repair-storage-small-parts-bin-label-001 小零件盒分类标签、repair-storage-fabric-rope-dry-check-001 布料和绳索储藏复查、repair-storage-wood-metal-material-stack-001 木材金属片堆放边界、repair-storage-shelf-overload-warning-001 储藏架超载警示|无|无|无|
|obs_storage_battery_leak_skin|observation|pass|DG-0112 电池漏液处理、DG-0841 电池异常隔离、DG-0064 疑似化学污染：远离、脱外层、冲洗|medical-chemical-skin-eye-exposure-001 化学物接触皮肤和眼睛的冲洗边界、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁、hygiene-contamination-log-001 污染记录帮助追溯来源、hygiene-isolation-supplies-001 隔离用品不足时的替代、energy-battery-parallel-series-boundary-001 电池串并联禁用边界、energy-battery-chemistry-mix-stop-001 不同电池类型混用停用边界、energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离|energy_battery_abnormal_isolation、medical_poisoning_chemical_exposure|off_domain_primary、storage_vs_medical、storage_vs_waste、storage_vs_energy|无|
|obs_storage_unknown_residue_container|observation|pass|DG-0893 塑料桶 / 容器再利用前判断、DG-0894 废弃物与材料池记录交接、DG-0899 工具和材料防潮防锈储藏|repair-recycling-plastic-container-intake-check-001 塑料容器入池判断、hygiene-waste-contaminated-container-downgrade-001 污染容器降级和禁用、repair-recycling-cleanable-noncleanable-material-001 可清洁材料和不可再用材料分界、repair-recycling-material-downgrade-label-001 材料降级用途标签、general-recycling-material-pool-ledger-001 材料池台账最小字段、general-waste-reuse-failure-record-001 回收再利用失败记录、general-waste-disposal-handover-card-001 废弃物处理交接卡、general-waste-source-hazard-log-001 废弃物来源和风险记录|waste_material_pool_reuse|off_domain_primary、storage_vs_manufacturing、storage_vs_waste、storage_vs_hygiene|无|

## 6. Cross Domain 统计

- food 抢主位观察：2
- agriculture 抢主位观察：0
- medical 抢主位观察：1
- manufacturing 抢主位观察：1
- waste 抢主位观察：3
- hygiene 抢主位观察：2
- contamination 抢主位观察：1
- energy 抢主位观察：1
- fire 抢主位观察：0
- repair 抢主位观察：0
- tools 抢主位观察：0
- records 抢主位观察：0

Cross domain labels：
- off_domain_primary：3
- storage_vs_food：1
- storage_vs_medical：1
- storage_vs_waste：2
- storage_vs_energy：1
- storage_vs_manufacturing：1
- storage_vs_hygiene：1

## 7. 重点分析

- Food 抢粮食霉味 / 食物能不能吃主位：入口食用判断由 Food 主导是合理的；未入口前的批次隔离、标签和复查应有 Storage evidence。
- Agriculture 抢种子受潮 / 发芽率复测主位：播种可行性和复测由 Agriculture 主导；储藏盒、潮湿隔离、种食分离和轮换记录应保留 Storage 入口。
- Medical 抢药品过期 / 医疗物资主位：治疗和药品使用判断由 Medical 主导；药箱分区、急救包复查、破损隔离不能被百科或药物判断替代。
- Manufacturing 抢货架 / 容器 / 承重主位：制作货架归 Manufacturing；Storage 应补充堆叠、超载警示、分区和交接需求。
- Waste / Recycling 抢污染 / 漏液 / 过期物隔离主位：最终废弃物流向归 Waste；Storage 负责从主库存移出、禁用标签、待复查和交接。
- Energy / Fire 抢电池、燃料和火柴主位：使用和火源安全归相邻领域；Storage 需要保持保存位置、禁火距离、轮换标签和异常隔离。
- Tools / Repair 抢工具防锈 / 刃具存放主位：维修归 Repair；Storage 需要进入干燥、防锈、刃口封存和儿童隔离 evidence。

## 8. Long-Term Storage Evidence 稳定性

- Long-Term Storage Guide 作为 top evidence 的比例：88.5%。
- Strict cases 中 Guide 命中率：100.0%。
- Strict cases 中 Wiki 全量命中率：100.0%。
- Guide-Wiki 精准组合率：100.0%。
- 本阶段只记录命中表现，不根据结果调整 profile、selector、ranking 或知识内容。

## 9. 是否需要 profile

暂不建议新增 profile；可将 Long-Term Storage v0.1 作为 stable candidate 继续观察。

## 10. 是否进入 Root Cause Review

建议进入 Batch12-E Long-Term Storage Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。

## 11. 逐条复盘

### storage_zone_labeling

- query：家里的储藏区应该怎么分区和贴标签？
- 类型：strict
- focus：Storage 主行动链起点：分区、标签、可疑区、复查日和交接，不应被泛记录或安全建议替代。
- verdict：pass
- expected Guide：DG-0895
- allowed secondary：无
- selected Guide：DG-0895、DG-0902、DG-0896
- expected Wiki：general-storage-zone-basic-layout-001
- selected Wiki：general-storage-zone-basic-layout-001、general-storage-label-minimum-fields-001、general-storage-moisture-daily-check-001、general-storage-pest-rodent-check-001、general-storage-fifo-shelf-rule-001、repair-storage-shelf-overload-warning-001、general-storage-handover-card-001、general-storage-inventory-card-minimum-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_dry_grain_moisture_pest

- query：米面豆类怎么防潮防虫长期保存？
- 类型：strict
- focus：干粮长期保存应由 Storage/Food storage evidence 主导，入口食用判断只作为后续补充。
- verdict：pass
- expected Guide：DG-0896
- allowed secondary：无
- selected Guide：DG-0896、DG-0902、DG-0901
- expected Wiki：food-storage-dry-grain-zone-001
- selected Wiki：food-storage-dry-grain-zone-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001、general-storage-pest-rodent-check-001、food-storage-can-jar-monthly-check-001、food-storage-opened-package-short-use-001、food-storage-emergency-ration-reserve-line-001、general-storage-handover-card-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_rodent_food_batch_isolation

- query：储粮区发现鼠咬痕迹，要怎么隔离批次？
- 类型：strict
- focus：鼠咬食物先做批次隔离和记录，不直接进入食用判断。
- verdict：pass
- expected Guide：DG-0896、DG-0901
- allowed secondary：无
- selected Guide：DG-0901、DG-0896、DG-0902
- expected Wiki：food-storage-rodent-bite-isolation-001
- selected Wiki：hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001、hygiene-storage-contaminated-container-hold-001、medical-storage-expired-unknown-medicine-hold-001、energy-storage-battery-leak-suspect-bin-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_moldy_dry_food_batch_hold

- query：干粮有霉味，但还没准备吃，应该怎么处理这批？
- 类型：strict
- focus：霉味干粮在未入口前应由储藏批次隔离主导，Food safety 可补充不可入口边界。
- verdict：pass
- expected Guide：DG-0896、DG-0901
- allowed secondary：无
- selected Guide：DG-0896、DG-0901、DG-0902
- expected Wiki：food-storage-grain-moisture-clump-check-001
- selected Wiki：food-storage-dry-grain-zone-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001、general-storage-pest-rodent-check-001、food-storage-can-jar-monthly-check-001、food-storage-opened-package-short-use-001、food-storage-emergency-ration-reserve-line-001、hygiene-storage-leak-odor-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_fifo_grain_rotation

- query：储粮应该怎么做先入先出，避免旧粮被忘掉？
- 类型：strict
- focus：先入先出是 Storage 记录和轮换动作，不应被一般食物库存泛化。
- verdict：pass
- expected Guide：DG-0896、DG-0902
- allowed secondary：无
- selected Guide：DG-0902、DG-0896、DG-0901
- expected Wiki：general-storage-fifo-shelf-rule-001
- selected Wiki：general-storage-handover-card-001、general-storage-inventory-card-minimum-001、general-storage-check-calendar-001、general-storage-issue-log-001、general-storage-fifo-shelf-rule-001、general-storage-label-minimum-fields-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_seed_moisture_recheck

- query：种子袋有潮气，应该怎么复查和隔离？
- 类型：strict
- focus：受潮种子储藏复查和待复测隔离应命中 DG-0897，Agriculture 生产计划可补充。
- verdict：pass
- expected Guide：DG-0897
- allowed secondary：无
- selected Guide：DG-0897、DG-0879
- expected Wiki：agriculture-storage-seed-moisture-quarantine-001
- selected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-food-separation-shelf-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001、agriculture-storage-seed-box-dry-check-001、agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_seed_food_separation

- query：留种和食用种子应该怎么分开放？
- 类型：strict
- focus：留种和食用分离属于种子 storage 主链，应避免被普通 food storage 抢走。
- verdict：pass
- expected Guide：DG-0897
- allowed secondary：无
- selected Guide：DG-0897、DG-0879、DG-0886
- expected Wiki：agriculture-storage-seed-food-separation-shelf-001
- selected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-food-separation-shelf-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001、agriculture-storage-seed-box-dry-check-001、agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_seed_library_rotation_record

- query：种子库怎么做轮换记录？
- 类型：strict
- focus：种子库轮换记录应由种子储藏入口主导，通用交接记录可为 secondary。
- verdict：pass
- expected Guide：DG-0897
- allowed secondary：DG-0902
- selected Guide：DG-0897、DG-0902、DG-0232
- expected Wiki：agriculture-storage-seed-library-rotation-card-001
- selected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-food-separation-shelf-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001、agriculture-storage-seed-box-dry-check-001、general-storage-handover-card-001、general-storage-inventory-card-minimum-001、general-storage-check-calendar-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_first_aid_kit_review

- query：急救包多久复查一次，最少看哪些东西？
- 类型：strict
- focus：急救包储藏复查应命中 DG-0898，不做医疗治疗建议。
- verdict：pass
- expected Guide：DG-0898
- allowed secondary：无
- selected Guide：DG-0898、DG-0901
- expected Wiki：medical-storage-first-aid-kit-monthly-check-001
- selected Wiki：medical-storage-dressing-bandage-dry-pack-001、medical-storage-expired-unknown-medicine-hold-001、medical-storage-first-aid-kit-monthly-check-001、medical-storage-medicine-dry-dark-box-001、medical-storage-care-supply-zone-001、hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_damaged_medicine_package

- query：药品包装破损了，能不能放回药箱？
- 类型：strict
- focus：包装破损药品应由储藏隔离和药箱复查主导，不做过期药品可用性判断。
- verdict：pass
- expected Guide：DG-0898、DG-0901
- allowed secondary：无
- selected Guide：DG-0901、DG-0898
- expected Wiki：medical-storage-expired-unknown-medicine-hold-001
- selected Wiki：hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001、hygiene-storage-contaminated-container-hold-001、medical-storage-expired-unknown-medicine-hold-001、energy-storage-battery-leak-suspect-bin-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_dressing_bandage_dry

- query：敷料和绷带怎么防潮保存？
- 类型：strict
- focus：敷料绷带保存应进入医疗物资 storage evidence，Medical 处置只补充。
- verdict：pass
- expected Guide：DG-0898
- allowed secondary：无
- selected Guide：DG-0898、DG-0901、DG-0896
- expected Wiki：medical-storage-dressing-bandage-dry-pack-001
- selected Wiki：medical-storage-dressing-bandage-dry-pack-001、medical-storage-expired-unknown-medicine-hold-001、medical-storage-first-aid-kit-monthly-check-001、medical-storage-medicine-dry-dark-box-001、medical-storage-care-supply-zone-001、hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_tool_rust_prevention

- query：工具怎么防锈储藏，避免下次不能用？
- 类型：strict
- focus：工具防锈储藏应命中 DG-0899，Repair 维修不是主入口。
- verdict：pass
- expected Guide：DG-0899
- allowed secondary：无
- selected Guide：DG-0899、DG-0895、DG-0373
- expected Wiki：repair-storage-tool-dry-rust-check-001
- selected Wiki：repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001、repair-storage-fabric-rope-dry-check-001、repair-storage-wood-metal-material-stack-001、repair-storage-shelf-overload-warning-001、general-storage-zone-basic-layout-001、general-storage-label-minimum-fields-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_sharp_tool_safe_box

- query：刃具和锋利工具应该怎么安全存放？
- 类型：strict
- focus：锋利工具可用储藏和儿童隔离应由 Storage 工具材料入口主导。
- verdict：pass
- expected Guide：DG-0899
- allowed secondary：DG-0901
- selected Guide：DG-0899、DG-0888
- expected Wiki：repair-storage-sharp-tool-secure-box-001
- selected Wiki：repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001、repair-storage-fabric-rope-dry-check-001、repair-storage-wood-metal-material-stack-001、repair-storage-shelf-overload-warning-001、hygiene-waste-sharp-glass-temporary-container-001、hygiene-waste-metal-edge-scrap-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_small_parts_sorting

- query：小零件怎么分类，避免维修时找不到？
- 类型：strict
- focus：小零件分类标签和领用记录应进入 Storage 记录链，不只是 repair 泛建议。
- verdict：pass
- expected Guide：DG-0899、DG-0902
- allowed secondary：无
- selected Guide：DG-0899、DG-0309、DG-0272
- expected Wiki：repair-storage-small-parts-bin-label-001
- selected Wiki：repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001、repair-storage-fabric-rope-dry-check-001、repair-storage-wood-metal-material-stack-001、repair-storage-shelf-overload-warning-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_battery_power_bank_zone

- query：电池和充电宝长期不用，应该怎么分区保存？
- 类型：strict
- focus：电池和充电宝长期保存应命中 DG-0900，Energy 使用维护只作为补充。
- verdict：pass
- expected Guide：DG-0900
- allowed secondary：无
- selected Guide：DG-0900、DG-0895、DG-0902
- expected Wiki：energy-storage-battery-dry-cool-zone-001
- selected Wiki：energy-storage-battery-dry-cool-zone-001、energy-storage-power-bank-rotation-label-001、fire-storage-match-candle-dry-box-001、fire-storage-fuel-away-from-living-zone-001、energy-storage-battery-leak-suspect-bin-001、general-storage-zone-basic-layout-001、general-storage-label-minimum-fields-001、general-storage-moisture-daily-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_matches_ignition_dry_check

- query：火柴和点火材料受潮，应该怎么保存和复查？
- 类型：strict
- focus：点火材料防潮储藏应由能源/燃料 storage 入口主导，Fire 使用边界补充。
- verdict：pass
- expected Guide：DG-0900
- allowed secondary：无
- selected Guide：DG-0900、DG-0899
- expected Wiki：fire-storage-match-candle-dry-box-001
- selected Wiki：energy-storage-battery-dry-cool-zone-001、energy-storage-power-bank-rotation-label-001、fire-storage-match-candle-dry-box-001、fire-storage-fuel-away-from-living-zone-001、energy-storage-battery-leak-suspect-bin-001、repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_fuel_common_goods_boundary

- query：燃料能不能和普通物资放在一起？
- 类型：strict
- focus：燃料储藏分区和远离火源是 Storage/Energy-Fire 边界，不应被泛 fire 操作替代。
- verdict：pass
- expected Guide：DG-0900
- allowed secondary：DG-0901
- selected Guide：DG-0900
- expected Wiki：fire-storage-fuel-away-from-living-zone-001
- selected Wiki：energy-storage-battery-dry-cool-zone-001、energy-storage-power-bank-rotation-label-001、fire-storage-match-candle-dry-box-001、fire-storage-fuel-away-from-living-zone-001、energy-storage-battery-leak-suspect-bin-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_leak_odor_item_isolation

- query：有漏液和刺鼻味的物品，应该放进正常仓库吗？
- 类型：strict
- focus：漏液异味物品应命中隔离入口，Waste/WASH 只作为交接和清洁补充。
- verdict：pass
- expected Guide：DG-0901
- allowed secondary：无
- selected Guide：DG-0901、DG-0895、DG-0112
- expected Wiki：hygiene-storage-leak-odor-isolation-001
- selected Wiki：hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001、hygiene-storage-contaminated-container-hold-001、medical-storage-expired-unknown-medicine-hold-001、energy-storage-battery-leak-suspect-bin-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_suspicious_batch_review_label

- query：过期或可疑批次应该怎么贴标签，等复查？
- 类型：strict
- focus：过期和可疑批次应先隔离、禁用或待复查标签，再交接。
- verdict：pass
- expected Guide：DG-0901
- allowed secondary：DG-0902
- selected Guide：DG-0901、DG-0895、DG-0902
- expected Wiki：general-storage-suspect-batch-quarantine-001
- selected Wiki：hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001、hygiene-storage-contaminated-container-hold-001、medical-storage-expired-unknown-medicine-hold-001、energy-storage-battery-leak-suspect-bin-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### storage_handover_minimum_fields

- query：储藏区交接时，下一班最少要知道什么？
- 类型：strict
- focus：储藏交接最小字段应进入 DG-0902，不应被泛团队记录替代。
- verdict：pass
- expected Guide：DG-0902
- allowed secondary：无
- selected Guide：DG-0902、DG-0895
- expected Wiki：general-storage-handover-card-001
- selected Wiki：general-storage-handover-card-001、general-storage-inventory-card-minimum-001、general-storage-check-calendar-001、general-storage-issue-log-001、general-storage-fifo-shelf-rule-001、general-storage-label-minimum-fields-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### obs_storage_moldy_food_eat

- query：粮食有霉味还能不能吃？
- 类型：observation
- focus：Food 可以主导入口食用判断；观察 Storage 是否补充批次隔离和记录。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0608、DG-0628、DG-0886
- expected Wiki：无
- selected Wiki：food-canned-food-001、food-canned-food-002、food-children-001、food-cooking-001、food-food-knowledge-001、food-food-knowledge-002、food-food-knowledge-003、food-food-poisoning-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、storage_vs_food
- root cause：无
- failure reasons：无

### obs_storage_seed_moisture_sow

- query：种子受潮还能不能播？
- 类型：observation
- focus：Agriculture 可以主导播种/发芽率判断；观察 Storage 是否补充储藏复查。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0897、DG-0879、DG-0886
- expected Wiki：无
- selected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-food-separation-shelf-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001、agriculture-storage-seed-box-dry-check-001、agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### obs_storage_expired_medicine_use

- query：药品过期还能不能用？
- 类型：observation
- focus：Medical 可以主导用药判断；观察 Storage 是否补充过期标记和隔离。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0901、DG-0587、DG-0898
- expected Wiki：无
- selected Wiki：hygiene-storage-leak-odor-isolation-001、general-storage-suspect-batch-quarantine-001、general-storage-expired-item-hold-card-001、hygiene-storage-contaminated-container-hold-001、medical-storage-expired-unknown-medicine-hold-001、energy-storage-battery-leak-suspect-bin-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### obs_storage_shelf_stronger

- query：仓库货架怎么做更结实？
- 类型：observation
- focus：Manufacturing 可以主导制作；观察 Storage 是否补充承重、分区和堆叠安全需求。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0899、DG-0059、DG-0079
- expected Wiki：无
- selected Wiki：repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001、repair-storage-fabric-rope-dry-check-001、repair-storage-wood-metal-material-stack-001、repair-storage-shelf-overload-warning-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### obs_storage_battery_leak_skin

- query：电池漏液碰到手怎么办？
- 类型：observation
- focus：Medical/Contamination 可以主导人体接触；观察 Storage 是否补充隔离区和标签。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0112、DG-0841、DG-0064
- expected Wiki：无
- selected Wiki：medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001、hygiene-isolation-supplies-001、energy-battery-parallel-series-boundary-001、energy-battery-chemistry-mix-stop-001、energy-battery-leak-corrosion-isolation-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、storage_vs_medical、storage_vs_waste、storage_vs_energy
- root cause：无
- failure reasons：无

### obs_storage_unknown_residue_container

- query：旧容器有不明残留，还能不能继续用来储藏？
- 类型：observation
- focus：Waste/Recycling 可以主导再利用判断；观察 Storage 是否补充储藏区隔离和不可混入正常库存。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0893、DG-0894、DG-0899
- expected Wiki：无
- selected Wiki：repair-recycling-plastic-container-intake-check-001、hygiene-waste-contaminated-container-downgrade-001、repair-recycling-cleanable-noncleanable-material-001、repair-recycling-material-downgrade-label-001、general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001、general-waste-source-hazard-log-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、storage_vs_manufacturing、storage_vs_waste、storage_vs_hygiene
- root cause：无
- failure reasons：无

## 12. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_long_term_storage_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_long_term_storage_field.py --no-answer
```

边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。
