# Batch8-F Navigation Retrieval Minimal Apply Report

生成日期：2026-07-16

本阶段执行 Navigation Retrieval 最小入口修复。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

未新增 Wiki，未新增 Guide，未修改 Wiki 正文，未修改 Guide 正文，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback、schema 或 PocketBase 数据结构。

参考：

- `docs/knowledge/batch8_e_navigation_root_cause_review.md`
- `docs/knowledge/batch8_d_navigation_field_test_report.md`
- `docs/knowledge/batch8_d_navigation_field_test_results.json`

## 1. 新增 Profile

本阶段新增 4 个 navigation query profile。

|profile|用途|主入口|说明|
|---|---|---|---|
|`navigation_terrain_risk`|洪水后道路、旧路、夜间移动、山区道路、路线风险、封路、绕行|DG-0865|防止 water / evacuation / security 抢占“路线能不能走”的主入口。|
|`navigation_route_planning`|外出采集路线、返回路线、检查点、集合点、回营地、路线记录|DG-0863|让“外出前怎么规划路线”“路线检查点”进入个人外出路线规划 evidence。|
|`navigation_lost_return`|迷路、回营地、原路返回、历史安全路线返回|DG-0864|防止 security / general safety 抢占“怎么回来”的主入口。|
|`navigation_track_management`|GPS track、轨迹、session、路线历史、return summary|DG-0870|用于 track 和历史路线记录入口；这是 Batch8-E 中的可选 profile，本轮因 contract test 覆盖 track 场景而启用。|

另外新增一个窄触发 profile：

|profile|用途|主入口|说明|
|---|---|---|---|
|`navigation_checkpoint_management`|检查点、集合点、位置报告、编号、超时和共享|DG-0866|只在“检查点/集合点 + 设置/编号/记录/共享”类 query 触发，避免为了检查点场景过度收窄 `navigation_route_planning`。|

## 2. Evidence Priority 调整

### DG-0863 个人外出路线规划

调整 `related_wiki` 前段顺序：

1. `navigation-outbound-route-card-001`
2. `navigation-route-checkpoint-numbering-001`
3. `navigation-return-route-plan-001`
4. `navigation-time-distance-turnback-line-001`

目的：路线规划 query 同时带出路线卡、检查点、返回策略和折返点。

同时补充 Wiki frontmatter 反向关系：

- `navigation-route-checkpoint-numbering-001`：`guide_links` 增加 `DG-0863`

未修改该 Wiki 正文。

### DG-0865 撤离路线检查

调整 `related_wiki` 前段顺序：

1. `navigation-route-risk-score-001`
2. `navigation-flood-terrain-risk-001`
3. `navigation-alternate-route-selection-001`
4. `navigation-route-closure-recheck-001`
5. `navigation-flood-road-stop-line-001`
6. `navigation-night-route-risk-001`
7. `navigation-night-movement-stop-line-001`

目的：洪水旧路和夜间路线风险 query 中，让 navigation terrain / night evidence 在 selected wiki 截断前进入。

### DG-0866 检查点记录与团队集合

DG-0866 原本已包含：

- `navigation-route-checkpoint-numbering-001`

本阶段未扩大 DG-0866 的 related_wiki。通过 `navigation_checkpoint_management` profile 提高检查点 query 的 Guide 入口稳定性。

## 3. Contract Test 结果

新增：

- `tests/test_navigation_retrieval_profiles.py`

覆盖 6 个场景：

1. 洪水后原来的道路还能走吗？
2. 夜晚必须移动，路线应该怎么判断？
3. 出去采集物资前怎么规划路线？
4. 怎么给外出路线设置检查点？
5. 走过一次安全路线，下次怎么回来？
6. 过去走过哪些路线怎么记录？

运行：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_navigation_retrieval_profiles.py
```

结果：

```text
15 passed in 4.09s
```

## 4. Field Test 前后变化

Batch8-D 修复前：

|指标|Batch8-D|
|---|---:|
|total|18|
|strict / observation|10 / 8|
|pass / partial / fail|12 / 3 / 3|
|Guide hit|90.0%|
|Wiki full hit|50.0%|
|Guide-Wiki precise|70.0%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|cross domain|10|

Batch8-F 修复后：

|指标|Batch8-F|
|---|---:|
|total|18|
|strict / observation|10 / 8|
|pass / partial / fail|18 / 0 / 0|
|strict pass / partial / fail|10 / 0 / 0|
|Guide hit|100.0%|
|主 Guide hit|100.0%|
|Wiki full hit|100.0%|
|Wiki any hit|100.0%|
|Guide-Wiki precise|100.0%|
|navigation primary rate|77.8%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|cross domain|4|

Field Test 输出：

- `docs/knowledge/batch8_d_navigation_field_test_results.json`
- `docs/knowledge/batch8_d_navigation_field_test_report.md`

说明：脚本仍沿用 Batch8-D 输出文件名和固定报告模板。本报告记录 Batch8-F apply 后的真实回归状态。

## 5. 剩余 Partial 分类

无 strict partial。

无 observation partial。

当前剩余 cross domain 均为 observation 中可接受的相邻领域主导：

|场景|主导领域|判断|
|---|---|---|
|附近有没有水源？|water|合理，水源查询由 water 主导，navigation 可作为后续位置记录补充。|
|无线电失联以后怎么找队友？|communication|合理，失联先由通信检查主导，navigation 可补充集合点/路线。|
|发现危险区域如何记录？|security|合理，危险区域安全标记可由 security 主导，navigation 可补充地图标记。|
|想知道过去走过哪些路线|journal / legacy route|仍为 observation，未造成 fail；track strict case 已 pass。|

## 6. 验证结果

Wiki audit：

```text
Articles: markdown=912 pocketbase=912 categories=24
Issues: errors=0 warnings=0 advisories=0
```

Guide audit：

```text
Guides: 796
Issues: errors=0 warnings=0 advisories=0
```

Guide-Wiki：

- single_forward_without_reverse = 0
- single_reverse_without_forward = 0
- invalid_guide_id = 0
- invalid_wiki_slug = 0

Field Test：

```text
pass / partial / fail = 18 / 0 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
safety = 100%
fallback = 100%
record/check = 100%
```

## 7. 是否达到 Navigation Retrieval v0.1 Stable Candidate

结论：达到 Navigation Retrieval v0.1 stable candidate。

理由：

- 10 个 strict cases 全部 pass。
- 3 个 Batch8-E fail 已全部消除。
- 3 个 Batch8-E partial 已全部消除。
- Wiki full hit 从 50.0% 提升到 100.0%。
- Guide-Wiki precise 从 70.0% 提升到 100.0%。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety / fallback / record-check 均为 100%。

建议进入 Batch8-G Final Verification，不继续扩 profile，不新增 Navigation Wiki/Guide，不修改 Retrieval Pipeline。
