# LanternBox Batch16-E: Cross-Domain Contract Revision

生成日期：2026-08-25

输入：

- `docs/knowledge/batch16_b_cross_domain_contract_design.md`
- `docs/knowledge/batch16_d_cross_domain_contract_validation_report.md`

本阶段只修订设计，不执行 Apply。未修改 contract data、tests、Retrieval、profiles、Wiki、Guide 或 database。

## 1. Existing Contract Review

Batch16-D 证明 v1.0 Contract Layer 已具备部分跨域泛化能力：

- Battery abnormal 可从电池泛化到移动电源，并保持 Energy Safety primary。
- 明确起火与浓烟可提升 Fire primary。
- Energy、Manufacturing / Repair、Agriculture 的记录 lifecycle / trust 均可由 Records 主导。
- Spare parts 等技术物资可由 Storage primary、Records supplemental。

同时暴露三个独立语义缺口：

1. Fire signal 使用无局部语境的子串命中，“没有看到明火”仍触发 Fire primary。
2. Active Hazard 没有 Human Injury 到 Medical primary 的 ownership 路径；“已经断电”还会错误结束对伤者的紧急判断。
3. Physical Storage 对象词表只覆盖技术物资，未覆盖种子等 Biological Material。

修订后的治理顺序为：

1. 识别当前 Human Harm；有人身伤害时先确定 Medical ownership。
2. 对 Fire positive evidence 执行局部否定与历史过滤。
3. 仍有未被否定的当前 Fire evidence 时进入 Fire；否则由 Energy Safety 处理设备异常。
4. 当前危险与人员伤害处理后，再执行 Information Object Contract。

这是一份 v1.1 设计修订，不改变 Batch16-B 的“当前危险优先于优化、维修、记录和储藏”总原则。

## 2. Fire Revision

### 2.1 Negative Fire Evidence Contract

Fire primary 不能只由“明火、烟、燃烧”等词是否出现决定。契约必须区分 positive evidence、negative evidence、historical evidence 与当前设备异常。

#### Negative Fire Evidence

以下描述本身不能触发 Fire primary：

- 没有明火
- 未见明火
- 看不到火焰
- 没有烟
- 未冒烟
- 无烟气
- 无燃烧
- 没有起火
- 未发生燃烧
- 火已熄灭且没有复燃迹象

否定范围必须绑定到同一 hazard signal 或同一局部语句。例如：

- “没有看到明火，但充电器仍在冒烟”否定的是明火，不是否定冒烟。
- “没有烟，但接头越来越烫”否定的是烟，不是否定异常热。
- “不要等到起火才停用”是禁止性警告，不是当前已经起火。

不得把句中任意“没有/无/未”当作全句否定，也不得因否定句包含“明火/燃烧”子串而提升 Fire。

#### Historical Fire Evidence

以下历史表达在没有当前危险证据时不能触发 Fire primary：

- 昨天/上周/此前曾经起火
- 历史事故记录
- 事后复盘
- 火已熄灭、现场已安全
- 过去出现过烟气或焦味

历史事件的默认 ownership：

- 问复盘、归档、原因或整改：Records primary，Safety / Fire supplemental。
- 问恢复使用或重新通电：Energy Safety primary，必须先确认当前安全状态。
- 仍有复燃、余热、烟气、火花或人员暴露：不退出 Active Hazard。

#### Positive Evidence Override

局部否定或历史描述不能压制后续明确的当前 positive evidence：

- “之前没有明火，现在已经起火” -> Fire primary。
- “火已熄灭，但又开始冒浓烟并升温” -> Fire primary。
- “昨天曾冒烟，今天设备无异常，想归档记录” -> Records primary。

决策必须按时间与语句范围选择最新、仍有效的状态，不能简单地让 negative 或 historical term 全局覆盖。

### 2.2 Revised Fire / Energy Safety Boundary

|有效证据|Primary|Secondary / required boundary|
|---|---|---|
|未被否定的明火、起火、燃烧、火势扩大、浓烟蔓延、热失控|Fire|Energy Safety；有人受伤时 Medical|
|设备冒烟/焦味/火花/异常热，但明确无明火或无燃烧|Energy Safety|Fire escalation boundary|
|异常热但无烟、无明火|Energy Safety|停止、断开、隔离|
|只有历史火灾，现场已安全，问题是复盘/归档|Records|Fire / Safety supplemental|
|历史火灾后询问能否重新通电|Energy Safety|Repair 只能在安全确认后进入|

明确：异常热和设备异常属于 Energy Safety，不属于 Fire。Fire secondary 的存在不代表 Fire primary。

### 2.3 Fire Contract Output

修订后 Fire decision 必须提供：

1. 有效 positive fire evidence。
2. 被排除的 negative / historical evidence 及其作用范围。
3. Fire primary 或 Energy Safety primary 的明确结论。
4. 即使 Fire 未触发，设备冒烟、火花、焦味、过热仍保持 STOP / disconnect / isolate。

## 3. Medical Priority Revision

### 3.1 Human Harm Priority Contract

Human Harm 与设备危险是两个并行状态。危险源断电、停用或隔离只能结束设备暴露，不代表人员伤害已经解除。

Human Harm 触发需要：

- 人员对象：有人、成员、操作人员、儿童、伤者、患者等；以及
- 当前伤害或症状：电击/触电、烧伤/烫伤、烟气吸入、呼吸困难、咳嗽头晕、漏液接触、皮肤或眼口刺激、人员受伤。

### 3.2 Ownership Rule

|人身状态|Primary|Required secondary|不得抢 primary|
|---|---|---|---|
|无人身伤害，仅设备异常|Energy Safety 或 Fire|按设备危险层级决定|Medical|
|有人电击伤、烧伤、烟气吸入或其他当前伤害|Medical|Energy Safety；有烟火时 Fire|Repair、Deepening、Management、Records、Storage|
|有人受伤且危险源仍在|Medical primary|Safety / Fire 必须同时提供先脱离危险源、断电、撤离边界|Repair、Records|
|危险源已断电但伤者仍有症状|Medical primary|Energy Safety 确认现场保持隔离|Repair|
|明确无人受伤、无人接触且只有设备异常|Energy Safety / Fire|Medical 不强制进入|Deepening、Management、Repair 按 Active Hazard 规则抑制|
|历史受伤事件、人员已稳定且问题是复盘|Records|Medical / Safety supplemental|当前 Medical 不必 primary|

明确规则：

- 无人身伤害：Safety primary。
- 有当前人身伤害：Medical primary，Safety secondary。
- “设备已经断电”不能单独取消 Medical primary。
- “无人受伤”“没有人接触”“只是设备异常”属于 Human Harm negative evidence。

### 3.3 Fire and Medical Interaction

有人受伤且现场仍有明火时，ownership 与动作顺序必须同时表达：

- 查询焦点是伤者处置：Medical primary，Fire / Safety mandatory secondary。
- Fire / Safety 先保证脱离危险区和不可返回；Medical 行动不得要求在危险区内停留。
- 查询焦点是多人撤离、出口或火势：Fire primary，Medical mandatory secondary。

因此 Human Harm Priority 是对设备 Safety ownership 的 override，不是取消 Fire 的撤离边界。实现必须保留 query focus 与 scene safety 两个维度。

### 3.4 Medical Exit Conditions

只有在以下条件明确时，Medical 才可退出 primary：

- 明确无人受伤或无人暴露；或
- 伤者已经完成评估、症状稳定/解除，当前请求已转为事后记录、设备修复或归档。

仅有“已断电”“已灭火”“设备已隔离”不满足 Medical exit。

## 4. Information Object Revision

### 4.1 Expanded Physical Object Classes

Information Object Contract 的核心仍是判断“被操作的是信息还是实物”。Physical Object 从单一技术物资扩展为以下类别：

|类别|典型对象|默认 ownership|
|---|---|---|
|Technical Material|设备、工具、零件、电池、燃料、材料、成品|Storage / 对应 Safety 域|
|Biological Material|种子、种苗、繁殖材料、培养物、土壤或生物实物样本|Seed Library / Agriculture Storage / Bio Safety，Records supplemental|
|Consumable|食物、饮水、药品、燃料、卫生耗材、一次性用品|对应 Food / Water / Medical / Energy Storage，Records supplemental|
|Sample|医疗样本、农业样本、材料样本、环境样本|按样本本体、采集动作、检测结果分别判断|

新增类别不意味着所有对象都交给通用 Storage。Contract 先确认它是物理对象，再由对象所属领域决定具体 custody、环境、安全和保存规则。

### 4.2 Biological Material

种子本体属于 Physical Object：

- 装袋、容器、防潮、避光、温度、批次分区、入库、轮换 -> Seed Library / Storage primary。
- 标签、批次号、复查日期 -> Records supplemental。
- 霉变、污染、发热或虫害 -> Agriculture / Seed Safety 先于普通 Storage。

种植实验记录属于 Information Object：

- 发芽率数据、试验日志、跨季比较、版本、可信度、归档 -> Records primary。
- 当前播种或种植动作 -> Agriculture primary，Records supplemental。

对象主题“种子”不能让实验记录变成物理 Storage；动作“装袋入库”也不能让种子本体变成 Records primary。

### 4.3 Consumable

Consumable 按请求动作决定：

- 实物库存、保质条件、轮换、污染隔离、领用 -> 对应 Storage / Safety 域 primary。
- 消耗记录、批次日志、版本、可信度、长期趋势 -> Records primary。
- 当前用量分配或治疗使用 -> 对应 Management / Medical 域 primary。

“耗材记录”是信息对象；“贴标签后入库的耗材”是物理对象。

### 4.4 Sample

Sample 必须区分三种对象：

1. 样本本体：装在容器中的血液、尿液、拭子、土壤、植物组织、材料切片等。
2. 采集行为：如何取得样本。
3. 样本数据：检测结果、照片、编号、报告与可信度。

|请求|Primary|Secondary|
|---|---|---|
|医疗样本本体的容器、温度、隔离、交接|Medical / Bio Safety custody|Records 记录编号与链路|
|采集医疗样本|Medical|Records 记录同意、时间与来源|
|医疗样本检测结果归档、版本或可信度|Records|Medical 作为业务域|
|农业样本本体保存|Agriculture / Storage|Records|
|样本污染、容器破裂或人员暴露|对应 Safety / Medical|Storage / Records 后置|

明确：医疗样本不因“样本”一词自动进入通用 Storage，也不因带编号自动进入 Records。必须根据被操作的是样本本体、采集动作还是结果数据判断。

### 4.5 Revised Information Object Decision

修订后的顺序：

1. 是否有当前 Human Harm 或 Active Hazard；若有，先执行 Medical / Safety / Fire。
2. 被操作的是信息对象还是物理对象。
3. 物理对象属于 Technical、Biological、Consumable 还是 Sample。
4. 请求动作是保存/custody、采集/使用，还是 lifecycle/trust。
5. 确定业务域 primary 与 Records supplemental，或 Records primary 与业务域 supplemental。

## 5. Backward Compatibility Analysis

### Active Hazard Compatibility

|既有边界|修订后结果|兼容性|
|---|---|---|
|A01 鼓包电池仍想测容量|Energy Safety primary|保持|
|A02 控制器正在冒烟|Energy Safety primary，Fire escalation|保持|
|A03 未知来源电源并联|Energy Safety primary|保持|
|A04 拆包换芯|Energy Safety primary|保持|
|N01 正常容量下降复查|Energy Deepening|保持|
|N02 Solar 跨季基线|Energy Deepening|保持|
|明确明火/浓烟扩大|Fire primary|保持|

Negative Fire Evidence 只过滤被局部否定或纯历史的 Fire signal，不降低冒烟、火花、焦味、异常热的 Energy Safety 优先级，也不削弱明确起火的 Fire priority。

### Information Object Compatibility

|既有边界|修订后结果|兼容性|
|---|---|---|
|Energy records archive|Records|保持|
|Manufacturing repair logs|Records|保持|
|Agriculture experiment records|Records|保持|
|备用电池长期保存|Storage / Energy Safety|保持|
|设备贴标签后入库|Storage primary，Records supplemental|保持|
|Spare parts storage|Storage primary，Records supplemental|保持|

Physical Object Expansion 是类别扩展，不改变既有信息对象定义。新增 Human Harm Priority 只在当前人员伤害存在时生效，不影响无人受伤的设备异常。

## 6. Batch16-D Failure Mapping

|Batch16-D gap|v1.1 revision|预期结果|
|---|---|---|
|“没有看到明火”被判 Fire|Negative Fire Evidence + 局部否定范围|Energy Safety primary，Fire secondary|
|电击烧伤且设备已断电无决策|Human Harm Priority + 独立 Medical exit|Medical primary，Energy Safety secondary|
|种子本体无法分类|Biological Material physical class|Seed Library / Storage primary，Records supplemental|

三个缺口均有明确语义修订，不需要修改 Retrieval ranking、selector、Wiki 或 Guide。

## 7. New Validation Cases

### Fire Negative / Positive

|ID|Query intent|Expected|
|---|---|---|
|F-N01|充电器冒烟但没有看到明火|Energy Safety primary；Fire secondary|
|F-N02|接头异常热，但没有烟、无燃烧|Energy Safety primary|
|F-N03|昨天起火，已经熄灭且现场安全，现在做复盘|Records primary；Fire/Safety supplemental|
|F-P01|设备已经起火并出现浓烟|Fire primary|
|F-P02|之前没有明火，现在已经起火|Fire primary；current positive overrides prior negative|

### Human Harm

|ID|Query intent|Expected|
|---|---|---|
|H-01|人员电击烧伤，设备已断电|Medical primary；Energy Safety secondary|
|H-02|人员吸入烟气后咳嗽头晕|Medical primary；Fire/Safety secondary|
|H-03|设备火花但明确无人受伤|Energy Safety primary|
|H-04|有人受伤且现场仍有明火，询问伤者处置|Medical primary；Fire mandatory secondary；先撤离危险区|
|H-05|无人受伤、设备已断电，询问外部线缆检查|Repair 可在安全确认后 primary|

### Physical Object Expansion

|ID|Query intent|Expected|
|---|---|---|
|P-01|种子本体装袋、防潮、入库|Seed Library / Storage primary；Records supplemental|
|P-02|种植试验日志与发芽率数据归档|Records primary|
|P-03|医疗样本试管的低温保存与交接|Medical / Bio Safety custody primary；Records supplemental|
|P-04|医疗样本检测结果标记可信度并归档|Records primary；Medical supplemental|
|P-05|卫生耗材贴标签后入库存放|Medical Storage primary；Records supplemental|
|P-06|卫生耗材消耗日志做长期趋势|Records primary；Medical supplemental|

Revalidation 必须同时运行 Batch16-C 原始 contract cases 与 Batch16-D validation cases，确保修订不是用新增类别替换既有边界。

设计已完整覆盖 Fire negative evidence、Human Harm override 和 Physical Object expansion，并给出正反边界、优先级、退出条件、兼容性与新验证矩阵。

最终状态：**READY FOR REVALIDATION**
