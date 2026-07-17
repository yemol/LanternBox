# Batch8-D Navigation Field Test Report

生成时间：2026-07-17T06:31:13.472059+00:00

## 1. 测试范围

本阶段只测试 Batch8-C2 新增 Navigation v0.1 Guide/Wiki 是否稳定进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。

覆盖：个人外出路线规划、迷路返回、GNSS fix / drift、检查点、撤离路线风险、夜间移动、GPS track 复盘，以及 water / communication / outdoor / shelter / security / journal / team 的跨域观察。

## 2. 18 cases 结果

- 用例总数：18
- strict / observation：10 / 8
- pass / partial / fail：18 / 0 / 0
- strict pass / partial / fail：10 / 0 / 0

## 3. Guide / Wiki 命中

- Guide 命中率（strict，含 allowed secondary）：100.0%
- 主 Guide 命中率（strict，仅 expected）：100.0%
- Wiki 全量命中率（strict，全部 expected Wiki）：100.0%
- Wiki 任一命中率（strict，至少一个 expected Wiki）：100.0%
- Guide-Wiki 精准组合率（strict）：100.0%
- navigation 主 Guide 进入率（全部 cases）：77.8%

安全指标：

- safety boundary：100.0%
- fallback：100.0%
- record/check：100.0%
- dangerous suggestion：0
- Kiwix 越权：0

## 4. Case 明细

|case|type|verdict|selected Guide|selected Wiki|profiles|cross domain|root cause|
|---|---|---|---|---|---|---|---|
|navigation_plan_before_foraging|strict|pass|DG-0863 个人外出路线规划、DG-0870 野外任务轨迹记录、DG-0866 检查点记录与团队集合|navigation-outbound-route-card-001 个人外出去程路线卡、navigation-route-checkpoint-numbering-001 路线检查点编号、navigation-return-route-plan-001 返回路线规划、navigation-time-distance-turnback-line-001 时间距离和折返线、navigation-compass-orientation-basics-001 指南针和地图定向基础、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-offline-map-version-control-001 离线地图版本管理、navigation-offline-map-layer-check-001 离线地图图层检查|navigation_route_planning|无|无|
|navigation_lost_return_where|strict|pass|DG-0864 迷路后的定位与返回、DG-0863 个人外出路线规划、DG-0865 撤离路线检查|navigation-lost-stop-and-last-known-point-001 迷路后的停止和最近确定点、navigation-gnss-no-fix-downgrade-001 GNSS 无信号时的降级定位、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-position-error-check-001 定位误差复查、navigation-return-route-plan-001 返回路线规划、navigation-track-to-return-summary-001 轨迹转返回摘要、navigation-return-route-marking-001 返回路线标记、navigation-outbound-route-card-001 个人外出去程路线卡|navigation_lost_return|无|无|
|navigation_gps_drift|strict|pass|DG-0867 GNSS 坐标记录、DG-0638 近距离灯光信号三条规则、DG-0581 手机没信号网络断了：家庭通讯计划和集合留言|navigation-gnss-basics-001 GNSS 定位基础和限制、navigation-coordinate-record-format-001 坐标记录最小字段、navigation-coordinate-paper-backup-001 坐标纸质备份和交接、navigation-position-error-check-001 定位误差复查、navigation-gnss-no-fix-downgrade-001 GNSS 无信号时的降级定位、communication-communication-discipline-001 断供期间通信纪律、communication-communication-failure-001 通信失败时避免盲目寻找、communication-communication-knowledge-001 消息内容为什么要短而明确|无|无|无|
|navigation_gnss_no_fix_record|strict|pass|DG-0867 GNSS 坐标记录、DG-0864 迷路后的定位与返回、DG-0870 野外任务轨迹记录|navigation-gnss-basics-001 GNSS 定位基础和限制、navigation-coordinate-record-format-001 坐标记录最小字段、navigation-coordinate-paper-backup-001 坐标纸质备份和交接、navigation-position-error-check-001 定位误差复查、navigation-gnss-no-fix-downgrade-001 GNSS 无信号时的降级定位、navigation-lost-stop-and-last-known-point-001 迷路后的停止和最近确定点、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-return-route-plan-001 返回路线规划|无|无|无|
|navigation_route_checkpoint_setup|strict|pass|DG-0863 个人外出路线规划、DG-0866 检查点记录与团队集合、DG-0870 野外任务轨迹记录|navigation-outbound-route-card-001 个人外出去程路线卡、navigation-route-checkpoint-numbering-001 路线检查点编号、navigation-return-route-plan-001 返回路线规划、navigation-time-distance-turnback-line-001 时间距离和折返线、navigation-compass-orientation-basics-001 指南针和地图定向基础、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-offline-map-version-control-001 离线地图版本管理、navigation-offline-map-layer-check-001 离线地图图层检查|navigation_route_planning、navigation_checkpoint_management|无|无|
|navigation_evac_route_risk_check|strict|pass|DG-0865 撤离路线检查、DG-0863 个人外出路线规划、DG-0870 野外任务轨迹记录|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|navigation_terrain_risk、navigation_route_planning|无|无|
|navigation_night_movement_route|strict|pass|DG-0865 撤离路线检查、DG-0863 个人外出路线规划、DG-0870 野外任务轨迹记录|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|navigation_terrain_risk、navigation_route_planning|无|无|
|navigation_track_review_after_day|strict|pass|DG-0870 野外任务轨迹记录、DG-0867 GNSS 坐标记录、DG-0864 迷路后的定位与返回|navigation-gps-track-minimum-fields-001 GPS 轨迹最小字段、navigation-track-drift-gap-check-001 轨迹漂移和断点检查、navigation-track-to-return-summary-001 轨迹转返回摘要、navigation-task-route-record-001 任务路线记录、navigation-poi-minimum-fields-001 POI 点位最小字段、navigation-gnss-basics-001 GNSS 定位基础和限制、navigation-coordinate-record-format-001 坐标记录最小字段、navigation-coordinate-paper-backup-001 坐标纸质备份和交接|navigation_track_management|无|无|
|navigation_safe_route_return_next_time|strict|pass|DG-0863 个人外出路线规划、DG-0864 迷路后的定位与返回、DG-0870 野外任务轨迹记录|navigation-outbound-route-card-001 个人外出去程路线卡、navigation-route-checkpoint-numbering-001 路线检查点编号、navigation-return-route-plan-001 返回路线规划、navigation-time-distance-turnback-line-001 时间距离和折返线、navigation-compass-orientation-basics-001 指南针和地图定向基础、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-offline-map-version-control-001 离线地图版本管理、navigation-offline-map-layer-check-001 离线地图图层检查|navigation_route_planning、navigation_lost_return|无|无|
|navigation_flood_old_road|strict|pass|DG-0865 撤离路线检查、DG-0866 检查点记录与团队集合、DG-0870 野外任务轨迹记录|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|navigation_terrain_risk|无|无|
|navigation_observe_nearby_water_source|observation|pass|DG-0827 先在水源附近设一个临时取水点、DG-0678 粪便远离水源、DG-0553 可疑水源：过滤、煮沸和禁用判断|water-container-labeling-004 储水容器标签与编号、water-safe-collection-007 安全取水的基本步骤、water-source-judgement-002 水源判断的基本标准、water-storage-location-006 储水位置选择与避险、agriculture-animal-feed-001 饲料短缺时的替代边界、agriculture-animal-feed-002 养殖记录和饲料消耗、agriculture-animal-food-001 动物性食物处理卫生、agriculture-bite-risk-001 动物咬伤和抓伤感染|无|off_domain_primary、navigation_vs_water|无|
|navigation_observe_radio_lost_find_team|observation|pass|DG-0858 无线电通信前检查、DG-0855 通信设备无法连接排查、DG-0195 无线电通话纪律|communication-antenna-wet-weather-stop-001 雨天潮湿天线停止线、communication-antenna-connection-check-001 天线连接检查、communication-radio-listen-before-transmit-001 无线电发射前先监听、communication-radio-basic-terms-001 无线电通信基本原理、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界、communication-radio-channel-plan-minimum-001 最小频道计划、communication-radio-range-line-of-sight-001 无线电距离和遮挡判断|无|off_domain_primary、navigation_vs_communication|无|
|navigation_observe_mountain_road_danger|observation|pass|DG-0865 撤离路线检查、DG-0866 检查点记录与团队集合、DG-0870 野外任务轨迹记录|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|navigation_terrain_risk|无|无|
|navigation_observe_night_camp_site|observation|pass|DG-0865 撤离路线检查、DG-0866 检查点记录与团队集合、DG-0870 野外任务轨迹记录|navigation-route-risk-score-001 撤离路线风险评分、navigation-flood-terrain-risk-001 洪水风险地形、navigation-alternate-route-selection-001 备用路线选择、navigation-route-closure-recheck-001 道路封闭和绕行复查、navigation-flood-road-stop-line-001 洪水后道路停止线、navigation-night-route-risk-001 夜间路线风险、navigation-night-movement-stop-line-001 夜间移动停止线、navigation-slope-landslide-bypass-001 坡地和滑坡绕行|navigation_terrain_risk|无|无|
|navigation_observe_danger_zone_record|observation|pass|DG-0208 危险区域标记、DG-0021 发热：降温、补水、记录、DG-0707 判断外部信息可信度|medical-child-elder-dehydration-risk-001 儿童老人脱水风险、medical-common-infectious-disease-index-001 常见传染病基础索引、medical-high-fever-consciousness-risk-001 高热伴意识异常风险、safety-safety-knowledge-003 真假信息的交叉验证|无|off_domain_primary、navigation_vs_security|无|
|navigation_observe_phone_map_unavailable|observation|pass|DG-0863 个人外出路线规划、DG-0561 手机低电量：保通信模式、DG-0581 手机没信号网络断了：家庭通讯计划和集合留言|navigation-outbound-route-card-001 个人外出去程路线卡、navigation-route-checkpoint-numbering-001 路线检查点编号、navigation-return-route-plan-001 返回路线规划、navigation-time-distance-turnback-line-001 时间距离和折返线、navigation-compass-orientation-basics-001 指南针和地图定向基础、navigation-bearing-distance-estimate-001 方位和距离估算、navigation-offline-map-version-control-001 离线地图版本管理、navigation-offline-map-layer-check-001 离线地图图层检查|无|无|无|
|navigation_observe_past_routes|observation|pass|DG-0589 外出探路：返回时间和路线标记、DG-0601 夜间取水前的路线和容器检查、DG-0634 夜间低亮厕所路线|navigation-map-orientation-basics-index-001 地图与方位基础索引、shelter-route-scouting-002 探路和路线标记原则、food-rationing-priority-001 食物短缺时先给谁吃和怎么分配、food-spoilage-boundary-001 变质食物和怪味食物的停用边界、water-boiling-002 浑浊水沉淀过滤煮沸的顺序、water-boiling-003 煮沸不能解决的污染类型、water-chemical-contamination-001 油膜异味和化学污染的禁用判断、water-drinking-water-001 多人共用饮用水的消耗率判断|无|off_domain_primary|无|
|navigation_observe_team_share_rally|observation|pass|DG-0866 检查点记录与团队集合、DG-0867 GNSS 坐标记录、DG-0864 迷路后的定位与返回|navigation-route-checkpoint-numbering-001 路线检查点编号、navigation-team-rally-point-rules-001 团队集合点规则、navigation-missed-checkpoint-response-001 错过检查点后的处理、navigation-map-symbol-standard-001 地图符号和图例统一、navigation-poi-minimum-fields-001 POI 点位最小字段、communication-position-report-navigation-fields-001 位置报告的导航字段、navigation-coordinate-paper-backup-001 坐标纸质备份和交接、navigation-danger-zone-marking-001 危险区域标记|navigation_checkpoint_management|无|无|

## 5. Cross Domain 分析

- communication 抢主位观察：1
- evacuation 抢主位观察：0
- outdoor 抢主位观察：0
- security 抢主位观察：1
- geography 抢主位观察：0
- weather 抢主位观察：0
- water 抢主位观察：1
- shelter 抢主位观察：0
- team 抢主位观察：0
- journal 抢主位观察：1
- electronics 抢主位观察：0

Cross domain labels：
- off_domain_primary：4
- navigation_vs_water：1
- navigation_vs_communication：1
- navigation_vs_security：1

## 6. Navigation Evidence 稳定性

- Navigation 主域 Guide 作为 top evidence 的比例：77.8%。
- Strict cases 中 Guide 命中率：100.0%。
- Strict cases 中 Wiki 全量命中率：100.0%。
- Guide-Wiki 精准组合率：100.0%。
- 本阶段只记录命中表现，不根据结果调整 profile、selector、ranking 或知识内容。

## 7. 是否需要 profile

暂不建议新增 profile；可将 Navigation v0.1 作为 stable candidate 继续观察。

## 8. 是否进入 Navigation Retrieval Root Cause Review

建议进入 Batch8-E Navigation Retrieval Root Cause Review。原因：存在 strict partial/fail 或 observation cross-domain 信号，需要判断是 profile 缺口、selector/ranking 问题、Guide 设计问题还是合理跨域。

## 9. 逐条复盘

### navigation_plan_before_foraging

- query：出去采集物资前应该怎么规划路线？
- 类型：strict
- focus：个人外出路线规划，路线卡、检查点和返回线应进入 evidence。
- verdict：pass
- expected Guide：DG-0863
- allowed secondary：无
- selected Guide：DG-0863、DG-0870、DG-0866
- expected Wiki：navigation-outbound-route-card-001、navigation-route-checkpoint-numbering-001
- selected Wiki：navigation-outbound-route-card-001、navigation-route-checkpoint-numbering-001、navigation-return-route-plan-001、navigation-time-distance-turnback-line-001、navigation-compass-orientation-basics-001、navigation-bearing-distance-estimate-001、navigation-offline-map-version-control-001、navigation-offline-map-layer-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_lost_return_where

- query：走出去以后迷路了，不知道回哪里怎么办？
- 类型：strict
- focus：迷路后停止扩散、最近确定点和返回路线。
- verdict：pass
- expected Guide：DG-0864
- allowed secondary：无
- selected Guide：DG-0864、DG-0863、DG-0865
- expected Wiki：navigation-lost-stop-and-last-known-point-001、navigation-return-route-plan-001
- selected Wiki：navigation-lost-stop-and-last-known-point-001、navigation-gnss-no-fix-downgrade-001、navigation-bearing-distance-estimate-001、navigation-position-error-check-001、navigation-return-route-plan-001、navigation-track-to-return-summary-001、navigation-return-route-marking-001、navigation-outbound-route-card-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_gps_drift

- query：GPS 有信号，但是位置一直漂移怎么办？
- 类型：strict
- focus：GNSS 坐标记录和定位误差复查。
- verdict：pass
- expected Guide：DG-0867
- allowed secondary：无
- selected Guide：DG-0867、DG-0638、DG-0581
- expected Wiki：navigation-gnss-basics-001、navigation-position-error-check-001
- selected Wiki：navigation-gnss-basics-001、navigation-coordinate-record-format-001、navigation-coordinate-paper-backup-001、navigation-position-error-check-001、navigation-gnss-no-fix-downgrade-001、communication-communication-discipline-001、communication-communication-failure-001、communication-communication-knowledge-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_gnss_no_fix_record

- query：GNSS 没有定位成功还能继续记录吗？
- 类型：strict
- focus：无 fix 时的记录停止线和降级定位。
- verdict：pass
- expected Guide：DG-0867
- allowed secondary：DG-0864
- selected Guide：DG-0867、DG-0864、DG-0870
- expected Wiki：navigation-gnss-no-fix-downgrade-001
- selected Wiki：navigation-gnss-basics-001、navigation-coordinate-record-format-001、navigation-coordinate-paper-backup-001、navigation-position-error-check-001、navigation-gnss-no-fix-downgrade-001、navigation-lost-stop-and-last-known-point-001、navigation-bearing-distance-estimate-001、navigation-return-route-plan-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_route_checkpoint_setup

- query：怎么给外出路线设置检查点？
- 类型：strict
- focus：检查点编号、留守记录和路线进度。
- verdict：pass
- expected Guide：DG-0866
- allowed secondary：DG-0863
- selected Guide：DG-0863、DG-0866、DG-0870
- expected Wiki：navigation-route-checkpoint-numbering-001
- selected Wiki：navigation-outbound-route-card-001、navigation-route-checkpoint-numbering-001、navigation-return-route-plan-001、navigation-time-distance-turnback-line-001、navigation-compass-orientation-basics-001、navigation-bearing-distance-estimate-001、navigation-offline-map-version-control-001、navigation-offline-map-layer-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_evac_route_risk_check

- query：撤离路线应该怎么检查风险？
- 类型：strict
- focus：撤离路线风险评分和洪水地形证据。
- verdict：pass
- expected Guide：DG-0865
- allowed secondary：无
- selected Guide：DG-0865、DG-0863、DG-0870
- expected Wiki：navigation-flood-terrain-risk-001、navigation-route-risk-score-001
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_night_movement_route

- query：夜晚必须移动，路线应该怎么判断？
- 类型：strict
- focus：夜间移动路线风险，DG-0868 未创建时 DG-0863/DG-0865 可承接。
- verdict：pass
- expected Guide：DG-0863
- allowed secondary：DG-0865、DG-0868
- selected Guide：DG-0865、DG-0863、DG-0870
- expected Wiki：navigation-night-route-risk-001
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_track_review_after_day

- query：记录了一天 GPS 轨迹，回来后怎么看？
- 类型：strict
- focus：FT/GPS track 字段、漂移检查和返回摘要。
- verdict：pass
- expected Guide：DG-0870
- allowed secondary：无
- selected Guide：DG-0870、DG-0867、DG-0864
- expected Wiki：navigation-gps-track-minimum-fields-001、navigation-track-to-return-summary-001
- selected Wiki：navigation-gps-track-minimum-fields-001、navigation-track-drift-gap-check-001、navigation-track-to-return-summary-001、navigation-task-route-record-001、navigation-poi-minimum-fields-001、navigation-gnss-basics-001、navigation-coordinate-record-format-001、navigation-coordinate-paper-backup-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_safe_route_return_next_time

- query：走过一次安全路线，下次怎么回来？
- 类型：strict
- focus：返回路线、最近确定点和轨迹摘要。
- verdict：pass
- expected Guide：DG-0864
- allowed secondary：DG-0863、DG-0870
- selected Guide：DG-0863、DG-0864、DG-0870
- expected Wiki：navigation-return-route-plan-001
- selected Wiki：navigation-outbound-route-card-001、navigation-route-checkpoint-numbering-001、navigation-return-route-plan-001、navigation-time-distance-turnback-line-001、navigation-compass-orientation-basics-001、navigation-bearing-distance-estimate-001、navigation-offline-map-version-control-001、navigation-offline-map-layer-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_flood_old_road

- query：洪水后原来的道路还能走吗？
- 类型：strict
- focus：洪水后路线通行性，熟路不能默认安全。
- verdict：pass
- expected Guide：DG-0865
- allowed secondary：无
- selected Guide：DG-0865、DG-0866、DG-0870
- expected Wiki：navigation-flood-terrain-risk-001
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_observe_nearby_water_source

- query：附近有没有水源？
- 类型：observation
- focus：观察 navigation vs water：水源寻找可由 water 主导，但 navigation 点位记录应作为补充。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0827、DG-0678、DG-0553
- expected Wiki：无
- selected Wiki：water-container-labeling-004、water-safe-collection-007、water-source-judgement-002、water-storage-location-006、agriculture-animal-feed-001、agriculture-animal-feed-002、agriculture-animal-food-001、agriculture-bite-risk-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、navigation_vs_water
- root cause：无
- failure reasons：无

### navigation_observe_radio_lost_find_team

- query：无线电失联以后怎么找队友？
- 类型：observation
- focus：观察 navigation vs communication：通信故障可主导，但搜索/检查点边界应进入。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0858、DG-0855、DG-0195
- expected Wiki：无
- selected Wiki：communication-antenna-wet-weather-stop-001、communication-antenna-connection-check-001、communication-radio-listen-before-transmit-001、communication-radio-basic-terms-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001、communication-radio-channel-plan-minimum-001、communication-radio-range-line-of-sight-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、navigation_vs_communication
- root cause：无
- failure reasons：无

### navigation_observe_mountain_road_danger

- query：山区道路危险怎么办？
- 类型：observation
- focus：观察 navigation vs outdoor/geography：危险地形路线判断是否进入。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0865、DG-0866、DG-0870
- expected Wiki：无
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_observe_night_camp_site

- query：夜晚露营怎么选择地点？
- 类型：observation
- focus：观察 navigation vs shelter：露营选址可由 shelter 主导，路线/夜间风险为补充。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0865、DG-0866、DG-0870
- expected Wiki：无
- selected Wiki：navigation-route-risk-score-001、navigation-flood-terrain-risk-001、navigation-alternate-route-selection-001、navigation-route-closure-recheck-001、navigation-flood-road-stop-line-001、navigation-night-route-risk-001、navigation-night-movement-stop-line-001、navigation-slope-landslide-bypass-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_observe_danger_zone_record

- query：发现危险区域如何记录？
- 类型：observation
- focus：观察 navigation vs security：危险区记录和地图标记是否进入。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0208、DG-0021、DG-0707
- expected Wiki：无
- selected Wiki：medical-child-elder-dehydration-risk-001、medical-common-infectious-disease-index-001、medical-high-fever-consciousness-risk-001、safety-safety-knowledge-003
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary、navigation_vs_security
- root cause：无
- failure reasons：无

### navigation_observe_phone_map_unavailable

- query：手机地图不能用了怎么办？
- 类型：observation
- focus：观察 navigation vs communication/electronics：地图不可用时离线地图和纸质降级是否进入。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0863、DG-0561、DG-0581
- expected Wiki：无
- selected Wiki：navigation-outbound-route-card-001、navigation-route-checkpoint-numbering-001、navigation-return-route-plan-001、navigation-time-distance-turnback-line-001、navigation-compass-orientation-basics-001、navigation-bearing-distance-estimate-001、navigation-offline-map-version-control-001、navigation-offline-map-layer-check-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### navigation_observe_past_routes

- query：想知道过去走过哪些路线
- 类型：observation
- focus：观察 track vs journal：历史路线记录是否由 track evidence 承接。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0589、DG-0601、DG-0634
- expected Wiki：无
- selected Wiki：navigation-map-orientation-basics-index-001、shelter-route-scouting-002、food-rationing-priority-001、food-spoilage-boundary-001、water-boiling-002、water-boiling-003、water-chemical-contamination-001、water-drinking-water-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：off_domain_primary
- root cause：无
- failure reasons：无

### navigation_observe_team_share_rally

- query：团队成员如何共享集合点
- 类型：observation
- focus：观察 navigation vs team/communication：集合点共享是否兼顾导航字段和团队交接。
- verdict：pass
- expected Guide：无
- allowed secondary：无
- selected Guide：DG-0866、DG-0867、DG-0864
- expected Wiki：无
- selected Wiki：navigation-route-checkpoint-numbering-001、navigation-team-rally-point-rules-001、navigation-missed-checkpoint-response-001、navigation-map-symbol-standard-001、navigation-poi-minimum-fields-001、communication-position-report-navigation-fields-001、navigation-coordinate-paper-backup-001、navigation-danger-zone-marking-001
- guide_hit / wiki_hit / precise：True / True / True
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

## 10. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_navigation_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_navigation_field.py --no-answer
```

边界声明：本批没有修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema 或 PocketBase。
