# LanternBox Batch9-B：Manufacturing & Production Foundation Coverage Planning

日期：2026-07-17

阶段性质：覆盖分析与规划报告。  
遵守：`docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。  
本阶段未规划修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

## 1. 当前覆盖审查

### 1.1 目录与数量

实际检查到的相关目录如下：

|范围|目录|状态|Wiki / Guide 数量|已有 evidence 情况|判断|
|---|---|---:|---:|---|---|
|Manufacturing|`wiki_import/manufacturing`|存在|23 Wiki|20 篇有 `guide_links`|内容实质偏材料/维修索引，全部 P2 / normal|
|Repair|`wiki_import/repair`|存在|100 Wiki|73 篇有 `guide_links`|Repair / Tools 稳定基础，偏修复与现场安全|
|Tools|`wiki_import/tools`|存在|4 Wiki|4 篇有 `guide_links`|工具安全与基础使用补充|
|Materials|`wiki_import/materials`|不存在|0|无|未形成独立材料知识层|
|Production|`wiki_import/production`|不存在|0|无|未形成生产流程知识层|
|Workshop|`wiki_import/workshop`|不存在|0|无|未形成工坊运行知识层|
|Recycling|`wiki_import/recycling`|不存在|0|无|仅有零散废料/再利用相关 Guide|
|Repair Guides|`data/guides/repair`|存在|5 Guide|5 个有 `related_wiki`，约 69 条边|维修 v0.1 / Wave2 已较稳|
|Tools Guides|`data/guides/tools`|存在|48 Guide|24 个有 `related_wiki`，约 66 条边|包含大量生活/工具/替代材料入口|
|Materials Guides|`data/guides/materials`|不存在|0|无|缺少材料行动入口|
|Workshop Guides|`data/guides/workshop`|不存在|0|无|缺少工坊行动入口|
|Production Guides|`data/guides/production`|不存在|0|无|缺少生产行动入口|

当前 manufacturing 目录中的 23 篇 Wiki 主要是 `repair-*` slug，例如刀具维护、容器修补材料、材料降级使用、木材基础性质、金属腐蚀、绳索承重、扎带边界、简单杠杆原理等。它们对制造有参考价值，但尚未形成“从材料到成品”的行动链。

### 1.2 Guide-Wiki 关系现状

|项目|数量|说明|
|---|---:|---|
|Repair Guide-Wiki 边|约 69|集中在 DG-0621 至 DG-0625 等 repair 行动入口|
|Tools Guide-Wiki 边|约 66|集中在 DG-0833 至 DG-0840 等工具安全入口|
|Manufacturing Wiki 有 `guide_links`|20 / 23|多指向 repair/tools Guide，不是 production Guide|
|Manufacturing Wiki 无行动入口|3|`repair-plastic-aging-001`、`repair-simple-lever-principle-001`、`repair-wear-judgement-001`|
|专用 manufacturing / workshop / production Guide|0|当前没有第一类生产制造行动入口|

### 1.3 有 Guide 但缺少 Wiki evidence 的相邻主题

以下 Tools Guide 与制造/工坊/材料复用相邻，但当前没有 `related_wiki`，不建议硬塞不精准 Wiki，应在后续用新 Wiki 或精准证据补足：

|Guide|标题|与 Manufacturing 的关系|建议|
|---|---|---|---|
|DG-0300|纸板的应急用途|材料替代|后续可接入材料再利用 Wiki|
|DG-0302|布料和旧衣物再利用|软材料替代|后续可接入布料/绳/皮革替代材料 Wiki|
|DG-0305|简易漏斗制作|简单制作|可作为轻量制作示例，不作为主制造入口|
|DG-0308|简易保温箱|成品制作|可与箱体/保温材料 Wiki 精准关联|
|DG-0311|不明来源容器处理|材料安全|可作为塑料/容器再利用 stop line|
|DG-0312|破损容器二次用途|材料降级使用|可接入容器复用边界|
|DG-0315|废料存放上限|工坊库存安全|可接入危险品区/废料区|
|DG-0323|护目和面部防护替代|PPE|可接入工坊最低 PPE|
|DG-0386|每日晨间检查|开工检查|可被新工坊 Guide 复用为参考|
|DG-0387|每日晚间收束|收工清点|可被新工坊 Guide 复用为参考|
|DG-0509|工具用途说明卡|工具管理|可接入工具区标记|
|DG-0512|危险工具权限|危险工具封存|可接入工坊收工 Guide|

### 1.4 与 Tools / Repair stable 内容的重叠点

已有强项包括：

- 工具区安全：儿童远离工具区、危险工具临时隔离、刀锯作业前清场。
- 维修停止线：低光维修停止、工具破损禁用标签、固定点失效判断。
- 基础操作安全：锯切前固定与支撑、胶水粘接前表面处理。
- 修复与回收：旧物拆件防割伤、锈蚀工具清洁和存放、临时替代件、材料降级使用。

这些内容可作为 Manufacturing v0.1 的安全底座，但不能替代制造能力。当前缺少的是：材料选择、加工流程、半成品管理、重复制作、质量复查、承重验证和生产记录。

## 2. Manufacturing 与 Repair 的能力边界

|能力域|核心问题|典型动作|输出物|Retrieval 主入口判断|
|---|---|---|---|---|
|Tools / Repair|已有物品坏了，能否继续用或临时修好|判断损坏、停用、补漏、固定、替代|恢复或降级使用的旧物|问题包含“坏了、漏了、松了、还能不能修、临时补”时优先 repair/tools|
|Manufacturing / Production|用材料制作新的可用物，能否重复做、检查质量|选材、测量、切割、打孔、连接、复查、记录|新成品、半成品、模板、批次产品|问题包含“做一个、制作、批量、材料、成品、承重前、工坊流程、模板、质量检查”时优先 manufacturing|

边界示例：

|用户问题|应优先领域|理由|
|---|---|---|
|“桶裂了怎么补？”|Repair|目标是修复已有容器|
|“用废木板做一个架子怎么检查？”|Manufacturing|目标是从材料制作新物，并需要承重检查|
|“胶水粘不住怎么办？”|Repair 或 Manufacturing|修旧物为 repair；为新制品选择连接方式为 manufacturing|
|“金属片边很锋利还能用吗？”|Manufacturing / Safety|未受伤时是材料加工和防割边；已割伤则 medical 主导|
|“承重前怎么检查？”|Manufacturing|面向新制成品的质量复查和使用前验证|
|“固定点松了还能挂东西吗？”|Repair / Tools|面向既有固定点失效判断|

后续 Retrieval 应避免把所有“工具、材料、修补”都拉回 repair。Manufacturing 的关键词应围绕：制作、成品、半成品、原料、工坊、批次、模板、重复、质量、承重前检查。

## 3. 主题缺口分析

### 3.1 P0 基础制造安全

|能力|当前覆盖|状态|缺口|
|---|---|---|---|
|工作台安全|有工具区安全，但缺生产工作台检查|Yellow|缺开工前台面、照明、夹持、危险品清场|
|刀锯钻磨基础安全|有刀锯和锯切安全，缺钻磨/碎屑/火花边界|Yellow|缺切割、钻孔、打磨组合风险|
|固定夹持|有锯切前支撑|Yellow|缺“所有加工前先夹持”的生产通用入口|
|粉尘 / 碎屑 / 火花|零散或缺失|Red|缺 PPE、通风、停止线、儿童旁人隔离|
|儿童远离工作区|已有强项|Green|可复用 DG-0838|
|低光停止线|已有维修入口|Green / Yellow|可复用，但需扩展到生产加工|
|工具损坏停用|已有强项|Green|可作为制造开工检查一部分|
|PPE 最低要求|有替代护目 Guide，但制造化不足|Yellow|缺眼、手、呼吸最低配置和停工线|

### 3.2 P0 材料基础

|能力|当前覆盖|状态|缺口|
|---|---|---|---|
|木材选择|有木材基础性质 P2|Yellow|缺是否可承重、是否腐朽、是否潮湿变形的行动入口|
|金属废料判断|有腐蚀/锈蚀，但偏维修|Yellow|缺锐边、裂纹、孔洞、受力方向和再利用边界|
|塑料容器再利用边界|有塑料老化无 Guide|Red|缺食物/水/工具/危险品用途分级|
|布料 / 绳 / 皮革替代材料|有布料磨损、绳索承重|Yellow|缺作为新制品材料时的选择与降级规则|
|胶粘 / 捆扎 / 铆接 / 螺丝连接边界|有胶水表面处理、扎带/螺丝背景|Yellow|缺“连接方式选择”行动入口|

### 3.3 P1 简易木工

当前没有完整木工行动链。已有 `repair-wood-basic-properties-001` 与锯切支撑，但缺：

- 测量、划线、切割线确认。
- 打孔边距和木材开裂判断。
- 螺丝连接和预孔。
- 简易框架方正检查。
- 架子、箱体、盖板、门板等低复杂成品。
- 承重前复查和失败停用。

状态：Red / Yellow。基础材料有少量内容，但不可直接形成“做一个可用物”的能力。

### 3.4 P1 简易金属加工

当前覆盖偏锈蚀和旧物拆解。缺：

- 薄金属片打孔前固定。
- 简易弯折边界。
- 防割边处理。
- 临时支架制作。
- 螺丝、铆钉、铁丝连接边界。
- 腐蚀材料再利用前检查。

状态：Red。需要以“薄片、低负荷、手工具”为边界，不进入冶炼或专业机加工。

### 3.5 P1 工坊流程

当前 Tools Guide 有晨检、晚间收束、危险工具权限等相邻内容，但缺制造场景下的：

- 工具区、原料区、半成品区、成品区、危险品区。
- 每日开工检查。
- 收工清点与危险工具封存。
- 生产记录。
- 质量复查。
- 缺陷隔离和返工记录。

状态：Red / Yellow。组织动作存在碎片，但没有工坊生产流程。

### 3.6 P2 简易机械与治具

当前只有 `repair-simple-lever-principle-001`，且无 guide_links。缺：

- 简易夹具。
- 定位模板。
- 重复切割/打孔模板。
- 杠杆、滑轮、绳轮的低风险边界。
- 简单传动概念。

状态：Red。v0.1 可少量铺底，v0.2 再扩。

## 4. 新增 Wiki 规划

建议 Manufacturing v0.1 第一阶段规划 42 篇 Wiki。Batch9-C 可从中选择 34-36 篇 Apply，优先 P0/P1，少量 P2。

|#|slug|title|priority|risk_level|summary|intended Guide relation|Field Test|
|---:|---|---|---|---|---|---|---|
|1|`manufacturing-workbench-start-safety-check-001`|工坊工作台开工前安全检查|P0|high|确认台面、照明、夹持、可燃物、旁人和工具状态，决定是否开工。|DG-0871|Yes|
|2|`manufacturing-work-zone-minimum-layout-001`|低资源工坊最小分区|P0|high|划分工具区、原料区、半成品区、成品区和危险品区，减少混放。|DG-0871, DG-0878|Yes|
|3|`manufacturing-cut-drill-grind-clear-zone-001`|切割钻孔打磨前清场|P0|high|处理碎屑、飞溅、火花、旁人和可燃物风险。|DG-0871, DG-0873, DG-0875|Yes|
|4|`manufacturing-clamp-before-cut-drill-001`|切割和钻孔前固定夹持|P0|high|强调材料加工前先固定，禁止手扶危险加工。|DG-0873, DG-0875|Yes|
|5|`manufacturing-dust-chip-spark-stop-line-001`|粉尘碎屑火花停止线|P0|high|粉尘、碎屑、火花和通风不足时停止加工。|DG-0871|Yes|
|6|`manufacturing-child-bystander-exclusion-zone-001`|儿童和旁人远离加工区|P0|high|建立加工时的旁人隔离和工具暂存规则。|DG-0871, DG-0878|Yes|
|7|`manufacturing-ppe-minimum-eye-hand-breathing-001`|制造作业最低 PPE|P0|high|定义护目、手部、防尘和衣袖发绳等最低边界。|DG-0871|Yes|
|8|`manufacturing-damaged-tool-production-stop-001`|工具损坏后的生产停用|P0|high|发现松动、裂纹、缺齿、电源异常时停止使用并标记。|DG-0871, DG-0878|Yes|
|9|`manufacturing-wood-selection-boundary-001`|木材制作前选择边界|P0|caution|判断木材腐朽、潮湿、裂纹、翘曲和承重用途。|DG-0872, DG-0873, DG-0874|Yes|
|10|`manufacturing-metal-scrap-selection-boundary-001`|金属废料再利用判断|P0|high|检查锈蚀、裂纹、锐边、未知污染和受力用途。|DG-0872, DG-0875|Yes|
|11|`manufacturing-plastic-container-reuse-boundary-001`|塑料容器再利用边界|P0|high|区分工具收纳、非饮食用途、污染容器和老化塑料。|DG-0872|Yes|
|12|`manufacturing-fabric-rope-leather-selection-001`|布料绳索皮革替代材料选择|P0|caution|判断磨损、潮湿、霉变、承重和接触皮肤用途。|DG-0872, DG-0876|Yes|
|13|`manufacturing-glue-tie-screw-choice-001`|胶粘捆扎螺丝连接选择|P0|caution|根据受力、潮湿、拆卸需求和材料选择连接方式。|DG-0876|Yes|
|14|`manufacturing-fastener-reuse-load-boundary-001`|旧螺丝铆钉扎带再利用边界|P0|caution|判断旧紧固件是否变形、锈蚀、脆裂和可承重。|DG-0872, DG-0876|Yes|
|15|`manufacturing-salvaged-part-clean-sort-label-001`|拆解零件清洁分类标记|P0|caution|把可用、待检查、危险、报废零件分开标记。|DG-0872, DG-0878|Yes|
|16|`manufacturing-material-prep-dry-clean-deburr-001`|材料预处理：干燥清洁去毛刺|P0|high|制作前完成干燥、清洁、去毛刺和污染排查。|DG-0872, DG-0875|Yes|
|17|`manufacturing-material-downgrade-label-001`|材料降级使用标签|P0|caution|把不可承重、不可饮食、不可高温等限制写在材料上。|DG-0872|Yes|
|18|`manufacturing-finished-product-load-test-001`|成品承重前检查|P0|high|在使用前逐级加载、观察变形、松动和异响。|DG-0877|Yes|
|19|`manufacturing-wood-measure-mark-cut-line-001`|木材测量划线和切割线确认|P1|caution|减少错切和返工，确认尺寸、方向和余量。|DG-0873|Yes|
|20|`manufacturing-wood-saw-cut-repeatable-support-001`|木材重复锯切支撑|P1|high|使用支撑和限位减少夹锯、崩裂和手部风险。|DG-0873|Yes|
|21|`manufacturing-wood-drill-hole-edge-distance-001`|木材打孔边距检查|P1|caution|避免孔位过近导致开裂或承重失败。|DG-0873, DG-0874|Yes|
|22|`manufacturing-wood-screw-joinery-basic-001`|木材螺丝连接基础|P1|caution|说明预孔、夹紧、受力方向和松动复查。|DG-0874, DG-0876|Yes|
|23|`manufacturing-simple-frame-square-check-001`|简易框架方正检查|P1|caution|制作框架时检查对角线、晃动和连接点。|DG-0874|Yes|
|24|`manufacturing-simple-shelf-bracket-check-001`|简易架子和支撑检查|P1|high|制作搁架、支架前检查支撑点、受力和倾倒风险。|DG-0874, DG-0877|Yes|
|25|`manufacturing-simple-box-crate-build-001`|简易箱体和周转箱制作|P1|caution|低复杂箱体的材料、连接、底部和搬运检查。|DG-0874|No|
|26|`manufacturing-door-cover-panel-basic-001`|门板盖板临时制作边界|P1|caution|制作盖板或门板时关注尺寸、夹伤、通风和固定。|DG-0874|No|
|27|`manufacturing-thin-metal-drilling-support-001`|薄金属片打孔支撑|P1|high|金属片打孔前固定、垫板和防旋转。|DG-0875|Yes|
|28|`manufacturing-thin-metal-bending-boundary-001`|薄金属简易弯折边界|P1|high|弯折前判断裂纹、回弹、锐边和夹手风险。|DG-0875|Yes|
|29|`manufacturing-metal-edge-deburr-cover-001`|金属锐边去毛刺和包边|P1|high|处理切边、孔边和尖角，避免后续割伤。|DG-0875|Yes|
|30|`manufacturing-simple-metal-bracket-001`|简易金属支架制作|P1|high|低负荷支架的材料、孔位、边缘和固定检查。|DG-0875, DG-0877|Yes|
|31|`manufacturing-metal-screw-rivet-wire-join-001`|金属螺丝铆接铁丝连接|P1|caution|比较临时和半永久连接方式及失败信号。|DG-0876|Yes|
|32|`manufacturing-corrosion-before-reuse-check-001`|腐蚀金属再利用前检查|P1|caution|判断锈蚀深度、孔洞、剥落和用途降级。|DG-0872, DG-0875|Yes|
|33|`manufacturing-raw-semi-finished-finished-zones-001`|原料半成品成品分区|P1|caution|避免缺陷件、未处理材料和成品混放。|DG-0871, DG-0878|Yes|
|34|`manufacturing-daily-start-tool-material-check-001`|每日开工工具材料检查|P1|caution|记录工具、材料、夹具、PPE 和任务风险。|DG-0871|Yes|
|35|`manufacturing-end-of-work-clean-count-seal-001`|收工清点与危险工具封存|P1|high|清点刀具、碎片、半成品、危险品和封存位置。|DG-0878|Yes|
|36|`manufacturing-production-batch-record-001`|小批量制作记录|P1|normal|记录材料来源、数量、失败件、改动和用途。|DG-0878|Yes|
|37|`manufacturing-quality-recheck-before-use-001`|成品使用前质量复查|P1|high|使用前检查松动、裂纹、变形、锐边和承重。|DG-0877|Yes|
|38|`manufacturing-defect-isolation-rework-log-001`|缺陷品隔离和返工记录|P1|caution|标记失败件，避免误用，记录返工原因。|DG-0877, DG-0878|No|
|39|`manufacturing-simple-jig-template-use-001`|简易夹具和定位模板使用|P2|caution|用低风险模板减少重复制作误差。|DG-0873, DG-0874|No|
|40|`manufacturing-repeat-cut-drill-template-001`|重复切割和打孔模板|P2|caution|为重复孔位和长度建立模板与复查线。|DG-0873|No|
|41|`manufacturing-simple-lever-load-boundary-001`|简易杠杆承重边界|P2|high|说明杠杆低风险用途、失效信号和旁人隔离。|DG-0877|No|
|42|`manufacturing-pulley-rope-wheel-basic-boundary-001`|滑轮绳轮基础边界|P2|high|只覆盖低负荷移动和停止线，不进入复杂起重。|DG-0877|No|

## 5. Guide 候选

建议最多规划 8 个 Guide，ID 从 DG-0871 起。Batch9-C 第一批可先 Apply 6 个，暂缓治具/复杂机械方向。

### DG-0871 工坊开工前安全检查

scenario：开始切割、打孔、打磨、装配或批量制作前，需要判断工作区、工具、材料、旁人和 PPE 是否满足最低安全条件。

steps：

1. 清空工作台上无关物品和可燃物。
2. 确认照明、通风、逃生路径和地面干燥。
3. 检查工具是否松动、裂纹、缺齿、漏电或缺保护。
4. 检查材料是否固定，是否有锐边、污染、潮湿或不明来源。
5. 清理儿童、旁人和宠物，不允许进入加工区。
6. 准备护目、手部保护和必要的防尘措施。
7. 记录当天任务、材料和风险点。

check：

- 工作台稳定。
- 材料能固定。
- 工具状态可用。
- 照明足够。
- 旁人已隔离。
- PPE 已到位。
- 有开工记录。

stop_or_escalate：

- 低光。
- 工具破损。
- 材料无法固定。
- 粉尘或火花无法控制。
- 儿童或旁人无法隔离。
- 不明污染材料。

fallback：

- 改为手工低风险整理。
- 延后切割/钻孔/打磨。
- 将材料标记为待检查。
- 手写开工风险记录。

related_wiki 候选：

- `manufacturing-workbench-start-safety-check-001`
- `manufacturing-work-zone-minimum-layout-001`
- `manufacturing-cut-drill-grind-clear-zone-001`
- `manufacturing-dust-chip-spark-stop-line-001`
- `manufacturing-child-bystander-exclusion-zone-001`
- `manufacturing-ppe-minimum-eye-hand-breathing-001`
- `manufacturing-damaged-tool-production-stop-001`
- `manufacturing-daily-start-tool-material-check-001`

### DG-0872 材料再利用前判断

scenario：准备使用废木板、金属片、塑料容器、旧布料、绳索或拆解零件制作新物品前，需要判断用途、污染、承重和降级边界。

steps：

1. 确认材料来源和原用途。
2. 检查污染、油污、化学品、霉变和异味。
3. 检查裂纹、腐朽、锈蚀、脆化、锐边和变形。
4. 判断是否接触饮水、食物、皮肤、火源或承重。
5. 对无法确认的材料降级使用或隔离。
6. 清洁、干燥、去毛刺并标记限制。
7. 记录材料来源和用途。

check：

- 来源已记录。
- 危险用途已排除。
- 材料限制已标记。
- 未将不明材料用于饮食或高温。
- 承重用途已另行复查。

stop_or_escalate：

- 不明化学污染。
- 强烈异味。
- 电池/燃料/药品容器。
- 深度腐蚀。
- 承重用途无法验证。

fallback：

- 降级为非承重、非饮食、非高温用途。
- 放入待检查区。
- 使用更确定的替代材料。

related_wiki 候选：

- `manufacturing-wood-selection-boundary-001`
- `manufacturing-metal-scrap-selection-boundary-001`
- `manufacturing-plastic-container-reuse-boundary-001`
- `manufacturing-fabric-rope-leather-selection-001`
- `manufacturing-fastener-reuse-load-boundary-001`
- `manufacturing-salvaged-part-clean-sort-label-001`
- `manufacturing-material-prep-dry-clean-deburr-001`
- `manufacturing-material-downgrade-label-001`
- `manufacturing-corrosion-before-reuse-check-001`

### DG-0873 木材切割与夹持

scenario：用手锯、刀具或手钻处理木材，准备切割、打孔或重复加工。

steps：

1. 选择无明显腐朽、裂纹和潮湿变形的木材。
2. 测量并标记切割线、孔位和余量。
3. 将木材固定在稳定台面或支撑上。
4. 确认手、脚和旁人不在切割路径上。
5. 低速开始，发现夹锯、开裂、晃动立即停止。
6. 完成后处理毛刺和尖角。
7. 记录尺寸、失败件和可复用模板。

check：

- 木材已固定。
- 切割线清楚。
- 孔位边距足够。
- 支撑不晃。
- 成品无明显锐边。

stop_or_escalate：

- 木材无法固定。
- 裂纹延伸。
- 夹锯严重。
- 刀锯缺齿或松动。
- 低光或疲劳。

fallback：

- 改用更短、更薄或更稳定材料。
- 改用预制件。
- 暂停加工并标记失败件。

related_wiki 候选：

- `manufacturing-wood-selection-boundary-001`
- `manufacturing-clamp-before-cut-drill-001`
- `manufacturing-wood-measure-mark-cut-line-001`
- `manufacturing-wood-saw-cut-repeatable-support-001`
- `manufacturing-wood-drill-hole-edge-distance-001`
- `manufacturing-simple-jig-template-use-001`
- `manufacturing-repeat-cut-drill-template-001`

### DG-0874 简易支架/框架制作

scenario：用木材、旧板、角料或低负荷材料制作架子、框架、箱体、盖板或临时支撑。

steps：

1. 明确用途、承重、放置位置和失败后果。
2. 选择材料并排除腐朽、开裂和污染。
3. 先做简单尺寸图或路线卡。
4. 切割和打孔前固定材料。
5. 连接后检查方正、晃动、裂纹和尖角。
6. 使用前做逐级承重或功能复查。
7. 标记最大用途和检查日期。

check：

- 材料用途匹配。
- 连接点无松动。
- 结构不晃动。
- 尖角已处理。
- 有使用限制标记。

stop_or_escalate：

- 用于承人、吊挂重物或关键 shelter 结构。
- 连接点开裂。
- 木材腐朽。
- 使用中明显变形。

fallback：

- 降低用途。
- 增加支撑点。
- 改为非承重收纳件。
- 标记禁止承重。

related_wiki 候选：

- `manufacturing-simple-frame-square-check-001`
- `manufacturing-simple-shelf-bracket-check-001`
- `manufacturing-simple-box-crate-build-001`
- `manufacturing-door-cover-panel-basic-001`
- `manufacturing-wood-screw-joinery-basic-001`
- `manufacturing-finished-product-load-test-001`
- `manufacturing-quality-recheck-before-use-001`

### DG-0875 金属废料打孔与防割边

scenario：准备使用薄金属片、旧支架、罐片或金属废料制作低负荷部件。

steps：

1. 检查金属来源、锈蚀、裂纹、尖角和污染。
2. 排除不明化学容器、压力容器和高风险材料。
3. 固定金属片，禁止手扶钻孔。
4. 钻孔、弯折或剪切后立即处理毛刺。
5. 对边缘进行折边、包边或标记。
6. 连接后检查锐边、晃动和腐蚀点。
7. 记录用途和不可承重限制。

check：

- 金属来源可接受。
- 已固定。
- 锐边已处理。
- 无未标记尖角。
- 未用于高温、高压或关键承重。

stop_or_escalate：

- 不明压力容器。
- 电池或化学容器残片。
- 锐边无法处理。
- 深度腐蚀。
- 工具打滑或钻头卡死。

fallback：

- 改用木材或塑料低风险材料。
- 降级为非接触、非承重用途。
- 将材料隔离到危险废料区。

related_wiki 候选：

- `manufacturing-metal-scrap-selection-boundary-001`
- `manufacturing-thin-metal-drilling-support-001`
- `manufacturing-thin-metal-bending-boundary-001`
- `manufacturing-metal-edge-deburr-cover-001`
- `manufacturing-simple-metal-bracket-001`
- `manufacturing-metal-screw-rivet-wire-join-001`
- `manufacturing-corrosion-before-reuse-check-001`

### DG-0876 胶粘/捆扎/螺丝连接选择

scenario：制作或装配新物品时，需要在胶粘、捆扎、螺丝、铆接或铁丝连接之间选择。

steps：

1. 明确连接点是否承重、受潮、受热或需要拆卸。
2. 判断材料类型：木、金属、塑料、布料、绳索。
3. 清洁接触面，去除油污、粉尘和松散层。
4. 选择连接方式并记录限制。
5. 初次连接后做轻载试验。
6. 使用前复查松动、开胶、滑移和断裂。

check：

- 连接方式与材料匹配。
- 接触面已处理。
- 受力方向明确。
- 有失效信号检查。
- 不把临时连接当永久承重。

stop_or_escalate：

- 需要承人或吊挂重物。
- 潮湿、高温或油污无法处理。
- 材料脆裂。
- 连接后明显滑移。

fallback：

- 增加机械固定。
- 降低用途。
- 选择可拆卸临时方案并标记。
- 改用更可靠材料。

related_wiki 候选：

- `manufacturing-glue-tie-screw-choice-001`
- `manufacturing-fastener-reuse-load-boundary-001`
- `manufacturing-fabric-rope-leather-selection-001`
- `manufacturing-wood-screw-joinery-basic-001`
- `manufacturing-metal-screw-rivet-wire-join-001`

### DG-0877 成品承重前检查

scenario：新做的架子、支架、箱体、挂点、框架或低负荷装置准备投入使用前，需要判断是否能承受预期重量。

steps：

1. 明确最大用途和失败后果。
2. 检查材料裂纹、腐朽、锈蚀、尖角和连接点。
3. 确认结构不晃动、不倾倒、不变形。
4. 从轻载开始逐级增加负荷。
5. 观察声音、松动、开裂、弯曲和位移。
6. 标记最大用途、禁止用途和复查日期。
7. 记录检查人和结果。

check：

- 连接点稳定。
- 无明显变形。
- 无锐边伤人风险。
- 负荷测试有记录。
- 限制用途已标记。

stop_or_escalate：

- 用于承人。
- 悬挂重物在睡眠区或火源上方。
- 测试中出现裂声、开裂、滑移、变形。
- 无法确认材料强度。

fallback：

- 降级为轻载或非承重用途。
- 增加支撑但重新测试。
- 标记禁止使用并隔离。

related_wiki 候选：

- `manufacturing-finished-product-load-test-001`
- `manufacturing-quality-recheck-before-use-001`
- `manufacturing-simple-shelf-bracket-check-001`
- `manufacturing-simple-metal-bracket-001`
- `manufacturing-defect-isolation-rework-log-001`
- `manufacturing-simple-lever-load-boundary-001`
- `manufacturing-pulley-rope-wheel-basic-boundary-001`

### DG-0878 收工清点与危险工具封存

scenario：完成制作、修整、切割、打孔或批量加工后，需要清点工具、材料、碎屑、半成品和危险品。

steps：

1. 停止所有切割、钻孔、打磨和加热动作。
2. 清点刀具、锯、钻头、夹具和紧固件。
3. 收集金属锐边、木刺、碎片和粉尘。
4. 将原料、半成品、成品、缺陷品分区存放。
5. 危险工具封存并标记权限。
6. 记录成品数量、失败件、待复查项和次日风险。

check：

- 危险工具已封存。
- 碎屑锐边已清理。
- 缺陷品未混入成品。
- 半成品有标记。
- 有收工记录。

stop_or_escalate：

- 工具缺失。
- 锐片无法清理。
- 危险材料混入普通区。
- 儿童可能接触工具或半成品。

fallback：

- 暂停工作区使用。
- 设置临时隔离线。
- 手写缺失工具和危险点。
- 次日开工前优先复查。

related_wiki 候选：

- `manufacturing-end-of-work-clean-count-seal-001`
- `manufacturing-raw-semi-finished-finished-zones-001`
- `manufacturing-production-batch-record-001`
- `manufacturing-defect-isolation-rework-log-001`
- `manufacturing-salvaged-part-clean-sort-label-001`
- `manufacturing-work-zone-minimum-layout-001`

## 6. Retrieval 风险预测

### 6.1 可能抢主位的领域

|竞争领域|典型 query|风险|建议边界|
|---|---|---|---|
|repair|“胶水粘不住”“固定点松了”“旧木板坏了”|Repair 可能吞掉制造入口|修旧物时 repair 主导；制作新成品、选择连接方式、承重前检查时 manufacturing 主导|
|tools|“怎么锯”“怎么钻”“工具坏了”|Tools safety 可能成为唯一证据|工具使用安全可以 secondary；生产目标、材料和质量检查应由 manufacturing 主导|
|energy|“打磨有火花”“电钻异常”|能源/电气安全可能抢主位|电源、电池、漏电、火花引燃时 energy/safety 主导；普通材料加工仍 manufacturing 主导|
|shelter|“做一个架子/门板/支撑”|Shelter 可能抢结构用途|制作部件由 manufacturing 主导；放置、居住区分区和 shelter 风险作为 secondary|
|waste / recycling|“废木板能不能用”“旧容器能不能用”|回收分类可能抢主位|处理废弃物时 recycling 主导；为制作新物选材时 manufacturing 主导|
|medical / safety|“金属边很锋利”“割到手了”|安全或医疗可能抢主位|未受伤前是 manufacturing 防割边；已经受伤则 medical 主导|
|agriculture|“做育苗箱/支架”|Planting 可能抢主位|作物管理由 planting 主导；箱体/支架制作由 manufacturing 主导|

### 6.2 重点 query 判断

1. “做一个架子”  
   Manufacturing 应主导，Shelter / Repair 只能补充。关键 evidence 是材料选择、连接方式、承重前检查。

2. “废木板能不能用”  
   如果目标是丢弃或分类，waste/recycling 主导；如果目标是制作新物，Manufacturing 主导。

3. “金属边很锋利”  
   未受伤时 Manufacturing 防割边主导；已割伤、出血或感染风险时 Medical 主导。

4. “胶水粘不住”  
   维修旧物时 Repair 主导；新制品连接方式选择时 Manufacturing 主导。

5. “承重前检查”  
   新成品或自制支架由 Manufacturing 主导；旧固定点失效由 Tools / Repair 主导。

### 6.3 后续可能需要的 query profile

本阶段不建议新增 profile。Field Test 后可判断是否需要：

- `manufacturing_material_reuse`
- `manufacturing_workshop_safety`
- `manufacturing_simple_woodwork`
- `manufacturing_metal_edge_drill`
- `manufacturing_load_quality_check`

## 7. Batch9-C Apply 建议

### 7.1 推荐方案

建议 Batch9-C 选择“基础 Wiki + 少量 Guide + evidence chain”，不提前修改 Retrieval/profile。

|项目|建议|
|---|---|
|新增 Wiki|34-36 篇|
|新增 Guide|6 个优先，最多 8 个|
|优先级|P0 全部 + 核心 P1 木工/金属/工坊流程|
|Guide-Wiki|建立双向关系，避免批量硬关联|
|Retrieval/Profile|不修改，等 Field Test 后再判断|
|PocketBase|按项目当前 Apply 流程同步|
|下一步|Batch9-D Manufacturing Field Test|

### 7.2 Batch9-C 推荐 Wiki 范围

建议第一批包含：

- P0 安全：1-8 全部。
- P0 材料：9-18 全部。
- P1 木工：19-24 优先，25-26 可选。
- P1 金属：27-32 全部。
- P1 工坊流程：33-38 全部。
- P2 治具/机械：39-40 可选；41-42 暂缓。

这样第一批约 36-40 篇。如需严格控制，建议暂缓 `manufacturing-simple-box-crate-build-001`、`manufacturing-door-cover-panel-basic-001`、`manufacturing-defect-isolation-rework-log-001`、P2 治具/机械。

### 7.3 Batch9-C 推荐 Guide 范围

第一批建议新增 6 个 Guide：

1. DG-0871 工坊开工前安全检查。
2. DG-0872 材料再利用前判断。
3. DG-0873 木材切割与夹持。
4. DG-0875 金属废料打孔与防割边。
5. DG-0876 胶粘/捆扎/螺丝连接选择。
6. DG-0877 成品承重前检查。

可选同步：

- DG-0874 简易支架/框架制作。
- DG-0878 收工清点与危险工具封存。

如果只能做 6 个，优先保留 DG-0878，暂缓 DG-0874，因为工坊收束对长期安全和重复生产更基础。

### 7.4 Field Test 设计方向

Batch9-D 建议 18-22 个 case，覆盖：

- 用废木板做架子。
- 开工前工作台检查。
- 切割前是否必须夹持。
- 低光下是否继续切割。
- 儿童靠近工具区。
- 废木板能否承重。
- 塑料容器能否用于饮水/食物/零件。
- 金属片锐边如何处理。
- 金属片打孔能否手扶。
- 胶水、扎带、螺丝如何选择。
- 成品承重前如何测试。
- 成品有裂声是否继续用。
- 收工后刀具和碎屑如何封存。
- 缺陷品是否能混入成品区。
- 重复切割是否需要模板。
- 废料存放上限和危险品区。
- 制作育苗箱时 agriculture 是否抢主位。
- 做临时门板时 shelter 是否抢主位。

### 7.5 延后到 Manufacturing v0.2

- 更完整的治具系统。
- 滑轮、杠杆、简单传动的操作 Guide。
- 小批量生产排程。
- 工坊角色分工和权限。
- 更细木工结构。
- 更细金属连接和表面处理。
- 原料库存和质量批号体系。

## 8. 不建议投入方向

当前不建议投入以下内容：

1. 高复杂工业制造。
2. 金属冶炼、铸造和热处理深水区。
3. 电池自制或电芯重组。
4. 燃油发动机维修深水区。
5. 枪械、武器或攻击性装置制造。
6. 危险化学品生产。
7. 专业机加工、车床、铣床、CNC 教程。
8. 高压容器、压力系统和锅炉制作。
9. 建筑工程级承重结构设计。
10. 高压电系统或并网电力工程。

原因：这些方向风险高、验证困难、依赖专业设备或法规约束强，短期内无法提升 LanternBox 的“个人长期生存节点”基础制造能力。

## 9. 结论

Manufacturing / Production 当前应维持 Red 判断。项目已有 Tools / Repair 的安全底座，但真正的制造生产能力尚未建立：缺少专用 Wiki、专用 Guide、工坊流程、材料预处理、成品质量复查和生产记录。

建议下一阶段进入 Batch9-C：Manufacturing & Production Foundation Apply。第一批应以 P0 安全、P0 材料、P1 木工/金属低复杂制作、工坊开收工和成品承重复查为核心，不提前新增 Retrieval profile。完成 Apply 后进入 Field Test，再通过 Root Cause Review 决定是否需要最小 profile 修复。
