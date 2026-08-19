# LanternBox Knowledge Coverage Map v0.4

生成日期：2026-07-18

本报告只做全局知识覆盖分析和路线规划。  
遵守：`docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

本阶段未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_3_final.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch9_f_manufacturing_apply_report.md`
- 已完成的 Planting / Tools / Energy / Shelter-Fire-WASH / Communication / Medical / Navigation / Manufacturing 报告

当前系统规模：

|项目|数量 / 状态|
|---|---:|
|Markdown Wiki|950|
|Guide JSON|804|
|Wiki priority|P0=622, P1=226, P2=102|
|Wiki risk_level|high=414, caution=402, normal=134|
|Guide priority|P0=592, P1=145, P2=67|
|Guide risk_level|critical=14, high=101, caution=581, normal=108|
|有 Guide 链接的 Wiki|约 940 / 950|
|有 related_wiki 的 Guide|371 / 804|
|Guide-Wiki related_wiki 边|2076|

核心判断：Batch9-G 后，Manufacturing Foundation 已从 v0.3 的 Red 缺口转为 v0.1 stable。LanternBox v1.0 的个人长期生存节点已经基本闭环。下一阶段主要矛盾转为 v1.5 的小规模自持生产：农业第二阶段、能源系统深化、资源循环、长期储存和生产系统化。

## 1. Stable Domain 总览

目录计数采用当前实际目录，不用宽关键词扩算。

|Domain|Wiki数量|Guide数量|Retrieval状态|Field Test状态|剩余风险|
|---|---:|---:|---|---|---|
|Planting / Food Production|134|43|基础种植 retrieval 可行动；v0.1 foundation stable|Planting 10 / 0 / 0；Guide hit 4 / 10；Guide-Wiki combo 9 / 10|二阶段农业入口不足：轮作、害虫、主食生产、留种体系仍弱|
|Tools / Repair|104|53|Tools & Repair v0.1 stable；Wave2 现场安全 stable candidate|v0.1 10 / 0 / 0；Wave2 16 / 4 / 0|工作台、维修现场、工坊过渡仍有 partial；与 manufacturing 边界需保持|
|Manufacturing Foundation|61|8|Manufacturing Retrieval v0.1 stable|Batch9-G 16 / 2 / 0；Guide hit 100%；Guide-Wiki precise 100%|剩余 partial 是 selected Wiki top8 截断；生产系统化未完成|
|Energy Safety|74|72|Energy Safety Retrieval v0.1 stable|Batch5-D 8 / 2 / 0；fail=0；danger=0|未知电源 / loose connector 仍有 evidence 截断 partial，不阻塞安全 stable|
|Energy Management|74|72|Energy Management Retrieval v0.1 stable candidate|Batch5-H 14 / 1 / 0；Guide-Wiki precise 100%|夜间照明 / 低功耗连续运行仍有 partial；系统规划未完成|
|Shelter / Fire / WASH|158|99|Shelter / Fire / WASH v0.2 stable|Batch5-N 16 / 0 / 0|可冻结；不要继续扩 profile|
|Communication|65|23|Communication Retrieval v0.1 stable candidate|Batch6-E 13 / 6 / 0；fail=0；cross-domain=0|短波未核信息、低功耗多设备、LoRa 未知节点仍需 v0.2|
|Medical|51|75|Medical Retrieval v0.1 stable|Batch7-F 19 / 3 / 0；fail=0；cross-domain=0|剩余 partial 为观察 / fixture 边界，不阻塞 stable|
|Navigation|51|6|Navigation Retrieval v0.1 stable|Batch8-G 18 / 0 / 0|个人定位和返回已稳定；团队移动和 FT-02 深度融合留 v0.2|

状态说明：

- Stable：final verification 或等价验证通过，fail=0，dangerous suggestion=0，Kiwix 越权=0，安全 / fallback / record-check 完整。
- Stable Candidate：fail=0 且安全链完整，但仍有较多 partial、未 final verification，或覆盖只到 v0.1。
- v0.4 不建议继续围绕 Green 领域扩 profile；应把新增能力转向 Yellow / Red 领域。

## 2. Capability Matrix v0.4

评分标准：

- 0：无覆盖。
- 1：少量知识。
- 2：基础知识。
- 3：有行动 Guide。
- 4：Evidence + Retrieval stable。

|能力域|知识|Guide|Evidence|Retrieval|Field Test|综合等级|
|---|---:|---:|---:|---:|---:|---|
|Water System|4|3|3|2|1|Yellow：基础强，缺专项 Field Test 和长期水系统运行链|
|Food / Planting|4|3|3|3|4|Green：基础种植和食物生产可行动|
|Agriculture Second Stage|2|2|2|1|1|Yellow/Red：长期粮食生产、轮作、病虫害和留种体系不足|
|Tools / Repair|4|4|4|4|3|Green：v0.1 stable，Wave2 仍有现场安全 partial|
|Manufacturing Foundation|4|4|4|4|4|Green：v0.1 stable|
|Production System / Workshop|3|2|2|1|1|Yellow：有工坊安全和基础制造，但缺生产流程、批量、质检和仓储系统|
|Energy Safety|4|4|4|4|3|Green：安全入口稳定|
|Energy Management|4|4|4|3|3|Green/Yellow：管理稳定，系统深化不足|
|Energy System Deepening|2|2|2|1|1|Yellow：太阳能、储能、低压配电和发电维护未成系统|
|Shelter / Fire / WASH|4|4|4|4|4|Green：v0.2 stable|
|Communication|4|3|3|3|3|Green/Yellow：v0.1 stable candidate，v0.2 场景未完|
|Navigation|4|4|4|4|4|Green：v0.1 stable|
|Medical|4|4|4|4|4|Green：v0.1 stable|
|Organization / Team Management|3|2|2|1|1|Red/Yellow：记录多，团队行动入口和 Field Test 少|
|Long-Term Storage|3|2|2|1|1|Yellow：食物、水、种子、材料、药品分散存在，缺总控|
|Waste / Recycling|3|2|2|1|1|Red/Yellow：WASH 污染处理强，但废弃物生命周期和材料池弱|
|Records / Knowledge Transfer|3|3|2|1|1|Yellow：记录 Wiki 多，知识传承和版本冻结弱|
|Resource Allocation|2|1|1|0|0|Red：缺跨域预算、水食能源材料联动和配给决策入口|
|Security / Risk Decision|3|3|2|1|1|Yellow：风险决策和安全边界有基础，团队治理和资源冲突未稳定|

## 3. 红黄绿缺口更新

### Green：已经 stable，可以冻结

|领域|依据|建议|
|---|---|---|
|Shelter / Fire / WASH|Batch5-N：16 / 0 / 0，Guide/Wiki/precise 100%。|冻结 v0.2，保留回归基线。|
|Medical|Batch7-F：19 / 3 / 0，fail=0，cross-domain=0。|冻结 v0.1，不追逐 partial。|
|Navigation|Batch8-G：18 / 0 / 0。|冻结 v0.1，团队导航另起 v0.2。|
|Manufacturing Foundation|Batch9-G：16 / 2 / 0，Guide hit 100%，Guide-Wiki precise 100%。|冻结 v0.1，不再扩 profile。|
|Tools / Repair v0.1|Tools repair field：10 / 0 / 0；Wave2 fail=0。|冻结核心维修入口，现场安全 partial 留给生产系统化。|
|Energy Safety|Batch5-D：8 / 2 / 0，danger=0。|冻结安全异常入口。|

### Yellow：已有基础，需要下一阶段扩展

|领域|当前基础|主要缺口|
|---|---|---|
|Agriculture Second Stage|种子、育苗、土壤、采收已有点状 Guide/Wiki；Planting field test 10 / 0 / 0。|轮作只有极少条目；害虫专门覆盖弱；长期粮食生产和留种体系不足。|
|Energy System Deepening|太阳能低产出、低压安全、容量记录、负载管理已有。|没有“储能”命名体系；微电网为 0；低压配电、发电维护和系统化维护不足。|
|Production System / Workshop|Manufacturing Foundation stable；工坊分区、切割、连接、承重已可行动。|批量制作、工具维护计划、原料 / 半成品 / 成品管理、质量复查系统仍不足。|
|Long-Term Storage|食物、水、种子、材料、药品都有局部条目。|缺统一长期储存策略、轮换、最低线、报废和跨域清点。|
|Communication v0.2|v0.1 fail=0，湿设备和天线安全已修。|短波未核信息、低功耗多设备、LoRa 未知节点和通信-导航联动需要后续。|
|Water System Deepening|水源、过滤、煮沸、储水、污染隔离基础强。|缺长期储水 Field Test、净水设备维护、灾后水系统和水质变化记录。|

### Red：关键长期能力缺口

|领域|红区原因|优先级|
|---|---|---|
|Resource Allocation|缺跨域资源预算与配给：水、食物、能源、材料、人力没有统一行动入口。|高，但应等农业/能源二阶段后再做。|
|Organization / Team Management|成员、任务、交接有内容，但技能档案、值班轮换、决策流程、冲突处理、经验传承未成行动链。|高，偏 v2.0。|
|Waste / Recycling|污染物处理强，但废弃物分类、材料回收、厨余 / 灰烬 / 污染物分流和材料池弱。|高，适合承接 WASH + Manufacturing。|
|Agriculture Second Stage|长期粮食生产、轮作、病虫害和留种是 v1.5 的食物独立核心。|最高，适合下一批。|

## 4. 重点领域复核

### Agriculture Second Stage

|检查项|当前判断|缺口|
|---|---|---|
|种子保存|有 DG-0671 种子干燥封存、发芽率小样测试、批次记录。|缺年度种子账、留种隔离、发芽率衰减、淘汰线。|
|育苗体系|有简易育苗、育苗失败记录、移栽和缓苗。|缺连续育苗排程、苗床容量、季节窗口和替补作物。|
|土壤改良|土壤、排水、草木灰、堆肥成熟有基础。|缺有机质恢复、pH 近似判断、土壤轮换、污染地块恢复。|
|病虫害|有病残体分流和少量人工害虫移除。|害虫专门覆盖很弱；缺物理防控、传播路径、工具清洁与区域隔离。|
|轮作|仅有少量基础条目。|缺作物分组、前后茬风险、地块记录、轮作失败复盘。|
|收获后处理|有收获分级和部分食物保存。|缺采后阴干、脱水、种食分流、霉变批次隔离。|
|长期粮食生产|主食/粮食多偏食物储备和配给。|缺主食作物、热量产出、季节计划、低投入连续生产。|

判断：这是 v0.4 后最优先的 Yellow/Red 区域。

### Energy System Deepening

|检查项|当前判断|缺口|
|---|---|---|
|太阳能系统|太阳能低产出、阴影、白天充电计划已有。|缺板位选择、角度记录、长期维护、接口防潮。|
|储能规划|容量记录和电池老化条目存在，但“储能”体系命名为 0。|缺多电池组、容量衰减、充放电轮换、储能分区。|
|低压配电|低压停用边界强。|缺低压 DC 分区、保险标签、负载隔离、线缆损耗。|
|负载分级|负载优先级、关键设备保护已有。|缺连续阴天、多设备并发和团队分摊模型。|
|发电维护|发电内容少。|缺发电设备维护、手摇/车载/太阳能控制器边界。|
|微电网雏形|当前为 0。|不应立刻做大型微电网，只做低压小系统规划。|

判断：重要，但 Energy Management 已稳定候选，可排在 Agriculture Second Stage 后。

### Organization / Team Management

|检查项|当前判断|缺口|
|---|---|---|
|成员登记|成员档案和名单有内容。|缺行动化 Guide：权限、健康状态、任务限制、风险职责。|
|技能档案|只有少量技能教学条目。|缺技能分级、替补训练、禁做任务、训练记录。|
|任务分配|任务和交接内容较多。|缺任务队列、优先级、疲劳调整、闭环复盘。|
|值班轮换|“值班”直接命中为 0，但夜间轮值交接存在。|缺值班表、轮换规则、疲劳停止线。|
|决策流程|风险决策有基础。|缺团队否决线、争议处理、决策记录模板。|
|冲突处理|有心理和风险决策相关内容。|缺资源冲突、任务冲突、外部冲突的团队流程。|
|经验传承|“传承”直接命中为 0。|缺新人教学、演练、纸质 SOP、知识版本冻结。|

判断：这是 v2.0 的核心，但不应早于农业/能源/循环系统。

### Waste / Recycling

|检查项|当前判断|缺口|
|---|---|---|
|废弃物分类|有少量垃圾和污染物处理。|缺统一分类、临时储存、气味、虫害、清运失败链。|
|材料回收|有再利用和制造材料判断。|缺材料池入库、污染分级、可回收物保洁、用途降级。|
|污染物处理|contamination / hygiene 强。|缺与材料回收的分界：何时绝不回收。|
|可再利用材料池|“材料池”直接命中为 0。|缺材料池分区、标签、复测、禁用线。|
|厨余 / 灰烬 / 污染物分流|厨余、灰烬、污染物分散存在。|缺综合运行表和分流 Guide。|

判断：应在 Agriculture Second Stage 后推进，因为堆肥、厨余、污染分流和材料回收会支撑农业与制造。

### Production System / Workshop

|检查项|当前判断|缺口|
|---|---|---|
|工坊流程|Manufacturing Foundation 已建立开工/收工、分区和安全。|缺标准工序、任务排程、工时和材料消耗记录。|
|批量制作|有小批量记录和重复尺寸 observation。|缺模板、治具、样件、尺寸公差和批量复查。|
|工具维护计划|Tools / Repair 强。|缺工具寿命计划、保养周期、关键工具替补。|
|原料 / 半成品 / 成品管理|有分区 Wiki。|缺入库、待检、成品放行、缺陷隔离总流程。|
|质量复查|承重和质量复查已可行动。|缺跨产品质检表和失败件复盘。|
|安全记录|开工记录、收工清点有基础。|缺工坊日报和事故/near miss 记录。|

判断：Manufacturing Foundation 已 stable，不建议马上连续扩 Production System；应先让农业和资源循环承接制造能力。

## 5. v1.0 / v1.5 / v2.0 路线判断

### v1.0：个人长期生存节点

核心能力：水、食物、医疗、能源、庇护、工具维修、通信、导航。

|能力|状态|判断|
|---|---|---|
|水|基础强，Field Test 缺|基本可用，但应补 Water System Deepening。|
|食物|保存、腐败、基础种植可用|个人节点基本可用。|
|医疗|v0.1 stable|完成。|
|能源|Safety stable，Management stable candidate|基本完成。|
|庇护|Shelter / Fire / WASH v0.2 stable|完成。|
|工具维修|v0.1 stable，Wave2 candidate|基本完成。|
|通信|v0.1 stable candidate|基本完成。|
|导航|v0.1 stable|完成。|

结论：v1.0 已基本完成，剩余漏洞是 Water System Deepening 与少量 stable candidate 的持续观察，不应阻塞进入 v1.5。

### v1.5：小规模自持生产节点

候选能力：Agriculture Second Stage、Energy System Deepening、Production System / Workshop、Waste / Recycling。

|能力|当前状态|优先级|
|---|---|---|
|Agriculture Second Stage|基础种植 stable，但长期粮食生产不足|最高|
|Energy System Deepening|能源管理稳定候选，但系统化不足|高|
|Waste / Recycling|WASH + Manufacturing 有基础，循环链缺口明显|高|
|Production System / Workshop|Manufacturing Foundation stable，生产系统化不足|中高|

结论：v1.5 应从 Agriculture Second Stage 开始。它直接决定食物独立，并能同时拉动水、能源、制造、废弃物循环和长期储存。

### v2.0：团队 / 社区节点

候选能力：Organization / Team Management、Resource Allocation、Knowledge Transfer、Governance / Decision Flow、Team Safety。

|能力|当前状态|建议|
|---|---|---|
|Organization / Team Management|记录和任务内容分散存在，缺行动化 Field Test|v1.5 核心生产能力后推进。|
|Resource Allocation|当前红区，缺跨域预算|应在农业/能源二阶段后做。|
|Knowledge Transfer|记录有基础，传承弱|可与组织管理同批或后续批。|
|Governance / Decision Flow|风险决策有基础，团队治理弱|v2.0 重点。|
|Team Safety|安全条目多，但团队安全流程弱|与组织管理联动。|

结论：v2.0 不应先于 v1.5。否则会出现管理流程多、可生产资源少的结构性问题。

## 6. 未来 10 个 Batch 优先级

|Batch|领域|原因|预计 Wiki|预计 Guide|Retrieval 风险|优先级|
|---|---|---|---:|---:|---|---|
|Batch10-A|Agriculture Second Stage Planning|v1.5 食物独立核心；基础种植已稳定，可向长期生产升级|规划 35-45|规划 6-8|planting / food / water / contamination 混淆|P0|
|Batch10-B|Agriculture Second Stage Apply|补种子保存、连续育苗、土壤改良、病虫害、轮作、采后处理|30-40|最多 6|短期菜园 Guide 抢长期生产入口|P0|
|Batch10-C|Agriculture Retrieval Field Test|验证长期种植和粮食生产是否进入 evidence|0|0|food preservation 与 planting 边界|P0|
|Batch11-A|Water System Deepening Planning|v1.0 最后明显运行缺口；长期储水和灾后水系统未 field test|规划 25-35|规划 4-6|water / WASH / medical 混淆|P1|
|Batch11-B|Energy System Deepening Planning|v1.5 供电基础；太阳能、储能、低压系统缺系统规划|规划 25-35|规划 4-6|Energy Safety 抢系统规划主位|P1|
|Batch12-A|Waste / Recycling Planning|承接 WASH + Manufacturing + Agriculture；材料池和厨余/污染分流弱|规划 30-40|规划 5-7|hygiene / contamination / manufacturing 竞争|P1|
|Batch12-B|Long-Term Storage Planning|食物、水、种子、材料、药品分散存在，缺长期总控|规划 25-35|规划 4-6|food / water / records 分散抢主位|P1|
|Batch13-A|Production System / Workshop v0.2 Planning|Manufacturing Foundation 已 stable，但批量、质检、工坊日报弱|规划 25-35|规划 4-6|manufacturing / tools / repair 边界|P2|
|Batch14-A|Organization / Team Management Planning|v2.0 核心；成员、任务、值班、决策、冲突和传承未成链|规划 30-40|规划 6-8|records / risk_decision / security 泛化|P2|
|Batch15-A|Resource Allocation & Security Decision Planning|跨域配给、资源预算、风险决策是团队节点核心|规划 25-35|规划 5-7|water / food / energy / security 抢主位|P2|

## 7. 下一阶段推荐

推荐下一个 Batch：

```text
A. Agriculture Second Stage
```

推荐理由：

1. Manufacturing Foundation 已在 Batch9-G 达到 v0.1 stable，长期自持的“制作工具和基础结构件”前置能力已经补上。
2. v1.0 个人长期生存节点基本闭环，下一步应进入 v1.5 小规模自持生产节点。
3. Agriculture Second Stage 是 v1.5 的第一性能力：没有持续食物生产，能源、制造、组织和循环都会变成维护现有库存，而不是创造新增供给。
4. 当前基础种植虽 10 / 0 / 0，但 Guide hit 只有 4 / 10，说明系统能答，但长期生产行动入口不够稳定。
5. Agriculture Second Stage 能自然承接已有 stable domains：
   - Water：浇灌、水质、灰水边界。
   - WASH：病残体、污染土、工具清洁。
   - Manufacturing：育苗盘、支架、遮阴、储存容器。
   - Energy：育苗光照、低温/高温管理。
   - Records：种子批次、地块轮作、收获记录。

为什么不是其他候选：

|候选|暂不优先原因|
|---|---|
|B. Energy System Deepening|重要，但 Energy Safety / Management 已有 stable / stable candidate；不会立刻补足食物新增供给。|
|C. Organization / Team Management|偏 v2.0；应在农业、能源、循环系统更实之后推进，避免先建管理壳。|
|D. Waste / Recycling|很适合作为 Agriculture 后续，因为厨余、堆肥、污染分流会直接服务土壤和材料循环。|
|E. Production System / Workshop|Manufacturing Foundation 刚 stable，建议先让农业和循环需求牵引 v0.2，而不是连续扩同域。|

建议 Batch10-A 只做规划，不直接 Apply。重点定义：

- 长期粮食生产与短期菜园的边界。
- 种子 / 育苗 / 土壤 / 病虫害 / 轮作 / 采后处理六段能力模型。
- 与 Water、Food、WASH、Manufacturing、Records 的 retrieval 边界。
- 不做现代农业设备依赖，不做农药配方大全，不做大田工业化种植。

## 8. 不建议投入方向

|方向|原因|
|---|---|
|高复杂工业制造|设备、精度、能源和安全门槛高；Manufacturing v0.1 应冻结，不应跳到工业制造。|
|专业医学深水区|Medical v0.1 已覆盖 high / critical 行动入口；继续深挖易产生高风险替代医疗。|
|大型电力工程|Energy 下一步应是低压小系统和储能规划，不是大型电力工程或并网系统。|
|大规模社会治理|组织能力应从小团队任务和资源分配开始，不能直接做社区治理体系。|
|云端系统 / 互联网依赖|LanternBox 目标是离线、断供、低资源环境；云端系统投入产出低。|
|武器制造|违反安全边界，且不服务知识系统的生存生产主线。|
|危险化学品生产|高风险、难验证、容易产生不可控伤害；不适合作为 LanternBox 知识扩展方向。|

## 9. v0.4 结论

Manufacturing Retrieval v0.1 stable 后，LanternBox 已从 v0.3 的“制造生产红区”跨入“基础生产能力已接入”的状态。当前 Green 领域足以支撑个人长期生存节点，系统重心应转向小规模自持生产。

下一阶段推荐：

```text
Batch10-A Agriculture Second Stage Coverage Planning
```

目标不是泛化农业百科，而是建立长期断供、低资源、小团队条件下的连续食物生产链：

种子保存 -> 育苗 -> 土壤恢复 -> 病虫害控制 -> 轮作 -> 收获后处理 -> 批次记录。
