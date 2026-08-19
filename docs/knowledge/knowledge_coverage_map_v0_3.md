# LanternBox Knowledge Coverage Map v0.3

生成日期：2026-07-16

本报告只做全局知识覆盖分析和路线规划。分析对象为当前 `wiki_import/*/*.md`、`data/guides/**/*.json`、既有 Coverage Map、Batch4/5/6/7 系列报告与 Field Test 结果。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

核心判断：LanternBox 已经从“水、能源、工具、种植”扩展到“Shelter / Fire / WASH、Communication、Medical”的可验证能力链。当前主要问题不再是 P0 数量不足，而是下一圈能力的行动入口、evidence 链和 Field Test 缺失。

## 1. 当前系统总览

|项目|当前数量 / 状态|
|---|---:|
|Markdown Wiki|880|
|Guide JSON|790|
|Wiki priority|P0=575, P1=203, P2=102|
|Wiki risk_level|high=380, caution=371, normal=129|
|Guide priority|P0=579, P1=144, P2=67|
|Guide risk_level|critical=14, high=90, caution=578, normal=108|
|Guide-Wiki related_wiki 边|1967|
|有 Guide 链接的 Wiki|819 / 880|
|有 related_wiki 的 Guide|357 / 790|

v0.2 -> v0.3 主要变化：

- Wiki 从 804 增至 880。
- Guide 从 776 增至 790。
- Guide-Wiki 边从 1833 增至 1967。
- Communication Wiki 从 32 增至 64。
- Medical Wiki 从 43 增至 51，并完成 high / critical 医疗入口修复。
- Shelter / Fire / WASH 已从 Yellow 进入 stable。

## 2. Stable 能力地图

|领域|Wiki 数量|Guide 数量|Retrieval 状态|Field Test 状态|风险|
|---|---:|---:|---|---|---|
|Food / Planting|134|43|Candidate|Planting 10 / 0 / 0；但 guide_hit 仅 4 / 10，Guide-Wiki combo 9 / 10|种植基础可用；长期粮食生产、轮作、病虫害深度和采后处理仍不足。|
|Tools / Repair|127|53|Candidate|Batch4-H 10 / 0 / 0；Wave2 16 / 4 / 0|现场安全强；制造、加工、工装夹具和长期维修计划不足。|
|Energy Safety|74|72|Stable|Batch5-B 能源安全 8 / 2 / 0，后续 Minimal Apply 稳定；danger/Kiwix/cross-domain 为 0|电池、低压、异常停用稳定；发电维护和微电网仍是第二阶段。|
|Energy Management|74|72|Stable Candidate|Batch5-F 14 / 1 / 0；Guide hit 100%，Guide-Wiki precise 100%|能源预算、充电队列、太阳能低产出强；夜间照明 Wiki priority 有 1 个 partial。|
|Shelter / Fire / WASH|158|99|Stable|Batch5-N 16 / 0 / 0；Guide/Wiki/precise 100%|v0.2 stable；后续只需观察，不建议继续扩同一批。|
|Communication|64|23|Stable Candidate|Batch6-E 13 / 6 / 0；fail=0，danger=0，cross-domain=0|通信安全稳定，但 LoRa 未知节点、短波未核信息、低功耗多设备调度仍 partial。|
|Medical|51|75|Stable|Batch7-F 19 / 3 / 0；fail=0，danger=0，cross-domain=0|high / critical 入口稳定；误服药 fixture 边界和低体温专属 Wiki 是后续观察点。|

状态定义：

- Stable：fail=0，danger=0，Kiwix 越权=0，cross-domain=0，Field Test 验证完整，剩余 partial 不阻塞行动链。
- Stable Candidate：fail=0 且安全指标全绿，但仍有较多 partial 或未做 final verification。
- Candidate：有可执行能力和 Field Test，但覆盖面、Guide 命中或 evidence 组合仍不够稳定。
- Incomplete：有内容但缺行动入口、证据链或专项 Field Test。

## 3. Knowledge Coverage Matrix

评分标准：

- 0：不存在。
- 1：有少量内容。
- 2：基础覆盖。
- 3：完整覆盖。
- 4：Stable。

|领域|知识存在|Guide 行动入口|Evidence 链|Retrieval 稳定|Field Test 验证|总评|
|---|---:|---:|---:|---:|---:|---:|
|水系统|4|3|3|2|1|3|
|食物保存|3|3|3|2|1|3|
|种植基础|4|3|3|3|3|3|
|农业第二阶段|3|2|2|2|1|2|
|能源安全|4|4|4|4|3|4|
|能源管理|4|4|4|3|3|4|
|太阳能 / 发电维护|3|3|3|3|2|3|
|工具维修安全|4|4|3|3|3|3|
|制造与生产|2|1|1|1|0|1|
|Shelter / Fire / WASH|4|4|4|4|4|4|
|Communication|4|3|3|3|3|3|
|Medical|4|4|4|4|4|4|
|导航与地图|2|1|1|1|0|1|
|组织与团队|3|2|2|1|0|2|
|记录 / 知识保存|3|2|2|1|0|2|
|废弃物与循环利用|2|1|1|1|0|1|
|外出 / 撤离安全|3|3|2|2|1|2|

## 4. 红黄绿区域

### Green

|领域|依据|保持策略|
|---|---|---|
|Shelter / Fire / WASH|Batch5-N：16 / 0 / 0，Guide/Wiki/precise/safety/fallback/record 全 100%。|冻结 v0.2；只做回归，不继续扩内容。|
|Medical|Batch7-F：19 / 3 / 0，fail/danger/Kiwix/cross-domain 全 0，high/critical Guide hit 93.3%。|标记 v0.1 stable；不继续追逐 partial。|
|Energy Safety|电池异常、低压设备、湿设备停用、短路和连接异常已形成安全边界。|保持回归；下一步不做安全重复扩写。|
|Energy Management|每日预算、最低电量线、充电队列、太阳能低产出已进入 evidence。|可进入 v0.2 规划，但不是下一批最高优先。|

### Yellow

#### 水系统深化

当前水系统有 79 篇 Wiki、110 个 water 目录 Guide，并且水源、过滤、消毒、储存、配给都有基础。Yellow 的原因不是没有知识，而是缺少完整 Field Test 和“长期运行链”：

- 长期储水：有容器和污染边界，但水龄、轮换、沉淀复查、储水区巡查需要串成 Guide。
- 水源管理：有不明水源、安全水、污水接触判断；缺多水源优先级和枯水期记录。
- 水质变化判断：浑浊、异味、恢复供水复查存在；缺连续变化趋势表。
- 储水污染：有污染隔离；缺桶/盖/取水工具/手接触的综合运行表。
- 净水设备维护：过滤、煮沸存在；滤材更换、堵塞、降级用途不足。
- 灾后水系统：供水恢复复查有起点；缺灾后水网冲洗、分区用水、家庭到小队交接。

建议：先做 Water System Field Test，再决定是否 Batch8-C 扩内容。

#### 农业第二阶段

已有 planting 10 / 0 / 0，说明种子发芽、雨季积水、污染地块、草木灰、低水、幼苗、病斑、安全选址、采收、失败复盘可行动。但当前仍是“容器菜园 / 小菜园阶段”，还不是小规模自持生产：

- 种子保存：有基础，但缺年度种子账、交叉授粉、发芽率衰减和批次淘汰。
- 育苗体系：已有流程和失败复盘；缺季节排程、苗床容量、连续育苗。
- 土壤改良：有污染边界和草木灰；缺堆肥成熟判断、有机质、pH 近似判断。
- 病虫害：有隔离和病残体分流；缺常见虫害观察、物理防控、工具/浇水传播链。
- 轮作：明显不足。
- 长周期粮食生产：不足。当前更像“蔬菜补充”，不是热量主食生产。

#### 能源第二阶段

能源安全与能源管理已强，但第二阶段仍有黄区：

- 太阳能系统：已有低产出排程、天气记录和充电目标；缺板位选择、线缆损耗、角度、阴影、日维护。
- 储能规划：有电池轮换和最低线；缺多电池组、容量衰减、替换策略。
- 微电网：不建议做复杂工程，但可做低压 DC 分区、保险、标签、负载隔离。
- 能源预算：稳定；可扩到季节/连续阴天模型。
- 发电维护：手摇、车载、太阳能控制器、连接件防潮仍不足。

### Red

#### 导航与地图

当前 maps 目录 22 篇 Wiki，只有 6 篇有 guide_links；没有 `data/guides/maps` 目录，导航行动入口主要散落在 evacuation 和 communication。红色原因：

- GNSS：缺离线定位误差、坐标记录、没信号时的降级。
- 离线地图：有维护概念，但缺下载/纸图/索引/版本/标注流程行动卡。
- 路线规划：撤离路线强，但日常外出路线、补给路线、返回路线不成体系。
- 地形判断：缺坡度、河道、积水、林地、可视线、夜间危险。
- 风险路线：有危险区概念，缺路线评分、检查点、超时、撤回线。

#### 制造与生产

制造目录 23 篇 Wiki，只有 20 篇有 Guide 链接；Guide 主要在 tools/repair，缺独立生产能力链。红色原因：

- 木工：有锯切、支撑、材料判断，但缺尺寸、连接、承重、夹具和重复制作。
- 金属加工：基本缺。
- 简单机械：基本缺。
- 工具制造：有工具维修，不等于工具制造。
- 替代材料：有大量 repair Wiki，但未形成材料选择 -> 加工 -> 测试 -> 记录。
- 维修再制造：缺拆解件分类、可复用零件库、失败件隔离和复测。

#### 组织与团队

team 20 Wiki、5 个 team 目录 Guide，records/general 较多但 Field Test 缺失。红色/深黄原因：

- 成员登记：有成员档案和记录基础，但缺权限、健康、技能、风险边界。
- 任务分工：有轮值交接，缺长期职责、替补、疲劳调整。
- 资源管理：库存记录有基础，缺跨域资源预算和冲突处理。
- 决策流程：风险决策有内容，但团队决策、否决线、复盘机制不足。
- 经验传承：知识保存有点状内容，缺训练、演练、纸质 SOP、交接教学。

#### 废弃物与循环利用

相关知识分散在 contamination、hygiene、repair、manufacturing，总量不少，但没有独立运行链。红色原因：

- 分类：污染/普通/尖锐有起点，但缺废弃物路线和临时堆放。
- 再利用：repair 中有拆解利用，缺“可复用 / 禁用 / 污染 / 待检”分类。
- 材料回收：缺金属、塑料、玻璃、布料、木材的低风险复用边界。
- 污染控制：WASH 强，但废弃物生命周期弱。

## 5. LanternBox 路线规划

### v1.0：个人长期生存节点

目标：一个人或家庭在长期断供、断网、低资源环境下维持生命、安全和基本行动。

核心能力：

- 安全饮水、储水、基础净化。
- 食物保存、简单烹饪和腐败判断。
- 医疗急救 high / critical 入口。
- Shelter / Fire / WASH。
- 能源安全和能源预算。
- 工具维修现场安全。
- 基础通信窗口和记录。

必须知识：

- 水系统 Field Test。
- 导航 / 地图基础。
- 个人外出路线与返回路线。
- 最小废弃物和污染区运行。

可延期知识：

- 大规模农业。
- 金属加工。
- 微电网。
- 社区治理。

### v1.5：小规模自持生产节点

目标：小团队能持续补充食物、维修设备、维持通信和组织运转。

核心能力：

- 种植第二阶段：种子、育苗、土壤、病虫害、轮作。
- 食物生产后处理：干燥、防霉、加工、分级储藏。
- 能源第二阶段：太阳能维护、储能轮换、低压分区。
- 制造与再制造基础：木工、材料替代、拆解件库。
- 通信 v0.2：LoRa/无线电/短波/检查点与定位联动。

必须知识：

- 生产计划和季节记录。
- 工具/材料/零件库存。
- 导航通信联动 Field Test。
- 废弃物再利用和污染控制。

可延期知识：

- 精密机械。
- 高级电子维修。
- 专业农业病理。
- 大型工程施工。

### v2.0：团队 / 社区节点

目标：多个成员或小社区能持续分工、交接、决策、训练和应对外部接触。

核心能力：

- 成员登记、技能、健康、权限。
- 任务分工、轮值、疲劳、冲突处理。
- 资源预算和公平分配。
- 决策流程、复盘、经验传承。
- 多点通信和外出队管理。
- 小规模生产、维修、废弃物循环闭环。

必须知识：

- 团队组织 Field Test。
- 通信纪律和导航检查点。
- 纸质 SOP / 双备份 / 培训记录。
- 外部接触和交换风险复盘。

可延期知识：

- 大规模公共治理。
- 专业医疗体系。
- 大型能源工程。
- 复杂工业制造。

## 6. 未来 10 个 Batch 优先级

|Batch|领域|原因|预计 Wiki|预计 Guide|Retrieval 风险|
|---|---|---|---:|---:|---|
|Batch8-B|Navigation & Field Movement Planning|maps Wiki 孤立最明显，通信/撤离/外出都依赖路线和返回链。|30-40|6-8|evacuation / communication / GPS / safety 抢主位。|
|Batch8-C|Navigation & Field Movement Apply + Field Test|建立 GNSS、离线地图、纸图、检查点、风险路线、返回路线。|30-35|≤6|路线规划和撤离判断边界混淆。|
|Batch8-D|Water System Deepening Field Test|水知识很多但未专项验证，v1.0 必须补。|0-10|0-4|hygiene / medical / food 污染场景抢主位。|
|Batch8-E|Organization & Team Operations Planning|长期小团队是 v2.0 核心，当前 team Guide 少且无 Field Test。|30-40|6-8|records / psychology / security 泛化抢主位。|
|Batch8-F|Waste & Circular Use Planning|废弃物链分散，WASH 后需要闭环。|25-35|5-7|hygiene / contamination / repair 边界混淆。|
|Batch8-G|Manufacturing & Production Basics Planning|制造是红区，repair 不等于生产能力。|35-45|6-8|repair safety / tools / shelter 抢主位。|
|Batch8-H|Agriculture Second Stage Planning|种植基础 pass，但还不是长期粮食生产。|35-45|6-8|food / water / contamination 与 planting 边界。|
|Batch8-I|Food Production Post-processing Apply|采收后处理决定生产能否变储备。|25-35|5-6|food preservation / hygiene / planting 混淆。|
|Batch8-J|Energy v0.2 Solar & Storage Planning|能源已稳定，下一步做太阳能维护和储能规划。|25-35|5-6|power safety 与 energy management 互抢。|
|Batch8-K|Communication v0.2 + Navigation Link Review|通信 v0.1 candidate 仍有 6 partial，需与导航联动。|0-15|0-4|navigation / communication / external info 边界。|

## 7. 下一阶段推荐 Batch

推荐进入：

```text
Batch8-B：Navigation & Field Movement Coverage Planning
```

理由：

1. 这是当前最明显的红区：maps 22 篇 Wiki 中 16 篇无 Guide 链接。
2. 它是 Communication、Evacuation、外出安全、资源搜索和团队检查点的共同底座。
3. v1.0 个人长期生存节点不能缺“出去、回来、避开危险、说明位置”的能力。
4. 不宜先做复杂制造或社区治理；导航是更靠近生命安全和 Field Test 的基础能力。

Batch8-B 应只做规划，不 Apply。重点输出：

- 现有 maps / evacuation / communication 关联审查。
- GNSS / 离线地图 / 纸图 / 检查点 / 返回路线 / 风险路线缺口。
- 30-40 篇 Wiki 规划。
- ≤8 个 Guide 候选。
- Retrieval 风险预测：evacuation、communication、GPS、security。

## 8. 不建议投入方向

当前不建议优先建设：

- 高复杂工业制造：车床、焊接体系、精密加工、发动机维修。
- 专业医学深度：药物剂量、侵入性操作、诊断替代、高级生命支持。
- 大型能源工程：并网、逆变系统设计、大型蓄电站、复杂微电网。
- 现代互联网依赖系统：云服务、在线地图、商业通信网络、SaaS 管理。
- 高风险室内燃烧和炉具制造：不做烟囱施工、复杂炉具制造、室内烧炭操作指南。
- 武器化或对抗性内容：不把安全、外部接触、制造知识转成攻击能力。
- 采购推荐型内容：不把知识库变成商品清单。
- 大型农业专业体系：不直接进入完整农场经营、病理学、化肥农药体系。

## 9. 结论

LanternBox v0.3 的能力形态已经从“基础生存知识库”进入“多领域可验证行动系统”：

- Stable：Shelter / Fire / WASH、Medical、Energy Safety。
- Stable Candidate：Energy Management、Communication。
- Candidate：Food / Planting、Tools / Repair。
- Incomplete / Red：Navigation & Maps、Manufacturing & Production、Organization & Team、Waste & Circular Use。

下一阶段不应继续在已 stable 领域堆内容。优先从红区中选择能支撑多个系统的基础能力，建议从 Navigation & Field Movement 开始。
