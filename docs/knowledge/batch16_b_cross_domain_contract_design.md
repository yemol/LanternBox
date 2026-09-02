# LanternBox Batch16-B: Cross-Domain Ownership Contract Design

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_a_cross_domain_ownership_debt_plan.md`
- `docs/knowledge/batch15_g_energy_system_deepening_boundary_review.md`

本阶段只设计契约。未修改 Wiki、Guide、Retrieval、profile、ranking、selector、tests、schema 或 database。

## 1. Design Scope

本设计定义两个 implementation-neutral 的跨域治理契约：

1. Active Hazard Contract：当前危险或高风险侵入式动作必须先进入 Safety / Fire。
2. Information Object Contract：先判断被处理的是信息对象还是物理对象，再决定 Records 或 Storage 主位。

契约只规定语义输入、优先级、允许与禁止的 ownership。它不规定具体 profile、分数、ranking 权重、selector 数量或 schema 实现。

两个契约的总优先级：

1. 当前人身、火灾、电气或电池危险。
2. 当前业务处置。
3. 事后记录、归档、趋势与可信度。
4. 长期物理储藏。

若两个契约同时出现，Active Hazard Contract 先处理当前危险；现场安全并完成隔离后，Information Object Contract 才处理记录与物资的后续去向。

## 2. Active Hazard Contract

### 2.1 Contract Intent

当查询包含正在发生或尚未排除的危险时，系统必须先提供停止、断开、隔离、撤离或升级判断，不能先提供容量诊断、产出优化、排程、维修或长期分析。

覆盖危险族：

- Battery abnormal：鼓包、漏液、异常热、异味、腐蚀、开裂、嘶嘶声、快速变形。
- Smoke：冒烟、浓烟、烟味、焦味或来源不明烟气。
- Fire：明火、起火、火势扩大、热失控或燃烧迹象。
- Overheat：烫手、持续升温、线皮变软、接头变黑。
- Spark：火花、电弧、短路迹象、反复跳断或焦黑。
- Unknown dangerous state：来源、电压、极性、内部完整性、潮湿状态或损伤程度不明，同时有人准备通电、连接、充电、测试或拆解。
- Invasive battery action：拆包、换芯、点焊、电芯重组、BMS 改造或修理损坏锂电池。

### 2.2 Trigger Conditions

契约触发必须满足以下任一组合：

#### A. Object + Active Hazard State

对象属于电池、充电宝、电池包、充电器、太阳能控制器、低压设备、供电线、接头、电源模块或未知电源，并出现当前危险状态。

当前性信号包括“正在”“现在”“仍在”“继续”“刚刚”“越来越”“还有输出”“能不能继续用/充/测/接”。没有时间词时，鼓包、漏液、冒烟、明火、火花和快速升温默认按未解除危险处理。

#### B. Object + Invasive / Energizing Action

即使没有冒烟或鼓包，只要有人请求对电池包拆解、换芯、点焊、BMS 改造，或对来源/电压/极性不明的电源通电、并联、反送、接线、充电或测试，契约也必须触发。

#### C. Uncertainty + Consequence

“不确定/不知道/无法确认”不能单独触发。只有不确定项会直接影响通电、接触、连接、拆解、继续运行或人员暴露时，才归入 Unknown dangerous state。

### 2.3 Non-Trigger / Exit Conditions

以下情形不能只因历史危险词出现就保持 Active Hazard 主位：

- 明确说明已经断电、停用、隔离、冷却稳定且现场已安全，当前问题是事后复盘或记录归档。
- 正常成品电池无鼓包、漏液、异常热或损伤，只做非侵入式容量趋势复查。
- 已知低压、已确认电压与极性、已断开负载，只做允许范围内的测量或维护。
- 文本是在陈述禁止规则，例如“禁止并联”“不要拆包”，而不是允许危险操作。

退出 Active Hazard 主位至少需要明确的安全状态：危险源停止、断开或隔离，且没有继续冒烟、升温、漏液、火花或暴露。仅写“处理过了”不足以退出。

### 2.4 Priority and Ownership

契约只在触发后应用以下优先级，不构成全局 ranking：

|危险状态|Primary owner|Required secondary / escalation|不得取得 primary 的领域|
|---|---|---|---|
|明火、火势扩大、浓烟、出口受阻、热失控|Fire|Energy Safety；有人暴露或受伤时 Medical|Energy Deepening、Management、Repair、Records、Storage|
|电池鼓包、漏液、异常热、异味、损伤但无明火|Energy Safety|出现烟火升级时 Fire|Energy Deepening、Management、Repair、Storage|
|低压设备/太阳能控制器冒烟、火花、焦味、过热|Energy Safety|Fire 必须进入升级边界；出现明火/浓烟时 Fire 转 primary|Solar Management、Energy Deepening、Repair|
|未知来源/电压/极性并准备通电、连接、并联或测试|Energy Safety|必要时 Fire / professional escalation|Energy Deepening、Management、Repair|
|拆包、换芯、点焊、电芯重组、BMS 改造|Energy Safety|Fire stop boundary|Energy Deepening、Repair、Manufacturing|

同一查询中 Fire 与 Energy Safety 都可进入，但 primary 必须由当前危险层级决定：

- 已经有明火、浓烟蔓延或人员撤离问题：Fire primary。
- 设备局部冒烟、火花、异常热或电池异常，尚无明火蔓延：Energy Safety primary，Fire 为强制升级域。

### 2.5 Domain Boundaries

#### Energy Deepening

允许进入：现场已安全后，处理非侵入式容量复查、长期产出、冗余或降级模型。

禁止：在 Active Hazard 阶段以容量、效率、剩余输出、长期趋势或备用价值抢 primary；不得先测试再决定是否隔离。

#### Energy Safety

必须进入：电池异常、低压设备冒烟/火花/过热、未知电源通电风险、侵入式电池操作。

职责：停止、断开、隔离、不可通电、不可继续、专业升级；不提供危险拆解或高压操作步骤。

#### Fire

必须进入：明火、浓烟、燃烧扩大、热失控或无法安全确认火源状态。

允许作为 secondary：设备局部冒烟或焦味尚未发展为明火时，提供撤离与升级边界。

#### Repair

允许进入：危险源已断开、隔离、冷却且明确可安全检查后，处理外部可视低风险问题。

禁止：当前冒烟、火花、异常热、鼓包、漏液、未知带电状态或锂电池拆包换芯时取得 primary。Repair 不得把“修好”置于“停用隔离”之前。

#### Other Domains

- Energy Management / Solar Management：只能在危险解除后安排负载、充电或恢复顺序。
- Records：可记录事件，但当前危险时不能以日志、复盘或可信度抢 primary。
- Storage：只能在物品已安全分类后处理长期保存；异常电池不得进入普通储藏流程。
- Medical：有人吸入烟气、接触漏液、烧伤或电击时，可与 Fire / Safety 并列或转为 primary。

### 2.6 Required Contract Output

任何实现只要判定 Active Hazard Contract 触发，结果必须满足：

1. top ownership 是 Fire 或对应 Safety 域。
2. selected evidence 包含明确 STOP / 断开 / 隔离 / 撤离 / 不可继续之一。
3. 当前危险未解除前，不选择以诊断、优化、排程或维修为首要动作的入口。
4. 不因否定句中的危险词产生 dangerous suggestion false positive。
5. 真实允许危险操作的表达仍必须被安全检测拦截。

## 3. Information Object Contract

### 3.1 Contract Intent

“保存”“标签”“版本”“长期”同时可描述物理储藏和信息生命周期。契约必须先识别被操作对象，而不是根据记录内容中的业务域名词决定 ownership。

核心问题：

- 被保存的是电池、燃料、设备或物资本身：Storage。
- 被保存的是文件、数据、日志、记录、版本、索引或可信度信息：Records。

“太阳能记录”是 Records 对象；“太阳能控制器”是物理设备对象。内容主题不能覆盖对象类型。

### 3.2 Object Classification

|对象/意图|默认归属|边界说明|
|---|---|---|
|文件、纸本档案、卡片、表格|Records|负责版本、索引、归档、访问与可信度；纸张的防潮容器可由 Storage supplemental|
|数据、测量值、观察结果|Records|当问题是保存、来源、校验、单位、归档或可信度；若用于当前业务决策，业务域可 primary|
|日志、记录、台账、交接卡|Records|负责 lifecycle、时间、来源、状态与长期检索|
|趋势、跨月/跨季比较、重复问题|Records / business co-domain|找规律和证据链由 Records；当前能源、农业或维修动作可由业务域 primary|
|版本、CURRENT、SUPERSEDED、ARCHIVED|Records|负责版本状态、回滚依据与历史查询；固件故障本身可由 Repair/Tools 处理|
|可信度、verification、亲历/转述、来源冲突|Records|业务域不能自行把未验证记录提升为行动事实|
|物资、燃料、零件、工具、成品|Storage|负责物理位置、环境、分区、轮换、污染和储藏状态|
|电池、充电宝、设备本体|Storage / Safety|正常长期保存由 Storage；异常、冒烟、鼓包、漏液时 Active Hazard Contract 优先|
|设备状态|按请求动作判定|“设备状态记录/日志/历史”属于 Records；“设备当前异常怎么办”属于 Safety/Repair；“设备贴状态标签后入库”属于 Storage + Records co-domain|

### 3.3 Records Trigger Conditions

Records 主位需要“信息对象 + lifecycle / trust 动作”的复合语义：

- 信息对象：记录、日志、数据、文件、档案、版本、表格、台账、测量结果、观察结果。
- lifecycle 动作：归档、索引、备份、版本切换、长期检索、销毁判断、CURRENT / SUPERSEDED / ARCHIVED。
- trust 动作：标记可信度、验证来源、亲历/转述、冲突记录、verification、是否可用于行动。
- trend 动作：跨月/跨季比较、找规律、重复故障、长期变化。

单独出现“记录”“长期”“保存”不能自动让 Records primary；必须确认用户正在处理信息对象或其 evidence lifecycle。

### 3.4 Storage Trigger Conditions

Storage 主位需要“物理对象 + 保存条件/位置动作”的复合语义：

- 物理对象：电池、燃料、设备、工具、材料、零件、成品、食物、种子等。
- 保存动作：入库、分区、堆放、封存、轮换、防潮、避光、温湿度、远离火源、容器选择、污染隔离。

物理对象上的标签不自动变成 Records primary：若标签用于说明位置、批次、状态、复查日期并服务入库，Storage 可 primary，Records supplemental。

### 3.5 Ambiguity Resolution

按以下顺序消解歧义：

1. 识别语法/语义上的被操作对象。
2. 识别请求动作是信息 lifecycle、当前业务判断还是物理保存。
3. 识别当前危险；若有危险，先执行 Active Hazard Contract。
4. 再确定 primary 与 supplemental domain。

典型对照：

|表达|对象|Primary|
|---|---|---|
|“电池放三个月怎么保存”|电池本体|Storage / Energy Safety|
|“保存了三个月的电池记录怎么归档”|记录|Records|
|“给库存电池贴复查日期后入库”|电池本体|Storage，Records supplemental|
|“给电池测试记录标记可信度”|测试记录|Records|
|“设备现在冒烟，先记日志还是继续用”|当前危险设备|Active Hazard Contract；Energy Safety|
|“设备故障日志三个月内重复出现”|日志与趋势|Records / Repair co-domain，按请求动作决定 primary|

### 3.6 Priority and Ownership

|请求动作|Primary owner|允许进入|禁止抢占|
|---|---|---|---|
|记录归档、索引、版本、可信度|Records|记录所属业务域作为 supplemental|Storage 不能因内容出现电池/燃料而 primary|
|基于历史记录找趋势|Records 或业务域 co-primary|Repair、Energy、Agriculture 等|Storage 不参与，除非问题转为物理保存|
|正常物资/设备长期保存|Storage|Records 记录位置、状态和复查日期|Records 不能接管环境、分区和物理安全|
|当前设备异常|Safety 或 Repair|Records 事后记录|Records / Storage 不能先于当前处置|
|异常物资入库|Safety 先行|安全解除后 Storage；Records 留痕|普通 Storage 流程不得先行|

### 3.7 Required Contract Output

Information Object Contract 生效时，任何实现必须满足：

1. 明确区分信息对象与物理对象。
2. 信息 lifecycle / trust 查询由 Records primary。
3. 物理保存条件与位置查询由 Storage primary。
4. 业务域名词只作为记录主题，不改变信息对象归属。
5. 当前危险优先于 Records / Storage。
6. supplemental domain 不得覆盖 primary domain 的行动边界。

## 4. Historical Case Mapping

|Case|契约触发|应有主域|允许进入|必须被压制的错误主位|说明|
|---|---|---|---|---|---|
|O01|Active Hazard: 鼓包 + 仍想测容量|Energy Safety|Fire escalation；安全解除后 Deepening supplemental|DG-0921 / Energy Deepening|Safety candidate 已存在；异常状态必须先于容量复查|
|O05|Active Hazard: 太阳能控制器正在冒烟|Energy Safety|Fire 强制升级；出现明火/浓烟时 Fire 转 primary|Solar Management、Energy Deepening|当前为设备局部冒烟，先停用隔离；“还有输出”不能保留 Management 主位|
|O07|Active Hazard: 电池包 + 拆包换芯|Energy Safety|Fire stop boundary|Energy Deepening、Repair、Manufacturing|侵入式锂电池操作本身触发，不等待鼓包或冒烟|
|O08|Information Object: 电池/太阳能记录 + 长期归档 + 可信度|Records|Energy 作为记录主题；Storage 仅在另问物理介质保存时 supplemental|Long-Term Storage、Energy Management|被归档的是记录，不是电池或太阳能设备本体|

O05 的升级边界：当前仅为局部设备冒烟时 Energy Safety primary；若描述明火、浓烟扩大、撤离或出口风险，Fire primary。

## 5. Contract Interaction Matrix

|Active Hazard|Information Object|Primary decision|
|---|---|---|
|否|否|由现有业务域决定|
|是|否|Fire / Safety 按危险层级主导|
|否|是|Records 或 Storage 按对象类型主导|
|是|是|Fire / Safety 先处理；安全解除后 Records / Storage 处理后续|

示例：“电池正在冒烟，记录表怎么归档”仍必须先由 Energy Safety / Fire 主导；不能因为问到记录表就跳过停用、隔离或撤离。

## 6. Apply Contract Boundaries

未来 Apply 必须遵守：

- 先验证 candidate recall，再验证 top1 ownership，不用一次全局调权掩盖两层根因。
- 不修改 Energy Deepening Knowledge、Guide 或已冻结长期容量 profile。
- 不新增宽泛 Safety、Records 或 Storage profile。
- 不用 case ID、完整 fixture query 或 Guide ID 写生产硬编码。
- 不用全局 risk_level 排序代替语义契约。
- 每个契约都需要 positive 与 negative boundary；尤其保护正常容量复查、Solar 排程、Repair 安全后检查、物理电池储藏和业务域当前决策。
- Active Hazard 与 Information Object 应分批 Apply 和验证，确保回归可以归因。

设计已给出触发条件、退出条件、主位优先级、允许进入领域、禁止抢占领域、历史案例映射和冲突消解顺序，足以进入最小 Apply 评审。

最终结论：**READY FOR APPLY**
