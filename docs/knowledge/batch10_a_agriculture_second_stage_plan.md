# LanternBox Batch10-A: Agriculture Second Stage Coverage Planning

本报告用于规划 Agriculture Second Stage 第一阶段扩容。当前阶段只做覆盖分析与路线规划，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval、Prompt、profile、schema、PocketBase 或测试。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_4.md`
- `docs/knowledge/planting_retrieval_field_test_results.json`
- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

## 1. 当前覆盖审查

### 1.1 实际目录

项目中未发现 `wiki_import/agriculture/` 或 `data/guides/agriculture/`。当前农业、种植、食物生产相关内容实际分布在：

|类型|目录|数量|
|---|---|---:|
|Wiki|`wiki_import/planting`|53|
|Wiki|`wiki_import/food`|35|
|Wiki|`wiki_import/livestock`|22|
|Wiki|`wiki_import/wild_food`|24|
|Guide|`data/guides/planting`|12|
|Guide|`data/guides/food`|21|
|Guide|`data/guides/livestock`|5|
|Guide|`data/guides/wild_food`|5|

统计口径为 agriculture / planting / food production 相关目录：

|项目|数量|
|---|---:|
|相关 Wiki 总数|134|
|相关 Guide 总数|43|
|Guide -> Wiki related_wiki 边|312|
|无 guide_links 的相关 Wiki|6|
|无 related_wiki 的相关 Guide|6|

### 1.2 当前 stable 范围

Batch9-H / Coverage Map v0.4 将 Planting / Food Production 标记为 stable。现有 Field Test 结果：

|指标|结果|
|---|---:|
|Field Test pass / partial / fail|10 / 0 / 0|
|Guide hit|4 / 10|
|Guide-Wiki precise combo|9 / 10|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|

这说明 Planting v0.1 的安全边界和 evidence 基础稳定，但行动 Guide 命中率仍偏低。后续 Agriculture Second Stage 需要先建立更明确的长期生产行动入口，再进入 Retrieval profile 修复。

### 1.3 已有强项

当前已有较好基础的主题：

- 种子干燥、封存、发芽率小样测试。
- 基础育苗、移栽、缓苗检查。
- 小菜园选址、容器排水、雨后复查。
- 少水灌溉、土面覆盖、低水量浇灌排序。
- 堆肥成熟风险、草木灰禁用边界。
- 病害隔离、工具分流、病株残体分流。
- 污染地块禁用记录、粪污与食用种植区隔离。
- 采收成熟判断、采收后分级与防霉隔离。

### 1.4 明显缺口

当前不足主要集中在“长期连续生产”而非“能种一次”：

- 种子库缺少批次化管理、复测周期、失效判断和食用/留种边界。
- 育苗缺少连续排期、失败复盘、苗期病害隔离、移栽前硬化。
- 土壤恢复仍偏索引化，缺少贫瘠、板结、盐分、积水后的连续恢复流程。
- 堆肥已有成熟风险，但缺少未腐熟肥停止线、厨余/粪肥进入种植区的运行边界。
- 病虫害已有早期观察和人工除虫，但缺少周期巡查、虫卵/幼虫发现、工具带病害转移的记录链。
- 轮作只有基础条目，缺少作物家族、高耗肥/低耗肥、快慢作物组合、多季计划。
- 收获后处理已有防霉隔离，但缺少晾晒记录、干燥确认、容器复查、鼠虫防护、留种与食用批次分离。
- 小规模粮食作物、产量记录、水肥预算、任务交接仍不足。

### 1.5 有 Wiki 但无行动入口的主题

以下 Wiki 当前没有 `guide_links`，后续应优先建立精准 Guide 入口，不应批量硬关联：

|Wiki|title|建议|
|---|---|---|
|`agriculture-local-crop-profile-001`|本地作物行动档案|纳入多季种植计划或小规模粮食优先级|
|`agriculture-growing-record-handover-001`|种植记录交接复述|纳入年度计划 / 团队交接|
|`agriculture-crop-rotation-basic-001`|小地块作物轮作排布|纳入轮作 Guide|
|`agriculture-trellis-support-check-001`|支架与攀援物受力检查|暂缓，可归入 Agriculture v0.2 或 Manufacturing 协同|
|`agriculture-seedling-thinning-001`|幼苗间苗与保留株选择|纳入连续育苗 Guide|
|`agriculture-soil-texture-field-check-001`|土壤质地手感判断|纳入土壤恢复 Guide|

### 1.6 有行动入口但 evidence 不足的 Guide

以下 Guide 当前 `related_wiki` 为空：

|Guide|title|risk_level|建议|
|---|---|---|---|
|DG-0166|宠物走失预防|caution|不属于 Agriculture Second Stage，暂不处理|
|DG-0167|宠物受伤处理|caution|不属于 Agriculture Second Stage，暂不处理|
|DG-0168|家禽饮水和粪便管理|caution|后续 Livestock 专项处理|
|DG-0169|不明动物接触原则|caution|后续 Livestock / Safety 专项处理|
|DG-0228|容器种植排水|caution|可在后续精准补链，但不是 Batch10-A 核心|
|DG-0545|口粮管理：只剩三天食物时的分配|caution|Food rationing，不属于 Agriculture Second Stage|

## 2. 能力边界定义

### 2.1 Planting v0.1

Planting v0.1 解决的是“资源有限时能否安全种一点东西”：

- 选地和污染避让。
- 播种、育苗基础、移栽。
- 浇水、排水、雨后复查。
- 病害隔离和工具分流。
- 采收判断和初步防霉。
- 种植失败复盘。

### 2.2 Agriculture Second Stage

Agriculture Second Stage 解决的是“小规模长期生产循环能否持续”：

- 种子保存、复测、留种批次管理。
- 连续育苗、苗期病害隔离、育苗失败复盘。
- 土壤恢复、堆肥成熟判断、未腐熟肥停止线。
- 病虫害周期巡查、工具带病害分流、污染地块停用。
- 轮作计划、多季种植、快慢作物组合。
- 收获后晾晒、防霉、留种、储藏和鼠虫防护。
- 小规模粮食作物优先级、产量记录、水肥预算、经验交接。

### 2.3 与相邻领域的 Retrieval 边界

|相邻领域|应由 Agriculture 主导的情况|相邻领域主导的情况|
|---|---|---|
|Food|采收后干燥、留种、储藏前批次处理|已经要吃、已经霉变、食物中毒风险|
|Hygiene / WASH|堆肥成熟、粪肥与食用地块隔离、工具分流|人类排泄物处理、洗手、病人隔离|
|Contamination|污染地块停用和种植区标记|未知化学暴露、人身污染、污染区清理|
|Water|作物灌溉排序、积水后根系风险|饮水、水源、洪水路线或水质判断|
|Manufacturing|晾晒架、容器、支架作为农业任务配套|制作结构、承重、材料加工本身|
|Tools / Repair|带病害工具分流、种植工具使用边界|工具损坏、维修、替代工具制作|
|Medical|作业人员接触污染后观察|人身症状、伤口感染、中毒处理|

## 3. 主题缺口分析

### 3.1 P0 种子与育苗

现状：已有种子保存、留种、发芽测试、育苗失败原因、移栽检查。

缺口：

- 缺少种子批次台账、复测周期和失效判定。
- 缺少种子干燥后复查和潮湿失败停止线。
- 缺少留种与食用之间的保底边界。
- 连续育苗排期不足，无法支撑多季生产。
- 苗期病害隔离、间苗、移栽前硬化仍缺行动入口。

状态：Yellow。已有基础，但还没有形成稳定连续育苗链。

### 3.2 P0 土壤与肥力

现状：已有土壤污染、排水下渗、肥力索引、土壤质地手感、草木灰边界、堆肥成熟风险。

缺口：

- 贫瘠、板结、盐分、积水后的判断和恢复流程不足。
- 未腐熟堆肥进入种植区的停止线不足。
- 土壤恢复记录、水肥预算和多季对比不足。
- 草木灰、厨余、粪肥的使用边界仍需更明确的 action chain。

状态：Yellow / Red。基础概念存在，但长期恢复能力不足。

### 3.3 P0 病虫害与污染

现状：已有病虫害早期观察、人工除虫、病株隔离、工具分流、污染地块禁用。

缺口：

- 缺少虫卵/幼虫巡查、复查周期、处理记录。
- 缺少工具、手套、容器携带病害的跨区停止线。
- 缺少不明化学污染地块的农业停用流程。
- 厨余、粪肥和种植区的运行边界仍需补强。

状态：Yellow。安全边界存在，但周期管理不足。

### 3.4 P1 轮作与生产计划

现状：已有季节滚动计划、叶菜和根菜周期差异、轮作基础条目。

缺口：

- 作物家族、重耗肥/轻耗肥、快慢作物搭配不足。
- 小规模粮食作物优先级不足。
- 失败后备用计划不足。
- 本地作物行动档案有 Wiki 但无 Guide 入口。

状态：Red / Yellow。当前还难以支撑长期粮食生产规划。

### 3.5 P1 收获后处理

现状：已有采收成熟判断、采收后防霉隔离和 Food 侧霉变丢弃。

缺口：

- 晾晒架检查、干燥记录、批次标记不足。
- 留种批次与食用批次分离不足。
- 储藏容器干净/干燥检查不足。
- 鼠虫防护、根茎类短期熟化、防潮复查不足。

状态：Yellow。已有安全底线，但长期储粮能力不足。

### 3.6 P2 小规模农业系统

现状：已有种植记录字段、库存联动、季节计划。

缺口：

- 年度菜地计划、产量记录、种子库索引、水肥预算、任务交接不足。
- 团队分工和经验传承仍偏 v2.0 组织能力。

状态：Red。需要小批量补底座，但不宜先做复杂农业管理系统。

## 4. 新增 Wiki 规划

建议 Batch10-B 第一阶段规划 40 篇 Wiki，优先 P0/P1，少量 P2。以下均为规划，不在本阶段创建。

|#|slug|title|priority|risk_level|summary|intended Guide relation|Field Test|
|---:|---|---|---|---|---|---|---|
|1|`agriculture-seed-batch-viability-ledger-001`|种子批次与发芽率复测台账|P0|caution|记录来源、年份、复测结果和是否保留。|DG-0879|是|
|2|`agriculture-seed-drying-recheck-001`|种子干燥后的复查|P0|caution|封存前复查潮气、霉味、结块和容器干燥。|DG-0879|是|
|3|`agriculture-seed-storage-moisture-failure-001`|种子受潮失效判断|P0|high|发现霉味、结块或虫害时停止继续混入种子库。|DG-0879|是|
|4|`agriculture-seed-reserve-use-line-001`|留种和食用种子的保底线|P0|caution|缺粮时判断哪些种子不能轻易吃掉。|DG-0879 / DG-0886|是|
|5|`agriculture-continuous-nursery-schedule-001`|连续育苗排期|P0|caution|按采收周期安排下一批育苗，避免生产断档。|DG-0880|是|
|6|`agriculture-seedling-damping-off-isolation-001`|苗期猝倒和隔离|P0|high|发现倒伏、腐烂、扩散时隔离苗盘和工具。|DG-0880 / DG-0883|是|
|7|`agriculture-transplant-hardening-off-001`|移栽前炼苗和缓苗风险|P0|caution|移栽前逐步适应光照、风和水分变化。|DG-0880|是|
|8|`agriculture-nursery-capacity-limit-001`|育苗数量和照护上限|P0|caution|按水、光、容器和照护人力限制育苗规模。|DG-0880|否|
|9|`agriculture-soil-poor-fertility-signs-001`|土壤贫瘠的现场信号|P0|caution|通过长势、叶色、板结和产量变化判断肥力不足。|DG-0881|是|
|10|`agriculture-compost-maturity-second-check-001`|堆肥成熟二次确认|P0|high|施用前确认气味、温度、形态和未腐熟风险。|DG-0882|是|
|11|`agriculture-immature-compost-stop-line-001`|未腐熟肥进入食用区停止线|P0|high|未腐熟、发臭、发热或来源不明时禁止进入食用种植区。|DG-0882|是|
|12|`agriculture-soil-compaction-recovery-001`|土壤板结恢复|P0|caution|通过浅松、覆盖、有机质和休耕恢复板结地块。|DG-0881|是|
|13|`agriculture-salinity-crust-stop-line-001`|土表盐壳和盐分风险|P0|high|发现白壳、灼伤和长期盐水风险时停止盲目施肥。|DG-0881|是|
|14|`agriculture-raised-bed-drainage-recovery-001`|积水地块的高畦恢复|P0|caution|排水差时用高畦、沟渠和暂停浇水降低烂根风险。|DG-0881|是|
|15|`agriculture-soil-recovery-log-001`|土壤恢复记录|P0|caution|记录地块问题、处理措施、复查日期和作物反应。|DG-0881|否|
|16|`agriculture-pest-early-scouting-routine-001`|虫害早期巡查流程|P0|caution|固定检查叶背、嫩芽、土面和受害扩散范围。|DG-0883|是|
|17|`agriculture-egg-larvae-leaf-back-check-001`|叶背虫卵和幼虫检查|P0|caution|在虫害扩散前发现卵、幼虫和取食痕迹。|DG-0883|是|
|18|`agriculture-manual-pest-removal-record-001`|人工除虫后的复查记录|P0|caution|记录处理区域、数量、复发时间和是否扩大隔离。|DG-0883|是|
|19|`agriculture-diseased-tool-zone-separation-001`|带病害工具分区|P0|high|病株接触过的剪刀、手套和容器不跨区使用。|DG-0883|是|
|20|`agriculture-unknown-chemical-plot-stop-001`|不明化学污染地块停用|P0|high|闻到刺鼻味、看见残留或来源不明时停止种植食用作物。|DG-0883|是|
|21|`agriculture-manure-compost-food-zone-boundary-001`|粪肥与食用地块边界|P0|high|粪肥必须经过成熟判断和分区管理，不能直接靠近可食部位。|DG-0882 / DG-0883|是|
|22|`agriculture-kitchen-waste-compost-boundary-001`|厨余堆肥进入种植区边界|P0|high|含油、肉、病人残渣或腐败异常的厨余不进入食用区。|DG-0882|是|
|23|`agriculture-crop-family-rotation-card-001`|作物家族轮作卡|P1|normal|按作物家族记录上一季和下一季安排。|DG-0884|是|
|24|`agriculture-heavy-light-feeder-rotation-001`|高耗肥和低耗肥作物轮作|P1|normal|避免连续重耗肥作物耗尽同一地块。|DG-0884|是|
|25|`agriculture-quick-leaf-long-cycle-balance-001`|快收叶菜和长周期作物搭配|P1|normal|同时安排短期补给和长期产量。|DG-0884 / DG-0886|是|
|26|`agriculture-seasonal-planting-calendar-001`|多季种植日历|P1|normal|按季节、温度和作物周期安排连续生产。|DG-0884|是|
|27|`agriculture-staple-crop-small-plot-priority-001`|小地块粮食作物优先级|P1|caution|在有限地块中评估粮食、蔬菜和留种价值。|DG-0886|是|
|28|`agriculture-production-failure-backup-plan-001`|生产失败备用计划|P1|caution|为干旱、病虫害、种子失败准备替代作物和补种窗口。|DG-0884 / DG-0886|是|
|29|`agriculture-harvest-drying-rack-check-001`|收获晾晒架检查|P1|caution|晾晒前检查架面清洁、通风、防雨和防鼠虫。|DG-0885|是|
|30|`agriculture-postharvest-drydown-record-001`|收获后干燥记录|P1|caution|记录晾晒日期、天气、翻动和是否返潮。|DG-0885|是|
|31|`agriculture-seed-food-harvest-separation-001`|留种批次与食用批次分离|P1|caution|采收后立即标记留种、食用、待观察批次。|DG-0879 / DG-0885|是|
|32|`agriculture-storage-container-dry-clean-check-001`|储藏容器干燥清洁检查|P1|caution|入库前检查容器潮气、异味、虫害和旧残渣。|DG-0885|是|
|33|`agriculture-rodent-insect-storage-barrier-001`|储粮鼠虫防护|P1|caution|使用离地、封闭、巡查和批次隔离降低损失。|DG-0885|是|
|34|`agriculture-mold-batch-discard-line-001`|霉变批次丢弃边界|P1|high|发现霉味、结块、变色或扩散时隔离并避免混批。|DG-0885|是|
|35|`agriculture-root-crop-curing-basic-001`|根茎作物短期熟化和储藏|P1|normal|采后阴干、伤口检查和分批储藏。|DG-0885|否|
|36|`agriculture-annual-garden-plan-card-001`|家庭菜地年度计划卡|P2|normal|汇总地块、季节、作物、种子和目标产量。|DG-0884|否|
|37|`agriculture-yield-record-minimum-fields-001`|产量记录最小字段|P2|normal|记录地块、作物、投入、收获量和失败原因。|DG-0884 / DG-0886|否|
|38|`agriculture-seed-library-box-index-001`|小型种子库盒索引|P2|normal|按作物、年份、批次和复测日期管理种子盒。|DG-0879|否|
|39|`agriculture-water-fertility-budget-card-001`|水肥预算卡|P2|caution|把可用水、堆肥、覆盖物和优先作物对应起来。|DG-0881 / DG-0886|否|
|40|`agriculture-team-garden-task-handover-001`|菜地任务交接|P2|normal|交接浇水、除虫、隔离、采收和复查任务。|DG-0884|否|

## 5. Guide 候选

以下为候选 Guide，不在本阶段创建。ID 从当前稳定批次之后顺延，实际 Apply 前需再次检查 ID 是否冲突。

### DG-0879 种子保存与发芽率复测

- scenario：留种、获赠种子或旧种子需要判断是否还能保留、播种或作为食物消耗。
- steps：分批标记来源和年份；检查干燥、霉味、虫害；做小样发芽率复测；按结果分为留种、优先播种、淘汰；更新种子库记录。
- check：批次标签、复测日期、发芽率、潮湿迹象、是否混批。
- stop_or_escalate：霉味、虫害、结块、来源不明污染、种子库大面积受潮。
- fallback：没有标准容器时使用干燥小包分批封存；手写记录；保留最适合本地的小批次核心种子。
- related_wiki：`agriculture-seed-batch-viability-ledger-001`, `agriculture-seed-drying-recheck-001`, `agriculture-seed-storage-moisture-failure-001`, `agriculture-seed-reserve-use-line-001`, `agriculture-seed-library-box-index-001`。

### DG-0880 连续育苗与失败复盘

- scenario：需要连续补种、育苗失败、移栽后缓苗差或苗盘出现病害。
- steps：确认目标作物和下一次采收窗口；检查种子、土、容器、水和光；分批育苗；发现病苗立即隔离；记录失败原因并调整下一批。
- check：播种日期、出苗率、苗盘密度、病苗位置、移栽前状态。
- stop_or_escalate：苗盘扩散性腐烂、根部黑腐、连续两批失败、水源或土壤疑似污染。
- fallback：缩小育苗规模；优先快收作物；保留一盘对照小样。
- related_wiki：`agriculture-continuous-nursery-schedule-001`, `agriculture-seedling-damping-off-isolation-001`, `agriculture-transplant-hardening-off-001`, `agriculture-nursery-capacity-limit-001`, `agriculture-seedling-thinning-001`。

### DG-0881 土壤贫瘠与板结恢复判断

- scenario：作物长势差、土壤板结、积水、盐壳或多年连续种植后产量下降。
- steps：观察长势和土表；做排水和手感检查；判断贫瘠、板结、积水或盐分；选择覆盖、浅松、高畦、休耕或补充成熟堆肥；记录复查。
- check：土壤质地、下渗、作物反应、处理日期、下一次复查。
- stop_or_escalate：不明化学气味、盐壳明显、长期积水烂根、施肥后灼伤扩大。
- fallback：改种耐受作物；缩小种植面积；转移到容器或高畦。
- related_wiki：`agriculture-soil-poor-fertility-signs-001`, `agriculture-soil-compaction-recovery-001`, `agriculture-salinity-crust-stop-line-001`, `agriculture-raised-bed-drainage-recovery-001`, `agriculture-soil-recovery-log-001`, `agriculture-soil-texture-field-check-001`。

### DG-0882 堆肥成熟和未腐熟风险判断

- scenario：准备把堆肥、厨余堆肥或粪肥相关材料用于食用种植区。
- steps：确认来源；检查气味、温度、形态和是否仍在腐败；判断成熟；未成熟则继续隔离；成熟后也按食用区边界少量使用并记录。
- check：来源、成熟状态、接触作物、施用位置、复查日期。
- stop_or_escalate：发臭、发热、未分解、含病人残渣、含未知化学物、粪肥未成熟。
- fallback：延长堆置；只用于非食用植物或继续隔离；使用安全覆盖材料替代。
- related_wiki：`agriculture-compost-maturity-second-check-001`, `agriculture-immature-compost-stop-line-001`, `agriculture-manure-compost-food-zone-boundary-001`, `agriculture-kitchen-waste-compost-boundary-001`。

### DG-0883 病虫害早期隔离与工具分流

- scenario：叶片出现虫卵、幼虫、斑点、腐烂、病株或工具可能带病害跨区。
- steps：停止跨区操作；标记问题地块；检查叶背和扩散范围；人工清除或隔离病株；分流工具和手套；记录复查。
- check：受影响作物、虫害阶段、病株数量、使用过的工具、是否跨区。
- stop_or_escalate：扩散迅速、未知化学污染、整盘苗病害、工具已经接触多个区域。
- fallback：缩小种植区；优先保护未感染地块；使用单独工具或临时标记工具。
- related_wiki：`agriculture-pest-early-scouting-routine-001`, `agriculture-egg-larvae-leaf-back-check-001`, `agriculture-manual-pest-removal-record-001`, `agriculture-diseased-tool-zone-separation-001`, `agriculture-unknown-chemical-plot-stop-001`。

### DG-0884 轮作和多季种植计划

- scenario：同一小地块要连续生产，需要安排下一季作物、轮作、补种和失败备用。
- steps：列出地块和上一季作物；按作物家族和耗肥程度分组；安排快收和长周期作物；保留补种窗口；记录失败备用计划。
- check：上一季作物、下一季作物、播种窗口、预计采收、备用作物。
- stop_or_escalate：连续病害、土壤明显衰退、种子不足、水肥预算不足。
- fallback：减少作物种类；优先短周期叶菜和保底粮食；保留休耕或恢复地块。
- related_wiki：`agriculture-crop-family-rotation-card-001`, `agriculture-heavy-light-feeder-rotation-001`, `agriculture-quick-leaf-long-cycle-balance-001`, `agriculture-seasonal-planting-calendar-001`, `agriculture-production-failure-backup-plan-001`, `agriculture-annual-garden-plan-card-001`, `agriculture-growing-record-handover-001`。

### DG-0885 收获后晾晒防霉与储藏

- scenario：采收后需要晾晒、分级、留种、入容器储藏或发现返潮霉变。
- steps：分离食用、留种和待观察批次；检查晾晒位置；记录干燥过程；入库前检查容器；定期检查鼠虫和霉变。
- check：批次、晾晒日期、天气、容器状态、霉味、虫害、鼠害。
- stop_or_escalate：霉味、结块、变色、虫害扩散、容器返潮、与病害作物混批。
- fallback：重新晾晒可疑批次；小批量分开储藏；弃用明显霉变批次。
- related_wiki：`agriculture-harvest-drying-rack-check-001`, `agriculture-postharvest-drydown-record-001`, `agriculture-seed-food-harvest-separation-001`, `agriculture-storage-container-dry-clean-check-001`, `agriculture-rodent-insect-storage-barrier-001`, `agriculture-mold-batch-discard-line-001`, `agriculture-root-crop-curing-basic-001`。

### DG-0886 小规模粮食生产优先级

- scenario：地块、水、种子和人力有限，需要判断先种什么、保留什么种子、失败时怎么调整。
- steps：盘点地块、水、种子和现有口粮；区分快收补给和长周期粮食；保留核心种子；安排小样试种；记录产量和失败原因。
- check：种子数量、作物周期、预计热量或食用价值、水肥需求、失败备用。
- stop_or_escalate：种子库即将耗尽、连续失败、污染地块、口粮不足以等到长周期作物成熟。
- fallback：缩小粮食试种规模；优先短周期作物补缺；保留本地适应性种子。
- related_wiki：`agriculture-staple-crop-small-plot-priority-001`, `agriculture-seed-reserve-use-line-001`, `agriculture-quick-leaf-long-cycle-balance-001`, `agriculture-production-failure-backup-plan-001`, `agriculture-yield-record-minimum-fields-001`, `agriculture-water-fertility-budget-card-001`, `agriculture-local-crop-profile-001`。

## 6. Retrieval 风险预测

|场景|可能抢主位领域|判断|
|---|---|---|
|“粮食有霉味还能不能吃？”|Food / Medical|Food safety 应主导；Agriculture 只补充收获后批次隔离和储藏记录。|
|“收获的豆子怎么晒干避免发霉？”|Food / Storage|Agriculture 应主导，因为目标是采收后处理和长期生产保存。|
|“厨余堆肥能不能倒菜地？”|Hygiene / WASH / Contamination|Agriculture 应主导成熟判断和食用地块边界；WASH 补充污染风险。|
|“粪肥能不能直接用？”|Hygiene / Contamination|高风险场景，Agriculture Guide 必须有未腐熟停止线，Contamination 可作为 secondary。|
|“土壤积水怎么办？”|Water / Shelter|若目标是作物根系和地块恢复，Agriculture 主导；若是洪水、饮水或住所，其他领域主导。|
|“工具剪过病株还能剪别的菜吗？”|Tools / Repair / Hygiene|Agriculture 应主导病害分流；Tools 只补充工具状态。|
|“晾晒架怎么做？”|Manufacturing|制作结构由 Manufacturing 主导；农业只说明晾晒质量、通风、防雨和防鼠虫要求。|
|“虫害怎么处理？”|General safety / Food|Agriculture 应主导早期巡查、人工清除、隔离和记录。|

预计 Batch10-B 后 Field Test 可能出现的 root cause：

- Agriculture Guide 存在但被 Food safety Guide 抢主位。
- 堆肥、粪肥 query 被 Hygiene / Contamination 完全吸走。
- 晾晒架、储藏容器 query 被 Manufacturing 主导，但农业 evidence 未进入。
- 土壤积水 query 被 Water 泛化。
- 作物病害工具分流 query 被 Tools / Repair 泛化。

因此 Batch10-B 不建议提前新增 profile。应先建立 Wiki + Guide + evidence chain，再通过 Field Test 判断是否需要 Agriculture Second Stage profile。

## 7. Batch10-B Apply 建议

建议进入 Batch10-B：Agriculture Second Stage Knowledge Apply。

推荐范围：

- 新增 38-40 篇 Wiki，优先采用本报告规划的 P0/P1 条目。
- 新增 6-8 个 Guide。若要控制范围，优先 6 个：
  - 种子保存与发芽率复测
  - 连续育苗与失败复盘
  - 土壤贫瘠与板结恢复判断
  - 堆肥成熟和未腐熟风险判断
  - 病虫害早期隔离与工具分流
  - 收获后晾晒防霉与储藏
- 轮作和小规模粮食生产可以同步加入；若 Batch10-B 控制规模，则放到 Batch10-B2。
- 只建立精准 Guide-Wiki 双向 evidence chain。
- 不修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking 或 fallback。
- Apply 后进入 Batch10-C Agriculture Field Test。

Field Test 草案方向：

1. 旧种子发芽率低是否还要大面积播种。
2. 种子受潮有霉味能不能混回种子库。
3. 连续育苗怎么避免断档。
4. 苗盘出现倒伏腐烂怎么办。
5. 土壤板结、积水、白壳如何判断。
6. 未腐熟厨余堆肥能否用在叶菜旁。
7. 粪肥能不能直接进食用种植区。
8. 叶背发现虫卵怎么办。
9. 剪过病株的工具能否继续使用。
10. 同一小地块下一季种什么。
11. 收获后如何晾晒防霉。
12. 留种和食用批次怎么分开。
13. 储藏容器潮湿还能不能装粮。
14. 小地块先种叶菜还是粮食作物。
15. 生产失败后下一批怎么调整。

延后到 Agriculture v0.2：

- 更复杂的种子交换网络。
- 高级育种和品种改良。
- 家禽、畜牧与粪肥闭环。
- 大型灌溉系统。
- 温室工程。
- 商业化产量管理。
- 农业团队排班系统。

## 8. 不建议投入方向

当前不建议投入：

- 农药配方。
- 化学灭虫剂自制。
- 高风险化学处理。
- 大型灌溉系统。
- 商业温室工程。
- 大型农机和机械化农业。
- 高复杂畜牧。
- 高复杂育种。
- 食品加工工业化。
- 依赖现代供应链的农业投入品方案。

原因：这些方向成本高、风险边界复杂，且短期内不如种子、土壤、病虫害、轮作和收获后处理更能提升 LanternBox v1.5 的小规模自持生产能力。

## 结论

Agriculture Second Stage 应作为 Batch10-B 的优先建设方向。现有 Planting v0.1 已能支撑基础种植和安全边界，但尚不足以支撑连续育苗、种子库、土壤恢复、病虫害周期管理、轮作和收获后长期储藏。建议 Batch10-B 先新增 38-40 篇 Wiki 与 6-8 个行动 Guide，完成 evidence chain 后再进入 Field Test，不提前修改 Retrieval。
