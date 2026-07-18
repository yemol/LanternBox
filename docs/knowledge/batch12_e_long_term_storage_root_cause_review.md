# LanternBox Batch12-E: Long-Term Storage Retrieval Root Cause Review

生成日期：2026-07-18

本阶段只生成分析报告。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`；未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase、测试或 `.env`。

## 1. Field Test 总结

- strict / observation：20 / 6
- pass / partial / fail：16 / 1 / 9
- strict pass / partial / fail：10 / 1 / 9
- Guide hit：80.0%
- Wiki full hit：70.0%
- Guide-Wiki precise：80.0%
- Long-Term Storage primary Guide rate：50.0%
- safety / fallback / record-check：100.0% / 100.0% / 100.0%
- dangerous suggestion：0
- Kiwix 越权：0

总体判断：本轮失败不是安全边界失败，也不是 Kiwix 越权。新增 Storage Guide/Wiki 已经存在，且多数 fail 中正确 Guide 或正确 Wiki 已经进入候选或 selected evidence；主要问题是没有 Long-Term Storage 专属 query profile，导致旧 Food / Agriculture / Medical / Energy / Records / Hygiene 入口在 selected top3/top8 中抢主位。少数 case 还暴露 selected evidence priority 和 top8 截断问题。

|判断项|结论|说明|
|---|---|---|
|fail 是否来自知识缺失|基本否|40 篇 Wiki 和 8 个 Guide 已覆盖本轮 strict 主题；所谓“数据缺口”多为正确 Wiki 未进 candidates/selected。|
|fail 是否来自 Guide-Wiki 缺链|否|Guide audit 0/0/0，Field Test 中 Guide-Wiki precise 80.0%。|
|是否来自 Storage profile 缺失|是，主因|多项 storage 语义触发既有 agriculture/medical/energy/inventory profile，Storage Guide 没有保护入口。|
|是否来自相邻领域抢主位|是，主因|13 个 case 有 off_domain_primary，其中 strict fail 9 个。|
|是否来自 selected priority/top8 截断|是，次因|DG-0898/DG-0900/DG-0901 常在 selected #2/#3，related_wiki 被前序 Guide 截断。|
|是否来自 fixture expected 过窄|局部是|Batch12-D 原文使用规划期 `storage-*` 占位 slug；fixture 已改为实际有效 slug。个别 case 可允许 Storage Guide 在 secondary，但 stable 目标仍应争取 top1。|
|dangerous/Kiwix|否|dangerous suggestion=0，Kiwix 越权=0。|

## 2. 9 个 fail + 1 个 partial 根因

|case|query|expected|actual selected|Storage 命中|cross domain|root cause|最小修复建议|
|---|---|---|---|---|---|---|---|
|`storage_moldy_dry_food_batch_hold`|干粮有霉味，但还没准备吃，应该怎么处理这批？|Guide DG-0896、DG-0901; Wiki food-storage-grain-moisture-clump-check-001|Guide DG-0628、DG-0606、DG-0896; Wiki hygiene-hand-hygiene-001、hygiene-handwashing-001、hygiene-handwashing-002、hygiene-hygiene-knowledge-002...|Guide DG-0896; Wiki 无|off_domain_primary、storage_vs_waste、storage_vs_hygiene|A/C/H: DG-0896 已在 selected #3，但 top1 被 DG-0628 霉味衣物、top2 被 DG-0606 干粮三天配给占用；related_wiki 未进入 top8，selected Wiki 被 hygiene 泛化内容填满。|新增 storage_food_grain_rotation 或 storage_suspicious_item_isolation profile；DG-0896/DG-0901 evidence 不必新增，需保护霉味但未入口的“批次隔离/待复查”意图。|
|`storage_seed_food_separation`|留种和食用种子应该怎么分开放？|Guide DG-0897; Wiki agriculture-storage-seed-food-separation-shelf-001|Guide DG-0879、DG-0886、DG-0885; Wiki agriculture-seed-batch-viability-ledger-001、agriculture-seed-drying-recheck-001、agriculture-seed-storage-moisture-failure-001、agriculture-seed-reserve-use-line-001...|Guide 无; Wiki 无|off_domain_primary、storage_vs_agriculture|A/B/C/F: DG-0897 在 guide candidates #4，正确 Wiki 在 wiki candidates #4，但 selected top3/top8 被 Agriculture Second Stage profile 推到 DG-0879/DG-0886/DG-0885 和旧 seed Wiki。|新增 storage_seed_storage profile；可小幅把 DG-0897 中 agriculture-storage-seed-food-separation-shelf-001 靠前；不新增 Wiki。|
|`storage_seed_library_rotation_record`|种子库怎么做轮换记录？|Guide DG-0897; Wiki agriculture-storage-seed-library-rotation-card-001|Guide DG-0232、DG-0706、DG-0879; Wiki agriculture-planting-inventory-link-001、agriculture-seed-germination-test-001、agriculture-soil-fertility-basics-index-001、safety-review-001...|Guide 无; Wiki 无|off_domain_primary、storage_vs_agriculture|A/B/C/F/L: DG-0897 在 guide candidates #4，DG-0902 未进 candidates；selected 由旧 DG-0232 种子库存、DG-0706 交换记录、DG-0879 主导。正确 Wiki 未进 wiki candidates，属于 storage seed/record 意图缺入口。|新增 storage_seed_storage + storage_inventory_handover profile；必要时调整 DG-0897/DG-0902 触发词和 related_wiki 顺序，但不新增知识。|
|`storage_damaged_medicine_package`|药品包装破损了，能不能放回药箱？|Guide DG-0898、DG-0901; Wiki medical-storage-expired-unknown-medicine-hold-001|Guide DG-0214、DG-0901、DG-0477; Wiki general-storage-suspect-batch-quarantine-001、hygiene-storage-leak-odor-isolation-001、hygiene-storage-contaminated-container-hold-001、energy-storage-battery-leak-suspect-bin-001...|Guide DG-0901; Wiki general-storage-suspect-batch-quarantine-001、hygiene-storage-leak-odor-isolation-001、hygiene-storage-contaminated-container-hold-001...|off_domain_primary、storage_vs_medical|A/G/H: DG-0901 已 selected #2 且正确 Wiki 已进入 selected；fail 来自 top1 DG-0214 药品受潮处理，被 Medical 旧 Guide 抢主位。|新增 storage_medical_supply_storage 或 storage_suspicious_item_isolation profile；该 case 可在 Apply 后期望 DG-0901/DG-0898 top1。|
|`storage_dressing_bandage_dry`|敷料和绷带怎么防潮保存？|Guide DG-0898; Wiki medical-storage-dressing-bandage-dry-pack-001|Guide DG-0896、DG-0556、DG-0898; Wiki food-storage-dry-grain-zone-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001、food-storage-can-jar-monthly-check-001...|Guide DG-0896、DG-0898; Wiki food-storage-dry-grain-zone-001、food-storage-grain-moisture-clump-check-001、food-storage-rodent-bite-isolation-001...|无|A/B/C/D/G: DG-0898 selected #3，但 related_wiki 被 DG-0896 的食物储藏 related_wiki 占满 top8，正确 Wiki 未进入 selected。|新增 storage_medical_supply_storage profile；将 DG-0898 的 medical-storage-dressing-bandage-dry-pack-001 保持前列，并避免 DG-0896 对“敷料/绷带”误匹配。|
|`storage_small_parts_sorting`|小零件怎么分类，避免维修时找不到？|Guide DG-0899、DG-0902; Wiki repair-storage-small-parts-bin-label-001|Guide DG-0309、DG-0899、DG-0272; Wiki repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001、repair-storage-fabric-rope-dry-check-001...|Guide DG-0899; Wiki repair-storage-tool-dry-rust-check-001、repair-storage-sharp-tool-secure-box-001、repair-storage-small-parts-bin-label-001...|off_domain_primary、storage_vs_manufacturing、storage_vs_tools|A/I/K: DG-0899 selected #2，正确 Wiki 已进入 selected；top1 是旧 DG-0309 临时小零件盒，属于 Tools/Repair 旧入口抢主位。|新增 storage_tools_materials_storage profile；无需新增 Wiki/Guide。|
|`storage_battery_power_bank_zone`|电池和充电宝长期不用，应该怎么分区保存？|Guide DG-0900; Wiki energy-storage-battery-dry-cool-zone-001|Guide DG-0616、DG-0108、DG-0563; Wiki energy-battery-001、energy-battery-002、energy-battery-003、energy-battery-capacity-001...|Guide 无; Wiki 无|off_domain_primary、storage_vs_energy|A/B/C/J: DG-0900 在 guide candidates #4，但 selected top3 全是 Energy 管理/异常旧 Guide；正确 Wiki 不进入 selected，wiki candidates 为空。|新增 storage_energy_fuel_storage profile；DG-0900 related_wiki 首位保留 battery dry/cool 与 power bank rotation。|
|`storage_matches_ignition_dry_check`|火柴和点火材料受潮，应该怎么保存和复查？|Guide DG-0900; Wiki fire-storage-match-candle-dry-box-001|Guide DG-0225、DG-0900、DG-0899; Wiki energy-storage-battery-dry-cool-zone-001、energy-storage-battery-leak-suspect-bin-001、energy-storage-power-bank-rotation-label-001、fire-storage-match-candle-dry-box-001...|Guide DG-0900、DG-0899; Wiki energy-storage-battery-dry-cool-zone-001、energy-storage-battery-leak-suspect-bin-001、energy-storage-power-bank-rotation-label-001...|off_domain_primary|A/J: DG-0900 selected #2，正确 Wiki 已进入 selected；top1 是旧 DG-0225 引火材料管理。问题是 Fire/燃料管理主入口优先于 storage 防潮复查。|新增 storage_energy_fuel_storage profile；不改知识。|
|`storage_leak_odor_item_isolation`|有漏液和刺鼻味的物品，应该放进正常仓库吗？|Guide DG-0901; Wiki hygiene-storage-leak-odor-isolation-001|Guide DG-0112、DG-0064、DG-0901; Wiki medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001...|Guide DG-0901; Wiki general-storage-suspect-batch-quarantine-001、hygiene-storage-leak-odor-isolation-001|off_domain_primary、storage_vs_medical、storage_vs_waste、storage_vs_hygiene|A/H/G: DG-0901 selected #3，正确 Wiki 已进入 selected；medical_poisoning_chemical_exposure profile 触发，DG-0112/DG-0064 抢主位。|新增 storage_suspicious_item_isolation profile，并限定只有“物品是否进正常仓库/库存”时 Storage 主导，人体接触仍 Medical/Contamination 主导。|
|`storage_handover_minimum_fields`|储藏区交接时，下一班最少要知道什么？|Guide DG-0902; Wiki general-storage-handover-card-001|Guide DG-0860、DG-0127、DG-0212; Wiki communication-emergency-net-opening-check-001、communication-message-priority-levels-001、communication-check-in-missed-window-001、communication-radio-log-minimum-fields-001...|Guide 无; Wiki 无|off_domain_primary、storage_vs_records、storage_vs_safety|A/B/C/L: DG-0902 未进 guide candidates；inventory_consumption profile 触发，通信交接和物资消耗记录抢主位；正确 Wiki 不进 selected。|新增 storage_inventory_handover profile；可调整 fixture 词面使用“储藏区交接/库存卡/复查日/批次”。不新增 Guide/Wiki。|

根因分类对照：A=storage profile 缺失；B=correct Guide exists but not selected；C=correct Wiki exists but not selected；D=related_wiki 顺序/截断；E=Food；F=Agriculture；G=Medical；H=Waste/Hygiene/Contamination；I=Manufacturing/Repair；J=Energy/Fire；K=Tools/Repair；L=Records；M=fixture 过窄；N=确实缺 Wiki/Guide；O=合理 observation。

## 3. 新增 8 个 Storage Guide 稳定性审查

|Guide|title|selected 次数|top1 次数|稳定性|问题|建议|
|---|---|---:|---:|---|---|---|
|`DG-0895`|储藏区基础分区与标签|3|1|中等|分区/标签命中稳定，但“交接/库存记录”会被 records 或旧 Guide 抢走。|profile 保护 storage_basic_zone_labeling；related_wiki 顺序保持核心分区/标签/潮湿/虫鼠靠前。|
|`DG-0896`|干粮 / 米面豆类防潮防虫储藏|3|2|中等|干粮防潮命中稳定；霉味未入口 case 被 hygiene/mold 和配给旧 Guide 抢主位。|profile 保护 storage_food_grain_rotation；霉味/鼠咬/结块隔离相关 Wiki 应靠前。|
|`DG-0897`|种子储藏与复查|2|1|偏弱|种子受潮命中；留种/食用分架和种子库轮换被 Agriculture Second Stage 旧 profile/Guide 抢主位。|profile 保护 storage_seed_storage；种食分离和轮换卡需要进入前列。|
|`DG-0898`|医疗物资与急救包储藏复查|3|1|偏弱|急救包命中；敷料/药品包装破损时被 Medical/食物防潮相关 Guide 挤到 #3 或后面。|profile 保护 storage_medical_supply_storage；敷料/药箱/过期隔离优先。|
|`DG-0899`|工具和材料防潮防锈储藏|7|3|中等|工具材料类最稳定；小零件分类被旧 Tools/Repair 入口抢 top1。|profile 保护 storage_tools_materials_storage；小零件分类可作为 top evidence。|
|`DG-0900`|能源 / 燃料物资安全储藏|2|1|偏弱|燃料边界可命中；电池/充电宝长期保存和火柴受潮被 Energy/Fire 旧入口压过。|profile 保护 storage_energy_fuel_storage；电池 dry/cool、power bank rotation、火柴防潮优先。|
|`DG-0901`|霉变 / 漏液 / 过期 / 污染物隔离|5|3|中等|隔离类进入率不错；漏液/刺鼻味被 Medical/Contamination profile 抢 top1。|profile 保护 storage_suspicious_item_isolation；库存隔离意图优先于人体暴露。|
|`DG-0902`|储藏记录、先入先出与交接|1|1|偏弱|FIFO 命中；储藏区交接被 communication/inventory/records 抢主位，几乎无保护。|profile 保护 storage_inventory_handover；储藏交接字段优先于通信交接。|

## 4. 新增 40 篇 Storage Wiki evidence 审查

|Wiki|关联 Guide|是否进入 evidence|未进入原因|建议|
|---|---|---|---|---|
|`agriculture-storage-seed-bag-label-001`|`[DG-0897, DG-0902]`|是|进入 1 个 case：storage_seed_moisture_recheck|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`agriculture-storage-seed-box-dry-check-001`|`[DG-0897]`|是|进入 1 个 case：storage_seed_moisture_recheck|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`agriculture-storage-seed-food-separation-shelf-001`|`[DG-0897]`|是|进入 1 个 case：storage_seed_moisture_recheck|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`agriculture-storage-seed-library-rotation-card-001`|`[DG-0897, DG-0902]`|是|进入 1 个 case：storage_seed_moisture_recheck|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`agriculture-storage-seed-moisture-quarantine-001`|`[DG-0897, DG-0901]`|是|进入 1 个 case：storage_seed_moisture_recheck|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`energy-storage-battery-dry-cool-zone-001`|`[DG-0900]`|是|进入 2 个 case：storage_matches_ignition_dry_check、storage_fuel_common_goods_boundary|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`energy-storage-battery-leak-suspect-bin-001`|`[DG-0900, DG-0901]`|是|进入 6 个 case：storage_rodent_food_batch_isolation、storage_damaged_medicine_package、storage_matches_ignition_dry_check...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`energy-storage-power-bank-rotation-label-001`|`[DG-0900, DG-0902]`|是|进入 2 个 case：storage_matches_ignition_dry_check、storage_fuel_common_goods_boundary|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`fire-storage-fuel-away-from-living-zone-001`|`[DG-0900, DG-0901]`|是|进入 2 个 case：storage_matches_ignition_dry_check、storage_fuel_common_goods_boundary|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`fire-storage-match-candle-dry-box-001`|`[DG-0900]`|是|进入 2 个 case：storage_matches_ignition_dry_check、storage_fuel_common_goods_boundary|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-can-jar-monthly-check-001`|`[DG-0896]`|是|进入 2 个 case：storage_dry_grain_moisture_pest、storage_dressing_bandage_dry|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-dry-grain-zone-001`|`[DG-0896]`|是|进入 2 个 case：storage_dry_grain_moisture_pest、storage_dressing_bandage_dry|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-emergency-ration-reserve-line-001`|`[DG-0896, DG-0902]`|是|进入 3 个 case：storage_dry_grain_moisture_pest、storage_fifo_grain_rotation、storage_dressing_bandage_dry|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-grain-moisture-clump-check-001`|`[DG-0896, DG-0901]`|是|进入 6 个 case：storage_dry_grain_moisture_pest、storage_rodent_food_batch_isolation、storage_damaged_medicine_package...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-opened-package-short-use-001`|`[DG-0896, DG-0902]`|是|进入 3 个 case：storage_dry_grain_moisture_pest、storage_fifo_grain_rotation、storage_dressing_bandage_dry|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`food-storage-rodent-bite-isolation-001`|`[DG-0896, DG-0901]`|是|进入 6 个 case：storage_dry_grain_moisture_pest、storage_rodent_food_batch_isolation、storage_damaged_medicine_package...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-check-calendar-001`|`[DG-0902]`|是|进入 1 个 case：storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-expired-item-hold-card-001`|`[DG-0901, DG-0902]`|否|未被本轮 26 cases 触发；多数为 supporting Wiki 或未被 case 覆盖。|不新增内容；后续 field test 可补覆盖，或由 Guide related_wiki 保持支撑。|
|`general-storage-fifo-shelf-rule-001`|`[DG-0895, DG-0902]`|是|进入 2 个 case：storage_zone_labeling、storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-handover-card-001`|`[DG-0902]`|是|进入 1 个 case：storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-inventory-card-minimum-001`|`[DG-0902]`|是|进入 1 个 case：storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-issue-log-001`|`[DG-0902]`|是|进入 1 个 case：storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-label-minimum-fields-001`|`[DG-0895, DG-0902]`|是|进入 2 个 case：storage_zone_labeling、storage_fifo_grain_rotation|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-moisture-daily-check-001`|`[DG-0895]`|是|进入 1 个 case：storage_zone_labeling|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-pest-rodent-check-001`|`[DG-0895, DG-0896]`|是|进入 3 个 case：storage_zone_labeling、storage_dry_grain_moisture_pest、storage_dressing_bandage_dry|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-suspect-batch-quarantine-001`|`[DG-0901]`|是|进入 5 个 case：storage_rodent_food_batch_isolation、storage_damaged_medicine_package、storage_leak_odor_item_isolation...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`general-storage-zone-basic-layout-001`|`[DG-0895]`|是|进入 1 个 case：storage_zone_labeling|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`hygiene-storage-contaminated-container-hold-001`|`[DG-0901]`|是|进入 4 个 case：storage_rodent_food_batch_isolation、storage_damaged_medicine_package、storage_suspicious_batch_review_label...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`hygiene-storage-leak-odor-isolation-001`|`[DG-0901]`|是|进入 5 个 case：storage_rodent_food_batch_isolation、storage_damaged_medicine_package、storage_leak_odor_item_isolation...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`medical-storage-care-supply-zone-001`|`[DG-0898]`|是|进入 1 个 case：storage_first_aid_kit_review|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`medical-storage-dressing-bandage-dry-pack-001`|`[DG-0898, DG-0901]`|是|进入 5 个 case：storage_rodent_food_batch_isolation、storage_first_aid_kit_review、storage_damaged_medicine_package...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`medical-storage-expired-unknown-medicine-hold-001`|`[DG-0898, DG-0901]`|是|进入 5 个 case：storage_rodent_food_batch_isolation、storage_first_aid_kit_review、storage_damaged_medicine_package...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`medical-storage-first-aid-kit-monthly-check-001`|`[DG-0898]`|是|进入 1 个 case：storage_first_aid_kit_review|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`medical-storage-medicine-dry-dark-box-001`|`[DG-0898]`|是|进入 1 个 case：storage_first_aid_kit_review|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-fabric-rope-dry-check-001`|`[DG-0899]`|是|进入 4 个 case：storage_tool_rust_prevention、storage_sharp_tool_safe_box、storage_small_parts_sorting...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-sharp-tool-secure-box-001`|`[DG-0899, DG-0901]`|是|进入 5 个 case：storage_tool_rust_prevention、storage_sharp_tool_safe_box、storage_small_parts_sorting...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-shelf-overload-warning-001`|`[DG-0895, DG-0899]`|是|进入 5 个 case：storage_zone_labeling、storage_tool_rust_prevention、storage_sharp_tool_safe_box...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-small-parts-bin-label-001`|`[DG-0899, DG-0902]`|是|进入 5 个 case：storage_tool_rust_prevention、storage_sharp_tool_safe_box、storage_small_parts_sorting...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-tool-dry-rust-check-001`|`[DG-0899]`|是|进入 6 个 case：storage_dry_grain_moisture_pest、storage_tool_rust_prevention、storage_sharp_tool_safe_box...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|
|`repair-storage-wood-metal-material-stack-001`|`[DG-0899]`|是|进入 4 个 case：storage_tool_rust_prevention、storage_sharp_tool_safe_box、storage_small_parts_sorting...|保持；如其为核心触发词，可在 profile 中列 object/state/action。|

## 5. Cross Domain 根因

|相邻领域|本轮表现|边界判断|Batch12-F 处理|
|---|---|---|---|
|Food|霉味粮食和干粮配给会抢主位。|“能不能吃/食物中毒/入口前”Food 主导；“还没准备吃的批次隔离、防潮、防虫、FIFO”Storage 主导。|profile 中加入未入口、储粮区、批次、隔离、复查触发。|
|Agriculture|种子受潮、留种、种子库轮换被 Agriculture profile 和 DG-0879 等压过。|播种可行性和发芽率 Agriculture 主导；种子袋/盒保存、种食分离、轮换记录 Storage 主导。|新增 storage_seed_storage，避免吞掉“能不能播”。|
|Medical|药品包装破损、敷料绷带、漏液接触容易进入 Medical。|治疗/用药 Medical 主导；药箱保存、破损隔离、急救包复查 Storage 主导。|新增 storage_medical_supply_storage；人体接触仍 Medical/Contamination。|
|Manufacturing / Repair|货架、小零件、工具防锈可被旧工具/维修入口抢 top1。|制作/修复由 Manufacturing/Repair；保存、防锈、分类、领用记录由 Storage。|新增 storage_tools_materials_storage。|
|Waste / Hygiene / Contamination|漏液、刺鼻味、污染容器常由污染/医疗入口先选中。|已判废弃和清洁处理由 Waste/WASH；是否进入正常库存、待复查标签、隔离区由 Storage。|新增 storage_suspicious_item_isolation；保持 Waste 为交接/处置补充。|
|Energy / Fire|电池、充电宝、火柴、燃料被 Energy/Fire 旧 Guide 抢主位。|使用、充放电、火源安全由相邻领域；长期不用保存、防潮、轮换、远离火源 Storage 主导。|新增 storage_energy_fuel_storage。|
|Records / Knowledge Transfer|储藏区交接被通信记录、物资消耗 profile 抢走。|通信/任务交接归原领域；库存卡、批次、复查日、领用、FIFO、储藏交接 Storage 主导。|新增 storage_inventory_handover。|

## 6. 是否需要 profile

|profile|是否必要|object/state/action triggers|可能误伤|Batch12-F 建议|
|---|---|---|---|---|
|storage_basic_zone_labeling|必要|储藏区、分区、标签、货架、可疑区、复查日、负责人；怎么分区/贴标签/编号|water 标签、general records|实现：保护 DG-0895，secondary 可为 DG-0902|
|storage_food_grain_rotation|必要|米面豆类、干粮、罐头、储粮、霉味、鼠咬、虫害、先入先出；保存/隔离/轮换|Food 入口食用判断|实现：未入口前 Storage 主导，能不能吃由 Food 主导|
|storage_seed_storage|必要|种子袋、种子盒、留种、食用种子、种子库、受潮、轮换；保存/分架/复查/记录|Agriculture 发芽率/播种计划|实现：保存和轮换 Storage 主导，播种可行性 Agriculture 主导|
|storage_medical_supply_storage|必要|急救包、敷料、绷带、药箱、药品包装破损、过期标记；保存/复查/隔离|Medical 用药/治疗判断|实现：物资保存和隔离 Storage，药品使用 Medical|
|storage_tools_materials_storage|必要|工具防锈、刃具、锋利工具、小零件、材料堆放；保存/分类/领用|Repair/Tools 使用修复|实现：储藏动作 Storage，维修动作 Repair|
|storage_energy_fuel_storage|必要|电池、充电宝、火柴、点火材料、燃料；长期不用/分区保存/防潮/远离火源|Energy 运行维护、Fire 用火安全|实现：保存和标签 Storage，使用与异常处置相邻领域|
|storage_suspicious_item_isolation|必要|漏液、刺鼻味、过期、可疑批次、污染容器；放不放正常仓库/库存|Waste/Hygiene/Medical 人体接触与废弃处理|实现：库存隔离 Storage，废弃处置 Waste|
|storage_inventory_handover|必要|储藏区交接、库存卡、领用、复查日、批次记录、先入先出|通信/团队/通用记录|实现：储藏物资字段 Storage，通信/团队交接仍保留原主域|

结论：需要新增 Storage query profile，而且应一次性覆盖 8 个小 profile，而不是扩 top_k 或改 selector limit。原因是 Storage 是横跨 Food/Agriculture/Medical/Energy/Waste/Records 的能力域，没有 profile 时会持续被成熟旧领域吸走。

## 7. 是否需要 Guide-Wiki 顺序调整

|Guide|related_wiki 是否过宽|是否需调整|建议|
|---|---|---|---|
|DG-0895|不宽|轻微|核心顺序保持：zone -> label -> moisture -> pest -> FIFO；无需新增。|
|DG-0896|不宽|是|霉味/受潮/鼠咬 case 中 `food-storage-grain-moisture-clump-check-001` 与 `food-storage-rodent-bite-isolation-001` 应靠前；避免被普通配给 Guide 压过。|
|DG-0897|不宽|是|种食分离、轮换卡在留种/种子库 queries 中应前置；当前被旧 agriculture seed Wiki 抢 evidence。|
|DG-0898|不宽|是|`medical-storage-dressing-bandage-dry-pack-001` 和 `medical-storage-expired-unknown-medicine-hold-001` 应针对敷料/破损药品优先。|
|DG-0899|不宽|轻微|小零件分类已在 related_wiki 中；主要是 profile 问题。|
|DG-0900|不宽|是|电池 dry/cool、power bank rotation、火柴防潮和燃料远离火源均应对应 profile 触发排前。|
|DG-0901|略宽但合理|轻微|跨域隔离 Guide 覆盖多类可疑物，顺序可按漏液/异味、可疑批次、医疗/电池/食物分组。|
|DG-0902|不宽|是|交接/库存 queries 应优先 `general-storage-handover-card-001`、inventory card、check calendar、issue log。|

## 8. 是否需要新增 Wiki / Guide

不建议新增 Wiki 或 Guide。逐项判断如下：

|主题|判断|
|---|---|
|储藏区分区 / 标签|已有 DG-0895；`general-storage-zone-basic-layout-001`、`general-storage-label-minimum-fields-001`。 不新增。|
|干粮 / 米面豆类 / 鼠咬 / 霉味批次隔离|已有 DG-0896/DG-0901；相关 food/general storage Wiki 已存在。 不新增。|
|种子储藏 / 受潮 / 轮换|已有 DG-0897；相关 agriculture-storage Wiki 已存在。 不新增。|
|药品 / 急救包 / 敷料绷带|已有 DG-0898/DG-0901；相关 medical-storage Wiki 已存在。 不新增。|
|工具 / 材料 / 小零件|已有 DG-0899/DG-0902；相关 repair-storage Wiki 已存在。 不新增。|
|电池 / 充电宝 / 燃料 / 火柴|已有 DG-0900；相关 energy/fire storage Wiki 已存在。 不新增。|
|漏液 / 刺鼻味 / 过期 / 可疑批次隔离|已有 DG-0901；相关 hygiene/general/energy/medical storage Wiki 已存在。 不新增。|
|储藏记录 / FIFO / 交接|已有 DG-0902；相关 general-storage Wiki 已存在。 不新增。|

## 9. Batch12-F 最小 Apply 建议

推荐选择 **C. profile + Guide-Wiki 顺序都需要**。

建议范围：

1. 新增 8 个 Storage query profile：`storage_basic_zone_labeling`、`storage_food_grain_rotation`、`storage_seed_storage`、`storage_medical_supply_storage`、`storage_tools_materials_storage`、`storage_energy_fuel_storage`、`storage_suspicious_item_isolation`、`storage_inventory_handover`。
2. 小范围调整 DG-0896、DG-0897、DG-0898、DG-0900、DG-0901、DG-0902 的 related_wiki 优先顺序；DG-0895/DG-0899 仅轻微确认。
3. 新增 contract tests，覆盖本轮 9 fail + 1 partial 的核心 query。
4. 重新运行 Long-Term Storage Field Test，目标 fail=0，dangerous=0，Kiwix=0，safety/fallback/record=100%。
5. 不新增 Wiki，不新增 Guide，不改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。

不建议把 observation case 全部修成 Storage top1：例如“粮食有霉味还能不能吃”“种子受潮还能不能播”“电池漏液碰到手怎么办”可以由 Food/Agriculture/Medical 主导，只要求 Storage evidence 作为补充进入。

## 10. 不建议修改项

- 不新增大量 Wiki 或 Guide。
- 不扩大 top_k 或 selector limit。
- 不改 Prompt。
- 不改 Retrieval Pipeline 或 ranking 主逻辑。
- 不让 Storage 吞掉 Food / Agriculture / Medical / Manufacturing / Waste / Energy / Fire 的真实主职责。
- 不把相邻领域完全压过 Storage 的保存 / 标签 / 复查 / 轮换 / 隔离 / 交接链。
- 不硬编码 field case。
- 不处理 `.env` 或无关 dirty 文件。

## 11. 结论

Long-Term Storage v0.1 的知识和 evidence chain 已建立，但 Retrieval 入口还不是 stable。当前最小根因是缺少 Storage 专属 query profiles，以及少量 selected evidence 顺序不利。Batch12-F 应做最小 Apply：新增 Storage profiles + 小范围 related_wiki 顺序调整 + contract tests + Field Test 回归。
