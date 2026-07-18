# LanternBox Batch12-C: Long-Term Storage Knowledge Apply Report

生成日期：2026-07-18

本阶段执行 Long-Term Storage v0.1 第一阶段知识与行动入口扩展。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`；未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 schema。

## 1. 新增 Wiki 清单

新增 40 篇 Wiki，存放于 `wiki_import/storage/`。由于项目当前正式 slug domain 未包含 `storage`，本批使用现有允许 domain 前缀，并以 `storage` 作为主题词保持长期储藏语义。

|slug|title|category|priority|risk_level|guide_links|
|---|---|---|---|---|---|
|`agriculture-storage-seed-bag-label-001`|种子袋批次标签|种植与食物生产|P0|caution|`[DG-0897, DG-0902]`|
|`agriculture-storage-seed-box-dry-check-001`|种子盒干燥复查|种植与食物生产|P0|high|`[DG-0897]`|
|`agriculture-storage-seed-food-separation-shelf-001`|留种和食用批次分架|种植与食物生产|P0|high|`[DG-0897]`|
|`agriculture-storage-seed-library-rotation-card-001`|小型种子库轮换卡|种植与食物生产|P1|caution|`[DG-0897, DG-0902]`|
|`agriculture-storage-seed-moisture-quarantine-001`|受潮种子待复测隔离|种植与食物生产|P0|high|`[DG-0897, DG-0901]`|
|`energy-storage-battery-dry-cool-zone-001`|电池干燥阴凉储藏区|能源|P1|high|`[DG-0900]`|
|`energy-storage-battery-leak-suspect-bin-001`|漏液疑似电池隔离盒|能源|P0|high|`[DG-0900, DG-0901]`|
|`energy-storage-power-bank-rotation-label-001`|充电宝轮换标签|能源|P1|caution|`[DG-0900, DG-0902]`|
|`fire-storage-fuel-away-from-living-zone-001`|燃料远离生活区储藏线|火源 / 保温 / 通风 / 一氧化碳风险|P1|high|`[DG-0900, DG-0901]`|
|`fire-storage-match-candle-dry-box-001`|火柴蜡烛防潮盒|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|`[DG-0900]`|
|`food-storage-can-jar-monthly-check-001`|罐头和密封食品月度复查|食物|P0|high|`[DG-0896]`|
|`food-storage-dry-grain-zone-001`|干粮和米面豆类储藏区|食物|P0|high|`[DG-0896]`|
|`food-storage-emergency-ration-reserve-line-001`|应急口粮保底线|食物|P0|high|`[DG-0896, DG-0902]`|
|`food-storage-grain-moisture-clump-check-001`|米面豆类受潮结块检查|食物|P0|high|`[DG-0896, DG-0901]`|
|`food-storage-opened-package-short-use-001`|开封食品短期使用标签|食物|P0|caution|`[DG-0896, DG-0902]`|
|`food-storage-rodent-bite-isolation-001`|鼠咬食物批次隔离|食物|P0|high|`[DG-0896, DG-0901]`|
|`general-storage-check-calendar-001`|储藏复查日历|信息保存与长期重建|P2|normal|`[DG-0902]`|
|`general-storage-expired-item-hold-card-001`|过期物品暂存卡|信息保存与长期重建|P1|caution|`[DG-0901, DG-0902]`|
|`general-storage-fifo-shelf-rule-001`|先入先出货架规则|信息保存与长期重建|P0|caution|`[DG-0895, DG-0902]`|
|`general-storage-handover-card-001`|储藏区交接卡|信息保存与长期重建|P2|normal|`[DG-0902]`|
|`general-storage-inventory-card-minimum-001`|储藏库存卡最小字段|信息保存与长期重建|P2|normal|`[DG-0902]`|
|`general-storage-issue-log-001`|储藏异常记录|信息保存与长期重建|P2|normal|`[DG-0902]`|
|`general-storage-label-minimum-fields-001`|储藏标签最小字段|信息保存与长期重建|P0|high|`[DG-0895, DG-0902]`|
|`general-storage-moisture-daily-check-001`|储藏区潮湿日查|信息保存与长期重建|P0|high|`[DG-0895]`|
|`general-storage-pest-rodent-check-001`|储藏区虫鼠巡查|信息保存与长期重建|P0|high|`[DG-0895, DG-0896]`|
|`general-storage-suspect-batch-quarantine-001`|可疑批次隔离区|污染控制 / 隔离 / 清洁分区|P0|high|`[DG-0901]`|
|`general-storage-zone-basic-layout-001`|长期储藏区基础分区|信息保存与长期重建|P0|high|`[DG-0895]`|
|`hygiene-storage-contaminated-container-hold-001`|污染容器暂存标签|污染控制 / 隔离 / 清洁分区|P0|high|`[DG-0901]`|
|`hygiene-storage-leak-odor-isolation-001`|漏液异味物品隔离|污染控制 / 隔离 / 清洁分区|P0|high|`[DG-0901]`|
|`medical-storage-care-supply-zone-001`|照护用品储藏分区|医疗急救|P1|caution|`[DG-0898]`|
|`medical-storage-dressing-bandage-dry-pack-001`|敷料和绷带干燥封存|医疗急救|P0|high|`[DG-0898, DG-0901]`|
|`medical-storage-expired-unknown-medicine-hold-001`|过期和不明药品暂存|医疗急救|P0|high|`[DG-0898, DG-0901]`|
|`medical-storage-first-aid-kit-monthly-check-001`|急救包月度复查|医疗急救|P0|high|`[DG-0898]`|
|`medical-storage-medicine-dry-dark-box-001`|药品避光防潮盒|医疗急救|P0|high|`[DG-0898]`|
|`repair-storage-fabric-rope-dry-check-001`|布料和绳索储藏复查|维修 / 制作 / 替代 / 拆解再利用|P1|caution|`[DG-0899]`|
|`repair-storage-sharp-tool-secure-box-001`|刃具和锋利工具安全盒|基础制造与材料维修|P1|high|`[DG-0899, DG-0901]`|
|`repair-storage-shelf-overload-warning-001`|储藏架超载警示|基础制造与材料维修|P1|high|`[DG-0895, DG-0899]`|
|`repair-storage-small-parts-bin-label-001`|小零件盒分类标签|基础制造与材料维修|P1|normal|`[DG-0899, DG-0902]`|
|`repair-storage-tool-dry-rust-check-001`|工具干燥防锈储藏|基础制造与材料维修|P1|caution|`[DG-0899]`|
|`repair-storage-wood-metal-material-stack-001`|木材金属片堆放边界|基础制造与材料维修|P1|high|`[DG-0899]`|

覆盖范围：食物长期储藏、种子储藏、医疗物资储藏、工具材料储藏、能源/燃料储藏、污染/损坏/过期隔离、储藏记录与交接。

## 2. 新增 Guide 清单

新增 8 个 Guide，存放于 `data/guides/storage/`。Guide 均为行动入口，包含 scenario、steps、check、stop_or_escalate、fallback、risk_level 和 related_wiki。

|Guide|title|category|priority|risk_level|related_wiki 数|
|---|---|---|---|---|---:|
|`DG-0895`|储藏区基础分区与标签|信息保存与长期重建|P0|high|6|
|`DG-0896`|干粮 / 米面豆类防潮防虫储藏|食物|P0|high|7|
|`DG-0897`|种子储藏与复查|种植与食物生产|P0|high|5|
|`DG-0898`|医疗物资与急救包储藏复查|医疗急救|P0|high|5|
|`DG-0899`|工具和材料防潮防锈储藏|基础制造与材料维修|P1|high|6|
|`DG-0900`|能源 / 燃料物资安全储藏|能源|P1|high|5|
|`DG-0901`|霉变 / 漏液 / 过期 / 污染物隔离|污染控制 / 隔离 / 清洁分区|P0|high|12|
|`DG-0902`|储藏记录、先入先出与交接|信息保存与长期重建|P2|normal|13|

## 3. Guide-Wiki 双向关系

本批建立新增 Guide-Wiki 边 `59` 条。每条关系均满足：Guide `related_wiki` 包含 Wiki slug，Wiki `guide_links` 包含 Guide ID。

|Guide|核心 evidence|
|---|---|
|`DG-0895` 储藏区基础分区与标签|`general-storage-zone-basic-layout-001`, `general-storage-label-minimum-fields-001`, `general-storage-moisture-daily-check-001`, `general-storage-pest-rodent-check-001`, `general-storage-fifo-shelf-rule-001` 等 6 篇|
|`DG-0896` 干粮 / 米面豆类防潮防虫储藏|`food-storage-dry-grain-zone-001`, `food-storage-grain-moisture-clump-check-001`, `food-storage-rodent-bite-isolation-001`, `food-storage-can-jar-monthly-check-001`, `food-storage-opened-package-short-use-001` 等 7 篇|
|`DG-0897` 种子储藏与复查|`agriculture-storage-seed-box-dry-check-001`, `agriculture-storage-seed-bag-label-001`, `agriculture-storage-seed-food-separation-shelf-001`, `agriculture-storage-seed-moisture-quarantine-001`, `agriculture-storage-seed-library-rotation-card-001`|
|`DG-0898` 医疗物资与急救包储藏复查|`medical-storage-medicine-dry-dark-box-001`, `medical-storage-first-aid-kit-monthly-check-001`, `medical-storage-dressing-bandage-dry-pack-001`, `medical-storage-expired-unknown-medicine-hold-001`, `medical-storage-care-supply-zone-001`|
|`DG-0899` 工具和材料防潮防锈储藏|`repair-storage-tool-dry-rust-check-001`, `repair-storage-sharp-tool-secure-box-001`, `repair-storage-small-parts-bin-label-001`, `repair-storage-fabric-rope-dry-check-001`, `repair-storage-wood-metal-material-stack-001` 等 6 篇|
|`DG-0900` 能源 / 燃料物资安全储藏|`energy-storage-battery-dry-cool-zone-001`, `energy-storage-battery-leak-suspect-bin-001`, `energy-storage-power-bank-rotation-label-001`, `fire-storage-match-candle-dry-box-001`, `fire-storage-fuel-away-from-living-zone-001`|
|`DG-0901` 霉变 / 漏液 / 过期 / 污染物隔离|`general-storage-suspect-batch-quarantine-001`, `hygiene-storage-leak-odor-isolation-001`, `hygiene-storage-contaminated-container-hold-001`, `energy-storage-battery-leak-suspect-bin-001`, `medical-storage-expired-unknown-medicine-hold-001` 等 12 篇|
|`DG-0902` 储藏记录、先入先出与交接|`general-storage-inventory-card-minimum-001`, `general-storage-check-calendar-001`, `general-storage-handover-card-001`, `general-storage-issue-log-001`, `general-storage-label-minimum-fields-001` 等 13 篇|

边界控制：未把 Food 的入口食用判断、Agriculture 的播种生产计划、Medical 的药品治疗判断、Manufacturing 的货架制作、Waste 的废弃物分流或 Energy 的电池使用维护硬并入 Storage。Storage 主职责保持为保存、标签、复查、轮换、隔离、交接。

## 4. PocketBase 同步结果

- 已将 40 篇新增 Wiki upsert 到本地 `pocketbase/pb_data/data.db` 的 `wiki_articles`。
- 使用现有 `wiki_categories` category id，同步 title、slug、category、summary、content、tags、risk_level、status、source。
- `python3 tools/audit_wiki.py` 确认 Markdown Wiki 与 PocketBase wiki_articles 数量一致：`1066 / 1066`。

## 5. Audit 结果

|命令|结果|
|---|---|
|`python3 tools/audit_wiki.py`|errors=0, warnings=0, advisories=0|
|`python3 tools/build_guides.py`|Generated 828 Guides; Generated 828 Guide Index Items|
|`python3 scripts/audit_guides.py`|errors=0, warnings=0, advisories=0|

Guide-Wiki 验收：single_forward=0，single_reverse=0，invalid Guide ID=0，invalid Wiki slug=0。

## 6. 未覆盖内容

- 未新增 Retrieval profile，等待 Batch12-D Field Test 后再判断是否需要最小检索入口修复。
- 未做冷链系统、大型仓库工程、城市供应链管理或现代库存软件依赖。
- 未做专业药品替代判断、过期药品深度可用性判断、危险化学品储藏教程、大型燃料库工程。
- 未扩展到团队级资源配给治理；本批只完成 Long-Term Storage v0.1 的保存、标签、复查、隔离和交接主链。

## 7. 下一步 Field Test 建议

建议进入 Batch12-D Long-Term Storage Retrieval Field Test，覆盖 20-24 个 case。重点观察：

- “米面有霉味/虫害/鼠咬还能不能吃”是否由 Storage evidence 进入，Food 作为入口判断补充。
- “种子受潮/标签不清/留种食用混放”是否命中 DG-0897。
- “药品过期/敷料受潮/急救包缺件”是否命中 DG-0898，Medical 不应被 Kiwix 背景替代。
- “电池漏液/燃料靠近火源/火柴受潮”是否命中 DG-0900 或 DG-0901，Energy/Fire 作为安全补充。
- “库存卡、先入先出、交接卡、异常记录”是否命中 DG-0902。
- cross domain 重点统计 Food、Agriculture、Medical、Manufacturing、Waste、Energy、Fire 是否抢主位。

## 8. 结论

Batch12-C 已完成 Long-Term Storage v0.1 第一阶段 Apply：40 篇 Wiki、8 个 Guide、双向 evidence chain、PocketBase 同步、guide_index / emergency_guides 更新与审计清零。下一阶段应只做 Field Test，不提前修改 Retrieval。

