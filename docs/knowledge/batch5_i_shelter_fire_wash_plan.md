# Batch5-I Shelter / Fire / Clothing / WASH Integration Plan

生成日期：2026-07-15

本报告只做规划，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval、Prompt、query profile、top_k、selector limit、fallback、schema、测试或 PocketBase。参考 `docs/knowledge/knowledge_coverage_map_v0_2.md`，目标是规划环境与居住核心补洞：在长期断供、低温潮湿、临时住所、小团队居住条件下建立可执行生存链。

## 1. 当前覆盖审查

### 1.1 目录统计

|方向|Wiki 目录|Wiki 数量|Wiki 链接状态|Guide 目录|Guide 数量|Guide related_wiki 状态|判断|
|---|---|---:|---|---|---:|---|---|
|Shelter|`wiki_import/shelter`|20|20 linked / 0 unlinked|`data/guides/shelter`|12|7 linked / 5 unlinked|Wiki 分区强，Guide 行动入口仍偏小批次。|
|Fire|`wiki_import/fire`|30|30 linked / 0 unlinked|`data/guides/fire`|5|5 linked / 0 unlinked|火源风险边界强，Guide 数量偏少。|
|Clothing / PPE|`wiki_import/clothing`|20|20 linked / 0 unlinked|`data/guides/clothing`|5|5 linked / 0 unlinked|湿冷、鞋袜、防水有基础，PPE 深度不足。|
|Hygiene|`wiki_import/hygiene`|32|32 linked / 0 unlinked|`data/guides/hygiene`|64|13 linked / 51 unlinked|Wiki 解释层强，早期 Guide 链路弱。|
|Contamination|`wiki_import/contamination`|21|21 linked / 0 unlinked|`data/guides/contamination`|5|5 linked / 0 unlinked|污染隔离小批次完整。|
|Water support|`wiki_import/water`|79|78 linked / 1 unlinked|相关 Guide 不在本批主改|110 water Guide|54 linked / 56 unlinked|水强，但 WASH 中只作为补充证据。|
|Food support|`wiki_import/food`|35|35 linked / 0 unlinked|相关 Guide 不在本批主改|21 food Guide|19 linked / 2 unlinked|食物保存强，厨房污染控制需与 WASH 串联。|
|Disaster support|不单列|不适用|不适用|`data/guides/disaster`|46|3 linked / 43 unlinked|旧灾害 Guide 与 Shelter/Fire 场景重叠，但证据链弱。|

### 1.2 Shelter 现有覆盖

|已有覆盖|代表 Wiki / Guide|判断|
|---|---|---|
|入口缓冲区、鞋物分离、污染线|`shelter-entry-buffer-001`；DG-0656|强。已有 Guide -> Wiki -> Action -> check 基础。|
|睡眠区与污染区距离|`shelter-contamination-zone-003`；DG-0657|强。适合作为 WASH 联动证据。|
|火源区周边清空|`shelter-ventilation-001`；DG-0658|强，但与 Fire Guide 需要组合。|
|物资高处防潮|`shelter-food-zone-001`；DG-0659|可用，但缺居住点整体防潮策略。|
|夜间通道清障|`shelter-night-safety-001`；DG-0660|可用。|
|临时住所地面潮湿风险|`shelter-temporary-shelter-003`|有 Wiki，但缺选址/搭建/每日维护行动入口。|
|通风路径判断|`shelter-ventilation-002`|有 Wiki，Guide 层未形成完整通风-保温-火源权衡。|

结论：Shelter 不缺“空间分区”知识，缺的是临时住所完整运行链：选址、防雨、防潮、地面隔离、保温、通风、长期居住点每日检查。

### 1.3 Fire 现有覆盖

|已有覆盖|代表 Wiki / Guide|判断|
|---|---|---|
|室内燃烧与一氧化碳|`fire-indoor-combustion-carbon-monoxide-001`；DG-0591|强。高风险边界明确。|
|睡前火源复查|`fire-fire-knowledge-002`；DG-0646|强。|
|烟味/烟气倒灌|`fire-smoke-002`；DG-0647、DG-0595|强，但易与 evacuation 混淆。|
|酒精炉加燃料与倾倒|`fire-alcohol-stove-001`；DG-0648、DG-0593|可用。|
|蜡烛无人看守禁用|`fire-candle-001`；DG-0649、DG-0592|强。|
|明火和燃气味禁忌|`fire-gas-smell-no-ignition-001`|有 Wiki，需进入综合火源 Guide。|
|初起小火停止线|`fire-small-fire-stop-001`|有 Wiki，Guide 入口不足。|

结论：Fire 的禁止、停止、撤离边界较强；缺生火前场地判断、火种保存、干湿燃料分级、灰烬处理、儿童/旁人隔离、临时炉具综合边界。

### 1.4 Clothing / PPE 现有覆盖

|已有覆盖|代表 Wiki / Guide|判断|
|---|---|---|
|湿袜与脚部风险|`clothing-wet-socks-001`；DG-0651|强。|
|雨具破损临时修补|`clothing-repair-001`；DG-0652|强。|
|高温防晒补水|`clothing-heat-001`；DG-0653|可用，但本批重点是低温潮湿。|
|低温睡前分层穿衣|`clothing-insulation-001`；DG-0654|可用。|
|鞋底破损转移风险|`clothing-shoe-repair-001`；DG-0655|可用。|
|手套清理维修边界|`clothing-gloves-001`|有 Wiki，缺 Guide。|
|污染衣物分区|`clothing-contamination-zone-001`|有 Wiki，需纳入 WASH。|

结论：Clothing 已覆盖湿冷和鞋袜；PPE 只到手套层，护目、口鼻防护、皮肤保护、工作后晾干动线不足。

### 1.5 WASH / 卫生污染控制现有覆盖

|已有覆盖|代表 Wiki / Guide|判断|
|---|---|---|
|少水洗手|`hygiene-handwashing-001`；DG-0572、DG-0081|强，但 Guide 旧链路不均。|
|桶厕和覆盖材料|`hygiene-toilet-001`；DG-0571、DG-0626|强。|
|呕吐物/粪便污染清理|`hygiene-vomit-001`；DG-0574、DG-0662|强。|
|污染区/清洁区标记|`hygiene-contamination-zone-001`；DG-0663|强。|
|病人用品隔离|DG-0083；`hygiene-shared-items-001`|有行动卡，但 related_wiki 缺。|
|厨房生熟分区|DG-0084；`food-waste-001`、`hygiene-contaminated-surface-001`|有行动入口，但证据链弱。|
|卫生异常记录|`hygiene-contamination-log-001`、`hygiene-hygiene-knowledge-004`|有 Wiki，缺整合 Guide。|

结论：WASH 的单点知识非常多，但缺“饮水区 / 洗手区 / 厕所区 / 厨房区 / 病人用品 / 污染区 / 每日巡查”一张运行表式行动入口。

### 1.6 已有强项

- Shelter：入口缓冲、睡眠区远离污染区、火源区清空、夜间通道、物资防潮。
- Fire：室内燃烧、一氧化碳、蜡烛、酒精炉、烟雾、睡前复查。
- Clothing：湿袜、鞋底损坏、雨衣补片、分层保温、脚部风险。
- WASH：少水洗手、桶厕、污染区、污染垃圾、呕吐物/粪便、血液污染、化学味容器禁用。
- Water / Food support：饮用水分级、容器标记、厨房/食物污染风险、食物残渣虫害。

### 1.7 明显缺口

|缺口|原因|优先级|
|---|---|---|
|临时住所选址与防雨防潮一体化|现有 Shelter 偏室内分区，不足以指导“今晚睡哪里”。|P0|
|地面隔离和睡眠保温联动|Clothing 有分层，Shelter 有睡眠区，但缺低温潮湿下统一行动卡。|P0|
|火源使用前场地判断|现有 Fire 偏风险边界，缺点火前场地、旁人、燃料、通风检查。|P0|
|火种保存和干湿燃料分级|现有条目较少，难支撑低资源持续取暖/烹饪。|P1|
|灰烬与余火处理|存在灾害旧 Guide DG-0488，但未形成 Fire 主导链。|P0|
|低资源 PPE|手套有 Wiki，护目、口鼻、防皮肤污染不足。|P0/P1|
|工作后衣物晾干和污染分区|有衣物污染和湿衣知识，缺行动流程。|P0|
|WASH 运行表|各单点完整，缺小团队日常运行、巡查、记录和交接。|P0|
|厨房与病人用品污染隔离|早期 Guide 有，但 related_wiki 缺，容易被医疗/食物分散。|P0|
|Retrieval Field Test 缺失|Shelter / Fire / Clothing / WASH 尚未专项验证。|P0|

### 1.8 重复或近似主题

|主题族|已有条目|处理建议|
|---|---|---|
|火源取暖 / 室内燃烧|`fire-heating-001` 至 `fire-heating-004`、`fire-indoor-combustion-carbon-monoxide-001`、DG-0591|不重写旧条目，新 Wiki 只补“使用前检查/禁区/记录”角度。|
|蜡烛风险|`fire-candle-001`、`fire-candle-002`、DG-0592、DG-0649|不再扩写蜡烛百科，只在 Guide 中引用为停止条件。|
|酒精炉风险|`fire-alcohol-stove-001`、`fire-alcohol-stove-002`、DG-0593、DG-0648|不再做炉具教程，只补临时炉具边界。|
|低温保温|`fire-hypothermia-*`、`fire-insulation-*`、`clothing-insulation-*`、DG-0594、DG-0654|避免重复讲原理，新内容聚焦地面隔离与行动复查。|
|洗手|`hygiene-handwashing-001/002/003`、`water-handwashing-001`、DG-0572、DG-0081|不重复洗手原理，规划 WASH 优先级和运行表。|
|桶厕|`hygiene-toilet-001/002/003`、DG-0571、DG-0626|不重复桶厕原理，补厕所区与饮水/厨房的距离、记录。|
|污染区|`hygiene-contamination-zone-*`、`shelter-contamination-zone-*`、DG-0663、DG-0657|保留双域视角，新 Guide 负责把空间和卫生串起来。|

### 1.9 不应改动的旧条目

以下旧条目已经承担稳定证据或高风险边界，不建议在 Batch5-I Apply 中改正文；如需使用，只新增关联或新 Guide 引用，但本规划阶段不执行：

- Fire：`fire-indoor-combustion-carbon-monoxide-001`、`fire-carbon-monoxide-001`、`fire-heating-001`、`fire-fire-knowledge-002`、`fire-candle-001`、`fire-alcohol-stove-001`。
- Shelter：`shelter-entry-buffer-001`、`shelter-contamination-zone-002`、`shelter-contamination-zone-003`、`shelter-ventilation-001`、`shelter-ventilation-002`。
- Clothing：`clothing-wet-socks-001`、`clothing-wet-shoes-001`、`clothing-insulation-001`、`clothing-shoe-repair-001`、`clothing-repair-001`。
- Hygiene / Contamination：`hygiene-handwashing-001`、`hygiene-toilet-001`、`hygiene-vomit-001`、`hygiene-contamination-zone-001`、`hygiene-contamination-log-001`、`hygiene-sewage-001`。
- Water / Food support：`water-handwashing-001`、`water-contamination-separation-033`、`food-waste-001`、`food-cooking-001`。

## 2. 缺口分析

Batch5-I 的根因不是 Wiki 数量不足，而是四条链路未完全闭合：

1. Shelter 链路：临时住所选址 -> 防雨防潮 -> 地面隔离 -> 睡眠保温 -> 通风 -> 每日检查。
2. Fire 链路：点火前场地判断 -> 燃料/火种 -> 临时炉具边界 -> 室内燃烧停止线 -> 灰烬余火 -> 记录复查。
3. Clothing/PPE 链路：湿冷早期信号 -> 鞋袜/衣物处理 -> 低资源手套/护目/口鼻防护 -> 污染衣物分区 -> 工作后晾干。
4. WASH 链路：饮水区 -> 洗手区 -> 厕所区 -> 厨房区 -> 病人用品 -> 污染区 -> 巡查表 -> 异常记录。

规划原则：

- 新 Wiki 补“判断边界、组合关系、记录复查”，避免重复旧百科。
- 新 Guide 必须是行动入口，不是百科入口。
- P0 优先处理立即影响失温、火灾、一氧化碳、粪口传播、厨房污染的场景。
- Apply 后必须进入 Shelter / Fire / WASH Field Test；不要通过 Retrieval 调参掩盖 evidence 缺失。

## 3. 新增 Wiki 规划清单

规划 41 篇 Wiki：P0 24 篇，P1 14 篇，P2 3 篇。以下仅为规划，不创建文件。

|方向|slug|title|category|priority|risk_level|用途一句话|是否需要 Guide|可能关联 Guide|
|---|---|---|---|---|---|---|---|---|
|Shelter|shelter-site-selection-weather-exposure-001|临时住所选址的风雨和暴露判断|庇护空间分区|P0|caution|帮助判断今晚是否能留在某处睡眠，避免低洼、风口、坠物和暴露位置。|是|DG-0847|
|Shelter|shelter-rain-leak-first-line-001|漏雨时先保护睡眠区和物资区|庇护空间分区|P0|caution|说明漏雨时先保人、干衣、食物和电源，不追求彻底修房。|是|DG-0847|
|Shelter|shelter-ground-moisture-barrier-001|潮湿地面的隔离层判断|庇护空间分区|P0|caution|帮助选择垫高、隔湿、换位和停用湿地面的边界。|是|DG-0848|
|Shelter|shelter-sleep-heat-loss-ground-001|睡眠区地面失温风险|庇护空间分区|P0|high|解释低温潮湿地面为什么会快速带走体温。|是|DG-0848|
|Shelter|shelter-ventilation-heat-balance-001|保温和通风的冲突平衡|庇护空间分区|P0|high|用于火源、多人睡眠和潮湿环境下判断通风不能完全关闭。|是|DG-0848；DG-0850|
|Shelter|shelter-kitchen-fire-sleep-distance-001|厨房火源区和睡眠区距离|庇护空间分区|P0|high|把厨房、火源、睡眠、儿童活动区分开，减少烧伤和烟气风险。|是|DG-0849；DG-0852|
|Shelter|shelter-daily-habitability-check-001|长期居住点每日可住性检查|庇护空间分区|P1|caution|建立漏水、异味、霉味、通道、火源、厕所的每日复查。|是|DG-0847|
|Shelter|shelter-roof-wall-floor-seepage-signs-001|屋顶墙面地面渗水信号|庇护空间分区|P1|caution|帮助判断渗水来源和是否需要转移睡眠区。|否|DG-0847|
|Shelter|shelter-wind-rain-entrance-buffer-001|风雨天气入口缓冲区加固|庇护空间分区|P1|caution|补强雨天鞋物、湿衣、工具和污染带入控制。|否|DG-0847；DG-0851|
|Shelter|shelter-long-stay-zone-review-001|长期居住区分区复盘|庇护空间分区|P2|normal|用于每周复查睡眠、厨房、厕所、物资、污染区是否漂移。|否|DG-0852|
|Fire|fire-before-lighting-site-check-001|生火前场地和周边检查|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|点火前确认可燃物、旁人、通风、退出路线和水/土覆盖材料。|是|DG-0849|
|Fire|fire-dry-wet-fuel-sorting-001|干湿燃料分级和禁用判断|火源 / 保温 / 通风 / 一氧化碳风险|P0|caution|避免湿燃料浓烟、化学材料燃烧和室内错误使用。|是|DG-0849|
|Fire|fire-tinder-storage-dry-boundary-001|火种和引火物干燥保存边界|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|说明引火物只做备用，不靠易燃危险品冒险。|否|DG-0849|
|Fire|fire-indoor-combustion-no-go-zone-001|室内燃烧禁区和停止线|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|把密闭空间、睡眠区、儿童区、燃气味、烟气倒灌列为禁用边界。|是|DG-0850|
|Fire|fire-carbon-monoxide-suspect-stop-001|疑似一氧化碳时停止取暖|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|强调头晕、恶心、困倦、多人异常时立即停火通风转移。|是|DG-0850|
|Fire|fire-smoke-backdraft-room-response-001|烟雾反流时的开窗和撤离判断|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|帮助判断烟进屋后是封堵、通风还是离开。|是|DG-0850|
|Fire|fire-ash-ember-cooling-disposal-001|灰烬和余火冷却处理|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|防止余火复燃、烫伤、垃圾引燃和室内烟气。|是|DG-0851|
|Fire|fire-temporary-stove-stability-boundary-001|临时炉具稳定性和禁用边界|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|只判断边界，不教复杂炉具制造；不稳、倾倒、漏燃料即停用。|是|DG-0849|
|Fire|fire-children-bystander-clear-zone-001|儿童和旁人远离火源区|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|设置火源区清场、传递物品和照护边界。|是|DG-0849|
|Fire|fire-night-final-extinguish-log-001|夜间火源熄灭记录|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|建立谁检查、何时检查、是否有余热和异味的记录。|否|DG-0851|
|Clothing/PPE|clothing-wet-cold-early-hypothermia-signs-001|湿冷失温早期信号|衣物 / 鞋袜 / 体温防护|P0|high|把发抖、动作变慢、判断变差与立即停工保温联系起来。|是|DG-0852|
|Clothing/PPE|clothing-foot-check-after-wet-work-001|湿作业后的脚部检查|衣物 / 鞋袜 / 体温防护|P0|caution|补强湿袜后脚趾、破皮、水泡、鞋内异物复查。|是|DG-0852|
|Clothing/PPE|clothing-shoe-sole-failure-outing-stop-001|鞋底损坏后的外出停止线|衣物 / 鞋袜 / 体温防护|P0|caution|明确鞋底开裂、打滑、进水时外出降级或停止。|是|DG-0852|
|Clothing/PPE|clothing-layering-work-rest-adjustment-001|干活和休息时的分层调整|衣物 / 鞋袜 / 体温防护|P1|caution|避免出汗后停下变冷，建立增减衣流程。|否|DG-0852|
|Clothing/PPE|clothing-raincoat-patch-field-limit-001|雨衣临时补片的使用上限|衣物 / 鞋袜 / 体温防护|P1|caution|说明补片只能降级防雨，不可替代长时间暴雨防护。|否|DG-0852|
|Clothing/PPE|clothing-glove-contamination-cut-boundary-001|手套使用和破损污染边界|衣物 / 鞋袜 / 体温防护|P0|high|区分清洁、污染、维修手套，破损或渗漏后停用。|是|DG-0852|
|Clothing/PPE|clothing-eye-protection-low-resource-001|低资源护目防护边界|衣物 / 鞋袜 / 体温防护|P1|caution|清理灰尘、烟灰、碎片、飞溅时说明护目替代的边界。|否|DG-0852|
|Clothing/PPE|clothing-mouth-nose-dust-smoke-limit-001|口鼻防护的粉尘烟雾边界|衣物 / 鞋袜 / 体温防护|P1|high|说明布料遮挡不能替代危险烟雾/化学气味撤离。|否|DG-0852|
|Clothing/PPE|clothing-work-after-drying-route-001|工作后衣物晾干动线|衣物 / 鞋袜 / 体温防护|P1|caution|把湿衣、污染衣、睡眠区和火源区分开。|否|DG-0852；DG-0853|
|Clothing/PPE|clothing-contaminated-laundry-zone-001|污染衣物临时存放区|衣物 / 鞋袜 / 体温防护|P0|high|避免污染衣物进入睡眠区、厨房区和饮水区。|是|DG-0853|
|WASH|hygiene-wash-zone-layout-minimum-001|饮水洗手厕所厨房最小分区|污染控制 / 隔离 / 清洁分区|P0|high|为小团队建立 WASH 空间运行图，先防粪口传播和厨房污染。|是|DG-0853|
|WASH|hygiene-handwater-priority-table-001|洗手水优先级表|卫生|P0|high|在水少时决定哪些时刻必须洗手，哪些只能降级擦拭。|是|DG-0853|
|WASH|hygiene-bucket-toilet-changeover-001|桶厕更换和封存流程|卫生|P0|high|把满袋、破袋、异味、儿童接近作为停止和更换条件。|是|DG-0853|
|WASH|hygiene-patient-cup-towel-isolation-001|病人杯子毛巾餐具隔离|污染控制 / 隔离 / 清洁分区|P0|high|补强病人用品与厨房、饮水、睡眠区的隔离。|是|DG-0854|
|WASH|hygiene-kitchen-raw-cooked-contamination-line-001|厨房生熟和污染线|食物|P0|high|把生熟分区、污染表面、病人用品、厕所距离串联。|是|DG-0854|
|WASH|hygiene-contamination-zone-visible-marking-001|污染区可见标记方法|污染控制 / 隔离 / 清洁分区|P1|caution|规划低资源标记，不依赖专业标识。|否|DG-0853|
|WASH|hygiene-daily-wash-round-checklist-001|每日 WASH 巡查表|卫生|P1|caution|检查饮水、洗手、厕所、厨房、垃圾、病人用品和异常气味。|是|DG-0853|
|WASH|hygiene-wash-abnormal-record-001|卫生异常记录和追溯|卫生|P1|caution|记录腹泻、呕吐、异味、虫害、污染区破坏和处理人。|否|DG-0853；DG-0854|
|WASH|hygiene-food-water-toilet-distance-review-001|食物水桶厕所距离复核|污染控制 / 隔离 / 清洁分区|P1|high|每天复核水、食物和厕所是否被动线挤到一起。|是|DG-0853|
|WASH|hygiene-weekly-zone-drift-review-001|每周卫生分区漂移复盘|卫生|P2|normal|用于长期居住点防止清洁区、污染区、厨房区边界变模糊。|否|DG-0853|
|WASH|hygiene-simple-team-wash-handover-001|小团队 WASH 交接摘要|卫生|P2|normal|交接当前水量、厕所状态、病人用品、垃圾和待复查点。|否|DG-0853|

## 4. Guide 候选

Guide 候选最多 8 个。建议 Apply 第一批不超过 6 个，优先 DG-0847 至 DG-0852；DG-0853/DG-0854 可作为第二批或视 Apply 风险合并。

### DG-0847 临时住所选址与防雨防潮

- scenario：低温潮湿、漏雨或临时避难点条件差，小团队需要判断今晚能否停留，以及睡眠区放在哪里。
- steps：
  1. 先排除低洼积水、坠物、明显异味、烟气、污水、墙体持续渗水的位置。
  2. 按人、干衣、食物、水、电源的顺序确定最小干燥核心区。
  3. 将睡眠区抬高或垫隔湿层，湿鞋湿衣放在入口缓冲区。
  4. 漏雨时先转移睡眠和物资，不做高风险修补。
  5. 标出入口、通道、厕所、厨房/火源和污染区。
- check：
  - 睡眠区地面是否干燥或已隔湿。
  - 头顶、墙边、地面是否仍有滴水或渗水。
  - 夜间通道是否不穿过厕所区和火源区。
  - 是否记录漏水点、湿区和第二天复查时间。
- stop_or_escalate：
  - 有持续渗水、污水倒灌、霉味刺鼻、结构异响、烟味进入或人员发冷发抖时，停止把该处作为睡眠区。
  - 无法保持一块干燥睡眠区时，转入撤离/换点判断。
- fallback：
  - 无法完全防雨时，只保一块最小睡眠区和干衣区。
  - 无足够垫材时，优先给儿童、老人、病人和湿冷人员。
- related_wiki：
  - `shelter-site-selection-weather-exposure-001`
  - `shelter-rain-leak-first-line-001`
  - `shelter-ground-moisture-barrier-001`
  - `shelter-roof-wall-floor-seepage-signs-001`
  - `shelter-daily-habitability-check-001`

### DG-0848 睡眠区保温和地面隔离

- scenario：夜间低温、地面潮湿、衣物不足或多人共用临时睡眠区，需要降低失温风险。
- steps：
  1. 先判断地面是否潮、冷、渗水或靠近门窗风口。
  2. 用可用材料做隔湿和隔冷，睡眠区离开污染区、厕所区和火源区。
  3. 睡前检查脚、袜、鞋、外层衣物是否湿。
  4. 保留最低通风，不因保温完全封闭有火源或多人空间。
  5. 记录夜间发冷、咳嗽、潮湿和第二天需晾干物品。
- check：
  - 身体不直接接触湿冷地面。
  - 湿衣湿鞋不进入睡眠层。
  - 通风没有被完全堵死。
  - 夜间有人负责复查儿童、老人、病人。
- stop_or_escalate：
  - 人员持续发抖、动作变慢、意识变差、湿衣无法更换时，停止继续睡在原位置。
  - 火源取暖导致头晕、恶心、困倦或烟味加重时，立即停火通风转移。
- fallback：
  - 垫材不足时集中保一小块睡眠区。
  - 衣物不足时用干燥材料补头颈手脚和地面隔层。
- related_wiki：
  - `shelter-sleep-heat-loss-ground-001`
  - `shelter-ground-moisture-barrier-001`
  - `shelter-ventilation-heat-balance-001`
  - `clothing-wet-cold-early-hypothermia-signs-001`
  - `clothing-layering-work-rest-adjustment-001`

### DG-0849 火源使用前检查

- scenario：断电、低温或需要做饭，小团队打算使用明火、临时炉具、蜡烛或酒精炉。
- steps：
  1. 先判断是否在室内、睡眠区、儿童活动区、燃气味、烟味倒灌或通风不良环境。
  2. 清空火源周边可燃物，设置旁人和儿童距离。
  3. 分开干湿燃料，禁用不明化学材料、塑料、油污材料。
  4. 检查炉具或容器是否稳定、会不会倾倒、是否能快速停止。
  5. 指定看火人和熄灭复查人。
- check：
  - 火源周围是否清空。
  - 是否有通风和退出路线。
  - 是否有水、土、盖子等低风险熄灭材料。
  - 是否记录点火人、用途和预计熄灭时间。
- stop_or_escalate：
  - 燃气味、头晕恶心、烟气倒灌、炉具不稳、儿童无法隔离、无人看守时不点火。
  - 室内烧炭、密闭取暖、不明燃料燃烧直接禁用。
- fallback：
  - 不能安全用火时，改用冷食、保温衣物、集中睡眠、低功耗照明。
  - 做饭优先选择少烟、短时、可熄灭方式。
- related_wiki：
  - `fire-before-lighting-site-check-001`
  - `fire-dry-wet-fuel-sorting-001`
  - `fire-temporary-stove-stability-boundary-001`
  - `fire-children-bystander-clear-zone-001`
  - `fire-tinder-storage-dry-boundary-001`

### DG-0850 室内燃烧和一氧化碳停止线

- scenario：有人想在室内烧炭、用炉具取暖或用火做饭，担心冷、烟、通风和一氧化碳风险。
- steps：
  1. 先确认是否密闭、是否有燃气味、是否多人睡眠、是否已有头晕恶心。
  2. 区分取暖、照明、做饭，不允许用高风险燃烧替代长期取暖。
  3. 保留通风路径，火源区远离睡眠区和儿童区。
  4. 每次用火设定结束时间和复查人。
  5. 出现异常时先停火、通风、离开，再讨论是否继续。
- check：
  - 是否有人出现头晕、恶心、困倦、意识变慢。
  - 烟是否向室内倒灌。
  - 门窗通风是否被完全封死。
  - 火源是否已经完全熄灭并复查。
- stop_or_escalate：
  - 密闭室内烧炭、燃气味环境点火、睡眠时留火、多人同时异常，全部停止。
  - 疑似一氧化碳时不再尝试“忍一忍”或继续睡。
- fallback：
  - 用无火保暖、分层衣物、地面隔离、集中睡眠代替燃烧取暖。
  - 必须做饭时缩短时间，人员离开烟气方向。
- related_wiki：
  - `fire-indoor-combustion-no-go-zone-001`
  - `fire-carbon-monoxide-suspect-stop-001`
  - `fire-smoke-backdraft-room-response-001`
  - `shelter-ventilation-heat-balance-001`
  - `fire-indoor-combustion-carbon-monoxide-001`

### DG-0851 灰烬与余火处理

- scenario：做饭、取暖或照明结束后，现场有灰烬、余火、热炉具、焦物或烟熏垃圾。
- steps：
  1. 先确认没有继续燃烧、冒烟、发热或被风吹散。
  2. 让灰烬和炉具远离睡眠区、垃圾、衣物、儿童和通道。
  3. 不把热灰直接倒入塑料袋、纸箱、室内垃圾或干草木堆。
  4. 冷却后再封存或转移，转移前二次复查。
  5. 记录火源结束时间、处理人和复查时间。
- check：
  - 触碰前是否仍热。
  - 是否还有烟味或火星。
  - 是否靠近可燃物。
  - 是否完成夜间复查。
- stop_or_escalate：
  - 灰烬仍热、仍有火星、现场有风、儿童无法远离时，不搬运或倒入垃圾。
  - 出现复燃、烟气加重、烫伤时停止处理并隔离区域。
- fallback：
  - 没有安全容器时，延长冷却和看守时间。
  - 不确定是否冷却时，按仍有余火处理。
- related_wiki：
  - `fire-ash-ember-cooling-disposal-001`
  - `fire-night-final-extinguish-log-001`
  - `fire-small-fire-stop-001`
  - `fire-fire-response-001`

### DG-0852 湿冷衣物和脚部保护

- scenario：雨天、涉水、低温、长距离外出或湿作业后，成员衣物鞋袜潮湿，仍需活动或睡眠。
- steps：
  1. 先检查脚、袜、鞋内、手、头颈和贴身层是否湿冷。
  2. 湿袜湿鞋离开睡眠区，脚部擦干或换到最干状态。
  3. 外出前检查鞋底、鞋面、鞋带和防滑；破损时降级任务。
  4. 工作和休息切换时调整衣物，避免出汗后静止受冷。
  5. 污染衣物与普通湿衣分开放置。
- check：
  - 脚趾是否发白、麻木、破皮、水泡或持续冰冷。
  - 鞋底是否打滑、开裂或进水。
  - 睡前是否移除贴身湿衣。
  - 是否记录需要晾干或修补的衣物鞋袜。
- stop_or_escalate：
  - 持续发抖、行动迟缓、判断变差、脚部破皮感染迹象时停止外出和高风险工作。
  - 鞋底严重开裂、湿冷无法处理时不执行远距离外出。
- fallback：
  - 没有干袜时先隔湿、减少步行、缩短任务。
  - PPE 不足时降低任务风险，不用薄布口鼻防护进入烟雾/化学气味区。
- related_wiki：
  - `clothing-wet-cold-early-hypothermia-signs-001`
  - `clothing-foot-check-after-wet-work-001`
  - `clothing-shoe-sole-failure-outing-stop-001`
  - `clothing-glove-contamination-cut-boundary-001`
  - `clothing-contaminated-laundry-zone-001`

### DG-0853 小团队 WASH 分区运行

- scenario：小团队长期共处，水有限，有桶厕、厨房、病人、污染物和垃圾，需要每天维持最小卫生系统。
- steps：
  1. 标出饮水区、洗手区、厕所区、厨房区、污染区和睡眠区。
  2. 确定必须洗手时刻：处理厕所/呕吐物/垃圾后、做饭前、吃饭前、照护病人前后。
  3. 每日巡查水桶、洗手点、桶厕、垃圾、病人用品、厨房表面。
  4. 发现污染区漂移或标记破坏时，先隔离再清理。
  5. 交接 WASH 状态和待处理异常。
- check：
  - 厕所区是否远离饮水和厨房。
  - 洗手点是否可用。
  - 污染垃圾是否二次封存。
  - 是否有腹泻、呕吐、异味、虫害记录。
- stop_or_escalate：
  - 厕所/污物接近饮水或厨房、桶厕破袋、多人腹泻呕吐、污染区无法标记时停止共用相关区域。
  - 食物接触污染表面时按弃用或隔离判断。
- fallback：
  - 水不足时保留关键时刻洗手，其他清洁降级。
  - 空间不足时至少保持饮水、厨房、厕所三者不重叠。
- related_wiki：
  - `hygiene-wash-zone-layout-minimum-001`
  - `hygiene-handwater-priority-table-001`
  - `hygiene-bucket-toilet-changeover-001`
  - `hygiene-daily-wash-round-checklist-001`
  - `hygiene-food-water-toilet-distance-review-001`

### DG-0854 病人用品与厨房污染隔离

- scenario：有人腹泻、呕吐、发热或伤口污染，同时小团队还需要做饭、喝水和共用餐具。
- steps：
  1. 给病人杯子、毛巾、餐具、垃圾袋设专用位置和标记。
  2. 病人用品不进入公共厨房和饮水桶旁。
  3. 清理病人用品后先洗手，再处理食物或饮水。
  4. 厨房生熟、清洁/污染表面分开。
  5. 记录症状、接触物品、清理人和复查时间。
- check：
  - 病人用品是否混入公用餐具。
  - 厨房表面是否接触呕吐物、粪便、血液或污染衣物。
  - 做饭前是否完成手卫生。
  - 是否有人出现相同症状。
- stop_or_escalate：
  - 病人用品混入公共餐具、多人腹泻呕吐、厨房被污染且无法清洁时，停止使用相关餐具和区域。
  - 严重脱水、意识异常、持续高热等转入医疗高风险判断。
- fallback：
  - 餐具不足时用个人编号和固定位置，不混用。
  - 无法彻底清洁时降级为非食物用途或封存。
- related_wiki：
  - `hygiene-patient-cup-towel-isolation-001`
  - `hygiene-kitchen-raw-cooked-contamination-line-001`
  - `hygiene-wash-abnormal-record-001`
  - `hygiene-shared-items-001`
  - `hygiene-contaminated-surface-001`

## 5. Retrieval 风险预测

### 5.1 Shelter / Evacuation 混淆

|Query 类型|Shelter 主导条件|Evacuation 主导条件|证据组合建议|
|---|---|---|---|
|今晚睡哪里更安全？|仍可在当前区域建立干燥睡眠区、通道、污染隔离和火源边界。|无法保持干燥睡眠区、有结构/烟气/污水/持续渗水危险。|Shelter Guide 主导，Evacuation 仅补“不可停留信号”。|
|房子漏雨还能不能留？|局部漏雨，可转移睡眠区和物资区，地面可隔湿。|漏雨导致电气、结构、污水、低温失控，无法形成干燥核心区。|先 DG-0847；触发停止线时切到撤离判断。|
|要不要转移？|问题核心是空间布置、睡眠、防潮、防雨。|问题核心是住所不可继续停留、外部危险、路线和携带物。|Evacuation 主导，Shelter 提供当前住所风险证据。|

风险：`shelter` domain 在很多 Guide 中出现，Selector 可能把撤离卡或灾害旧卡提前。Field Test 需要覆盖“留还是走”的边界题。

### 5.2 Fire / Energy 混淆

|Query 类型|Fire 主导条件|Energy 主导条件|边界|
|---|---|---|---|
|没电了能不能室内烧炭取暖？|出现燃烧、取暖、烟、一氧化碳、室内火源。|只问照明、电池、充电、低功耗安排。|Fire 高风险停止线必须优先：密闭室内烧炭禁用。|
|用火取暖怎么通风？|火源/取暖/烟气/CO 是核心风险。|能源只作为“无火替代”补充。|不能用节能逻辑压过 CO 风险。|
|火源和照明怎么安排？|明火、蜡烛、睡前复查主导。|手电、头灯、电池预算补充。|火源安全 > 能源管理。|

### 5.3 Fire / Cooking / Food 混淆

|Query 类型|Fire 主导条件|Food 主导条件|证据组合建议|
|---|---|---|---|
|做饭时烟很大怎么办？|烟、通风、燃料、临时炉具、室内燃烧。|食物是否熟、低燃料烹饪。|Fire 主导，Food 补烹饪安全。|
|临时炉具能不能放屋里？|炉具稳定、倾倒、CO、烟气、儿童区。|做饭需求只是背景。|Fire 主导，Shelter 补火源区/睡眠区距离。|
|湿木头能不能烧？|干湿燃料分级、烟雾反流。|食物加热需求补充。|Fire 主导，不输出高风险室内燃烧技巧。|

### 5.4 Hygiene / Water / Medical 混淆

|Query 类型|WASH 主导条件|Water / Medical 补充条件|边界|
|---|---|---|---|
|病人用过的杯子怎么处理？|病人用品、餐具、厨房、洗手、隔离。|医疗补症状观察；水补清洗水优先级。|WASH 主导，避免变成泛医疗治疗。|
|洗手水不够怎么办？|关键时刻洗手、降级清洁、厕所/厨房动线。|水补配给和分级。|WASH 主导，不用“省水”压过粪口传播。|
|厨房和厕所太近怎么办？|空间分区、污染线、饮水/食物安全。|Shelter 补空间布置，Food 补食物弃用。|WASH 主导。|

### 5.5 Clothing / Medical 混淆

|Query 类型|Clothing 主导条件|Medical 主导条件|边界|
|---|---|---|---|
|脚一直湿会不会有问题？|湿袜、鞋内潮湿、脚部检查、外出降级。|已有破皮感染、冻伤、意识异常。|Clothing 主导，Medical 补高风险信号。|
|鞋底破了还要不要外出？|鞋底、打滑、进水、任务降级。|已经受伤或感染时医疗补充。|Clothing/PPE 主导，Risk Decision 可补停止线。|
|冷得发抖但还要干活？|分层、停工保温、湿冷处理。|发抖持续、意识变差、失温信号。|先停止高风险任务，再医疗观察。|

## 6. 不建议加入内容

- 不做建筑工程教程。
- 不做烟囱施工教程。
- 不做复杂炉具制造。
- 不做野外求生花活。
- 不做高风险室内燃烧操作指南。
- 不做医疗治疗替代。
- 不做化学消毒剂配方大全。
- 不做采购推荐。
- 不做现代物业/消防/电力依赖建议。
- 不做燃气、煤炭、酒精等燃料的“提高效率技巧”。
- 不做让儿童或非操作人员参与火源、污染物、维修的流程。
- 不做需要专业设备才能验证的一氧化碳判断替代方案。

## 7. Batch5-I Apply 范围

### 7.1 推荐 Apply 第一批

建议第一批 Apply：30 到 35 篇 Wiki，Guide 不超过 6 个，不改 Retrieval、Prompt、query profile、top_k、selector limit、fallback。

推荐第一批 Wiki：35 篇。

|方向|建议数量|优先选择|
|---|---:|---|
|Shelter|8|`shelter-site-selection-weather-exposure-001`、`shelter-rain-leak-first-line-001`、`shelter-ground-moisture-barrier-001`、`shelter-sleep-heat-loss-ground-001`、`shelter-ventilation-heat-balance-001`、`shelter-kitchen-fire-sleep-distance-001`、`shelter-daily-habitability-check-001`、`shelter-roof-wall-floor-seepage-signs-001`|
|Fire|9|`fire-before-lighting-site-check-001`、`fire-dry-wet-fuel-sorting-001`、`fire-indoor-combustion-no-go-zone-001`、`fire-carbon-monoxide-suspect-stop-001`、`fire-smoke-backdraft-room-response-001`、`fire-ash-ember-cooling-disposal-001`、`fire-temporary-stove-stability-boundary-001`、`fire-children-bystander-clear-zone-001`、`fire-night-final-extinguish-log-001`|
|Clothing / PPE|8|`clothing-wet-cold-early-hypothermia-signs-001`、`clothing-foot-check-after-wet-work-001`、`clothing-shoe-sole-failure-outing-stop-001`、`clothing-layering-work-rest-adjustment-001`、`clothing-glove-contamination-cut-boundary-001`、`clothing-eye-protection-low-resource-001`、`clothing-mouth-nose-dust-smoke-limit-001`、`clothing-contaminated-laundry-zone-001`|
|WASH|10|`hygiene-wash-zone-layout-minimum-001`、`hygiene-handwater-priority-table-001`、`hygiene-bucket-toilet-changeover-001`、`hygiene-patient-cup-towel-isolation-001`、`hygiene-kitchen-raw-cooked-contamination-line-001`、`hygiene-contamination-zone-visible-marking-001`、`hygiene-daily-wash-round-checklist-001`、`hygiene-wash-abnormal-record-001`、`hygiene-food-water-toilet-distance-review-001`、`hygiene-simple-team-wash-handover-001`|

推荐第一批 Guide：6 个。

|建议 ID|title|是否第一批|
|---|---|---|
|DG-0847|临时住所选址与防雨防潮|是|
|DG-0848|睡眠区保温和地面隔离|是|
|DG-0849|火源使用前检查|是|
|DG-0850|室内燃烧和一氧化碳停止线|是|
|DG-0851|灰烬与余火处理|是|
|DG-0852|湿冷衣物和脚部保护|是|
|DG-0853|小团队 WASH 分区运行|建议第二批或替换 DG-0851|
|DG-0854|病人用品与厨房污染隔离|建议第二批|

说明：如果坚持本批必须覆盖 WASH Guide，则建议第一批 Guide 调整为 DG-0847、DG-0848、DG-0849、DG-0850、DG-0852、DG-0853，并把 DG-0851 灰烬与余火处理推迟为第二批。

### 7.2 Apply 后必须进入 Field Test

建议新增 Shelter / Fire / WASH Field Test，至少覆盖：

- 今晚睡哪里更安全。
- 房子漏雨还能不能留。
- 湿地面睡觉怎么处理。
- 没电能不能室内烧炭取暖。
- 用火取暖怎么通风。
- 做饭时烟很大怎么办。
- 灰烬还有点热能不能倒垃圾袋。
- 湿袜湿鞋还要不要外出。
- 冷得发抖但还要干活怎么办。
- 洗手水不够怎么分配。
- 厨房和厕所太近怎么办。
- 病人用过的杯子毛巾怎么处理。

Field Test 指标建议：

- Guide 命中率 >= 90%。
- Wiki 命中率 >= 85%。
- Guide-Wiki 精准组合率 >= 85%。
- high/caution safety boundary = 100%。
- fallback = 100%。
- record/check = 100%。
- 危险建议 = 0。
- Kiwix 越权 = 0。
- Shelter/Evacuation、Fire/Energy、Hygiene/Water/Medical 混淆必须可解释。

## 8. 验收建议

Batch5-I Apply 阶段验收必须包含：

- Wiki audit 0/0/0。
- Guide audit 0/0/0。
- 不新增 category。
- Guide-Wiki 单边关系 0。
- 无效 Guide ID 0。
- 无效 Wiki slug 0。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不修改 query profile。
- 不修改 top_k。
- 不修改 selector limit。
- 不修改 fallback。
- 不修改 schema。
- 不修改测试来适配内容。
- 后续必须进入 Shelter / Fire / WASH Field Test。

Apply 复核顺序建议：

1. 先生成 Wiki，确认 slug、category、priority、risk_level、guide_links 规划一致。
2. 再生成不超过 6 个 Guide，确保每个 Guide 都是行动入口。
3. 做 Guide-Wiki 双向校验，不允许单边关系。
4. 运行 Wiki audit 和 Guide audit。
5. 再做 Shelter / Fire / WASH Field Test。
6. 只有 Field Test 发现 evidence 断链时，回到知识源或关联层定位根因；不要通过 Retrieval 参数或 Prompt 修饰掩盖。
