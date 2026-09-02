# LanternBox Batch13-B: Production System / Workshop Coverage Planning

生成日期：2026-08-24

本阶段只做覆盖分析与规划报告。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、tests、`.env`、`data/lanternbox.db` 或无关 dirty 文件；未运行生产 Apply。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_6.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`
- `docs/knowledge/batch11_f_waste_recycling_final_verification.md`
- `docs/knowledge/batch12_g_long_term_storage_final_verification.md`

核心判断：Production System / Workshop 不是继续扩 Manufacturing。它的第一阶段目标是把 Manufacturing Foundation、Waste / Recycling、Long-Term Storage、Agriculture Second Stage 串成可重复运行的小规模生产流程：今天做什么、用哪些原料、谁来做、做到哪一步、半成品放哪里、成品怎么检查、失败怎么复盘、成品交给 Storage 还是 Repair / Waste、下一班如何接手。

## 1. 当前覆盖审查

指令中的 `wiki_import/energy` 和 `data/guides/energy` 在项目实际结构中对应 `power`；`organization` 对应 `team`，并参考 `security` / `risk_decision`；`wiki_import/safety` 存在但 `data/guides/safety` 不存在，安全 Guide 分散在 `security`、`tools`、`power`、`fire` 等目录。

### 1.1 目录统计

|目录|Wiki 总数|有 guide_links|无 guide_links|Guide 总数|有 related_wiki|无 related_wiki|related_wiki 边|
|---|---:|---:|---:|---:|---:|---:|---:|
|manufacturing|61|58|3|8|8|0|59|
|waste|36|36|0|8|8|0|51|
|storage|40|40|0|8|8|0|59|
|tools|4|4|0|48|24|24|66|
|repair|100|73|27|5|5|0|69|
|planting|93|92|1|20|16|4|153|
|power|74|73|1|72|35|37|153|
|safety|3|3|0|0|0|0|0|
|security|31|31|0|116|34|82|122|
|risk_decision|20|20|0|5|5|0|66|
|team|20|20|0|5|5|0|66|
|records|3|3|0|25|10|15|11|
|合计|485|453|32|320|158|162|875|

狭义 production / workshop / workflow 相关内容按“工坊、生产、半成品、成品、质量、质检、小批量、批量、工序、开工、收工、待检、返工、报废、未完成品、任务交接、任务状态、原料、材料清点、工具清点、制作记录、失败复盘、near miss”等词审查：

|口径|Wiki 数量|相关 Guide 数量|Guide-Wiki 边|无 guide_links Wiki|无 related_wiki Guide|
|---|---:|---:|---:|---:|---:|
|相邻目录全量|485|320|875|32|162|
|狭义生产 / 工坊核心|194|36|232|11|2|

### 1.2 当前已有能力

|来源|已有覆盖|可供 Production System 借用的接口|
|---|---|---|
|Manufacturing Foundation|工坊开工、材料再利用、切割固定、结构件制作、连接方式、金属处理、承重检查、收工封存。|具体制作方法、单次作业安全、成品承重前检查。|
|Waste / Recycling|废弃物分类、材料池判断、污染物隔离、材料降级、记录交接。|原料来源判断、不合格品分流、可回收材料入池。|
|Long-Term Storage|储藏分区、标签、FIFO、工具材料防潮防锈、可疑批次隔离、储藏交接。|原料 / 成品长期保存、入库标签、交接卡。|
|Agriculture Second Stage|种子、育苗、土壤、堆肥、病虫害、轮作、收获后晾晒、生产失败备用计划。|农业产出作为生产计划输入，种植任务交接作为交接范式。|
|Tools / Repair|工具借还、工具破损禁用、维修区安全、临时替代。|工具状态、维修转交、关键工具停用。|
|Records / Team|任务失败复盘、负责人交接、每日复盘、轮值交接。|生产失败复盘和班次交接模板。|

### 1.3 主要重叠点

|重叠对象|当前主导域|Production System 应新增的上层职责|
|---|---|---|
|工坊开工 / 收工|Manufacturing|从单次作业安全扩展为当日任务、材料、工具、风险和未完成品状态。|
|材料再利用|Waste / Manufacturing|从“能不能用”扩展为“领用到哪个任务、剩余多少、替代方案是什么”。|
|成品检查|Manufacturing|从承重和结构安全扩展为尺寸、功能、锐边、标签、放行、入库或返工。|
|不合格品|Manufacturing / Waste|建立返工、降级、报废、隔离和复盘入口。|
|半成品放置|Storage|Storage 负责保存条件，Production 负责 WIP 状态、下一步动作和责任人。|
|工具坏了|Tools / Repair|Repair 负责修，Production 负责停工、替代、任务重排和交接。|
|工坊交接|Records / Team|Records 负责长期归档，Production 负责班次级操作交接。|

## 2. 能力边界

Production System / Workshop 主导：

- 工坊流程。
- 生产任务排程。
- 原料 / 半成品 / 成品流转。
- 小批量重复制作。
- 工序检查。
- 质量复查。
- 生产失败复盘。
- 工坊任务交接。
- 工坊安全记录。
- 工坊开工 / 收工流程。

相邻领域边界：

|领域|主导内容|Production System / Workshop 不替代的内容|
|---|---|---|
|Manufacturing|具体制作方法、材料加工、连接方式、承重检查、结构件制作。|不教“怎么做一个架子”的具体制作步骤，只管理流程、批次、检查和交接。|
|Waste / Recycling|废弃物分类、材料池判断、不可用材料分流、污染物隔离。|不替代污染 / 废弃物危险判断，只接收材料池状态和输出不合格品分流。|
|Long-Term Storage|成品 / 原料 / 工具如何长期保存、批次标签、FIFO、储藏区交接。|不替代长期储藏规则，只定义生产中的暂存、待检和交接状态。|
|Tools / Repair|工具维修、临时替代工具、损坏判断。|不修工具，只决定停工、换工具、降级任务或转 Repair。|
|Records / Knowledge Transfer|长期经验沉淀、技能档案、跨班组知识传承。|不做长期知识库治理，只做生产当天和班次级记录。|
|Organization / Team Management|人员、技能、分工、值班、决策流程。|不管理团队制度，只记录生产任务负责人、替补和交接对象。|
|Energy / Fire / Safety|电气、火花、低照明、燃料和现场安全边界。|不替代危险判断；触发停止线时转相邻安全域。|

Production System / Workshop 只负责：**让生产过程可重复、可检查、可交接、可复盘。**

## 3. 主题缺口分析

|主题|当前覆盖|缺口|优先级|
|---|---|---|---|
|工坊开工 / 收工流程|DG-0871、DG-0878 已覆盖开工安全和收工封存。|缺材料清点、任务状态、未完成品标记、当日目标和收工断电后的交接状态。|P0|
|原料 / 半成品 / 成品流转|已有原料半成品成品分区、材料池和储藏分区。|缺领取、待加工、待检、返工、报废、成品入库、与 Waste / Storage 交接的统一流程。|P0|
|质量复查|已有承重前检查和制造质量复查。|缺尺寸、稳定性、锐边、毛刺、松动、功能检查、返工 / 报废判断和检查记录。|P0|
|小批量重复制作|已有小批量制作记录和重复锯切支撑。|缺首件确认、模板 / 尺寸标准、每件检查、批次一致性和失败批次隔离。|P1|
|工坊安全记录|已有 PPE、清场、粉尘火花停止线、儿童远离加工区。|缺 near miss 记录、低照明停止线、疲劳停止线、防护用品缺失停工和非操作人员隔离记录。|P1|
|工坊任务交接|有收工封存、负责人交接和储藏交接。|缺未完成任务、半成品位置、工具状态、风险点、下一步动作和失败原因交接。|P1|
|生产计划与资源预算|农业有产量 / 水肥预算，Energy 有每日预算，Records 有任务优先级。|缺今日任务优先级、材料预算、工具时间、能源、人力、产出目标和备用计划。|P2|

## 4. Wiki 规划

建议 Batch13-C 第一阶段新增 41 篇 Wiki。优先 P0 / P1，少量 P2。以下为规划清单，不在本阶段创建 Wiki。

|slug|title|priority|risk_level|summary|intended Guide relation|Field Test|
|---|---|---|---|---|---|---|
|production-workshop-daily-start-check-001|工坊每日开工总检查|P0|high|开工前核对区域、工具、材料、能源、风险源和当天任务，避免带病开工。|DG-0903|需要|
|production-workshop-task-board-minimum-001|工坊任务板最小字段|P0|normal|用任务、负责人、状态、材料、下一步、风险和交接对象建立离线任务板。|DG-0903, DG-0910|需要|
|production-workshop-tool-count-before-work-001|开工前工具清点|P0|high|生产前确认关键工具齐全、可用、无破损，破损时停用并转 Repair。|DG-0903|需要|
|production-workshop-material-count-before-work-001|开工前材料清点|P0|caution|按任务核对原料、替代材料、保留线和不可用材料，避免中途混用。|DG-0903, DG-0905|需要|
|production-workshop-risk-source-scan-001|工坊风险源开工扫描|P0|high|检查粉尘、火花、刃具、电气、儿童进入、低照明和潮湿风险。|DG-0903, DG-0909|需要|
|production-workshop-unfinished-item-tag-001|未完成品状态标签|P0|caution|给未完成品标记任务、步骤、负责人、风险和下一步动作。|DG-0904, DG-0910|需要|
|production-workshop-daily-stop-shutdown-001|工坊收工断电与封存|P0|high|收工时断电、封存刃具、清理碎屑、标记未完成品和危险点。|DG-0904|需要|
|production-workshop-end-of-day-status-log-001|当日任务状态记录|P0|normal|记录完成、暂停、返工、报废和交接状态，为下一班接手提供依据。|DG-0904, DG-0910|需要|
|production-workshop-raw-material-intake-card-001|原料入工坊卡|P0|caution|记录原料来源、等级、数量、风险、可用任务和保留线。|DG-0905|需要|
|production-workshop-material-reservation-line-001|生产材料保留线|P0|high|定义哪些材料不得被当日任务耗尽，避免破坏维修、储藏或农业关键需求。|DG-0905|需要|
|production-workshop-wip-zone-layout-001|半成品暂存区布局|P0|caution|区分待加工、待检、返工、隔离和暂停任务，防止半成品混入成品或储藏区。|DG-0905|需要|
|production-workshop-wip-handover-tag-001|半成品交接标签|P0|caution|半成品标签包含当前工序、下一步、负责人、风险和禁止使用说明。|DG-0905, DG-0910|需要|
|production-workshop-finished-goods-hold-zone-001|成品待检区|P0|caution|成品未完成检查前进入待检区，不直接入 Storage 或投入使用。|DG-0905, DG-0907|需要|
|production-workshop-nonconforming-item-quarantine-001|不合格品隔离区|P0|high|将开裂、松动、锐边、污染、尺寸错误或来源不明成品从主流程隔离。|DG-0908|需要|
|production-workshop-finished-goods-storage-transfer-001|成品入库转交|P0|caution|成品质检后补齐用途、限制、批次和复查日，再转 Long-Term Storage。|DG-0905, DG-0907|需要|
|production-workshop-waste-transfer-card-001|生产废料转 Waste 交接卡|P0|high|生产废料按可回收、污染、锐边、不可用和待判断分类交给 Waste / Recycling。|DG-0905, DG-0908|需要|
|production-workshop-batch-label-minimum-001|生产批次标签最小字段|P0|normal|批次标签包含任务、原料、数量、日期、负责人、检查状态和去向。|DG-0905, DG-0906|需要|
|production-workshop-size-check-sheet-001|尺寸复查表|P0|normal|对重复件、箱体、支架和替代件做尺寸复核，减少批量偏差。|DG-0907|需要|
|production-workshop-stability-check-sheet-001|稳定性复查表|P0|high|对架子、支撑、箱体和工作台做晃动、倾倒、承重前稳定性检查。|DG-0907|需要|
|production-workshop-edge-burr-check-001|锐边毛刺检查|P0|high|成品或半成品存在锐边、毛刺、突钉时不得放行，先返工或隔离。|DG-0907, DG-0908|需要|
|production-workshop-loose-fastener-check-001|松动连接复查|P0|high|检查螺丝、扎带、胶粘、捆扎、铆接和铁丝连接松动情况。|DG-0907|需要|
|production-workshop-function-check-before-release-001|成品放行前功能检查|P0|caution|确认成品符合用途、限制清楚、风险已处理，再进入使用或储藏。|DG-0907|需要|
|production-workshop-rework-decision-line-001|返工判断线|P0|caution|定义哪些缺陷可返工，哪些必须降级、报废或转 Waste。|DG-0908|需要|
|production-workshop-scrap-decision-line-001|报废判断线|P0|high|开裂承重件、污染材料、不可控锐边和来源不明高风险件不得返工放行。|DG-0908|需要|
|production-workshop-quality-record-minimum-001|质量检查记录最小字段|P1|normal|记录检查人、检查项、问题、处理、复查结果和最终去向。|DG-0907, DG-0908|需要|
|production-workshop-template-size-standard-001|模板与尺寸标准卡|P1|normal|为重复制作建立模板编号、关键尺寸、允许偏差和更新记录。|DG-0906|需要|
|production-workshop-first-article-check-001|首件确认|P1|caution|小批量制作先完成首件检查，再继续后续重复件。|DG-0906|需要|
|production-workshop-repeat-step-card-001|重复制作步骤卡|P1|normal|把重复制作拆成固定步骤，避免换人后顺序混乱。|DG-0906|需要|
|production-workshop-each-piece-check-001|每件检查规则|P1|caution|小批量每件都做关键项检查，不用抽象“看起来差不多”替代。|DG-0906, DG-0907|需要|
|production-workshop-batch-consistency-check-001|批次一致性复查|P1|normal|检查同批成品在尺寸、材料、连接、标签和用途限制上是否一致。|DG-0906|需要|
|production-workshop-failed-batch-isolation-001|失败批次隔离|P1|high|批量制作发现系统性缺陷时整批暂停，防止混入合格品。|DG-0906, DG-0908|需要|
|production-workshop-ppe-check-before-task-001|生产防护用品检查|P1|high|缺护目、手套、口罩或固定工具时，不进入切割、打磨、拆解和钻孔任务。|DG-0909|需要|
|production-workshop-dust-spark-near-miss-log-001|粉尘火花 near miss 记录|P1|high|记录火花、粉尘、碎屑飞溅和险些受伤事件，作为停工复盘依据。|DG-0909|需要|
|production-workshop-low-light-stop-line-001|低照明生产停止线|P1|high|照明不足时停止切割、钻孔、打磨、承重检查和锐器作业。|DG-0909|需要|
|production-workshop-fatigue-stop-line-001|疲劳生产停止线|P1|high|手抖、注意力下降、连续错误或争执后暂停高风险生产。|DG-0909, DG-0910|需要|
|production-workshop-bystander-exclusion-log-001|非操作人员隔离记录|P1|high|记录儿童、旁观者和无关人员离开加工区后的确认。|DG-0909|需要|
|production-workshop-incident-review-card-001|工坊伤害与 near miss 复盘卡|P1|high|事故或险情后记录原因、停工项、整改项和复工条件。|DG-0909|需要|
|production-workshop-shift-handover-card-001|工坊班次交接卡|P1|normal|交接未完成任务、工具状态、半成品位置、风险点和下一步动作。|DG-0910|需要|
|production-workshop-tool-state-handover-001|工具状态交接|P1|caution|交接可用、待修、禁用、外借和缺失工具，防止下一班误用。|DG-0910|需要|
|production-workshop-next-action-handover-001|下一步动作交接|P1|normal|用一句话写清每个未完成任务的下一步、停止条件和负责人。|DG-0910|需要|
|production-workshop-daily-resource-budget-001|当日生产资源预算|P2|normal|估算材料、工具时间、能源、人力、产出目标和备用计划。|DG-0903, DG-0910|需要|

## 5. Guide 候选

建议最多新增 8 个 Guide，候选 ID 从当前最高 DG-0902 之后顺延。以下为规划，不在本阶段创建 Guide。

### DG-0903 工坊开工前检查

- scenario：当天开始生产前，任务、工具、材料、能源和风险状态不清。
- steps：确认今日任务和负责人；检查工作区通道、照明、潮湿、粉尘和旁人；清点关键工具；清点材料和保留线；标记风险源；写入任务板。
- check：任务明确；工具可用；材料够用且未越过保留线；高风险源已处理；未授权人员不在加工区。
- stop_or_escalate：工具破损、低照明、潮湿电气、火花粉尘不可控、儿童进入或材料来源不明时停止开工。
- fallback：任务拆小；转低风险清理 / 标记任务；等待照明或工具恢复；转 Repair / Waste / Storage 判断。
- related_wiki 候选：`production-workshop-daily-start-check-001`, `production-workshop-task-board-minimum-001`, `production-workshop-tool-count-before-work-001`, `production-workshop-material-count-before-work-001`, `production-workshop-risk-source-scan-001`, `production-workshop-daily-resource-budget-001`

### DG-0904 工坊收工与未完成品封存

- scenario：当天工作结束或必须暂停，工坊内有未完成品、危险工具和未清理材料。
- steps：停止加工；断开非必要电源；清点刃具和关键工具；清理碎屑和可燃物；给未完成品贴状态标签；隔离危险件；记录当日任务状态。
- check：工具封存；未完成品不混入成品；危险材料隔离；任务状态可被下一班理解。
- stop_or_escalate：缺失刃具、未断电、可疑火花 / 气味、半成品承重不明或标签缺失时不允许离场。
- fallback：空间不足时设置临时暂停区；标签不足时用编号 + 总表；工具缺失时封闭工坊并查找。
- related_wiki 候选：`production-workshop-unfinished-item-tag-001`, `production-workshop-daily-stop-shutdown-001`, `production-workshop-end-of-day-status-log-001`, `production-workshop-wip-handover-tag-001`

### DG-0905 原料 / 半成品 / 成品流转

- scenario：原料、半成品、成品、废料和待检物混放，下一步去向不清。
- steps：为原料建入工坊卡；设置待加工、半成品、待检、返工、报废和成品转交区；每次移动都更新标签；不合格品转隔离；合格成品转 Storage。
- check：每件物品有状态；半成品不进入成品区；报废和污染物不回主流程；成品入库字段完整。
- stop_or_escalate：来源不明、污染、结构裂纹、缺标签、成品未经检查或 Waste / Storage 边界不明时停止流转。
- fallback：空间不足时用颜色 / 编号临时区；记录不足时先建总账；争议物放中立待检区。
- related_wiki 候选：`production-workshop-raw-material-intake-card-001`, `production-workshop-wip-zone-layout-001`, `production-workshop-wip-handover-tag-001`, `production-workshop-finished-goods-hold-zone-001`, `production-workshop-finished-goods-storage-transfer-001`, `production-workshop-waste-transfer-card-001`, `production-workshop-batch-label-minimum-001`

### DG-0906 小批量重复制作流程

- scenario：需要连续制作多个相似零件、箱体、支架、标签板或简单工具，但容易批量偏差。
- steps：先做首件；确认模板和关键尺寸；固定重复步骤卡；每件做关键检查；发现系统性问题时暂停整批；记录批次一致性。
- check：首件合格；模板编号清楚；每件有检查痕迹；失败批次已隔离。
- stop_or_escalate：首件不合格、模板尺寸争议、同类缺陷连续出现、材料批次混乱或疲劳操作时停止批量制作。
- fallback：降低批量规模；改为单件制作；只完成低风险部件；重新做模板。
- related_wiki 候选：`production-workshop-template-size-standard-001`, `production-workshop-first-article-check-001`, `production-workshop-repeat-step-card-001`, `production-workshop-each-piece-check-001`, `production-workshop-batch-consistency-check-001`, `production-workshop-failed-batch-isolation-001`

### DG-0907 成品质量复查

- scenario：成品准备使用或入库前，需要判断是否可放行。
- steps：核对用途和限制；复查尺寸；复查稳定性和承重前状态；检查锐边、毛刺、松动和污染；做功能检查；记录放行、返工、降级或报废。
- check：尺寸合格；无危险锐边；连接未松动；功能符合用途；去向明确。
- stop_or_escalate：承重不明、锐边无法处理、连接松动、污染、儿童可能接触或检查记录缺失时不放行。
- fallback：降级为非承重 / 非接触用途；转返工；转 Waste；临时封存待复查。
- related_wiki 候选：`production-workshop-size-check-sheet-001`, `production-workshop-stability-check-sheet-001`, `production-workshop-edge-burr-check-001`, `production-workshop-loose-fastener-check-001`, `production-workshop-function-check-before-release-001`, `production-workshop-quality-record-minimum-001`

### DG-0908 不合格品 / 返工 / 报废判断

- scenario：成品或半成品出现开裂、松动、尺寸错误、污染、锐边或功能失败。
- steps：先隔离；判断是否可返工；不可返工则降级或报废；可回收材料转 Waste；原因写入复盘；禁止混回合格区。
- check：不合格品有标签；返工条件清楚；报废理由记录；材料去向可追踪。
- stop_or_escalate：承重件裂纹、污染来源不明、锐边不可控、反复返工失败或涉及电气 / 火灾风险时停止返工。
- fallback：改作练习件；降级为低风险用途；拆解入材料池；封存待有经验者复查。
- related_wiki 候选：`production-workshop-nonconforming-item-quarantine-001`, `production-workshop-rework-decision-line-001`, `production-workshop-scrap-decision-line-001`, `production-workshop-quality-record-minimum-001`, `production-workshop-waste-transfer-card-001`

### DG-0909 工坊安全记录与 near miss 复盘

- scenario：生产中出现低照明、疲劳、粉尘、火花、工具险些伤人或旁人误入。
- steps：立即暂停相关任务；隔离风险源；记录 near miss；确认 PPE 和人员状态；决定复工、降级或停工；把整改项写入任务板。
- check：险情记录完整；风险源已处理；复工条件明确；重复风险有负责人。
- stop_or_escalate：有人受伤、火花 / 烟味、漏电、刃具失控、疲劳明显或儿童进入时停止生产并转相邻安全 / 医疗域。
- fallback：只做清理、标记、记录和低风险整理；延后高风险工序；双人复核后再复工。
- related_wiki 候选：`production-workshop-ppe-check-before-task-001`, `production-workshop-dust-spark-near-miss-log-001`, `production-workshop-low-light-stop-line-001`, `production-workshop-fatigue-stop-line-001`, `production-workshop-bystander-exclusion-log-001`, `production-workshop-incident-review-card-001`

### DG-0910 工坊任务交接

- scenario：下一班、另一个成员或第二天需要接手未完成生产任务。
- steps：列出未完成任务；说明半成品位置和状态；交接工具状态；标记风险点；写清下一步动作；交接失败原因和禁止操作；双方复述确认。
- check：任务、物品、工具、风险、下一步、负责人都可追踪。
- stop_or_escalate：半成品状态不明、工具缺失、风险点未说明、失败原因未记录或接手人无技能授权时不交接开工。
- fallback：交接给更有经验者；暂停任务；只交接清理 / 记录任务；把半成品转待检区。
- related_wiki 候选：`production-workshop-shift-handover-card-001`, `production-workshop-tool-state-handover-001`, `production-workshop-next-action-handover-001`, `production-workshop-wip-handover-tag-001`, `production-workshop-end-of-day-status-log-001`

## 6. Retrieval 风险预测

|风险问题|预期主导|容易抢主位的领域|判断|
|---|---|---|---|
|“怎么做一个架子”|Manufacturing|Production|应由 Manufacturing 主导；Production 只补任务、批次、质检和入库流程。|
|“一批成品怎么检查和入库”|Production System / Workshop|Manufacturing / Storage|Production 主导检查、放行和转交；Storage 补入库保存、标签和复查周期。|
|“半成品放哪里”|Production System / Workshop|Storage|Production 主导 WIP 暂存和下一步动作；Storage 只主导长期保存条件。|
|“不合格品怎么处理”|Production System / Workshop|Waste / Manufacturing|Production 主导返工 / 报废 / 降级决策入口；Waste 主导废料分类和污染隔离。|
|“工具坏了怎么办”|Tools / Repair|Production|Repair 主导维修；Production 只负责停工、替代、任务重排和交接。|
|“工坊交接怎么写”|Production System / Workshop|Records / Team|Production 主导班次级操作交接；Records 主导长期归档和知识沉淀。|
|“低照明还要不要继续干活”|Safety / Tools / Fire 补充，Production 触发停工|Safety / Fire / Power|生产场景下 Production 可主导停工入口，但危险判断必须保留 Safety / Fire / Power 边界。|
|“材料来自废料池能不能用”|Waste / Recycling 或 Manufacturing|Production|Waste 判断入池安全，Manufacturing 判断加工适配；Production 记录领用、用途和去向。|
|“当天任务怎么排”|Production System / Workshop|Records / Organization / Energy|Production 主导生产任务板；Organization 管人员制度，Energy 管电力预算。|

预测：Batch13-C 如果只新增 Wiki / Guide 而不加 profile，初始 Field Test 可能出现 Manufacturing、Storage、Waste、Records 抢主位。按 ROOT_CAUSE_FIX_POLICY，应先通过 Field Test 复现，再判断是否需要 Production System 专属 query profile，而不是提前扩大 top_k 或改 selector limit。

## 7. Batch13-C Apply 建议

推荐 Batch13-C：

|项目|建议|
|---|---|
|新增 Wiki|新增 41 篇，集中在开工/收工、流转、质检、小批量、安全记录、任务交接，少量生产预算。|
|新增 Guide|新增 8 个候选 Guide：DG-0903 至 DG-0910。|
|Guide-Wiki|建立每个 Guide 与 5-7 个核心 Wiki 的双向关系；保留 Manufacturing / Waste / Storage / Repair 的边界链接。|
|Retrieval / profile|先不改 Retrieval / profile / top_k / selector limit / ranking / fallback。|
|Field Test|Batch13-D 新增 Production System Field Test，覆盖严格场景和 observation 场景。|
|Root Cause|Field Test 后进入 Batch13-E Root Cause Review，再判断是否需要最小 profile。|

Field Test 设计方向：

- 严格场景：开工检查、收工封存、半成品暂存、成品待检、不合格品隔离、批量首件确认、低照明停工、交接卡。
- observation 场景：具体做架子、工具坏了、废料能不能用、成品长期放哪、工坊事故受伤、能源不足。
- 关键指标：Production Guide hit、expected Guide hit、Wiki full hit、Guide-Wiki precise、cross-domain 合理性、dangerous suggestion、Kiwix 越权、safety / fallback / record-check。

延后到 Production System v0.2：

- 跨周 / 跨季生产计划。
- 团队技能授权矩阵。
- 工坊产能估算。
- 多工坊或多地点协同。
- 复杂设备维护排程。
- 长期质量趋势分析。
- 与 Resource Allocation 的正式配给接口。

## 8. 旧 Coverage Map 文件整理建议

只给建议，不移动文件。

|文件|建议|理由|是否保留引用价值|
|---|---|---|---|
|`docs/knowledge/knowledge_coverage_map_v0_3.md`|可归档为历史草稿。|v0.3 Final 已覆盖其主要结论；保留可追踪 Batch4-7 后状态。|中等|
|`docs/knowledge/knowledge_coverage_map_v0_3_final.md`|保留。|是 Navigation / Medical / Shelter-Fire-WASH 等 stable milestone 后的基线。|高|
|`docs/knowledge/knowledge_coverage_map_v0_4.md`|保留。|记录 Manufacturing Foundation stable 后的路线判断。|高|
|`docs/knowledge/knowledge_coverage_map_v0_5.md`|保留。|记录 Agriculture Second Stage、Manufacturing、Waste stable 后识别 Long-Term Storage 缺口。|高|
|`docs/knowledge/knowledge_coverage_map_v0_6.md`|作为当前主地图保留。|纳入 Long-Term Storage stable，并把下一阶段转向 Production System / Workshop。|高|

建议暂不执行文件移动。等 Batch13-C / D / E 完成后，如果 Coverage Map 引用关系稳定，再统一归档草稿文件。

## 9. 不建议投入方向

当前不做：

- 高复杂工业制造。
- 大型工厂排产系统。
- 自动化生产线。
- 专业质量认证体系。
- 武器制造。
- 危险化学品生产。
- 高压电气作业。
- 现代 ERP / 云端库存系统。
- 城市级工业供应链管理。

这些方向要么超出 LanternBox 当前个人 / 小团队自持节点，要么风险过高，要么会引入云端依赖和大型组织假设。Batch13-C 应保持低资源、离线、可纸面执行的小工坊流程能力。

## 10. 结论

Production System / Workshop 第一阶段应新增 41 篇 Wiki 和 8 个 Guide，围绕“开工、流转、质检、批量、安全记录、交接”建立独立行动层。它不替代 Manufacturing、Waste、Storage、Repair、Records，而是在它们之间建立可重复、可检查、可交接、可复盘的生产流程。

Batch13-C 建议只做基础 Wiki + 少量 Guide Apply，不提前新增 profile。Field Test 后再进入 Root Cause Review，按真实检索失败决定是否做最小 Retrieval profile。
