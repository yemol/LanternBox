# Batch9-D Manufacturing & Production Retrieval Field Test Report

生成时间：2026-07-17T16:17:46.766612+00:00

## 1. 测试范围

本阶段只测试 Batch9-C 新增 Manufacturing & Production Foundation Guide/Wiki 是否稳定进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。

覆盖：工坊开工检查、材料再利用、木材固定切割、简易结构件、连接方式、废旧金属处理、成品承重、收工封存，以及 repair / tools / shelter / recycling / medical / safety / fire 的跨域观察。

## 2. 18 cases 结果

- 用例总数：18
- strict / observation：12 / 6
- pass / partial / fail：16 / 2 / 0
- strict pass / partial / fail：10 / 2 / 0

## 3. Guide / Wiki 命中

- Guide 命中率（strict，含 allowed secondary）：100.0%
- 主 Guide 命中率（strict，仅 expected）：100.0%
- Wiki 全量命中率（strict，全部 expected Wiki）：83.3%
- Wiki 任一命中率（strict，至少一个 expected Wiki）：100.0%
- Guide-Wiki 精准组合率（strict）：100.0%
- manufacturing 主 Guide 进入率（全部 cases）：83.3%

安全指标：

- safety boundary：100.0%
- fallback：100.0%
- record/check：100.0%
- dangerous suggestion：0
- Kiwix 越权：0

## 4. Case 明细

|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|
|---|---|---|---|---|---|---|---|
|manufacturing_workbench_start_check|strict|pass|DG-0871 工坊开工前安全检查|repair-manufacturing-zone-layout-001 低资源工坊最小分区、repair-manufacturing-workbench-start-check-001 工坊工作台开工前安全检查、repair-manufacturing-dust-spark-stop-line-001 粉尘碎屑火花停止线、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-ppe-minimum-001 制造作业最低 PPE、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|manufacturing_workspace_safety|无|无|
|manufacturing_scrap_wood_shelf_judgement|strict|partial|DG-0872 材料再利用前安全判断、DG-0877 制作完成后的承重检查、DG-0876 废旧金属材料安全处理|repair-manufacturing-scrap-material-check-001 废旧材料制作前总检查、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-metal-scrap-check-001 金属废料再利用安全判断、repair-manufacturing-plastic-reuse-boundary-001 塑料容器再利用边界、repair-manufacturing-material-prep-001 材料预处理：干燥清洁去毛刺、repair-manufacturing-fabric-rope-leather-check-001 布料绳索皮革替代材料选择、repair-manufacturing-fastener-reuse-boundary-001 旧螺丝铆钉扎带再利用边界、repair-manufacturing-downgrade-label-001 材料降级使用标签|manufacturing_material_reuse|无|selector 问题|
|manufacturing_wood_cut_clamp_no_foot|strict|pass|DG-0873 木材切割前固定与支撑、DG-0839 刀锯作业前清场、DG-0834 锯切前固定与支撑|repair-manufacturing-clamp-before-processing-001 切割和钻孔前固定夹持、repair-manufacturing-wood-saw-support-001 木材重复锯切支撑、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-measure-mark-001 木材测量划线和切割线确认、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界|repair_site_clearance_boundary、manufacturing_cutting_clamping|无|无|
|manufacturing_simple_box_not_fall_apart|strict|pass|DG-0874 简易结构件制作检查、DG-0808 先用多层布做简易过滤、DG-0809 先用砂石和布做简易滤层|repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-frame-square-check-001 简易框架方正检查、repair-manufacturing-shelf-support-check-001 简易架子和支撑检查、repair-manufacturing-box-crate-check-001 简易箱体和周转箱制作检查、water-clarity-judgement-013 浑浊水的处理判断、water-filter-use-027 过滤器使用边界|无|无|无|
|manufacturing_connection_method_choice|strict|pass|DG-0875 连接方式选择：胶粘/螺丝/捆扎、DG-0874 简易结构件制作检查、DG-0877 制作完成后的承重检查|repair-manufacturing-connection-choice-001 胶粘捆扎螺丝连接选择、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-fastener-reuse-boundary-001 旧螺丝铆钉扎带再利用边界、repair-manufacturing-metal-join-choice-001 金属螺丝铆接铁丝连接边界、repair-manufacturing-fabric-rope-leather-check-001 布料绳索皮革替代材料选择、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-frame-square-check-001 简易框架方正检查|manufacturing_connection_load_check|无|无|
|manufacturing_sharp_scrap_metal_bracket|strict|pass|DG-0876 废旧金属材料安全处理、DG-0872 材料再利用前安全判断、DG-0877 制作完成后的承重检查|repair-manufacturing-metal-scrap-check-001 金属废料再利用安全判断、repair-manufacturing-metal-edge-safe-001 金属锐边去毛刺和包边、repair-manufacturing-thin-metal-drill-001 薄金属片打孔支撑、repair-manufacturing-corroded-metal-reuse-001 腐蚀金属再利用前检查、repair-manufacturing-material-prep-001 材料预处理：干燥清洁去毛刺、repair-manufacturing-metal-bracket-check-001 简易金属支架制作检查、repair-manufacturing-thin-metal-bend-001 薄金属简易弯折边界、repair-manufacturing-metal-join-choice-001 金属螺丝铆接铁丝连接边界|manufacturing_material_reuse|无|无|
|manufacturing_finished_shelf_heavy_load|strict|pass|DG-0874 简易结构件制作检查、DG-0877 制作完成后的承重检查、DG-0875 连接方式选择：胶粘/螺丝/捆扎|repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-frame-square-check-001 简易框架方正检查、repair-manufacturing-shelf-support-check-001 简易架子和支撑检查、repair-manufacturing-box-crate-check-001 简易箱体和周转箱制作检查、repair-manufacturing-load-test-001 成品承重前检查、repair-manufacturing-quality-recheck-001 成品使用前质量复查|manufacturing_connection_load_check|无|无|
|manufacturing_shutdown_child_tool_safety|strict|partial|DG-0871 工坊开工前安全检查、DG-0878 工坊收工与工具封存、DG-0838 儿童远离工具区|repair-manufacturing-zone-layout-001 低资源工坊最小分区、repair-manufacturing-workbench-start-check-001 工坊工作台开工前安全检查、repair-manufacturing-dust-spark-stop-line-001 粉尘碎屑火花停止线、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-ppe-minimum-001 制造作业最低 PPE、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|repair_child_tool_zone、manufacturing_workspace_safety|无|selector 问题|
|manufacturing_low_light_continue_cutting|strict|pass|DG-0871 工坊开工前安全检查、DG-0873 木材切割前固定与支撑、DG-0839 刀锯作业前清场|repair-manufacturing-zone-layout-001 低资源工坊最小分区、repair-manufacturing-workbench-start-check-001 工坊工作台开工前安全检查、repair-manufacturing-dust-spark-stop-line-001 粉尘碎屑火花停止线、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-ppe-minimum-001 制造作业最低 PPE、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|repair_low_light_work_stop、repair_site_clearance_boundary、manufacturing_workspace_safety、manufacturing_cutting_clamping|无|无|
|manufacturing_drilling_material_sliding|strict|pass|DG-0873 木材切割前固定与支撑、DG-0876 废旧金属材料安全处理|repair-manufacturing-clamp-before-processing-001 切割和钻孔前固定夹持、repair-manufacturing-wood-saw-support-001 木材重复锯切支撑、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-measure-mark-001 木材测量划线和切割线确认、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-metal-scrap-check-001 金属废料再利用安全判断、repair-manufacturing-metal-edge-safe-001 金属锐边去毛刺和包边|manufacturing_cutting_clamping|无|无|
|manufacturing_dust_no_mask_continue_sanding|strict|pass|DG-0871 工坊开工前安全检查|repair-manufacturing-zone-layout-001 低资源工坊最小分区、repair-manufacturing-workbench-start-check-001 工坊工作台开工前安全检查、repair-manufacturing-dust-spark-stop-line-001 粉尘碎屑火花停止线、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-ppe-minimum-001 制造作业最低 PPE、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|manufacturing_workspace_safety|无|无|
|manufacturing_sparks_near_fabric|strict|pass|DG-0871 工坊开工前安全检查、DG-0839 刀锯作业前清场、DG-0878 工坊收工与工具封存|repair-manufacturing-zone-layout-001 低资源工坊最小分区、repair-manufacturing-workbench-start-check-001 工坊工作台开工前安全检查、repair-manufacturing-dust-spark-stop-line-001 粉尘碎屑火花停止线、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-ppe-minimum-001 制造作业最低 PPE、repair-manufacturing-bystander-exclusion-001 儿童和旁人远离加工区、repair-manufacturing-damaged-tool-stop-001 工具损坏后的生产停用、repair-manufacturing-raw-finished-zones-001 原料半成品成品分区|repair_site_clearance_boundary、manufacturing_workspace_safety|无|无|
|manufacturing_observe_repair_vs_rebuild_shelf|observation|pass|DG-0827 先在水源附近设一个临时取水点、DG-0874 简易结构件制作检查、DG-0877 制作完成后的承重检查|water-container-labeling-004 储水容器标签与编号、water-safe-collection-007 安全取水的基本步骤、water-source-judgement-002 水源判断的基本标准、water-storage-location-006 储水位置选择与避险、repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-frame-square-check-001 简易框架方正检查|无|off_domain_primary|无|
|manufacturing_observe_plastic_bucket_toolbox|observation|pass|DG-0301 塑料瓶再利用、DG-0865 撤离路线检查、DG-0624 工具生锈清洁和封存|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|无|off_domain_primary、manufacturing_vs_tools|无|
|manufacturing_observe_tarp_support_pole|observation|pass|DG-0873 木材切割前固定与支撑、DG-0827 先在水源附近设一个临时取水点、DG-0307 简易遮阳和遮雨棚|repair-manufacturing-clamp-before-processing-001 切割和钻孔前固定夹持、repair-manufacturing-wood-saw-support-001 木材重复锯切支撑、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-measure-mark-001 木材测量划线和切割线确认、repair-manufacturing-cut-drill-clear-zone-001 切割钻孔打磨前清场、repair-manufacturing-wood-selection-001 木材制作前选择边界、water-container-labeling-004 储水容器标签与编号、water-safe-collection-007 安全取水的基本步骤|无|无|无|
|manufacturing_observe_rope_stick_hanger|observation|pass|DG-0827 先在水源附近设一个临时取水点、DG-0304 木板和木棍再利用、DG-0569 门窗松动：临时加固|water-container-labeling-004 储水容器标签与编号、water-safe-collection-007 安全取水的基本步骤、water-source-judgement-002 水源判断的基本标准、water-storage-location-006 储水位置选择与避险、repair-wet-wood-strength-check-001 木料受潮后强度判断、repair-wood-basic-properties-001 木材基础性质、repair-fixing-and-support-001 固定与支撑|无|off_domain_primary|无|
|manufacturing_observe_repeated_blocks_same_size|observation|pass|DG-0874 简易结构件制作检查、DG-0875 连接方式选择：胶粘/螺丝/捆扎、DG-0877 制作完成后的承重检查|repair-manufacturing-wood-selection-001 木材制作前选择边界、repair-manufacturing-wood-drill-position-001 木材打孔定位和边距检查、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-frame-square-check-001 简易框架方正检查、repair-manufacturing-shelf-support-check-001 简易架子和支撑检查、repair-manufacturing-box-crate-check-001 简易箱体和周转箱制作检查、repair-manufacturing-connection-choice-001 胶粘捆扎螺丝连接选择、repair-manufacturing-fastener-reuse-boundary-001 旧螺丝铆钉扎带再利用边界|manufacturing_connection_load_check|无|无|
|manufacturing_observe_no_screws_connection_substitute|observation|pass|DG-0875 连接方式选择：胶粘/螺丝/捆扎、DG-0376 螺丝钉钉子小件库存、DG-0855 通信设备无法连接排查|repair-manufacturing-connection-choice-001 胶粘捆扎螺丝连接选择、repair-manufacturing-wood-screw-join-001 木材螺丝连接基础、repair-manufacturing-fastener-reuse-boundary-001 旧螺丝铆钉扎带再利用边界、repair-manufacturing-metal-join-choice-001 金属螺丝铆接铁丝连接边界、repair-manufacturing-fabric-rope-leather-check-001 布料绳索皮革替代材料选择、repair-fastener-washer-storage-001 螺丝钉垫片分类保存、repair-screws-and-nuts-001 螺丝和螺母、communication-device-fault-tree-001 通信设备故障排查顺序|无|无|无|

## 5. Cross Domain 分析

- repair 抢主位观察：3
- tools 抢主位观察：3
- shelter 抢主位观察：1
- recycling 抢主位观察：1
- medical 抢主位观察：0
- safety 抢主位观察：0
- fire 抢主位观察：0
- agriculture 抢主位观察：0
- team 抢主位观察：0

Cross domain labels：
- off_domain_primary：3
- manufacturing_vs_tools：1

### 重点抢主位分析

- repair 抢主位：木板再利用、脚踩锯切、低光切割、重复木块等场景中旧 repair/tools Guide 经常作为 top evidence；多数情况下 manufacturing Guide 已进入 top3，但不是 primary。
- tools 抢主位：刀锯清场、低光维修停止线、锯切固定等旧工具安全 Guide 对加工安全 query 仍很强，说明工具安全与制造入口需要后续边界排序分析。
- shelter 抢主位：临时雨棚支撑杆属于 observation，当前 manufacturing evidence 能进入；后续需要判断 shelter 用途和制作动作谁应主导。
- recycling / waste 抢主位：废塑料桶和废木板 query 暴露材料再利用与旧物再利用入口竞争，当前结果主要表现为旧 tools/repair 或 navigation 异常证据进入，而不是专用 recycling 域。
- medical / safety 抢主位：粉尘无口罩 case 被口罩/野外食物等非制造证据带偏；金属锐边 case 表现较好，DG-0876 能主导。
- fire 抢主位：火花飞到布料 strict case 中 manufacturing Wiki 进入，但 top Guide 是旧刀锯清场，Fire 未明显抢主位；问题更像工具安全入口抢主位。

## 6. Manufacturing Evidence 稳定性

- Manufacturing Guide 作为 top evidence 的比例：83.3%。
- Strict cases 中 Guide 命中率：100.0%。
- Strict cases 中 Wiki 全量命中率：83.3%。
- Guide-Wiki 精准组合率：100.0%。
- 本阶段只记录命中表现，不根据结果调整 profile、selector、ranking 或知识内容。

## 7. 是否需要 profile

需要进入 Batch9-E Manufacturing Retrieval Root Cause Review 后再判断。当前不直接新增 profile。
Root cause 初步分类：
- selector 问题：2

## 8. 是否进入 Manufacturing Retrieval Root Cause Review

建议进入 Batch9-E Manufacturing Retrieval Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。

## 9. 逐条复盘

### manufacturing_workbench_start_check

- query：开工前做一个小工作台，要先检查什么？
- 类型：strict
- focus：工坊开工前安全检查，工作区分区、台面、工具异常和旁人隔离应进入 evidence。
- verdict：pass
- expected Guide：DG-0871
- allowed secondary：无
- selected Guide：DG-0871
- expected Wiki：repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-zone-layout-001
- selected Wiki：repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-ppe-minimum-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_scrap_wood_shelf_judgement

- query：废木板能不能拿来做架子，怎么判断？
- 类型：strict
- focus：材料再利用判断，木材选择、废旧材料检查和承重边界应进入 evidence。
- verdict：partial
- expected Guide：DG-0872
- allowed secondary：DG-0874、DG-0877
- selected Guide：DG-0872、DG-0877、DG-0876
- expected Wiki：repair-manufacturing-load-test-001、repair-manufacturing-scrap-material-check-001、repair-manufacturing-wood-selection-001
- selected Wiki：repair-manufacturing-scrap-material-check-001、repair-manufacturing-wood-selection-001、repair-manufacturing-metal-scrap-check-001、repair-manufacturing-plastic-reuse-boundary-001、repair-manufacturing-material-prep-001、repair-manufacturing-fabric-rope-leather-check-001、repair-manufacturing-fastener-reuse-boundary-001、repair-manufacturing-downgrade-label-001
- guide_hit / wiki_hit / precise：True / False / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：selector 问题
- failure reasons：未命中全部预期 Wiki

### manufacturing_wood_cut_clamp_no_foot

- query：切木板前怎么固定，能不能直接用脚踩着锯？
- 类型：strict
- focus：木材切割前固定与支撑，脚踩手扶应作为错误操作被阻断。
- verdict：pass
- expected Guide：DG-0873
- allowed secondary：DG-0834
- selected Guide：DG-0873、DG-0839、DG-0834
- expected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001
- selected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-wood-selection-001、repair-knife-saw-clear-zone-001、repair-bystander-position-boundary-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_simple_box_not_fall_apart

- query：想做一个简易箱体，怎么保证不散架？
- 类型：strict
- focus：简易结构件制作检查，框架、拼接、连接和成品复查应进入 evidence。
- verdict：pass
- expected Guide：DG-0874
- allowed secondary：DG-0875、DG-0877
- selected Guide：DG-0874、DG-0808、DG-0809
- expected Wiki：repair-manufacturing-box-crate-check-001、repair-manufacturing-frame-square-check-001、repair-manufacturing-wood-screw-join-001
- selected Wiki：repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-frame-square-check-001、repair-manufacturing-shelf-support-check-001、repair-manufacturing-box-crate-check-001、water-clarity-judgement-013、water-filter-use-027
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_connection_method_choice

- query：胶水、螺丝、绳子捆扎，什么时候用哪种连接？
- 类型：strict
- focus：连接方式选择，胶粘、螺丝、捆扎、铆接/铁丝边界应进入 evidence。
- verdict：pass
- expected Guide：DG-0875
- allowed secondary：无
- selected Guide：DG-0875、DG-0874、DG-0877
- expected Wiki：repair-manufacturing-connection-choice-001、repair-manufacturing-fastener-reuse-boundary-001、repair-manufacturing-metal-join-choice-001
- selected Wiki：repair-manufacturing-connection-choice-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-fastener-reuse-boundary-001、repair-manufacturing-metal-join-choice-001、repair-manufacturing-fabric-rope-leather-check-001、repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-frame-square-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_sharp_scrap_metal_bracket

- query：废金属片边缘很锋利，能不能直接拿来做支架？
- 类型：strict
- focus：废旧金属材料安全处理，金属废料判断、防割边和支架承重风险应进入 evidence。
- verdict：pass
- expected Guide：DG-0876
- allowed secondary：DG-0877
- selected Guide：DG-0876、DG-0872、DG-0877
- expected Wiki：repair-manufacturing-metal-bracket-check-001、repair-manufacturing-metal-edge-safe-001、repair-manufacturing-metal-scrap-check-001
- selected Wiki：repair-manufacturing-metal-scrap-check-001、repair-manufacturing-metal-edge-safe-001、repair-manufacturing-thin-metal-drill-001、repair-manufacturing-corroded-metal-reuse-001、repair-manufacturing-material-prep-001、repair-manufacturing-metal-bracket-check-001、repair-manufacturing-thin-metal-bend-001、repair-manufacturing-metal-join-choice-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_finished_shelf_heavy_load

- query：做好的架子能不能直接放重物？
- 类型：strict
- focus：制作完成后的承重检查，逐级加载、连接点复查和降级用途应进入 evidence。
- verdict：pass
- expected Guide：DG-0877
- allowed secondary：DG-0874
- selected Guide：DG-0874、DG-0877、DG-0875
- expected Wiki：repair-manufacturing-load-test-001、repair-manufacturing-quality-recheck-001、repair-manufacturing-shelf-support-check-001
- selected Wiki：repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-frame-square-check-001、repair-manufacturing-shelf-support-check-001、repair-manufacturing-box-crate-check-001、repair-manufacturing-load-test-001、repair-manufacturing-quality-recheck-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_shutdown_child_tool_safety

- query：收工时工具和半成品怎么处理，避免孩子碰到？
- 类型：strict
- focus：工坊收工与工具封存，危险工具、半成品、儿童隔离和记录应进入 evidence。
- verdict：partial
- expected Guide：DG-0878
- allowed secondary：DG-0871
- selected Guide：DG-0871、DG-0878、DG-0838
- expected Wiki：repair-manufacturing-bystander-exclusion-001、repair-manufacturing-end-clean-count-001、repair-manufacturing-raw-finished-zones-001
- selected Wiki：repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-ppe-minimum-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / False / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：selector 问题
- failure reasons：未命中全部预期 Wiki

### manufacturing_low_light_continue_cutting

- query：低光环境下还要继续切割材料，可以吗？
- 类型：strict
- focus：低光制造停止线。Tools / Repair 可补充，但 manufacturing safety 应进入 evidence。
- verdict：pass
- expected Guide：DG-0871
- allowed secondary：DG-0836、DG-0873
- selected Guide：DG-0871、DG-0873、DG-0839
- expected Wiki：repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-workbench-start-check-001
- selected Wiki：repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-ppe-minimum-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_drilling_material_sliding

- query：打孔时材料一直滑动怎么办？
- 类型：strict
- focus：固定夹持和打孔定位。Repair 可补充，但 manufacturing 应进入 evidence。
- verdict：pass
- expected Guide：DG-0873
- allowed secondary：DG-0834、DG-0876
- selected Guide：DG-0873、DG-0876
- expected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-drill-position-001
- selected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-wood-selection-001、repair-manufacturing-metal-scrap-check-001、repair-manufacturing-metal-edge-safe-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_dust_no_mask_continue_sanding

- query：粉尘很多但没有专业口罩，还能继续磨吗？
- 类型：strict
- focus：工坊粉尘控制和最低 PPE。Medical / safety 只能 secondary。
- verdict：pass
- expected Guide：DG-0871
- allowed secondary：无
- selected Guide：DG-0871
- expected Wiki：repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-ppe-minimum-001
- selected Wiki：repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-ppe-minimum-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_sparks_near_fabric

- query：火花飞到旁边布料上，工作区要怎么处理？
- 类型：strict
- focus：工坊火花风险和工作区清理。Fire 可补充，但 manufacturing safety 应进入 evidence。
- verdict：pass
- expected Guide：DG-0871
- allowed secondary：无
- selected Guide：DG-0871、DG-0839、DG-0878
- expected Wiki：repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-dust-spark-stop-line-001
- selected Wiki：repair-manufacturing-zone-layout-001、repair-manufacturing-workbench-start-check-001、repair-manufacturing-dust-spark-stop-line-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-ppe-minimum-001、repair-manufacturing-bystander-exclusion-001、repair-manufacturing-damaged-tool-stop-001、repair-manufacturing-raw-finished-zones-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_observe_repair_vs_rebuild_shelf

- query：修一个坏掉的架子和重新做一个架子有什么区别？
- 类型：observation
- focus：观察 repair / manufacturing 边界：修旧物可由 repair 主导，重新制作应带出 manufacturing evidence。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0827、DG-0874、DG-0877
- expected Wiki：无
- selected Wiki：water-container-labeling-004、water-safe-collection-007、water-source-judgement-002、water-storage-location-006、repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-frame-square-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary
- root cause：无
- failure reasons：无

### manufacturing_observe_plastic_bucket_toolbox

- query：废塑料桶能不能改成工具盒？
- 类型：observation
- focus：观察 recycling / manufacturing 边界：再利用材料判断与工具收纳用途。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0301、DG-0865、DG-0624
- expected Wiki：无
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、manufacturing_vs_tools
- root cause：无
- failure reasons：无

### manufacturing_observe_tarp_support_pole

- query：临时做一个雨棚支撑杆。
- 类型：observation
- focus：观察 shelter / manufacturing 边界：雨棚用途可由 shelter 补充，支撑杆制作应带出材料和承重 evidence。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0873、DG-0827、DG-0307
- expected Wiki：无
- selected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-wood-selection-001、water-container-labeling-004、water-safe-collection-007
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_observe_rope_stick_hanger

- query：绳子和木棍做一个临时挂架。
- 类型：observation
- focus：观察 tools / repair / manufacturing 边界：软材料、木材和承重检查是否协同。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0827、DG-0304、DG-0569
- expected Wiki：无
- selected Wiki：water-container-labeling-004、water-safe-collection-007、water-source-judgement-002、water-storage-location-006、repair-wet-wood-strength-check-001、repair-wood-basic-properties-001、repair-fixing-and-support-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary
- root cause：无
- failure reasons：无

### manufacturing_observe_repeated_blocks_same_size

- query：做几个一样大小的小木块，怎么保证尺寸一致？
- 类型：observation
- focus：观察 repeated production / 模板 / 批量记录是否进入 evidence。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0874、DG-0875、DG-0877
- expected Wiki：无
- selected Wiki：repair-manufacturing-wood-selection-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-frame-square-check-001、repair-manufacturing-shelf-support-check-001、repair-manufacturing-box-crate-check-001、repair-manufacturing-connection-choice-001、repair-manufacturing-fastener-reuse-boundary-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### manufacturing_observe_no_screws_connection_substitute

- query：没有螺丝了，用什么方式替代连接？
- 类型：observation
- focus：观察 repair / manufacturing / material substitution 协同。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0875、DG-0376、DG-0855
- expected Wiki：无
- selected Wiki：repair-manufacturing-connection-choice-001、repair-manufacturing-wood-screw-join-001、repair-manufacturing-fastener-reuse-boundary-001、repair-manufacturing-metal-join-choice-001、repair-manufacturing-fabric-rope-leather-check-001、repair-fastener-washer-storage-001、repair-screws-and-nuts-001、communication-device-fault-tree-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

## 10. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_manufacturing_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_manufacturing_field.py --no-answer
```

边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。
