# LanternBox Batch9-E：Manufacturing Retrieval Root Cause Review

日期：2026-07-17

阶段性质：只生成分析报告。  
遵守：`docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

本阶段未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

参考：

- `docs/knowledge/batch9_b_manufacturing_production_plan.md`
- `docs/knowledge/batch9_c_manufacturing_production_apply_report.md`
- `docs/knowledge/batch9_d_manufacturing_field_test_report.md`
- `docs/knowledge/batch9_d_manufacturing_field_test_results.json`

## 1. Field Test 总结

Batch9-D 当前结果：

|指标|结果|
|---|---:|
|total|18|
|strict / observation|12 / 6|
|pass / partial / fail|11 / 2 / 5|
|strict pass / partial / fail|5 / 2 / 5|
|Guide 命中率 strict，含 allowed secondary|91.7%|
|主 Guide 命中率 strict，仅 expected|83.3%|
|Wiki 全量命中率 strict|50.0%|
|Wiki 任一命中率 strict|91.7%|
|Guide-Wiki 精准组合率 strict|75.0%|
|Manufacturing 主 Guide top evidence 率|50.0%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|

总体判断：

|问题|判断|
|---|---|
|fail 是否来自知识缺失|大多数不是。38 篇 Wiki 和 8 个 Guide 已存在，并且多数 fail 中 manufacturing Guide/Wiki 已进入 selected evidence。|
|fail 是否来自 Guide-Wiki 缺链|不是主因。Guide audit 已验证双向关系，且多数 expected Wiki 是相关 Guide 的 `related_wiki`。|
|fail 是否来自 manufacturing profile 缺失|是主要原因之一。当前没有 manufacturing 专属 profile，旧 repair/tools profile 对加工词强匹配。|
|fail 是否来自 repair / tools 旧入口抢主位|是主要原因。刀锯清场、低光维修、木板再利用等旧入口常在 top1。|
|fail 是否来自 selected Wiki evidence 被截断|是部分原因。related_wiki 与 independent wiki 合并后只取前 8，核心 Wiki 如承重检查、质量复查、木材锯切支撑被挤出。|
|是否存在 dangerous suggestion 或 Kiwix 越权|否。dangerous suggestion=0，Kiwix 越权=0。|

不要把本轮问题归因为安全字段缺失。所有 cases 的 safety boundary、fallback、record/check 均为 100%。核心问题是 selected Guide / selected Wiki 的 evidence priority 与 manufacturing 入口稳定性。

## 2. 5 个 Fail 根因

说明：Batch9-D 实际 5 个 fail 为 `scrap_wood_shelf`、`wood_cut_clamp`、`low_light_cutting`、`dust_no_mask`、`sparks_near_fabric`。用户点名的“重复制作同尺寸木块”在当前 fixture 中是 observation pass，但有 off-domain primary 信号，单独在后文记录。

|case|query|expected|actual selected|root cause|最小修复建议|
|---|---|---|---|---|---|
|`manufacturing_scrap_wood_shelf_judgement`|废木板能不能拿来做架子，怎么判断？|Guide DG-0872；Wiki `wood-selection`、`scrap-material-check`、`load-test`|Guide：DG-0830、DG-0304、DG-0872；Wiki 命中 wood/scrap，但未命中 `load-test`|A manufacturing profile 缺失；E repair/tools 抢主位；C correct Wiki exists but not selected；D DG-0872 不带承重检查或顺序不足|新增 `manufacturing_material_reuse` profile；DG-0872 对架子/承重场景补强 `repair-manufacturing-load-test-001` 的 evidence priority，或让 DG-0877 更易作为 secondary|
|`manufacturing_wood_cut_clamp_no_foot`|切木板前怎么固定，能不能直接用脚踩着锯？|Guide DG-0873；Wiki `clamp-before-processing`、`wood-saw-support`|Guide：DG-0839、DG-0304、DG-0873；Wiki 命中 clamp，但未命中 wood-saw-support|B correct Guide exists but not selected top1；E repair/tools 抢主位；C correct Wiki exists but not selected；D DG-0873 顺序中 `wood-saw-support` 排第 5|新增 `manufacturing_cutting_clamping` profile；DG-0873 related_wiki 调整为 clamp / wood-saw-support / cut-clear-zone 优先|
|`manufacturing_low_light_continue_cutting`|低光环境下还要继续切割材料，可以吗？|Guide DG-0871；Wiki `workbench-start-check`、`dust-spark-stop-line`|Guide：DG-0836、DG-0839、DG-0871；Wiki 命中 workbench，但未命中 dust/spark|B correct Guide exists but not selected top1；E repair/tools 抢主位；C correct Wiki exists but not selected；fixture expected 可略窄，因为旧低光 Guide 合理 secondary|新增 `manufacturing_workspace_safety` profile；DG-0871 中低光/切割类场景优先 workbench + cut-clear-zone + dust-spark；保留 DG-0836 secondary|
|`manufacturing_dust_no_mask_continue_sanding`|粉尘很多但没有专业口罩，还能继续磨吗？|Guide DG-0871；Wiki `dust-spark-stop-line`、`ppe-minimum`|Guide：DG-0094、DG-0322、DG-0244；Wiki：wildlife risk index|A manufacturing profile 缺失；F medical/safety 抢主位；H 表面看像 PPE/口罩，但 manufacturing sanding/dust 入口完全未召回|必须新增 `manufacturing_workspace_safety`，覆盖粉尘/打磨/口罩不足/继续磨；无需新增 Wiki，现有 `dust-spark` 和 `ppe-minimum` 足够|
|`manufacturing_sparks_near_fabric`|火花飞到旁边布料上，工作区要怎么处理？|Guide DG-0871；Wiki `cut-drill-clear-zone`、`dust-spark-stop-line`|Guide：DG-0839、DG-0871、DG-0397；Wiki 两个 expected 均进入|B correct Guide exists but not top1；E tools 抢主位；C/D 不是问题，Wiki evidence 已足够；fire 未真正抢主位|新增 `manufacturing_workspace_safety` profile，覆盖火花/布料/工作区；不新增 Wiki；不需让 Fire 主导|

根因归类：

- A. manufacturing profile 缺失：废木板、切割固定、低光、粉尘、火花均受影响。
- B. correct Guide exists but not selected：DG-0871、DG-0872、DG-0873 多次进入 top3 但不是 top1。
- C. correct Wiki exists but not selected：承重检查、木材重复锯切、粉尘/PPE 在部分 case 被挤出。
- D. Guide-Wiki related_wiki 顺序问题：DG-0871、DG-0872、DG-0873、DG-0877 需要最小顺序调整。
- E. repair / tools 抢主位：最主要跨域来源。
- F. shelter / recycling / medical / safety 抢主位：粉尘 case medical/PPE 侧明显；shelter/recycling 更多出现在 observation。
- G. fixture expected 过窄：低光 case 应允许 DG-0836 secondary；废木板做架子要求 `load-test` 合理但需承认它依赖 DG-0877 secondary。
- H. 确实缺 Wiki / Guide：strict fail 中暂无必须新增 Wiki/Guide 的证据。
- I. 合理 observation，不修：重复制作同尺寸木块、雨棚支撑杆、废塑料桶工具盒等先观察。

## 3. 2 个 Partial 分类

|case|问题类型|actual|expected|root cause|建议|
|---|---|---|---|---|---|
|`manufacturing_finished_shelf_heavy_load`|正确 Guide 未稳定；Wiki evidence 不足|Guide：DG-0874、DG-0865、DG-0556；Wiki 命中 `shelf-support`，未命中 `load-test`、`quality-recheck`|DG-0877 或 DG-0874；`load-test`、`quality-recheck`、`shelf-support`|B correct Guide exists but not selected；C correct Wiki exists but not selected；D DG-0877 未进入 selected，导致其 related_wiki 未加载|新增 `manufacturing_load_check` profile；DG-0877 priority 顺序保持 `load-test` 第一，并让“放重物/承重/直接放”触发 DG-0877|
|`manufacturing_drilling_material_sliding`|正确 Guide 已进入，但 Wiki evidence 偏金属，木材打孔 Wiki 不足|Guide：DG-0876、DG-0873、DG-0872；Wiki 命中 `clamp`，未命中 `wood-drill-position`|DG-0873；`clamp-before-processing`、`wood-drill-position`|B/D mixed：打孔 query 被 DG-0876 金属处理 top1；DG-0873 进入但 related_wiki 被 DG-0876 的金属 Wiki 挤占|新增 `manufacturing_cutting_clamping` profile；DG-0873 中 clamp + wood-drill-position 前置；DG-0876 只在金属对象明确时主导|

不建议把这两个 partial 视为知识缺失。两个主题均有现有 Wiki 和 Guide，问题是入口和顺序。

## 4. 新增 8 个 Manufacturing Guide 稳定性审查

|Guide|title|selected evidence|top evidence|related_wiki 表现|稳定性|问题|建议|
|---|---|---:|---:|---|---|---|
|DG-0871|工坊开工前安全检查|4 cases|1 case|工作台、分区、切割清场、粉尘火花可进入；PPE 只在开工 case 进入|中|低光、粉尘、火花场景常被旧 tools/PPE 抢 top1|需要 `manufacturing_workspace_safety`；related_wiki 可按 workbench / dust-spark / PPE / cut-clear-zone 重排|
|DG-0872|材料再利用前安全判断|3 cases|0 case|木材、废旧材料、金属、塑料、软材料进入；承重检查不在本 Guide|弱|废木板被草木灰/旧木板再利用抢 top1；没有 top1|需要 `manufacturing_material_reuse`；可考虑把 `scrap-material-check` 放到 wood-selection 前，减少材料类型泛化|
|DG-0873|木材切割前固定与支撑|4 cases|1 case|clamp、wood-selection、wood-measure、wood-saw、wood-drill 均能进入不同 case|中|脚踩锯切被 DG-0839/DG-0304 抢；打孔滑动被 DG-0876 抢|需要 `manufacturing_cutting_clamping`；前置 clamp、wood-saw-support、wood-drill-position|
|DG-0874|简易结构件制作检查|3 cases|2 cases|结构件 Wiki 进入稳定；但承重 query 会把 DG-0877 挤掉|较稳|容易吞掉 DG-0877 的“成品承重”场景|需要与 `manufacturing_load_check` 明确边界：制作中 DG-0874，制作后承重 DG-0877|
|DG-0875|连接方式选择：胶粘/螺丝/捆扎|2 cases|2 cases|连接方式 Wiki 全量命中|稳|旧 DG-0835/DG-0376 作为 secondary 合理|可暂不调整；若建 profile，`manufacturing_connection_choice` 可低风险加入|
|DG-0876|废旧金属材料安全处理|2 cases|2 cases|金属相关 Wiki 进入稳定|稳，但略泛化|“打孔材料滑动”未明确金属也由 DG-0876 top1|profile 应要求金属对象触发 DG-0876；一般木材打孔交给 DG-0873|
|DG-0877|制作完成后的承重检查|1 observation|0 strict/top|`load-test`、`quality-recheck` 未进入 strict selected|弱|承重/放重物 query 被 DG-0874 和外域 Guide 挤占|需要 `manufacturing_load_check`；必要时在 DG-0874 related_wiki 中补/前置 `load-test` 作为 secondary|
|DG-0878|工坊收工与工具封存|1 case|1 case|收工清点、分区、儿童隔离、批量记录进入稳定|稳|无明显问题|暂不调整；可在后续 profile `manufacturing_workshop_closure` 中覆盖|

## 5. 新增 Manufacturing Wiki Evidence 审查

|Wiki|关联 Guide|是否进入 evidence|未进入原因|建议|
|---|---|---:|---|---|
|`repair-manufacturing-workbench-start-check-001`|DG-0871|3|开工、低光、火花均可进入|保留核心，DG-0871 前排|
|`repair-manufacturing-zone-layout-001`|DG-0871, DG-0878|4|稳定进入|保留|
|`repair-manufacturing-cut-drill-clear-zone-001`|DG-0871, DG-0873, DG-0876|8|进入很多 case，略泛化但有用|保留，可作为 workspace safety 核心|
|`repair-manufacturing-clamp-before-processing-001`|DG-0873, DG-0876|5|稳定进入|保留，DG-0873 前排|
|`repair-manufacturing-dust-spark-stop-line-001`|DG-0871|2|火花进入；粉尘 case 完全未召回|需要 profile 触发，不是内容缺失|
|`repair-manufacturing-bystander-exclusion-001`|DG-0871, DG-0878|3|稳定进入儿童/火花/开工|保留|
|`repair-manufacturing-ppe-minimum-001`|DG-0871|1|粉尘无口罩未召回|需要 workspace safety profile；可在 DG-0871 顺序前移|
|`repair-manufacturing-damaged-tool-stop-001`|DG-0871, DG-0878|2|开工/收工进入|保留|
|`repair-manufacturing-wood-selection-001`|DG-0872, DG-0873, DG-0874|7|稳定进入木材场景|保留|
|`repair-manufacturing-scrap-material-check-001`|DG-0872|1|废木板进入；其他 observation 未明显召回|需要 material_reuse profile|
|`repair-manufacturing-metal-scrap-check-001`|DG-0872, DG-0876|3|金属场景稳定|保留|
|`repair-manufacturing-plastic-reuse-boundary-001`|DG-0872|1|废塑料桶 observation 未进入，反而导航异常 evidence|需要 material_reuse profile；不新增 Wiki|
|`repair-manufacturing-fabric-rope-leather-check-001`|DG-0872, DG-0875|3|连接/软材料可进入|保留|
|`repair-manufacturing-connection-choice-001`|DG-0875|2|连接 case 稳定|保留|
|`repair-manufacturing-fastener-reuse-boundary-001`|DG-0872, DG-0875|2|连接替代稳定|保留|
|`repair-manufacturing-salvaged-parts-sort-001`|DG-0872, DG-0878|1|仅收工进入|可保留后排|
|`repair-manufacturing-material-prep-001`|DG-0872, DG-0876|2|金属/打孔进入|保留|
|`repair-manufacturing-downgrade-label-001`|DG-0872|0|没有专门问降级标签|不修；future observation|
|`repair-manufacturing-load-test-001`|DG-0877|0|DG-0877 未被 strict selected，related_wiki 未加载|需要 load_check profile；可在 DG-0874 secondary 关联中补强|
|`repair-manufacturing-wood-measure-mark-001`|DG-0873|2|雨棚支撑杆/重复木块进入|保留|
|`repair-manufacturing-wood-saw-support-001`|DG-0873|2|重复/雨棚进入；脚踩锯未进入|DG-0873 顺序前移|
|`repair-manufacturing-wood-drill-position-001`|DG-0873, DG-0874|5|结构/重复进入；打孔滑动未进入|DG-0873 顺序前移|
|`repair-manufacturing-wood-screw-join-001`|DG-0874, DG-0875|5|结构/连接进入稳定|保留|
|`repair-manufacturing-frame-square-check-001`|DG-0874|3|结构进入稳定|保留|
|`repair-manufacturing-shelf-support-check-001`|DG-0874, DG-0877|2|架子/承重进入|保留|
|`repair-manufacturing-box-crate-check-001`|DG-0874|2|箱体进入|保留|
|`repair-manufacturing-thin-metal-drill-001`|DG-0876|2|金属/打孔进入|保留|
|`repair-manufacturing-thin-metal-bend-001`|DG-0876|2|金属进入|保留|
|`repair-manufacturing-metal-edge-safe-001`|DG-0876|2|锐边进入稳定|保留|
|`repair-manufacturing-metal-bracket-check-001`|DG-0876, DG-0877|2|金属支架进入|保留|
|`repair-manufacturing-metal-join-choice-001`|DG-0875, DG-0876|2|连接进入|保留|
|`repair-manufacturing-corroded-metal-reuse-001`|DG-0872, DG-0876|0|无腐蚀专门 case|不修；future observation|
|`repair-manufacturing-raw-finished-zones-001`|DG-0871, DG-0878|2|开工/收工进入|保留|
|`repair-manufacturing-start-record-001`|DG-0871|0|无每日开工记录专门 case|不修；可在 workspace profile 后自然观察|
|`repair-manufacturing-end-clean-count-001`|DG-0878|1|收工进入|保留|
|`repair-manufacturing-batch-record-001`|DG-0878|1|收工进入；重复制作 query 未主导|需 `manufacturing_template_repeat_production` 判断是否实现|
|`repair-manufacturing-quality-recheck-001`|DG-0877|0|DG-0877 未进入 strict selected|需要 load_check profile；不新增 Wiki|
|`repair-manufacturing-defect-isolation-001`|DG-0877, DG-0878|1|收工进入|保留|

重点结论：

- 工作台安全、分区、切割清场、夹持、木材选择、连接方式、金属锐边、收工清点已有 evidence。
- 承重检查、质量复查、粉尘/PPE、降级标签、腐蚀金属、开工记录的 evidence 入口不稳定。
- 重复制作同尺寸木块已有 `wood-measure`、`wood-saw-support`、`batch-record`，但缺少专门 P2 模板 Wiki；当前不建议为 observation 立即新增。

## 6. Cross Domain 根因

### Repair / Tools

表现：

- `DG-0839 刀锯作业前清场` 抢切割固定和火花场景 top1。
- `DG-0836 低光维修停止线` 抢低光切割 top1。
- `DG-0304 木板和木棍再利用` 抢废木板做架子 top2。
- `DG-0834 锯切前固定与支撑` 抢重复同尺寸木块 observation top1。

边界规则：

- Manufacturing 应主导：从材料制作新物品、多件重复制作、工坊流程、连接方式选择、成品承重检查、材料再利用前判断。
- Repair / Tools 应主导：修已有物品、单个工具损坏、临时替代修补、已有结构失效。
- 协同：切割安全、低光停工、工具损坏可作为 secondary，但当 query 的目标是“做一个/制作/成品/材料再利用/承重前”，manufacturing 应进入 top evidence。

### Shelter

表现：

- 雨棚支撑杆 observation 中 DG-0873 top1，shelter 未抢主位，表现可接受。

边界规则：

- Shelter 主导：居住空间、防雨、防潮、保温、通风、雨棚本身是否适合居住。
- Manufacturing 主导：支撑杆、盖板、架子、箱体等部件如何制作、选材、连接和承重检查。

### Recycling / Waste

表现：

- 废木板、废塑料桶暴露材料池入口不稳定。废塑料桶 observation 中出现 DG-0301、DG-0865、DG-0624，manufacturing material reuse 未进入。

边界规则：

- Recycling / Waste 主导：废弃物分类、污染处理、是否可进入材料池。
- Manufacturing 主导：材料已经准备进入制作流程后，判断用途、降级、连接、成品质量。

### Medical / Safety

表现：

- 粉尘无专业口罩被 DG-0094、DG-0322、DG-0244 带偏，Manufacturing 完全未进入。
- 金属锐边 case 表现良好，DG-0876 top1。

边界规则：

- Medical / Safety 主导：已经出现伤害、吸入症状、烧伤、割伤、眼部刺激。
- Manufacturing 主导：尚未受伤时的粉尘控制、PPE 最低线、锐边处理、火花清场、低光停工。

### Fire

表现：

- 火花飞到布料 case 未被 Fire 抢主位，而是 tools 抢主位。DG-0871 进入 top2，两个 expected Wiki 均进入。

边界规则：

- Fire 主导：已有燃烧、冒烟、余火、室内燃烧、一氧化碳。
- Manufacturing 主导：加工产生火花前后的工作区清理、可燃物移除、停止加工。

## 7. 是否需要 Profile

现有 `data/retrieval_query_profiles.json` 有 repair/tools、medical、navigation、communication 等 profile，没有 manufacturing 专属 profile。建议 Batch9-F 做最小 profile apply。

|profile|是否必要|triggers|可能误伤|Batch9-F 是否实现|
|---|---|---|---|---|
|`manufacturing_workspace_safety`|必要|工坊、工作台、开工、低光、切割、粉尘、火花、打磨、PPE、旁人、儿童、继续加工|可能压过 tools/fire/medical；需限定为“加工前/工作区/能否继续制作”|是，最高优先|
|`manufacturing_material_reuse`|必要|废木板、废塑料桶、废金属片、旧材料、再利用、做架子、做工具盒、材料判断、降级|可能压过 recycling/waste；需区分“进入材料池”与“制作新物”|是|
|`manufacturing_cutting_clamping`|必要|切木板、锯、打孔、固定、夹持、脚踩、手扶、材料滑动、尺寸切割|可能压过 tools；但 tools 可 secondary|是|
|`manufacturing_connection_choice`|可选|胶水、螺丝、绳子、扎带、铁丝、铆接、替代连接、什么时候用|当前表现稳，旧 repair secondary 合理|可暂缓或低风险加入|
|`manufacturing_load_check`|必要|做好的架子、放重物、承重前、成品检查、摇晃、裂声、质量复查|可能压过 shelter/repair；需限定“新制成品/自制/制作完成后”|是|
|`manufacturing_workshop_closure`|可选|收工、清点、半成品、危险工具、孩子碰到、封存|当前表现稳|可暂缓|
|`manufacturing_template_repeat_production`|可选观察|一样大小、重复制作、模板、样件、批量、小木块、尺寸一致|当前 observation 有 manufacturing Wiki 进入但 top1 是 DG-0834|可暂缓；若 Batch9-F 想覆盖 observation，可加入但非必要|

建议：Batch9-F 先实现 4 个 profile：`manufacturing_workspace_safety`、`manufacturing_material_reuse`、`manufacturing_cutting_clamping`、`manufacturing_load_check`。`connection_choice`、`workshop_closure`、`template_repeat_production` 暂缓或仅在 contract test 不足时加。

## 8. 是否需要 Guide-Wiki 顺序调整

需要少量调整，不新增内容。

|Guide|related_wiki 是否过宽|核心 Wiki 是否排第一|repair/tools 是否挤占|建议|
|---|---|---|---|---|
|DG-0871|略宽，但可接受|工作台第一；粉尘/PPE 对 dust case 不够靠前|外部 repair/tools 抢，不是本 Guide 内部 repair Wiki|将 `dust-spark-stop-line`、`ppe-minimum` 提到 cut-clear-zone 后，保留 workbench 第一|
|DG-0872|较宽，材料类型多|wood-selection 第一，但废旧材料总检查更适合作主入口|旧 DG-0304/DG-0830 抢|将 `scrap-material-check` 第一，wood/metal/plastic 随后；可把 `load-test` 作为架子场景 secondary，或不动 Guide 仅靠 profile|
|DG-0873|不宽|cut-clear-zone 第一，clamp 第二；脚踩锯 query 更需要 clamp/wood-saw|DG-0839/DG-0304 抢|调整为 clamp -> wood-saw-support -> wood-drill-position -> cut-clear-zone -> wood-selection|
|DG-0874|合理|结构件顺序合理|承重 query 抢 DG-0877，不是 DG-0874 内部问题|可不动；或把 `shelf-support` 前移到 frame 前|
|DG-0875|合理|connection-choice 第二，软材料第一略偏|旧胶水/螺丝 Guide 作为 secondary 合理|将 `connection-choice` 第一，fastener 第二，wood/metal join 随后|
|DG-0876|较宽|metal-scrap 第三；前两项是通用清场/夹持|对一般打孔 query 过强，抢 DG-0873|将 `metal-scrap-check`、`metal-edge-safe`、`thin-metal-drill` 前置；通用 clear/clamp 后移|
|DG-0877|合理但未召回|load-test 第一，quality 第四|DG-0874/外域抢|保持 load-test 第一；将 quality-recheck 第二；可 contract/profile 强化|
|DG-0878|合理|收工 Wiki 第六，前面是分区/儿童/工具|当前通过|可把 `end-clean-count` 提到第一，zone/bystander/damaged-tool 随后|

结论：Batch9-F 应做 profile + Guide-Wiki 顺序最小调整。只调整相关 6-7 个 Guide 的 related_wiki 顺序，不新增关联、不删除旧关联。

## 9. 是否需要新增 Wiki / Guide

默认不新增。本轮 strict fail/partial 没有证明缺 Wiki/Guide。

|主题|现有支撑|是否新增|理由|
|---|---|---|---|
|重复制作同尺寸木块|`wood-measure-mark`、`wood-saw-support`、`batch-record`|暂不新增|observation pass，虽 top1 是 DG-0834，但 manufacturing evidence 已进入。模板/治具 Wiki 可留到 v0.2|
|粉尘控制|`dust-spark-stop-line`、`ppe-minimum`|不新增|内容存在，未召回是 profile/入口问题|
|火花风险|`cut-drill-clear-zone`、`dust-spark-stop-line`|不新增|expected Wiki 已进入，问题是 top Guide|
|低光停止线|`workbench-start-check` 中有低光；旧 `repair-low-light-stop-line` 合理 secondary|不新增|可通过 workspace profile 和 Guide-Wiki 顺序解决|
|材料再利用|`scrap-material-check`、wood/metal/plastic/fabric/fastener Wiki|不新增|内容足够，profile 缺失和旧入口抢主位|
|成品承重检查|`load-test`、`quality-recheck`、`shelf-support`|不新增|DG-0877 未稳定进入，需 load_check profile|

不建议新增 Guide。DG-0871 至 DG-0878 已覆盖本批行动入口。

## 10. Batch9-F 最小 Apply 建议

建议选择 C：profile + Guide-Wiki 顺序都需要。

### A. 新增 manufacturing query profile

建议新增 4 个：

1. `manufacturing_workspace_safety`
   - 覆盖：工坊、工作台、开工、低光、继续切割、粉尘、打磨、火花、布料旁、PPE 不足、儿童旁人。
   - 优先 Guide：DG-0871。
   - secondary：DG-0878、DG-0836、DG-0839、fire safety。

2. `manufacturing_material_reuse`
   - 覆盖：废木板、废塑料桶、废金属片、旧材料、再利用、做架子、做工具盒、材料判断、降级用途。
   - 优先 Guide：DG-0872。
   - secondary：DG-0874、DG-0877、recycling/waste、repair。

3. `manufacturing_cutting_clamping`
   - 覆盖：切木板、锯、打孔、固定、夹持、脚踩、手扶、材料滑动。
   - 优先 Guide：DG-0873。
   - secondary：DG-0876、DG-0834、DG-0839。

4. `manufacturing_load_check`
   - 覆盖：做好的架子、放重物、承重前、成品复查、质量检查、摇晃、裂声、直接放。
   - 优先 Guide：DG-0877。
   - secondary：DG-0874、shelter/repair。

可暂缓：

- `manufacturing_connection_choice`：当前 strict pass。
- `manufacturing_workshop_closure`：当前 strict pass。
- `manufacturing_template_repeat_production`：observation 先记录。

### B. Guide-Wiki 顺序最小调整

建议只调整顺序：

- DG-0871：`workbench-start`、`cut-drill-clear`、`dust-spark-stop`、`ppe-minimum`、`zone-layout`、`bystander`、`damaged-tool`、`raw-finished`、`start-record`。
- DG-0872：`scrap-material-check`、`wood-selection`、`plastic-reuse`、`metal-scrap`、`fabric-rope-leather`、`material-prep`、`downgrade-label`、其余后置。
- DG-0873：`clamp-before-processing`、`wood-saw-support`、`wood-drill-position`、`wood-measure-mark`、`cut-drill-clear`、`wood-selection`。
- DG-0875：`connection-choice` 第一。
- DG-0876：`metal-scrap-check`、`metal-edge-safe`、`thin-metal-drill`、`material-prep` 前置，通用 clear/clamp 后移。
- DG-0877：`load-test`、`quality-recheck`、`shelf-support`、`metal-bracket`、`defect-isolation`。
- DG-0878：`end-clean-count` 第一，`zone-layout`、`bystander`、`damaged-tool` 随后。

### C. Contract tests

Batch9-F 应新增最小 contract tests，覆盖：

1. “粉尘很多但没有专业口罩，还能继续磨吗？” -> DG-0871。
2. “废木板能不能拿来做架子，怎么判断？” -> DG-0872，DG-0877 secondary。
3. “切木板前怎么固定，能不能用脚踩？” -> DG-0873。
4. “做好的架子能不能直接放重物？” -> DG-0877。
5. “火花飞到旁边布料上，工作区怎么处理？” -> DG-0871。

### D. Fixture expected

可做少量修正：

- 低光继续切割：允许 DG-0836 作为 secondary 保留，当前 fixture 已允许；不应要求旧低光完全消失。
- 废木板做架子：`load-test` 作为 expected 合理，但若 DG-0877 未 selected，应作为 root cause 而非 fixture 错误。
- 重复制作同尺寸木块：仍作为 observation，不进入 strict 修复目标。

### E. 暂不修复 observation

暂不针对以下 observation 扩 profile：

- 修坏架子 vs 重新做架子。
- 废塑料桶改工具盒。
- 雨棚支撑杆。
- 绳子和木棍做临时挂架。
- 重复小木块尺寸一致。
- 没有螺丝的替代连接。

它们用于观察边界，不应成为 Batch9-F 的主要修复驱动。

### F. 不新增 Wiki / Guide

不建议 Batch9-F 新增 Wiki 或 Guide。现有内容足够支撑最小修复。

## 11. 不建议修改项

Batch9-F 不建议：

- 不新增大量 Wiki。
- 不新增 Guide。
- 不扩 top_k。
- 不改 Prompt。
- 不改 selector limit。
- 不改 Retrieval Pipeline。
- 不硬编码 field test case。
- 不让 repair 完全压过 manufacturing。
- 不让 manufacturing 吞掉 repair/tools。
- 不修改 schema。
- 不同步 PocketBase，除非 Batch9-F 实际调整 Wiki/Guide 数据需要按流程同步。

## 12. 结论

Manufacturing v0.1 的知识层已经存在，安全边界也稳定。当前问题不是“没有知识”，而是 manufacturing action entrance 在检索层没有独立 profile，导致旧 repair/tools 入口在切割、低光、旧木板、材料滑动等 query 中抢 top1；同时少数 Guide 的 `related_wiki` 顺序让核心 Wiki 在 top8 evidence 中被挤出。

建议进入 Batch9-F：Manufacturing Retrieval Minimal Apply。最小范围为：

1. 新增 4 个 manufacturing query profile。
2. 只调整 DG-0871、DG-0872、DG-0873、DG-0875、DG-0876、DG-0877、DG-0878 的 `related_wiki` 顺序。
3. 新增最小 contract tests。
4. 重跑 Manufacturing Field Test。

不建议新增 Wiki/Guide，也不建议修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。
