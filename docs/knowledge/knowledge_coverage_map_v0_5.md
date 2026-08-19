# LanternBox Knowledge Coverage Map v0.5

生成日期：2026-07-18

本阶段只做全局知识覆盖分析和路线规划。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_4.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`
- `docs/knowledge/batch11_f_waste_recycling_final_verification.md`

当前系统规模：

|项目|数量 / 状态|
|---|---:|
|正式 Markdown Wiki|1026|
|Guide JSON|820|
|Wiki priority|P0=656, P1=259, P2=111|
|Wiki risk_level|high=442, caution=436, normal=148|
|Guide priority|P0=602, P1=150, P2=68|
|Guide risk_level|critical=14, high=111, caution=584, normal=111|
|有 guide_links 的 Wiki|974 / 1026|
|有 related_wiki 的 Guide|387 / 820|
|Guide-Wiki related_wiki 边|2205|

核心判断：Batch10-F 后 Agriculture Second Stage 已达到 v0.1 stable；Batch11-F 后 Waste / Recycling 已达到 v0.1 stable；Manufacturing Foundation 已在 Batch9-G 达到 v0.1 stable。LanternBox v1.5 已具备“种植深化 + 基础制造 + 资源分流”的骨架，下一阶段主要矛盾转为长期储存：把农业产出、种子、医疗物资、工具材料和可回收材料稳定保存、轮换、标记、隔离和报废。

## 1. Stable Domain 总览

目录计数采用当前实际目录。Planting / Food Production 使用 v0.4 的基础食物生产口径，不把 Batch10 的 40 篇 Agriculture Second Stage 重复计入。

|Domain|Wiki数量|Guide数量|Retrieval状态|Field Test状态|剩余风险|冻结建议|
|---|---:|---:|---|---|---|---|
|Planting / Food Production|134|43|基础种植和食物生产可行动；stable candidate|Planting 10 / 0 / 0；Guide hit 偏低但 fail=0|基础种植不再是短板；不要把长期农业问题塞回 v0.1|冻结基础种植入口，后续只作为 Agriculture / Storage 接口|
|Agriculture Second Stage|40|8|v0.1 stable|Batch10-F 24 / 0 / 0；Guide-Wiki precise 100%|后续 v0.2 可做更长周期产量和种子库，但不在短期小修|冻结 v0.1，不继续扩 agriculture profile|
|Tools / Repair|104|53|Tools / Repair v0.1 stable；Wave2 stable candidate|v0.1 10 / 0 / 0；Wave2 16 / 4 / 0|现场安全 partial 与生产系统交界，不阻塞核心 repair|冻结核心维修入口|
|Manufacturing Foundation|61|8|v0.1 stable|Batch9-G 16 / 2 / 0；Guide hit 100%；Guide-Wiki precise 100%|剩余 partial 是 top8 evidence 截断，不是能力缺失|冻结 v0.1，不继续扩 manufacturing profile|
|Waste / Recycling|36|8|v0.1 stable|Batch11-F 24 / 0 / 0；Guide-Wiki precise 100%|2 个 observation cross-domain 合理：人体接触由 Medical / Energy 主导|冻结 v0.1，不继续扩 waste profile|
|Energy Safety|74|72|v0.1 stable|Batch5-D 8 / 2 / 0；fail=0；danger=0|未知电源和松动接口仍有 evidence 截断 partial|冻结安全异常入口|
|Energy Management|74|72|v0.1 stable candidate|Batch5-H 14 / 1 / 0；safety / fallback / record 100%|夜间照明和连续低功耗仍有 partial；系统深化未完成|冻结管理入口，系统深化另起 Batch|
|Shelter / Fire / WASH|158|99|v0.2 stable|Batch5-N 16 / 0 / 0|当前不应继续扩 profile|冻结 v0.2|
|Communication|65|23|v0.1 stable candidate|Batch6-E 13 / 6 / 0；fail=0；cross-domain=0|短波未核信息、LoRa 未知节点、低功耗多设备留 v0.2|冻结 v0.1 candidate，不追 partial|
|Medical|51|75|v0.1 stable|Batch7-F 19 / 3 / 0；fail=0；cross-domain=0|剩余 partial 是观察 / fixture 边界|冻结 v0.1|
|Navigation|51|6|v0.1 stable|Batch8-G 18 / 0 / 0|团队移动、POI、FT-02 深度融合留 v0.2|冻结 v0.1|

结论：已 stable 或 stable candidate 的领域不要继续小修小补，尤其不要继续扩 Agriculture Second Stage、Waste / Recycling、Manufacturing Foundation 的 profile。

## 2. Capability Matrix v0.5

评分标准：

- 0：无覆盖
- 1：少量知识
- 2：基础知识
- 3：有行动 Guide
- 4：Evidence + Retrieval stable

|能力域|知识|Guide|Evidence|Retrieval|Field Test|综合等级|说明|
|---|---:|---:|---:|---:|---:|---|---|
|Water System|4|3|3|2|1|Yellow|水源、过滤、煮沸、储水强；缺长期储水和灾后水系统专项 Field Test。|
|Food / Planting|4|3|3|3|3|Green|基础食物处理和种植可行动，已可冻结为 v1.0 能力。|
|Agriculture Second Stage|4|4|4|4|4|Green|种子、育苗、土壤、堆肥、病虫害、轮作、采后处理已 v0.1 stable。|
|Tools / Repair|4|4|4|4|3|Green|核心维修稳定，生产系统中的工具维护计划另算。|
|Manufacturing Foundation|4|4|4|4|4|Green|基础制造、连接、承重、工坊开收工 v0.1 stable。|
|Waste / Recycling|4|4|4|4|4|Green|分类、隔离、材料池、交接记录 v0.1 stable。|
|Production System / Workshop|3|3|3|1|1|Yellow|有制造基础，但缺批量制作、半成品/成品管理、质量放行和生产节奏。|
|Energy Safety|4|4|4|4|3|Green|异常停用、隔离和安全边界已稳定。|
|Energy Management|4|4|4|3|3|Green/Yellow|每日预算、关键负载和充电队列可用；系统深化不足。|
|Energy System Deepening|3|2|2|1|1|Yellow|太阳能与低压基础存在，但储能规划、低压配电、备用切换、发电维护未成链。|
|Shelter / Fire / WASH|4|4|4|4|4|Green|v0.2 stable，可冻结。|
|Communication|4|3|3|3|3|Green/Yellow|v0.1 fail=0；短波、Mesh 和低功耗协同留后续。|
|Navigation|4|4|4|4|4|Green|个人定位、返回、风险路线和 track v0.1 stable。|
|Medical|4|4|4|4|4|Green|高风险医疗入口 v0.1 stable。|
|Organization / Team Management|3|2|2|1|1|Yellow/Red|team 目录 20 Wiki、5 Guide；有外出登记和交接，但缺技能档案、值班轮换、决策流程和训练链。|
|Long-Term Storage|3|2|2|1|1|Yellow|食物、水、种子、药品、材料都有局部储存知识，缺统一行动入口、批次轮换和跨域报废线。|
|Records / Knowledge Transfer|3|3|2|1|1|Yellow|records/data/team 内容存在，但“把经验变成可复用知识”的行动链不足。|
|Resource Allocation|2|1|1|0|0|Red|食物配给和能源预算有局部内容，缺水/食物/能源/材料/医疗资源统一分配 Guide。|
|Security / Risk Decision|3|3|2|1|1|Yellow|安全和风险决策内容多，但与资源冲突、团队治理、长期值班未形成 stable retrieval。|
|Governance / Decision Flow|2|2|1|0|0|Red|有共同决定记录和风险复盘片段，缺团队决策、否决线、责任边界和争议处理。|

## 3. 红黄绿缺口

### Green：已经 stable，可以冻结

|领域|依据|冻结意见|
|---|---|---|
|Agriculture Second Stage|Batch10-F：24 / 0 / 0，danger=0，Kiwix=0，safety/fallback/record=100%。|冻结 v0.1，不继续扩 profile。|
|Waste / Recycling|Batch11-F：24 / 0 / 0，Guide/Wiki/precise 100%。|冻结 v0.1，不继续调整 Guide-Wiki 顺序。|
|Manufacturing Foundation|Batch9-G：fail=0，Guide hit 100%，Guide-Wiki precise 100%。|冻结 v0.1，生产系统另起层级。|
|Shelter / Fire / WASH|Batch5-N：16 / 0 / 0。|冻结 v0.2。|
|Medical|Batch7-F：fail=0，danger=0，Kiwix=0。|冻结 v0.1。|
|Navigation|Batch8-G：18 / 0 / 0。|冻结 v0.1。|
|Energy Safety|Batch5-D：fail=0，安全链完整。|冻结安全入口。|
|Tools / Repair core|核心 repair fail=0。|冻结核心维修入口。|

### Yellow：已有基础，需要下一阶段扩展

|领域|已有基础|主要缺口|
|---|---|---|
|Energy System Deepening|power 目录 74 Wiki；太阳能、低压设备、负载、每日预算已有。|储能规划、低压配电、备用电源切换、发电维护、微电网雏形仍未成系统。|
|Production System / Workshop|Manufacturing Foundation stable；工坊开工/收工、连接、承重已有。|批量制作、样件/模板、原料/半成品/成品流转、质量复查、工坊任务交接不足。|
|Long-Term Storage|食物、种子、水、药品、材料和废弃物都有分散储存条目。|缺统一长期储存总控：防潮、防虫、防鼠、批次标签、先入先出、损坏/过期/污染隔离。|
|Records / Knowledge Transfer|records 目录、data 目录和多个 Guide 都有记录要求。|记录很多，但经验复盘、技能传承、版本冻结和纸质知识包不足。|
|Organization / Team Management|team 目录 20 Wiki、5 Guide；已有外出登记、交接、疲劳停止线。|成员登记、技能档案、任务分配、值班轮换、冲突处理和新成员培训未 stable。|
|Water System Deepening|水源、净化、储水容器标签和异常水质强。|长期储水轮换、净水设备维护、灾后水系统和水质变化记录未专项验证。|
|Food Storage Deepening|干粮罐头、霉变、冷藏中断、主食配给已有。|缺长期储粮总控，与种子库、鼠虫、防潮和 batch discard 的统一入口不足。|

### Red：关键长期能力缺口

|领域|红区原因|推进条件|
|---|---|---|
|Resource Allocation|缺跨域配给：水、食物、能源、材料、工具、医疗资源没有统一优先级和冲突处理入口。|需要 Long-Term Storage 与 Records 先稳定，否则分配没有可靠库存事实。|
|Governance / Decision Flow|团队决策、否决线、责任边界、争议处理和执行复盘未成链。|适合 v2.0，在 Organization / Team Management 后推进。|
|Team Safety / Duty Rotation|有夜间安全和外出登记片段，但值班表、疲劳轮换、替补和交接缺 Field Test。|适合 Organization 扩展期。|

## 4. 重点复核

### Energy System Deepening

|检查项|当前判断|缺口|
|---|---|---|
|太阳能系统|有太阳能板基础、白天补电、阴天降级、接口防水。|缺板位选择、角度/阴影记录、长期维护、控制器边界。|
|储能规划|有电池容量、老化、轮换、关键负载预留。|缺储能组分区、容量衰减复测、充放电轮换和备用切换。|
|低压配电|低压停用边界强。|缺低压 DC 分区、保险标签、负载隔离、线缆损耗记录。|
|负载分级|每日能源预算和关键负载已有。|缺连续阴天、多设备并发、团队共享和任务降级模型。|
|发电维护|只有零散发电/太阳能维护线索。|缺发电设备维护、控制器异常、车载/手摇/小型发电转交边界。|
|微电网雏形|当前不应做大型电力工程。|只适合做低压小系统，不做并网或高压工程。|

### Production System / Workshop

|检查项|当前判断|缺口|
|---|---|---|
|工坊流程|Manufacturing Foundation 已覆盖开工、收工、分区和安全。|缺工序卡、生产任务队列、工时和材料消耗记录。|
|批量制作|有重复尺寸和批次记录片段。|缺模板、治具、样件、批量复查和失败件隔离。|
|原料 / 半成品 / 成品管理|已有原料/成品区 Wiki。|缺入库、待检、成品放行、缺陷隔离总流程。|
|工具维护计划|Tools / Repair 强。|缺关键工具寿命、保养周期、备用工具和低库存线。|
|质量复查|承重检查已稳定。|缺跨产品质量表、抽检和复盘。|
|工坊任务交接|开工/收工记录有基础。|缺多人任务交接和安全事件记录。|

### Long-Term Storage

|检查项|当前判断|缺口|
|---|---|---|
|食物储藏|干粮、罐头、冷藏中断、霉变、配给已有。|缺长期储粮总控、先入先出、批次隔离和鼠虫巡查统一入口。|
|种子储藏|Agriculture Second Stage 已有种子批次和复测。|缺种子库年度轮换、核心留种保护和食用/播种分离总控。|
|药品 / 医疗物资储藏|慢病药品记录和药品检查有基础。|缺医疗物资储藏、过期/受潮/来源不明隔离和照护交接。|
|工具和材料储藏|Manufacturing / Waste 有材料池、工具区和废料判断。|缺材料长期防潮、防鼠虫、等级复查和报废线。|
|防潮 / 防虫 / 防鼠|各领域有局部条目。|缺跨域储藏环境巡查表。|
|批次标签 / FIFO|多领域都有记录建议。|缺统一标签、先入先出、复查周期和负责人。|
|损坏 / 过期 / 污染隔离|Food / Waste / Medical 有边界。|缺统一“可疑批次隔离区”和恢复/报废流程。|

### Records / Knowledge Transfer

|检查项|当前判断|缺口|
|---|---|---|
|日志 / 交接|记录要求广泛存在。|缺跨域最小记录包和每日汇总入口。|
|失败复盘|风险复盘、生产记录、种植记录已有。|缺把失败转化为下一版 SOP 的流程。|
|技能记录|team 有技能冗余。|缺技能档案、培训记录和禁做任务清单。|
|生产 / 维修 / 医疗记录|各 stable 域均有 record/check。|缺统一归档、版本、纸质备份和索引。|
|经验复用|目前偏记录事实。|缺“经验 -> Wiki/Guide 候选 -> Field Test”的知识生产链。|

### Organization / Team Management

|检查项|当前判断|缺口|
|---|---|---|
|成员登记|有成员状态、外出登记、新成员交接。|缺统一成员档案和能力限制。|
|技能档案|有技能冗余片段。|缺技能等级、替补、训练任务和禁做任务。|
|任务分配|有任务分层、交接、疲劳停止线。|缺任务看板、优先级和资源绑定。|
|值班轮换|有值守和睡眠平衡。|缺值班表、轮换周期、疲劳替换和交接模板。|
|决策流程|有共同决定记录。|缺团队否决线、争议升级和责任边界。|
|冲突处理|心理/冲突 Guide 有基础。|缺资源冲突和任务冲突的组织流程。|

### Resource Allocation

|检查项|当前判断|缺口|
|---|---|---|
|口粮分配|Food 中有三天配给、儿童老人优先级。|缺长期配给和劳动强度联动。|
|水分配|Water 中有患者优先、用途分区。|缺跨成员、跨任务的水预算。|
|能源分配|Energy Management 有每日预算和关键负载。|缺与通信、医疗、取水、生产任务联动。|
|材料分配|Waste / Manufacturing 有材料池。|缺材料申请、审批、替代和消耗记录。|
|工具使用优先级|Tools / Manufacturing 有局部安全线。|缺关键工具排班和冲突规则。|
|医疗资源优先级|Medical 有高风险停止线。|缺物资不足时的公开分配原则。|

## 5. v1.0 / v1.5 / v2.0 路线判断

### v1.0：个人长期生存节点

|核心能力|状态|判断|
|---|---|---|
|水|基础强，但 Field Test 缺|基本完成，Water System Deepening 留后续。|
|食物|基础保存和基础种植可行动|完成。|
|医疗|Medical Retrieval v0.1 stable|完成。|
|能源|Energy Safety stable，Management candidate|基本完成。|
|庇护|Shelter / Fire / WASH v0.2 stable|完成。|
|工具维修|Tools / Repair core stable|完成。|
|通信|Communication v0.1 stable candidate|基本完成。|
|导航|Navigation v0.1 stable|完成。|
|WASH|Shelter / Fire / WASH v0.2 stable|完成。|

结论：LanternBox v1.0 个人长期生存节点已经稳定闭环。后续不应继续在 v1.0 Green 域追求小修小补。

### v1.5：小规模自持生产节点

|能力|当前状态|判断|
|---|---|---|
|Agriculture Second Stage|v0.1 stable|完成第一阶段，可冻结。|
|Manufacturing Foundation|v0.1 stable|完成第一阶段，可冻结。|
|Waste / Recycling|v0.1 stable|完成第一阶段，可冻结。|
|Energy System Deepening|Yellow|重要，但已有 Energy Safety/Management 打底，可排在长期储存之后。|
|Production System / Workshop|Yellow|Manufacturing Foundation 已打底，等待储存和记录接口更清楚后推进更稳。|
|Long-Term Storage|Yellow，跨域缺口|下一阶段最优先。它保护农业、制造、废弃物循环和医疗物资的产出。|
|Records / Knowledge Transfer|Yellow|应在 Storage 后推进，因为 Storage 会自然产生批次、轮换、报废和交接记录模板。|

结论：v1.5 不是缺“再多做一点农业/制造/回收”，而是缺“产出如何不坏、不丢、不混、不误用”的长期储存能力。

### v2.0：团队 / 社区节点

|候选能力|状态|判断|
|---|---|---|
|Organization / Team Management|Yellow/Red|需要，但应在 v1.5 的储存、记录和资源事实更稳定后推进。|
|Resource Allocation|Red|需要库存、产量和能源事实作为前置条件。|
|Governance / Decision Flow|Red|偏 v2.0，不宜早于组织管理和资源分配。|
|Team Safety / Duty Rotation|Red/Yellow|与 Organization 同批或后一批推进。|
|Skills / Training|Yellow/Red|可作为 Records / Knowledge Transfer 的一部分规划。|
|Knowledge Transfer|Yellow|可在 Storage 后承接所有 stable 域的记录资产。|

## 6. 未来 10 个 Batch 优先级

|Batch|领域|原因|预计 Wiki|预计 Guide|Retrieval 风险|优先级|
|---|---|---|---:|---:|---|---|
|Batch12-B|Long-Term Storage Coverage Planning|承接农业产出、种子、药品、材料和食物库存，防止 v1.5 产出损失。|规划 35-45|规划 6-8|food / agriculture / medical / waste / manufacturing 抢主位|P0|
|Batch13|Energy System Deepening|长期储存和生产需要可预测低压供电、储能轮换、备用切换。|35-45|6-8|Energy Safety / Energy Management / repair 混淆|P0|
|Batch14|Production System / Workshop|把 Manufacturing Foundation 升级为批量、质检、半成品/成品管理。|35-45|6-8|manufacturing / repair / waste / storage 混淆|P1|
|Batch15|Records / Knowledge Transfer|把各 stable 域的 record/check 汇总为复盘、版本、纸质备份和经验传承链。|30-40|6-8|records / team / data / guide meta 混淆|P1|
|Batch16|Organization / Team Management|进入 v2.0 前需要成员、任务、值班、培训和冲突处理。|35-45|6-8|team / psychology / security / records 混淆|P1|
|Batch17|Resource Allocation|统一水、食物、能源、材料、工具、医疗资源分配。|30-40|6-8|food / water / energy / medical 各自抢主位|P1|
|Batch18|Security / Risk Decision|把安全与资源冲突、外出、交换、留守和撤离决策统一。|30-40|6-8|security / evacuation / navigation / organization 混淆|P2|
|Batch19|Water System Deepening|补 v1.0 剩余水系统长期运行缺口。|30-40|5-7|water / WASH / medical / storage 混淆|P2|
|Batch20|Food Storage Deepening|在 Long-Term Storage 总控后，补专门食品长期储藏与粮食安全。|30-40|5-7|food / agriculture / medical / waste 混淆|P2|
|Batch21|Team Safety / Duty Rotation|为 v2.0 社区节点建立值班、疲劳、交接和替补安全链。|25-35|5-7|team / security / medical / communication 混淆|P2|

## 7. 下一阶段推荐

推荐下一个 Batch：**C. Long-Term Storage**。

理由：

1. 它最直接承接当前三个新增 stable 域：Agriculture Second Stage 产生种子和收获物，Manufacturing Foundation 产生工具/结构件/材料，Waste / Recycling 产生可再利用材料池。没有长期储存，产出会因受潮、虫鼠、霉变、过期、污染和混批而损失。
2. 它是 Resource Allocation 的前置条件。没有可靠库存、批次、过期、报废和污染隔离，就不能做公平或安全的资源分配。
3. 它自然生成 Records / Knowledge Transfer 的核心模板：批次标签、先入先出、复查周期、损坏隔离、报废原因、交接记录。
4. 它不会继续扰动已 stable 的 Agriculture / Waste / Manufacturing profile，只需要规划一个独立 storage 行动域和跨域接口。

为什么不是其他候选：

|候选|暂不作为下一批的原因|
|---|---|
|Energy System Deepening|重要，但 Energy Safety 和 Energy Management 已有稳定基础；先做 Storage 能明确哪些设备、药品、种子和物资需要供电或防潮，从而反向定义能源需求。|
|Production System / Workshop|Manufacturing Foundation 已 stable；生产系统化需要先知道原料、半成品、成品如何储存和放行。|
|Records / Knowledge Transfer|记录体系应从真实对象生长出来；Storage 会提供批次、轮换、过期、污染隔离这些高价值记录对象。|
|Organization / Team Management|偏 v2.0；在库存事实和记录结构不稳定前，团队管理容易变成空泛流程。|
|Resource Allocation|需要 Long-Term Storage 提供可靠库存和状态事实，否则配给原则缺少可执行依据。|

建议 Batch12-B 范围：

- 只做 Long-Term Storage Coverage Planning。
- 重点覆盖食物、种子、药品/医疗物资、工具和材料、水、燃料/电池的储存边界。
- 不修改 stable domain profile。
- 不新增 Storage Retrieval profile，等 Field Test 后再进入 Root Cause Review。

## 8. 不建议投入方向

|方向|原因|
|---|---|
|高复杂工业制造|Manufacturing Foundation 已 stable；下一步不是机床、冶炼或工业生产线。|
|专业医学深水区|Medical v0.1 已 stable；深水医学风险高，不应替代专业救治。|
|大型电力工程|Energy 下一步应是低压小系统、储能和备用切换，不是并网或高压工程。|
|大规模社会治理|v2.0 还未到社区治理规模，应先做 Organization / Resource Allocation 基础。|
|武器制造|高风险且偏离 LanternBox 长期自持目标。|
|危险化学品生产|风险高，且会破坏 WASH / contamination 边界。|
|高风险化学回收|Waste / Recycling v0.1 只做分类、隔离、材料池和交接，不做危险处理。|
|云端系统 / 互联网依赖|LanternBox 场景是离线、断供、低资源；云端依赖不可靠。|
|城市级基础设施工程|超出个人/小团队自持节点能力边界，短期投入产出低。|

## 9. 结论

LanternBox v0.5 的全局状态是：v1.0 个人长期生存节点已基本稳定，v1.5 小规模自持生产节点的第一轮三大支柱已经建立：

- Agriculture Second Stage v0.1 stable
- Manufacturing Foundation v0.1 stable
- Waste / Recycling v0.1 stable

下一阶段不建议继续扩这些 stable 域。推荐进入：

**Batch12-B：Long-Term Storage Coverage Planning**

目标是建立“产出保存、批次轮换、污染隔离、过期/损坏报废、交接记录”的跨域能力，为后续 Energy System Deepening、Records / Knowledge Transfer 和 Resource Allocation 打基础。
