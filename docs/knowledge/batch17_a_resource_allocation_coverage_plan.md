# Batch17-A Resource Allocation Coverage Plan

日期：2026-09-01

范围：仅做 Resource Allocation v0.1 Coverage Planning，不执行 Knowledge Apply、Retrieval 调整或 Contract 修改。

## 1. Domain Mission

Resource Allocation 负责在资源有限、补给不确定、决策跨越多个时间尺度时，回答：现有资源应分给哪些用途、保留多少、何时替代、何时降级、何时重新评估。

正式决策链：

`Available Resources -> Demand / Consumption -> Priority & Safety Minimum -> Reserve -> Allocation -> Degraded Mode -> Re-evaluation`

v0.1 处理物资、能源、设备能力、生产能力与基础供应，不建设 ERP、复杂优化器、人员排班系统或临床医疗 triage。它必须使用各专业域给出的可用量、安全线和技术边界，不能自行重写这些判断。

## 2. Existing Knowledge Inventory

只读审计范围：1160 篇 Markdown Wiki。Energy、Storage、Food、Water、Agriculture、Production 和 Records 已有大量局部能力；以下 25 篇可直接作为 Resource Allocation evidence，其他相邻内容不计入复用统计。

|#|Existing Wiki|Direct reuse|
|---:|---|---|
|1|`general-storage-inventory-card-minimum-001` 储藏库存卡最小字段|数量、位置、状态、领用、异常和复查字段|
|2|`general-inventory-consumption-log-001` 库存消耗记录|消耗事实与异常复核|
|3|`energy-device-consumption-baseline-001` 设备耗电基线记录|单设备实际消耗基线|
|4|`energy-offgrid-capacity-inventory-001` 小型离网能源容量盘点表|能源可用容量、未知项和未来可维持时间|
|5|`energy-daily-energy-budget-001` 每日能源预算表|当日可用量、补电、预留和任务窗口|
|6|`energy-essential-load-reserve-001` 关键负载预留量|不可挪用的关键能源 reserve|
|7|`energy-critical-device-minimum-line-001` 关键设备最低电量线|能源最低保障线和收紧动作|
|8|`energy-solar-multi-day-cloudy-degraded-mode-001` 连续阴天多日降级运行|长期发电不足的降级与恢复条件|
|9|`energy-solar-seasonal-output-baseline-001` 太阳能跨季产出基线|季节补给基线与不确定性|
|10|`energy-generation-component-failure-degraded-path-001` 发电部件故障后的隔离与降级路径|剩余能力和已验证备用路径|
|11|`food-inventory-001` 食物库存天数估算的误差来源|可用天数、损耗与不确定性|
|12|`food-rationing-priority-001` 食物短缺时先给谁吃和怎么分配|Food 域的生理需求证据；不得泛化为人员价值排序|
|13|`food-staple-food-002` 基础热量和主食配给|食物余量与任务强度联动|
|14|`food-storage-emergency-ration-reserve-line-001` 应急口粮保底线|食物 reserve 边界|
|15|`water-quantity-estimate-021` 储水数量的估算方法|水的可用数量估计|
|16|`water-drinking-water-001` 多人共用饮用水的消耗率判断|人数、天气和活动量基线|
|17|`water-drinking-water-002` 饮用水最低需求和缺水信号|Water 域最低安全输入|
|18|`water-rationing-priority-001` 缺水时饮用水和清洁水的优先级|按用途而非身份分配水|
|19|`water-shortage-response-041` 缺水时收缩用水范围|短缺降级动作|
|20|`water-usage-log-023` 用水记录与消耗跟踪|观察和偏差记录|
|21|`production-workshop-daily-resource-budget-001` 当日生产资源预算|生产任务资源需求和备用计划|
|22|`production-workshop-material-reservation-line-001` 生产材料保留线|避免当日生产耗尽跨域关键材料|
|23|`agriculture-planting-inventory-link-001` 种植计划与库存消耗联动|种植投入、预计收获与生活底线|
|24|`agriculture-water-fertility-budget-card-001` 水肥预算卡|农业资源承受能力|
|25|`records-energy-capacity-trend-001` 能源容量与产出趋势|跨周期比较 evidence，不负责当前分配|

这 25 篇 Wiki 的 47 条 `guide_links` 均指向有效 Guide，且反向 `related_wiki` 全部存在。

## 3. Existing Guide Inventory

只读审计范围：851 个 Guide。以下 18 个可直接复用；其中 14 个已有 `related_wiki`，其 78 条 outgoing evidence relation 全部双向一致。4 个 legacy Guide 没有 `related_wiki`，可作行动参考，但不能冒充完整 Guide-Wiki evidence chain。

|Guide|Reusable capability|Evidence status|
|---|---|---|
|DG-0126 库存盘点流程|账实不符时重新确认实物|legacy，`related_wiki` 为空|
|DG-0127 物资消耗异常预警|消耗偏差、临时限制、三日复查|双向 chain 完整|
|DG-0388 每周物资盘点|跨物资定期盘点|legacy，`related_wiki` 为空|
|DG-0442 食物库存天数估算|食物可维持时间|legacy，`related_wiki` 为空|
|DG-0443 基础配给线|Food 域基础份额|legacy，`related_wiki` 为空|
|DG-0551 饮用水只够一天：用途排序和配给|Water 安全用途优先级|双向 chain 完整|
|DG-0596 食物只够三天：基础配给|短期食物分段与应急备用|双向 chain 完整|
|DG-0605 少水洗手和饮用水保留|饮用与清洁用途冲突|双向 chain 完整|
|DG-0823 先在缺水时收缩用水范围|Water degraded mode|双向 chain 完整|
|DG-0824 先按人和天数做饮水配额|Water horizon budget|双向 chain 完整|
|DG-0844 每日能源预算|Energy 当日预算和补电失败降级|双向 chain 完整|
|DG-0845 关键设备最低电量线|Energy safety minimum / reserve|双向 chain 完整|
|DG-0902 储藏记录、先入先出与交接|库存事实、低库存、保底线和异常|双向 chain 完整|
|DG-0903 工坊开工前检查|生产前材料、能源和保留线检查|双向 chain 完整|
|DG-0905 原料 / 半成品 / 成品流转|材料状态与生产保留线|双向 chain 完整|
|DG-0914 重复问题与经验提取|跨周期比较和再验证|双向 chain 完整|
|DG-0919 小型离网能源容量盘点|数日到数周能源容量和 unknown|双向 chain 完整|
|DG-0920 连续阴天与长期发电不足降级运行|Energy 长期 degraded / minimum mode|双向 chain 完整|

当前最大 Guide ID 为 DG-0925。Planning 不预占新 ID；未来 Apply 应在执行时重新确认最大 ID 后顺延。

## 4. Reuse Map

|Existing owner|Resource Allocation consumes|Owner remains responsible for|
|---|---|---|
|Energy Management|当天预算、关键负载、最低电量线|当天设备调度和充电顺序|
|Energy Deepening|保守容量、季节产出、长期降级、备用路径状态|长期能源容量、拓扑、退化和韧性|
|Storage / Long-Term Storage|库存卡、隔离状态、保底物资、可用/不可用状态|保存、轮换、防潮、防损和实物 custody|
|Food / Water|最低需求、消耗率、可维持天数、用途边界|食品与饮水安全、域内配给方法|
|Agriculture|投入预算、预计收获、育苗能力上限|种植、收获和农业技术判断|
|Medical|必要物资清单、不可替代项、专业求助线|具体医疗处置、临床优先级和禁忌|
|Manufacturing / Repair|材料可替代性、设备可用状态、技术停止线|制造方法、维修方法和技术合格判断|
|Production / Workshop|任务资源需求、材料保留线、是否可延期|开工、流转、质检和制作执行|
|Records|历史消耗、可信度、版本、趋势与 review evidence|记录生命周期和长期经验沉淀|
|Team Operations|执行人可用性和交接事实|谁执行、谁值班、技能授权和排班|
|Safety / Fire / Cross-Domain Contract|Active Hazard、Human Harm 和安全 gate|危险优先级、停用、隔离、撤离和医疗 override|

复用原则：Resource Allocation 只引用各域结论并做跨资源用途决策，不复制其技术步骤。

## 5. Capability Matrix

|Capability|Status|Existing evidence|True gap / planning decision|Domain owner|
|---|---|---|---|---|
|RA-01 Resource Inventory|PARTIAL|库存卡、盘点 Guide、能源容量盘点|缺统一的 usable / unavailable / reserved / uncertain / expected replenishment 状态卡|Resource Allocation；实物状态由 Storage / 专业域确认|
|RA-02 Consumption Baseline|PARTIAL|设备耗电、用水、食物天数和库存消耗记录|缺 daily / weekly、normal / minimum / high-load 的跨资源统一口径|Resource Allocation；原始事实归 Records|
|RA-03 Safety Minimum|CROSS-DOMAIN|饮水最低需求、关键设备最低线、口粮保底、医疗储藏检查|最低线必须由 Water / Energy / Food / Medical 提供；缺聚合登记和不可挪用规则|专业域定义，Resource Allocation 执行保护|
|RA-04 Reserve Policy|PARTIAL|关键负载预留、应急口粮、生产材料保留线|缺 operational / emergency / strategic reserve 的轻量共同语义|Resource Allocation|
|RA-05 Priority Allocation|PARTIAL|Water、Food、Energy 各有域内优先级|缺按资源分别建模的跨用途冲突表；禁止单一全局排序|Resource Allocation + 专业域|
|RA-06 Allocation Horizon|MISSING|Energy 有当天和跨季片段，Food/Water 有 1-3 天片段|缺 today / 3-7 days / weeks / months 的同表比较|Resource Allocation|
|RA-07 Replenishment|MISSING|太阳能跨季产出、农业预计收获片段|缺 confirmed / expected / uncertain / none 与置信度、到达日、失约动作|Resource Allocation；来源判断归专业域|
|RA-08 Substitution|CROSS-DOMAIN|能源备用路径、材料降级、少水清洁替代|缺“替代/延期/取消”的选择框架；技术步骤不得并入本域|Resource Allocation 选路径，Repair / Manufacturing / 专业域验证|
|RA-09 Degraded Mode|PARTIAL|Energy、Water、Food 都有局部降级|缺跨资源模式卡、允许用途、暂停用途、恢复条件|Resource Allocation + 专业域|
|RA-10 Trigger Threshold|MISSING|最低电量线、短缺收缩、异常消耗已有局部 trigger|缺统一 trigger -> action -> recheck 结构|Resource Allocation|
|RA-11 Allocation Review Loop|CROSS-DOMAIN|每日能源复盘、消耗预警、Records trend|缺 Allocation 专用 Plan -> Observe -> Record -> Compare -> Adjust 入口|Resource Allocation；Records 保存 evidence|
|RA-12 Uncertainty|MISSING|Food 误差、Energy unknown、太阳能季节基线|缺 best / expected / worst 手算、保守取值和未知项升级规则|Resource Allocation|

矩阵结论：0 个能力块可仅靠现有内容标记为完整 EXISTING；5 个 PARTIAL、3 个 CROSS-DOMAIN、4 个 MISSING。知识不是空白，但缺少系统级决策骨架。

## 6. True Knowledge Gaps

确认需要新知识的仅是以下系统性缺口：

1. 跨资源统一可用量状态，而不是账面总量。
2. 把 daily / weekly 消耗与多时间尺度可维持天数放进同一保守模型。
3. 汇总各专业域最低线，但不自行发明饮水、医疗或电气安全标准。
4. operational / emergency / strategic reserve 的共同语义和启用条件。
5. 不同资源分别建模的用途优先级，避免一张全局粗暴排序表。
6. 补给确定性、到达时间、失约后的 fallback 与再评估。
7. 替代、延期、取消和 degraded mode 的决策方式，不含技术制作/维修步骤。
8. trigger、action、recheck 和恢复线。
9. Allocation review loop 与 Records evidence handoff。
10. 离线可手算的 best / expected / worst uncertainty card。

不属于缺口：水处理、食品安全、储藏轮换、能源当天排程、能源长期容量、制造/维修方法、生产流程、临床处置和团队排班。

## 7. Domain Boundaries

|Boundary|Resource Allocation primary when|Neighbor primary when|
|---|---|---|
|vs Energy Management|多个资源/任务发生冲突，或需要跨 3-7 天以上分配|仅安排今天的电量、充电口和设备顺序|
|vs Energy Deepening|使用已确认容量决定跨资源保留、替代、延期|分析长期发电、储能、转换、退化和系统韧性|
|vs Storage|决定可用库存分给何用途、保留多少|判断物资怎么保存、轮换、隔离和入库|
|vs Records|根据 evidence 形成或调整分配决定|记录、版本、可信度、趋势和归档|
|vs Production|决定任务是否获得材料/能源、是否延期或降级|开工、流转、质检、小批量制作和交接|
|vs Manufacturing / Repair|从已验证选项中选择替代路径|制作替代件、维修设备、确认技术可用性|
|vs Medical|管理医疗耗材库存、保底、补充和非临床替代路径|具体患者处置、治疗优先级、药物与临床判断|
|vs Team Operations|说明任务需要多少人时、何时必须延期|决定谁值班、谁负责、谁有技能授权|

Resource Allocation 不得以“资源珍贵”为理由覆盖任何安全停用线。

## 8. Safety / Ethical Boundaries

Safety 顺序固定：当前火灾、电气危险、电池危险、设备冒烟、化学危险或人员伤害出现时，Safety / Fire / Medical primary，Resource Allocation 后置。Batch16 Limited Production v1.0 的 LP1-LP3、Safety Gate、audit 和 allowlist 全部保持冻结，本 Batch 不修改 Contract。

硬停止线：

- 危险设备不能因为唯一、昂贵或不可替代而继续使用。
- 不明来源、不明状态、污染、过期或隔离中的资源不得计入 usable quantity。
- 低于专业域最低安全线时，先停止非关键用途，再重新预算。
- 任何替代路径必须先由对应专业域确认安全和可用。

伦理边界：

- 不建立人员价值排序、伤员价值排序或身份排除模型。
- Food / Water 的生理最低需求只能作为专业域 safety evidence，不能推导人的价值等级。
- 医疗耗材不足只讨论库存、reserve、补充、合规替代与寻求专业支持；患者临床优先级仍归 Medical。
- 分配表按用途、系统功能、安全最低线和时间约束组织，不按身份、地位或服从程度组织。

## 9. Proposed Wiki

建议未来 Apply 新增 10 篇 Resource Allocation Wiki。`expected related_wiki` 表示未来 Guide 应同时引用的现有/新 evidence 集，不是要求给 Wiki schema 新增字段。

|Slug / title|Purpose|Domain boundary|Expected related_wiki|
|---|---|---|---|
|`resource-allocation-available-resource-status-001` 可分配资源状态卡|统一 usable、unavailable、reserved、uncertain、expected replenishment|不判断保存质量或专业安全；只接收 Storage / 专业域状态|库存卡、库存消耗、能源容量盘点|
|`resource-allocation-consumption-horizon-baseline-001` 跨资源消耗基线与可维持时长|normal / minimum / high-load 的 daily / weekly 基线和 horizon|不替代 Water/Food/Energy 的计算方法|设备耗电、用水、食物库存天数|
|`resource-allocation-safety-minimum-register-001` 关键资源最低保障线登记|汇总不可突破线、来源、适用条件和下一复核|最低值由专业域提供；不做临床 triage|饮水最低、关键设备最低电量、应急口粮|
|`resource-allocation-reserve-class-policy-001` 运行、应急与战略储备分级|定义 reserve class、启用条件、授权证据和恢复要求|不处理储藏方法和人员制度|关键负载预留、应急口粮、生产材料保留线|
|`resource-allocation-resource-specific-priority-001` 按资源建立用途优先级|每种资源单独排序用途，记录冲突和拒绝原因|禁止全局人员排序；域内安全用途由专业域给出|Energy budget、Water priority、Food baseline|
|`resource-allocation-horizon-replenishment-001` 分配时距与补给置信度|today / 3-7 days / weeks / months + confirmed / expected / uncertain / none|不预测天气或农业产量，只记录来源证据和置信度|太阳能季节基线、农业库存联动、能源容量|
|`resource-allocation-substitution-degraded-mode-001` 替代路径与降级模式选择|在替代、延期、取消、降级之间做可回退选择|不包含 Repair / Manufacturing 技术步骤|能源降级、部件故障路径、材料降级 evidence|
|`resource-allocation-trigger-action-recheck-001` 分配触发线、动作与复查条件|统一 shortage / reserve / replenishment failure 的 trigger-action-recheck|不覆盖 Safety trigger；Safety 永远先行|消耗异常、缺水收缩、关键设备最低线|
|`resource-allocation-review-loop-001` 资源分配复查循环|Plan、Observe、Record、Compare、Adjust 与决定版本|长期保存归 Records；本条只定义决策复查|库存消耗、Records trend、每日生产预算|
|`resource-allocation-uncertainty-scenarios-001` Best / Expected / Worst 离线手算卡|处理错误库存、补给不确定、效率下降和天气变化|不做复杂优化或自动预测|Food 误差、Water estimate、Energy unknown / seasonal baseline|

## 10. Proposed Guides

Coverage Audit 支持新增 5 个执行型 Guide，不机械固定为 6 个。Planning 使用逻辑键，不预占 DG ID。

|Key / proposed title|Primary scenario|Capabilities|Expected related_wiki|Stop / handoff boundary|
|---|---|---|---|---|
|RA-G01 有限资源快速盘点与可用量确认|库存记录与实物不一致，需先确认真正可分配量|RA-01、RA-12|可分配资源状态卡、库存卡、库存消耗、能源容量盘点|不明、污染、隔离、危险状态不计 usable；转 Storage / Safety|
|RA-G02 未来 7 天跨资源预算|食物、水、电、燃料和关键耗材可维持时间不同|RA-02、RA-06、RA-07、RA-12|消耗时距基线、补给置信度、Food/Water/Energy baselines|专业域最低线缺失时不得补猜；采用 worst credible case|
|RA-G03 关键资源最低保障线与储备分级|普通需求将触碰 safety minimum 或 reserve|RA-03、RA-04、RA-05|最低保障线登记、reserve policy、resource-specific priority|不得挪用安全最低线；Medical / Water / Energy owner 可否决|
|RA-G04 补给中断后的替代与降级分配|补给失败或关键资源不可用，需要减少用途和选择替代|RA-07、RA-08、RA-09、RA-10|替代与降级、trigger-action-recheck、现有 Energy/Water degraded evidence|危险设备、未知替代或未经验证的技术路径立即交专业域|
|RA-G05 长期短缺资源重分配与周期复查|短缺持续数周，原计划偏差扩大|RA-06、RA-10、RA-11、RA-12|review loop、uncertainty scenarios、Records trend|每轮保留原决定和 evidence；不自行修改专业安全线|

未来 Guide 必须包含明确 `scenario`、`steps`、`check`、`stop_or_escalate`、`fallback` 和 `related_wiki`。P0/high 场景要明确 Active Hazard、污染资源、专业域最低线、医疗边界和未知替代停止线。

## 11. Cross-Domain Conflicts

|Conflict|Likely failure mode|Planning ownership rule|Status|
|---|---|---|---|
|RA vs Energy Management|“今天分电”被新域吞掉|单日纯电调度继续 Energy Management；跨资源/跨期冲突才由 RA|boundary test required|
|RA vs Energy Deepening|长期能源容量被当作资源分配|系统容量/退化 primary 为 Deepening；RA 只消费结论|boundary test required|
|RA vs Storage|库存/入库/保存查询被 RA 抢占|custody 和保存 primary 为 Storage；用途分配才是 RA|boundary test required|
|RA vs Records|盘点记录、趋势和归档被 RA 抢占|记录对象 primary 为 Records；“据此怎么重新分配”才是 RA|candidate debt|
|RA vs Production|开工与制作流程被 RA 抢占|Production 决定怎么做；RA 只决定是否给资源/延期|boundary test required|
|RA vs Medical|医疗耗材不足演变为临床 triage|库存/reserve 可由 RA；患者处置必须 Medical|high-risk boundary|
|RA vs Team Operations|资源预算演变为排班和人员控制|RA 输出任务资源窗口；谁执行归 Team Operations|boundary test required|
|RA vs Safety / Fire|唯一设备冒烟仍建议继续使用|Active Hazard / Human Harm 无条件先行；RA 后置|existing Contract protected|

当前不修改 `data/cross_domain_contracts.json`。若 Field Test 证明通用 ownership rule 缺失，只登记为 candidate debt，必须先做独立 Root Cause Review。

## 12. Field Test Plan

未来 Field Test 至少使用以下 16 个场景。Apply 阶段只准备知识，不提前写答案特判。

|ID|Query intent|Expected primary / boundary|
|---|---|---|
|A|食物够 14 天，电只够 5 天，怎么安排？|Resource Allocation；跨资源、跨 horizon|
|B|未来连续阴天，哪些用电应该保留？|Energy Deepening / Energy Management primary；RA 不吞单一 Energy 域|
|C|某耗材库存很多，但关键零件只剩一个|Resource Allocation；按资源分别设 reserve，不用总数量平均|
|D|预计 10 天后有补给，当前库存怎么分配？|Resource Allocation；expected replenishment + 10-day horizon|
|E|补给时间不确定，如何设置 reserve？|Resource Allocation；best / expected / worst 与保守 reserve|
|F|资源消耗突然比计划快 30%|Resource Allocation + Records evidence；触发再预算和复查|
|G|库存记录和实物数量不一致|先盘点确认 usable；Records / Storage evidence，未确认前不得分配|
|H|今天要不要启动高耗能生产任务？|Resource Allocation 负责 go/defer；Production / Energy 提供需求和底线|
|I|设备冒烟，但它是唯一一台设备|Energy Safety / Fire primary；不得因 scarcity 继续使用|
|J|医疗耗材库存不足|RA 处理库存/reserve；具体患者处置和临床优先级归 Medical|
|K|今天手机、照明和收音机怎么分电|Energy Management primary；RA 不吞当天纯能源调度|
|L|未来一个月太阳能和电池容量够不够|Energy Deepening primary；RA 不重算系统容量|
|M|备用零件如何防潮贴标签长期保存|Storage primary；RA 不吞 physical custody|
|N|如何制作替代支架或维修坏泵|Manufacturing / Repair primary；RA 只选择已验证方案|
|O|三个月消耗日志如何归档并找趋势|Records primary；RA 可在明确要求重分配时进入|
|P|谁值夜班、谁负责发放物资|Team Operations primary；RA 不做排班|

正式指标应包含：expected Guide hit、Wiki any/full hit、Guide-Wiki precise、owner primary、Safety first、record-check、fallback、dangerous suggestion=0、Kiwix override=0，以及 strict / observation 分离。I、J 必须作为高风险边界，不允许 partial 掩盖。

## 13. Retrieval Strategy

当前 `data/retrieval_query_profiles.json` 有 80 个 profile，没有 Resource Allocation 命名 profile。现有关键边界：

- `inventory_consumption` 会把盘点/消耗导向 Records/Data/Inventory，是 G/F 的潜在 ownership competition。
- `energy_daily_budget_priority` 必须继续保护当天 Energy Management。
- `energy_long_term_capacity_planning` 必须继续保护 Energy Deepening。
- Storage profiles 必须继续保护实体 custody。
- Production profiles 必须继续保护工坊流程。
- Batch16 Limited Production v1.0 的 allowlist 和 Safety Gate 不变。

v0.1 顺序固定：Knowledge Apply -> Field Test -> Root Cause Review -> 仅在证明 candidate selection gap 后考虑 Minimal Apply。不得提前增加 profile、top_k、selector limit、ranking 或 fallback。

**query profiles proposed now = 0**

## 14. Expected Apply Scope

下一阶段 Knowledge Apply 的预期上限：

1. 新增上述 10 篇 Resource Allocation Wiki。
2. 新增上述 5 个 Resource Allocation Guide，执行时从当前最大 Guide ID 后顺延。
3. 只为新 Guide 建立精确双向 Guide-Wiki evidence chain；不强改 4 个 legacy Guide，也不复制 stable 域正文。
4. 更新 Guide 构建产物和索引，并按 Apply 指令决定是否同步 PocketBase。
5. 运行 Wiki / Guide / Guide-Wiki 审计。
6. 不新增 query profile，不修改 Contract、Retrieval、ranking、selector、top_k、Prompt、generation 或 database schema。
7. 不处理 `.env`、`data/lanternbox.db`、FT-02、Core 或其他既有 dirty 文件。

Planning 本身只新增本报告，没有执行以上 Apply 动作。

## 15. Version Target

|Metric|Count / target|
|---|---:|
|Total Wiki audited|1160|
|Total Guide audited|851|
|Current query profiles audited|80|
|Existing reusable Wiki|**25**|
|Existing reusable Guide|**18**|
|New Wiki proposed|**10**|
|New Guide proposed|**5**|
|Query profiles proposed now|**0**|

版本路线：`Resource Allocation Knowledge v0.1 Apply Candidate`。Knowledge Apply 通过审计后再进入 Retrieval Field Test；在 Field Test、Root Cause Review 和必要的 Minimal Apply 完成前，不标记 Retrieval stable。

本计划以复用 25 篇 Wiki 和 18 个 Guide 为基础，仅补跨资源决策骨架；Domain、Safety、伦理和 Batch16 frozen boundary 均已给出可测试边界。

**READY FOR KNOWLEDGE APPLY**
