# Batch8-C1 Navigation & Field Movement Knowledge Apply Planning

生成日期：2026-07-16

本阶段只生成 Navigation v0.1 第一阶段 Apply 计划。未修改 Wiki、Guide、Guide-Wiki、Retrieval、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或 tests。

参考：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch8_b_navigation_field_movement_plan.md`

目标：把 Batch8-B 的 36 篇 Navigation 规划收束为第一批可 Apply 范围，优先建立“个人定位和返回能力”：能定位、能记录、能规划返回、能识别迷路、能把 FT track 转成可行动返回摘要。

## 1. Wiki 清单

Navigation v0.1 第一批建议 Apply 32 篇 Wiki。筛选原则：

- 优先 P0 / high / caution。
- 优先 GNSS、坐标、返回路线、迷路停止线、路线检查点、FT track。
- 保留少量撤离/通信联动条目，用于路线通行性和位置报告。
- 暂缓团队搜索、大规模团队移动、复杂地图系统、高级地形分析和团队同步深水区。

### 1.1 第一批 Apply Wiki

|#|slug|title|category|priority|risk_level|summary|intended Guide|
|---:|---|---|---|---|---|---|---|
|1|navigation-gnss-basics-001|GNSS 定位基础和限制|地图地形与环境监测|P0|caution|解释 GNSS/FIX/卫星数/时间源的最小含义，避免把无 fix 或漂移位置当真实位置。|DG-0867|
|2|navigation-gnss-no-fix-downgrade-001|GNSS 无信号时的降级定位|地图地形与环境监测|P0|high|无 fix 时用最近确定点、地标、方向和时间估算降级，不继续深入陌生路线。|DG-0864|
|3|navigation-coordinate-record-format-001|坐标记录最小字段|地图地形与环境监测|P0|caution|记录坐标、时间、来源、精度、地标和记录人，支持纸质交接。|DG-0867|
|4|navigation-coordinate-paper-backup-001|坐标纸质备份和交接|地图地形与环境监测|P1|normal|把基地点、水源点、集合点和危险点坐标写到纸面并定期复核。|DG-0867 / DG-0866|
|5|navigation-position-error-check-001|定位误差复查|地图地形与环境监测|P0|caution|用多次读数、地标和移动方向检查位置漂移，避免被单点误导。|DG-0867|
|6|navigation-compass-orientation-basics-001|指南针和地图定向基础|地图地形与环境监测|P0|caution|用指南针、地标和地图北向确认行动方向。|DG-0863|
|7|navigation-bearing-distance-estimate-001|方位和距离估算|地图地形与环境监测|P0|caution|用步数、时间、地标间距和方位估算下一检查点距离。|DG-0863 / DG-0864|
|8|navigation-time-distance-turnback-line-001|时间距离和折返线|地图地形与环境监测|P0|high|出发前设最晚折返时间和距离，不因目标接近而透支返回能力。|DG-0863|
|9|navigation-lost-stop-and-last-known-point-001|迷路后的停止和最近确定点|地图地形与环境监测|P0|high|迷路时停止扩散、回到最近确定点、记录最后确定位置。|DG-0864|
|10|navigation-offline-map-version-control-001|离线地图版本管理|地图地形与环境监测|P1|normal|管理地图区域、日期、来源、设备副本和废止版本。|DG-0863|
|11|navigation-offline-map-layer-check-001|离线地图图层检查|地图地形与环境监测|P1|normal|检查道路、地形、水系、避难点和危险点图层是否可离线打开。|DG-0863|
|12|navigation-poi-minimum-fields-001|POI 点位最小字段|地图地形与环境监测|P0|caution|资源点、危险点、集合点记录位置、类型、状态、复查时间和可信度。|DG-0866 / DG-0870|
|13|navigation-paper-map-index-card-001|纸质地图索引卡|地图地形与环境监测|P1|normal|记录地图编号、覆盖范围、图例和存放位置，支持电子地图不可用时降级。|DG-0863|
|14|navigation-map-symbol-standard-001|地图符号和图例统一|地图地形与环境监测|P0|caution|统一水源、危险、禁行、返回、检查点符号，避免只有绘制者能读懂。|DG-0866|
|15|navigation-outbound-route-card-001|个人外出去程路线卡|地图地形与环境监测|P0|high|记录目的、路线、检查点、风险、最晚返回和留守联系人。|DG-0863|
|16|navigation-return-route-plan-001|返回路线规划|地图地形与环境监测|P0|high|出发前规划原路返回、备用返回、最近安全点和撤回线。|DG-0863 / DG-0864|
|17|navigation-alternate-route-selection-001|备用路线选择|地图地形与环境监测|P0|caution|用风险、距离、地形、通信覆盖和水源决定备用路线。|DG-0865|
|18|navigation-route-checkpoint-numbering-001|路线检查点编号|地图地形与环境监测|P0|caution|按起点、转折点、危险点、补给点和返回点编号，支持留守方判断是否超时。|DG-0866|
|19|navigation-route-risk-score-001|撤离路线风险评分|地图地形与环境监测|P0|high|用洪水、夜间、坡地、冲突、体力和通信因素评分。|DG-0865|
|20|navigation-route-closure-recheck-001|道路封闭和绕行复查|地图地形与环境监测|P0|high|遇封路、断桥、积水、冲突时停止硬闯，记录绕行或撤回。|DG-0865|
|21|navigation-flood-road-stop-line-001|洪水后道路停止线|地图地形与环境监测|P0|high|判断积水、暗流、桥涵、泥泞和水位变化，不把熟路当安全路。|DG-0865|
|22|navigation-slope-landslide-bypass-001|坡地和滑坡绕行|地图地形与环境监测|P0|high|识别裂缝、落石、湿坡和崩塌边缘，优先绕行或撤回。|DG-0865|
|23|navigation-night-movement-stop-line-001|夜间移动停止线|地图地形与环境监测|P0|high|夜间缺照明、标记不连续、队伍疲劳或天气恶化时停止深入。|DG-0863 / DG-0865|
|24|navigation-gps-track-minimum-fields-001|GPS 轨迹最小字段|地图地形与环境监测|P0|caution|解释 track/session/path point/time/source，并说明如何读轨迹。|DG-0870|
|25|navigation-track-drift-gap-check-001|轨迹漂移和断点检查|地图地形与环境监测|P0|caution|识别漂移、跳点、断点和停留点，避免按坏轨迹返回。|DG-0870|
|26|navigation-track-to-return-summary-001|轨迹转返回摘要|地图地形与环境监测|P0|caution|把轨迹简化为可复述的地标、转折点、耗时和危险点。|DG-0864 / DG-0870|
|27|navigation-team-rally-point-rules-001|团队集合点规则|地图地形与环境监测|P0|caution|规定主集合点、备用点、失效条件、变更记录和人员确认。|DG-0866|
|28|navigation-missed-checkpoint-response-001|错过检查点后的处理|地图地形与环境监测|P0|high|未按时到检查点时留守方和外出方的等待、回撤、通信和搜索边界。|DG-0866|
|29|navigation-task-route-record-001|任务路线记录|地图地形与环境监测|P0|caution|把取水、巡查、采集和转移任务写成可复核路线。|DG-0870|
|30|evacuation-route-passability-review-001|撤离路线通行复查|避难转移|P0|high|撤离前按水、火、坍塌、冲突、照明和人员状态复查路线。|DG-0865|
|31|evacuation-route-go-no-go-line-001|撤离路线继续或停止线|避难转移|P0|high|明确何时继续、绕行、等待或取消撤离。|DG-0865|
|32|communication-position-report-navigation-fields-001|位置报告的导航字段|通讯|P0|caution|报位置时包含坐标、地标、方向、检查点、返回路线和下一窗口。|DG-0866|

### 1.2 暂缓 Wiki

|slug|暂缓原因|建议版本|
|---|---|---|
|navigation-vegetation-visibility-risk-001|属于更细的地形/可视线风险，v0.1 可由夜间、洪水、坡地和路线评分覆盖。|v0.2|
|navigation-track-share-team-001|重点是团队共享和后处理，需等 v0.1 track 字段、漂移检查、返回摘要稳定后再做。|v0.2|
|navigation-search-area-boundary-001|团队搜索范围属于团队移动和失联处理深水区，不适合第一批和个人返回能力混在一起。|v0.2|
|communication-navigation-sync-handover-001|FT、纸图、通信日志同步交接需要等 navigation / communication 联合 Field Test 后再定。|v0.2 / v0.3|

## 2. Guide 清单

第一阶段最多新增 6 个 Guide。建议新增以下 6 个，不新增 DG-0868 / DG-0869；夜间移动与危险地形先作为 DG-0863 / DG-0865 的 stop_or_escalate 和 related_wiki 支撑，避免 Guide 入口过碎。

### DG-0863 个人外出路线规划

- category：地图地形与环境监测
- priority：P0
- risk_level：high
- scenario：个人外出取水、巡查、采集或寻找物资，需要在出发前确定去程、返回、检查点、风险和停止线。
- steps：
  1. 写明人员、目的、出发时间、最晚返回时间和留守联系人。
  2. 标出起点、目标、去程、返回路线和备用路线。
  3. 设置检查点、折返线和夜间停止线。
  4. 标出危险点、禁行点、通信窗口和可等待位置。
  5. 出发前把路线卡交给留守人或写入固定记录。
- check：
  - 返回路线能被别人复述。
  - 至少有一个备用路线或撤回点。
  - 最晚返回时间和折返线明确。
  - 留守人知道超时后先做什么。
- stop_or_escalate：
  - 无返回路线、无地图/草图、夜间照明不足、天气恶化、人员体力不足、通信窗口不可用时不扩大外出范围。
- fallback：
  - 没有地图时使用手绘草图、地标顺序、步数和时间。
  - 不能确认返回时缩短任务距离，只到最近确定点。
- related_wiki：
  - navigation-outbound-route-card-001
  - navigation-return-route-plan-001
  - navigation-time-distance-turnback-line-001
  - navigation-compass-orientation-basics-001
  - navigation-offline-map-version-control-001
  - navigation-offline-map-layer-check-001
  - navigation-paper-map-index-card-001
  - navigation-night-movement-stop-line-001

### DG-0864 迷路后的定位与返回

- category：地图地形与环境监测
- priority：P0
- risk_level：high
- scenario：外出后方向不确定、路线标记丢失、GNSS 无 fix、地图不可用或无法确认当前位置。
- steps：
  1. 停止继续深入，不开新路。
  2. 找最近确定点、最后检查点、明显地标或最后可信 track point。
  3. 比对方向、时间、地形、轨迹和身体状态。
  4. 优先返回最近确定点，而不是继续找目标。
  5. 记录迷路时间、最后可信位置和采取的返回动作。
- check：
  - 最近确定点明确。
  - 队伍人数完整。
  - 光照、体力、天气允许返回。
  - 返回路线可复述。
- stop_or_escalate：
  - 两个连续节点无法确认、天黑、天气恶化、有人受伤、GNSS 漂移明显、轨迹断点明显时停止继续移动。
- fallback：
  - 留在安全可见位置。
  - 用哨声、灯光、通信或纸条报告位置。
  - 无通信时按原定等待规则保留体力。
- related_wiki：
  - navigation-lost-stop-and-last-known-point-001
  - navigation-gnss-no-fix-downgrade-001
  - navigation-bearing-distance-estimate-001
  - navigation-position-error-check-001
  - navigation-track-to-return-summary-001

### DG-0865 撤离路线检查

- category：避难转移
- priority：P0
- risk_level：high
- scenario：需要判断某条撤离或外出路线是否还能走，尤其在洪水、坍塌、冲突、烟火、夜间或天气变化后。
- steps：
  1. 对路线做风险评分。
  2. 检查积水、桥涵、坡地、坍塌、冲突、烟火、照明、人员状态和返回路。
  3. 设继续、绕行、等待和取消条件。
  4. 标记不可通行点和复查时间。
  5. 把路线版本交给留守人或团队记录。
- check：
  - 危险点已标出。
  - 备用路线存在。
  - 返回路线没有被切断。
  - 继续/停止线不是临时口头决定。
- stop_or_escalate：
  - 水流不明、桥面受损、坡面开裂、落石、烟火阻断、夜间照明不足或路线进入冲突区域时停止通行。
- fallback：
  - 改用备用路线。
  - 缩短移动距离。
  - 转入临时避难点等待复查。
- related_wiki：
  - navigation-route-risk-score-001
  - navigation-alternate-route-selection-001
  - navigation-route-closure-recheck-001
  - navigation-flood-road-stop-line-001
  - navigation-slope-landslide-bypass-001
  - evacuation-route-passability-review-001
  - evacuation-route-go-no-go-line-001
  - navigation-night-movement-stop-line-001

### DG-0866 检查点记录与团队集合

- category：地图地形与环境监测
- priority：P0
- risk_level：high
- scenario：个人外出或小队分组行动时，需要用检查点、集合点和位置报告让留守方判断是否正常、超时或需要停止行动。
- steps：
  1. 编号起点、转折点、危险点、补给点、目标点和返回点。
  2. 为每个检查点写到达时间、最晚报告时间和下一动作。
  3. 定义主集合点、备用集合点和失效条件。
  4. 位置报告包含坐标/地标/方向/风险/下一窗口。
  5. 留守方记录超时、等待、回撤或启动搜索的边界。
- check：
  - 检查点编号清楚。
  - 集合点至少两级。
  - 位置报告字段完整。
  - 通信记录和纸图一致。
- stop_or_escalate：
  - 检查点错过、集合点失效、队伍分散、天气恶化、危险点扩大或连续通信窗口失败时停止按原计划推进。
- fallback：
  - 回到上一个检查点。
  - 转备用集合点。
  - 用纸条、哨声、灯光或下一个通信窗口降级。
- related_wiki：
  - navigation-route-checkpoint-numbering-001
  - navigation-team-rally-point-rules-001
  - navigation-missed-checkpoint-response-001
  - navigation-map-symbol-standard-001
  - navigation-poi-minimum-fields-001
  - communication-position-report-navigation-fields-001
  - navigation-coordinate-paper-backup-001

### DG-0867 GNSS 坐标记录

- category：地图地形与环境监测
- priority：P0
- risk_level：caution
- scenario：使用 FT、手机或 GNSS 设备记录基地点、水源、风险点、集合点、检查点或路线节点。
- steps：
  1. 确认 fix 状态、时间来源和设备电量。
  2. 记录坐标、时间、设备、精度/卫星状态、地标描述和记录人。
  3. 做第二次读数或用地标复核。
  4. 写入纸质备份或同步给留守人。
  5. 标记点位类型和下次复查时间。
- check：
  - 有 fix。
  - 坐标有时间。
  - 地标可识别。
  - 纸质备份存在。
  - 单点漂移已复查。
- stop_or_escalate：
  - 无 fix、位置跳变、地标无法说明、设备电量不足、记录人无法解释坐标来源时，不把该坐标作为关键决策依据。
- fallback：
  - 用地标、方向、距离、步数、时间和手绘草图替代坐标。
- related_wiki：
  - navigation-gnss-basics-001
  - navigation-coordinate-record-format-001
  - navigation-coordinate-paper-backup-001
  - navigation-position-error-check-001
  - navigation-gnss-no-fix-downgrade-001

### DG-0870 野外任务轨迹记录

- category：地图地形与环境监测
- priority：P0
- risk_level：caution
- scenario：使用 FT、手机或 GNSS 记录巡查、取水、采集、探路或撤离前侦察轨迹，并需要把轨迹转成可返回、可交接的路线摘要。
- steps：
  1. 开始 session，确认 base 和时间。
  2. 记录关键 path point：转折点、危险点、资源点、等待点、返回点。
  3. 给重要点写备注，不只保存坐标。
  4. 返回后检查漂移、跳点、断点和停留点。
  5. 转成纸质路线摘要：地标、方向、耗时、危险点、返回线。
- check：
  - session 有起止。
  - 关键点有时间和备注。
  - 轨迹可回放。
  - 漂移和断点已标记。
  - 返回摘要可被第二个人理解。
- stop_or_escalate：
  - 无 fix、SD 不可写、轨迹跳点、设备低电、路线与实际不符、base 不可信时，不按轨迹直接返回。
- fallback：
  - 手写点位和时间。
  - 用地标顺序替代轨迹。
  - 回到基地后补录为任务路线记录。
- related_wiki：
  - navigation-gps-track-minimum-fields-001
  - navigation-track-drift-gap-check-001
  - navigation-track-to-return-summary-001
  - navigation-task-route-record-001
  - navigation-poi-minimum-fields-001

## 3. Evidence 设计

### 3.1 Guide -> Wiki 规划

|Guide|主 evidence Wiki|为什么支持该 Guide|补充 evidence Wiki|关系边界|
|---|---|---|---|---|
|DG-0863|navigation-outbound-route-card-001; navigation-return-route-plan-001; navigation-time-distance-turnback-line-001|三者共同定义个人外出的路线、返回和折返线，是 v0.1 行动入口核心。|navigation-compass-orientation-basics-001; navigation-offline-map-version-control-001; navigation-offline-map-layer-check-001; navigation-paper-map-index-card-001; navigation-night-movement-stop-line-001|不链接团队搜索、复杂同步或高级地形分析。|
|DG-0864|navigation-lost-stop-and-last-known-point-001; navigation-gnss-no-fix-downgrade-001|直接支撑迷路停止、最近确定点和无 fix 降级。|navigation-bearing-distance-estimate-001; navigation-position-error-check-001; navigation-track-to-return-summary-001|不把通信失联、治安风险或撤离决策作为主证据。|
|DG-0865|navigation-route-risk-score-001; evacuation-route-passability-review-001; evacuation-route-go-no-go-line-001|支撑路线可走/不可走判断和继续/停止线。|navigation-alternate-route-selection-001; navigation-route-closure-recheck-001; navigation-flood-road-stop-line-001; navigation-slope-landslide-bypass-001; navigation-night-movement-stop-line-001|不替代 Shelter 是否可留、Evacuation 是否必须发生的上游决策。|
|DG-0866|navigation-route-checkpoint-numbering-001; navigation-missed-checkpoint-response-001; communication-position-report-navigation-fields-001|支撑检查点、失联窗口和位置报告字段。|navigation-team-rally-point-rules-001; navigation-map-symbol-standard-001; navigation-poi-minimum-fields-001; navigation-coordinate-paper-backup-001|不进入大规模搜索范围和队伍战术。|
|DG-0867|navigation-gnss-basics-001; navigation-coordinate-record-format-001; navigation-position-error-check-001|支撑 GNSS fix、坐标最小字段和误差复查。|navigation-coordinate-paper-backup-001; navigation-gnss-no-fix-downgrade-001|不处理通信设备维修或地图文件管理。|
|DG-0870|navigation-gps-track-minimum-fields-001; navigation-track-drift-gap-check-001; navigation-track-to-return-summary-001|支撑 FT track 的字段、异常检查和返回摘要。|navigation-task-route-record-001; navigation-poi-minimum-fields-001|不进入团队共享、云同步或复杂地图渲染。|

### 3.2 Wiki -> Guide 规划

每篇新增 Wiki 的 `guide_links` 应只指向实际行动入口。建议关系：

|Wiki slug|guide_links|理由|
|---|---|---|
|navigation-gnss-basics-001|DG-0867|GNSS 基础只直接支撑坐标记录。|
|navigation-gnss-no-fix-downgrade-001|DG-0864, DG-0867|既支撑迷路降级，也支撑坐标记录停止线。|
|navigation-coordinate-record-format-001|DG-0867|坐标字段属于 GNSS 坐标记录主证据。|
|navigation-coordinate-paper-backup-001|DG-0867, DG-0866|坐标纸质备份用于坐标交接和检查点集合。|
|navigation-position-error-check-001|DG-0867, DG-0864|误差复查支撑坐标可信度和迷路判断。|
|navigation-compass-orientation-basics-001|DG-0863|指南针定向是外出路线规划的基础动作。|
|navigation-bearing-distance-estimate-001|DG-0863, DG-0864|用于计划检查点，也用于迷路时回推距离。|
|navigation-time-distance-turnback-line-001|DG-0863|折返线属于出发前规划，不单独扩展到撤离决策。|
|navigation-lost-stop-and-last-known-point-001|DG-0864|迷路停止线是 DG-0864 主证据。|
|navigation-offline-map-version-control-001|DG-0863|离线地图版本是出发前准备的一部分。|
|navigation-offline-map-layer-check-001|DG-0863|地图图层检查用于确认路线卡是否可用。|
|navigation-poi-minimum-fields-001|DG-0866, DG-0870|POI 同时支撑检查点记录和 track 备注。|
|navigation-paper-map-index-card-001|DG-0863|纸质索引用于手机地图不可用时的路线准备。|
|navigation-map-symbol-standard-001|DG-0866|统一符号用于检查点、集合点和危险点沟通。|
|navigation-outbound-route-card-001|DG-0863|个人外出去程路线卡是 DG-0863 主证据。|
|navigation-return-route-plan-001|DG-0863, DG-0864|出发前计划和迷路返回都需要返回路线。|
|navigation-alternate-route-selection-001|DG-0865|备用路线属于路线检查和撤离路可通行判断。|
|navigation-route-checkpoint-numbering-001|DG-0866|检查点编号是 DG-0866 主证据。|
|navigation-route-risk-score-001|DG-0865|路线风险评分是 DG-0865 主证据。|
|navigation-route-closure-recheck-001|DG-0865|封路和绕行复查属于路线通行性判断。|
|navigation-flood-road-stop-line-001|DG-0865|洪水路线停止线支撑撤离路线检查。|
|navigation-slope-landslide-bypass-001|DG-0865|坡地和滑坡绕行支撑危险路线检查。|
|navigation-night-movement-stop-line-001|DG-0863, DG-0865|夜间停止线既约束个人外出，也约束撤离路线通行。|
|navigation-gps-track-minimum-fields-001|DG-0870|track 字段是 DG-0870 主证据。|
|navigation-track-drift-gap-check-001|DG-0870|漂移/断点是轨迹行动化前的停止线。|
|navigation-track-to-return-summary-001|DG-0870, DG-0864|轨迹摘要既用于任务记录，也用于返回。|
|navigation-team-rally-point-rules-001|DG-0866|集合点规则支撑检查点和团队集合。|
|navigation-missed-checkpoint-response-001|DG-0866|错过检查点是 DG-0866 的关键异常场景。|
|navigation-task-route-record-001|DG-0870|任务路线记录支撑 track 后处理和复盘。|
|evacuation-route-passability-review-001|DG-0865|撤离路线通行复查属于 DG-0865 主证据。|
|evacuation-route-go-no-go-line-001|DG-0865|继续/停止线属于 DG-0865 主证据。|
|communication-position-report-navigation-fields-001|DG-0866|位置报告字段支撑检查点和集合点联动。|

### 3.3 Existing maps 断链修复规划

Batch8-B 发现 `wiki_import/maps` 有 16 篇无 `guide_links`。Batch8-C Apply 可在不改正文的前提下，精准补双向关系；但不要为了清空断链把所有 maps Wiki 塞入新 Guide。

建议优先修复：

|现有 Wiki|建议 Guide|理由|
|---|---|---|
|navigation-offline-map-maintenance-001|DG-0863|直接支撑出发前离线地图准备。|
|navigation-return-route-marking-001|DG-0863 / DG-0864|支撑返回路线标记和迷路返回。|
|navigation-flood-terrain-risk-001|DG-0865|支撑洪水后路线通行性判断。|
|navigation-night-route-risk-001|DG-0863 / DG-0865|支撑夜间移动停止线。|
|navigation-route-difficulty-record-001|DG-0865|支撑路线风险评分和复查。|
|navigation-danger-zone-marking-001|DG-0865 / DG-0866|支撑危险点标记与检查点沟通。|
|navigation-map-scale-001|DG-0863|支撑距离估算，但不是独立行动入口。|
|navigation-terrain-height-001|DG-0865|支撑高差、坡地和绕行判断。|
|navigation-water-level-change-001|DG-0865|支撑洪水/积水路线停止线。|
|navigation-landmark-selection-001|DG-0863 / DG-0864|支撑路线卡和最近确定点。|

暂不优先修复：

|现有 Wiki|暂缓原因|
|---|---|
|navigation-temperature-humidity-record-001|偏天气/环境记录，不是 v0.1 导航行动入口。|
|navigation-pressure-change-001|偏天气趋势，不直接支撑个人定位返回。|
|navigation-rainfall-record-001|可作为路线复查补充，但优先级低于洪水停止线。|
|navigation-soil-moisture-judgement-001|偏农业/环境判断。|
|navigation-weather-observation-basics-index-001|索引型背景，不作为主行动证据。|
|navigation-wind-direction-observation-001|偏天气观察，可后续与路线烟火/风向风险结合。|

## 4. FT 对接需求

本阶段只记录需求，不进入代码。

|FT 能力|当前基础|v0.1 软件需求|v0.1 知识需求|
|---|---|---|---|
|GNSS fix|FT-01 已有 GNSS/FIX 状态|显示 fix/no fix、时间源、可能精度或状态提示|解释 fix 可信度、无 fix 降级、单次读数不可作为唯一依据。|
|坐标记录|可记录 base / path point|保存坐标、时间、session、point type、备注|坐标最小字段、纸质备份、地标复核。|
|Path point|FT-01 有 path point|关键点标记：转折、危险、资源、等待、返回|POI 最小字段和任务路线记录。|
|Track|FT-01 有 auto track 和 session|track 起止、断点提示、导出/查看|track 字段、漂移/断点检查、转返回摘要。|
|Session|FT-01 有 session start/stop|session 命名和任务关联|任务路线记录、返回后复盘。|
|SD 记录|已有 SD 路径和 JSONL 记录|写入失败提示和备份提醒|SD 不可写时手写降级。|
|Map|缺地图显示|后续离线地图、POI、风险标记|离线地图版本、图层检查、纸质地图降级。|
|团队同步|未形成导航闭环|后续 Core sync / team handover|v0.1 只要求位置报告字段，团队同步放到 v0.2。|

## 5. Field Test 草案

Batch8-C Apply 后应进入 Navigation Field Test。建议 18 个 case：14 strict + 4 observation。

### 5.1 Strict cases

|#|query|expected primary Guide|expected Wiki|观察点|
|---:|---|---|---|---|
|1|手机地图打不开，今天还要外出去取水，出发前要写什么？|DG-0863|navigation-outbound-route-card-001; navigation-return-route-plan-001|不应由 communication 或 electronics 主导。|
|2|GNSS 一直没有 fix，还能继续往前走吗？|DG-0864 / DG-0867|navigation-gnss-no-fix-downgrade-001|应给停止/降级，不应只解释 GPS 原理。|
|3|坐标只有一次读数，能不能发给队友当集合点？|DG-0867|navigation-position-error-check-001; navigation-coordinate-record-format-001|应要求复查和地标。|
|4|外出前怎么规划回营地的路线？|DG-0863|navigation-return-route-plan-001|返回路线应主导，evacuation 只能补充。|
|5|走着走着不知道自己在哪了，先做什么？|DG-0864|navigation-lost-stop-and-last-known-point-001|应先停止深入。|
|6|离目标很近但快到折返时间了，要不要继续？|DG-0863|navigation-time-distance-turnback-line-001|应遵守折返线。|
|7|路线检查点应该怎么编号？|DG-0866|navigation-route-checkpoint-numbering-001|检查点 evidence 应进入。|
|8|检查点错过通信窗口，留守方先怎么处理？|DG-0866|navigation-missed-checkpoint-response-001; communication-position-report-navigation-fields-001|communication 可补充，不应完全抢主位。|
|9|洪水后原来的路还能不能走？|DG-0865|navigation-flood-road-stop-line-001; evacuation-route-passability-review-001|geography/safety 不应替代导航路线判断。|
|10|桥边有积水和暗流，要不要硬过去？|DG-0865|navigation-flood-road-stop-line-001; evacuation-route-go-no-go-line-001|必须有停止线。|
|11|夜里路线标记看不清，但离营地不远，要不要继续？|DG-0863 / DG-0865|navigation-night-movement-stop-line-001|不应被 shelter/clothing 主导。|
|12|FT 记录了一段 track，怎么把它变成返回路线？|DG-0870|navigation-track-to-return-summary-001; navigation-gps-track-minimum-fields-001|FT track 应进入 evidence。|
|13|FT track 中间跳了一大段，还能按它原路返回吗？|DG-0870|navigation-track-drift-gap-check-001|应提示漂移/断点停止线。|
|14|出门巡查要记录哪些 path point？|DG-0870|navigation-task-route-record-001; navigation-poi-minimum-fields-001|任务路线记录应主导。|

### 5.2 Observation cases

|#|query|观察目标|
|---:|---|---|
|15|洪水后要撤离，路线不是最短那条怎么办？|evacuation 和 navigation 是否协同，路线评分是否进入。|
|16|队伍分散后有人只报了“我在河边”，够不够？|communication 是否补充位置报告，navigation 是否提供字段。|
|17|纸质地图和手机上的路线不一样，听谁的？|地图版本、离线地图、纸质降级是否出现。|
|18|山坡边有落石但绕路很远，还能走吗？|危险地形是否能进入 evidence；不要求 v0.1 完全 stable。|

## 6. 风险边界

### 6.1 本批不处理内容

本阶段不处理：

- Retrieval profile。
- selector。
- ranking。
- top_k。
- Prompt。
- FT-02 导航代码。
- PocketBase 同步。
- 测试实现。

原因：Batch8-B 的根因是 navigation 知识链缺少行动入口和 evidence，而不是 Retrieval 调参。应先建立 Guide -> Wiki -> Action -> Safety boundary -> Fallback -> Record/check 的基础链，再通过 Field Test 判断是否需要 profile。

### 6.2 Apply 风险

|风险|说明|计划内控制|
|---|---|---|
|Guide 过碎|DG-0868 / DG-0869 若第一批加入，会让夜间和危险地形抢主入口。|第一批只做 6 个 Guide，把夜间/危险地形作为 DG-0863 / DG-0865 的停止线。|
|硬关联 maps 旧 Wiki|maps 有 16 篇断链，但不是每篇都适合 navigation v0.1。|只精准补与路线、返回、洪水、夜间、地标直接相关的旧 Wiki。|
|evacuation 抢主位|撤离路线与普通外出路线相近。|DG-0865 限定为路线检查；是否撤离仍由 Evacuation 决策 Guide 主导。|
|communication 抢主位|检查点和位置报告容易被通信主导。|DG-0866 主导路线/检查点，communication 只补报告字段和窗口。|
|FT 软件领先知识|FT 已有 track/session/path point，但知识库缺解释。|DG-0870 和 track Wiki 先补行动解释，不改代码。|
|Kiwix 背景越权|导航术语可能被百科解释覆盖。|Guide 必须提供步骤、停止线、fallback 和记录字段。|

### 6.3 Apply 后验收建议

Batch8-C Apply 后建议运行：

- `python3 tools/audit_wiki.py`
- `python3 tools/build_guides.py`
- `python3 scripts/audit_guides.py`

验收目标：

- Wiki audit 0/0/0。
- Guide audit 0/0/0。
- Guide-Wiki 单边关系 0。
- 无效 Guide ID 0。
- 无效 Wiki slug 0。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不修改 query profile。
- 不修改 top_k。
- 不修改 selector limit。
- 不修改 ranking。
- 不修改 fallback。
- 后续进入 Navigation Field Test。

## 7. Batch8-C 建议

建议 Batch8-C 执行范围：

1. 新增 32 篇 Navigation v0.1 Wiki。
2. 新增 6 个 Guide：DG-0863、DG-0864、DG-0865、DG-0866、DG-0867、DG-0870。
3. 建立新增 Guide-Wiki 双向关系。
4. 精准补少量 existing maps 断链，不做批量硬关联。
5. 更新 guide_index / emergency_guides。
6. 不改 Retrieval、Prompt、profile、top_k、selector limit、ranking、fallback。
7. Apply 后进入 Batch8-D Navigation Field Test。

结论：Navigation v0.1 第一阶段应从“地图资料”升级为“个人定位和返回行动链”。首批 Apply 不追求团队搜索、复杂地图系统或 FT-02 深度融合；先确保一个人能出发前写路线、途中判断迷路、无 fix 时降级、用 track 复盘并形成可返回的记录。
