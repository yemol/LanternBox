# LanternBox Batch14-B: Long-Term Records / Knowledge Transfer Coverage Plan

生成日期：2026-08-24

本阶段只做只读资产审查、能力边界定义和 Batch14-C 规划。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`；未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、参数、schema、PocketBase、tests、fixture、应用代码、FT-02 firmware、`.env`、`data/lanternbox.db` 或无关 dirty 文件，也未执行 Apply。

参考：

- `docs/knowledge/knowledge_coverage_map_v0_7.md`
- `docs/KNOWLEDGE_STRATEGY.md`
- Production、Long-Term Storage、Manufacturing、Waste、Agriculture Second Stage、Tools / Repair、Medical、Energy Safety / Management、Team / Organization 的现有 Wiki、Guide 和 stable 报告

## 0. 规划结论

Long-Term Records / Knowledge Transfer v0.1 不应建设成新的“通用日志中心”。现有知识库已经有事件、维修、种植、医疗、能源、生产、库存和交接记录；真正缺少的是这些记录之上的统一生命周期：

```text
现场原始记录
  -> 来源与上下文复核
  -> 可验证事实
  -> 成功 / 失败经验
  -> SOP / Knowledge Card 候选
  -> 人工审核与验证
  -> CURRENT 正式版本
  -> 运行结果再次反馈
```

Batch14-C 建议：

- 新增 **32 篇**系统级 Records Wiki。
- 复用 **13 篇**现有 Wiki，不复制具体域记录正文。
- 新增 **8 个**系统级 Guide，候选 ID 为 `DG-0911` 至 `DG-0918`。
- 只为 `DG-0144`、`DG-0401`、`DG-0529` 规划补充精准 evidence；其余无 evidence 的旧 `records` Guide 保持不动。
- Batch14-C 不新增 query profile，不修改 Retrieval；Batch14-D 先做 Field Test，Batch14-E 再做 Root Cause Review。

## 1. 当前资产全面审查

### 1.1 统计口径

- Wiki 总数按目标目录中的 Markdown 文件统计。
- Guide 总数按对应 `data/guides/<domain>` 中 JSON 文件统计。
- “显式记录类资产”按标题是否包含本任务给出的记录、日志、台账、交接、复盘、档案、观察、事件、事故、near miss、批次、版本、变更、经验、教学、培训、技能、SOP、知识卡、索引、可信度、来源、归档、废止、回滚、审核等词统计。
- 标题统计用于测量明确职责，不把正文中例行出现“记录”二字的所有知识都算成 Records 资产。
- 目录名不作为归属结论；例如 `data/guides/records` 中存在库存、医疗、排班、外出和种植操作 Guide。

### 1.2 目录与显式资产

|范围|Wiki 总数|显式记录类 Wiki|Guide 总数|显式记录类 Guide|当前判断|
|---|---:|---:|---:|---:|---|
|Records|3|2|25|10|目录小且 Guide 混域，不能直接当作系统覆盖度。|
|Data|29|17|0|0|可信度、事件、失败、归档、索引、备份和交接的主要基础池。|
|Team / Organization|20|5|5|2|已有团队复盘、交接、入组和技能冗余。|
|Production|41|13|8|2|批次、质检、near miss、WIP 和班次交接已经 stable。|
|Repair|100|12|5|0|维修履历、工具寿命、借还、失败和培训记录丰富。|
|Agriculture|93|18|20|2|种植、产量、跨批次、失败和交接记录丰富。|
|Storage|40|6|8|1|库存卡、异常日志、批次和交接已经 stable。|
|Medical|51|15|75|16|观察和连续记录丰富，但行动处置必须仍由 Medical 主导。|
|Waste|36|6|8|1|材料池、来源、失败和交接记录已存在。|
|Energy|74|7|72|6|充放电、容量、轮换、产出与交班记录已存在。|

这十个范围合计不是去重后的全库数量；同一能力可能由 Data 上层页和具体业务域页面共同承接。

### 1.3 Records Guide evidence 状态

|项目|数量|
|---|---:|
|Records Guide|25|
|有 `related_wiki`|10|
|无 `related_wiki`|15|
|`related_wiki` evidence 边|11|

已有 11 条边只连接 10 个 Wiki，其中 `general-small-team-handoff-001` 被两个 Guide 复用。当前 evidence 密度不足，但不能按目录批量补链，因为 15 个无链 Guide 中只有 3 个属于本规划主域。

建议在 Batch14-C 补 evidence：

|Guide|理由|候选 evidence|
|---|---|---|
|`DG-0144` 家庭成员信息包|属于成员档案、访问和离线交接边界。|复用 `general-member-info-backup-001`；关联新的访问保留规则。|
|`DG-0401` 五分钟技能教学|属于技能证据、监督练习和复训链。|新的技能等级、监督练习、复训 Wiki。|
|`DG-0529` 成员档案权限分级|属于 Records 隐私与最小访问分类。|新的记录保留/访问规则；复用医疗记录保存。|

以下 12 个无链 Guide 不应在 Batch14-C 顺手处理：

- `DG-0123`、`DG-0125`、`DG-0126`、`DG-0128`：Inventory / Storage。
- `DG-0164`、`DG-0327`：Medical / Care。
- `DG-0391`、`DG-0395`：Organization / Task allocation。
- `DG-0396`：Outdoor / Safety。
- `DG-0437`：Weather / Navigation。
- `DG-0513`、`DG-0515`：Agriculture / Storage。

### 1.4 明显重复与层级重叠

未发现 Records / Data 中标题完全相同的 Wiki；主要问题是语义层级重叠：

|重复簇|现有内容|判断|
|---|---|---|
|交接|`general-small-team-handoff-001`、Team 交接、Production 班次交接、Storage 交接、Repair 负责人交接、Energy 交班|不是删除型重复。Records 只保留长期摘要、原记录定位和跨域未结项，不复制具体班次字段。|
|失败 / 事故复盘|通用失败经验、Team 复盘、Production near miss、Repair 失败、Agriculture 失败、Waste 再利用失败|保留具体域现场模板；新增上层重复模式、改进验证和知识晋升规则。|
|维修记录|`general-repair-log-001` 与 Repair 内维修、耗材、寿命、借还记录|通用页可作为索引入口，具体怎么修仍归 Repair。|
|医疗记录|`general-medical-record-archive-001` 与 Medical 观察记录|Records 只管格式、隐私、交接和归档；症状判断与处置不迁移。|
|农业批次|`general-seed-batch-record-001` 与种植、发芽率、产量、土壤和跨季记录|Records 只新增跨季比较和经验验证，不复制种植字段。|
|能源日志|通用能源日志、充放电、天气产出、日终复盘、Energy 内容量记录|Records 只做长期趋势和来源可信度；当日电量调度仍归 Energy。|
|知识交接 / 新成员|通用知识交接、Team onboarding、技能双人掌握、五分钟教学|新增授权等级、监督证据和复测闭环，不建设 HR 系统。|

### 1.5 已有但没有统一上层入口

- 事件、任务、批次、设备、地点之间没有稳定的跨域关联规则。
- 记录可信度有单页，但没有与 review、verification、冲突记录和晋升状态组成闭环。
- 各域都有复盘，没有“重复发生多少次才值得提取经验”的统一判断入口。
- 知识卡已有归档概念，但没有 CURRENT、SUPERSEDED、ARCHIVED 的默认行动规则。
- `docs/KNOWLEDGE_STRATEGY.md` 已定义 Experience / Knowledge Inbox 审核后进入 Guide、Wiki 或 Archive，但缺可执行 promotion gate。
- 新成员、培训和技能冗余存在，没有回答“当前可以安全独立做什么”的统一记录。
- LanternBox Journal、Task、Terminal Sync、Audio Log、Track 和 FieldManual 已有事实载体，没有统一说明其记录何时只是 raw evidence、何时可成为正式知识。

### 1.6 完全缺失的系统能力

1. 记录成熟度状态与正式知识版本状态的分离模型。
2. `RAW -> REVIEWED -> VERIFIED -> LESSON` 的人工晋升条件。
3. `SOP_CANDIDATE -> CURRENT -> SUPERSEDED / ARCHIVED` 的版本门禁。
4. 冲突记录并存、复核和禁止覆盖原文的统一规则。
5. 独立来源、适用条件、验证方式和 reviewer 的最低字段。
6. 改进项是否真正降低重复失败的关闭验证。
7. 当前 SOP 清单、旧版隔离和回滚后重新验证。
8. 技能授权等级、禁止任务、监督任务和长期未练习复训。
9. 医疗、成员、路线和安全记录的最小访问分类。
10. FT-02 原始证据与 Core 审核事实之间的信息接口。

## 2. Long-Term Records / Knowledge Transfer 能力边界

### 2.1 本领域主导能力

|能力|Records 主导内容|完成标准|
|---|---|---|
|A. Record Lifecycle|原始记录、日志、观察、批次结果、失败、near miss、交接事实和后续复查的状态。|任何条目都能看出来源、当前成熟度、下一复核动作和是否关闭。|
|B. Record Trust|来源、时间、记录者/设备、亲历/转述、不确定性、复核、适用条件和冲突记录。|不确定信息不会伪装成已确认事实。|
|C. Review / Learning|成功/失败复盘、重复故障、重复风险、有效做法、原因分类、假设和改进验证。|复盘产生可测试的下一步，而不是“以后注意”。|
|D. Knowledge Promotion|`raw record -> reviewed fact -> lesson -> candidate -> verification -> current`。|正式知识必须经过人工审核和验证。|
|E. Version Control|版本号、生效日、修订原因、范围、废止、回滚和历史保留。|只有 CURRENT 默认指导行动。|
|F. Archive / Index|统一索引、标签、日期、对象、事件、任务、批次、设备、地点、归档和复查周期。|完全离线时能找到原始记录、当前知识和历史版本。|
|G. Knowledge Transfer|学习路径、低风险首任务、监督、技能事实、授权、禁做任务、复测、复训和交接摘要。|能回答某成员当前可安全独立执行什么。|

### 2.2 相邻 stable domain 边界

|相邻域|相邻域主导|Records 只负责|Records 停止线|
|---|---|---|---|
|Production|今日任务、WIP、质检、班次交接。|长期生产历史、趋势、经验晋升、SOP 版本。|查询当前班次或未完成品下一步时，返回 Production 主位。|
|Medical|诊断、急救、症状处置、医疗行动。|观察格式、照护交接摘要、隐私、结束与归档。|出现流血、呼吸异常、意识异常等当前症状时，立即转 Medical。|
|Storage|库存、FIFO、储藏异常、储藏条件。|跨批次历史、长期趋势和 Storage SOP 变更。|当前库存数量、位置、隔离和交接由 Storage 主导。|
|Repair / Manufacturing|怎么修、是否停用、替代方法、怎么制作。|维修履历、重复故障、失败模式和经验总结。|当前破损、承重、工具安全或制造步骤由 Repair / Manufacturing 主导。|
|Agriculture|种植、病虫害、土壤、种子和收获。|跨季对比、地点/天气/批次历史和经验验证。|当前作物处置与种植动作由 Agriculture 主导。|
|Waste|废弃物分类、污染和材料可用性判断。|来源与处理历史、重复失败和经验晋升。|当前材料是否可回收、清洁或降级由 Waste 主导。|
|Energy|预算、充电、电池安全、负载优先级。|容量趋势、产出历史、维护记录和退化事实。|当前电量怎么分配或异常电池怎么处置由 Energy 主导。|
|Organization|人员分工、值班、团队制度。|技能事实、授权历史、培训/复训记录和经验传承。|Records 不决定谁应担任岗位，不做绩效评价。|

## 3. Record Lifecycle

### 3.1 采用两条状态轴

建议不要把事实可信度与正式文档版本混成一条状态链。

**记录成熟度轴：**

```text
RAW -> REVIEWED -> VERIFIED -> LESSON -> SOP_CANDIDATE
```

**正式知识版本轴：**

```text
SOP_CANDIDATE --批准并发布--> CURRENT
CURRENT --新版本生效--> SUPERSEDED
CURRENT / SUPERSEDED --退出日常使用--> ARCHIVED
SUPERSEDED --人工回滚和重新验证--> CURRENT(new version)
```

`APPROVED` 应保留为审核决定和审核记录，不再作为与 CURRENT 并列的长期状态。批准成功后，正式条目获得版本号并进入 CURRENT；批准失败则仍是 candidate 或退回补证。

### 3.2 状态必要性与晋升控制

|状态|是否必要|进入条件|是否可自动进入|人工审核|
|---|---|---|---|---|
|RAW|必要|现场原始记录已保存，允许不完整但必须标出缺失。|可以由 FT-02 / Core 创建。|创建不需审核。|
|REVIEWED|必要|来源、时间、对象、上下文、事实/推测已检查。|否；系统只能提示缺字段。|需要记录者或指定 reviewer。|
|VERIFIED|必要|有独立来源、重复观察、实际复现或可靠外部依据支持。|否；相似记录聚类不等于验证。|需要 reviewer。|
|LESSON|必要|适用条件、结果、失败边界和下次改变已明确。|否。|需要能够理解业务域的人审核。|
|SOP_CANDIDATE|必要|可能改变行动步骤、停止线、检查项或正式知识。|否。|需要 domain reviewer。|
|APPROVED|不作持久状态|作为批准/驳回决定事件保存。|否。|必须。|
|CURRENT|必要|候选通过审核、冲突检查和所需验证，并分配版本。|否。|必须；高风险需更严格审核。|
|SUPERSEDED|必要|有更高版本 CURRENT 明确取代它。|可由批准动作机械标记，但决定不能自动。|版本批准者确认。|
|ARCHIVED|必要|只保留历史、审计或学习价值，不再默认行动。|可提示到期，不可自动归档关键知识。|责任人确认。|

### 3.3 LanternBox 可以自动辅助但不能决定的事项

可以辅助：自动时间、设备 ID、有效 GNSS、文件哈希、任务 ID；检查缺字段；建议相似记录；生成待复核列表；提示旧版；制作导出清单。

不能自动决定：记录是否真实、两个来源是否独立、原因是否成立、经验是否通用、谁有技能授权、是否修改 Guide/Wiki、SOP 是否批准、高风险知识是否可发布。

明确禁止：**一次日志出现后自动修改正式 Guide / Wiki。**

## 4. Trust / Verification 模型

### 4.1 最低字段

字段应能写在纸上，也能用 Markdown、JSON、SD 文件或 FT-02 表单表达。

|字段|最低表达|必需性|说明|
|---|---|---|---|
|`record_id`|稳定短编号|必需|纸本可用“本号-页号-序号”。|
|`source_type`|person / device / document / derived|必需|不把设备输出等同于事实。|
|`timestamp`|ISO 时间或明确相对时间|必需|时间不确定时标 `approximate`。|
|`recorder`|姓名、成员代号或设备 ID|必需|匿名信息不能冒充直接观察。|
|`device / source`|设备、文件、纸本页或来源人|必需|用于回到原始证据。|
|`direct_observation`|yes / no / mixed|必需|二手信息必须写来源链。|
|`confidence`|low / medium / high|必需|是记录者信心，不是真实概率。|
|`verification_status`|unreviewed / reviewed / corroborated / verified / contradicted|必需|与生命周期状态一致但不替代它。|
|`related_event`|事件编号|条件必需|事故、near miss、异常必须填。|
|`related_task`|任务编号|条件必需|任务结果或经验必须填。|
|`related_batch`|批次编号|条件必需|生产、种植、储藏、材料记录适用。|
|`location`|地点代号或有效 GNSS|按需|敏感地点必须受访问分类限制。|
|`applicable_conditions`|天气、材料、工具、人员、地点、版本|经验必需|没有适用条件不得晋升 Lesson。|
|`contradictory_record`|冲突记录 ID 列表|按需|冲突时保留双方，不覆盖。|
|`reviewer`|审核者代号|REVIEWED 后必需|高风险领域需具备对应能力。|
|`review_date`|日期/时间|REVIEWED 后必需|旧审核不能无限有效。|
|`access_class`|OPEN / LIMITED / RESTRICTED|必需|控制导出、交接和展示范围。|

纸本最小行可压缩为：`ID | 时地 | 记录者/来源 | 事实 | 已做动作 | 可信度 | 下一复查 | 访问级别`。关联编号和审核栏可写在背面或索引页。

### 4.2 验证规则

- 亲历不等于正确，只说明来源链较短。
- 两人转述同一传言不是两个独立来源。
- 设备时间、GNSS 或传感器异常时保留原值并标记，不手工伪装成准确值。
- 冲突记录保持并列，建立“冲突点、待查证据、复查人、复查日”；不得删除较旧一方来制造一致。
- `confidence=high` 不能跳过 verification。
- 高风险结论必须回到 Medical、Fire、Energy、Chemical、Food Safety 或 Structural Safety 的当前正式知识和 reviewer。

## 5. 跨域记录模板

统一的是索引与生命周期字段，不统一各域的专业内容。

|模板|最小字段|主导边界|规划方式|
|---|---|---|---|
|Daily Summary|今天发生了什么、未完成、异常、风险、明天第一步、原记录 ID|Records 汇总；当前任务细节仍归各域。|新增上层 Wiki + `DG-0911`。|
|Incident / Near Miss|事件、时间、条件、立即动作、可能原因、复工条件、整改、是否重复|当前处置归 Safety / Medical / Production 等域。|新增上层 Wiki + `DG-0913`。|
|Failure Review|任务、失败表现、原因分类、临时处理、是否重复、下一次改变、是否需修订 SOP|具体修复/生产/种植行动仍归业务域。|复用通用失败经验并新增原因与验证 Wiki。|
|Maintenance History|对象、故障、修复、材料、临时/永久、复查、再次故障|怎么修和停用由 Repair 主导。|新增趋势 Wiki；复用 Repair 原记录。|
|Production Batch Review|批次、原料、产出、不合格、失败原因、改进项、关联质检|当批质检和放行由 Production 主导。|新增长期批次回顾 Wiki。|
|Agriculture Season Review|地点、品种、天气/环境、产量、病虫害、成功/失败因素|怎么种与当前处置由 Agriculture 主导。|新增跨季比较 Wiki。|
|Energy Trend|设备、充入、消耗、太阳能产出、可用容量、异常、测量条件|当前调度和电池安全由 Energy 主导。|新增长期趋势 Wiki。|
|Medical Observation Handover|必要事实、时间变化、已采取行动、下一观察点、访问边界|不含诊断、剂量或新的医疗处置。|新增边界 Wiki；复用医疗记录保存。|

## 6. Knowledge Promotion

### 6.1 晋升门槛

|目标|最低条件|
|---|---|
|Lesson Learned|有原始记录、适用条件、成功/失败结果、至少一次复核、可执行的下一次改变。|
|Knowledge Card Candidate|Lesson 已说明受众、使用场景、停止线、风险等级、与现有知识的冲突和原始证据。|
|SOP Candidate|可能改变行动顺序、检查、停止、升级或交接；必须有 reviewer、复现/Field Test 方案和候选版本。|
|CURRENT 正式知识|通过来源复核、冲突检查、风险审核、实际复现或 Field Test；有版本号、生效日、适用范围和 supersedes。|

### 6.2 不得跳过的检查

1. 原始记录仍可找到。
2. 适用条件明确。
3. 结果不是只写“有效”或“失败”。
4. 至少一次独立复核。
5. 已检查与 CURRENT Guide / Wiki 的冲突。
6. 风险等级已确定。
7. reviewer 和审核日期存在。
8. 有实际复现、Field Test 或可靠依据。
9. 分配版本号。
10. 写明生效时间和旧版处理。

Medical、Fire、Energy、Chemical、Food Safety、Structural Safety 的现场经验不得单独晋升为正式行动知识。此类候选至少需要对应域 reviewer、可靠来源、明确停止线和实际验证；无法满足时只保留为待验证记录，不生成“民间 SOP”。

## 7. SOP Version / Supersede / Archive

### 7.1 最低版本字段

|字段|要求|
|---|---|
|`document_id`|跨版本稳定，不用文件名代替身份。|
|`version`|简单递增，如 `0.1`、`0.2`、`1.0`。|
|`status`|只允许 CANDIDATE、CURRENT、SUPERSEDED、ARCHIVED。|
|`effective_date`|CURRENT 必需。|
|`supersedes`|新版本取代的 document/version。|
|`reason_for_change`|触发变更的事件、Field Test、风险或事实。|
|`reviewer`|审核者及必要的业务域角色。|
|`risk_level`|沿用 Guide/Wiki 风险口径。|
|`applicable_scope`|适用设备、材料、地点、人员和条件。|
|`evidence`|原始记录、复盘、测试和来源 ID。|

### 7.2 使用规则

- 只有 CURRENT 默认作为行动依据。
- SUPERSEDED 必须保留历史和取代关系，但从默认行动索引中隔离。
- ARCHIVED 只允许历史查询，不得作为当前步骤或停止线来源。
- 冲突时优先“verified + CURRENT + 适用条件匹配”的版本；若仍冲突，停止高风险行动并请求 reviewer。
- 回滚不是把旧版标签改回 CURRENT；应基于旧内容建立一个新的 CURRENT 版本，记录回滚理由并重新验证。
- 不删除旧文件来掩盖版本历史；纸本旧版应盖明显“旧版/不可默认使用”标记并移出现场操作夹。

## 8. Skills / Training Transfer

### 8.1 最低技能记录

|字段|用途|
|---|---|
|`skill_name`|使用稳定技能名，不用模糊岗位名。|
|`learner` / `trainer`|成员代号；trainer 不等于永久授权者。|
|`current_level`|OBSERVE / ASSIST / SUPERVISED / INDEPENDENT。|
|`allowed_tasks`|当前可执行任务。|
|`prohibited_tasks`|当前明确禁做任务。|
|`supervised_tasks`|必须有谁在场或复核的任务。|
|`last_practice`|最近实际练习或执行日期。|
|`next_review`|复测或复训日期。|
|`evidence`|演示、监督执行、检查结果和相关任务 ID。|
|`notes`|限制条件、设备型号或安全停止线。|

### 8.2 等级边界

|等级|可以做|不能据此推断|
|---|---|---|
|OBSERVE|观看完整流程并复述停止线。|不能操作。|
|ASSIST|在明确指令下做低风险子步骤。|不能独立完成整项任务。|
|SUPERVISED|在授权监督者在场时完成任务。|不能因做过一次就独立。|
|INDEPENDENT|在记录的适用范围内独立执行，并能识别停止线。|不能跨设备、跨风险等级或无限期有效。|

等级晋升必须有实际表现证据和人工确认。长期未练习、设备/流程变化、事故或违反停止线后，应降回 SUPERVISED 或安排复训。Records 只记录事实和授权历史；由 Organization / 对应业务域决定人员安排。

## 9. Wiki 缺口规划

建议把新增系统级条目放入清晰的 Records 分类；slug 只作候选，Batch14-C 创建前仍需按全库 slug 审计。下表共 **45 个规划项：32 个 new，13 个 reuse**。这满足 30-45 篇的规划范围，同时避免复制各 stable domain 的现场模板。

### 9.1 Record Lifecycle / Trust

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|records-lifecycle-state-model-001|记录生命周期状态模型|P0|caution|new|Knowledge Strategy lifecycle；当前无统一状态|DG-0912、DG-0915、DG-0916|分开记录成熟度与正式知识版本。|
|records-raw-record-minimum-fields-001|原始现场记录最小字段|P0|normal|new|`general-event-log-001`、纸质记录本|DG-0911、DG-0912|让纸本、FT-02 和 Core 使用同一最低口径。|
|records-review-verification-checklist-001|记录复核与验证检查表|P0|caution|new|`general-record-trust-label-001`|DG-0912|定义 REVIEWED 和 VERIFIED 的人工门槛。|
|records-source-direct-reported-boundary-001|亲历、转述与设备来源边界|P0|caution|new|`general-record-trust-label-001`|DG-0912|防止二手信息和设备输出冒充事实。|
|records-applicable-conditions-context-001|经验适用条件与上下文字段|P1|normal|new|失败、维修、种植记录|DG-0912、DG-0914、DG-0915|没有条件的经验不能安全复用。|
|records-contradictory-evidence-handling-001|冲突记录保留与复核|P0|caution|new|事件日志、可信度标注|DG-0912、DG-0913|禁止覆盖不一致记录来制造确定性。|
|general-event-log-001|事件日志|P2|normal|reuse|`wiki_import/data`|DG-0297、DG-0911、DG-0913|已覆盖事件编号、事实/推测和更正。|
|general-record-trust-label-001|记录可信度标注|P2|normal|reuse|`wiki_import/data`|DG-0912|已有亲历、转述、推测、待验证标签。|
|education-paper-notebook-001|纸质记录本使用规范|P2|normal|reuse|`wiki_import/data`|DG-0911、DG-0917|提供断电与纸本降级路径。|

### 9.2 Review / Learning / Promotion

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|records-success-review-minimum-001|成功做法复盘最小字段|P1|normal|new|Team review、Production first article|DG-0913、DG-0914|不仅记录失败，也验证可重复有效做法。|
|records-failure-cause-classification-001|失败原因分类与边界|P0|caution|new|`general-failure-lesson-record-001` 和各域失败页|DG-0913|区分信息、材料、工具、环境、疲劳、流程等原因。|
|records-repeated-problem-trend-review-001|重复问题与趋势复查|P0|caution|new|Repair、Agriculture、Production、Energy 历史|DG-0914|把分散事件聚成可验证模式。|
|records-pending-hypothesis-next-test-001|待验证假设与下一次测试|P1|caution|new|失败经验记录|DG-0914|不把推测原因直接晋升为 Lesson。|
|records-corrective-action-verification-001|整改项验证与关闭|P0|caution|new|incident / failure review|DG-0913、DG-0914|确认改进是否真正降低重复失败。|
|records-knowledge-promotion-gate-001|经验晋升正式知识门槛|P0|high|new|Knowledge Strategy Experience / Inbox|DG-0915|禁止一次日志自动进入 Guide/Wiki。|
|general-failure-lesson-record-001|失败经验记录|P2|normal|reuse|`wiki_import/data`|DG-0295、DG-0913|已覆盖事实、原因、损失和下一次改变。|

### 9.3 SOP Version / Archive

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|records-sop-version-minimum-fields-001|SOP 与知识卡版本最小字段|P0|high|new|知识卡归档、Knowledge Strategy metadata|DG-0916|补 document_id、version、status、evidence。|
|records-current-version-selection-001|CURRENT 版本选择规则|P0|high|new|离线目录索引|DG-0916|只有 CURRENT 可默认指导行动。|
|records-superseded-version-isolation-001|SUPERSEDED 旧版隔离|P0|high|new|知识卡归档、长期档案轮检|DG-0916、DG-0917|防止现场继续使用旧 SOP。|
|records-rollback-revalidation-001|回滚与重新验证|P0|high|new|当前无完整能力|DG-0916|回滚必须形成新版本并重新审核。|
|records-high-risk-knowledge-review-gate-001|高风险知识审核门禁|P0|high|new|Knowledge Strategy review rules|DG-0915、DG-0916|Medical、Fire、Energy 等不得靠单次经验发布。|
|records-version-change-log-001|知识版本变更日志|P1|caution|new|决策日志、知识卡归档|DG-0916|记录变更原因、内容、生效、废止和 reviewer。|
|records-retention-access-disposal-001|记录保留、访问与到期处理|P1|caution|new|医疗记录、成员权限、长期档案|DG-0917|提供最小访问分类和保留周期。|
|general-knowledge-card-archive-001|知识卡片归档|P2|normal|reuse|`wiki_import/data`|DG-0915、DG-0916、DG-0917|已有来源、版本日期、废止标记和待审页。|
|general-long-term-archive-check-001|长期档案轮检|P2|normal|reuse|`wiki_import/data`|DG-0917|已有纸本/介质可读性和旧版检查。|
|general-offline-directory-index-001|离线目录索引|P2|normal|reuse|`wiki_import/data`|DG-0916、DG-0917|已有主题、位置、版本和转业务域规则。|
|education-offline-data-backup-001|离线资料备份基础|P2|normal|reuse|`wiki_import/data`|DG-0917|已有多副本、异介质、校验和恢复演练。|
|general-member-info-backup-001|成员信息备份|P2|normal|reuse|`wiki_import/data`|DG-0144、DG-0529、DG-0532、DG-0917|为成员信息包、权限分级和备份校验提供现有 evidence。|

### 9.4 Cross-Domain Review Templates

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|records-daily-cross-domain-summary-001|每日跨域运行摘要|P0|normal|new|事件日志、各域交接卡|DG-0911|只汇总异常、未结项和原记录位置。|
|records-incident-near-miss-review-template-001|事故与 near miss 上层复盘|P0|caution|new|Production、Repair、Team 事件页|DG-0913|统一事件号、复工条件、整改和重复判断。|
|records-maintenance-history-trend-001|维修履历与重复故障趋势|P1|caution|new|Repair logs|DG-0914|聚合长期历史，不写维修步骤。|
|records-production-batch-long-term-review-001|生产批次长期回顾|P1|caution|new|Production batch / quality records|DG-0914|跨批次看一致性和改进，不接管当批质检。|
|records-agriculture-season-comparison-001|农业跨季对比|P1|normal|new|Planting logs / yield / failure review|DG-0914|连接地点、天气、品种、产量和复现条件。|
|records-energy-capacity-trend-001|能源容量与产出趋势|P1|caution|new|Energy logs|DG-0914|只记录长期退化事实，不调度当日电量。|
|records-medical-observation-handover-boundary-001|医疗观察交接与隐私边界|P0|high|new|Medical observation + archive|DG-0145、DG-0912、DG-0917|限制为必要事实、时间变化和访问边界。|
|general-medical-record-archive-001|医疗记录保存|P2|normal|reuse|`wiki_import/data`|DG-0145|已有连续观察、错误更正和隐私交接。|

### 9.5 Knowledge / Skill Transfer

|slug candidate|title|priority|risk|new/reuse|existing source|Guide relation|reason|
|---|---|---|---|---|---|---|---|
|records-skill-authorization-minimum-001|技能授权记录最小字段|P0|caution|new|Member model training_record|DG-0401、DG-0918|回答当前允许、监督和禁止的任务。|
|records-skill-level-boundaries-001|OBSERVE 到 INDEPENDENT 等级边界|P0|caution|new|五分钟教学、技能双人掌握|DG-0401、DG-0918|防止一次演示被当作独立授权。|
|records-supervised-practice-evidence-001|监督练习证据记录|P1|caution|new|五分钟教学|DG-0401、DG-0918|保存实际执行、纠错和停止线表现。|
|records-skill-expiry-refresher-001|技能过期、降级与复训|P1|caution|new|当前无统一规则|DG-0401、DG-0918|长期未用、流程变化或事故后重新确认。|
|records-new-member-learning-path-001|新成员低风险学习路径|P0|caution|new|Team onboarding|DG-0918|从规则、禁区和低风险首任务开始。|
|records-knowledge-transfer-handover-summary-001|知识与技能交接摘要|P1|normal|new|小团队知识交接|DG-0918|交接 CURRENT 材料、技能边界和待复训项。|
|general-small-team-handoff-001|小团队知识交接|P2|normal|reuse|`wiki_import/data`|DG-0148、DG-0296、DG-0918|已有状态、未完成、风险、原记录位置和复述。|
|organization-onboarding-001|新成员加入的信息交接|P2|caution|reuse|`wiki_import/team`|DG-0918|已有禁区、规则、责任人和低风险首任务。|
|organization-skill-redundancy-001|技能双人掌握的意义|P2|normal|reuse|`wiki_import/team`|DG-0918|已有关键技能不依赖单人的组织基础。|

## 10. Guide 候选

当前最高 Guide ID 为 `DG-0910`。以下 ID 只作 Batch14-C 候选，本阶段不创建。

### DG-0911 每日跨域运行摘要

- scenario：一天结束或换日时，需要从各域记录中提取异常、未完成和明日第一步。
- intended scope：只写摘要和原记录 ID，不复制各域完整日志。
- stop_or_escalate：存在当前医疗急症、火灾、电气异常、污染或人员失联时，先进入对应行动 Guide，不等待写摘要。
- fallback：无设备时用纸本五栏“发生、未完、异常、风险、明日第一步”。
- related_wiki candidates：`records-daily-cross-domain-summary-001`、`records-raw-record-minimum-fields-001`、`general-event-log-001`、`education-paper-notebook-001`。
- adjacent-domain boundary：不替代 Production 班次交接、Storage 交接或 Energy 调度。

### DG-0912 原始记录可信度检查

- scenario：一条人写、设备采集、音频或转述记录将被用于判断或复盘。
- intended scope：检查来源、时间、亲历、上下文、置信度、冲突和复核状态。
- stop_or_escalate：来源不明或记录冲突会影响高风险行动时，不得按其行动，转对应域 reviewer。
- fallback：纸上标记“亲历/转述/设备/推测”和“待复核”。
- related_wiki candidates：`records-review-verification-checklist-001`、`records-source-direct-reported-boundary-001`、`records-contradictory-evidence-handling-001`、`general-record-trust-label-001`。
- adjacent-domain boundary：不判断医疗诊断、材料可用性、电池安全或结构安全。

### DG-0913 失败 / 事故记录复盘

- scenario：任务失败、事故或 near miss 已完成现场处置，需要建立事实时间线和整改项。
- intended scope：事实、原因分类、复工条件、整改、复查和关闭。
- stop_or_escalate：人员仍受伤、危险未隔离或现场未稳定时停止复盘，先进入 Medical / Safety / Fire / Energy 等行动流程。
- fallback：情绪高或信息不足时先冻结事实时间线，原因留待后审。
- related_wiki candidates：`records-incident-near-miss-review-template-001`、`records-failure-cause-classification-001`、`records-corrective-action-verification-001`、`general-failure-lesson-record-001`。
- adjacent-domain boundary：不提供事故现场处置，也不替代具体域复工检查。

### DG-0914 重复问题与经验提取

- scenario：同一设备、批次、作物、流程或风险多次出现类似结果。
- intended scope：聚合记录、判断是否真重复、提取条件、假设和下一次验证。
- stop_or_escalate：样本来源相同、条件不同或高风险原因未确认时，只保留假设，不形成经验结论。
- fallback：用纸卡按对象列日期、条件、结果和不同点。
- related_wiki candidates：`records-repeated-problem-trend-review-001`、`records-pending-hypothesis-next-test-001`、`records-maintenance-history-trend-001`、`records-agriculture-season-comparison-001`、`records-energy-capacity-trend-001`。
- adjacent-domain boundary：不决定当前怎么修、怎么种、怎么生产或怎么调度能源。

### DG-0915 经验升级为 SOP 候选

- scenario：一个 Lesson 可能改变正式行动步骤、检查、停止线或知识卡。
- intended scope：执行 promotion gate，收集 evidence、适用条件、冲突、风险和 reviewer。
- stop_or_escalate：高风险领域缺 reviewer、可靠来源或验证时不得晋升；与 CURRENT 冲突时保持 candidate。
- fallback：继续放在 Knowledge Inbox / 待验证清单，不进入默认检索。
- related_wiki candidates：`records-knowledge-promotion-gate-001`、`records-high-risk-knowledge-review-gate-001`、`general-knowledge-card-archive-001`。
- adjacent-domain boundary：Records 管理晋升流程，具体内容是否正确由业务域审核。

### DG-0916 SOP / Knowledge Card 版本更新与废止

- scenario：候选获批、现有 SOP 修订、发现旧版危险或需要回滚。
- intended scope：分配版本、记录生效/取代/理由、隔离旧版和重建 CURRENT 索引。
- stop_or_escalate：无法确定 CURRENT、两个当前版冲突或高风险版本未审核时，停止按争议版本行动。
- fallback：继续使用最后一个已验证 CURRENT；若其本身被判危险则暂停对应高风险任务。
- related_wiki candidates：`records-sop-version-minimum-fields-001`、`records-current-version-selection-001`、`records-superseded-version-isolation-001`、`records-rollback-revalidation-001`、`records-version-change-log-001`。
- adjacent-domain boundary：不修改业务域内容，只管理已审核版本的状态和可见性。

### DG-0917 长期记录归档与索引

- scenario：纸本、SD、Core 和备份介质中的记录需要索引、轮检、导出或归档。
- intended scope：目录、版本、访问级别、校验、备份、复查周期和历史保留。
- stop_or_escalate：唯一副本损坏、校验失败、敏感数据暴露或当前版无法确认时，停止覆盖和扩散。
- fallback：建立纸质当前版清单和最小摘要，保留原介质只读。
- related_wiki candidates：`records-retention-access-disposal-001`、`general-offline-directory-index-001`、`general-long-term-archive-check-001`、`education-offline-data-backup-001`。
- adjacent-domain boundary：不接管 Storage 的物资库存，也不设计云端文档平台。

### DG-0918 新成员知识与技能交接

- scenario：新成员加入、关键成员离开、任务需要替补或技能长期未使用。
- intended scope：学习路径、等级、允许/禁止/监督任务、练习证据、复测和 CURRENT 材料位置。
- stop_or_escalate：接手人无授权、不能复述停止线、技能过期或高风险任务无监督者时，不得独立执行。
- fallback：降级为 OBSERVE / ASSIST，安排低风险任务和监督练习。
- related_wiki candidates：`records-skill-authorization-minimum-001`、`records-skill-level-boundaries-001`、`records-supervised-practice-evidence-001`、`records-skill-expiry-refresher-001`、`records-new-member-learning-path-001`、`general-small-team-handoff-001`。
- adjacent-domain boundary：Organization 决定分工，对应业务域决定技能标准；Records 只保存证据和授权历史。

## 11. Retrieval 风险预测

### 11.1 核心风险

Records 的“记录、日志、交接、复盘、版本、经验”都是跨域通用词。若提前增加宽泛 profile，Records 会吞掉所有当前行动查询；反过来，若完全依赖具体域词，系统级版本和经验晋升查询又会被 Production、Storage、Medical 或 Repair 抢主位。

### 11.2 预期主位与共域边界

|查询|预期主位|Records 合理角色|禁止结果|
|---|---|---|---|
|工坊下一班怎么交接？|Production|无或历史位置补充|Records 抢主 Guide。|
|储藏区交接卡写什么？|Storage|无或归档说明|通用交接覆盖 Storage 卡。|
|病人现在流血怎么办？|Medical|无|返回记录模板而延误急救。|
|病人的观察记录怎么交给下一位照护者？|Medical + Records|格式、时间趋势、访问边界|扩写诊断或处置。|
|电池今天怎么分配？|Energy|无|Records 趋势页主导当日调度。|
|过去三个月电池容量下降多少？|Records + Energy|长期趋势、测量条件、原始日志|把趋势当作电池拆修依据。|
|这个架子怎么修？|Repair / Manufacturing|无|返回维修履历代替动作。|
|这个架子已坏三次，历史怎么看？|Records + Repair|故障聚合、条件和改进验证|Records 给出具体修法。|
|怎么种番茄？|Agriculture|无|跨季回顾抢种植步骤。|
|过去三季番茄为什么都失败？|Records + Agriculture|跨季比较、假设、复现条件|把相关性写成确定原因。|
|这个 SOP 应该用哪个版本？|Records|CURRENT、supersedes、适用范围|SUPERSEDED / ARCHIVED 默认行动。|

### 11.3 Retrieval 实施纪律

- Batch14-C 不新增 query profile，不调整 top_k、selector、ranking、fallback 或 Prompt。
- 先通过 Guide-Wiki evidence 建立语义明确的 Records 入口。
- Batch14-D 用 strict 与 observation 同时测“Records 该出现”和“Records 不该出现”。
- 只有 Batch14-E 证明存在系统性入口根因后，Batch14-F 才考虑最小 retrieval apply。
- 不为单题写关键词硬编码，也不通过扩 selector limit 掩盖跨域主位问题。

## 12. FT-02 / LanternBox 信息接口规划

当前代码和架构已有 Journal、Inventory、Task、Terminal Sync、Audio archive、validated track、SD storage 与 FieldManual；本阶段只定义未来信息关系，不修改任何接口。

### 12.1 采集与整理分工

|能力|FT-02 适合做|Core 适合做|必须人工决定|
|---|---|---|---|
|Journal / field note|短事件、观察、任务结果、风险标记、离线草稿|补全关联、去重提示、review queue、访问分类|事实/推测、原因、可信度和是否验证。|
|Task System|显示当前任务、提交结果/异常、关联设备和地点|保留任务 revision、事件历史，连接复盘|任务结果是否构成 Lesson。|
|Audio Log|保留原始音频、设备/文件 ID、采集时间|校验文件、关联 Journal、供人工回听|音频内容是否真实、说话人、事实结论。|
|Track / GNSS|记录原始点、任务轨迹、设备时间和 fix 状态|验证轨迹、保留 raw archive、生成可读摘要|地点是否可信、漂移原因、路线经验。|
|Inventory|引用物资/批次 ID|连接消耗和任务结果|库存差异原因与跨批次结论。|
|FieldManual|只读 CURRENT Guide / Knowledge Card 子集|构建 current manifest、校验和、版本包|候选批准、旧版废止和高风险发布。|

### 12.2 可自动附加的 metadata

- Core 接收时间和设备本地时间，二者都保留。
- `device_id`、源文件名、audio ID、task ID、同步批次。
- GNSS UTC、坐标、fix / validator 状态，仅在实际存在时附加。
- 文件大小、校验和、导入/归档位置。
- 自动字段只能描述“从哪里来”，不能自动把 `verification_status` 设为 verified。

### 12.3 Audio Log 原则

Audio Log 永远先是 RAW evidence。自动转写若未来存在，也只是 derived record，必须保留音频 ID、转写版本和不确定内容；不得因为语音中有人说“已经确认”就自动认定事实，更不得自动修改 Guide/Wiki。

### 12.4 FieldManual 版本区分

- 现场索引只列 CURRENT。
- manifest 应能表达 `document_id`、`version`、`effective_date`、`risk_level` 和 checksum。
- SUPERSEDED / ARCHIVED 放在单独历史包或历史索引，不进入默认搜索结果。
- 设备离线且版本冲突时，显示“版本未确认”，高风险任务按停止线处理。

### 12.5 完全离线导出、备份和迁移

建议未来使用可读的导出包：`manifest + Markdown/JSONL + attachments/audio + checksums`。每次导出保留源设备、时间范围、记录数量、访问级别和校验结果；至少两种介质，关键 CURRENT 清单另有纸本。导入新 Core 时先只读校验，再建立索引，不覆盖唯一原件或冲突记录。

## 13. 安全与隐私

### 13.1 特有风险与控制

|风险|最低控制|
|---|---|
|错误经验被固化|promotion gate、reviewer、适用条件、复现/Field Test。|
|旧 SOP 继续使用|CURRENT manifest、旧版隔离、纸本废止标记。|
|二手信息冒充亲历|`direct_observation`、来源链、可信度标签。|
|未验证经验进入 Guide|Knowledge Inbox 隔离，不参与默认检索。|
|关键记录丢失|多副本、异介质、校验、恢复抽查、纸本关键摘要。|
|单一介质损坏|不得覆盖唯一副本；损坏介质先只读隔离。|
|时间戳错误|保留设备时间与 Core 接收时间，标 approximate / invalid。|
|重复记录冲突|并存、互链、指定 reviewer 和复查日。|
|医疗记录暴露|RESTRICTED，按照护必要性最小交接。|
|成员技能/行动信息暴露|LIMITED 或 RESTRICTED，不导出完整成员画像。|
|安全/路线记录传播|敏感地点和行动节奏默认 RESTRICTED，导出前最小化。|

### 13.2 最小访问分类

- `OPEN`：低敏感操作历史、公开环境数据、正式 CURRENT 知识。
- `LIMITED`：团队内部事件、技能记录、维修/生产趋势、未完成事项，仅相关成员可见。
- `RESTRICTED`：医疗、身份、精确敏感路线、安全部署、访问凭据，只给必要责任人。

这只是纸本封套、目录标记和导出筛选也能执行的三级分类，不建设复杂权限平台。

### 13.3 高风险审核

- 至少一名对应域 reviewer。
- 原始 evidence 和冲突记录可回查。
- 可靠外部知识或现有 CURRENT 知识对照。
- 明确 stop_or_escalate、fallback 和适用范围。
- 实际复现或 Field Test。
- 任何一项缺失时保持 candidate，不进入 CURRENT。

## 14. Field Test 规划

### 14.1 Batch14-D strict 场景

|场景|预期能力|关键验收|
|---|---|---|
|daily summary|DG-0911|汇总异常/未完成/下一步，并指回原记录。|
|failure review|DG-0913|事实、原因假设、改进和复查不混淆。|
|incident review|DG-0913|现场已安全后复盘，含复工和整改验证。|
|SOP candidate|DG-0915|不会从单次日志直接发布。|
|SOP current version|DG-0916|只选 CURRENT 且检查适用条件。|
|superseded version|DG-0916|旧版不作为默认行动入口。|
|archive|DG-0917|历史可查但不指导当前行动。|
|skill handover|DG-0918|能回答允许、禁止、监督和复测。|
|repeated repair failure|DG-0914 + Repair|Records 聚合历史，Repair 主导修法。|
|repeated agriculture failure|DG-0914 + Agriculture|Records 做跨季比较，Agriculture 主导种植动作。|
|medical observation handover|Records + Medical|只给必要事实、趋势、下一观察点和隐私边界。|
|production long-term trend|DG-0914 + Production|跨批次趋势，不抢当前班次/WIP。|
|energy capacity trend|DG-0914 + Energy|容量历史与条件，不抢当日电量调度。|

### 14.2 Observation 场景

|场景|必须主导|Records 允许角色|
|---|---|---|
|当前医疗急救|Medical|不应主导。|
|当前制造操作|Manufacturing|不应主导。|
|当前维修|Repair|不应主导。|
|当前 Storage 交接|Storage|只可补长期归档，不抢主位。|
|当前 Production 班次交接|Production|只可补历史索引，不抢主位。|
|当前 Energy 调度|Energy|不应主导。|

未来结果需至少统计 strict pass/partial/fail、Records primary rate、expected Guide hit、Wiki any/full hit、Guide-Wiki precise、危险建议、Kiwix 越权、CURRENT/SUPERSEDED 版本正确率，以及六个 observation 的主域保持率。

## 15. Batch14-C Apply 建议

### 15.1 最终范围

|项目|建议|
|---|---|
|新增 Wiki|32 篇。|
|复用现有 Wiki|13 篇。|
|新增 Guide|8 个，`DG-0911` 至 `DG-0918`。|
|补 evidence 的旧 Records Guide|`DG-0144`、`DG-0401`、`DG-0529`。|
|不碰的旧无链 Guide|`DG-0123`、`DG-0125`、`DG-0126`、`DG-0128`、`DG-0164`、`DG-0327`、`DG-0391`、`DG-0395`、`DG-0396`、`DG-0437`、`DG-0513`、`DG-0515`。|
|query profile|Batch14-C 不新增。|
|Retrieval|Batch14-D Field Test 前保持完全不动。|

### 15.2 Apply 顺序

1. 对 45 个规划 Wiki 项做最终 slug、标题和语义去重。
2. 只创建 32 个 new Wiki，13 个 reuse Wiki 不复制文件。
3. 创建 8 个系统级 Guide，保持行动短、停止线明确。
4. 建立新 Guide 与 new/reuse Wiki 的双向精准 evidence。
5. 只为 3 个语义匹配的旧 Guide 补 evidence。
6. 运行 Wiki / Guide / 双向关系审计，并确认没有无效 slug / ID。
7. 不同步或调整 Retrieval；将实际 Apply 清单作为 Batch14-D fixture 设计依据。

优先原则保持：

```text
Batch14-C: Knowledge + evidence first
Batch14-D: Field Test
Batch14-E: Root Cause Review
Batch14-F: Minimal Retrieval Apply, only if proven necessary
```

## 16. 不建议投入方向

Long-Term Records / Knowledge Transfer v0.1 明确不做：

- 企业级文档管理、ERP、CRM 或云端知识库。
- 多租户权限平台、复杂 PKI 或区块链记录。
- 自动修改正式 Guide / Wiki、自动批准 SOP 或自动判断真实性。
- 大型 HR 培训系统、全自动绩效评估或成员排名。
- 大规模监控、行为画像或社会信用体系。
- 把 Records 变成所有日志、交接、复盘查询的默认主域。
- 修改 FastAPI、PocketBase schema、UI、sync protocol 或 FT-02 firmware。

## 17. 结论

Long-Term Records / Knowledge Transfer v0.1 的建设价值不在“再写一批日志”，而在建立三个可靠接口：

1. **事实接口**：知道记录从哪里来、是否复核、是否冲突。
2. **知识接口**：知道经验何时只能待验证，何时可以成为 CURRENT 正式知识。
3. **人员接口**：知道新成员当前能安全独立做什么，何时必须监督或复训。

Batch14-C 应按本报告执行 Knowledge + evidence first，并保持 Retrieval 完全不动。只有未来 Field Test 证明系统级 Records 入口确实无法稳定检索时，才进入独立 Root Cause Review 和最小修复。
