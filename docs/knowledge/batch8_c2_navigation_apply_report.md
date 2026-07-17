# Batch8-C2 Navigation v0.1 Knowledge Apply Report

生成日期：2026-07-16

## 1. 新增 Wiki 清单

本批新增 32 篇 Navigation v0.1 Wiki，覆盖个人定位、路线规划、迷路处理、返回、检查点、路线风险和 FT track 记录。

|slug|title|category|priority|risk_level|guide_links|
|---|---|---|---|---|---|
|navigation-gnss-basics-001|GNSS 定位基础和限制|地图地形与环境监测|P0|caution|DG-0867|
|navigation-gnss-no-fix-downgrade-001|GNSS 无信号时的降级定位|地图地形与环境监测|P0|high|DG-0864, DG-0867|
|navigation-coordinate-record-format-001|坐标记录最小字段|地图地形与环境监测|P0|caution|DG-0867|
|navigation-coordinate-paper-backup-001|坐标纸质备份和交接|地图地形与环境监测|P1|normal|DG-0867, DG-0866|
|navigation-position-error-check-001|定位误差复查|地图地形与环境监测|P0|caution|DG-0867, DG-0864|
|navigation-compass-orientation-basics-001|指南针和地图定向基础|地图地形与环境监测|P0|caution|DG-0863|
|navigation-bearing-distance-estimate-001|方位和距离估算|地图地形与环境监测|P0|caution|DG-0863, DG-0864|
|navigation-time-distance-turnback-line-001|时间距离和折返线|地图地形与环境监测|P0|high|DG-0863|
|navigation-lost-stop-and-last-known-point-001|迷路后的停止和最近确定点|地图地形与环境监测|P0|high|DG-0864|
|navigation-offline-map-version-control-001|离线地图版本管理|地图地形与环境监测|P1|normal|DG-0863|
|navigation-offline-map-layer-check-001|离线地图图层检查|地图地形与环境监测|P1|normal|DG-0863|
|navigation-poi-minimum-fields-001|POI 点位最小字段|地图地形与环境监测|P0|caution|DG-0866, DG-0870|
|navigation-paper-map-index-card-001|纸质地图索引卡|地图地形与环境监测|P1|normal|DG-0863|
|navigation-map-symbol-standard-001|地图符号和图例统一|地图地形与环境监测|P0|caution|DG-0866|
|navigation-outbound-route-card-001|个人外出去程路线卡|地图地形与环境监测|P0|high|DG-0863|
|navigation-return-route-plan-001|返回路线规划|地图地形与环境监测|P0|high|DG-0863, DG-0864|
|navigation-alternate-route-selection-001|备用路线选择|地图地形与环境监测|P0|caution|DG-0865|
|navigation-route-checkpoint-numbering-001|路线检查点编号|地图地形与环境监测|P0|caution|DG-0866|
|navigation-route-risk-score-001|撤离路线风险评分|地图地形与环境监测|P0|high|DG-0865|
|navigation-route-closure-recheck-001|道路封闭和绕行复查|地图地形与环境监测|P0|high|DG-0865|
|navigation-flood-road-stop-line-001|洪水后道路停止线|地图地形与环境监测|P0|high|DG-0865|
|navigation-slope-landslide-bypass-001|坡地和滑坡绕行|地图地形与环境监测|P0|high|DG-0865|
|navigation-night-movement-stop-line-001|夜间移动停止线|地图地形与环境监测|P0|high|DG-0863, DG-0865|
|navigation-gps-track-minimum-fields-001|GPS 轨迹最小字段|地图地形与环境监测|P0|caution|DG-0870|
|navigation-track-drift-gap-check-001|轨迹漂移和断点检查|地图地形与环境监测|P0|caution|DG-0870|
|navigation-track-to-return-summary-001|轨迹转返回摘要|地图地形与环境监测|P0|caution|DG-0870, DG-0864|
|navigation-team-rally-point-rules-001|团队集合点规则|地图地形与环境监测|P0|caution|DG-0866|
|navigation-missed-checkpoint-response-001|错过检查点后的处理|地图地形与环境监测|P0|high|DG-0866|
|navigation-task-route-record-001|任务路线记录|地图地形与环境监测|P0|caution|DG-0870|
|navigation-evacuation-route-passability-review-001|撤离路线通行复查|避难转移|P0|high|DG-0865|
|navigation-evacuation-route-go-no-go-line-001|撤离路线继续或停止线|避难转移|P0|high|DG-0865|
|communication-position-report-navigation-fields-001|位置报告的导航字段|通讯|P0|caution|DG-0866|

说明：Batch8-C1 原规划中的两篇 `evacuation-*` slug 在 Apply 时按 `tools/audit_wiki.py` 的固定 slug domain contract 调整为 `navigation-evacuation-*`。分类仍为正式 category `避难转移`，语义未改变，避免通过修改审计规则绕过 slug 规范。

## 2. 新增 Guide

本批新增 6 个 Navigation 行动入口 Guide，均放入 `data/guides/navigation/`。

|Guide|title|category|priority|risk_level|目标|
|---|---|---|---|---|---|
|DG-0863|个人外出路线规划|地图地形与环境监测|P0|high|外出前建立路线卡、返回线、检查点和留守记录。|
|DG-0864|迷路后的定位与返回|地图地形与环境监测|P0|high|迷路、无 fix 或路线标记丢失时停止扩散并回到最近确定点。|
|DG-0865|撤离路线检查|避难转移|P0|high|复查路线通行性，建立继续、绕行、等待和取消条件。|
|DG-0866|检查点记录与团队集合|地图地形与环境监测|P0|high|用检查点、集合点和位置报告支撑留守判断和超时处理。|
|DG-0867|GNSS 坐标记录|地图地形与环境监测|P0|caution|记录 fix、坐标、时间、来源、地标和复查状态。|
|DG-0870|野外任务轨迹记录|地图地形与环境监测|P0|caution|把 FT / GNSS track 转成可返回、可交接的任务路线摘要。|

## 3. Evidence chain

本批建立的主链：

个人定位 -> 路线规划 -> 迷路处理 -> 返回 -> Track 记录

|Guide|主 evidence|补充 evidence|边界|
|---|---|---|---|
|DG-0863|navigation-outbound-route-card-001; navigation-return-route-plan-001; navigation-time-distance-turnback-line-001|navigation-compass-orientation-basics-001; navigation-bearing-distance-estimate-001; navigation-offline-map-version-control-001; navigation-offline-map-layer-check-001; navigation-paper-map-index-card-001; navigation-night-movement-stop-line-001; navigation-offline-map-maintenance-001; navigation-return-route-marking-001; navigation-night-route-risk-001|外出路线规划主导；不处理团队搜索和 FT-02 深度同步。|
|DG-0864|navigation-lost-stop-and-last-known-point-001; navigation-gnss-no-fix-downgrade-001|navigation-bearing-distance-estimate-001; navigation-position-error-check-001; navigation-return-route-plan-001; navigation-track-to-return-summary-001; navigation-return-route-marking-001|迷路停止和最近确定点主导；不把通信失联或撤离决策作为主入口。|
|DG-0865|navigation-route-risk-score-001; navigation-evacuation-route-passability-review-001; navigation-evacuation-route-go-no-go-line-001|navigation-alternate-route-selection-001; navigation-route-closure-recheck-001; navigation-flood-road-stop-line-001; navigation-slope-landslide-bypass-001; navigation-night-movement-stop-line-001; existing maps 风险条目|路线通行性主导；是否必须撤离仍由 Evacuation 决策链承担。|
|DG-0866|navigation-route-checkpoint-numbering-001; navigation-missed-checkpoint-response-001; communication-position-report-navigation-fields-001|navigation-team-rally-point-rules-001; navigation-map-symbol-standard-001; navigation-poi-minimum-fields-001; navigation-coordinate-paper-backup-001; navigation-danger-zone-marking-001|检查点/集合点主导；通信只补字段和窗口。|
|DG-0867|navigation-gnss-basics-001; navigation-coordinate-record-format-001; navigation-position-error-check-001|navigation-coordinate-paper-backup-001; navigation-gnss-no-fix-downgrade-001|坐标可信度和记录主导；不处理设备维修。|
|DG-0870|navigation-gps-track-minimum-fields-001; navigation-track-drift-gap-check-001; navigation-track-to-return-summary-001|navigation-task-route-record-001; navigation-poi-minimum-fields-001|FT track 行动化主导；不处理团队云同步和地图渲染。|

双向关系检查：

```text
wiki=912
guides=796
forward_edges=2016
reverse_edges=2016
single_forward_without_reverse=0
single_reverse_without_forward=0
invalid_guide_id=0
invalid_wiki_slug=0
```

## 4. Existing maps 修复

本批只修复 6 篇精准相关 existing maps Wiki 的 `guide_links`，没有批量清空 maps 无链接条目。

|Existing Wiki|新增 guide_links|证据理由|
|---|---|---|
|navigation-offline-map-maintenance-001|DG-0863|支撑外出前离线地图准备。|
|navigation-return-route-marking-001|DG-0863, DG-0864|支撑返回路线标记和迷路后回到确定点。|
|navigation-flood-terrain-risk-001|DG-0865|支撑洪水后路线通行性判断。|
|navigation-night-route-risk-001|DG-0863, DG-0865|支撑夜间外出和路线检查停止线。|
|navigation-route-difficulty-record-001|DG-0865|支撑路线风险评分和通行复查。|
|navigation-danger-zone-marking-001|DG-0865, DG-0866|支撑危险点标记与检查点沟通。|

## 5. PocketBase 同步

同步方式：本地 SQLite upsert / update 到 `pocketbase/pb_data/data.db` 的 `wiki_articles` 表，仅写入本批新增 Wiki 的 metadata 与 content，并同步后续 slug/summary 修正。未修改 PocketBase schema。

```text
同步前 wiki_articles=880
新增 wiki_articles=32
同步后 wiki_articles=912
Markdown Wiki=912
PocketBase wiki_articles=912
PocketBase wiki_categories=24
```

抽样确认本批新增记录存在：

```text
navigation-gnss-basics-001
navigation-track-to-return-summary-001
communication-position-report-navigation-fields-001
navigation-evacuation-route-passability-review-001
```

## 6. 验证结果

已运行：

```text
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

```text
Wiki audit:
Articles: markdown=912 pocketbase=912 categories=24
Issues: errors=0 warnings=0 advisories=0

build_guides:
Generated 796 Guides
Generated 796 Guide Index Items

Guide audit:
Guides: 796
Issues: errors=0 warnings=0 advisories=0
```

## 7. 未修改项

本批未修改：

- Retrieval Pipeline
- Prompt
- query profile
- top_k
- selector limit
- ranking
- fallback
- schema
- tests 逻辑
- PocketBase schema

本批未提前做 Navigation Retrieval / Field Test。下一阶段建议进入 Batch8-D Navigation Field Test，重点验证：

- 手机地图不可用时 DG-0863 是否进入主 evidence。
- GNSS 无 fix / 坐标漂移时 DG-0864 / DG-0867 是否进入。
- 洪水、夜间、封路时 DG-0865 是否主导路线判断。
- 检查点错过时 DG-0866 是否不被 communication 完全抢位。
- FT track 转返回摘要时 DG-0870 是否稳定命中。
