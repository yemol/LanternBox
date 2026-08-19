# LanternBox Knowledge Coverage Map v0.3 Final

生成日期：2026-07-17

本报告只做全局知识覆盖分析和路线规划。分析对象为当前 `wiki_import/*/*.md`、`data/guides/**/*.json`、`docs/knowledge/knowledge_coverage_map_v0_3.md` 以及 Batch4/5/6/7/8 已完成报告。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

当前系统规模：

|项目|数量 / 状态|
|---|---:|
|Markdown Wiki|912|
|Guide JSON|796|
|Wiki priority|P0=603, P1=207, P2=102|
|Wiki risk_level|high=393, caution=386, normal=133|
|Guide priority|P0=585, P1=144, P2=67|
|Guide risk_level|critical=14, high=94, caution=580, normal=108|
|有 Guide 链接的 Wiki|855 / 912|
|有 related_wiki 的 Guide|363 / 796|
|Guide-Wiki related_wiki 边|2017|

核心判断：LanternBox v0.3 已完成从“基础应急知识库”到“多个稳定行动检索域”的跃迁。Shelter / Fire / WASH、Medical、Navigation 已完成 final verification；Energy Safety 稳定，Energy Management、Communication、Tools / Repair、Planting 已具备可行动基础但仍有二阶段缺口。下一圈主要矛盾转为长期自持：制造生产、农业深化、能源系统深化、组织管理、资源循环与长期储存。

## 1. Stable Domain 总览

目录计数采用当前实际目录，不用宽关键词扩算。

|Domain|Wiki|Guide|Retrieval|Field Test|Status|
|---|---:|---:|---|---|---|
|Planting / Food Production|134|43|种植基础 retrieval 可用；Guide hit 偏低|Planting 10 / 0 / 0；Guide hit 4 / 10；Guide-Wiki combo 9 / 10|Stable Candidate|
|Tools / Repair|127|53|Tools Repair v0.1 stable；Wave2 仍有现场安全 partial|v0.1 10 / 0 / 0；Wave2 16 / 4 / 0|Stable Candidate|
|Energy Safety|74|72|电池异常、低压设备、未知电源入口稳定|Batch5-D 后 8 / 2 / 0，fail=0，danger=0，安全指标 100%|Stable|
|Energy Management|74|72|预算、最低电量、充电队列、太阳能低产出稳定；夜间照明仍 partial|Batch5-H 后 14 / 1 / 0，Guide-Wiki precise 100%|Stable Candidate|
|Shelter / Fire / WASH|158|99|v0.2 final stable|Batch5-N 16 / 0 / 0，Guide/Wiki/precise 100%，cross-domain=0|Stable|
|Communication|65|23|湿设备、天线天气、故障隔离稳定；短波/低功耗/LoRa 仍有 partial|Batch6-E 13 / 6 / 0，fail=0，cross-domain=0|Stable Candidate|
|Medical|51|75|high / critical 医疗入口稳定|Batch7-F 19 / 3 / 0，fail=0，cross-domain=0，安全指标 100%|Stable|
|Navigation|51|6|v0.1 final stable|Batch8-G 18 / 0 / 0，Guide/Wiki/precise 100%，fail=0|Stable|

状态说明：

- Stable：final verification 或等价验证通过，fail=0，dangerous suggestion=0，Kiwix 越权=0，安全/降级/记录链完整。
- Stable Candidate：fail=0 且安全链完整，但仍有较多 partial、field test 未 final verification，或覆盖只到 v0.1。
- Incomplete：有内容但缺行动入口、证据链或 Field Test。

## 2. Capability Matrix

评分标准：

- 0：没有覆盖。
- 1：少量知识。
- 2：基础知识。
- 3：行动指南。
- 4：Evidence + Retrieval 稳定。

|能力|知识|行动|Evidence|Retrieval|价值|
|---|---:|---:|---:|---:|---|
|水：饮水、过滤、消毒、储水|4|3|3|2|v1.0 必需；需要 Field Test 与长期储水运行链。|
|食物保存与基础烹饪|3|3|3|2|v1.0 必需；腐败判断和保存已有，长期储存仍需深化。|
|Planting / Food Production|4|3|3|3|v1.5 核心；基础种植可行动，粮食化生产不足。|
|Tools / Repair|4|4|3|3|v1.0/v1.5 核心；维修安全强，制造深度不足。|
|Energy Safety|4|4|4|4|v1.0 stable；应冻结安全重复扩写。|
|Energy Management|4|4|4|3|v1.0 stable candidate；v1.5 需扩太阳能与储能系统。|
|Shelter / Fire / WASH|4|4|4|4|v1.0 stable；当前不建议继续扩同域。|
|Communication|4|3|3|3|v1.0 stable candidate；v1.5 需与 navigation / team 联动。|
|Medical|4|4|4|4|v1.0 stable；高风险入口已可冻结。|
|Navigation|4|4|4|4|v1.0 stable；个人定位、返回、路线和 track 已成链。|
|制造与生产|2|1|1|1|v1.5 红区；长期自持的下一个核心缺口。|
|组织与团队管理|3|2|2|1|v2.0 红/黄区；记录多，行动入口和 field test 少。|
|资源循环与废弃物|2|1|1|1|v1.5/v2.0 红区；WASH 强但废弃物生命周期弱。|
|长期储存|3|2|2|1|v1.5 黄区；食物、水、种子、材料、药品分散存在，缺总控。|
|地图测绘与团队协作|3|3|3|3|个人导航 stable；团队测绘、POI、同步仍为 v0.2。|

## 3. v1.0 路线：个人长期生存节点

目标：个人或家庭在长期断供、断网、低资源环境下维持生命、安全、移动和基本修复。

必须覆盖：

|能力|当前状态|明显漏洞|
|---|---|---|
|水|基础知识强，Wiki 79，Guide 110|缺 Water Field Test；长期储水、取水工具污染、储水轮换和灾后水系统还未稳定成链。|
|食物|基础保存、腐败判断、烹饪可用|长期储存、防霉、分级消耗和采后处理不够系统。|
|医疗|Medical Retrieval v0.1 stable|剩余 partial 不阻塞；低体温专属 evidence 可后续观察。|
|能源|Energy Safety stable，Energy Management stable candidate|夜间照明 partial；太阳能/储能系统是 v1.5。|
|工具维修|Tools Repair v0.1 stable candidate|现场安全强，制造与工作台/夹具深度不足。|
|庇护|Shelter / Fire / WASH v0.2 stable|可冻结。|
|通信|Communication v0.1 stable candidate|短波未核信息、低功耗多设备、LoRa 未知节点留观察。|
|导航|Navigation Retrieval v0.1 stable|个人定位和返回已达 stable；团队移动留 v0.2。|

v1.0 结论：个人长期生存节点已经接近闭环。最明显漏洞不再是高风险医疗或 Shelter/Fire/WASH，而是水系统的长期运行 Field Test、食物长期储存、以及工具维修向制造生产的过渡。

## 4. v1.5 路线：小规模自持生产节点

目标：小团队可以持续补充食物、维护设备、复用材料、维持通信与能源，并把经验保留下来。

### 农业深化

|检查项|当前判断|缺口|
|---|---|---|
|种子保存|有基础 Wiki 和种子批次记录|缺年度种子账、发芽率衰减、留种隔离、批次淘汰。|
|育苗|简易育苗、移栽、失败复盘可行动|缺季节排程、连续育苗、苗床容量管理。|
|土壤恢复|污染边界、草木灰、排水有基础|缺堆肥成熟判断、有机质恢复、pH 近似判断、土壤轮换。|
|病虫害|病残体分流、病斑隔离有基础|缺常见虫害观察、物理防控、工具/浇水传播链。|
|轮作|明显不足|缺作物分组、地块记录、前后茬风险。|
|长期粮食生产|不足|当前偏蔬菜补充，不是热量主食生产。|

### 制造与维修深化

|检查项|当前判断|缺口|
|---|---|---|
|木工|锯切、支撑、固定有基础|缺尺寸测量、连接方式、重复制作、承重验证。|
|金属加工|非常弱|缺薄金属修补、钻孔、去毛刺、冷弯、边缘防割。|
|简单机械|非常弱|缺杠杆、滑轮、轴、铰链、传动的低风险应用。|
|工具维护|较强|需要从“工具安全”升级到“工具寿命和维修计划”。|
|替代材料|Repair Wiki 分散存在|缺材料选择矩阵、禁用材料、测试记录。|
|零件再制造|不足|缺拆解件分类、可复用零件库、复测与隔离。|

### 能源系统深化

|检查项|当前判断|缺口|
|---|---|---|
|太阳能系统|低产出排程稳定|缺板位选择、阴影记录、角度、线缆损耗和日维护。|
|储能规划|电池轮换和最低线可用|缺多电池组、容量衰减、替换策略。|
|电力预算|稳定|可扩到连续阴天、季节模型和团队分摊。|
|低压系统设计|安全边界强|缺低压 DC 分区、保险、标签、负载隔离。|
|发电维护|不足|手摇、车载、太阳能控制器、接头防潮仍弱。|

v1.5 结论：最该补的是制造与生产基础，其次是农业第二阶段和能源系统深化。它们决定 LanternBox 是否能从“活下来”进入“长期修复和生产”。

## 5. v2.0 路线：团队 / 社区节点

目标：多人长期协作，形成资源、生产、知识和任务系统。

### 组织能力

|检查项|当前判断|缺口|
|---|---|---|
|成员登记|有成员信息备份和记录基础|缺技能、健康、权限、风险边界的统一行动 Guide。|
|技能档案|不足|缺训练记录、替补能力、资格限制。|
|任务分配|有零散记录和交接|缺任务队列、优先级、疲劳调整、责任闭环。|
|资源管理|库存记录有基础|缺跨域预算、水食能源材料联动。|
|决策流程|风险决策有基础|缺团队否决线、争议处理、复盘。|
|经验传承|知识保存有基础|缺演练、SOP、纸质训练卡和新人教学链。|

### 生产系统

|检查项|当前判断|缺口|
|---|---|---|
|工坊|Repair 区域安全有基础|缺工坊布局、工序、工具权限和低风险生产线。|
|仓储|食物/能源/材料记录分散存在|缺统一仓储分区、周转、禁用、复查。|
|原料管理|拆解和材料替代有点状内容|缺原料入库、污染隔离、可用性测试。|
|质量控制|不足|缺试制、复测、失败隔离、使用限制标记。|
|维修体系|维修安全较强|缺维修排期、备件库、复发故障分析。|

### 信息系统

|检查项|当前判断|缺口|
|---|---|---|
|知识归档|data 目录有 29 篇 Wiki|缺知识卡生命周期、版本冻结、废弃条目处理。|
|版本管理|少量基础|缺地图/Guide/记录版本统一规则。|
|地图更新|Navigation v0.1 已建立个人 track|缺团队 POI、风险标记共享、地图复核流程。|
|任务记录|有记录 Wiki|缺任务单、检查点、交接和复盘统一模板。|

v2.0 结论：组织和信息系统是最大红区，但应该在制造、农业、能源二阶段之后推进，否则会变成“管理表格很多，生产能力不足”。

## 6. 红黄绿缺口

### Green：已经 stable / 可冻结

|领域|依据|建议|
|---|---|---|
|Shelter / Fire / WASH|Batch5-N：16 / 0 / 0，cross-domain=0，安全指标 100%。|冻结 v0.2，仅保留回归。|
|Medical|Batch7-F：19 / 3 / 0，fail=0，cross-domain=0，high/critical 入口稳定。|冻结 v0.1，不追逐剩余 partial。|
|Navigation|Batch8-G：18 / 0 / 0，Guide/Wiki/precise 100%。|冻结 v0.1，团队导航另起 v0.2。|
|Energy Safety|Batch5-D 后 fail=0，安全/fallback/record 100%。|冻结安全重复扩写。|

### Yellow：已有基础，需要扩展

|领域|当前基础|下一步|
|---|---|---|
|Water System|水源、过滤、煮沸、储存、污染隔离已有|做 Water System Field Test，再规划长期储水和灾后水系统。|
|Agriculture Second Stage|基础 planting 10 / 0 / 0|扩种子、育苗、土壤、病虫害、轮作、主食生产。|
|Energy System|管理链稳定|扩太阳能系统维护、储能规划、低压分区和发电维护。|
|Communication v0.2|v0.1 fail=0|处理短波未核信息、低功耗多设备、LoRa 未知节点、通信导航联动。|
|Long-Term Storage|多域有点状 Wiki|建立食物、水、种子、材料、药品的统一储存链。|

### Red：缺少核心能力

|领域|红区原因|优先级|
|---|---|---|
|Manufacturing / Production|没有独立生产能力链；木工、金属、简单机械、零件再制造不足。|最高|
|Organization / Team Management|记录多，行动入口和 field test 少；多人协作闭环不足。|高|
|Waste / Recycling|WASH 强但废弃物生命周期、材料回收、再利用边界不足。|高|
|Production System / Workshop|缺工坊、仓储、原料、质量控制、维修体系。|高|
|Team Mapping / Surveying|个人导航 stable；团队地图、POI、风险同步不足。|中|

## 7. Retrieval Coverage Gap

|类型|领域|判断|
|---|---|---|
|A. 知识存在 + Retrieval stable|Shelter / Fire / WASH、Medical、Navigation、Energy Safety|可冻结，不应继续堆内容。|
|A/B. 知识存在 + Retrieval stable candidate|Energy Management、Communication、Tools / Repair、Planting|fail=0 或安全链稳定，但仍有 partial 或二阶段深度不足。|
|B. 知识存在 + Retrieval不足|Water System、Long-Term Storage、Organization records|有大量 Wiki/Guide，但缺专项 Field Test 和 profile 边界。|
|C. Guide存在 + Wiki不足|组织、团队、长期生产管理、部分低体温/特殊医疗观察|Guide 或记录入口存在，精准 Wiki 支撑不完整。|
|D. Wiki存在 + 没有行动入口|manufacturing、maps v0.2、data records、waste/recycling|Wiki 较多但没有稳定行动 Guide 和 Field Test。|
|E. 未来需要新增能力|制造生产、农业二阶段、能源二阶段、组织管理、资源循环|这些决定 v1.5/v2.0。|

重点风险：

- 制造：容易与 repair/tools/engineering 混淆，需要先定义低资源制造边界。
- 组织：容易被 records/general 泛化，需要行动 Guide 而不是表格百科。
- 生产：容易分散在 food/planting/repair，需要“生产系统”层级。
- 循环利用：容易与 contamination/WASH 混淆，需要污染优先级。
- 储存：容易被 food/water/data 各自吸走，需要长期储存总控。
- 农业二阶段：容易与 food/water/contamination 竞争，需要 field profiles。
- 能源二阶段：容易与 Energy Safety 抢主位，需要清晰区分“系统规划”与“异常停用”。

## 8. 未来 Batch 优先级

|Batch|领域|原因|预计 Wiki|预计 Guide|Retrieval 风险|
|---|---|---|---:|---:|---|
|Batch9-B|Manufacturing & Production Foundation Planning|长期自持核心红区；从维修升级到生产。|规划 35-45|规划 6-8|repair / tools / engineering / safety 混淆。|
|Batch9-C|Manufacturing & Production Apply|建立木工、材料选择、简单加工、零件复用第一批行动链。|30-35|最多 6|旧 repair Guide 抢主位，制造 Wiki 无入口。|
|Batch9-D|Manufacturing Retrieval Field Test|验证生产能力是否进入 evidence。|0|0|repair safety 与 production action 边界。|
|Batch10-A|Agriculture Second Stage Planning|粮食独立和长期种植能力不足。|规划 35-45|规划 6-8|planting / food / water / contamination 混淆。|
|Batch10-B|Agriculture Second Stage Apply|补种子、育苗、土壤、病虫害、轮作、主食生产。|30-40|最多 6|短期容器菜园旧入口抢长期生产主位。|
|Batch10-C|Agriculture Retrieval Field Test|验证从小菜园到持续生产的 evidence 稳定性。|0|0|food preservation 和 planting 边界。|
|Batch11-A|Water System Deepening Planning|v1.0 明显漏洞：水基础强但长期运行未 Field Test。|规划 25-35|规划 4-6|water / hygiene / medical / WASH 混淆。|
|Batch11-B|Energy System Deepening Planning|从能源管理升级到系统维护和储能规划。|规划 25-35|规划 4-6|Energy Safety 异常停用抢系统规划。|
|Batch12-A|Organization & Team Management Planning|v2.0 核心；多人协作目前缺行动入口。|规划 30-40|规划 6-8|records/general/security 泛化。|
|Batch12-B|Waste / Recycling & Long-Term Storage Planning|资源循环和长期储存连接生产系统。|规划 30-40|规划 5-7|WASH / contamination / food / repair 竞争。|

## 9. 下一阶段推荐

推荐进入：

```text
Batch9-B Manufacturing & Production Foundation Coverage Planning
```

理由：

1. Navigation 已在 Batch8-G 达到 v0.1 stable，原红区已关闭。
2. Manufacturing / Production 是当前最大红区，也是 v1.5 小规模自持生产节点的基础。
3. Tools / Repair 已有大量安全和维修 evidence，可作为制造生产的前置能力，但不能替代生产行动链。
4. 制造生产能同时支撑能源维护、农业工具、Shelter 修补、通信设备支架、仓储与资源循环，横向价值最高。

Batch9-B 应只做规划，不直接 Apply。重点先定义：

- 低资源制造边界。
- 木工 / 简单加工 / 材料选择 / 拆解复用 / 质量检查的能力模型。
- 与 repair/tools 的 retrieval 边界。
- 不做高风险工程、不做复杂机加工、不做现代工业依赖。

## 10. 不建议投入方向

当前不建议投入：

|方向|原因|
|---|---|
|高复杂工业制造|设备、精度、能源和安全门槛高，短期不能提升 LanternBox 的离线行动能力。|
|专业医学深度|Medical v0.1 已覆盖 high / critical 入口；继续深挖容易越界到诊断、处方和侵入操作。|
|大型能源工程|当前需要低压、安全、维护和预算，不需要微电网工程教程。|
|现代互联网服务|LanternBox 核心是假设断网/低资源，云服务依赖会削弱离线能力。|
|复杂通信工程|Communication v0.1 仍有 practical partial，暂不做商业网络、复杂短波工程或互联网部署。|
|大型建筑工程|Shelter v0.2 已 stable；下一步不是建筑教程，而是低风险修补和制造生产。|
|高风险化学处理|WASH/contamination 已有边界，不做消毒剂配方大全、化学中和大全或危险材料加工。|
|军事化/冲突导向内容|会偏离个人/小团队长期自持目标，并增加安全与合规风险。|

## 11. 总结

LanternBox 当前已经具备 v1.0 的大部分可执行能力：医疗、庇护、火源、WASH、能源安全、导航已稳定，通信和能源管理达到 stable candidate，工具维修与种植具备基础行动能力。

下一阶段不应继续在 stable 领域堆内容。最优路线是进入制造与生产基础，把系统从“能生存、能修补”推进到“能持续制作、替代、复用和维护”。这也是 v1.5 小规模自持生产节点的关键门槛。
