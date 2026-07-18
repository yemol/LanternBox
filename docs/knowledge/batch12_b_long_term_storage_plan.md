# LanternBox Batch12-B: Long-Term Storage Coverage Planning

生成日期：2026-07-18

本阶段只做覆盖分析与规划报告。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或 tests。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_5.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`
- `docs/knowledge/batch11_f_waste_recycling_final_verification.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`

核心判断：当前没有独立 `wiki_import/storage` 或 `data/guides/storage` 目录。长期储藏能力分散在 Food、Agriculture、Medical、Manufacturing、Waste、Energy、Tools / Repair 中，有大量局部条目，但缺一个 Long-Term Storage 主行动链。Batch12-C 应建立“分类储藏 -> 防潮防虫鼠 -> 批次标签 -> 先入先出 -> 周期复查 -> 可疑隔离 -> 储藏交接”的第一阶段能力。

## 1. 当前覆盖审查

实际存在的相关目录：

|类型|存在目录|不存在目录|
|---|---|---|
|Wiki|`wiki_import/food`、`wiki_import/planting`、`wiki_import/medical`、`wiki_import/tools`、`wiki_import/repair`、`wiki_import/manufacturing`、`wiki_import/waste`、`wiki_import/power`|`wiki_import/storage`、`wiki_import/agriculture`、`wiki_import/energy`|
|Guide|`data/guides/food`、`data/guides/planting`、`data/guides/medical`、`data/guides/tools`、`data/guides/repair`、`data/guides/manufacturing`、`data/guides/waste`、`data/guides/power`|`data/guides/storage`、`data/guides/agriculture`、`data/guides/energy`|

目录总量：

|目录|Wiki|Guide|
|---|---:|---:|
|food|35|21|
|planting|93|20|
|medical|51|75|
|tools|4|48|
|repair|100|5|
|manufacturing|61|8|
|waste|36|8|
|power|74|72|

长期储藏相关抽取：

|口径|Wiki|Guide|Guide-Wiki 边|说明|
|---|---:|---:|---:|---|
|宽口径 storage 语义|521|271|795|命中“记录、标签、批次、电池、材料”等宽词，说明储藏相关信息非常分散，不代表能力完整。|
|核心储藏口径|65|40|174|命中标题/slug/summary/tags 中的储藏、保存、库存、防潮、防虫、防鼠、有效期、材料池、种子库、轮换等核心语义。|

核心储藏 Wiki 分布：

|目录|核心 storage Wiki|有 guide_links|
|---|---:|---:|
|waste|22|22|
|planting|19|19|
|food|10|10|
|power|5|5|
|repair|5|5|
|medical|3|3|
|manufacturing|1|1|

核心储藏 Guide 分布：

|目录|核心 storage Guide|有 related_wiki|无 related_wiki|
|---|---:|---:|---:|
|power|12|5|7|
|food|8|8|0|
|waste|6|6|0|
|tools|5|3|2|
|medical|5|1|4|
|planting|4|3|1|

重要现有强项：

- Food 已有干粮罐头轮换、食物库存估算、包装破损、霉变粮食停用、保存食物远离垃圾区。
- Agriculture Second Stage 已有种子保存、发芽率复测、收获后晾晒防霉、储粮鼠虫防护、种食分离。
- Medical 已有常用药储存检查、药品有效期/状态/标签检查、药品不足记录。
- Waste / Recycling 已有材料池台账、材料池分区、污染容器降级、可疑回收材料批次隔离。
- Tools / Repair 已有工具防锈防潮、工具寿命记录、小零件分类保存、耗材补给记录。
- Energy 已有备用电池封存和轮换、充电宝轮换、移动电源保管、电池异常隔离。

明显缺口：

- 没有统一 Long-Term Storage 主入口和目录。
- 没有储藏区基础分区、离地、防潮、防虫、防鼠、防霉的总控 Guide。
- 食物、种子、药品、工具材料、电池/燃料的储藏分别存在，但缺统一批次标签、FIFO、复查周期和交接。
- 可疑批次、过期物、污染物、漏液物、异味物的“储藏隔离区”没有独立行动链。
- Medical 相关 storage Guide 中多个无 related_wiki，例如 DG-0214 药品受潮处理、DG-0209 药品分区、DG-0216 药箱访问规则。
- Energy / Tools 中很多储藏相关 Guide 是旧入口或管理入口，没有进入 Long-Term Storage evidence 结构。

与相邻 stable 域的重叠点：

|相邻领域|现有 stable / candidate|与 Storage 的接口|
|---|---|---|
|Agriculture Second Stage|v0.1 stable|种子、收获物、储粮批次、鼠虫巡查、种食分离。|
|Food|基础可行动|入口前能否食用、食物中毒风险和烹饪判断。Storage 只管保存状态和批次。|
|Medical|v0.1 stable|药品是否可用、医疗处置由 Medical 主导。Storage 管药箱分区、避光防潮、过期隔离和复查。|
|Manufacturing Foundation|v0.1 stable|货架、容器、支撑和承重制作由 Manufacturing 主导。Storage 提供储藏需求和堆叠/承重警示。|
|Waste / Recycling|v0.1 stable|损坏、污染、漏液、不可用物进入 Waste 分流。Storage 负责先隔离、标记和交接。|
|Energy Safety / Management|stable / candidate|电池使用、异常和供电安全由 Energy 主导。Storage 管保存位置、标签、轮换和高风险隔离。|

## 2. Long-Term Storage 能力边界

Long-Term Storage 主导：

- 储藏区规划。
- 分类存放。
- 防潮 / 防虫 / 防鼠 / 防霉。
- 批次标签。
- 先入先出。
- 定期复查。
- 损坏 / 过期 / 污染物隔离。
- 储藏记录。
- 储藏容器检查。
- 储藏区交接。

相邻领域主导：

|领域|主导内容|Storage 边界|
|---|---|---|
|Food|食物是否能吃、食物中毒风险、烹饪和入口前判断。|Storage 记录保存位置、批次、受潮、虫害、鼠咬和隔离状态。|
|Agriculture|留种、种子复测、采后初步晾晒、种子库生产计划。|Storage 管种子盒/袋、分区、复查日期、种食分离和储藏交接。|
|Medical|药品是否适合使用、医疗处置、症状观察。|Storage 管药品避光防潮、急救包复查、过期/受潮隔离和标签。|
|Manufacturing|制作货架、容器、支撑结构、承重检查。|Storage 提供储藏需求、堆叠限制、不可超载和复查要求。|
|Waste / Recycling|损坏物、污染物、废弃物分流、材料池判断、不可用物品标记。|Storage 负责先建立隔离区、标签和交接，不判断再利用加工。|
|Energy|电池使用与维护、供电系统安全、储能设备运行。|Storage 管电池/充电宝保存、轮换标签、远离热源和异常隔离。|
|Fire|明火、余火、燃烧、火源安全。|Storage 只管火柴、蜡烛、燃料防潮和远离火源，不指导燃烧操作。|

一句话边界：Long-Term Storage 只负责“东西如何被安全保存、标记、复查、轮换、隔离和交接”。

## 3. 主题缺口分析

### P0 食物长期储藏

|检查项|当前覆盖|缺口|
|---|---|---|
|干粮储藏|干粮罐头轮换、三天配给已有。|缺长期干粮区分区、离地、防潮和月度复查。|
|米面豆类防潮|有库存和霉变判断片段。|缺米面豆类专门储藏、结块/虫蛀/鼠咬隔离。|
|罐头 / 密封食品复查|鼓包禁食和轮换已有。|缺罐头外观巡查、锈蚀、标签脱落、批次架位。|
|霉味 / 虫害 / 鼠咬隔离|Food / Agriculture 有停用线。|缺 Storage 主导的“可疑批次隔离区”。|
|先入先出 / 批次标签|局部存在。|缺统一 FIFO 标签和领用记录。|
|储粮区防鼠虫|Agriculture 有储粮鼠虫防护。|缺跨食物储藏区日查/月查。|

### P0 种子储藏

|检查项|当前覆盖|缺口|
|---|---|---|
|种子干燥保存|Agriculture 已强。|Storage 需要把种子保存变成储藏区制度。|
|种子批次标签|已有批次与发芽率复测。|缺统一盒/袋标签、外来种子隔离架位。|
|种子盒 / 种子袋|有小型种子库盒索引。|缺防潮层、容器破损、虫蛀复查。|
|潮湿复查|有受潮失效判断。|缺周期复查和待复测区。|
|留种和食用分离|已有。|需要作为 Storage 停止线强化。|
|种子库轮换|有复测，但缺年度轮换总控。|规划 v0.1 做基础轮换，年度生产计划留 v0.2。|

### P0 医疗物资储藏

|检查项|当前覆盖|缺口|
|---|---|---|
|药品避光 / 防潮|常用药储存检查已有。|缺药箱分区和避光防潮日常复查 Guide。|
|包装破损 / 过期标记|DG-0210 有入口。|DG-0209 / DG-0214 / DG-0216 evidence 不足。|
|敷料 / 绷带 / 消毒用品保存|外伤耗材记录有片段。|缺敷料/绷带受潮、外包装破损、污染隔离。|
|医疗物资分区|有药品分区 Guide，但缺 related_wiki。|需要新增 storage Wiki 支撑，不替代 Medical 判断。|
|急救包复查|撤离包和药箱索引有片段。|缺急救包月度复查和缺件记录。|

### P1 工具与材料储藏

|检查项|当前覆盖|缺口|
|---|---|---|
|工具防锈|DG-0373 和 Wiki 已有。|缺作为 Storage 总控的一类物资复查链。|
|刃具和锋利物安全存放|Tools / Waste 有危险工具隔离和尖锐物。|缺“可用刃具储藏”与“废弃尖锐物”边界。|
|小零件分类|螺丝钉垫片分类保存已有。|缺小零件盒标签、领用和低库存线。|
|绳索 / 布料 / 木材 / 金属片保存|Waste / Manufacturing 有材料池。|缺长期储存防潮、防虫、降级复查。|
|材料池与储藏区边界|Waste 已强。|缺“材料池不是成品/主库存”的 Storage 分区。|
|承重 / 堆叠安全|Manufacturing 有承重检查。|Storage 需要堆叠高度、货架异常、通道占用停止线。|

### P1 能源与燃料储藏

|检查项|当前覆盖|缺口|
|---|---|---|
|电池储藏|备用电池封存、轮换、电池异常已有。|缺 Storage 主导的电池储藏区、标签和定期复查。|
|充电宝 / 小型电源储藏|移动电源保管、充电宝轮换已有。|缺“未使用时如何放、何时隔离”的统一储藏入口。|
|燃料远离火源|Fire / Energy 有使用边界。|缺燃料储藏点和生活区/火源区距离边界。|
|蜡烛 / 火柴 / 点火材料防潮|蜡烛使用边界已有。|缺防潮储藏和发霉/受潮停用判断。|
|能源物资标签|局部存在。|缺可用/待查/禁用三区标签。|
|高风险能源物品隔离|Energy Safety 强。|Storage 只承接隔离区和交接，不做拆修。|

### P1 污染 / 损坏 / 过期物隔离

|检查项|当前覆盖|缺口|
|---|---|---|
|过期物品隔离区|Food/Medical 有过期停用。|缺统一过期/待查隔离区。|
|污染物品隔离区|Waste / WASH 强。|缺 Storage 到 Waste 的交接卡。|
|霉变 / 漏液 / 异味物品|各领域有停用线。|缺同源批次复查和不混回主库的流程。|
|损坏容器|Tools/Repair 有容器修补/降级。|缺入口用途禁用和储藏降级标签。|
|等待复查标签|Waste 有来源标签。|缺可疑库存统一标签。|

### P2 储藏记录与交接

|检查项|当前覆盖|缺口|
|---|---|---|
|库存卡|Food/Repair/Waste 有局部台账。|缺 Storage 总库存卡。|
|批次记录|Agriculture / Food 强。|缺跨域字段统一。|
|复查日期|多 Wiki 都提到。|缺复查日历和逾期处理。|
|领用记录|工具借用、食物配给有片段。|缺储藏区领用记录。|
|先入先出记录|Food 有轮换。|缺跨食物/药品/材料的 FIFO 规则。|
|库存异常记录|Waste/Manufacturing 有失败记录。|缺异常到隔离/报废/补给的闭环。|
|下一班交接|Waste 和团队交接有基础。|缺 Storage 交接卡。|

## 4. Wiki 规划清单

建议 Batch12-C 第一批新增 40 篇 Wiki。由于当前无 `storage` slug domain，也不建议修改 schema，本规划使用现有正式 slug 前缀与 category：`food-*`、`agriculture-*`、`medical-*`、`repair-*`、`energy-*`、`general-*`、`hygiene-*`。如后续项目正式引入 `storage` domain，应另行 schema / audit 规划。

|slug|title|category|priority|risk_level|summary|intended Guide relation|Field Test|
|---|---|---|---|---|---|---|---|
|general-storage-zone-basic-layout-001|长期储藏区基础分区|信息保存与长期重建|P0|high|把食物、种子、医疗、工具材料、能源、可疑物分成固定区域，避免混放和误用。|DG-0895|是|
|general-storage-label-minimum-fields-001|储藏标签最小字段|信息保存与长期重建|P0|high|规定名称、来源、批次、入库日期、复查日、状态和负责人。|DG-0895, DG-0902|是|
|general-storage-fifo-shelf-rule-001|先入先出货架规则|信息保存与长期重建|P0|caution|用前后排、日期卡和领用记录执行 FIFO，避免旧批次被遗忘。|DG-0895, DG-0902|是|
|general-storage-moisture-daily-check-001|储藏区潮湿日查|信息保存与长期重建|P0|high|检查返潮、凝水、漏水、地面潮气和容器湿痕。|DG-0895|是|
|general-storage-pest-rodent-check-001|储藏区虫鼠巡查|信息保存与长期重建|P0|high|记录虫粪、咬痕、散落颗粒、袋口破损和鼠道。|DG-0895, DG-0896|是|
|general-storage-suspect-batch-quarantine-001|可疑批次隔离区|污染控制 / 隔离 / 清洁分区|P0|high|把霉味、漏液、异味、鼠咬、包装破损、标签丢失物品移出主库。|DG-0901|是|
|food-storage-dry-grain-zone-001|干粮和米面豆类储藏区|食物|P0|high|干粮、米面、豆类离地、分批、避潮、远离垃圾和气味源。|DG-0896|是|
|food-storage-grain-moisture-clump-check-001|米面豆类受潮结块检查|食物|P0|high|用结块、霉味、虫粉、潮袋和温热作为可疑批次信号。|DG-0896, DG-0901|是|
|food-storage-can-jar-monthly-check-001|罐头和密封食品月度复查|食物|P0|high|复查鼓包、锈蚀、漏液、标签脱落和同箱批次。|DG-0896|是|
|food-storage-rodent-bite-isolation-001|鼠咬食物批次隔离|食物|P0|high|鼠咬、鼠粪、尿味和袋口破损时先隔离同源批次。|DG-0896, DG-0901|是|
|food-storage-opened-package-short-use-001|开封食品短期使用标签|食物|P0|caution|开封后写日期、经手人和最晚复查，不混入未开封储备。|DG-0896, DG-0902|是|
|food-storage-emergency-ration-reserve-line-001|应急口粮保底线|食物|P0|high|把不可随意动用的最低口粮单独标记，防止日常消耗吃掉底线。|DG-0896|是|
|agriculture-storage-seed-box-dry-check-001|种子盒干燥复查|种植与食物生产|P0|high|检查种子盒、纸袋、干燥层、霉味、虫蛀和结块。|DG-0897|是|
|agriculture-storage-seed-bag-label-001|种子袋批次标签|种植与食物生产|P0|caution|每袋种子标作物、来源、年份、复测日、用途和保留等级。|DG-0897, DG-0902|是|
|agriculture-storage-seed-food-separation-shelf-001|留种和食用批次分架|种植与食物生产|P0|high|把核心留种、可播种、可食用和待复测批次分开。|DG-0897|是|
|agriculture-storage-seed-moisture-quarantine-001|受潮种子待复测隔离|种植与食物生产|P0|high|受潮、结块、虫蛀、外来来源不明种子不进主种子库。|DG-0897, DG-0901|是|
|agriculture-storage-seed-library-rotation-card-001|小型种子库轮换卡|种植与食物生产|P1|caution|按作物记录复测、播种、淘汰和保留数量。|DG-0897, DG-0902|是|
|medical-storage-medicine-dry-dark-box-001|药品避光防潮盒|医疗急救|P0|high|药品保持原标签、干燥避光、远离食物和儿童，不明药不入主盒。|DG-0898|是|
|medical-storage-first-aid-kit-monthly-check-001|急救包月度复查|医疗急救|P0|high|检查急救包位置、缺件、受潮、包装破损、有效期和交接人。|DG-0898|是|
|medical-storage-dressing-bandage-dry-pack-001|敷料和绷带干燥封存|医疗急救|P0|high|敷料、绷带、纱布受潮、开封、污染时从可用急救物资中移出。|DG-0898, DG-0901|是|
|medical-storage-expired-unknown-medicine-hold-001|过期和不明药品暂存|医疗急救|P0|high|过期、标签不清、来源不明、变色异味药品标记隔离，不进入正常药箱。|DG-0898, DG-0901|是|
|medical-storage-care-supply-zone-001|照护用品储藏分区|医疗急救|P1|caution|口服药、外用药、敷料、清洁用品、病人专用品分区存放。|DG-0898|是|
|repair-storage-tool-dry-rust-check-001|工具干燥防锈储藏|基础制造与材料维修|P1|caution|工具清洁、干燥、离地，锈蚀和裂损工具标记待查。|DG-0899|是|
|repair-storage-sharp-tool-secure-box-001|刃具和锋利工具安全盒|基础制造与材料维修|P1|high|刀具、锯片、钻头、针钉不裸露，不与普通材料混放。|DG-0899|是|
|repair-storage-small-parts-bin-label-001|小零件盒分类标签|基础制造与材料维修|P1|normal|螺丝、垫片、钉子、夹子按尺寸/用途/状态分盒记录。|DG-0899, DG-0902|是|
|repair-storage-fabric-rope-dry-check-001|布料和绳索储藏复查|维修 / 制作 / 替代 / 拆解再利用|P1|caution|旧布、绳、绑带防潮、防霉、防鼠咬，承重用途单独标记。|DG-0899|是|
|repair-storage-wood-metal-material-stack-001|木材金属片堆放边界|基础制造与材料维修|P1|high|木材离地通风，金属防锈，尖锐边和承重不明材料标记。|DG-0899|是|
|repair-storage-shelf-overload-warning-001|储藏架超载警示|基础制造与材料维修|P1|high|发现弯曲、摇晃、连接松动、地面下陷时停止堆叠并转 Manufacturing 检查。|DG-0895, DG-0899|是|
|energy-storage-battery-dry-cool-zone-001|电池干燥阴凉储藏区|能源|P1|high|电池、充电宝、小电源远离热源、潮湿、金属杂物和儿童。|DG-0900|是|
|energy-storage-power-bank-rotation-label-001|充电宝轮换标签|能源|P1|caution|给充电宝编号、记录最后充放电、异常和复查日期。|DG-0900, DG-0902|是|
|energy-storage-battery-leak-suspect-bin-001|漏液疑似电池隔离盒|能源|P0|high|鼓包、漏液、腐蚀、发热、异味电池不入正常储藏区。|DG-0900, DG-0901|是|
|fire-storage-match-candle-dry-box-001|火柴蜡烛防潮盒|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|火柴、蜡烛、点火材料保持干燥、标记数量、远离睡眠和儿童。|DG-0900|是|
|fire-storage-fuel-away-from-living-zone-001|燃料远离生活区储藏线|火源 / 保温 / 通风 / 一氧化碳风险|P1|high|燃料不靠近火源、厨房、睡眠区和通道；异常气味转 Fire / Energy 判断。|DG-0900|是|
|hygiene-storage-leak-odor-isolation-001|漏液异味物品隔离|污染控制 / 隔离 / 清洁分区|P0|high|发现漏液、刺鼻味、发黏、渗出时先移入隔离区并标来源。|DG-0901|是|
|hygiene-storage-contaminated-container-hold-001|污染容器暂存标签|污染控制 / 隔离 / 清洁分区|P0|high|来源不明、病人接触、化学味、腐败渗液容器不得回主库。|DG-0901|是|
|general-storage-expired-item-hold-card-001|过期物品暂存卡|信息保存与长期重建|P1|caution|过期、标签不清、状态可疑物品写明来源、发现人、下一步处理。|DG-0901, DG-0902|是|
|general-storage-inventory-card-minimum-001|储藏库存卡最小字段|信息保存与长期重建|P2|normal|记录物品、批次、数量、位置、状态、复查日、领用人。|DG-0902|是|
|general-storage-check-calendar-001|储藏复查日历|信息保存与长期重建|P2|normal|为食物、种子、药箱、工具材料、电池设置不同复查周期。|DG-0902|是|
|general-storage-handover-card-001|储藏区交接卡|信息保存与长期重建|P2|normal|交接可疑批次、缺件、低库存、隔离区和下次复查事项。|DG-0902|是|
|general-storage-issue-log-001|储藏异常记录|信息保存与长期重建|P2|normal|记录受潮、虫害、鼠咬、漏液、过期、丢失和误用，形成复盘。|DG-0902|是|

比例建议：

- P0：20 篇
- P1：16 篇
- P2：4 篇

## 5. Guide 候选

Guide ID 从当前最大 `DG-0894` 后顺延，仅为规划建议。

### DG-0895 储藏区基础分区与标签

- scenario：长期断供、小团队驻留或物资增多后，食物、种子、药品、工具材料、电池和可疑物混放，需要建立基础储藏区。
- steps：
  1. 先分出食物、种子、医疗、工具材料、能源物资、可疑/隔离物。
  2. 把储藏区远离地面潮气、垃圾、火源、厕所和儿童活动区。
  3. 给每个区贴名称、负责人、复查日。
  4. 给每批物品贴最小字段标签。
  5. 可疑物不得回主库。
  6. 设置每日潮湿/虫鼠快速检查。
- check：
  - 分区清楚。
  - 标签可读。
  - 可疑物有单独位置。
  - 地面潮气不直接接触物资。
  - 通道不被堆叠物堵住。
- stop_or_escalate：
  - 发现霉味、漏液、鼠咬、虫害、标签丢失、货架变形、儿童可接触高风险物时暂停领用。
- fallback：
  - 没有货架时用木板、砖、箱子离地。
  - 没有标签时用纸条和绳线固定。
  - 容器不足时优先隔离食品、药品、电池和尖锐物。
- related_wiki：
  - `general-storage-zone-basic-layout-001`
  - `general-storage-label-minimum-fields-001`
  - `general-storage-fifo-shelf-rule-001`
  - `general-storage-moisture-daily-check-001`
  - `general-storage-pest-rodent-check-001`
  - `repair-storage-shelf-overload-warning-001`

### DG-0896 干粮 / 米面豆类防潮防虫储藏

- scenario：干粮、米面、豆类、罐头或密封食品需要长期保存，现场担心受潮、虫害、鼠咬或批次混乱。
- steps：
  1. 把开封、未开封和可疑食品分开。
  2. 干粮离地、避潮、远离垃圾和燃料。
  3. 标记批次、入库日和复查日。
  4. 旧批次放在先取位置。
  5. 发现霉味、虫粉、鼠咬、漏液、鼓包时隔离同源批次。
  6. 记录领用和剩余量。
- check：
  - 干粮不接触地面。
  - 米面豆类无结块、虫粉、霉味。
  - 罐头无鼓包漏液。
  - FIFO 可执行。
- stop_or_escalate：
  - 霉味、鼠粪、虫害扩散、罐头鼓包、包装破损接触污染时停止入口，转 Food / Waste 判断。
- fallback：
  - 容器不足时小批分袋，优先保护主食和儿童/病人配给。
  - 标签不足时在外袋写日期和状态。
- related_wiki：
  - `food-storage-dry-grain-zone-001`
  - `food-storage-grain-moisture-clump-check-001`
  - `food-storage-can-jar-monthly-check-001`
  - `food-storage-rodent-bite-isolation-001`
  - `food-storage-opened-package-short-use-001`
  - `food-storage-emergency-ration-reserve-line-001`

### DG-0897 种子储藏与复查

- scenario：种子、留种批次、外来种子或种子盒需要保存和复查，避免受潮、虫蛀、混批或误食。
- steps：
  1. 按作物、来源、年份和用途分袋。
  2. 核心留种、可播种、可食用和待复测分区。
  3. 检查纸袋、盒子、干燥层、霉味、虫蛀、结块。
  4. 受潮或来源不明批次进入待复测区。
  5. 记录复测日期、保留等级和剩余量。
- check：
  - 种食分离。
  - 批次标签完整。
  - 受潮/虫害批次未进入主种子库。
  - 下一次复测日期明确。
- stop_or_escalate：
  - 标签丢失、霉味、虫蛀、批次混杂、核心留种被当作食物使用时停止领用并复盘。
- fallback：
  - 没有种子盒时用干净纸包和外层硬盒。
  - 复测条件不足时保持隔离，不混入主库。
- related_wiki：
  - `agriculture-storage-seed-box-dry-check-001`
  - `agriculture-storage-seed-bag-label-001`
  - `agriculture-storage-seed-food-separation-shelf-001`
  - `agriculture-storage-seed-moisture-quarantine-001`
  - `agriculture-storage-seed-library-rotation-card-001`
  - `agriculture-seed-batch-viability-ledger-001`
  - `agriculture-seed-library-box-index-001`

### DG-0898 医疗物资与急救包储藏复查

- scenario：药品、急救包、敷料、绷带、消毒用品和照护用品需要长期保存和定期复查。
- steps：
  1. 药品保留原标签，口服、外用、慢病、儿童、敷料分区。
  2. 药箱避光、防潮、远离食物和儿童。
  3. 检查有效期、包装、受潮、变色、异味和缺件。
  4. 过期、不明、受潮、包装破损物品进入暂存区。
  5. 记录缺件、复查日和负责人。
- check：
  - 急救包位置固定。
  - 药品标签可读。
  - 敷料干燥封存。
  - 过期/不明药未在正常药箱。
- stop_or_escalate：
  - 药品标签不清、受潮、变色、过期、儿童可接触、急救包缺关键物资时暂停常规可用状态并转 Medical 判断。
- fallback：
  - 没有药箱时用干燥避光容器分区。
  - 缺清单时先手写药名、用途、有效期、数量和位置。
- related_wiki：
  - `medical-storage-medicine-dry-dark-box-001`
  - `medical-storage-first-aid-kit-monthly-check-001`
  - `medical-storage-dressing-bandage-dry-pack-001`
  - `medical-storage-expired-unknown-medicine-hold-001`
  - `medical-storage-care-supply-zone-001`
  - `medical-medication-storage-check-001`

### DG-0899 工具和材料防潮防锈储藏

- scenario：工具、刃具、小零件、木材、金属片、布料、绳索和材料池物资需要长期保存，避免锈蚀、霉变、混放和堆叠风险。
- steps：
  1. 工具清洁干燥后归位。
  2. 刃具、钻头、针钉和锋利物放入安全盒。
  3. 小零件按尺寸/用途分盒。
  4. 木材、金属、布料、绳索离地防潮。
  5. 承重不明、锈蚀、虫蛀、污染物标记待查。
  6. 货架弯曲、摇晃或堆叠过高时停止加物。
- check：
  - 工具干燥。
  - 刃具不裸露。
  - 小零件有标签。
  - 材料池与成品/主库存分开。
  - 货架无变形。
- stop_or_escalate：
  - 发现锈蚀扩散、木材霉烂虫蛀、尖锐物裸露、货架变形、儿童可接触时停止领用。
- fallback：
  - 无货架时先离地、通风、小批分区。
  - 无防锈油时先擦干、包布、隔潮。
- related_wiki：
  - `repair-storage-tool-dry-rust-check-001`
  - `repair-storage-sharp-tool-secure-box-001`
  - `repair-storage-small-parts-bin-label-001`
  - `repair-storage-fabric-rope-dry-check-001`
  - `repair-storage-wood-metal-material-stack-001`
  - `repair-storage-shelf-overload-warning-001`

### DG-0900 能源 / 燃料物资安全储藏

- scenario：电池、充电宝、小电源、火柴、蜡烛、点火材料或燃料需要保存，避免受潮、发热、漏液、误触和靠近火源。
- steps：
  1. 电池/充电宝按编号、状态和复查日分区。
  2. 电池远离潮湿、热源、金属杂物和儿童。
  3. 鼓包、漏液、发热、异味物品进入隔离盒。
  4. 火柴、蜡烛、点火材料防潮保存。
  5. 燃料远离火源、厨房、睡眠区和通道。
  6. 记录轮换和异常。
- check：
  - 可用、待查、禁用分区清楚。
  - 电池无漏液、鼓包、腐蚀。
  - 点火材料干燥。
  - 燃料不靠近火源。
- stop_or_escalate：
  - 电池发热、漏液、鼓包、燃料异味、容器破损、火柴受潮霉变时停止常规储藏并转 Energy / Fire。
- fallback：
  - 容器不足时优先隔离异常电池和燃料。
  - 点火材料不足时小包分散防潮，不集中放在火源旁。
- related_wiki：
  - `energy-storage-battery-dry-cool-zone-001`
  - `energy-storage-power-bank-rotation-label-001`
  - `energy-storage-battery-leak-suspect-bin-001`
  - `fire-storage-match-candle-dry-box-001`
  - `fire-storage-fuel-away-from-living-zone-001`
  - `energy-battery-003`

### DG-0901 霉变 / 漏液 / 过期 / 污染物隔离

- scenario：储藏区发现霉味、漏液、异味、过期、鼠咬、虫害、包装破损或来源不明物品，需要决定是否停用、隔离或交接。
- steps：
  1. 停止继续领用同源批次。
  2. 移入可疑批次隔离区。
  3. 标记来源、发现时间、发现人、主要风险。
  4. 检查相邻批次。
  5. 判断转 Food / Medical / Waste / Energy / Manufacturing。
  6. 记录恢复、报废或等待复查。
- check：
  - 可疑物已离开主库。
  - 标签写明风险。
  - 同源批次已复查。
  - 没有回流到食物、药箱、材料池或能源区。
- stop_or_escalate：
  - 漏液、刺鼻味、霉尘扩散、鼠粪、药品不明、罐头鼓包、电池漏液时停止普通储藏流程。
- fallback：
  - 无隔离箱时用固定角落、纸板标记和外层袋隔离。
  - 无法判断时按更高风险处理。
- related_wiki：
  - `general-storage-suspect-batch-quarantine-001`
  - `hygiene-storage-leak-odor-isolation-001`
  - `hygiene-storage-contaminated-container-hold-001`
  - `general-storage-expired-item-hold-card-001`
  - `medical-storage-expired-unknown-medicine-hold-001`
  - `energy-storage-battery-leak-suspect-bin-001`
  - `food-storage-rodent-bite-isolation-001`

### DG-0902 储藏记录、先入先出与交接

- scenario：多人共用储藏区，物资有进出、复查、低库存、可疑隔离或下一班交接需求。
- steps：
  1. 建立库存卡：物品、批次、数量、位置、状态、复查日。
  2. 领用时写领用人、用途和剩余量。
  3. 旧批次优先领用，新批次放后位。
  4. 复查日历列出食物、种子、药箱、工具材料、电池。
  5. 交接可疑批次、缺件、低库存和隔离区。
  6. 异常写入日志并安排复查。
- check：
  - 找得到物品位置。
  - 知道哪个批次先用。
  - 隔离区有人接手。
  - 低库存有记录。
  - 下一次复查日明确。
- stop_or_escalate：
  - 标签丢失、库存与实物不符、关键物资缺失、可疑批次无人接手时暂停普通领用。
- fallback：
  - 没有本子时用墙面清单和袋身标记。
  - 忙乱时先记录 P0：食物、药品、电池、可疑隔离物。
- related_wiki：
  - `general-storage-inventory-card-minimum-001`
  - `general-storage-check-calendar-001`
  - `general-storage-handover-card-001`
  - `general-storage-issue-log-001`
  - `general-storage-fifo-shelf-rule-001`
  - `general-storage-label-minimum-fields-001`

## 6. Retrieval 风险预测

|Query 类型|可能抢主位|推荐判断|
|---|---|---|
|粮食霉味还能不能吃|Food|Food 应主导能否入口；Storage 补充批次隔离、同源复查、储藏区调整。|
|种子受潮还能不能播|Agriculture|Agriculture 应主导发芽率和播种判断；Storage 补充待复测区、受潮标签和种子库复查。|
|药品过期还能不能用|Medical|Medical 应主导药品可用性边界；Storage 只负责标记、隔离、药箱复查，不给用药替代判断。|
|货架怎么做|Manufacturing|Manufacturing 主导制作和承重；Storage 主导储藏需求、堆叠限制和异常停止线。|
|污染 / 漏液物品怎么办|Waste / Recycling|Waste 主导分流；Storage 主导发现后的隔离区、标签和交接。|
|电池怎么保存|Energy|Energy 可主导电池安全；Storage 应进入 evidence，提供保存位置、轮换、隔离盒和标签。|
|火柴 / 燃料怎么放|Fire / Energy|Fire / Energy 主导火源安全；Storage 补充防潮、远离生活区和复查。|
|库存卡怎么写|Records / Team|Storage 应主导，因为对象是储藏区库存和 FIFO。Records 可补充知识归档。|
|可疑批次放哪里|Waste / Food / Medical|Storage 应主导“暂时放哪里、如何标记、谁复查”；最终去向由相邻领域决定。|

预计需要 Field Test 观察：

- Food 是否完全吞掉“储粮防潮/批次轮换”。
- Agriculture 是否完全吞掉“种子储藏区管理”。
- Medical 是否完全吞掉“药箱储藏复查”。
- Waste 是否完全吞掉“可疑物隔离区”。
- Manufacturing 是否完全吞掉“储藏架超载/堆叠风险”。
- Energy / Fire 是否完全吞掉“电池、燃料、火柴储藏”。

Batch12-C 不建议提前新增 query profile。应先建立 Wiki + Guide + evidence chain，再做 Batch12-D Field Test。

## 7. Batch12-C Apply 建议

建议路线：

- 新增 40 篇 Long-Term Storage 第一阶段 Wiki。
- 新增 8 个 Guide：DG-0895 至 DG-0902。
- 建立 Guide-Wiki 双向 evidence chain。
- 不新增 Retrieval profile。
- 不修改 Retrieval Pipeline / Prompt / top_k / selector limit / ranking / fallback。
- Field Test 后再进入 Root Cause Review。

第一批覆盖：

|主题|Wiki|Guide|
|---|---:|---:|
|储藏区分区 / 标签 / FIFO / 潮湿虫鼠检查|6|DG-0895|
|食物长期储藏|6|DG-0896|
|种子储藏|5|DG-0897|
|医疗物资储藏|5|DG-0898|
|工具与材料储藏|6|DG-0899|
|能源 / 燃料储藏|5|DG-0900|
|可疑 / 污染 / 过期隔离|4|DG-0901|
|记录 / 复查 / 交接|4|DG-0902|

Field Test 设计方向：

1. 米面有霉味还能不能继续放主库。
2. 罐头外壳锈了怎么处理。
3. 开封干粮怎么标记和先用。
4. 种子袋受潮要不要混回种子盒。
5. 留种和食用种子放一起会怎样。
6. 药品过期了是否还能留在药箱。
7. 绷带外包装潮了是否还能放急救包。
8. 工具生锈和木材发霉怎么分区。
9. 储藏架开始弯曲还能不能继续堆。
10. 电池漏液旁边的物品怎么隔离。
11. 火柴受潮和蜡烛靠近火源怎么处理。
12. 可疑批次标签最少写什么。
13. 储藏区交接需要说哪些异常。
14. 库存卡和实物对不上怎么办。
15. 材料池和长期储藏区如何分开。

延后到 Long-Term Storage v0.2：

- 年度储藏计划。
- 多地点分散储藏策略。
- 大规模粮食仓储。
- 复杂药品稳定性判断。
- 冷链系统。
- 团队资源分配算法。
- 现代库存软件或条码系统。
- 专门 Food Storage Deepening。

## 8. 不建议投入方向

本批不建议做：

- 冷链系统。
- 大型仓库工程。
- 专业药品替代判断。
- 过期药品可用性深度判断。
- 危险化学品储藏。
- 大型燃料库。
- 城市供应链管理。
- 现代库存软件依赖。
- 塑料熔融、金属冶炼、危险化学回收。
- 通过扩大 top_k / selector limit 掩盖 Long-Term Storage evidence 缺失。

## 9. 结论

Long-Term Storage 当前不是“空白”，而是“分散”：Food、Agriculture、Medical、Waste、Manufacturing、Energy 都有局部储藏知识和行动入口。但这些入口服务各自领域，尚未形成长期自持所需的统一储藏链。

建议进入：

**Batch12-C：Long-Term Storage Knowledge Apply**

推荐范围：新增 40 篇 Wiki、8 个 Guide，先建立基础 evidence chain，不提前改 Retrieval/profile。完成后进入 Field Test，再按 Root Cause Review 判断是否需要 storage query profile。
