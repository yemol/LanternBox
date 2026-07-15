# LanternBox Knowledge Coverage Map v0.2

生成日期：2026-07-15

本报告只做知识覆盖分析，分析对象为 `data/guides/**/*.json`、`wiki_import/*/*.md` 与现有 Field Test 报告。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval、Prompt、query profile、top_k、selector limit、fallback、schema、测试或 PocketBase。

核心问题：如果现在进入长期断供、完全离线、小团队自持场景，LanternBox 哪些能力已经可用，哪些只是零散知识，哪些仍明显缺失。

## 1. 当前知识系统总览

|项目|数量 / 状态|
|---|---:|
|Markdown Wiki|804|
|PocketBase `wiki_articles`|804|
|PocketBase `wiki_categories`|24|
|Guide JSON|776|
|Guide-Wiki 关联边|1833|
|Wiki audit|errors=0, warnings=0, advisories=0|
|Guide audit|errors=0, warnings=0, advisories=0|

判断：结构审计全绿，但能力覆盖并非全绿。当前知识库已经有大量 P0 条目和行动卡，真正短板集中在“可执行能力链是否完整”：Guide -> Wiki -> Retrieval -> Evidence -> Action -> Safety boundary -> Fallback -> Record/check。

## 2. Wiki / Guide 统计表

### 2.1 Wiki 统计

|指标|数量|
|---|---:|
|Wiki 总数|804|
|Markdown Wiki 数量|804|
|PocketBase wiki_articles 数量|804|
|有 Guide 支撑的 Wiki|745|
|无 Guide 支撑的 Wiki|59|
|guide_links 总数|1833|
|无效 Guide ID|0|

#### Wiki category 分布

|category|数量|
|---|---:|
|维修 / 制作 / 替代 / 拆解再利用|104|
|水|79|
|能源|74|
|医疗急救|43|
|食物|35|
|安全|32|
|火源 / 保温 / 通风 / 一氧化碳风险|32|
|通讯|32|
|种植与食物生产|31|
|卫生|29|
|信息保存与长期重建|26|
|团队轮值与任务管理|25|
|污染控制 / 隔离 / 清洁分区|24|
|野外食物获取 / 狩猎捕捞 / 动物蛋白补充|24|
|基础制造与材料维修|23|
|地图地形与环境监测|22|
|外部接触与物资交换风险|22|
|小规模养殖|22|
|种植|22|
|庇护空间分区|21|
|避难转移|21|
|风险决策|21|
|心理韧性与冲突降温|20|
|衣物 / 鞋袜 / 体温防护|20|

#### Wiki priority / risk_level 分布

|priority|数量|
|---|---:|
|P0|520|
|P1|183|
|P2|101|

|risk_level|数量|
|---|---:|
|high|343|
|caution|334|
|normal|127|

#### 无 Guide 支撑的 Wiki 主要类别

|目录|Wiki|未链接|判断|
|---|---:|---:|---|
|repair|100|27|维修知识沉淀多，但部分没有行动入口。|
|maps|22|16|导航解释材料明显多于 Guide 行动卡。|
|planting|53|6|种植 Wiki 较完整，少量未入行动链。|
|data|29|5|记录/备份解释层有孤立条目。|
|manufacturing|23|3|制造材料知识缺少独立 Guide 入口。|
|water|79|1|水领域基本完成链接。|
|power|74|1|能源领域基本完成链接。|

### 2.2 Guide 统计

|指标|数量|
|---|---:|
|Guide 总数|776|
|带 `related_wiki` 的 Guide|327|
|无 Wiki 支撑的 Guide|449|
|related_wiki 引用总数|1833|
|无效 Wiki slug|0|

#### Guide category 分布

|category|数量|
|---|---:|
|水与食物保障|75|
|医疗急救与照护|66|
|组织秩序与训练演练|56|
|工具维修与替代制作|55|
|撤离交通与路线|54|
|通信信息与数据安全|54|
|灾害应对与灾后恢复|50|
|居住卫生与空间管理|49|
|水|35|
|库存资料与成员档案|34|
|种植养殖与采集|31|
|能源照明与设备|24|
|个人装备与防护|22|
|心理韧性与冲突降温|20|
|外部接触与交换风险|15|
|能源|11|
|安全与风险|10|
|种植|8|
|其他小类合计|67|

#### Guide domain 分布

|domain|数量|
|---|---:|
|security|401|
|shelter|248|
|water|239|
|records|200|
|medical|178|
|evacuation|165|
|hygiene|151|
|disaster|139|
|tools|139|
|power|136|
|food|118|
|comms|106|
|planting|52|
|repair|19|
|external_contact / general / psychology|各 20|
|clothing / contamination / fire / livestock / risk_decision / team / wild_food|各 5|

说明：domain 是多值字段，不能直接等同目录能力。例如 `shelter` domain 很高，但 `data/guides/shelter` 只有 12 个 Guide。

#### Guide priority / risk_level 分布

|priority|数量|
|---|---:|
|P0|565|
|P1|144|
|P2|67|

|risk_level|数量|
|---|---:|
|caution|575|
|normal|108|
|high|79|
|critical|14|

#### 无 Wiki 支撑的 Guide 主要类别

|目录|Guide|无 related_wiki|判断|
|---|---:|---:|---|
|medical|75|57|早期医疗行动卡多，但证据链薄。|
|water|110|56|水行动覆盖很广，早期卡仍需补链路。|
|hygiene|64|51|卫生行动多，解释层联动不足。|
|evacuation|58|48|撤离行动强，但很多卡未确定加载 Wiki。|
|disaster|46|43|灾害类 Guide 基本缺支撑 Wiki。|
|power|72|38|能源新批次强，旧卡仍缺链路。|
|tools|48|24|工具类一半缺 related_wiki。|
|records|25|15|记录行动卡多于解释层。|
|general|20|17|通用卡缺证据链。|

### 2.3 Guide-Wiki 单边关系检查

|检查项|数量|结论|
|---|---:|---|
|Wiki -> Guide guide_links|1833|均指向有效 Guide。|
|Guide -> Wiki related_wiki|1833|均指向有效 Wiki。|
|Wiki 引用 Guide、Guide 未反向引用 Wiki|0|无单边关系。|
|Guide 引用 Wiki、Wiki 未反向引用 Guide|0|无单边关系。|
|无效 Guide ID|0|通过。|
|无效 Wiki slug|0|通过。|

## 3. 评分方法

|Score|含义|
|---:|---|
|0|基本不存在。|
|1|有少量零散知识，但不足以指导行动。|
|2|有 Guide / Wiki，可以执行基础任务。|
|3|形成完整能力链：Guide、Wiki、Retrieval evidence、Action、Safety boundary、Fallback、Record/check。|

Green 必须接近完整能力链；Yellow 表示可执行但链路、场景或 Field Test 不完整；Red 表示明显缺 Guide、缺 Wiki，或只有泛知识无法行动。

## 4. 六层能力地图

|Layer|子领域|状态|Score|代表 Guide|代表 Wiki|Retrieval Field Test|主要短板|建议 Batch|
|---|---|---|---:|---|---|---|---|---|
|Layer 1|水|Green|3|DG-0030 浑浊水处理；DG-0031 饮用水容器分级|water-boiling-002 浑浊水沉淀过滤煮沸的顺序；water-storage-container-002 储水容器污染风险|未见专项 Field Test|水源、净化、储存、污染隔离强；水账本/配给记录可继续整合。|否，先做 Field Test|
|Layer 1|食物|Green|3|DG-0597 无电冰箱后；DG-0606 干粮和罐头三天配给表|food-refrigerator-outage-001 停电期间冰箱食物处理；food-dry-canned-rotation-001 干粮与罐头轮换|未见专项 Field Test|保存、腐败、配给强；长期储存与烹饪燃料联动仍需整合。|否，先做 Field Test|
|Layer 1|医疗|Yellow|2|DG-0006 伤口感染；DG-0021 发热记录|medical-wound-infection-001 少水环境下手卫生和伤口感染；medical-medical-record-001 医疗记录在团队照护中的作用|未见专项 Field Test|Guide 多但 related_wiki 覆盖不足，创伤/发热/脱水 evidence 链偏薄。|是|
|Layer 2|能源|Green|3|DG-0841 电池异常隔离；DG-0844 每日能源预算|energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离；energy-daily-energy-budget-001 每日能源预算表|已测：Batch5-B 8/2/0；Batch5-F 14/1/0|少量 selector 边界仍存在，但 safety/fallback/record 均 100%。|否|
|Layer 2|工具维修|Green|3|DG-0839 刀锯作业前清场；DG-0840 工具破损后禁用标签|repair-knife-saw-clear-zone-001 刀锯作业前清场；repair-damaged-tool-disable-tag-001 工具破损后禁用标签|已测：Batch4-H 10/0/0；Wave2 16/4/0|维修现场安全强；制造深度、精细加工和长期维修计划不足。|是，偏制造深度|
|Layer 3|Shelter / 庇护|Yellow|2|DG-0656 入口缓冲区鞋物分离；DG-0657 睡眠区远离污染区复核|shelter-entry-buffer-001 入口缓冲区的作用；shelter-temporary-shelter-003 临时住所的基本边界|未见专项 Field Test|分区、防潮、睡眠区有基础；临时住所选址、搭建、防雨、长期维护不足。|是|
|Layer 3|Fire / 火源|Yellow|2|DG-0591 室内取暖通风和一氧化碳；DG-0646 室内取暖睡前火源复查|fire-indoor-combustion-carbon-monoxide-001 室内燃烧和一氧化碳；fire-fire-knowledge-002 睡前火源复查的意义|未见专项 Field Test|室内燃烧边界强；生火、火种保存、燃料获取、灰烬处理体系不足。|是|
|Layer 3|衣物与个人防护|Yellow|2|DG-0651 湿袜更换和脚部检查；DG-0652 雨衣破洞临时补片|clothing-wet-socks-001 湿袜和脚部风险；clothing-gloves-001 手套在清理和维修中的作用|未见专项 Field Test|湿冷、鞋袜、防水有基础；护目、口罩、皮肤保护、劳动防护不足。|是|
|Layer 4|种植|Green|3|DG-0671 种子干燥封存；DG-0831 作物病害隔离与工具分流|agriculture-seed-germination-test-001 种子发芽测试；agriculture-contaminated-plot-ban-record-001 污染地块禁种记录|已测：planting 10/0/0|种子、土壤、病害强；浇灌水预算和轮作仍可补强。|否|
|Layer 4|食物生产与后处理|Yellow|2|DG-0832 采收后分级防霉；DG-0677 饲料霉变停用|agriculture-post-harvest-mold-prevention-001 采收后分级与防霉隔离；agriculture-mold-001 饲料霉变和动物健康|部分种植已测，后处理未专项测|防霉和养殖安全有基础；加工、干燥、腌制、储藏体系不足。|是|
|Layer 4|维修制造与材料利用|Yellow|2|DG-0621 门窗松动低噪加固；DG-0625 拆解旧物前割伤防护|repair-door-window-001 门窗松动的受力和低暴露加固；repair-cut-risk-001 维修时的割伤和火源风险|工具维修已测，制造未专项测|材料和临时修补强；测量、固定结构、容器修补、手工制作深度不足。|是|
|Layer 5|记录体系|Yellow|2|DG-0126 库存盘点流程；DG-0145 医疗记录备份|general-inventory-consumption-log-001 库存消耗记录；general-medical-record-archive-001 医疗记录保存|能源/种植/维修局部已测|库存、能源、种植记录有点状覆盖；跨领域统一日志和交接摘要不足。|是|
|Layer 5|知识保存|Yellow|2|DG-0276 三份备份位置；DG-0143 证件防水备份|general-paper-digital-dual-backup-001 纸质和数字双备份；general-offline-directory-index-001 离线目录索引|未见专项 Field Test|Guide/Wiki 本体强；纸质知识包、离线目录恢复演练不足。|是|
|Layer 5|团队组织|Yellow|2|DG-0686 夜间轮值交接表；DG-0690 每日十分钟团队复盘|organization-handover-001 交接为什么要固定字段；organization-review-001 团队复盘的时间点|未见专项 Field Test|交班、分工有基础；成员权限、长期治理、冲突升级和心理观察不足。|是|
|Layer 6|导航|Yellow|2|DG-0198 纸质地图标注；DG-0202 道路风险分级|navigation-offline-map-maintenance-001 离线地图维护；navigation-danger-zone-marking-001 危险区域标记|未见专项 Field Test|路线和危险区有基础；maps Wiki 22 个中 16 个无 Guide 链接。|是|
|Layer 6|通信|Yellow|2|DG-0636 断网家庭固定开机窗口；DG-0639 LoRa 节点每日状态记录|communication-contact-window-001 固定开机窗口节省电量的原理；communication-radio-communication-basics-index-001 无线电通信基础索引|未见专项 Field Test|家庭通信强；LoRa、无线电、短波、通信故障排查不足。|是|
|Layer 6|转移与外出安全|Green|3|DG-0068 是否撤离判断表；DG-0666 外出找资源停止线|shelter-evacuation-001 撤离包不是搬家包；organization-outside-movement-001 外出登记的最小字段|未见专项 Field Test|撤离和风险停止线强；与通信/导航联动仍需场景测。|否，先做 Field Test|

## 5. Top 20 知识缺口

|Rank|缺口|影响|当前状态|建议 Batch|
|---:|---|---|---|---|
|1|Shelter 临时住所体系不足|长期离线时影响防雨、防潮、保温、睡眠安全|有分区和睡眠区，缺完整搭建/维护链|Batch5-I|
|2|Fire 生火与火种体系不足|影响取暖、烹饪、消毒，也带来火灾/CO 风险|室内燃烧边界强，生火体系弱|Batch5-I|
|3|衣物 / 鞋袜 / 湿冷防护不足|失温、脚伤、转移失败风险上升|5 个 Guide 可用但面窄|Batch5-I|
|4|LoRa / 无线电 / 短波基础不足|断网后小队通信弹性不足|有 LoRa 记录和短波收听，缺完整通信链|Batch5-J|
|5|导航和路线安全 Field Test 不足|外出、撤离和返回路线可能证据不稳|路线 Guide 强，maps Wiki 链接弱|Batch5-J|
|6|食物生产后处理不足|种出来但保存不了，长期生产价值下降|防霉有起点，加工/干燥/腌制弱|Batch5-K|
|7|长期组织管理不足|小团队疲劳、冲突、权限混乱|team 只有 5 个 Guide|Batch5-L|
|8|卫生与污染控制需要 WASH 整合|水、厕所、病人、厨房互相污染|卫生 Guide 多但链路分散|Batch5-I 或 Batch5-M|
|9|小规模制造深度不足|工具维修停留在临时修补，不能持续替代|repair Wiki 多，制造 Guide 少|Batch5-N|
|10|成员信息和权限管理不足|照护、交接、隐私、安全边界不稳|records/team 有点状覆盖|Batch5-L|
|11|医疗证据链不足|P0 医疗回答可能缺稳定 Wiki evidence|75 Guide 仅 18 个带 related_wiki|Batch5-O|
|12|Disaster / General 旧 Guide 缺 Wiki|灾害场景可执行但证据薄|disaster 46 个仅 3 个 linked|Batch5-P|
|13|Repair 未链接 Wiki 较多|知识沉淀不能确定进入 evidence|repair 100 Wiki 中 27 未链接|Batch5-N|
|14|Maps Wiki 孤立|地图知识难转成行动卡|22 Wiki 中 16 未链接|Batch5-J|
|15|Records Wiki 薄|记录行动缺解释层|records Wiki 只有 3 个|Batch5-L|
|16|食物长期储存体系不足|断供延长后热量和安全双风险|腐败判断强，轮换/防虫/防潮需整合|Batch5-K|
|17|纸质知识备份不足|设备损坏后知识不可访问|有纸质备份概念，缺打印包和索引|Batch5-L|
|18|通信记录格式不足|消息确认、失联复盘困难|有通信计划，记录结构弱|Batch5-J|
|19|种植浇灌与水预算联动不足|水紧张时生产系统不稳|少水浇灌有 Guide，系统调度不足|Batch5-K|
|20|转移-通信-导航联动不足|外出任务断链后救援和复盘困难|三者各自有材料，缺联合 Field Test|Batch5-J|

## 6. Top 10 已稳定能力

|Rank|能力|为什么稳定|
|---:|---|---|
|1|能源安全|Batch5-B 已测，电池异常、低压设备、未知电源均有 safety/fallback/record。|
|2|能源管理|Batch5-F 已测，每日预算、关键设备最低线、充电排班、太阳能低产出进入 evidence。|
|3|工具维修现场安全|Batch4-H 与 Wave2 已测，刀锯清场、低光停止、破损工具禁用边界明确。|
|4|种植基础|planting Field Test 10/0/0，种子、土壤、病害、记录具备证据链。|
|5|食物腐败与禁食判断|霉变、鼓包、无电冰箱、剩饭过夜等 Guide/Wiki 充足。|
|6|水净化与污染隔离|水源判断、沉淀过滤煮沸、容器分级、化学污染禁用覆盖强。|
|7|撤离判断与外出停止线|撤离、负重、路线风险、外出找资源停止线清晰。|
|8|外部接触与交换风险|20 个 external_contact Guide 全部带 related_wiki。|
|9|心理韧性基础|20 个 psychology Guide 中 18 个带 related_wiki，链路较好。|
|10|小规模养殖安全边界|5 个 livestock Guide 全 linked，饲料霉变、粪便、水源、病死动物边界明确。|

## 7. 后续 Batch 优先级建议

|优先级|Batch|主题|目标|
|---:|---|---|---|
|1|Batch5-I|Shelter / Fire / Clothing / WASH Integration|补齐环境与居住 P0 生命安全链。|
|2|Batch5-J|Communication / Navigation Field Chain|补 LoRa、无线电、短波、地图、路线记录和联合 Field Test。|
|3|Batch5-K|Food Production Post-Processing|补采后加工、干燥、腌制、防霉、储藏和种植用水调度。|
|4|Batch5-L|Long-Term Organization & Records|补成员信息、权限、交接、冲突、纸质知识包和数据恢复演练。|
|5|Batch5-N|Repair Manufacturing Depth|从临时维修扩展到测量、加工、连接、承重测试和材料替代风险。|
|6|Batch5-O|Medical Evidence Link Repair|优先补创伤、发热、脱水、隔离、医疗记录的 Guide-Wiki 链路与 Field Test。|

## 8. 不建议继续扩充的方向

- 不建议继续堆叠水、能源、基础安全的碎片 Wiki，除非能补齐行动链和 Field Test。
- 不建议为了“看起来覆盖广”新增泛化 Guide；`security`、`water`、`medical` 已经有大量早期行动卡，下一步应补链路。
- 不建议修改 Retrieval、Prompt、query profile、top_k、selector limit 或 fallback 来掩盖 evidence 缺失。
- 不建议让 Kiwix/ZIM 替代 Guide/Wiki 的行动建议。
- 不建议继续做只换标题的模板卡；审计通过不等于形成独立能力。
- 不建议先上复杂生产技术，当前优先级应是住所、火、衣物、防污、通信和组织。

## 9. 建议 Batch5-I 主题

Batch5-I 建议命名为：环境与居住核心补洞。

范围：

1. Shelter：临时住所选址、搭建、防雨、防潮、地面隔离、通风、空间分区、长期居住点维护。
2. Fire：生火、火种保存、燃料管理、室内燃烧边界、一氧化碳、灰烬处理、临时炉具、烟雾和通风。
3. Clothing/PPE：保温、防水、鞋袜、湿冷风险、皮肤保护、手套、护目、口罩等低资源防护。
4. WASH Integration：饮水、洗手、桶厕、污物、厨房、生病隔离、污染区和记录交接合成一张运行表。

验收建议：每个新增或修复对象必须有 Guide、related Wiki、双向关联、Safety boundary、fallback、record/check；完成后增加专项 Field Test。不要修改 Retrieval、Prompt、profile、top_k、selector limit、fallback、schema 或测试以掩盖缺口。

## 10. 是否需要先做更多 Field Test

需要。当前已验证最强的是能源、工具维修、种植。以下领域建议先做 Field Test，再决定是否补内容或补链路：

|优先级|Field Test|原因|
|---:|---|---|
|1|Shelter / Fire / Clothing 综合场景|这些是当前最大 P0 居住风险，且尚无专项检索验证。|
|2|Communication / Navigation / Outside Movement 联合场景|断网、外出、返回路线、留言点需要联合证据链。|
|3|Medical P0 场景|创伤、发热、脱水、隔离 Guide 多但 Wiki 链接不足。|
|4|Food Storage / Post-Processing 场景|食物保存强，生产后处理弱，需要验证 evidence 是否进入。|
|5|Long-Term Records / Organization 场景|长期自持依赖交接和权限，当前缺实测。|

## 11. 验证

本阶段只运行以下审计：

```bash
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
```

当前结果：

```text
Wiki audit:
errors=0
warnings=0
advisories=0

Guide audit:
errors=0
warnings=0
advisories=0
```
