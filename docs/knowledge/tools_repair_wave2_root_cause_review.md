# Tools Repair Wave 2 Retrieval Root Cause Review

生成时间：2026-07-14

## 1. 范围

本报告复核 `docs/knowledge/tools_repair_wave2_field_test_report.md` 中 20 条 Wave 2 field test 结果，重点分析 18 个 partial 的真实原因。

本阶段未修改 Wiki、Guide、Guide-Wiki 关联、profile、Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或测试用例。

参考基线：

- Field Test：2 / 18 / 0
- Guide 命中率：18.8%
- Wiki 命中率：15.0%
- Guide-Wiki 精准组合率：20.0%
- high/caution 安全边界：100.0%
- fallback / 降级用途：100.0%
- 记录 / 复查建议：100.0%
- 外部依赖违规：0
- Kiwix 越权：0

结论先行：Wave 2 内容本身没有暴露安全边界缺口，主要问题是“维修现场环境/权限/人员边界”这批新主题没有稳定的 query profile 和 candidate anchor，因此新增 Wiki 已存在但不能稳定进入 evidence。少量案例还存在 Selector 在正确大领域内偏向旧工具/材料条目的排序问题。

## 2. 18 个 Partial 复盘

| case | 主要症状 | Root cause | 是否建议 Apply |
| --- | --- | --- | --- |
| wave2_low_light_headlamp_saw | 被停电照明、电池、冰箱食物 Guide 抢走；低光维修 Wiki 未进入 | 缺 profile；Wiki evidence 未进入 | 是 |
| wave2_low_light_small_cut | “光线很暗/切一点点”未绑定维修停止线，误入电器进水和泛安全 | 缺 profile；Wiki evidence 未进入 | 是 |
| wave2_damaged_saw_to_lend | “锯齿坏/给别人用”未绑定破损工具禁用，误入病人用品隔离、设备外借、草木灰 | 缺 profile；跨域竞争；Wiki evidence 未进入 | 是 |
| wave2_borrowed_tool_found_damaged | 命中 DG-0150、DG-0840、DG-0837，但选出的 Wiki 偏旧借用/寿命记录，破损隔离 Wiki 被挤掉 | Selector 排序；相关 Wiki 优先级不足 | 是，轻量 |
| wave2_child_watching_repair | “小孩/旁边看/修东西”未绑定工具区儿童隔离，误入洪水、撤离、雷雨 | 缺 profile；跨域竞争；Wiki evidence 未进入 | 是 |
| wave2_bystander_handing_tools | “旁边递工具”未绑定旁人站位，误入工具保养、低光、借用 | 缺 profile；Wiki evidence 未进入 | 是 |
| wave2_saw_floor_clutter | 命中锯切支撑，但未命中清场/地面杂物；“地上杂物”未形成清场 anchor | 缺 profile 或现有 cutting fixture anchor 不足 | 是 |
| wave2_one_saws_one_holds | “一个锯/一个扶”未绑定站位边界，电线安全和木材条目排在前面 | 缺 profile；Selector 排序 | 是 |
| wave2_no_workbench_platform | `repair_cutting_fixture` 触发，但只带出锯切/夹具，未带出临时工作台稳定性 | Wiki evidence 未进入；candidate anchor 不足 | 是 |
| wave2_wobbly_table_tools | “临时桌子晃/放工具”未绑定工作台稳定性，危险工具隔离误入但不精准 | 缺工作台/工具区 profile；Wiki evidence 未进入 | 是 |
| wave2_three_person_positions | “多人维修/切割/递工具/观察”未绑定多人分区和旁人边界 | 缺 profile；Wiki evidence 未进入 | 是 |
| wave2_people_pass_repair_site | “有人经过/需要停止”误入恐慌、维修失败、泛安全 | 缺 profile；跨域竞争；Wiki evidence 未进入 | 是 |
| wave2_missing_tool_after_repair | “维修结束后工具少一把”未触发现有 `repair_tool_handover`，误入工具保养和儿童工具区 | 现有 profile 覆盖不足；Wiki evidence 未进入 | 是，扩展现有 profile |
| wave2_mixed_repair_materials | “维修材料堆在一起”被材料替代/泛安全竞争，未命中材料混放风险 | 缺工具区材料秩序 profile；跨域竞争；Wiki evidence 未进入 | 是 |
| wave2_screws_blades_small_parts | 命中小零件/紧固件/工具归位，证据安全但偏 Wave 1，锐器钝器分区靠后 | 合理 partial；Selector 排序 | 可暂缓 |
| wave2_no_light_no_workbench | `repair_cutting_fixture` 因工作台触发，低光维修降级未成为主 evidence | Planner terms 提取不足；profile 优先级缺失 | 是 |
| wave2_child_playing_nearby | “孩子/附近玩”误入医疗儿童脱水，未绑定工具区非操作人员边界 | 缺 profile；跨域竞争；Wiki evidence 未进入 | 是 |
| wave2_uncertain_tool_urgent_task | “工具状态不确定/任务很急”未绑定破损工具禁用与隔离 | 缺 profile；Wiki evidence 未进入 | 是 |

## 3. 哪些属于缺 Profile

明确属于缺 profile 或现有 profile 覆盖不足的 case：

- 低光维修：`wave2_low_light_headlamp_saw`、`wave2_low_light_small_cut`、`wave2_no_light_no_workbench`
- 破损工具 / 禁用隔离：`wave2_damaged_saw_to_lend`、`wave2_uncertain_tool_urgent_task`
- 儿童和非操作人员：`wave2_child_watching_repair`、`wave2_child_playing_nearby`
- 旁人站位 / 清场 / 多人分区：`wave2_bystander_handing_tools`、`wave2_saw_floor_clutter`、`wave2_one_saws_one_holds`、`wave2_three_person_positions`、`wave2_people_pass_repair_site`
- 工作台 / 工具区布置：`wave2_no_workbench_platform`、`wave2_wobbly_table_tools`
- 工具清点遗失：`wave2_missing_tool_after_repair`
- 维修材料混放：`wave2_mixed_repair_materials`

这些不是 Wiki 缺失。对应 Wiki 已在 Batch4-N 新增，问题是 query 没有稳定落到“维修现场安全”子域。

不建议新增大量 profile。更小的修法是增加少数 Wave 2 场景 profile，并扩展既有 `repair_tool_handover`、`repair_cutting_fixture` 的触发词或 candidate anchors。

## 4. 哪些属于 Wiki Evidence 未进入

多数 partial 的直接表现是预期 Wiki 没进入 selected evidence：

- 完全未进入候选或未被正确 Guide 带出：`wave2_low_light_headlamp_saw`、`wave2_low_light_small_cut`、`wave2_damaged_saw_to_lend`、`wave2_child_watching_repair`、`wave2_bystander_handing_tools`、`wave2_saw_floor_clutter`、`wave2_one_saws_one_holds`、`wave2_no_workbench_platform`、`wave2_wobbly_table_tools`、`wave2_three_person_positions`、`wave2_people_pass_repair_site`、`wave2_missing_tool_after_repair`、`wave2_mixed_repair_materials`、`wave2_no_light_no_workbench`、`wave2_child_playing_nearby`、`wave2_uncertain_tool_urgent_task`
- 正确 Guide 已出现，但相关 Wiki 被旧条目挤出 selected evidence：`wave2_borrowed_tool_found_damaged`
- 正确领域 evidence 已出现，但 Wave 2 目标 Wiki 排位靠后：`wave2_screws_blades_small_parts`

因此不应通过扩大 top_k 或 selector limit 处理。扩大召回会让更多泛安全、能源、材料、医疗候选进入竞争，掩盖而不是解决 root cause。

## 5. 哪些属于 Selector 排序

明确排序类问题：

1. `wave2_borrowed_tool_found_damaged`
   - `repair_tool_handover` 已触发。
   - DG-0840、DG-0837 已被选中。
   - 但 selected Wiki 优先返回借用归还、编号交接、寿命记录、刀具锯具登记，破损禁用/损坏隔离 Wiki 没进最终证据。
   - 这是“正确 Guide 存在，但 Wave 2 Wiki anchor 不够强”的排序问题。

2. `wave2_one_saws_one_holds`
   - 问题包含锯、旁边扶木板、安全。
   - 结果被电线安全、木料再利用、草木灰竞争。
   - DG-0834 在候选中但没有进入主选。
   - 这是缺清场/站位 profile 后触发的排序漂移。

3. `wave2_screws_blades_small_parts`
   - 结果命中小零件盒、紧固件库存、工具归位，属于正确大领域。
   - `repair-sharp-blunt-tool-zone-storage-001` 等 Wave 2 条目靠后。
   - 这是轻度排序问题，可作为合理 partial 暂缓。

4. `wave2_no_light_no_workbench`
   - `repair_cutting_fixture` 把问题推向工作台/锯切支撑。
   - “没有照明”没有获得低光维修停止线优先级。
   - 这是 profile 优先级和 Planner terms 提取不足共同造成的排序偏移。

## 6. 哪些是合理 Partial，不需要立即修

可以暂不进入 Batch4-P 的 case：

- `wave2_screws_blades_small_parts`
  - 当前命中小零件盒、紧固件库存、工具归位、紧固件和刀片相关 Wiki。
  - 虽然未命中 Wave 2 的锐器/钝器分区存放，但回答方向不会危险，且安全边界、fallback、记录建议均满足。
  - 若 Batch4-P 目标是恢复 Wave 2 现场安全主能力，可暂缓这类“正确领域但不是目标条目”的排序优化。

可低优先级处理的 case：

- `wave2_borrowed_tool_found_damaged`
  - Guide 已命中 DG-0840 / DG-0837，问题主要是 selected Wiki 排序。
  - 如果 Batch4-P 修改 profile anchors 后自动改善，可不单独做 Selector 规则。

不建议把 18 个 partial 全部作为必须修复对象。Batch4-P 应优先修复会造成跨域竞争或核心 Wave 2 安全边界 evidence 缺失的 case。

## 7. 是否是内容缺口或关联缺口

本轮未发现必须新增 Wiki 的内容缺口。

本轮也没有证据表明需要新增大量 Guide-Wiki 关联：

- 低光维修、危险工具隔离、儿童远离工具区、刀锯清场、工具破损禁用标签等 Guide 和 Wiki 已存在。
- 部分 case 中正确 Guide 能出现，但 Wiki 排序仍偏旧条目，说明主因不是“无关联”，而是 profile/candidate anchor 和 selected evidence 排序。
- 若 Batch4-P 后仍出现“正确 Guide 被选中但核心 Wiki 仍不进入”的少数 case，再考虑 1-2 条精准关联或 related_wiki 顺序修正。

预计新增关联数：0。最多保留 1-2 条精准关联作为 Apply 后复测的备选，不作为第一手段。

## 8. 最小 Batch4-P Apply 范围

建议进入 Batch4-P Apply，但范围要小。

建议方案：

1. 新增 `repair_low_light_work_stop`
   - 目标：低光 + 维修 / 切割 / 锯 / 钻 / 工作台 / 简单处理。
   - 主 Guide：DG-0836。
   - 主 Wiki：`repair-low-light-stop-line-001`、`repair-low-light-repair-downgrade-001`。
   - 覆盖：`wave2_low_light_headlamp_saw`、`wave2_low_light_small_cut`、`wave2_no_light_no_workbench`。

2. 新增 `repair_tool_damage_isolation`
   - 目标：工具损坏、锯齿坏、状态不确定、给别人用、临时用一下、任务很急。
   - 主 Guide：DG-0840、DG-0837。
   - 主 Wiki：`repair-damaged-tool-disable-tag-001`、`repair-damaged-tool-report-isolation-001`、`repair-dangerous-tool-temporary-isolation-001`。
   - 覆盖：`wave2_damaged_saw_to_lend`、`wave2_uncertain_tool_urgent_task`，并稳定已 pass 的破损工具 case。

3. 新增或合并 `repair_site_clearance_boundary`
   - 目标：地面杂物、旁边递工具、有人经过、一个人锯一个人扶、多人维修站位。
   - 主 Guide：DG-0839，必要时补 DG-0834。
   - 主 Wiki：`repair-knife-saw-clear-zone-001`、`repair-bystander-position-boundary-001`、`repair-floor-clutter-trip-check-001`、`repair-multi-person-repair-zone-boundary-001`。
   - 覆盖：`wave2_bystander_handing_tools`、`wave2_saw_floor_clutter`、`wave2_one_saws_one_holds`、`wave2_three_person_positions`、`wave2_people_pass_repair_site`。

4. 新增 `repair_child_tool_zone`
   - 目标：小孩 / 孩子 / 儿童 + 工具 / 修东西 / 附近 / 旁边看 / 玩。
   - 主 Guide：DG-0838。
   - 主 Wiki：`repair-children-away-tool-zone-001`、`repair-non-operator-safety-card-001`。
   - 覆盖：`wave2_child_watching_repair`、`wave2_child_playing_nearby`。

5. 扩展既有 `repair_tool_handover`
   - 增加“工具少了一把 / 结束后 / 清点对不上 / 遗失 / 复查”等触发。
   - 目标 Wiki：`repair-post-task-tool-count-001`、`repair-lost-tool-site-recheck-001`。
   - 覆盖：`wave2_missing_tool_after_repair`。

6. 谨慎扩展工作区 / 工作台 anchors
   - 可放入 `repair_site_clearance_boundary`，或新增一个很小的 `repair_workbench_zone_layout`。
   - 目标：工作台、临时桌子、桌子晃、木板搭平台、材料混放。
   - 主 Wiki：`repair-temporary-workbench-stability-check-001`、`repair-manual-repair-workbench-setup-001`、`repair-minimum-work-zone-layout-001`、`repair-mixed-repair-material-risk-001`。
   - 覆盖：`wave2_no_workbench_platform`、`wave2_wobbly_table_tools`、`wave2_mixed_repair_materials`。
   - 如果严格控制新增 profile 数量，可先并入第 3 个现场边界 profile，不单独新增。

## 9. Batch4-P 修改上限

建议最大修改文件数量：

- 严格 Apply：2 个
  - `data/retrieval_query_profiles.json`
  - profile 单元测试文件，例如 `tests/test_tools_repair_retrieval_profiles.py`

- 若同时生成 Apply 报告和刷新 field test 结果：最多 5 个
  - `data/retrieval_query_profiles.json`
  - `tests/test_tools_repair_retrieval_profiles.py`
  - `docs/knowledge/tools_repair_wave2_field_test_results.json`
  - `docs/knowledge/tools_repair_wave2_field_test_report.md`
  - `docs/knowledge/tools_repair_wave2_apply_report.md`

预计新增 profile 数量：4 个。

可选第 5 个 profile：`repair_workbench_zone_layout`。只有在工作台/材料混放无法被 `repair_site_clearance_boundary` 精准覆盖时再加。

预计新增 Guide-Wiki 关联数量：0。

预计新增 Wiki 数量：0。

预计新增 Guide 数量：0。

## 10. 不应修复的问题

不建议在 Batch4-P 中做以下事情：

- 不扩大 top_k 或 selector_candidate_limit。
- 不新增重复的低光、儿童隔离、危险工具、清场 Wiki。
- 不新增大量 profile 逐题覆盖测试语句。
- 不硬编码 field test 的 question、Guide ID 或 Wiki slug。
- 不修改 Prompt 或 Retrieval Pipeline。
- 不继续调整 Batch4-M 已稳定的 rope / fastener / anchor profile，避免破坏 Tools Repair v0.1 stable。
- 不把 `wave2_screws_blades_small_parts` 这类正确大领域 partial 作为优先级最高的问题。

## 11. 最终判断

是否需要 Apply：需要。

Apply 原因：18 个 partial 中多数不是内容质量问题，而是 Wave 2 新主题缺少稳定 profile/candidate anchor，导致新增 Wiki 无法进入 evidence，或被能源、医疗、泛安全、旧工具维护条目竞争掉。

Apply 最大修改文件数量：建议严格控制在 2 个；若包含结果报告刷新，最多 5 个。

预计新增 profile 数量：4 个，最多 5 个。

预计新增关联数量：0。

是否可以保持 Tools & Repair v0.2 架构不变：可以。只需要补充少量 query profile 和现有 profile 触发词，不需要修改 Retrieval Pipeline、Prompt、ranking 架构或 selector limit。
