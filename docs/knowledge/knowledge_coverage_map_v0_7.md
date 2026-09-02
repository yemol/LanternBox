# LanternBox Knowledge Coverage Map v0.7

生成日期：2026-08-24

本阶段只做知识覆盖分析和路线更新。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`；未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、tests、fixture、`.env`、`data/lanternbox.db` 或无关 dirty 文件，也未执行 Apply。

参考基线：

- `docs/knowledge/knowledge_coverage_map_v0_6.md`
- `docs/knowledge/batch9_g_manufacturing_final_verification.md`
- `docs/knowledge/batch10_f_agriculture_second_stage_final_verification.md`
- `docs/knowledge/batch11_f_waste_recycling_final_verification.md`
- `docs/knowledge/batch12_g_long_term_storage_final_verification.md`
- `docs/knowledge/batch13_g_production_system_workshop_final_verification.md`

## 0. 当前规模与核心判断

|项目|v0.6|v0.7|变化|
|---|---:|---:|---:|
|正式 Markdown Wiki|1066|1107|+41|
|Guide|828|836|+8|
|有 `related_wiki` 的 Guide|395|403|+8|
|Guide-Wiki `related_wiki` 边|2264|2321|+57|
|Retrieval query profile|68|74|+6|

变化全部来自 Production System / Workshop v0.1：41 篇 Wiki、8 个 Guide、57 条双向 evidence 边和 6 个窄范围 retrieval profile。

核心判断：

1. **LanternBox v1.0 Personal Long-Term Survival Node 已完成，应正式冻结。**
2. Agriculture Second Stage、Manufacturing Foundation、Waste / Recycling、Long-Term Storage 和 Production System / Workshop 已构成 v1.5 的小规模自持生产主链。
3. Energy Safety / Management 已能支撑 v1.5 的低压、安全、可降级运行；Energy System Deepening 是韧性增强项，不是当前首要阻断项。
4. 当前最大系统缺口是：跨域记录很多，但没有形成可审核、可版本化、可复用、可交给新成员的长期知识传承闭环。
5. 下一阶段唯一推荐：**Long-Term Records / Knowledge Transfer Coverage Planning**。

## 1. Stable Domain 总览

下表中的复合域统计允许重叠，不可相加为全局总数。例如 Agriculture Second Stage 是 Planting / Food Production 的子集，Energy Safety 与 Energy Management 使用同一 `power` 知识池。

|Domain|Wiki|Guide|Retrieval|Field Test|状态|是否冻结|
|---|---:|---:|---|---|---|---|
|Planting / Food Production|174|51|v0.1 stable candidate|Planting 10 / 0 / 0；Guide hit 4 / 10|Green/Yellow，基础种植与食物生产可行动|是，冻结基础入口，不追局部 hit|
|Agriculture Second Stage|40|8|v0.1 stable|24 / 0 / 0|Green，种子、育苗、土壤、病虫害、轮作、采后处理闭环|是|
|Tools / Repair|104|53|核心 v0.1 stable candidate|Core 10 / 0 / 0；Wave2 16 / 4 / 0|Green/Yellow，工具安全和维修行动入口可用|是，剩余 partial 不扩 profile|
|Manufacturing Foundation|61|8|v0.1 stable|16 / 2 / 0|Green，具体制作、连接、固定、承重检查稳定|是|
|Waste / Recycling|36|8|v0.1 stable|24 / 0 / 0|Green，分类、隔离、材料池和交接稳定|是|
|Long-Term Storage|40|8|v0.1 stable|26 / 0 / 0|Green，分区、标签、FIFO、轮换、隔离、交接稳定|是|
|Production System / Workshop|41|8|v0.1 stable|Strict 20 / 0 / 0；Full 21 / 3 / 2|Green，流程、质检、小批量、安全记录和班次交接稳定|是|
|Energy Safety|74|72|v0.1 stable|8 / 2 / 0；fail=0|Green，异常停用、隔离和低压安全边界稳定|是|
|Energy Management|74|72|v0.1 stable candidate|14 / 1 / 0；fail=0|Green/Yellow，每日预算、负载优先级和充电排程可用|是，深层系统另立域|
|Shelter / Fire / WASH|158|99|v0.2 stable|16 / 0 / 0|Green，庇护、火灾与卫生行动链稳定|是|
|Communication|65|23|v0.1 stable candidate|13 / 6 / 0；fail=0|Green/Yellow，离线通信基础可用|是，v0.2 另行规划|
|Medical|51|75|v0.1 stable|19 / 3 / 0；fail=0|Green，高风险医疗行动入口稳定|是|
|Navigation|51|6|v0.1 stable|18 / 0 / 0|Green，个人定位、路线风险和返回行动链稳定|是|

冻结原则：已 stable 或 stable candidate 的主域，不继续扩 profile、不做局部 ranking 微调、不为清理 partial 增加 Wiki/Guide。只有新的独立 Field Test 和 Root Cause Review 证明存在系统性根因时，才能另开 Batch。

## 2. Capability Matrix v0.7

等级定义：

- 0：基本无覆盖
- 1：零散知识
- 2：Wiki 基础覆盖
- 3：Guide / evidence 行动入口
- 4：Retrieval + Field Test stable

评分是系统能力判断，不等于文件数量。`Knowledge / Guide / Evidence / Retrieval / Field Test` 采用 0-4 成熟度，最终等级取能否形成完整行动入口的综合结果。

### 2.1 Survival Foundation

|能力域|Knowledge|Guide|Evidence|Retrieval|Field Test|等级|主要缺口|
|---|---:|---:|---:|---:|---:|---:|---|
|Water|4|3|3|2|1|3|长期取水、净水设备维护和轮换未做独立 stable；不阻断 v1.0。|
|Food|4|3|3|3|3|3|长期粮食配给与跨季产量接口可在资源分配阶段深化。|
|Medical|4|4|4|4|4|4|已 stable；不进入专业医学深水区。|
|Shelter|4|4|4|4|4|4|已随 Shelter / Fire / WASH v0.2 stable。|
|Fire|4|4|4|4|4|4|火源、余火、燃料和撤离边界稳定。|
|WASH|4|4|4|4|4|4|卫生、污染隔离和生活区分区稳定。|
|Navigation|4|4|4|4|4|4|个人导航 stable；团队移动和 FT-02 深度融合留后续独立版本。|
|Communication|4|3|3|3|3|3|核心 fail=0；Mesh、短波和多设备同步留 v0.2。|

### 2.2 Self-Sustaining Production

|能力域|Knowledge|Guide|Evidence|Retrieval|Field Test|等级|主要缺口|
|---|---:|---:|---:|---:|---:|---:|---|
|Agriculture|4|3|3|3|3|3|基础种植可行动；不再作为近期 profile 扩展目标。|
|Agriculture Second Stage|4|4|4|4|4|4|跨季农业生产循环 stable，冻结。|
|Tools / Repair|4|4|4|4|4|4|核心安全和维修入口稳定；长期维护历史转 Records。|
|Manufacturing Foundation|4|4|4|4|4|4|具体制作方法 stable，冻结。|
|Waste / Recycling|4|4|4|4|4|4|废弃循环和材料池 stable，冻结。|
|Long-Term Storage|4|4|4|4|4|4|长期保存与库存交接 stable，冻结。|
|Production System / Workshop|4|4|4|4|4|4|开收工、WIP、质检、小批量、复盘、交接 stable，冻结。|
|Energy Safety|4|4|4|4|4|4|低压异常停用和电池隔离 stable，冻结。|
|Energy Management|4|4|4|3|3|3|预算、负载优先级和排程可用；系统维护和低压拓扑尚未成闭环。|

### 2.3 Long-Term System Capability

|能力域|Knowledge|Guide|Evidence|Retrieval|Field Test|等级|主要缺口|
|---|---:|---:|---:|---:|---:|---:|---|
|Energy System Deepening|3|2|2|1|1|2|缺低压系统拓扑、固定维护、备用切换和跨季发电/容量退化闭环。|
|Long-Term Records / Knowledge Transfer|3|3|2|1|1|2|跨域记录分散；缺统一索引、可信度、版本、审核、SOP 晋升和教学交接链。|
|Resource Allocation|2|1|1|0|0|1|水、食物、能源、材料、工具、医疗资源尚无统一冲突处理入口。|
|Organization / Team Management|3|2|2|1|1|2|成员、技能、分工和值班片段存在，未形成稳定团队运行系统。|
|Team Safety / Duty Rotation|3|3|2|2|1|2|轮值、疲劳和交接有行动卡，缺累计负荷、替补授权和长期 Field Test。|
|Security / Risk Decision|3|3|2|1|1|2|风险判断内容多，但与生产中断、资源冲突和团队升级流程未成 stable。|
|Governance / Decision Flow|2|2|1|0|0|1|缺否决线、责任边界、争议处理、执行跟踪和版本化决定。|
|Skills / Training|2|2|1|0|0|1|有五分钟教学和技能冗余片段，缺技能档案、授权级别、训练计划与复测。|

## 3. v1.0 Personal Long-Term Survival Node

|检查项|状态|冻结判断|
|---|---|---|
|Water|水源、过滤、煮沸、储水和污染边界可行动。|满足 v1.0；长期水系统深化另立版本。|
|Food|食物安全、储食、基础配给和农业输入可行动。|满足 v1.0。|
|Medical|Retrieval v0.1 stable。|冻结。|
|Shelter / Fire / WASH|v0.2 stable。|冻结。|
|Energy|Safety stable；Management fail=0，可预算、降级和安排关键负载。|满足个人节点，不要求微电网。|
|Tools / Repair|核心 Field Test 10 / 0 / 0。|满足日常维护与故障隔离。|
|Communication|v0.1 candidate fail=0。|满足离线联络基础。|
|Navigation|v0.1 stable。|满足个人定位、路线和返回。|

结论：**LanternBox v1.0 Personal Long-Term Survival Node 已完成，正式冻结。**

少量 cross-domain observation、stable candidate 的合理 partial、以及更深的水/能源/通信能力都不重新打开 v1.0。未来扩展必须以独立版本和真实能力目标命名，而不是继续追逐 v1.0 局部指标。

## 4. v1.5 Small Self-Sustaining Production Node

### 4.1 生产主链

当前五个 stable 域已经形成以下链条：

```text
Agriculture Second Stage
  -> 资源输入与农业产出
Production System / Workshop
  -> 任务、批次、WIP、质检、交接
Manufacturing Foundation
  -> 具体制作、连接、固定、承重检查
Waste / Recycling
  -> 废料分类、材料池、污染隔离、降级
Long-Term Storage
  -> 成品/原料长期保存、FIFO、轮换、库存交接
```

它对应：

```text
资源输入 -> 生产 -> 制造 -> 质量与流程 -> 废弃循环 -> 长期保存
```

边界已经清楚：Manufacturing 负责“怎么做”，Production 负责“如何重复、检查和交接”，Waste 负责“材料是否可回收和如何分流”，Storage 负责“如何长期保存”。

### 4.2 v1.5 当前判断

v1.5 的物质生产主链已经成立，但长期运行反馈链尚未成立。当前系统可以生产并保存物资，却不能稳定回答：

- 哪个操作经验已被验证多次？
- 哪条 SOP 是当前有效版本？
- 一次失败如何变成下一版行动卡？
- 原记录的可信度、来源和适用条件是什么？
- 关键技能只有一个人会时，如何形成可验证的学习材料？
- 新成员应先学什么、哪些任务必须监督、何时才能独立执行？

因此 v1.5 的下一缺口不是再扩生产知识，而是建立 **Long-Term Records / Knowledge Transfer**，把 stable domains 的运行结果变成下一轮可复用能力。

## 5. Long-Term Records / Knowledge Transfer 审查

### 5.1 当前数量

统计口径：目录总量用于判断基础规模；“显式记录资产”按标题含记录、日志、台账、交接、复盘、档案、观察、索引、技能等词识别，用于比较各域是否已经产生可沉淀对象，不等同于未来规划数量。

|范围|当前数量|evidence 状态|判断|
|---|---:|---|---|
|`wiki_import/records`|3 Wiki|2 篇明确记录条目|窄目录过小，只覆盖成员日查、异常观察和决定记录。|
|`wiki_import/data`|29 Wiki；17 个显式记录/归档资产|纸本、双备份、日志、档案、可信度、目录索引已有|是长期知识基础池，但未组织为统一 Records 域。|
|`data/guides/records`|25 Guide|10 个有 `related_wiki`；11 条边|Guide 数量不少，但 15 个 Guide 无 evidence chain，关系密度低。|
|Team / Organization|20 Wiki；5 Guide|5 Guide 均有链；66 条边|交接、复盘、轮值、新成员和技能冗余有模板，可作为相邻域。|
|Production records|41 Wiki 中 11 个显式记录资产|8 Guide 已 stable|任务状态、质量、near miss、班次和 WIP 交接已产生高质量记录对象。|
|Repair records|100 Wiki 中 13 个显式记录资产|维修、工具寿命、借还、失败和事故记录存在|内容强，但仍停留在 Repair 内部。|
|Agriculture records|93 Wiki 中 16 个显式记录资产|批次、发芽率、土壤恢复、产量、失败和任务交接存在|跨季数据已出现，缺版本化经验总结。|
|Storage records|至少 8 个库存卡、交接卡、日历、日志和标签资产|Storage Retrieval stable|可作为统一记录字段、批次和复查节奏的成熟模板。|
|Medical observation records|51 Wiki 中 15 个记录/观察/索引标题|Medical v0.1 stable|观察记录丰富，但隐私、访问级别、照护交接和长期摘要未统一。|
|Waste records|36 Wiki 中 5 个显式记录资产|材料池、来源风险、失败和交接存在|可作为材料生命周期事实输入。|
|Energy logs|`power` 8 个记录指标；`data` 另有 7 个明确能源日志资产|预算、天气产出、充放电、设备耗电和交班已有|运行日志充分，长期趋势和维护决策尚未统一。|

### 5.2 能力检查

|能力|现有状态|成熟度|主要缺口|
|---|---|---|---|
|每日日志|事件日志、成员日查、每日用电复盘、团队短会存在|有基础|缺跨域一页摘要、未完成事项和异常索引。|
|生产记录|任务状态、批次、质量、near miss、交接存在|较强|缺批次结果到 SOP 修订的晋升规则。|
|失败复盘|通用失败、生产、维修、农业复盘均存在|较强|缺原因分类、重复性判断、验证后关闭和版本更新。|
|维修记录|维修对象、材料、限用、复查、寿命和失败记录存在|较强|缺重复故障聚合、替代件历史和跨设备经验卡。|
|种植记录|种子、土壤、病虫害、产量、采后与失败记录存在|较强|缺跨季对比、地点/天气上下文和经验版本。|
|储藏记录|库存卡、FIFO、异常、复查日历和交接 stable|成熟模板|需要向其他域提供字段与轮检范式，不扩 Storage profile。|
|医疗观察记录|体温、尿量、精神状态、伤情变化等已有|有基础|缺隐私级别、照护交接摘要、结束条件和长期归档规则。|
|事故 / near miss|Production、Repair、Team 均有|有基础|缺统一事件编号、整改验证、重复风险趋势和关闭状态。|
|交接记录|Production、Storage、Repair、Team、Energy 均有|分域成熟|缺跨域交接摘要和优先级冲突处理。|
|技能档案|成员信息、技能双人掌握、工具培训记录存在|薄弱|缺技能矩阵、授权级别、禁做任务、监督人和复测日期。|
|操作经验|失败经验、知识卡、维修与种植经验存在|薄弱|缺经验可信度、适用条件、反例和复现次数。|
|版本 / 流程变化|地图版本、记录可信度、长期档案轮检有片段|缺口明显|缺 SOP 版本号、变更理由、生效/废止、回滚和旧版隔离。|
|新成员学习材料|新成员信息交接、五分钟技能教学、FieldManual 已有|薄弱|缺岗位学习路径、低风险首任务、考核、授权和复训闭环。|

### 5.3 系统性缺口

现状不是“没有记录”，而是 **记录对象多、跨域结构弱、知识晋升链缺失**：

```text
现场日志 / 观察 / 批次 / 失败
  -> 可信度与来源标注
  -> 复盘与重复性判断
  -> SOP / Knowledge Card 候选
  -> 审核与 Field Test
  -> 当前有效版本
  -> 新成员学习与授权
  -> 新运行记录反馈
```

`docs/KNOWLEDGE_STRATEGY.md` 已定义 Team Knowledge 默认进入 Knowledge Inbox，审核后才能进入 Guide、Wiki 或 Archive；当前知识库还没有把这条战略转成可检索、可执行、可交接的行动体系。

与 LanternBox / FT-02 的结合价值很高：软件已有日记、物资记录和终端同步服务；FT-02 已有 SD 存储、现场日志、录音日志、轨迹记录和 FieldManual。Records 下一阶段可以定义离线记录如何压缩、索引、导出、审核和进入行动卡，但不在 Coverage Planning 阶段修改应用或固件。

## 6. Energy System Deepening 审查

### 6.1 当前能力

|检查项|现有覆盖|成熟度|深化缺口|
|---|---|---|---|
|Solar|面板基础使用、光照温度、阴天降级、白天排程、目标选择、天气产出记录|3|缺固定板位复查、清洁/遮挡维护、控制器状态和跨季产出基线。|
|Battery|容量估算、温度、老化、鼓包、漏液、混用禁线、备用轮换|3|缺统一健康分级、容量复测周期和可用容量趋势。|
|Charge / discharge|充电队列、无人值守停止线、充放电记录、接口轮换|3|缺跨设备效率比较、损耗和周期性退化判断。|
|Low-voltage distribution|正负极、保险件、线缆发热、临时接线、低压异常停用|2|缺低压 DC 分区图、线缆/保险标签、支路隔离和固定配电检查表。|
|Load priority|设备优先级、关键负载预留、低电量断开顺序|4|已足以支撑 v1.5 降级运行；跨域冲突留 Resource Allocation。|
|Energy budget|每日预算、Wh/mAh 估算、续航卡、夜间照明计划|4|连续阴天和跨季模型可深化，但非当前阻断。|
|Backup power|备用电池封存轮换、小型发电机使用边界|3|缺备用切换演练、备用源可用性检查和启动失败降级。|
|Battery maintenance|存放、轮换、老化记录和异常隔离存在|2|缺安全范围内的清洁、端子检查、容量复测与退役周期；不进入危险维修。|
|Generation maintenance|太阳能位置/防水、发电机安全边界存在|1|缺低风险维护计划、故障转专业维修和维护日志。|
|Energy records|能源日志、耗电基线、太阳能天气产出、交班摘要、充放电记录|3|记录对象足够，长期趋势分析应承接 Records。|
|Emergency load shedding|非必要设备停用、低电量断负载顺序|4|行动入口已可用。|
|Small microgrid concept|发电、储能、负载与保护知识分散|1|尚无小型离线低压系统图；不得扩成工业微电网或专业电气工程。|

### 6.2 安全边界

Energy System Deepening 只允许研究低压、离线、小功率、可整体停用的系统。明确不进入：

- 高压电。
- 市电配电施工。
- 大型电站或并网系统。
- 危险电池拆解、维修或重组。
- 工业级微电网。
- 替代专业电气工程师的设计、施工或验收。

### 6.3 对 v1.5 的判断

Energy Safety / Management 已提供：异常停用、充电排程、关键负载、每日预算、太阳能低产出降级、备用轮换和能源交班。它足以让 v1.5 生产节点在电量不足时缩小批次、暂停高风险加工、保留医疗/通信/照明负载，而不是强行维持连续生产。

因此 Energy System Deepening 能显著提高可预测性和韧性，但 **不是 v1.5 当前最先补的系统缺口**。它更适合在 Records 建立后推进，因为电池衰减、太阳能产出、维护周期和备用切换都依赖长期记录事实。

## 7. Records 与 Energy 比较

|维度|Records / Knowledge Transfer|Energy System Deepening|
|---|---|---|
|对现有 stable domains 的连接价值|极高；直接连接 Agriculture、Repair、Manufacturing、Waste、Storage、Production、Medical、Energy。|高；主要连接 Production、Communication、Medical、Water 和照明。|
|对长期运行的重要性|决定经验能否跨人员、跨季节、跨版本保留。|决定电力可预测性、持续时间和设备可用性。|
|当前知识缺口|不是内容少，而是跨域索引、可信度、审核、版本、SOP 晋升和教学链缺失。|发电/储能/低压配电/维护的系统闭环不足，但基础行动知识已多。|
|当前 Guide 缺口|Records 25 Guide 中仅 10 个有 related Wiki；缺跨域日志汇总、经验审核、版本变更、教学授权入口。|已有预算、充电、太阳能、电池异常和负载 Guide；缺系统启停、维护和备用切换 Guide。|
|Retrieval 风险|高；“记录、日志、交接、复盘”跨所有域，容易被具体域抢主位或反向吞域。|中高；Energy Safety、Management、Repair、Storage 容易重叠，但已有 7 个相关 profile 和 stable 基线。|
|安全风险|低到中；主要是隐私、错误经验固化、旧版 SOP 误用和记录可信度。|高；接线、电池、发电机、火灾和触电边界要求严格。|
|与 FT-02 / LanternBox 实际功能结合|极高；已有 journal、inventory、terminal sync、音频日志、轨迹记录、SD 和 FieldManual。|高；FT-02 需要电量预算和备用供电，但系统级能源深化更多依赖硬件实测。|
|对 v1.5 完成度提升|直接补齐生产系统的反馈、复盘、版本和人员交接闭环。|增强运行时长和能源韧性，但不解决经验丢失与人员替换。|
|对 v2.0 承接能力|是 Organization、Skills、Resource Allocation、Governance 的事实和审计前置。|是 Resource Allocation 和基础设施韧性的前置，但不能独立支撑治理和训练。|
|前置依赖|Production / Storage 已 stable，条件成熟。|最好先有稳定的能源日志、维护历史和版本化操作卡；Records 可提供这些。|

选择：**Records / Knowledge Transfer 优先。**

不是 Energy 的原因：Energy Safety / Management 已足以支撑 v1.5 的安全降级运行；立即深化会进入较高安全风险和硬件实测依赖，而 Records 能先把现有能源日志、生产复盘和维护历史组织起来，反过来为后续 Energy Deepening 提供真实容量与故障数据。

## 8. Known Cross-Domain Debt

|Debt|性质|当前表现|严重度|是否阻断 roadmap|处理意见|
|---|---|---|---|---|---|
|`salvaged_wood`|Waste / Manufacturing semantic drift|旧木板材料可用性查询 top1 偏到 `DG-0830`；Production 未占主位。|Medium|否|登记到后续 Cross-Domain Retrieval Debt Review；不重新打开 Production、Manufacturing 或 Waste。|
|`workshop_injury`|Medical / Workshop injury cross-domain gap|已受伤流血时 top1 仍为 `DG-0839` Repair/Safety，而非 Medical；Production 未占主位。|High（安全语义）|否，不阻断 Production v0.1 或当前 roadmap|登记到后续 Medical cross-domain review；不使用 Production profile 修复，也不在本阶段重开 Medical。|

两项债务都不是 Production 知识缺口。它们不推翻 Manufacturing、Waste、Medical 或 Production 的 frozen 状态；后续只有独立跨域 review 可以决定是否存在系统性入口问题。

## 9. Future Batch Roadmap

近期不再把 Agriculture Second Stage、Manufacturing Foundation、Waste / Recycling、Long-Term Storage 或 Production System / Workshop 作为建设主域。

|顺序|建议 Batch 方向|目标|边界|
|---:|---|---|---|
|1|Batch14-B Long-Term Records / Knowledge Transfer Coverage Planning|定义记录生命周期、跨域字段、可信度、版本、SOP 晋升、归档与新成员学习边界。|只规划，不 Apply；不把所有日志拉进同一泛入口。|
|2|Batch14-C Records / Knowledge Transfer Knowledge Apply|建立少量系统级 Wiki/Guide 和双向 evidence chain，复用现有跨域记录。|不复制各 stable 域的具体记录正文。|
|3|Batch14-D Records Retrieval Field Test|验证日志、复盘、版本、归档、经验晋升、技能交接和 FT-02 离线记录场景。|只测试，不直接修。|
|4|Batch14-E Records Root Cause Review|区分知识缺口、Guide 缺口、generic records 抢域、具体域主位和 selector 截断。|遵守根因政策，不扩 top_k。|
|5|Batch14-F Records Minimal Apply|只修 review 证明的最小入口问题。|不做 query 级硬编码，不吞 Production/Medical/Storage 等域。|
|6|Batch14-G Records Final Verification|审计、contracts、Field Test 和冻结。|达到 stable 后停止扩 profile。|
|7|Batch15 Energy System Deepening Coverage Planning|规划低压系统图、维护、容量趋势、备用切换和跨季太阳能产出。|不进入高压、市电施工、危险电池维修或工业微电网。|
|8|Batch16 Resource Allocation|连接 Storage 库存、Production 消耗、Energy 预算和 Medical 保底线。|先建立透明优先级与停止线，不做自动 ERP。|
|9|Batch17 Team Operations|依次规划 Organization / Team Management、Team Safety / Duty Rotation、Skills / Training。|以 Records 的技能档案和交接事实为前置，不做社会控制。|
|10|Batch18 Cross-Domain Debt + Final v1.5 Capability Verification|独立审查已登记 retrieval debt，并验证生产、记录、能源和资源接口。|不为清零 observation 重开 stable 主域；Security / Governance 留 v2.0。|

v2.0 后续队列：Security / Risk Decision、Governance / Decision Flow。它们应在 Records、Resource Allocation 和 Team Operations 有 stable 事实基础后推进。

## 10. Coverage Map 文件整理建议

只给建议，不移动文件。

|文件|定位|建议|删除意见|
|---|---|---|---|
|`knowledge_coverage_map_v0_3.md`|早期中间态|可归档|不建议删除，保留历史判断。|
|`knowledge_coverage_map_v0_3_final.md`|v0.3 final milestone|保留|不建议删除。|
|`knowledge_coverage_map_v0_4.md`|Manufacturing stable 后的 milestone|保留|不建议删除。|
|`knowledge_coverage_map_v0_5.md`|Agriculture / Manufacturing / Waste stable 后的 milestone|保留|不建议删除。|
|`knowledge_coverage_map_v0_6.md`|Long-Term Storage stable、Production 前的主地图|保留为上一版主地图|不建议删除。|
|`knowledge_coverage_map_v0_7.md`|Production stable 后的全局状态|当前主地图|不建议删除。|

原则：stable milestone 与 final 文件保留；非 final 中间态可以归档，但不删除仍被后续报告引用的历史地图。

## 11. 下一阶段唯一推荐

选择：

> **A. Long-Term Records / Knowledge Transfer Coverage Planning**

### 为什么选择它

- Production stable 后，系统首次持续产生任务、批次、WIP、质检、失败、near miss 和交接事实。
- Agriculture、Repair、Storage、Medical、Waste 和 Energy 也已有大量分域记录，但缺统一的长期知识生命周期。
- Records 能把一次运行结果转成可信、可审核、可版本化、可复用的 SOP 与学习材料。
- LanternBox 软件和 FT-02 已具备 journal、inventory、终端日志、录音、轨迹、SD 与 FieldManual，实际结合路径清楚。
- 它是 Resource Allocation、Organization、Skills / Training 和 Governance 的共同前置。

### 为什么不是 Energy System Deepening

- Energy Safety 已 stable，Energy Management fail=0，当前已经能做预算、负载优先级、充电队列、太阳能降级和紧急减载。
- v1.5 可以在能源不足时降级生产，不要求连续供电或微电网。
- Energy 深化涉及更高安全风险与硬件实测；在没有长期容量、产出和故障记录体系前推进，容易写成抽象系统设计。
- Records 先完成后，Energy Deepening 可以基于真实耗电、天气产出、电池衰减和维护历史规划。

### 如何承接 Production System stable

承接对象不是制造步骤，而是 Production 已稳定产生的运行事实：

```text
任务板 / 批次标签 / 质检记录 / 失败批次 / near miss / 班次交接
  -> 可信度与来源
  -> 复盘与改进项
  -> SOP 候选与版本
  -> 审核 / Field Test
  -> 当前有效行动卡
  -> 新成员学习与授权
```

### 对 v1.5 完成度的影响

Production 解决“今天怎样稳定做事”，Records / Knowledge Transfer 解决“几个月后换人、换季、重复失败时，系统是否仍知道为什么这样做”。完成该方向后，v1.5 将从物质生产闭环升级为带反馈、学习、版本和人员替换能力的长期自持节点。

## 12. 结论

LanternBox 当前状态：

- **v1.0 Personal Long-Term Survival Node：完成并正式冻结。**
- **v1.5 Small Self-Sustaining Production Node：物质生产主链完成，长期学习反馈链未完成。**
- **v2.0 Team / Community Node：尚未进入，等待 Records、Resource Allocation 和 Team Operations。**

下一阶段不继续修 Agriculture、Manufacturing、Waste、Storage 或 Production，也不先进入高风险 Energy 深化。唯一推荐为：

> **Batch14-B：Long-Term Records / Knowledge Transfer Coverage Planning**

本阶段到此结束，不执行 Apply。
