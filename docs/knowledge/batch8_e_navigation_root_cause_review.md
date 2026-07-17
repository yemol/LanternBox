# Batch8-E Navigation Retrieval Root Cause Review

生成日期：2026-07-16

本阶段只生成 Navigation Retrieval 根因分析报告。未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或 tests。

参考：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch8_c2_navigation_apply_report.md`
- `docs/knowledge/batch8_d_navigation_field_test_report.md`
- `docs/knowledge/batch8_d_navigation_field_test_results.json`

## 1. Field Test 总结

Batch8-D Navigation Field Test 当前状态：

|指标|结果|
|---|---:|
|total|18|
|strict / observation|10 / 8|
|pass / partial / fail|12 / 3 / 3|
|strict pass / partial / fail|4 / 3 / 3|
|Guide hit（strict，含 allowed secondary）|90.0%|
|主 Guide hit（strict，仅 expected）|70.0%|
|Wiki full hit（strict）|50.0%|
|Wiki any hit（strict）|70.0%|
|Guide-Wiki precise|70.0%|
|navigation primary guide rate|44.4%|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|
|dangerous suggestion|0|
|Kiwix 越权|0|

结论：Navigation v0.1 的知识内容和安全边界是可用的，但 Retrieval 尚未稳定。GNSS、迷路、track 三条链较稳；路线检查点、夜间移动、洪水旧路、历史安全路线返回仍容易被 evacuation / water / security / power 等相邻领域抢主位，或被 selected wiki 截断。

## 2. 3 个 Fail 根因

### 2.1 夜间移动风险判断

|字段|结果|
|---|---|
|query|夜晚必须移动，路线应该怎么判断？|
|expected Guide|DG-0863；allowed secondary：DG-0865 / DG-0868|
|expected Wiki|navigation-night-route-risk-001|
|actual selected Guide|DG-0587 必须转移时：水食物药品证件优先级；DG-0563 移动电源：关键设备优先级；DG-0601 夜间取水前的路线和容器检查|
|actual selected Wiki|energy-bank-battery-priority-001；energy-lithium-battery-safety-index-001；energy-power-outage-002；energy-swollen-lithium-battery-stop-use-001；food-rationing-priority-001；food-spoilage-boundary-001；water-boiling-002；water-boiling-003|
|是否进入 evidence|DG-0863 / DG-0865 未进入 selected；navigation-night-route-risk-001 未进入 selected|
|cross domain 来源|evacuation / power / water 抢主位，报告标记为 off_domain_primary|
|root cause 分类|C. Guide 存在但未进入 selected；D. Wiki 存在但 related_wiki 未进入；F. profile 缺失；G. 相邻领域抢主位；部分 A：DG-0868 不存在但 fixture 允许它作为 secondary|

判断：

- `navigation-night-route-risk-001` 存在，并且已通过 existing maps repair 关联到 DG-0863 / DG-0865。
- DG-0863 / DG-0865 也存在，但 query 中“夜晚”“必须移动”触发了旧 evacuation / power / water 行动卡。
- 当前没有 navigation profile；`_matching_query_profiles` 返回空，无法把“夜晚 + 路线 + 判断”推向 navigation。
- DG-0868 在 Batch8-C1 被规划但 C2 没有创建；fixture 写了“DG-0868 或 DG-0863”。因此 DG-0868 不应作为本轮失败根因，但说明夜间移动确实是一个未独立成 Guide 的子入口。

根因：profile 缺失 + 相邻领域抢主位。不是 Wiki 不存在，也不是安全边界缺失。

建议：Batch8-F 不新增 DG-0868，先通过 `navigation_terrain_risk` profile 和 DG-0863 / DG-0865 evidence 顺序处理。

### 2.2 历史安全路线返回

|字段|结果|
|---|---|
|query|走过一次安全路线，下次怎么回来？|
|expected Guide|DG-0864；allowed secondary：DG-0863 / DG-0870|
|expected Wiki|navigation-return-route-plan-001|
|actual selected Guide|DG-0634 夜间低亮厕所路线；DG-0023 腹泻超过一天：补液、隔离、追溯；DG-0863 个人外出路线规划|
|actual selected Wiki|safety-low-profile-001；safety-night-movement-001；safety-night-movement-no-solo-001；safety-night-safety-002；safety-stranger-contact-002；medical-common-infectious-disease-index-001；navigation-outbound-route-card-001；navigation-return-route-plan-001|
|是否进入 evidence|allowed secondary DG-0863 进入 rank3；expected Wiki navigation-return-route-plan-001 进入 selected；expected DG-0864 未进入|
|cross domain 来源|security 抢主位，报告标记为 off_domain_primary|
|root cause 分类|G. 相邻领域抢主位；F. profile 缺失；H. fixture expected 部分过窄|

判断：

- `navigation-return-route-plan-001` 已进入 selected evidence，因此 Wiki 不缺。
- DG-0863 进入 selected，但在 rank3，top1 被 security 夜间/低暴露路线抢走。
- 该 query 的“安全路线”容易被安全领域理解为低暴露/安全行动，而不是 route return。
- fixture 期望 DG-0864 可以理解，因为“怎么回来”与返回有关；但实际“走过一次路线，下次复用/返回”也可由 DG-0863 或 DG-0870 承接。由于 allowed secondary 已含 DG-0863 / DG-0870，本 case 的实质问题是 navigation 没有主位，而不是 evidence 完全缺失。

根因：profile 缺失 + security 抢主位。fixture expected 不算完全错误，但主 Guide 期望偏窄。

建议：Batch8-F 用 `navigation_lost_return` 或 `navigation_route_planning` profile 覆盖“回来 / 回营地 / 下次怎么回来 / 走过路线 / 安全路线返回”，不用新增 Wiki/Guide。

### 2.3 洪水后旧路判断

|字段|结果|
|---|---|
|query|洪水后原来的道路还能走吗？|
|expected Guide|DG-0865|
|expected Wiki|navigation-flood-terrain-risk-001|
|actual selected Guide|DG-0602 洪水后取水点禁用判断；DG-0492 洪水后饮水保护；DG-0865 撤离路线检查|
|actual selected Wiki|food-rationing-priority-001；food-spoilage-boundary-001；water-boiling-002；water-boiling-003；water-chemical-contamination-001；water-drinking-water-001；water-filtering-001；water-flood-risk-049|
|是否进入 evidence|DG-0865 进入 rank3；navigation-flood-terrain-risk-001 存在于 DG-0865 related_wiki，但未进入 selected wiki|
|cross domain 来源|water 抢主位，报告标记为 off_domain_primary / navigation_vs_water|
|root cause 分类|C. Guide 存在但未进入主位；D. Wiki 存在但 related_wiki 未进入；E. related_wiki / selected wiki 顺序问题；F. profile 缺失；G. 相邻领域抢主位|

判断：

- `navigation-flood-terrain-risk-001` 存在，并已关联 DG-0865。
- DG-0865 进入 top3，但水系统 Guide 排在前两位；selected wiki 先被 water Guide 的 related_wiki 填满，DG-0865 的 navigation related_wiki 未进入最终 selected。
- 这不是知识缺失，而是 query “洪水后”触发 water domain；而用户问的是“道路还能走吗”，应由 navigation route passability 主导，water 只补充污染/水体风险。

根因：profile 缺失 + water 抢主位 + related_wiki 截断顺序。

建议：Batch8-F 用 `navigation_terrain_risk` profile 覆盖“洪水后 + 道路/旧路/路线/桥/还能走”，并把 DG-0865 的 `navigation-flood-terrain-risk-001` 与 `navigation-flood-road-stop-line-001` 提到更前。

## 3. 3 个 Partial 分类

### 3.1 外出采集前规划路线

|字段|结果|
|---|---|
|query|出去采集物资前应该怎么规划路线？|
|expected Guide|DG-0863|
|expected Wiki|navigation-outbound-route-card-001；navigation-route-checkpoint-numbering-001|
|actual selected Guide|DG-0863 rank1；DG-0247；DG-0266|
|actual selected Wiki|navigation-outbound-route-card-001；navigation-return-route-plan-001；navigation-time-distance-turnback-line-001；navigation-compass-orientation-basics-001；navigation-bearing-distance-estimate-001；navigation-offline-map-version-control-001；navigation-offline-map-layer-check-001；navigation-paper-map-index-card-001|
|分类|D. Wiki 存在但 related_wiki 未进入；E. related_wiki 顺序/关系设计；H. fixture expected 可接受但比当前 DG-0863 evidence 更细|

判断：

- DG-0863 稳定命中 rank1。
- `navigation-route-checkpoint-numbering-001` 存在，但没有关联到 DG-0863，只关联 DG-0866。
- C1/C2 设计里 DG-0863 负责“个人外出路线规划”，检查点是路线规划的自然子字段；strict fixture 期望它进入 evidence 合理。

建议：Batch8-F 给 DG-0863 精准补充 `navigation-route-checkpoint-numbering-001`，并放在 `navigation-outbound-route-card-001` / `navigation-return-route-plan-001` 附近。不需要新增 Wiki/Guide。

### 3.2 外出路线设置检查点

|字段|结果|
|---|---|
|query|怎么给外出路线设置检查点？|
|expected Guide|DG-0866；allowed secondary：DG-0863|
|expected Wiki|navigation-route-checkpoint-numbering-001|
|actual selected Guide|DG-0863 rank1；DG-0124；DG-0865|
|actual selected Wiki|DG-0863 related_wiki 前 8 条；未含 navigation-route-checkpoint-numbering-001|
|分类|C. DG-0866 存在但未进入 selected；D. Wiki 存在但未进入；F. profile 缺失；E. DG-0863 related_wiki 缺检查点 Wiki|

判断：

- DG-0866 作为检查点专门 Guide 不稳定，strict case 未进入 top3。
- Query 中“外出路线 + 检查点”被 DG-0863 泛路线入口吸走；这可以接受为 secondary，但必须带上 checkpoint Wiki。
- 只靠 related_wiki 顺序无法让 DG-0866 主导；需要 profile 或更强 query trigger。

建议：Batch8-F 同时做两件小改：`navigation_route_planning` profile 覆盖“检查点 / 路线设置 / 外出路线”，并给 DG-0863 加 `navigation-route-checkpoint-numbering-001` 作为 early related_wiki。

### 3.3 撤离路线风险检查

|字段|结果|
|---|---|
|query|撤离路线应该怎么检查风险？|
|expected Guide|DG-0865|
|expected Wiki|navigation-route-risk-score-001；navigation-flood-terrain-risk-001|
|actual selected Guide|DG-0865 rank1；DG-0071；DG-0703|
|actual selected Wiki|navigation-route-risk-score-001；navigation-alternate-route-selection-001；navigation-route-closure-recheck-001；navigation-flood-road-stop-line-001；navigation-slope-landslide-bypass-001；navigation-evacuation-route-passability-review-001；navigation-evacuation-route-go-no-go-line-001；navigation-night-movement-stop-line-001|
|分类|E. related_wiki 顺序问题；H. fixture expected 可复核|

判断：

- DG-0865 稳定命中 rank1。
- `navigation-route-risk-score-001` 已进入 selected。
- `navigation-flood-terrain-risk-001` 存在于 DG-0865 related_wiki，但顺序靠后，selected wiki 只截到前 8，未进入。
- 同时，`navigation-flood-road-stop-line-001` 已进入 selected，而且比旧 `navigation-flood-terrain-risk-001` 更行动化；fixture 期望旧 Wiki 有合理性，因为它是 existing maps repair 的重点，但未来也可以把 `navigation-flood-road-stop-line-001` 视为更精准 evidence。

建议：Batch8-F 只调整 DG-0865 related_wiki 顺序，把 `navigation-flood-terrain-risk-001` 提到 `navigation-flood-road-stop-line-001` 附近。是否扩展 fixture allowed Wiki 留到 Final Verification，不在 Apply 阶段硬改。

## 4. Navigation Guide 稳定性

|Guide|Field 命中|Wiki evidence|稳定性|问题|
|---|---|---|---|---|
|DG-0863 个人外出路线规划|4 次进入 selected；strict 中 2 次 rank1、1 次 rank3；observation 手机地图不可用 rank1|外出路线、返回、折返、离线地图稳定；缺 checkpoint wiki|Yellow|路线规划主入口可用，但 query 含“检查点”时仍未带 `navigation-route-checkpoint-numbering-001`；“安全路线返回”被 security 抢 top1。|
|DG-0864 迷路后的定位与返回|2 次进入 selected，均为 pass；lost rank1，GNSS no fix rank2|lost、no fix、position error、return plan 稳定|Green / Yellow|迷路和无 fix 稳；“走过安全路线，下次回来”未进入，说明 return-route query profile 不足。|
|DG-0865 撤离路线检查|3 次进入 selected；撤离风险 rank1，洪水旧路 rank3|route-risk 稳；flood-terrain 存在但常被截断|Yellow|水/撤离旧 Guide 会抢主位；洪水道路 query 需要 navigation terrain profile。|
|DG-0866 检查点记录与团队集合|仅 observation 团队集合 rank2；strict 检查点未进入|checkpoint Wiki 只作为 DG-0866 related，但 DG-0866 不稳|Red / Yellow|需要 profile 触发“检查点 / 集合点 / 位置报告”；同时 DG-0863 应补 checkpoint evidence。|
|DG-0867 GNSS 坐标记录|3 次进入 selected；GPS drift rank1，GNSS no fix rank1，track review rank2|GNSS basics、no fix、position error 稳定|Green|当前最稳定的 Navigation 子入口之一；暂无 profile 优先需求。|
|DG-0870 野外任务轨迹记录|2 次进入 selected；track review rank1，GNSS no fix rank3|track fields、drift、return summary 稳定|Green / Yellow|track review 稳；“过去走过哪些路线” observation 被旧探路/记录抢位，后续可通过 track profile 处理。|

## 5. Navigation Wiki Evidence 检查

|Wiki|selected evidence 表现|related_wiki 表现|判断|
|---|---|---|---|
|navigation-gnss-basics-001|GPS drift、GNSS no fix、track review selected|DG-0867 / DG-0870 相关链可加载|Green|
|navigation-gnss-no-fix-downgrade-001|lost、GPS drift、GNSS no fix selected|多处 related 可加载|Green|
|navigation-position-error-check-001|lost、GPS drift、GNSS no fix selected|多处 related 可加载|Green|
|navigation-return-route-plan-001|多 case selected，包括 lost、plan、checkpoint、safe route|DG-0863 / DG-0864 related 稳定|Green|
|navigation-lost-stop-and-last-known-point-001|lost、GNSS no fix selected|DG-0864 related 稳定|Green|
|navigation-route-checkpoint-numbering-001|未在 strict selected；仅 observation team share 中 related|仅 DG-0866 related；DG-0866 不稳|Red / Yellow|
|navigation-route-risk-score-001|evac route selected|DG-0865 related 稳定|Green|
|navigation-flood-terrain-risk-001|未 selected；但在 DG-0865 related 中多次出现|顺序靠后，常被截断|Yellow / Red|
|navigation-night-route-risk-001|未 selected；但在 DG-0863 / DG-0865 related 中出现|夜间 query 未选 navigation Guide|Red|
|navigation-gps-track-minimum-fields-001|track review selected|DG-0870 related 稳定|Green|
|navigation-track-to-return-summary-001|lost、track review selected|DG-0864 / DG-0870 related 稳定|Green|

结论：核心问题不在 GNSS / lost / track。薄弱点是 checkpoint、night-route、flood-terrain 三组 Wiki 的 selected 入口。

## 6. Cross Domain 分析

Field Test 统计：

|来源|表现|
|---|---|
|water|洪水后旧路、水源查询中明显抢主位；“洪水后”会强触发 water。|
|communication|无线电失联找队友由通信主导合理；手机地图不可用时 navigation 可进入。|
|security|“安全路线”“危险区域记录”“夜间”容易触发 security。|
|evacuation|夜间必须移动、撤离路线相关 case 与 navigation 高度重叠。|
|outdoor / geography|项目无正式 `outdoor` / `geography` domain；相关内容实际散落在 maps、evacuation、security、repair。山区道路 observation 未由 navigation 主导。|
|shelter|夜晚露营选址由 safety / exchange Guide 抢位，navigation 未主导；对该 query shelter 主导也可能合理。|

### Navigation 应优先的情况

Navigation 应主导：

- 问的是“怎么走 / 回哪里 / 回营地 / 下次怎么回来 / 路线怎么规划”。
- 问的是“检查点 / 集合点 / path point / track / 轨迹 / 坐标 / GNSS / GPS drift”。
- 问的是“道路还能不能走 / 洪水后旧路 / 夜间路线 / 封路绕行 / 路线风险评分”。
- 目标是保护返回能力、路线可复核、位置可交接。

### Geography / outdoor / shelter / water 应优先的情况

相邻领域应主导：

- 问水源是否可饮、怎么取水、怎么净化：water 主导，navigation 只补点位/路线。
- 问露营/住所是否可住、保暖、防雨、防潮：shelter 主导，navigation 只补位置风险和返回路线。
- 问地形知识解释，不问行动路线：geography/maps background 可补充。
- 问无线电设备、通信窗口、失联通信流程：communication 主导，navigation 只补检查点和最后位置。
- 问外部人、低暴露、安全冲突：security 主导，navigation 只补路线/危险区标记。

### 应协同的情况

需要协同：

- 洪水后路线：navigation 主导路线通行，water 补污染和水体风险。
- 无线电失联找队友：communication 主导通信排查，navigation 补检查点、最后确认点和搜索边界。
- 夜间露营/移动：shelter 主导露营，navigation 主导路线移动。
- 团队集合点共享：navigation 主导坐标/集合点字段，communication/organization 补交接和回执。

## 7. Profile 建议

当前 `data/retrieval_query_profiles.json` 未发现 navigation 类 profile。Field Test 中 `_matching_query_profiles` 均为空，这是 Navigation v0.1 不稳定的核心原因之一。

### navigation_route_planning

建议：需要。

覆盖：

- 外出路线
- 采集/取水/巡查前规划
- 检查点
- 返回路线
- 回营地
- 下次怎么回来
- 手机地图不可用

优先 Guide：

- DG-0863
- DG-0866
- DG-0864
- DG-0870

secondary：

- evacuation
- communication
- security

理由：解决 `navigation_route_checkpoint_setup`、`navigation_safe_route_return_next_time` 和路线规划 partial。

### navigation_lost_return

建议：需要，但可小于 route_planning。

覆盖：

- 迷路
- 不知道在哪
- 找回营地
- 回哪里
- 原路返回
- 走过一次路线
- 安全路线返回

优先 Guide：

- DG-0864
- DG-0863
- DG-0870

secondary：

- communication
- security

理由：lost case 已 pass，但“历史安全路线返回”被 security 抢主位；该 profile 可保护返回语义。

### navigation_terrain_risk

建议：最优先。

覆盖：

- 洪水后道路
- 旧路还能不能走
- 夜间移动
- 路线风险
- 封路
- 绕行
- 山区道路
- 坡地 / 滑坡
- 桥 / 积水 / 暗流

优先 Guide：

- DG-0865
- DG-0863

secondary：

- water
- evacuation
- shelter
- safety

理由：3 个 fail 中 2 个与 terrain / route risk 有关，且均表现为相邻领域抢主位。

### navigation_track_management

建议：暂缓或低优先。

覆盖：

- GPS track
- 轨迹
- 过去走过哪些路线
- path point
- session
- 轨迹转返回路线

优先 Guide：

- DG-0870
- DG-0864

理由：track strict case 已 pass。只有 observation “过去走过哪些路线”被旧探路/记录类抢位，可作为 Batch8-F 可选项，不应优先于 terrain / route / lost。

## 8. Batch8-F 最小 Apply 建议

建议选择：C. profile + evidence priority。

不建议：

- 不新增大量 Wiki。
- 不新增 Guide。
- 不提前做地图系统。
- 不修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。
- 不硬改 Field Test fixture 来制造通过。

建议 Batch8-F 范围：

1. 新增少量 navigation query profile：
   - `navigation_terrain_risk`
   - `navigation_route_planning`
   - `navigation_lost_return`
   - `navigation_track_management` 可选，若预算紧张则暂缓。
2. 最小 evidence priority 调整：
   - DG-0863：补充或前移 `navigation-route-checkpoint-numbering-001`，让路线规划能带出检查点字段。
   - DG-0865：前移 `navigation-flood-terrain-risk-001`、`navigation-flood-road-stop-line-001`、`navigation-night-route-risk-001`、`navigation-night-movement-stop-line-001`。
   - DG-0866：保持 checkpoint 主入口，不大批量扩 related_wiki。
3. 不新增 Wiki / Guide。
4. 不改 Retrieval Pipeline / Prompt / top_k / selector limit / ranking / fallback。
5. Apply 后重新运行 Navigation Field Test。

预期修复目标：

- 夜间移动风险判断：DG-0863 / DG-0865 进入 selected。
- 洪水后旧路判断：DG-0865 主导，navigation-flood-terrain-risk-001 或 navigation-flood-road-stop-line-001 进入 selected。
- 历史安全路线返回：navigation Guide 成为 top evidence，security 退为 secondary。
- 检查点 partial：navigation-route-checkpoint-numbering-001 稳定进入 DG-0863 / DG-0866 evidence。

是否需要新增知识：暂不需要。现有 Wiki 和 Guide 已覆盖行动边界，问题主要是检索入口和 evidence priority。
