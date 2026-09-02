# LanternBox Knowledge Coverage Map v0.6

生成日期：2026-08-24

本阶段只做全局知识覆盖分析和路线规划。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、测试、`.env` 或无关 dirty 文件；未运行生产 Apply。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_5.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`
- `docs/knowledge/batch11_f_waste_recycling_final_verification.md`
- `docs/knowledge/batch12_g_long_term_storage_final_verification.md`

当前系统规模：

|项目|数量 / 状态|
|---|---:|
|正式 Markdown Wiki|1066|
|Guide JSON|828|
|Wiki priority|P0=679, P1=272, P2=115|
|Wiki risk_level|high=467, caution=446, normal=153|
|Guide priority|P0=607, P1=152, P2=69|
|Guide risk_level|critical=14, high=118, caution=584, normal=112|
|有 guide_links 的 Wiki|1014 / 1066|
|有 related_wiki 的 Guide|395 / 828|
|Guide-Wiki related_wiki 边|2264|

核心判断：Batch12-G 后 Long-Term Storage Retrieval v0.1 已 stable。LanternBox 现在已经具备“个人长期生存 + 小规模自持生产的基础材料链”：Agriculture Second Stage、Manufacturing Foundation、Waste / Recycling、Long-Term Storage 均已 stable。下一阶段主要矛盾不再是继续扩这些 stable 域，而是把农业、制造、回收、储藏连接成可重复执行的生产流程系统。

## 1. Stable Domain 总览

已经 stable 的领域不建议继续扩 profile，不建议继续新增 Wiki，不建议继续微调 Retrieval。后续只有在新的 Field Test 或 Root Cause Review 证明存在真实根因缺口时，才应另开独立 Batch。

|Domain|Wiki数量|Guide数量|Retrieval状态|Field Test|当前状态|
|---|---:|---:|---|---|---|
|Shelter / Fire / WASH|158|99|v0.2 stable|Batch5-N 16 / 0 / 0|Green，冻结 v0.2|
|Medical|51|75|v0.1 stable|Batch7-F 19 / 3 / 0；fail=0|Green，冻结 v0.1|
|Navigation|51|6|v0.1 stable|Batch8-G 18 / 0 / 0|Green，冻结 v0.1|
|Communication|65|23|v0.1 stable candidate|Batch6-E 13 / 6 / 0；fail=0|Green/Yellow，冻结 v0.1 candidate|
|Energy Safety|74|72|v0.1 stable|Batch5-D 8 / 2 / 0；fail=0|Green，冻结安全异常入口|
|Energy Management|74|72|v0.1 stable candidate|Batch5-H 14 / 1 / 0；fail=0|Green/Yellow，冻结日常管理入口|
|Agriculture Second Stage|40|8|v0.1 stable|Batch10-F 24 / 0 / 0|Green，冻结 v0.1|
|Manufacturing Foundation|61|8|v0.1 stable|Batch9-G 16 / 2 / 0；fail=0|Green，冻结 v0.1|
|Waste / Recycling|36|8|v0.1 stable|Batch11-F 24 / 0 / 0|Green，冻结 v0.1|
|Long-Term Storage|40|8|v0.1 stable|Batch12-G 26 / 0 / 0|Green，冻结 v0.1|

## 2. Capability Matrix v0.6

评分标准：

- 0：无覆盖
- 1：基础知识
- 2：Wiki 覆盖
- 3：Guide 行动入口
- 4：Evidence + Retrieval stable

|能力域|Knowledge|Guide|Evidence|Retrieval|Field Test|等级|备注|
|---|---:|---:|---:|---:|---:|---|---|
|Water|4|3|3|2|1|Yellow|水源、过滤、煮沸、储水已有；缺长期水系统运行、轮换和设备维护专项 stable。|
|Food|4|3|3|3|3|Green|基础食物安全、配给和储食可行动；深层长期粮食系统可后续专项。|
|Medical|4|4|4|4|4|Green|高风险医疗入口 v0.1 stable。|
|Shelter|4|4|4|4|4|Green|与 Fire / WASH 共同达到 v0.2 stable。|
|Fire|4|4|4|4|4|Green|火灾、燃料、明火、撤离边界稳定。|
|WASH|4|4|4|4|4|Green|卫生、污染隔离和生活区安全边界稳定。|
|Agriculture|4|3|3|3|3|Green|基础种植可行动；不再作为短期 profile 扩展目标。|
|Agriculture Second Stage|4|4|4|4|4|Green|种子、育苗、土壤、堆肥、病虫害、轮作、采后处理已 stable。|
|Tools|4|4|4|4|3|Green|核心工具使用和安全入口稳定。|
|Repair|4|4|4|4|3|Green|维修核心能力稳定；与生产系统的维护计划另算。|
|Manufacturing|4|4|4|4|4|Green|基础制造、连接、承重、工坊开收工已 stable。|
|Waste Recycling|4|4|4|4|4|Green|分类、隔离、材料池、交接记录已 stable。|
|Long-Term Storage|4|4|4|4|4|Green|储藏分区、标签、FIFO、轮换、隔离和交接已 stable。|
|Communication|4|3|3|3|3|Green/Yellow|v0.1 fail=0；短波、Mesh、低功耗多设备留 v0.2。|
|Navigation|4|4|4|4|4|Green|个人定位、返回、路线风险和 track stable。|
|Energy Safety|4|4|4|4|3|Green|异常停用、隔离和安全边界稳定。|
|Energy Management|4|4|4|3|3|Green/Yellow|每日预算和关键负载可用；发电、储能、配电深化不足。|
|Production System / Workshop|3|3|3|1|1|Yellow|已有制造基础，但缺原料、半成品、成品、生产流程、质量检查和任务交接的系统入口。|
|Energy System Deepening|3|2|2|1|1|Yellow|发电、储能、配电、负载管理、能源预算和维护计划未成闭环。|
|Records / Knowledge Transfer|3|3|2|1|1|Yellow|储藏记录已增强，但生产、维修、医疗、种植经验尚未形成可传承 SOP 链。|
|Organization / Team Management|3|2|2|1|1|Yellow/Red|team 目录有基础；人员、技能、分工、值班、决策流程未 stable。|
|Resource Allocation|2|1|1|0|0|Red|水、食物、能源、材料、工具、医疗资源尚无统一配给和冲突处理入口。|
|Security / Risk Decision|3|3|2|1|1|Yellow|安全判断内容多，但与资源冲突、生产中断、团队治理未形成 stable retrieval。|
|Governance / Decision Flow|2|2|1|0|0|Red|共同决定和复盘片段存在，但缺否决线、责任边界、争议处理和执行闭环。|
|Team Training|2|2|1|0|0|Red/Yellow|有五分钟教学、技能冗余等片段；缺技能档案、训练计划、考核和禁做任务清单。|

## 3. Red / Yellow / Green

### Green：已经 stable，不继续扩展

|领域|依据|冻结意见|
|---|---|---|
|Shelter / Fire / WASH|Batch5-N 16 / 0 / 0。|冻结 v0.2，不继续追 profile partial。|
|Medical|Batch7-F fail=0，danger=0，Kiwix 越权=0。|冻结 v0.1，不进入专业医学深水区。|
|Navigation|Batch8-G 18 / 0 / 0。|冻结 v0.1。|
|Energy Safety|Batch5-D fail=0，安全链完整。|冻结异常停用和隔离入口。|
|Agriculture Second Stage|Batch10-F 24 / 0 / 0，Guide/Wiki/precise 100%。|冻结 v0.1，不继续扩 agriculture profile。|
|Manufacturing Foundation|Batch9-G fail=0，Guide hit 100%，Guide-Wiki precise 100%。|冻结 v0.1，生产系统另起层级。|
|Waste / Recycling|Batch11-F 24 / 0 / 0。|冻结 v0.1，不继续调整 Guide-Wiki 顺序。|
|Long-Term Storage|Batch12-G 26 / 0 / 0。|冻结 v0.1，不继续扩 storage profile。|

### Yellow：已有基础，需要提升系统能力

#### Production System / Workshop

|检查项|当前判断|主要缺口|
|---|---|---|
|原料|Waste / Recycling 和 Storage 已有材料池、标签和隔离。|缺原料入库、等级、可用状态、替代材料和领用规则。|
|半成品|Manufacturing Foundation 有制作过程，但不管理 WIP。|缺待加工、待检、返工、报废、暂停任务的半成品区。|
|成品|承重检查和储藏放行已有局部入口。|缺成品放行、用途标签、承载限制、复查周期和交接卡。|
|生产流程|工坊开工、连接、切割、收工已有。|缺工序卡、任务队列、批量制作节奏、样件和模板。|
|质量检查|承重检查 stable。|缺跨产品质量表、抽检、返工判定和失败件隔离。|
|失败复盘|Records 中有任务失败复盘。|缺把失败原因回填到下一版工序卡的流程。|
|工坊安全|开工/收工、儿童工具安全已有。|缺多人协作、危险区域、疲劳替换和停工线。|
|任务交接|收工工具封存和储藏交接已有。|缺跨班次生产任务交接、未完成件状态和责任人。|

#### Energy System Deepening

|检查项|当前判断|主要缺口|
|---|---|---|
|发电|太阳能板应急充电、小型发电机使用边界已有。|缺板位、阴影、发电日志、维护周期和天气降级计划。|
|储能|充电宝轮换、电池检查已有。|缺储能组分区、容量衰减复测、备用切换和充放电纪律。|
|配电|低压安全和插线板检查已有。|缺低压 DC 分区、保险/线缆标签、负载隔离和损耗记录。|
|负载管理|每日能源预算和充电优先级可用。|缺生产、通信、医疗、取水、照明的跨任务负载分配。|
|能源预算|日常预算可行动。|缺连续阴天、多设备并发、团队共享和任务降级模型。|
|维护计划|有零散设备复查。|缺控制器、电池、线缆、发电设备的固定维护表。|

#### Long-Term Records / Knowledge Transfer

|检查项|当前判断|主要缺口|
|---|---|---|
|生产记录|制造和储藏都有记录要求。|缺工序、材料、工时、缺陷、返工的统一生产记录。|
|维修记录|Tools / Repair 强。|缺工具寿命、重复故障、替换件和停用原因归档。|
|种植记录|Agriculture Second Stage 覆盖种子、育苗、土壤和收获后处理。|缺跨季节产量、失败原因、留种表现和土壤恢复版本化。|
|医疗记录|医疗记录备份和药品管理已有。|缺照护交接、物资消耗、症状变化和隐私分级的统一链。|
|储藏记录|Storage v0.1 已 stable。|可作为 Records 下一阶段模板源，但还未扩展到全域知识传承。|
|技能档案|team / records 有成员档案片段。|缺技能等级、可独立执行项、需监督项、禁做任务和训练记录。|
|经验传承|有复盘和五分钟教学片段。|缺“经验 -> SOP 候选 -> Field Test -> Wiki/Guide”的知识生产流程。|

#### Organization / Team Management

|检查项|当前判断|主要缺口|
|---|---|---|
|人员|成员档案、外出登记和照护备份已有。|缺统一成员状态、健康限制、可用时间和责任边界。|
|技能|技能冗余和短教学存在。|缺技能矩阵、替补名单、训练计划和授权边界。|
|分工|任务优先级和负责人交接已有。|缺任务看板、生产/储藏/照护/外出之间的冲突处理。|
|值班|夜间轮值和疲劳停止线已有。|缺轮换周期、替补规则、交接模板和疲劳累计记录。|
|决策流程|共同决定记录片段存在。|缺谁能否决、何时升级、如何复盘和如何执行追踪。|

#### Resource Allocation

|检查项|当前判断|主要缺口|
|---|---|---|
|水|水源、净水、储水和患者优先有基础。|缺跨成员、跨任务、跨天数的水预算。|
|食物|配给和储粮已有。|缺劳动强度、儿童老人、病人、留种和应急保底线的统一分配。|
|能源|Energy Management 有关键负载优先级。|缺与通信、医疗、取水、生产、照明联动的资源决策。|
|材料|Waste / Manufacturing / Storage 形成材料池。|缺申请、审批、替代、消耗和保留线。|
|工具|Tools / Repair 有安全线。|缺关键工具排班、损坏停用和多人冲突规则。|
|医疗资源|Medical 有高风险停止线。|缺短缺情境下的公开优先级、交接和审计记录。|

### Red：长期能力仍未形成 stable

|领域|红区判断|推进前置|
|---|---|---|
|Resource Allocation|不是完全没有知识，而是缺跨域统一入口；目前无法稳定处理水、食物、能源、材料、工具、医疗资源冲突。|需要 Storage 事实、Production 消耗和 Records 记录进一步稳定。|
|Governance / Decision Flow|团队决策、否决线、责任边界、争议处理、执行复盘仍未成链。|适合 v2.0，在 Organization / Resource Allocation 后推进。|
|Team Training|技能传承缺稳定检索入口和训练闭环。|适合与 Records / Knowledge Transfer 或 Organization 同步规划。|

## 4. v1.0 / v1.5 / v2.0 判断

### v1.0 Personal Survival Node

|核心|状态|判断|
|---|---|---|
|生存|Water、Food、Shelter、Fire、WASH 基础可行动。|基本完成。|
|医疗|Medical Retrieval v0.1 stable。|完成。|
|能源|Energy Safety stable，Energy Management candidate。|基本完成。|
|庇护|Shelter / Fire / WASH v0.2 stable。|完成。|
|通信|Communication v0.1 stable candidate。|基本完成。|
|导航|Navigation v0.1 stable。|完成。|
|基础生产|Tools / Repair、Agriculture、Manufacturing Foundation 已可行动。|完成 v1.0 所需基础。|

结论：v1.0 个人长期生存节点已稳定闭环。后续不应继续在 v1.0 Green 域追小修。

### v1.5 Small Self-Sustaining Production

当前已完成：

- Agriculture Second Stage v0.1 stable
- Manufacturing Foundation v0.1 stable
- Waste / Recycling v0.1 stable
- Long-Term Storage v0.1 stable

下一块关键缺口：**Production System / Workshop**。

理由：当前已有“产出、制造、回收、保存”的分立能力，但还没有把它们组织为可重复生产的流程：原料入库、半成品流转、成品放行、质量检查、失败复盘、工坊安全、任务交接。没有这一层，Storage 保存的是物资，Manufacturing 执行的是单次作业，Waste 提供的是材料池，Agriculture 产生的是农业产出，但系统无法稳定回答“今天做什么、用哪些材料、做多少、谁检查、失败怎么回滚、完成后如何交接”。

### v2.0 Team / Community Node

|能力|状态|判断|
|---|---|---|
|Organization|Yellow/Red|需要人员、技能、分工、值班和决策流程，但应在生产系统与记录链更清楚后推进。|
|Resource Allocation|Red|需要库存、产量、能源、材料消耗事实作为前置。|
|Knowledge Transfer|Yellow|Storage 已提供批次记录模板，但全域经验传承仍缺 SOP 化流程。|
|Governance|Red|偏 v2.0，不宜早于组织管理和资源分配。|

结论：v2.0 还不能直接跳到治理模拟。应先把 v1.5 的生产流程、能源系统、记录传承和资源分配逐步稳定。

## 5. Future Batch Roadmap

不把已经 stable 的 Agriculture Second Stage、Manufacturing Foundation、Waste / Recycling、Long-Term Storage 列为短期主要建设目标。

|Batch|方向|原因|预计收益|风险|优先级|
|---|---|---|---|---|---|
|Batch13-B|Production System / Workshop|当前最缺“原料 -> 半成品 -> 成品 -> 质检 -> 交接”的生产流程层。|把制造基础升级为可重复的小工坊生产系统。|Manufacturing / Repair / Waste / Storage 容易抢主位。|P0|
|Batch14|Energy System Deepening|生产、储藏、通信和医疗都需要更可预测的低压能源系统。|形成发电、储能、配电、负载、预算、维护闭环。|Energy Safety / Management / Repair 混淆；避免大型能源工程。|P0|
|Batch15|Records / Knowledge Transfer|Production 与 Storage 会产生高价值记录对象。|把生产、维修、种植、医疗、储藏记录转为 SOP 和训练资产。|容易变成泛泛记录模板，缺 Field Test 场景。|P1|
|Batch16|Organization / Team Management|进入 v2.0 前需要人员、技能、分工、值班、决策流程。|建立小团队日常运行和任务交接基础。|team / psychology / security / records 混淆。|P1|
|Batch17|Resource Allocation|Storage 提供库存，Production 提供消耗，Energy 提供预算后才能分配资源。|统一水、食物、能源、材料、工具、医疗资源优先级。|各领域抢主位；伦理和安全边界要清楚。|P1|
|Batch18|Security / Risk Decision|资源冲突、外出、留守、交换、撤离需要统一风险决策。|提升高压场景下的停止线和复盘能力。|不要滑向武器、对抗或社会控制。|P2|
|Batch19|Water System Deepening|v1.0 水能力强但未做长期运行专项。|补长期储水轮换、净水设备维护、灾后水系统记录。|Water / WASH / Medical / Storage 混淆。|P2|
|Batch20|Food System Deepening|Storage 已解决通用储藏，食物系统可做更长期粮食安全。|补长期储粮、粮食安全、配给与生产计划接口。|Food / Agriculture / Storage 边界要清楚。|P2|
|Batch21|Team Safety|值班、疲劳、外出、生产作业和照护都需要团队安全层。|形成轮换、替补、疲劳停止和交接安全链。|team / medical / security / communication 混淆。|P2|
|Batch22|Governance|在组织和资源分配有基础后，才处理决策流。|建立否决线、责任边界、争议处理、执行复盘。|不做大规模社会治理模拟。|P3|

## 6. 下一阶段推荐

推荐下一阶段：**Batch13-B Production System / Workshop Coverage Planning**。

建议范围：

- 原料入库、替代材料、保留线和危险材料隔离。
- 半成品区、待检区、返工区、报废区。
- 成品放行、用途标签、限制条件和复查周期。
- 工序卡、样件、模板、批量制作节奏。
- 质量检查、抽检、失败件隔离和复盘。
- 工坊安全、多人协作、停工线和任务交接。

不建议范围：

- 不新增 Manufacturing Foundation v0.1 小修。
- 不扩 Agriculture、Waste、Storage profile。
- 不做高复杂工业制造、机床、冶炼或危险化学处理。
- 不改 Retrieval Pipeline、Prompt、top_k、selector limit 或 ranking。

## 7. Production System / Workshop vs Records / Knowledge Transfer

重点比较：

|候选|当前基础|解决的问题|不足|优先级|
|---|---|---|---|---|
|Production System / Workshop|Manufacturing Foundation stable；Waste 材料池 stable；Storage 保存和标签 stable。|把原料、半成品、成品、流程、质检、失败复盘、工坊安全和交接串成生产闭环。|需要小心不重复制造基础，而是做系统层。|更优先|
|Long-Term Records / Knowledge Transfer|Records / data / team 有基础；Storage 已强化标签、FIFO 和交接记录。|把生产、维修、种植、医疗、储藏经验变成可传承 SOP。|缺生产流程对象时，记录容易空泛；没有稳定工序和质检，知识传承缺事实来源。|第二优先|

当前已有链条：

```text
Agriculture
  -> Manufacturing
  -> Waste Recycling
  -> Long-Term Storage
```

缺少的环节不是“再多一批储藏知识”，而是：

```text
Production Flow / Workshop System
```

也就是把库存和材料转成可计划、可检查、可复盘、可交接的生产活动。知识传承系统也需要做，但更适合作为 Production System 后的承接层：先产生稳定的工序、质检、失败复盘和交接记录，再把这些记录固化为 SOP、训练卡和下一版 Wiki / Guide 候选。

## 8. Coverage Map 文件整理建议

只给建议，不移动文件。

|文件|建议|理由|是否保留引用价值|
|---|---|---|---|
|`docs/knowledge/knowledge_coverage_map_v0_3.md`|归档为草稿 / 历史中间态。|v0.3 Final 已覆盖后续稳定判断；该文件仍可追踪 Batch4-7 后的早期状态。|中等|
|`docs/knowledge/knowledge_coverage_map_v0_3_final.md`|保留。|Final 版本，是 Navigation / Medical / Shelter-Fire-WASH 等 stable milestone 后的基线。|高|
|`docs/knowledge/knowledge_coverage_map_v0_4.md`|保留。|记录 Manufacturing Foundation stable 后、Agriculture/Waste/Storage 前的路线判断。|高|
|`docs/knowledge/knowledge_coverage_map_v0_5.md`|保留。|记录 Agriculture Second Stage、Manufacturing、Waste stable 后，把 Long-Term Storage 识别为下一缺口的关键 milestone。|高|
|`docs/knowledge/knowledge_coverage_map_v0_6.md`|保留为当前 active coverage map。|纳入 Long-Term Storage stable 后的新全局状态，并把下一阶段转向 Production System / Workshop。|高|

原则：final 保留，stable milestone 保留，草稿可归档，不删除仍被引用文件。

## 9. 不建议投入方向

|方向|不建议原因|
|---|---|
|高复杂工业制造|超出 v1.5 小规模自持生产边界；当前需要工坊流程，不是工业生产线。|
|大型能源工程|Energy 下一步应是低压小系统、储能、配电和维护，不是并网、高压或大型工程。|
|专业医学深水区|Medical v0.1 已 stable；深水医学风险高，不应替代专业救治。|
|危险化学品生产|风险高，且会破坏 WASH / contamination / storage 边界。|
|武器制造|高风险且偏离 LanternBox 的生存、自持、恢复目标。|
|城市基础设施|规模过大，投入产出不适合当前个人 / 小团队节点。|
|云端依赖系统|LanternBox 假设离线、断供、低资源，云端依赖不可靠。|
|大规模金融系统|不服务当前生存、生产、储藏、能源和团队运行缺口。|
|社会治理模拟|v2.0 还未到治理模拟阶段，应先建立组织、资源分配和决策流的行动基础。|

## 10. 结论

LanternBox v0.6 的状态是：v1.0 个人长期生存节点已经稳定闭环；v1.5 小规模自持生产节点已经完成四个基础稳定域：

- Agriculture Second Stage v0.1 stable
- Manufacturing Foundation v0.1 stable
- Waste / Recycling v0.1 stable
- Long-Term Storage v0.1 stable

下一阶段不建议继续扩这些 stable 域。推荐进入：

**Batch13-B：Production System / Workshop Coverage Planning**

目标是把“农业产出、制造能力、回收材料、长期储藏”连接成可重复、可质检、可复盘、可交接的小工坊生产系统。本阶段结束后不要 Apply，等待下一步 Batch13-B。
