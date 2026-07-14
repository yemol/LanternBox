# Tools Repair Wave 2 Retrieval Minimal Apply Report

生成时间：2026-07-14

## 1. 范围

本批按 `docs/knowledge/tools_repair_wave2_root_cause_review.md` 执行最小 Apply，只处理已确认的 Retrieval 根因。

本批未修改 Wiki 正文、Guide 正文、Guide-Wiki 关联、PocketBase schema、Prompt、Retrieval Pipeline、top_k、selector_candidate_limit、ranking 架构或 fallback 逻辑。

手工修改范围：

- `data/retrieval_query_profiles.json`
- `tests/test_tools_repair_retrieval_profiles.py`

验证刷新文件：

- `docs/knowledge/tools_repair_wave2_field_test_results.json`
- `docs/knowledge/tools_repair_wave2_field_test_report.md`

新增报告：

- `docs/knowledge/tools_repair_wave2_apply_report.md`

## 2. 修改 Profile 列表

### repair_low_light_work_stop

新增。用于把夜间、停电、头灯、手电不足、看不清、照明不足等维修场景导向低光维修停止线。

覆盖 case：

- `wave2_low_light_headlamp_saw`
- `wave2_no_light_no_workbench`

目标 evidence：

- Guide：DG-0836
- Wiki：`repair-low-light-stop-line-001`、`repair-low-light-repair-downgrade-001`

效果：`wave2_low_light_headlamp_saw` 从 partial 变为 pass，`wave2_no_light_no_workbench` 从 partial 变为 pass。`wave2_low_light_small_cut` 仍未触发，因为 query 没有明确维修对象词，归类为剩余 profile 触发缺口。

### repair_tool_damage_isolation

新增。用于把工具损坏、状态不确定、临时使用、借给别人等场景导向破损工具禁用和危险工具隔离。

覆盖 case：

- `wave2_damaged_saw_to_lend`
- `wave2_borrowed_tool_found_damaged`
- `wave2_damaged_tool_temporary_use`
- `wave2_uncertain_tool_urgent_task`

目标 evidence：

- Guide：DG-0840、DG-0837
- Wiki：`repair-damaged-tool-disable-tag-001`、`repair-damaged-tool-report-isolation-001`、`repair-dangerous-tool-temporary-isolation-001`

效果：相关破损工具 case 全部 pass，并且未引入外部依赖或 Kiwix 越权。

### repair_site_clearance_boundary

新增。用于维修现场、地面杂物、旁人、多人、工作区、工作台、临时桌子、木板平台和维修材料混放等现场边界问题。

覆盖 case：

- `wave2_bystander_handing_tools`
- `wave2_saw_floor_clutter`
- `wave2_one_saws_one_holds`
- `wave2_no_workbench_platform`
- `wave2_wobbly_table_tools`
- `wave2_three_person_positions`
- `wave2_people_pass_repair_site`
- `wave2_mixed_repair_materials`
- `wave2_no_light_no_workbench`

目标 evidence：

- Guide：DG-0839、DG-0834
- Wiki：`repair-knife-saw-clear-zone-001`、`repair-floor-clutter-trip-check-001`、`repair-bystander-position-boundary-001`、`repair-multi-person-repair-zone-boundary-001`
- 工作台相关目标：`repair-temporary-workbench-stability-check-001`、`repair-work-zone-minimum-layout-001`

效果：清场、旁人、多人、材料混放相关 case 明显改善。工作台两个 case 仍 partial，原因不是安全方向错误，而是工作台 Wiki evidence priority 仍被清场/固定支撑条目压住。

### repair_child_tool_zone

新增。用于儿童、小孩、孩子在工具区旁观或附近玩耍的场景。

覆盖 case：

- `wave2_child_watching_repair`
- `wave2_child_playing_nearby`

目标 evidence：

- Guide：DG-0838
- Wiki：`repair-children-away-tool-zone-001`、`repair-child-nonoperator-safety-card-001`

效果：儿童相关 case 全部 pass，且不再被 medical child / food / family 类 evidence 抢主位。

### repair_tool_handover

扩展。增加工具少了、找不到、结束后、清点、数量不对、遗失、归还检查等触发词和候选 anchor。

覆盖 case：

- `wave2_missing_tool_after_repair`

目标 evidence：

- Guide：DG-0150
- Wiki：`repair-post-task-tool-count-001`、`repair-lost-tool-site-recheck-001`

效果：工具遗失清点 case 从 partial 变为 pass。

## 3. Contract Test

新增或补充了 7 个 profile contract case：

1. 低光手锯
2. 工具损坏继续使用
3. 儿童旁边观看
4. 多人维修站位
5. 地面杂物清场
6. 工具遗失清点
7. 临时工作台

结果：

- `tests/test_tools_repair_retrieval_profiles.py`：22 passed

## 4. 测试前后变化

Batch4-N-R apply 前：

- pass / partial / fail：2 / 18 / 0
- Guide 命中率：18.8%
- Wiki 命中率：15.0%
- Guide-Wiki 精准组合率：20.0%
- high/caution 安全边界：100.0%
- fallback / 降级用途：100.0%
- 记录 / 复查建议：100.0%
- 外部依赖违规：0
- Kiwix 越权：0
- 跨域竞争：4

本批 apply 后：

- pass / partial / fail：16 / 4 / 0
- Guide 命中率：87.5%
- Wiki 命中率：85.0%
- Guide-Wiki 精准组合率：80.0%
- high/caution 安全边界：100.0%
- fallback / 降级用途：100.0%
- 记录 / 复查建议：100.0%
- 外部依赖违规：0
- Kiwix 越权：0
- 跨域竞争：0

目标达成情况：

- pass >= 18：未达成
- partial <= 2：未达成
- fail = 0：达成
- Wiki 命中率 >= 80%：达成
- 安全边界 100%：达成
- fallback 100%：达成
- 外部依赖违规 0：达成
- Kiwix 越权 0：达成

## 5. 剩余 Partial 分类

### wave2_low_light_small_cut

问题：`光线很暗但是只是切一点点，会不会没关系？`

当前表现：

- 未触发 `repair_low_light_work_stop`
- 误入电器进水、接触风险和泛安全 evidence

分类：profile 触发缺口。

原因：query 没有明确维修对象词，只出现“光线很暗”和“切一点点”。如果继续修，应评估是否允许 `repair_low_light_work_stop` 在没有明确工具/维修对象时触发。当前不继续扩 profile，避免把所有低光问题都推入维修停止线。

### wave2_no_workbench_platform

问题：`没有工作台，临时拿几块木板搭个平台维修可以吗？`

当前表现：

- 已触发 `repair_cutting_fixture` 和 `repair_site_clearance_boundary`
- Guide 命中 DG-0839 / DG-0834
- selected Wiki 偏向刀锯清场、旁人站位、地面杂物、锯切支撑
- 未命中 `repair-temporary-workbench-stability-check-001`、`repair-manual-repair-workbench-setup-001`

分类：Wiki evidence priority。

原因：工作台 Wiki 已存在，但没有作为 Guide 相关 evidence 被带出，独立 wiki 排序又被清场和锯切支撑压住。本批不修改 Guide-Wiki 关联，也不新增第五个 workbench profile。

### wave2_wobbly_table_tools

问题：`临时桌子有点晃，但是还能放工具，需要处理吗？`

当前表现：

- 已触发 `repair_site_clearance_boundary`
- selected Wiki 偏向刀锯清场、旁人站位、地面杂物、固定支撑、工具保养
- 未命中工作台稳定性目标 Wiki

分类：Selector 排序 / Wiki evidence priority。

原因：同一个现场边界 profile 同时覆盖清场、旁人、工作区和工作台时，清场类 evidence 仍然更容易进入前位。按 Batch4-O 的最小 Apply 原则，不继续新增 `repair_workbench_zone_layout`。

### wave2_screws_blades_small_parts

问题：`螺丝、刀片、小零件放一起容易丢，怎么办？`

当前表现：

- Guide 未命中 DG-0837 / DG-0150
- Wiki 命中正确领域：紧固件保存、螺丝螺母、刀片崩口、旧紧固件复用等

分类：合理 partial / Selector 排序。

原因：当前 evidence 不危险，方向仍在工具与小件管理；只是未命中 Wave 2 目标 Guide。Batch4-O 已将其列为可暂缓 case，本批不再为它新增规则。

## 6. 跨域竞争

跨域竞争从 4 降为 0。

已改善的原跨域问题：

- 破损锯具不再误入病人用品隔离 / 草木灰
- 儿童旁观不再误入洪水食物
- 维修现场有人经过不再误入心理恐慌
- 孩子附近玩不再误入医疗儿童脱水

未发现新增外部依赖违规。

未发现 Kiwix 越权。

## 7. 验证结果

已运行：

```bash
python3 tools/audit_wiki.py
```

结果：

- Articles：markdown=763 pocketbase=763 categories=24
- Issues：errors=0 warnings=0 advisories=0

已运行：

```bash
python3 tools/build_guides.py
```

结果：

- Generated 770 Guides
- Generated 770 Guide Index Items

已运行：

```bash
python3 scripts/audit_guides.py
```

结果：

- Guides：770
- Issues：errors=0 warnings=0 advisories=0

已运行：

```bash
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_tools_repair_retrieval_profiles.py
```

结果：

- 31 passed

已运行：

```bash
venv/bin/python scripts/test_tools_repair_wave2_field.py --no-answer
```

结果：

- pass / partial / fail：16 / 4 / 0

## 8. 是否需要下一轮优化

需要，但不建议在本批继续补规则。

建议下一轮先 Review，不直接 Apply：

1. 评估是否允许低光停止线在“光线很暗 + 切一点点”这种无明确对象 query 上触发。
2. 评估工作台类 Wiki 是否需要一个小型专用 profile，或是否需要精准 Guide-Wiki 关联。
3. 评估 `wave2_screws_blades_small_parts` 是否接受为合理 partial，还是要为锐器/钝器分区补一个小件工具区 profile。

不建议：

- 不扩大 top_k。
- 不扩大 selector_candidate_limit。
- 不新增重复 Wiki。
- 不新增重复 Guide。
- 不继续在同一个 profile 里无限堆词。
- 不修改 Prompt 或 Pipeline。

## 9. 结论

本批没有完全达到 18 / 2 / 0 的 field test 目标，但已完成最小 Apply 的主要收益：

- pass 从 2 提升到 16。
- partial 从 18 降到 4。
- fail 保持 0。
- 跨域竞争从 4 降到 0。
- Wiki 命中率从 15.0% 提升到 85.0%。
- 安全边界、fallback、记录建议均保持 100%。

剩余 partial 已分类，不继续扩 profile，避免进入无限补规则循环。
