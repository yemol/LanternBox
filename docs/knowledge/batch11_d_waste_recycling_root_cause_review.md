# LanternBox Batch11-D: Waste / Recycling Retrieval Root Cause Review

生成日期：2026-07-18

本阶段只生成分析报告。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、测试或 PocketBase。

参考：

- `docs/knowledge/batch11_a_waste_recycling_plan.md`
- `docs/knowledge/batch11_b_waste_recycling_apply_report.md`
- `docs/knowledge/batch11_c_waste_recycling_field_test_report.md`
- `docs/knowledge/batch11_c_waste_recycling_field_test_results.json`

## 1. Field Test 总结

Batch11-C 当前结果：

|指标|结果|
|---|---:|
|total|24|
|strict / observation|18 / 6|
|pass / partial / fail|16 / 1 / 7|
|strict pass / partial / fail|10 / 1 / 7|
|Guide hit|83.3%|
|主 Guide hit|72.2%|
|Wiki full hit|61.1%|
|Guide-Wiki precise|72.2%|
|Waste / Recycling 主 Guide 进入率|58.3%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|

总体判断：

|问题|判断|
|---|---|
|fail 是否来自知识缺失|总体不是。7 个 fail 中，多数正确 Guide/Wiki 已存在；失败来自 selected Guide 排序和 selected Wiki top8 截断。|
|fail 是否来自 Guide-Wiki 缺链|不是。Batch11-B audit 已确认 Guide-Wiki 双向关系为 0 问题。|
|fail 是否来自 Waste profile 缺失|是主要原因。当前没有 Waste/Recycling profile，碎玻璃、冷灰、塑料桶、交接等 query 被旧稳定域吸走。|
|fail 是否来自相邻领域抢主位|是。Fire、Agriculture、Energy/Medical、Manufacturing/Repair、Records 均有抢主位。|
|fail 是否来自 selected priority/top8 截断|是。正确 Guide 位于第 3/4 位时，其 related_wiki 常被前面 Guide 的 related_wiki 挤出 top8。|
|fail 是否来自 fixture expected 过窄|少量存在。热灰刚灭场景 Fire 主导合理，但 fixture 仍将非 Waste primary 判 fail。|
|是否有 dangerous suggestion / Kiwix 越权|没有。安全边界、fallback、record/check 均为 100%。|

结论：Batch11-D 不应把问题归因为安全失败或内容不可用。根因集中在 Waste / Recycling 缺少检索入口保护，以及测试中部分跨域主导边界需要更细。

## 2. Fail + Partial 根因

### 2.1 根因总表

|case|verdict|root cause 分类|最小修复建议|
|---|---|---|---|
|`waste_broken_glass_collect_mark`|fail|A/B/C/I|新增 `waste_sharp_broken_material` profile；保护 DG-0888 和锋利物 Wiki。|
|`waste_battery_leak_common_trash`|fail|A/C/G/J|新增 `waste_contaminated_trash` profile；电池漏液“垃圾/普通垃圾”语义下 Waste 进入 selected。|
|`waste_rotten_kitchen_scrap_leachate_compost`|fail|A/C/F|新增 `waste_kitchen_scrap_boundary` profile；腐败/渗液厨余先走 Waste 分流，Agriculture secondary。|
|`waste_hot_ash_direct_trash_bin`|fail|E/K|Fire primary 合理，但 Waste Wiki 已进入；建议 fixture / 判定允许 DG-0851 primary + DG-0891 selected。|
|`waste_cold_ash_store_later_use`|fail|A/B/C/E/F|新增 `waste_ash_char_boundary` profile；冷灰保存和分流应把 DG-0891 推入 selected。|
|`waste_plastic_bucket_odor_reuse`|fail|A/B/C/H|新增 `waste_container_reuse` profile；旧塑料桶异味是容器再利用边界，不是 Manufacturing 制作。|
|`waste_handover_minimum_fields`|fail|A/B/C|新增 `waste_record_handover` profile；避免 communication / inventory records 抢主位。|
|`waste_material_pool_ledger`|partial|C/D|新增 `waste_record_handover` profile 或调整 DG-0894 主入口；当前 DG-0894 selected 但 Wiki 被 DG-0892 related_wiki 截断。|

分类说明：

- A = waste profile 缺失
- B = correct Guide exists but not selected
- C = correct Wiki exists but not selected
- D = Guide-Wiki related_wiki 顺序问题
- E = Fire 抢主位
- F = Agriculture 抢主位
- G = WASH / Hygiene / Contamination 抢主位
- H = Manufacturing / Repair 抢主位
- I = Medical / Safety 抢主位
- J = Energy / Battery 抢主位
- K = fixture expected 过窄
- L = 确实缺 Wiki / Guide
- M = 合理 observation，不修

### 2.2 逐条复盘

|case|query|expected|actual selected|是否命中 Waste|cross domain|根因|最小建议|
|---|---|---|---|---|---|---|---|
|`waste_broken_glass_collect_mark`|碎玻璃散在地上，怎么收集和标记才安全？|DG-0888；`hygiene-waste-sharp-glass-temporary-container-001`|DG-0632 / DG-0552 / DG-0801；Wiki 全为 fire/safety/water|Guide 未命中；Wiki 未命中|Safety / Water|DG-0888 在 guide candidates 第 4 位，正确 Guide 存在但未入 selected；独立 wiki candidates 为空；不是知识缺失。|新增 `waste_sharp_broken_material` profile；Medical/Safety 仅在已割伤时 secondary。|
|`waste_battery_leak_common_trash`|电池漏液的垃圾能不能和普通垃圾放一起？|DG-0889 或 DG-0887；`hygiene-waste-battery-leakage-boundary-001`|DG-0112 / DG-0841 / DG-0064；Wiki 为 medical / contamination / energy|允许 Guide 命中 DG-0112；Waste Wiki 未命中|Medical / Energy|人体接触和电池安全旧域压过“垃圾能否混放”的 Waste 语义。Waste Guide 未进候选前 8。|新增 `waste_contaminated_trash` profile，触发词含“漏液的垃圾/普通垃圾/混放/废电池”。|
|`waste_rotten_kitchen_scrap_leachate_compost`|厨余已经发臭渗液，还能不能进堆肥？|DG-0890；`hygiene-waste-food-rot-wet-isolation-001`|DG-0882 / DG-0883 / DG-0890；Wiki 全为 Agriculture|Guide 命中；Waste Wiki 未命中|Agriculture|DG-0890 已 selected 第 3，但前两个 Agriculture Guide 的 related_wiki 占满 top8；正确 Wiki 在 DG-0890 related_wiki 中但被截断。|新增 `waste_kitchen_scrap_boundary` profile，把“发臭/渗液/能否进堆肥”先导向 DG-0890。|
|`waste_hot_ash_direct_trash_bin`|火堆刚灭的热灰能不能直接倒进垃圾桶？|DG-0891；`fire-waste-hot-ash-not-trash-001`|DG-0851 / DG-0830 / DG-0891；expected Wiki 已进入|Guide/Wiki 均进入，但 primary 不是 Waste|Fire / Hygiene|“刚灭热灰”由 Fire 判断余火和复燃是合理的；同时 Waste 分流证据已进入。当前 fail 更多来自 strict 判定对 primary domain 过硬。|Batch11-E 可选择只修 fixture expected：允许 DG-0851 primary，要求 DG-0891 / `fire-waste-hot-ash-not-trash-001` 进入 evidence。|
|`waste_cold_ash_store_later_use`|冷灰能不能保存起来以后再用？|DG-0891；`fire-waste-cold-ash-storage-boundary-001`|DG-0851 / DG-0830 / DG-0879；DG-0891 candidate 第 4|Guide 未 selected；Wiki 未命中|Fire / Agriculture|冷灰保存属于 Waste 分流和记录，但旧 Fire/Agriculture 灰烬条目抢主位；DG-0891 未入 selected top3。|新增 `waste_ash_char_boundary` profile，区分热灰仍 Fire 主导、冷灰保存/分流由 Waste 主导。|
|`waste_plastic_bucket_odor_reuse`|旧塑料桶有异味，还能不能拿来装东西？|DG-0893；`repair-recycling-plastic-container-intake-check-001`|DG-0872 / DG-0876 / DG-0877；DG-0893 candidate 第 4|Guide 未 selected；Wiki 未命中|Manufacturing / Repair|旧 Manufacturing 材料再利用 Guide 抢走“容器用途降级/残留异味”入口；正确 Guide 存在但未 selected。|新增 `waste_container_reuse` profile；容器异味/残留/装东西触发 DG-0893。|
|`waste_material_pool_ledger`|可用材料池怎么登记，避免以后没人知道能不能用？|DG-0894；`general-recycling-material-pool-ledger-001`|DG-0892 / DG-0894 / DG-0887；Wiki 被 DG-0892 material intake 占满|Guide 命中；Wiki 未命中|无严重跨域|DG-0894 selected 第 2，但 selected_wiki top8 全来自 DG-0892 related_wiki，DG-0894 的 ledger Wiki 被截断。|新增 `waste_record_handover` profile，促使 DG-0894 primary；或小范围调整 DG-0892/DG-0894 证据优先。|
|`waste_handover_minimum_fields`|废弃物交接时要告诉下一班什么？|DG-0894；`general-waste-disposal-handover-card-001`|DG-0860 / DG-0127 / DG-0212；Wiki 为 communication/inventory/medical records|Guide 未命中；Wiki 未命中|Records / Communication|“交接/下一班”被通信、库存和医疗记录旧 Guide 抢走；Waste 记录入口没有 profile 保护。|新增 `waste_record_handover` profile；触发废弃物/材料池/垃圾点 + 交接/下一班/最小字段。|

## 3. 新增 8 个 Waste Guide 稳定性审查

|Guide|Field 命中|Top1 命中|Wiki evidence|稳定性|问题|
|---|---:|---:|---|---|---|
|DG-0887 废弃物基础分类与临时隔离|5|3|核心分类 Wiki 稳定进入|较稳|混合垃圾、污染桶、不明废弃物表现好；电池漏液垃圾未能由 DG-0887 承接。|
|DG-0888 锋利 / 破碎 / 金属边角废物处理|2|1|金属边角进入稳定，碎玻璃弱|不稳|碎玻璃 query 中 DG-0888 仅 guide candidate 第 4，被 security/water 抢走。|
|DG-0889 病人垃圾与污染物分流|4|2|病人垃圾 Wiki 稳定进入|较稳|病人纸巾、病人剩饭表现好；电池漏液物和不明污染物场景更依赖 profile。|
|DG-0890 厨余和有机物进入堆肥前判断|3|2|病人剩饭和一般厨余较稳；腐败渗液弱|中等|腐败/渗液 query 中 DG-0890 进第 3，但 related_wiki 被 Agriculture top Guide 截断。|
|DG-0891 灰烬与炭渣冷却后分流|1|0|热灰 case Wiki 进入；冷灰 case 未进入|弱|Fire/Agriculture 灰烬旧入口强。热灰 Fire primary 合理，但冷灰保存应由 Waste 进入 selected。|
|DG-0892 可再利用材料进入材料池前检查|4|4|废木板/金属片/材料池准入稳定|稳定|材料池登记 query 中 DG-0892 过强，挤掉 DG-0894 ledger Wiki。|
|DG-0893 塑料桶 / 容器再利用前判断|1|1|不明残留容器稳定；塑料桶异味弱|不稳|旧 Manufacturing 材料再利用抢塑料桶异味主位。|
|DG-0894 废弃物与材料池记录交接|4|1|回收失败稳定；ledger/交接弱|不稳|“交接/下一班/登记”被 general records / communication / DG-0892 抢。|

判断：8 个 Guide 内容和 evidence chain 没有结构性缺陷。最弱的是 DG-0891、DG-0893、DG-0894；问题主要是 query profile 缺失和 selected Guide 排序，不是 Guide 正文或 related_wiki 缺链。

## 4. 新增 36 篇 Waste Wiki Evidence 审查

|Wiki|关联 Guide|是否进入 evidence|未进入原因|建议|
|---|---|---|---|---|
|`hygiene-waste-basic-sorting-isolation-001`|DG-0887|进入 3 次|稳定|保持。|
|`hygiene-waste-mixed-trash-stop-line-001`|DG-0887|进入 3 次|稳定|保持。|
|`general-waste-source-label-minimum-001`|DG-0887, DG-0894|进入 4 次|稳定|保持。|
|`hygiene-waste-child-access-control-001`|DG-0887, DG-0888|进入 4 次|稳定|保持。|
|`hygiene-waste-sharp-glass-temporary-container-001`|DG-0888|进入 1 次，但未进碎玻璃 strict|DG-0888 未入 selected；Safety/Water 抢主位|profile 保护 DG-0888。|
|`hygiene-waste-metal-edge-scrap-isolation-001`|DG-0888, DG-0892|进入 5 次|稳定|保持。|
|`hygiene-waste-battery-leakage-boundary-001`|DG-0887, DG-0889|进入 5 次，但未进电池漏液 strict|电池 query 被 Medical/Energy Guide 接管|profile 保护废弃物语义。|
|`hygiene-waste-unknown-chemical-item-hold-001`|DG-0887, DG-0889|进入 5 次|稳定|保持。|
|`hygiene-waste-patient-trash-double-bag-zone-001`|DG-0889|进入 2 次|稳定|保持。|
|`hygiene-waste-food-rot-wet-isolation-001`|DG-0887, DG-0890|进入 5 次，但未进腐败厨余 strict|DG-0890 第 3，Agriculture related_wiki 截断|profile 推 DG-0890 primary。|
|`fire-waste-hot-ash-not-trash-001`|DG-0891|进入 1 次|热灰 case 已命中|保持；fixture 判定需更细。|
|`hygiene-waste-contaminated-container-downgrade-001`|DG-0889, DG-0893|进入 3 次|稳定|保持。|
|`repair-recycling-material-pool-zone-layout-001`|DG-0892|进入 4 次|稳定|保持。|
|`repair-recycling-material-intake-checklist-001`|DG-0892|进入 4 次|稳定|保持。|
|`repair-recycling-salvaged-wood-intake-check-001`|DG-0892|进入 4 次|稳定|保持。|
|`repair-recycling-metal-sheet-intake-check-001`|DG-0892, DG-0888|进入 5 次|稳定|保持。|
|`repair-recycling-plastic-container-intake-check-001`|DG-0893|进入 1 次，但未进塑料桶异味 strict|DG-0893 第 4，Manufacturing 抢主位|profile 保护容器再利用。|
|`repair-recycling-fabric-rope-intake-check-001`|DG-0892|进入 4 次|稳定但未被 strict expected 覆盖|后续 observation 可补。|
|`repair-recycling-fasteners-small-parts-sort-001`|DG-0892, DG-0894|进入 5 次|稳定|保持。|
|`repair-recycling-cleanable-noncleanable-material-001`|DG-0892, DG-0893|进入 5 次|稳定|保持。|
|`repair-recycling-material-downgrade-label-001`|DG-0892, DG-0893, DG-0894|进入 2 次|中等|保持。|
|`agriculture-waste-kitchen-scrap-before-compost-001`|DG-0890|进入 2 次|一般厨余/病人剩饭可进入；腐败渗液被 Agriculture 截断|profile。|
|`hygiene-waste-patient-leftover-no-compost-001`|DG-0889, DG-0890|进入 4 次|稳定|保持。|
|`hygiene-waste-oil-meat-odor-organic-boundary-001`|DG-0890|进入 2 次|中等|保持。|
|`agriculture-waste-compost-waiting-bin-distance-001`|DG-0890|进入 2 次|中等|保持。|
|`hygiene-waste-organic-pest-odor-daily-check-001`|DG-0890, DG-0894|进入 3 次|中等|保持。|
|`fire-waste-cold-ash-storage-boundary-001`|DG-0891|进入 1 次，但未进冷灰 strict|DG-0891 未 selected；Fire/Agriculture 抢主位|profile 保护冷灰保存。|
|`fire-waste-charcoal-residue-reuse-check-001`|DG-0891|进入 1 次|热灰 case 作为补充进入|保持。|
|`agriculture-waste-ash-soil-use-interface-001`|DG-0891|未进入|缺少灰烬入土/转交类 query|后续 Field Test 可覆盖，不新增。|
|`hygiene-waste-ash-trash-mixing-ban-001`|DG-0891|未进入|DG-0891 不稳定；且热灰 case top8 已含前 3 条|可在 DG-0891 顺序中评估是否提前。|
|`hygiene-waste-temporary-overflow-plan-001`|DG-0887|进入 3 次|稳定|保持。|
|`hygiene-waste-recycling-batch-quarantine-001`|DG-0892|未进入|未出现可疑批次 query；DG-0892 top8 前面材料池基础 Wiki 优先|后续 observation，可不修。|
|`general-recycling-material-pool-ledger-001`|DG-0894|进入 1 次，但未进 ledger strict|DG-0892 primary 挤占 top8；DG-0894 第 2|profile 推 DG-0894 primary。|
|`general-waste-source-hazard-log-001`|DG-0894|进入 1 次|记录链补充进入|保持。|
|`general-waste-disposal-handover-card-001`|DG-0894|进入 1 次，但未进 handover strict|DG-0894 未进 candidates；records/communication 抢主位|profile 保护废弃物交接。|
|`general-waste-reuse-failure-record-001`|DG-0894, DG-0892|进入 1 次|回收失败 case 稳定|保持。|

判断：36 篇 Wiki 均有有效 evidence chain。没有发现必须新增 Wiki 的主题。未进入 evidence 的 Wiki 多数是没有对应 query 或正确 Guide 未 selected，不是内容缺失。

## 5. Cross Domain 根因

### Fire

抢主位场景：

- `waste_hot_ash_direct_trash_bin`
- `waste_cold_ash_store_later_use`
- observation `waste_observe_hot_ash_rekindle`

边界判断：

- Fire 应主导：火源是否熄灭、余火复燃、热灰、室内燃烧、一氧化碳和火灾风险。
- Waste 应主导：冷却后的分流、是否进入普通垃圾、冷灰保存、灰烬作为废弃物/材料的记录。

结论：热灰刚灭场景 Fire primary 合理；冷灰保存和垃圾桶分流需要 Waste profile 保护。

### Agriculture

抢主位场景：

- `waste_rotten_kitchen_scrap_leachate_compost`
- 灰烬保存场景中 DG-0830 草木灰来源审查也很强。

边界判断：

- Agriculture 应主导：堆肥成熟、土壤使用、食用地块边界、灰烬入土。
- Waste 应主导：厨余进入堆肥前分流、病人剩饭、腐败/渗液、油脂肉类、堆肥等待区。

结论：腐败厨余 query 同时含“堆肥”，导致 Agriculture 强主导；需要 `waste_kitchen_scrap_boundary` 将“发臭/渗液/病人剩饭/进入堆肥前”拉回 Waste。

### WASH / Hygiene / Contamination

表现：

- 病人纸巾、病人垃圾已能由 DG-0889 稳定主导。
- 电池漏液和不明污染物人体接触类 query 会由 contamination / medical 主导，这是合理。
- “漏液垃圾能不能混入普通垃圾”仍缺 Waste 主入口保护。

边界判断：

- WASH / Hygiene 应主导：人体卫生、病人隔离、洗手和接触后清洁、排泄物卫生处理。
- Contamination 应主导：人体接触污染、污染区清理、未知化学暴露。
- Waste 应主导：污染垃圾临时桶、污染垃圾标记、远离食物/饮水/睡眠区、废弃物来源记录。

结论：病人垃圾链较稳，电池漏液和未知污染“废弃物混放”需要 profile。

### Manufacturing / Repair

抢主位场景：

- `waste_plastic_bucket_odor_reuse`
- 部分材料池场景会由 DG-0892 与 Manufacturing v0.1 同域竞争。

边界判断：

- Manufacturing 应主导：材料加工、制作结构、成品质量检查。
- Repair 应主导：工具维修、损坏物修复。
- Waste 应主导：是否进入材料池、再利用前检查、污染/腐烂/残留停止线、可用材料等级记录。

结论：废木板和金属片准入已经稳定；塑料桶异味/容器再利用需要 `waste_container_reuse` profile。

### Medical / Safety

抢主位场景：

- `waste_broken_glass_collect_mark`
- observation `waste_observe_glass_cut_hand`

边界判断：

- Medical 应主导：已经割伤、扎伤、出血、感染风险。
- Safety 应主导：危险区域人员隔离、门窗破损等空间安全。
- Waste 应主导：未受伤前的碎片收集、硬容器、标记、剩余碎片处理。

结论：碎玻璃“未受伤、收集标记”必须由 DG-0888 进入 selected；当前 DG-0888 仅 candidate 第 4，profile 必要。

### Energy / Battery

抢主位场景：

- `waste_battery_leak_common_trash`
- observation `waste_observe_battery_leak_skin`

边界判断：

- Energy 应主导：电源系统安全、电池使用、充放电、储能风险。
- Medical / Contamination 应主导：皮肤/眼睛接触、人体暴露。
- Waste 应主导：废弃电池漏液物临时隔离、不与普通垃圾混放、标记和远离生活区。

结论：人体接触 query 不修；“漏液垃圾能不能混普通垃圾”需要 Waste contaminated trash profile。

## 6. 是否需要 Profile

|profile|是否必要|覆盖 triggers|可能误伤|Batch11-E 建议|
|---|---|---|---|---|
|`waste_basic_sorting`|可选|混合垃圾、临时隔离、垃圾分类、普通/污染/湿/尖锐分流、垃圾桶位置|可能抢 WASH 清洁分区|可暂缓；DG-0887 当前较稳。|
|`waste_sharp_broken_material`|必要|碎玻璃、玻璃片、锋利物、钉子、金属边角、散落、收集、标记、硬容器、避免扎伤|已受伤时不应抢 Medical|Batch11-E 实现，primary DG-0888，secondary Medical/Safety。|
|`waste_contaminated_trash`|必要|病人垃圾、污染垃圾、电池漏液垃圾、不明刺鼻废弃物、不能混普通垃圾、污染桶、标记、远离饮水/食物|人体暴露应让 Contamination/Medical 主导|Batch11-E 实现，primary DG-0889/DG-0887，secondary contamination/medical/energy。|
|`waste_kitchen_scrap_boundary`|必要|厨余、腐败、发臭、渗液、病人剩饭、油脂肉类、能否进堆肥、堆肥前、等待区|堆肥成熟/入土不应抢 Agriculture|Batch11-E 实现，primary DG-0890，secondary DG-0882/Agriculture。|
|`waste_ash_char_boundary`|必要|热灰倒垃圾桶、冷灰保存、炭渣、灰烬混入垃圾、冷却后分流、灰烬记录|刚灭/复燃应让 Fire 主导|Batch11-E 实现，但区分热灰 Fire primary 可接受；冷灰/垃圾桶分流推 DG-0891。|
|`waste_material_pool_reuse`|可选|材料池、废木板、旧金属片、入池、再利用前检查、材料等级|制作/加工不应抢 Manufacturing|可暂缓或轻量实现；DG-0892 已较稳。|
|`waste_container_reuse`|必要|旧塑料桶、旧容器、异味、不明残留、装东西、装水、用途降级、清一清还能不能用|储水容器清洁和食品接触边界需 WASH/Food secondary|Batch11-E 实现，primary DG-0893，secondary WASH/Food/Manufacturing。|
|`waste_record_handover`|必要|废弃物交接、材料池台账、下一班、最小字段、来源记录、回收失败、没人知道能不能用|通信记录/库存记录不应完全被压制，只做 secondary|Batch11-E 实现，primary DG-0894，secondary records/organization。|

建议：Batch11-E 新增 6 个必要 profile，暂缓 `waste_basic_sorting` 和 `waste_material_pool_reuse`，除非希望把分类/材料池也完全冻结到 stable。

## 7. 是否需要 Guide-Wiki 顺序调整

|Guide|当前判断|是否调整|建议|
|---|---|---|---|
|DG-0887|分类入口稳定；battery/unknown 在 related_wiki 第 7/8，但仍在 top8|暂不必须|profile 优先；不急于调顺序。|
|DG-0888|锋利物 Wiki 已排第 1/2；问题是 Guide 未 selected|不需要|profile 解决。|
|DG-0889|病人垃圾稳定；电池漏液 Wiki 第 4|可选|如实现 `waste_contaminated_trash`，可考虑把 `hygiene-waste-battery-leakage-boundary-001` 放到未知污染之后或前面，但不是主因。|
|DG-0890|腐败厨余 Wiki 第 6；当 DG-0890 第 3 时被截断|可小调但非主因|profile 推 DG-0890 primary 更有效；可将 `hygiene-waste-food-rot-wet-isolation-001` 提到前 2。|
|DG-0891|热灰/冷灰/炭渣顺序合理|不必大调|可把 `hygiene-waste-ash-trash-mixing-ban-001` 提前到第 3，以增强垃圾混放 query。|
|DG-0892|材料池准入过强，ledger query 会用它填满 top8|不建议削弱|用 `waste_record_handover` profile 推 DG-0894，不要破坏 DG-0892 稳定性。|
|DG-0893|塑料桶和污染容器顺序合理|不需要|profile 解决未 selected。|
|DG-0894|ledger/交接/失败记录顺序合理|不需要|profile 解决未 selected；material pool ledger partial 也会改善。|

结论：Guide-Wiki 顺序不是主要根因。Batch11-E 可做极小顺序调整，但核心应是 profile。不要大批量重排 related_wiki。

## 8. 是否需要新增 Wiki / Guide

逐项判断：

|主题|已有支撑|是否新增|
|---|---|---|
|热灰 / 冷灰 / 炭渣|DG-0891；`fire-waste-hot-ash-not-trash-001`、`fire-waste-cold-ash-storage-boundary-001`、`fire-waste-charcoal-residue-reuse-check-001`|不新增|
|厨余 / 病人剩饭 / 堆肥前分流|DG-0890 / DG-0889；厨余、病人剩饭、油脂异味、等待区 Wiki 已有|不新增|
|病人垃圾 / 污染物临时隔离|DG-0889 / DG-0887；病人垃圾、污染容器、不明污染物 Wiki 已有|不新增|
|废木板 / 金属片 / 塑料桶 / 容器再利用|DG-0892 / DG-0893；材料池和容器 Wiki 已有|不新增|
|电池漏液垃圾|DG-0887 / DG-0889；`hygiene-waste-battery-leakage-boundary-001` 已有|不新增|
|碎玻璃 / 锋利物|DG-0888；`hygiene-waste-sharp-glass-temporary-container-001` 已有|不新增|
|材料池台账 / 交接记录|DG-0894；ledger、source log、handover、failure record Wiki 已有|不新增|

结论：不需要新增 Wiki / Guide。现有知识足以支撑行动入口；问题是检索入口稳定性。

## 9. Batch11-E 最小 Apply 建议

建议选择：**C. profile + 少量 Guide-Wiki 顺序都需要**，其中 profile 是主修复，顺序调整只做可选小补。

### 9.1 建议新增 profile

优先实现：

1. `waste_sharp_broken_material`
   - primary：DG-0888
   - secondary：medical, safety
   - 覆盖：碎玻璃、玻璃片、锋利物、钉子、金属边角、散落、收集、标记、硬容器、避免扎伤。

2. `waste_contaminated_trash`
   - primary：DG-0889 / DG-0887
   - secondary：contamination, medical, energy
   - 覆盖：污染垃圾、病人垃圾、电池漏液垃圾、不明刺鼻废弃物、普通垃圾混放、临时桶、标记、远离饮水食物。

3. `waste_kitchen_scrap_boundary`
   - primary：DG-0890
   - secondary：DG-0882, agriculture, hygiene
   - 覆盖：厨余、腐败、发臭、渗液、病人剩饭、油脂肉类、能否进堆肥、堆肥前分流。

4. `waste_ash_char_boundary`
   - primary：DG-0891
   - secondary：DG-0851, fire, agriculture
   - 覆盖：热灰倒垃圾桶、冷灰保存、炭渣、灰烬混入普通垃圾、冷却后分流、灰烬记录。
   - 注意：刚灭、复燃、冒烟、红热仍允许 Fire primary。

5. `waste_container_reuse`
   - primary：DG-0893
   - secondary：DG-0892, hygiene, food, manufacturing
   - 覆盖：旧塑料桶、旧容器、异味、不明残留、装东西、装水、用途降级、清洁后是否可用。

6. `waste_record_handover`
   - primary：DG-0894
   - secondary：records, organization
   - 覆盖：废弃物交接、材料池台账、下一班、最小字段、来源记录、回收失败、用途限制。

可暂缓：

- `waste_basic_sorting`：DG-0887 已较稳。
- `waste_material_pool_reuse`：DG-0892 已较稳；如后续 Field Test 仍偏移再加。

### 9.2 可选 Guide-Wiki 小调整

仅在 Batch11-E 需要时考虑：

- DG-0890：将 `hygiene-waste-food-rot-wet-isolation-001` 提到前 2，增强腐败厨余/渗液 query。
- DG-0891：将 `hygiene-waste-ash-trash-mixing-ban-001` 提到第 3，增强“倒垃圾桶/混普通垃圾”语义。

不建议调整：

- DG-0892 的材料池准入顺序。它当前稳定，不应为 ledger case 牺牲材料池主链。
- DG-0894 顺序。当前顺序合理，问题是未被 selected。

### 9.3 Fixture 建议

建议在 Batch11-E 或 Final Verification 前审查：

- `waste_hot_ash_direct_trash_bin`：如果 query 含“刚灭/热灰/复燃”，允许 DG-0851 Fire primary；但必须要求 DG-0891 或 `fire-waste-hot-ash-not-trash-001` 进入 evidence。
- Observation cases 不修；只保留跨域观察。

## 10. 不建议修改项

Batch11-E 不建议：

- 不新增大量 Wiki。
- 不新增 Guide。
- 不改 Retrieval Pipeline。
- 不改 Prompt。
- 不改 top_k。
- 不改 selector limit。
- 不改 ranking。
- 不改 fallback。
- 不硬编码 case。
- 不让 Waste 吞掉 Fire / Agriculture / WASH / Manufacturing / Medical / Energy。
- 不让相邻领域完全压过 Waste 的分类 / 隔离 / 材料池 / 交接记录链。

下一阶段建议进入：Batch11-E Waste / Recycling Retrieval Minimal Apply。
