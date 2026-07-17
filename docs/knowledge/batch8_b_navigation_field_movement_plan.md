# Batch8-B Navigation & Field Movement Coverage Planning

生成日期：2026-07-16

本报告只做 Navigation / Maps / Field Movement 覆盖分析与下一阶段规划。未修改 Wiki、Guide、Guide-Wiki、Retrieval、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或测试。

参考：`docs/knowledge/knowledge_coverage_map_v0_3.md`

核心判断：LanternBox 当前不是完全没有导航知识，而是“地图资料、撤离路线、通信报平安、Field Terminal 轨迹记录”分散存在，尚未形成独立的现场行动能力链。下一阶段应从“地图资料”升级为“个人能定位、能返回、团队能知道去哪找”的 Navigation v0.1。

## 1. 当前覆盖

### 1.1 目录审查

实际存在目录：

|目标目录|实际情况|说明|
|---|---|---|
|`wiki_import/maps`|存在，22 篇 Wiki|导航、地图、地形和环境观察的主体目录。|
|`wiki_import/geography`|不存在|地理/地形内容目前归入 `maps`。|
|`wiki_import/communication`|不存在|项目实际使用 `wiki_import/comms`，64 篇 Wiki。|
|`wiki_import/evacuation`|存在，21 篇 Wiki|撤离、集合点、路线探查相关。|
|`wiki_import/outdoor`|不存在|野外移动内容散落在 maps / evacuation / security / comms。|

### 1.2 Wiki / Guide 统计

|范围|Wiki|有 guide_links|无 guide_links|说明|
|---|---:|---:|---:|---|
|maps|22|6|16|最大断链区。|
|evacuation|21|21|0|撤离 Wiki 链接完整，但多为 shelter / evacuation 语义。|
|comms|64|64|0|通信链完整，但导航只是报位置/检查点辅助。|
|合计|107|91|16|无链接主要集中在 maps。|

导航相邻 Guide：

|范围|数量|带 related_wiki|说明|
|---|---:|---:|---|
|evacuation 路线/集合/返回相关 Guide|57|少量|`DG-0198` 至 `DG-0203` 等存在，但多缺 `related_wiki`。|
|comms 位置/检查点/外出联络相关 Guide|21|部分|`DG-0856`、`DG-0860` 等用于通信链，不是导航主入口。|
|dedicated navigation/maps Guide|0|0|当前没有 `data/guides/maps` 或 navigation 目录。|

典型断链：

- `navigation-offline-map-maintenance-001`：有 Wiki，无 guide_links。
- `navigation-return-route-marking-001`：有 Wiki，无 guide_links。
- `navigation-flood-terrain-risk-001`：有 Wiki，无 guide_links。
- `navigation-night-route-risk-001`：有 Wiki，无 guide_links。
- `DG-0200 路线侦察小队规则`：Guide 存在，related_wiki 为空。
- `DG-0201 路线颜色标记`：Guide 存在，related_wiki 为空。
- `DG-0202 道路风险分级`：Guide 存在，related_wiki 为空。
- `DG-0203 返回路线保障`：Guide 存在，related_wiki 为空。

### 1.3 能力覆盖表

|能力|Wiki|Guide|Evidence|状态|
|---|---:|---:|---|---|
|GNSS / GPS 定位|1 个明确 GPS 地图包 Wiki；无 GNSS 专项 Wiki|0 个 dedicated Guide|无确定 evidence 链|Red|
|坐标记录|0 个明确坐标 Wiki|0 个 dedicated Guide|无|Red|
|离线地图|3 个明确 Wiki|`DG-0417`、`DG-0198` 可相邻支撑|部分 evidence；maps 维护 Wiki 未入 Guide|Yellow|
|纸质地图|多篇 maps / evacuation / comms 提及|`DG-0198` 纸质地图标注|有入口，但 related_wiki 指向不精准|Yellow|
|路线记录|maps / evacuation 多篇|`DG-0200`、`DG-0201`、`DG-0202`、`DG-0203`|Guide 存在但 related_wiki 多为空|Yellow / Red|
|轨迹记录|知识库无明确 Wiki|FT-01 软件已有 path_points|知识侧缺失|Red|
|返回路线|`navigation-return-route-marking-001` 等存在|`DG-0203`、`DG-0589`|Wiki/Guide 未完整双向承接|Yellow|
|检查点|通信中较多，地图侧弱|`DG-0856`、`DG-0860`、`DG-0642`|偏通信/撤离，不是 navigation 主入口|Yellow|
|风险路线判断|洪水、滑坡、夜间、危险区 Wiki 存在|`DG-0202` 等|Guide 缺 related_wiki|Yellow / Red|

## 2. Navigation 能力模型

### NAV-01 定位

当前状态：Red。

已有：

- Field Terminal 代码已有 GNSS FIX、base position、path point。
- `communication-offline-map-package-001` 提到 GPS / 离线地图包。

缺口：

- GNSS 基础 Wiki。
- 坐标记录格式。
- 定位误差和多次读数。
- 无信号 / 弱信号降级。
- 坐标纸质保存与团队交接。

判断：必须优先补。没有定位，路线、检查点、轨迹和团队同步都容易变成口头描述。

### NAV-02 航向与移动判断

当前状态：Yellow。

已有：

- `navigation-map-orientation-basics-index-001`
- `navigation-map-scale-001`
- 地标、方向、夜间风险等 Wiki。

缺口：

- 指南针基础行动卡。
- 方向偏差复查。
- 距离估算和时间估算。
- 迷路停止线。
- 原路返回和最近确定点规则。

判断：已有解释层，缺行动入口和 Field Test。

### NAV-03 离线地图

当前状态：Yellow。

已有：

- `navigation-offline-map-maintenance-001`
- `communication-offline-map-package-001`
- `DG-0417 离线地图纸卡`
- `DG-0198 纸质地图标注`

缺口：

- 地图文件管理与版本。
- 图层和覆盖范围。
- POI / 水源 / 避难点 / 危险点格式。
- 地形图基础。
- 离线地图失效降级。

判断：资料层较好，但 maps Wiki 没有稳定进入 navigation evidence。

### NAV-04 路线规划

当前状态：Yellow / Red。

已有：

- `DG-0071 撤离路线选择：不是越短越好`
- `DG-0200 路线侦察小队规则`
- `DG-0201 路线颜色标记`
- `DG-0202 道路风险分级`
- `DG-0203 返回路线保障`
- `navigation-return-route-marking-001`
- `navigation-route-difficulty-record-001`

缺口：

- 个人去程路线。
- 返回路线最小字段。
- 备用路线和撤回线。
- 检查点编号。
- 路线评分表。

判断：Guide 名义上存在，但 related_wiki 多为空，不能算完整能力链。

### NAV-05 风险地形判断

当前状态：Yellow。

已有：

- `navigation-flood-terrain-risk-001`
- `navigation-landslide-risk-001`
- `navigation-night-route-risk-001`
- `navigation-terrain-height-001`
- `navigation-water-level-change-001`
- `navigation-danger-zone-marking-001`

缺口：

- 洪水后路段通行停止线。
- 山地坡度和崩塌绕行。
- 夜间移动风险分级。
- 植被遮挡、低洼、泥泞、积水组合判断。
- Field Test 验证。

判断：Wiki 有，但行动入口和 Retrieval 主位不稳。

### NAV-06 轨迹记录

当前状态：Red。

已有：

- FT-01 `/lanternbox/tracks/path_points.jsonl`
- FT-01 auto track 30 秒。
- FT-01 track viewer / relative plot / compass to BASE or selected point。

缺口：

- GPS track 知识条目。
- 时间戳、session、path point 字段解释。
- 路径保存、导出和纸质摘要。
- 共享给团队后的读法。
- 轨迹异常 / 漂移 / 断点处理。

判断：软件能力领先于知识库。Batch8-C 必须补知识解释和行动 Guide。

### NAV-07 团队移动

当前状态：Yellow。

已有：

- communication field 中有检查点、通信窗口和分散报平安。
- `DG-0856 野外建立临时通信链路`
- `DG-0860 通信信息记录与交接`
- `DG-0642 集合点失效后的备用点`
- `DG-0643 探路超时后的留守规则`

缺口：

- 集合点 + 路线 + 检查点的统一任务表。
- 搜索范围和扇区。
- 失联处理的 navigation 主入口。
- 任务路线和返回窗口。
- 团队同步、纸图同步、FT 数据同步。

判断：通信有一半，导航缺另一半。

## 3. Field Terminal 对接分析

已有 FT-01 / FT-02 相关能力：

- GNSS UART 与 FIX 状态。
- SD 记录。
- session start / stop。
- base position。
- path point。
- auto track。
- `/lanternbox/tracks/path_points.jsonl`。
- Navigation UI：session list、point list、overview、relative position plot、north-fixed compass navigation to BASE or selected path point。

缺口：

- 地图显示。
- 路线缓存。
- POI。
- 风险标记。
- 任务导航。
- 团队同步。

|FT 能力|软件需求|知识需求|
|---|---|---|
|GNSS FIX|显示 fix / no fix、卫星数、时间源、精度等级|GNSS 定位误差、弱信号降级、不得把单次读数当绝对位置。|
|Base position|保存营地 / 住所 / 集合点基准|基地点命名、纸质备份、错误基地点覆盖防护。|
|Waypoint / path point|记录点位、备注、session、时间|点位最小字段：时间、目的、地标、风险、是否可共享。|
|Track / auto track|按间隔记录移动路径|轨迹漂移、断点、回放、返回路线摘要。|
|SD 记录|文件可读、路径固定、导出同步|SD 损坏、离线备份、纸质摘要降级。|
|Navigation UI|列表、地图、指南、返回基地|用户行动 Guide：如何在迷路、夜间、洪水后使用这些页面。|
|地图显示|尚需实现地图底图 / 离线瓦片 / 简化草图|离线地图版本、覆盖范围、不可用时手绘草图。|
|路线缓存|尚需实现任务路线文件|路线编号、检查点、备用路线、撤回线。|
|POI / 风险标记|尚需实现资源点 / 危险点图层|POI 分类、危险点有效期、复查和废止。|
|任务导航|尚需实现任务目标和检查点状态|任务路线卡、最晚返回时间、异常上报。|
|团队同步|已有 Core sync 服务基础，导航侧未闭环|出发前同步、返回后合并、冲突记录、谁可信。|

## 4. Navigation Wiki 规划

规划 36 篇 Wiki。分类使用现有正式分类，避免新增 category；目录建议后续按实际项目决定，优先 `maps`，通信和撤离联动条目可分别归入 `comms` / `evacuation`。

|#|方向|slug|title|category|priority|risk_level|summary|intended Guide|
|---:|---|---|---|---|---|---|---|---|
|1|maps|navigation-gnss-basics-001|GNSS 定位基础和限制|地图地形与环境监测|P0|caution|解释 GNSS/FIX/卫星数/时间源的最小含义，避免把无 fix 或漂移位置当真实位置。|DG-0867|
|2|maps|navigation-gnss-no-fix-downgrade-001|GNSS 无信号时的降级定位|地图地形与环境监测|P0|high|无 fix 时用最近确定点、地标、方向和时间估算降级，不继续深入陌生路线。|DG-0864|
|3|maps|navigation-coordinate-record-format-001|坐标记录最小字段|地图地形与环境监测|P0|caution|记录坐标、时间、来源、精度、地标和记录人，支持纸质交接。|DG-0867|
|4|maps|navigation-coordinate-paper-backup-001|坐标纸质备份和交接|地图地形与环境监测|P1|normal|把基地点、水源点、集合点和危险点坐标写到纸面并定期复核。|DG-0867 / DG-0866|
|5|maps|navigation-position-error-check-001|定位误差复查|地图地形与环境监测|P0|caution|用多次读数、地标和移动方向检查位置漂移，避免被单点误导。|DG-0867|
|6|maps|navigation-compass-orientation-basics-001|指南针和地图定向基础|地图地形与环境监测|P0|caution|用指南针、地标和地图北向确认行动方向。|DG-0863|
|7|maps|navigation-bearing-distance-estimate-001|方位和距离估算|地图地形与环境监测|P0|caution|用步数、时间、地标间距和方位估算下一检查点距离。|DG-0863 / DG-0864|
|8|maps|navigation-time-distance-turnback-line-001|时间距离和折返线|地图地形与环境监测|P0|high|出发前设最晚折返时间和距离，不因目标接近而透支返回能力。|DG-0863|
|9|maps|navigation-lost-stop-and-last-known-point-001|迷路后的停止和最近确定点|地图地形与环境监测|P0|high|迷路时停止扩散、回到最近确定点、记录最后确定位置。|DG-0864|
|10|maps|navigation-offline-map-version-control-001|离线地图版本管理|地图地形与环境监测|P1|normal|管理地图区域、日期、来源、设备副本和废止版本。|DG-0863|
|11|maps|navigation-offline-map-layer-check-001|离线地图图层检查|地图地形与环境监测|P1|normal|检查道路、地形、水系、避难点和危险点图层是否可离线打开。|DG-0863|
|12|maps|navigation-poi-minimum-fields-001|POI 点位最小字段|地图地形与环境监测|P0|caution|资源点、危险点、集合点记录位置、类型、状态、复查时间和可信度。|DG-0866 / DG-0870|
|13|maps|navigation-paper-map-index-card-001|纸质地图索引卡|地图地形与环境监测|P1|normal|用索引卡记录地图编号、覆盖范围、图例和存放位置。|DG-0863|
|14|maps|navigation-map-symbol-standard-001|地图符号和图例统一|地图地形与环境监测|P0|caution|统一水源、危险、禁行、返回、检查点符号，避免只有绘制者能读懂。|DG-0866|
|15|maps|navigation-outbound-route-card-001|个人外出去程路线卡|地图地形与环境监测|P0|high|记录目的、路线、检查点、风险、最晚返回和留守联系人。|DG-0863|
|16|maps|navigation-return-route-plan-001|返回路线规划|地图地形与环境监测|P0|high|出发前规划原路返回、备用返回、最近安全点和撤回线。|DG-0863 / DG-0864|
|17|maps|navigation-alternate-route-selection-001|备用路线选择|地图地形与环境监测|P0|caution|用风险、距离、地形、通信覆盖和水源决定备用路线。|DG-0865|
|18|maps|navigation-route-checkpoint-numbering-001|路线检查点编号|地图地形与环境监测|P0|caution|按起点、转折点、危险点、补给点和返回点编号，支持团队同步。|DG-0866|
|19|maps|navigation-route-risk-score-001|撤离路线风险评分|地图地形与环境监测|P0|high|用洪水、夜间、坡地、冲突、体力和通信因素评分。|DG-0865|
|20|maps|navigation-route-closure-recheck-001|道路封闭和绕行复查|地图地形与环境监测|P0|high|遇封路、断桥、积水、冲突时停止硬闯，记录绕行或撤回。|DG-0869|
|21|maps|navigation-flood-road-stop-line-001|洪水后道路停止线|地图地形与环境监测|P0|high|判断积水、暗流、桥涵、泥泞和水位变化，不把熟路当安全路。|DG-0869|
|22|maps|navigation-slope-landslide-bypass-001|坡地和滑坡绕行|地图地形与环境监测|P0|high|识别裂缝、落石、湿坡和崩塌边缘，优先绕行。|DG-0869|
|23|maps|navigation-night-movement-stop-line-001|夜间移动停止线|地图地形与环境监测|P0|high|夜间缺照明、标记不连续、队伍疲劳或天气恶化时停止深入。|DG-0868|
|24|maps|navigation-vegetation-visibility-risk-001|植被遮挡和可视线风险|地图地形与环境监测|P1|caution|判断高草、林地、遮挡、可视线和声音距离对路线安全的影响。|DG-0869|
|25|maps|navigation-gps-track-minimum-fields-001|GPS 轨迹最小字段|地图地形与环境监测|P0|caution|解释 track/session/path point/time/source，并说明如何读轨迹。|DG-0870|
|26|maps|navigation-track-drift-gap-check-001|轨迹漂移和断点检查|地图地形与环境监测|P0|caution|识别漂移、跳点、断点和停留点，避免按坏轨迹返回。|DG-0870|
|27|maps|navigation-track-to-return-summary-001|轨迹转返回摘要|地图地形与环境监测|P0|caution|把轨迹简化为可复述的地标、转折点、耗时和危险点。|DG-0864 / DG-0870|
|28|maps|navigation-track-share-team-001|轨迹共享给团队|地图地形与环境监测|P1|normal|返回后把轨迹同步成团队可读路线和风险记录。|DG-0870 / DG-0866|
|29|maps|navigation-team-rally-point-rules-001|团队集合点规则|地图地形与环境监测|P0|caution|规定主集合点、备用点、失效条件、变更记录和人员确认。|DG-0866|
|30|maps|navigation-search-area-boundary-001|搜索范围边界|地图地形与环境监测|P0|high|外出找人或找物时划定搜索边界、返回时间和停止线。|DG-0866|
|31|maps|navigation-missed-checkpoint-response-001|错过检查点后的处理|地图地形与环境监测|P0|high|未按时到检查点时留守方和外出方的等待、回撤、通信和搜索规则。|DG-0866|
|32|maps|navigation-task-route-record-001|任务路线记录|地图地形与环境监测|P0|caution|把取水、巡查、采集和转移任务写成可复核路线。|DG-0870|
|33|evacuation|evacuation-route-passability-review-001|撤离路线通行复查|避难转移|P0|high|撤离前按水、火、坍塌、冲突、照明和人员状态复查路线。|DG-0865|
|34|evacuation|evacuation-route-go-no-go-line-001|撤离路线继续或停止线|避难转移|P0|high|明确何时继续、绕行、等待或取消撤离。|DG-0865|
|35|communication|communication-position-report-navigation-fields-001|位置报告的导航字段|通讯|P0|caution|报位置时包含坐标/地标/方向/检查点/返回路线和下一窗口。|DG-0866|
|36|communication|communication-navigation-sync-handover-001|导航记录同步交接|通讯|P1|normal|FT、纸图、通信日志之间同步路线、点位和风险变化。|DG-0870 / DG-0866|

## 5. Guide 候选规划

最多 8 个。Guide 必须是行动入口，不是百科入口。以下只规划，不生成。

### DG-0863 个人外出路线规划

- scenario：个人外出取水、巡查、采集或寻找物资，需要在出发前确定路线、返回、检查点和停止线。
- steps：
  1. 写明人员、目的、最晚返回时间。
  2. 在地图或纸上标出起点、目标、去程、返回路线和备用路线。
  3. 设检查点和折返线。
  4. 标出危险点、禁行点、通信窗口。
  5. 出发前交给留守人。
- check：路线可读；返回线明确；至少一个备用路线；留守人知道何时处理超时。
- stop_or_escalate：无地图、无返回路线、天气恶化、夜间照明不足、人员体力不足、通信窗口不可用。
- fallback：没有地图时用手绘草图、地标顺序、步数和时间；不能确认返回时缩短任务距离。
- related_wiki：navigation-outbound-route-card-001、navigation-return-route-plan-001、navigation-time-distance-turnback-line-001、navigation-offline-map-version-control-001。

### DG-0864 迷路后的定位与返回

- scenario：外出后方向不确定、路线标记丢失、GNSS 无 fix 或无法确认当前位置。
- steps：
  1. 停止继续深入。
  2. 找最近确定点、最后检查点或明显地标。
  3. 比对方向、时间、轨迹和地形。
  4. 优先返回最近确定点，不开新路。
  5. 记录迷路时间和最后可信位置。
- check：最近确定点明确；队伍人数完整；体力和光照足够；返回路线可复述。
- stop_or_escalate：两个连续节点无法确认、天黑、天气恶化、有人受伤、GNSS 漂移明显。
- fallback：留在安全可见位置，用哨声/灯光/通信报位置；无通信时按原定等待规则。
- related_wiki：navigation-lost-stop-and-last-known-point-001、navigation-gnss-no-fix-downgrade-001、navigation-track-to-return-summary-001、navigation-position-error-check-001。

### DG-0865 撤离路线检查

- scenario：需要判断某条撤离路线是否还能走，尤其在洪水、坍塌、冲突或夜间后。
- steps：
  1. 对路线做风险评分。
  2. 检查水、火、坍塌、冲突、桥涵、照明、人员状态。
  3. 设置继续、绕行、等待和取消条件。
  4. 记录当前版本和复查人。
- check：路线风险低于阈值；备用路线存在；危险点有标记；返回路线未被切断。
- stop_or_escalate：水流不明、桥面受损、坡面开裂、路线被冲突/火/烟切断。
- fallback：改用备用路线；缩短移动距离；转入临时避难点等待。
- related_wiki：navigation-route-risk-score-001、evacuation-route-passability-review-001、evacuation-route-go-no-go-line-001、navigation-route-closure-recheck-001。

### DG-0866 检查点记录与团队集合

- scenario：团队外出、分组、巡查或撤离，需要用检查点、集合点和位置报告同步状态。
- steps：
  1. 编号检查点。
  2. 规定到达时间、最晚报告时间和下一动作。
  3. 定义主集合点、备用点和失效条件。
  4. 位置报告包含坐标/地标/方向/风险/下一窗口。
  5. 留守方记录超时和响应。
- check：检查点编号清楚；集合点至少两级；通信和纸图一致；失联规则明确。
- stop_or_escalate：检查点错过、集合点失效、队伍分散、天气或冲突扩大。
- fallback：回到上一个检查点；转备用集合点；用纸条/哨声/灯光信号降级。
- related_wiki：navigation-route-checkpoint-numbering-001、navigation-team-rally-point-rules-001、navigation-missed-checkpoint-response-001、communication-position-report-navigation-fields-001。

### DG-0867 GNSS 坐标记录

- scenario：使用 FT、手机或 GNSS 设备记录水源、风险点、集合点或路线节点。
- steps：
  1. 确认 fix 状态。
  2. 记录坐标、时间、设备、精度/卫星状态和地标描述。
  3. 做第二次读数或地标复核。
  4. 写入纸质备份或同步队伍。
- check：有 fix；坐标有时间；地标可识别；纸质备份存在。
- stop_or_escalate：无 fix、位置跳变、无法说明地标、设备电量不足。
- fallback：用地标、方向、距离和手绘草图替代坐标。
- related_wiki：navigation-gnss-basics-001、navigation-coordinate-record-format-001、navigation-coordinate-paper-backup-001、navigation-position-error-check-001。

### DG-0868 夜间移动风险判断

- scenario：天黑后仍考虑外出、返回、撤离或继续任务。
- steps：
  1. 判断是否必须夜间移动。
  2. 检查照明、路线熟悉度、标记连续性、人员体力和天气。
  3. 缩短路线，只走已确认路线。
  4. 设置停止和等待点。
- check：照明可持续；返回路线清楚；队伍不分散；危险地形已避开。
- stop_or_escalate：照明不足、标记断裂、队伍疲劳、下雨/大风、路线涉及水边/坡地/废墟。
- fallback：原地等待到天亮；转入最近安全点；用通信报告等待位置。
- related_wiki：navigation-night-movement-stop-line-001、navigation-night-route-risk-001、navigation-return-route-plan-001。

### DG-0869 危险地形绕行

- scenario：路线遇到洪水、坡地、滑坡、坍塌、泥泞或植被遮挡。
- steps：
  1. 停在安全观察点。
  2. 判断危险类型和扩展方向。
  3. 标记禁行边界。
  4. 选择绕行或撤回。
  5. 更新地图和团队记录。
- check：危险边界可见；绕行不进入更高风险；返回路线可用。
- stop_or_escalate：水流不明、坡面裂缝、落石、泥水上涨、视线不足。
- fallback：撤回最近确定点；等待水位下降；转通信求助或团队复核。
- related_wiki：navigation-flood-road-stop-line-001、navigation-slope-landslide-bypass-001、navigation-vegetation-visibility-risk-001、navigation-route-closure-recheck-001。

### DG-0870 野外任务轨迹记录

- scenario：使用 FT / 手机 / GNSS 记录巡查、取水、采集或探路轨迹，并回传给团队。
- steps：
  1. 开始 session。
  2. 记录 base 和关键 path point。
  3. 对危险点、资源点、转折点做备注。
  4. 返回后检查漂移和断点。
  5. 转成纸质路线摘要和团队可读记录。
- check：session 有起止；关键点有时间；轨迹可回放；风险点已标记。
- stop_or_escalate：无 fix、SD 不可写、轨迹跳点、设备低电、路线与实际不符。
- fallback：手写点位和时间；用地标顺序替代轨迹；回到基地后补录。
- related_wiki：navigation-gps-track-minimum-fields-001、navigation-track-drift-gap-check-001、navigation-track-to-return-summary-001、navigation-track-share-team-001、navigation-task-route-record-001。

## 6. Retrieval 风险预测

|风险 query|可能抢主位|应由谁主导|边界建议|
|---|---|---|---|
|怎么找到回营地路线？|outdoor / evacuation / communication|navigation|若重点是定位、返回、路线节点，navigation 主导；若已经必须撤离，evacuation secondary。|
|洪水后还能不能走这条路？|geography / safety / disaster|navigation + evacuation|若问路线通行性，navigation 主导；若住所不可留或撤离决策，evacuation 主导。|
|检查点没报平安怎么办？|communication / security|communication + navigation|若重点是通信窗口和回执，communication 主导；若重点是去哪找、路线和集合点，navigation 主导。|
|手机没信号怎么定位？|communication / electronics|navigation|无信号通信是 comms；无 GNSS/弱定位的行动降级是 navigation。|
|夜里还要不要走？|security / shelter / clothing|navigation|夜间路线、地形、照明和返回判断由 navigation 主导；保暖/PPE secondary。|
|地图打不开怎么办？|communication / data / electronics|navigation|离线地图使用和纸质降级由 navigation 主导；设备故障由 electronics secondary。|
|外出队失联了去哪找？|security / communication / evacuation|navigation + team movement|搜索范围、最后位置、检查点由 navigation 主导；通信和安全作为约束。|
|撤离路线怎么选？|evacuation|evacuation + navigation|撤离是否发生由 evacuation 主导；路线评分、地形、检查点由 navigation evidence 支撑。|

预计需要的 profile（只判断，不实现）：

- `navigation_return_route_recovery`
- `navigation_gnss_coordinate_record`
- `navigation_route_checkpoint_team`
- `navigation_hazard_terrain_route`
- `navigation_offline_map_fallback`

Batch8-C 不建议一开始就改 Retrieval。应先补 Wiki / Guide / 双向 evidence，再做 Field Test，最后根据 root cause 决定是否需要 profile。

## 7. 版本路线

### Navigation v0.1：个人定位和返回

目标：一个人外出后能定位、记录路线、识别迷路、返回基地。

范围：

- GNSS 基础。
- 坐标记录。
- 离线地图 / 纸质地图。
- 返回路线。
- 迷路停止线。
- 夜间移动停止线。

验收：

- 个人外出路线 Field Test。
- fail=0，danger=0，Kiwix 越权=0。
- safety / fallback / record-check = 100%。

### Navigation v0.2：团队移动和任务路线

目标：小团队能用检查点、集合点、失联处理和任务路线维持外出行动。

范围：

- 检查点编号。
- 团队集合点。
- 搜索范围。
- 失联处理。
- 路线报告。
- 通信联动。

验收：

- 与 Communication v0.2 联合 Field Test。
- 检查点、报平安、返回路线不被单一通信证据覆盖。

### Navigation v0.3：FT-02 深度融合

目标：Field Terminal 的 GNSS / track / waypoint / session 能与知识行动链互相支撑。

范围：

- 地图显示。
- 路线缓存。
- POI。
- 风险标记。
- 任务导航。
- 团队同步。

验收：

- FT 记录能转成 Guide 可解释的 evidence。
- 纸质降级和电子轨迹一致。
- route / track / POI 数据有同步和冲突处理规则。

## 8. Batch8-C 建议

建议选择：E. 混合路线，但严格分阶段执行。

不建议：

- 不先做 FT-02 导航软件。软件已有 GNSS/track 基础，知识链缺口更大。
- 不先改 Retrieval。当前最大问题是 Wiki/Guide 断链和 dedicated navigation Guide 缺失。
- 不只新增 Wiki。没有 Guide 行动入口会继续停留在“地图资料”。

Batch8-C 建议范围：

1. 新增第一批 30-35 篇 Navigation Wiki。
2. 新增 6 个以内 Guide，优先：
   - DG-0863 个人外出路线规划
   - DG-0864 迷路后的定位与返回
   - DG-0865 撤离路线检查
   - DG-0866 检查点记录与团队集合
   - DG-0867 GNSS 坐标记录
   - DG-0870 野外任务轨迹记录
3. 建立 Guide-Wiki 双向关系。
4. 不修改 Retrieval / Prompt / profile / top_k / selector limit / ranking / fallback。
5. Apply 后进入 Navigation Field Test。

Field Test 应覆盖：

- “手机地图打不开，怎么回营地？”
- “GNSS 没 fix，能不能继续走？”
- “洪水后原路还能走吗？”
- “天黑了但离目标很近，要不要继续？”
- “检查点没到，留守队怎么办？”
- “FT 记录了一段轨迹，怎么转成返回路线？”
- “撤离路线不是最短的那条，怎么判断？”
- “坐标只有一次读数，能不能发给队友？”

## 9. 结论

Navigation / Maps / Field Movement 当前处于 Incomplete / Red：

- maps Wiki 有基础，但 22 篇中 16 篇没有 guide_links。
- 路线 Guide 在 evacuation 中存在，但多缺 related_wiki。
- Communication 已具备检查点和报平安的一半能力，但 navigation 主入口缺失。
- FT-01 软件已经有 GNSS、SD、session、waypoint、track 和相对导航能力，知识库没有对应行动解释链。

建议 Batch8-C 从 Navigation v0.1 开始：先让个人能定位、能记录、能判断迷路、能返回，再扩展到团队移动和 FT-02 深度融合。
