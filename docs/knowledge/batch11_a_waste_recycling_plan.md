# LanternBox Batch11-A: Waste / Recycling Coverage Planning

生成日期：2026-07-18

本阶段只做覆盖分析与规划报告。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或 tests。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_4.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`

## 1. 当前覆盖审查

### 1.1 目录状态

当前项目没有独立 `wiki_import/waste`、`wiki_import/recycling`、`data/guides/waste` 或 `data/guides/recycling` 目录。Waste / Recycling 相关知识分散在 WASH / Hygiene、Contamination、Manufacturing、Repair、Fire、Planting 中。

|目录|相关 Wiki|有 guide_links|无 guide_links|判断|
|---|---:|---:|---:|---|
|`wiki_import/waste`|0|0|0|不存在独立 Waste Wiki 目录|
|`wiki_import/recycling`|0|0|0|不存在独立 Recycling Wiki 目录|
|`wiki_import/contamination`|1|1|0|只覆盖污染垃圾封存|
|`wiki_import/hygiene`|8|8|0|覆盖湿垃圾、干湿分离、垃圾异味、桶厕等卫生视角|
|`wiki_import/manufacturing`|16|15|1|覆盖废旧材料、塑料容器、金属废料进入制造前判断|
|`wiki_import/repair`|42|42|0|覆盖拆解、回收、材料修补和锐边风险，但多为 repair 视角|
|`wiki_import/fire`|1|1|0|覆盖热灰/余火处理，Fire 主导|
|`wiki_import/planting`|14|14|0|覆盖厨余、堆肥、病害残体、草木灰、农业污染边界|

严格按标题、slug、summary、tags 统计，当前相关 Wiki 约 82 篇，但其中大多数不是 Waste / Recycling 主入口，而是相邻领域的局部能力。

### 1.2 Guide 覆盖

|目录|相关 Guide|有 related_wiki|无 related_wiki|related_wiki 边|
|---|---:|---:|---:|---:|
|`data/guides/waste`|0|0|0|0|
|`data/guides/recycling`|0|0|0|0|
|`data/guides/contamination`|1|1|0|计入总边|
|`data/guides/hygiene`|17|6|11|计入总边|
|`data/guides/manufacturing`|3|3|0|计入总边|
|`data/guides/repair`|2|2|0|计入总边|
|`data/guides/fire`|1|1|0|计入总边|
|`data/guides/planting`|4|4|0|计入总边|
|合计|28|17|11|149|

无 `related_wiki` 的相关旧 Guide：

|Guide|title|判断|
|---|---|---|
|DG-0351|厨余气味控制|旧 hygiene 入口，缺 Waste 资源循环证据链|
|DG-0347|工具噪声控制|与 Waste 关联弱，不应硬修|
|DG-0350|烟雾外泄检查|Fire / security 边界，不应硬修|
|DG-0341|黑暗中锐器管控|可作为锐器安全参考，但不是 Waste 主入口|
|DG-0177|长期垃圾点维护|后续可由 Waste 体系重建，不建议硬补旧链|
|DG-0086|垃圾分区：普通、污染、尖锐|概念接近 Batch11-B，但旧 Guide 无 evidence，需要新入口替代或后续谨慎修链|
|DG-0090|蚊虫和苍蝇控制|Hygiene 主导|
|DG-0550|湿垃圾和食物残渣：密封、干湿分离与防虫处理|可作为后续参考，但缺 evidence|
|DG-0355|垃圾痕迹隐藏|Security / concealment，不应作为本批资源循环核心|
|DG-0483|饮食前公共区复核|Food / hygiene 场景|
|DG-0344|黑暗中人员清点|不属于 Waste 主线|

### 1.3 已有强项

- WASH / Hygiene 已覆盖垃圾异味、湿垃圾、污染垃圾袋渗漏、桶厕、洗手和污染区标记。
- Contamination 已覆盖污染垃圾二次密封、污染区、清理顺序和来源追溯。
- Manufacturing 已稳定，覆盖废旧材料、金属废料、塑料容器、拆解零件、材料降级和成品复查。
- Fire 已稳定覆盖灰烬与余火处理，热灰不得进普通垃圾的停止线明确。
- Agriculture Second Stage 已稳定覆盖厨余堆肥、未腐熟肥、粪肥、草木灰和食用地块边界。

### 1.4 明显缺口

当前缺的是 Waste / Recycling 自身的生命周期链：

废弃物产生  
-> 分类  
-> 临时隔离  
-> 污染边界判断  
-> 可再利用材料入池  
-> 不可用材料封存/远离生活区  
-> 与 Manufacturing / Agriculture / WASH / Fire 交接  
-> 记录与复查

## 2. 能力边界定义

### 2.1 Waste / Recycling 主导

Waste / Recycling 应主导：

- 废弃物基础分类：普通、湿垃圾、污染、尖锐、热源后、可再利用材料。
- 可再利用材料池：木板、金属片、塑料桶、布料、绳子、容器、小零件。
- 厨余、灰烬、炭渣、碎玻璃、金属边角、塑料、布料分流。
- 污染物临时隔离、标记、来源记录和远离生活区。
- 材料进入 Manufacturing 前的入池检查、降级标签、清洁等待和禁用线。
- 不可用材料的标记、封存、远离饮水/食物/睡眠区。

### 2.2 相邻领域主导

|领域|主导范围|与 Waste / Recycling 的接口|
|---|---|---|
|WASH / Hygiene|人体卫生、病人污染物、排泄物、洗手、清洁分区|Waste 接收“已封存/已标记/不可混入”的污染垃圾，不替代照护和清洁流程|
|Manufacturing|材料加工、制作结构件、成品质量检查|Waste 提供“材料池入库前检查”，Manufacturing 负责加工和承重|
|Agriculture|堆肥成熟、厨余/灰烬/有机物能否进入土壤或食用地块|Waste 做厨余和冷灰分流，Agriculture 判断入土和堆肥成熟|
|Fire|余火、热灰、复燃、火源安全|热灰和余火仍由 Fire 主导；Waste 只接收已冷却确认的灰烬/炭渣|
|Medical|割伤、污染暴露、病人症状|Waste 处理物品和区域分流，Medical 处理人体伤害和症状|
|Repair / Tools|修复已有物品、工具损坏、拆解安全|Waste 负责废物分类和材料池，Repair 负责修复或拆解动作|
|Food|入口食物安全、腐败食物能否食用|Waste 接收不可入口食物，Food 判断是否可食|

核心边界：Waste / Recycling 不是“清洁人”和“制造工”的替代，而是把废弃物和可用材料送到正确下一站的分流系统。

## 3. 主题缺口分析

### P0 废弃物安全分流

|主题|当前覆盖|缺口|
|---|---|---|
|锋利物|黑暗锐器、金属锐边、工具安全有点状覆盖|缺“碎玻璃/金属边角作为垃圾”封存容器、标记、儿童远离|
|玻璃碎片|几乎无独立覆盖|缺清扫、硬容器封存、不可混入普通袋|
|金属边角|Manufacturing 有金属锐边和废料判断|缺进入材料池或废弃区前的分流入口|
|电池 / 漏液物|Energy / Medical 有漏液处理|缺“漏液物作为废弃物”二次隔离和不得回收边界|
|病人垃圾|WASH 有病人用品隔离|缺病人垃圾进入污染物桶和记录交接|
|不明化学污染物|Contamination / Agriculture 有污染判断|缺未知物品封存、来源记录、不可清洗再利用|
|热灰 / 余火|Fire DG-0851 已强|缺冷灰进入 Waste / Agriculture 的后续分流|
|厨余腐败物|Hygiene / Agriculture 有基础|缺湿垃圾、堆肥候选、不可堆肥三分流|

### P0 污染与隔离

现有 WASH 强在污染区和清洁区，但 Waste 缺固定“污染物临时桶 / 标签 / 来源 / 禁混”体系。需要补：

- 污染物临时桶设置。
- 标记和来源记录。
- 远离饮水、食物、睡眠区。
- 混合垃圾禁止线。
- 儿童和旁人远离。
- 清洁等待区与不可再用区。

### P1 材料再利用池

Manufacturing 已有材料再利用前判断，但材料池本身没有体系。缺：

- 材料池分区：木材、金属、塑料、布料/绳索、容器、小零件、待清洁、禁用。
- 入池检查：干燥、异味、污染、锐边、裂纹、承重风险。
- 降级标签：不可承重、不可饮水、只做临时遮挡、只做样件。
- 材料池台账：来源、状态、用途限制、复查日期。

### P1 厨余 / 有机物分流

Agriculture 判断堆肥成熟和入土边界，但 Waste 需要在厨余产生端分流：

- 可堆肥厨余。
- 不可堆肥或暂缓厨余。
- 病人剩饭。
- 油脂、肉类、异味厨余。
- 堆肥等待桶和农业交接。
- 防虫、防鼠、防渗漏。

### P1 灰烬 / 炭渣 / 火源后废弃物

Fire 负责热灰；Waste 负责冷却后：

- 冷灰是否保存。
- 灰烬混入普通垃圾风险。
- 冷灰进入土壤需转交 Agriculture 判断。
- 炭渣是否可再用。
- 记录火源来源，避免把疑似污染燃料灰混入农业。

### P2 资源循环记录

当前记录分散在 WASH、Manufacturing、Agriculture。Waste 需要轻量记录：

- 材料池台账。
- 废弃物来源记录。
- 可用材料等级。
- 回收失败记录。
- 团队交接。
- 每日废物点巡查。

## 4. 新增 Wiki 规划

建议 Batch11-B 第一批规划 36 篇 Wiki：P0 12 篇，P1 20 篇，P2 4 篇。分类只使用现有正式分类，不新增 category。

|slug|title|category|priority|risk_level|summary|intended Guide relation|Field Test|
|---|---|---|---|---|---|---|---|
|`waste-basic-sorting-isolation-001`|废弃物基础分类与临时隔离|污染控制 / 隔离 / 清洁分区|P0|high|把普通、湿、污染、尖锐、热源后和可再利用材料分开，避免混合风险扩大|DG-0887|是|
|`waste-mixed-trash-stop-line-001`|混合垃圾禁止线|污染控制 / 隔离 / 清洁分区|P0|high|发现污染、尖锐、热灰、食物残渣混在一起时停止继续翻拣并重新隔离|DG-0887|是|
|`waste-source-label-minimum-001`|废弃物来源最小标签|信息保存与长期重建|P0|caution|记录来源、时间、风险类型和处理人，支撑后续追溯|DG-0887, DG-0894|是|
|`waste-child-access-control-001`|儿童远离废弃物和材料池|污染控制 / 隔离 / 清洁分区|P0|high|把儿童和旁人从尖锐、污染、热源后废弃物和待检材料旁移开|DG-0887, DG-0888|是|
|`waste-sharp-glass-temporary-container-001`|碎玻璃和锋利物临时容器|污染控制 / 隔离 / 清洁分区|P0|high|用硬容器、可见标记和禁压边界处理碎玻璃、刀片、钉子等锐物|DG-0888|是|
|`waste-metal-edge-scrap-isolation-001`|金属边角废料隔离|污染控制 / 隔离 / 清洁分区|P0|high|把锋利金属边角先隔离包边，再判断丢弃或进材料池|DG-0888, DG-0892|是|
|`waste-battery-leakage-waste-boundary-001`|电池漏液废弃物边界|污染控制 / 隔离 / 清洁分区|P0|high|电池漏液、沾污包装和接触物不得混入普通材料池|DG-0887, DG-0889|是|
|`waste-unknown-chemical-item-hold-001`|不明化学污染物暂存|污染控制 / 隔离 / 清洁分区|P0|high|对来源不明、刺鼻味、油污或粉末污染物进行暂存、标记和禁用|DG-0887, DG-0889|是|
|`waste-patient-trash-double-bag-zone-001`|病人垃圾双层封存区|污染控制 / 隔离 / 清洁分区|P0|high|病人纸巾、杯子残留、呕吐相关垃圾进入专用污染物区|DG-0889|是|
|`waste-food-rot-wet-waste-isolation-001`|腐败食物和湿垃圾隔离|卫生|P0|high|对有臭味、渗液、虫害的湿垃圾先密封并远离厨房和饮水区|DG-0887, DG-0890|是|
|`waste-hot-ash-not-trash-001`|热灰不得进入垃圾袋|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|确认热灰、火星、热炭仍由 Fire 主导，未冷却前不进入 Waste 流程|DG-0891|是|
|`waste-contaminated-container-downgrade-001`|污染容器降级和禁用|污染控制 / 隔离 / 清洁分区|P0|high|有化学味、病人污染、腐败渗漏的容器不得用于饮水或食物|DG-0893|是|
|`recycling-material-pool-zone-layout-001`|可再利用材料池分区|基础制造与材料维修|P1|caution|建立木材、金属、塑料、布料、容器、小件、待清洁和禁用区|DG-0892|是|
|`recycling-material-intake-checklist-001`|材料入池前检查清单|基础制造与材料维修|P1|caution|检查干燥、异味、污染、裂纹、锐边、虫害和用途限制|DG-0892|是|
|`recycling-salvaged-wood-intake-check-001`|废木板入池判断|基础制造与材料维修|P1|caution|废木板按腐朽、潮湿、裂纹、钉子和污染分级|DG-0892|是|
|`recycling-metal-sheet-intake-check-001`|金属片入池判断|基础制造与材料维修|P1|high|金属片先看锈蚀、锐边、油污、变形和打孔风险|DG-0892, DG-0888|是|
|`recycling-plastic-container-intake-check-001`|塑料容器入池判断|基础制造与材料维修|P1|caution|塑料桶按来源、异味、裂纹、脆化和用途降级判断|DG-0893|是|
|`recycling-fabric-rope-intake-check-001`|布料和绳索入池判断|基础制造与材料维修|P1|caution|布料和绳索按霉味、潮湿、污染、磨损、承重限制分级|DG-0892|是|
|`recycling-fasteners-small-parts-sort-001`|旧螺丝钉和小零件分类|维修 / 制作 / 替代 / 拆解再利用|P1|normal|小零件按锐利、锈蚀、尺寸、可复用和禁用分盒|DG-0892, DG-0894|否|
|`recycling-cleanable-vs-noncleanable-material-001`|可清洁材料和不可再用材料分界|污染控制 / 隔离 / 清洁分区|P1|high|判断哪些材料可清洁等待，哪些必须禁用封存|DG-0892|是|
|`recycling-material-downgrade-label-001`|材料降级用途标签|基础制造与材料维修|P1|caution|标记不可承重、不可饮水、只做遮挡、待复查等用途限制|DG-0892, DG-0894|是|
|`waste-kitchen-scrap-sorting-before-compost-001`|厨余进堆肥前分拣|种植与食物生产|P1|high|把可堆肥、暂缓、不可堆肥和病人剩饭分开交给 Agriculture 判断|DG-0890|是|
|`waste-patient-leftover-no-compost-001`|病人剩饭不直接进堆肥|污染控制 / 隔离 / 清洁分区|P1|high|病人剩饭、呕吐相关残留和照护垃圾先按污染物处理|DG-0889, DG-0890|是|
|`waste-oil-meat-odor-organic-boundary-001`|油脂肉类和异味厨余边界|卫生|P1|caution|油脂、肉类和强异味厨余先密封，避免虫鼠和臭味扩散|DG-0890|是|
|`waste-compost-waiting-bin-distance-001`|堆肥等待桶距离边界|种植与食物生产|P1|caution|堆肥等待桶远离饮水、厨房、睡眠区和儿童活动区|DG-0890|是|
|`waste-organic-pest-odor-daily-check-001`|有机废弃物虫害异味日查|卫生|P1|caution|每日检查渗漏、苍蝇、鼠迹、臭味和封盖状态|DG-0890, DG-0894|是|
|`waste-cold-ash-storage-boundary-001`|冷灰保存和丢弃边界|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|冷却确认后的灰烬按来源、污染、用途和封存分流|DG-0891|是|
|`waste-charcoal-residue-reuse-check-001`|炭渣再利用前检查|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|炭渣按是否完全冷却、是否受污染、是否可再燃判断|DG-0891|是|
|`waste-ash-soil-use-agriculture-interface-001`|灰烬进入土壤前转交判断|种植与食物生产|P1|high|灰烬是否入土必须转交 Agriculture，污染燃料灰不得进入食用地块|DG-0891|是|
|`waste-ash-trash-mixing-ban-001`|灰烬混入普通垃圾风险|污染控制 / 隔离 / 清洁分区|P1|high|冷灰也需防扬尘、防残热、防塑料袋破裂和误混|DG-0891|是|
|`waste-temporary-overflow-plan-001`|废弃物临时满溢处理|污染控制 / 隔离 / 清洁分区|P1|high|垃圾点满溢时优先隔离污染、尖锐和湿垃圾，不扩大生活区污染|DG-0887|是|
|`waste-recycling-batch-quarantine-001`|可疑回收材料批次隔离|污染控制 / 隔离 / 清洁分区|P1|caution|来源不明或混入污染的材料整批隔离，避免进入材料池|DG-0892|是|
|`recycling-material-pool-ledger-001`|材料池台账最小字段|信息保存与长期重建|P2|normal|记录来源、类别、状态、用途限制、入池人和复查时间|DG-0894|是|
|`waste-source-and-hazard-log-001`|废弃物来源和风险记录|信息保存与长期重建|P2|normal|记录污染、尖锐、热源后、厨余和不可再用材料来源|DG-0894|是|
|`waste-disposal-handover-card-001`|废弃物处理交接卡|信息保存与长期重建|P2|normal|交接垃圾点状态、满溢风险、待清理区和不可触碰物|DG-0894|是|
|`waste-reuse-failure-record-001`|回收再利用失败记录|信息保存与长期重建|P2|normal|记录材料为何失败、降级或禁用，避免重复误用|DG-0894, DG-0892|否|
|`waste-zone-daily-round-check-001`|废弃物和材料池每日巡查|污染控制 / 隔离 / 清洁分区|P2|caution|每日检查封盖、异味、虫鼠、儿童接近、标签脱落和渗漏|DG-0894, DG-0887|是|

## 5. Guide 候选

建议最多新增 8 个 Guide，ID 从当前 Batch10 后顺延候选为 DG-0887 至 DG-0894。Guide 必须是行动入口，不是百科解释。

### DG-0887 废弃物基础分类与临时隔离

scenario：断供或小团队居住点产生混合垃圾、湿垃圾、污染物、尖锐物、可再利用材料，需要先分类和临时隔离。

steps：

1. 停止把新垃圾继续倒入混合堆。
2. 按普通、湿垃圾、污染、尖锐、热源后、可再利用材料分区。
3. 先隔离污染、尖锐、疑似热源后的高风险物。
4. 给每区加可见标记和禁止儿童靠近边界。
5. 记录来源、时间和处理人。

check：分类清楚；高风险物未混入普通垃圾；饮水/食物/睡眠区远离；标签可见；有复查时间。

stop_or_escalate：发现热灰、刺鼻化学味、血液/呕吐物/粪便、玻璃散落、儿童无法隔离、垃圾袋渗漏时停止普通分类，转入专门流程。

fallback：无专用桶时用临时硬容器、纸板标记、绳线隔离；优先处理污染和尖锐物。

related_wiki：`waste-basic-sorting-isolation-001`, `waste-mixed-trash-stop-line-001`, `waste-source-label-minimum-001`, `waste-child-access-control-001`, `waste-temporary-overflow-plan-001`, `waste-zone-daily-round-check-001`

### DG-0888 锋利 / 破碎 / 金属边角废物处理

scenario：碎玻璃、钉子、刀片、金属边角、破裂塑料片或锋利废料出现在生活区、工坊或垃圾点。

steps：

1. 清空旁人和儿童。
2. 不用软袋直接装尖锐物。
3. 使用硬容器或临时包覆收集。
4. 明确标记“尖锐/不可压/不可翻”。
5. 金属边角先包边或隔离，再判断丢弃或入材料池。

check：无散落锐物；容器不易刺穿；标签清楚；处理人知道位置；工坊和生活区地面复查。

stop_or_escalate：有人受伤、玻璃进入食物/睡眠区、金属边带污染、没有硬容器、照明不足时停止继续清理并扩大隔离。

fallback：无硬容器时用多层纸板/布料临时包覆并固定角落隔离，等待白天或工具到位。

related_wiki：`waste-sharp-glass-temporary-container-001`, `waste-metal-edge-scrap-isolation-001`, `waste-child-access-control-001`, `recycling-metal-sheet-intake-check-001`

### DG-0889 病人垃圾与污染物分流

scenario：病人纸巾、剩饭、杯子残留、呕吐物接触物、血液/体液污染物或不明污染物接近普通垃圾、厨房或材料池。

steps：

1. 停止继续混入普通垃圾。
2. 标记接触位置和污染来源。
3. 病人垃圾进入专用污染物区或双层封存。
4. 与厨房、饮水、睡眠、材料池分开。
5. 完成照护者手卫生并记录恢复时间。

check：病人垃圾专区存在；未进入厨余堆肥；未进入材料池；污染范围有记录；照护者完成清洁。

stop_or_escalate：多人呕吐腹泻、接触饮水口、血液大量污染、袋体渗漏、不明化学味时转 WASH / Medical / Contamination 主导。

fallback：空间不足时固定角落隔离，使用明显标记和时间分区处理。

related_wiki：`waste-patient-trash-double-bag-zone-001`, `waste-patient-leftover-no-compost-001`, `waste-unknown-chemical-item-hold-001`, `waste-battery-leakage-waste-boundary-001`, `waste-contaminated-container-downgrade-001`

### DG-0890 厨余和有机物进入堆肥前判断

scenario：厨房产生菜叶、果皮、剩饭、油脂、肉类、病人剩饭或发臭湿垃圾，需要判断能否进入堆肥等待区。

steps：

1. 先分开可堆肥、暂缓、不可堆肥和污染来源。
2. 病人剩饭、呕吐相关残留先转污染物处理。
3. 油脂、肉类、强异味和虫鼠污染先密封，不直接进食用地块相关堆肥。
4. 可堆肥厨余进入等待桶，并远离水、食物、睡眠区。
5. 记录异常臭味、虫害和转交 Agriculture 时间。

check：厨余无病人污染；等待桶有盖；距离合格；无渗漏；农业交接明确。

stop_or_escalate：发臭扩散、虫鼠明显、渗液、混入病人剩饭或不明化学污染时停止进入堆肥链。

fallback：堆肥条件不足时小量密封、干湿分离、增加巡查，不扩大堆肥规模。

related_wiki：`waste-kitchen-scrap-sorting-before-compost-001`, `waste-patient-leftover-no-compost-001`, `waste-oil-meat-odor-organic-boundary-001`, `waste-compost-waiting-bin-distance-001`, `waste-organic-pest-odor-daily-check-001`

### DG-0891 灰烬与炭渣冷却后分流

scenario：火源结束后灰烬和炭渣已由 Fire 判断冷却，需要决定封存、丢弃、保留或转交 Agriculture。

steps：

1. 先确认无烟、无火星、无热感，否则返回 DG-0851。
2. 按来源分开：干净木灰、混合燃料灰、疑似污染燃料灰、炭渣。
3. 冷灰不直接混入软塑料袋或湿垃圾。
4. 想进入土壤前转交 Agriculture 判断。
5. 记录来源、冷却确认人和分流去向。

check：Fire 冷却确认完成；灰烬来源清楚；未混入普通垃圾；农业用途有二次判断；有记录。

stop_or_escalate：仍热、扬灰严重、混入塑料/化学燃料、来源不明、儿童接近时停止分流。

fallback：不确定来源或用途时封存标记，按不可入土材料处理。

related_wiki：`waste-hot-ash-not-trash-001`, `waste-cold-ash-storage-boundary-001`, `waste-charcoal-residue-reuse-check-001`, `waste-ash-soil-use-agriculture-interface-001`, `waste-ash-trash-mixing-ban-001`, `fire-ash-ember-cooling-disposal-001`

### DG-0892 可再利用材料进入材料池前检查

scenario：废木板、金属片、塑料桶、布料、绳子、容器、小零件等可能用于制造或维修，需要决定是否入池、降级或禁用。

steps：

1. 先排除污染、异味、渗漏、病人接触和不明化学来源。
2. 按材料类别分区。
3. 检查锐边、裂纹、潮湿、腐朽、锈蚀、脆化和承重限制。
4. 标记用途等级：可加工、待清洁、降级、禁用。
5. 入材料池台账并交给 Manufacturing 判断加工方式。

check：材料池分区清楚；用途等级清楚；污染材料未入池；锐边已隔离；来源可追踪。

stop_or_escalate：不明污染、化学味、严重锐边、儿童无法隔离、材料来源混乱时停止入池。

fallback：不确定材料单独隔离，作为待复查批次，不进入主材料池。

related_wiki：`recycling-material-pool-zone-layout-001`, `recycling-material-intake-checklist-001`, `recycling-salvaged-wood-intake-check-001`, `recycling-metal-sheet-intake-check-001`, `recycling-fabric-rope-intake-check-001`, `recycling-cleanable-vs-noncleanable-material-001`, `recycling-material-downgrade-label-001`, `waste-recycling-batch-quarantine-001`

### DG-0893 塑料桶 / 容器再利用前判断

scenario：塑料桶、瓶、盒、破裂容器或有异味容器想用于储水、食物、工具、厨余或材料池。

steps：

1. 先确认原用途和是否有化学味、油味、腐败味或病人污染。
2. 饮水和食物用途采用最高边界，不明来源直接禁用。
3. 检查裂纹、脆化、渗漏、盖子密封和承重。
4. 按用途降级：饮水/食物、工具、厨余、不可用。
5. 贴用途标签并记录。

check：不明容器未进入饮水食物链；容器用途标签清楚；裂纹容器减载或禁用；污染容器未入材料池。

stop_or_escalate：化学味、漏液、电池接触、病人污染、无法清洁、儿童误触风险时停用。

fallback：无法判断来源时只做外部非接触用途，或封存为不可用。

related_wiki：`recycling-plastic-container-intake-check-001`, `waste-contaminated-container-downgrade-001`, `recycling-cleanable-vs-noncleanable-material-001`, `recycling-material-downgrade-label-001`

### DG-0894 废弃物与材料池记录交接

scenario：多人轮值处理垃圾点、污染物区、厨余等待桶、材料池和不可用材料，需要交接状态和复查任务。

steps：

1. 记录当天新增废弃物来源和风险类型。
2. 更新材料池入库、降级、禁用和待复查批次。
3. 标记满溢、异味、虫鼠、渗漏、标签脱落。
4. 交接下一次处理时间和负责人。
5. 对异常区做口头复述和可见标记。

check：有来源记录；高风险区有复查时间；材料池台账更新；未处理异常有人接手。

stop_or_escalate：污染物无人接手、标签丢失、材料池混乱、儿童接近、废弃物满溢进入生活区时暂停新入库并先恢复分区。

fallback：无表格时使用纸条/墙面编号，先记地点、风险、时间、处理人四项。

related_wiki：`recycling-material-pool-ledger-001`, `waste-source-and-hazard-log-001`, `waste-disposal-handover-card-001`, `waste-reuse-failure-record-001`, `waste-zone-daily-round-check-001`, `waste-source-label-minimum-001`

## 6. Retrieval 风险预测

|冲突|典型 query|预期边界|风险|
|---|---|---|---|
|WASH / Hygiene|病人剩饭怎么处理？垃圾有臭味怎么办？|病人照护和人体清洁由 WASH 主导；废弃物流向和临时隔离由 Waste 主导|Hygiene 旧 Guide 较多，可能抢走 Waste 主入口|
|Contamination|不明化学味物品还能不能留？|人体接触、污染清理由 Contamination 主导；物品是否入材料池由 Waste 主导|化学污染词会强触发 Contamination / Medical|
|Manufacturing|废木板能不能用？塑料桶能不能改工具盒？|材料入池前由 Waste；加工制作由 Manufacturing|Manufacturing v0.1 stable，容易成为材料再利用 query top1|
|Agriculture|厨余能不能堆肥？灰烬能不能撒菜地？|产生端分流由 Waste；入土、堆肥成熟、食用地块由 Agriculture|Agriculture Second Stage profile 已强，可能完全抢主位|
|Fire|热灰怎么丢？炭渣还能不能留？|热灰/余火由 Fire；冷却后分流由 Waste|Fire DG-0851 应保留热灰主导|
|Medical|碎玻璃扎伤怎么办？污染物碰到皮肤怎么办？|人体伤害由 Medical；废弃物封存由 Waste|伤害 query 不应强行 Waste 主导|
|Repair / Tools|拆旧物、旧螺丝钉怎么留？|拆解动作由 Repair；分类入池和台账由 Waste|Repair 的“拆解再利用”覆盖广，可能抢 Waste|
|Food|腐败食物还能吃吗？|入口判断由 Food；丢弃、湿垃圾和堆肥前分流由 Waste|Food safety 应保留入口安全主位|

后续 Field Test 应重点观察：

1. “厨余能不能堆肥”是否被 Agriculture 完全抢主位。
2. “病人剩饭怎么处理”是否被 Hygiene 完全抢主位。
3. “热灰怎么丢”是否被 Fire 完全抢主位，且热灰未冷却时 Fire 主导是否保持。
4. “废木板能不能用”是否被 Manufacturing 完全抢主位。
5. “碎玻璃 / 金属边角”是否被 Medical / Safety 抢主位。
6. “电池漏液垃圾”是否被 Energy / Medical 抢主位。

Batch11-B 不建议提前新增 retrieval profile。先建立 Waste / Recycling 知识和 Guide-Wiki evidence chain，再由 Field Test 判断是否需要 `waste_sorting`, `recycling_material_pool`, `waste_contamination_boundary`, `waste_organic_compost_interface`, `waste_ash_cold_disposal` 等 profile。

## 7. Batch11-B Apply 建议

建议 Batch11-B：

- 新增 34-36 篇 Wiki。
- 新增 6-8 个 Guide。
- 建立精准 Guide-Wiki 双向关系。
- 更新 `guide_index` / `emergency_guides`。
- 同步 PocketBase wiki_articles，如当前项目流程要求。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不新增 query profile。
- 不修改 top_k、selector limit、ranking 或 fallback。

建议第一批优先级：

1. 先建 P0 废弃物分类、混合垃圾停止线、污染/尖锐/热源后隔离。
2. 再建 P1 材料池入库、厨余分流、冷灰分流。
3. 最后建 P2 记录和交接。

Field Test 设计方向：

- strict 12-16 cases：垃圾分类、碎玻璃、金属边角、电池漏液、病人剩饭、厨余堆肥、热灰、冷灰、废木板入池、塑料桶复用、材料池台账、垃圾点满溢。
- observation 6-8 cases：与 WASH、Manufacturing、Agriculture、Fire、Food、Medical、Repair 的边界竞争。
- 目标：fail=0、dangerous suggestion=0、Kiwix 越权=0、safety/fallback/record-check=100%。

延后到 Waste / Recycling v0.2：

- 长期垃圾点设计。
- 团队轮值清运系统。
- 大量材料池库存优化。
- 与 Organization / Resource Allocation 的跨域资源账。
- 更细的废水、灰水、粪污和农业循环接口。
- 地图化污染点和材料点管理。

## 8. 不建议投入方向

当前不做：

- 危险化学品处理教程。
- 电池拆解再造。
- 塑料熔融加工。
- 金属冶炼。
- 专业医疗废弃物处理替代。
- 大型垃圾处理系统。
- 现代城市环卫依赖流程。
- 焚烧垃圾教程。
- 把病人污染物、化学污染物或电池漏液物“清洗后回收”的高风险建议。

原因：这些方向风险高、依赖条件复杂，且短期不能提升 LanternBox v1.5 的低资源资源循环能力。

## 9. 结论

Waste / Recycling 当前不是完全空白，而是“分散覆盖、缺主入口、缺生命周期链”。WASH、Manufacturing、Agriculture、Fire 都已有稳定相邻能力，但它们无法替代废弃物与可再利用材料的统一分流系统。

建议进入 Batch11-B：Waste / Recycling Foundation Apply。第一批只建立基础 Wiki + 少量 Guide，不提前修 Retrieval。Batch11-C 再进行 Field Test，之后根据真实失败进入 Root Cause Review。
