# Tools & Repair Wave 2 Retrieval Field Test Report

生成时间：2026-07-14T05:57:27.502336+00:00

## 1. 测试范围

本阶段只测试 Batch4-N 新增的维修现场安全知识是否能进入本地 Retrieval evidence。脚本不调用 LLM Planner、Selector 或回答生成器，不修改 Wiki/Guide/关联/profile/pipeline。

覆盖主题：低光维修停止线、危险工具隔离、儿童和非操作人员边界、刀锯清场、工作台、多人员维修分区、工具清点、材料混放、降级维修和破损工具禁用。

## 2. 汇总

- 用例总数：20
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

## 3. 目标达成

- pass_gte_18：未达成
- partial_lte_2：未达成
- fail_eq_0：达成
- guide_hit_gte_90：未达成
- wiki_hit_gte_80：达成
- safety_eq_100：达成
- fallback_eq_100：达成
- record_eq_100：达成
- external_eq_0：达成
- kiwix_eq_0：达成

## 4. 20 条 Case 明细

| case | verdict | Guide | Wiki | profiles | safety | fallback | record | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| wave2_low_light_headlamp_saw | pass | DG-0836 低光维修停止线、DG-0839 刀锯作业前清场、DG-0621 门窗松动低噪加固 | repair-low-light-stop-line-001 低光维修停止线、repair-low-light-repair-downgrade-001 维修时照明不足的降级方案、repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界 | repair_low_light_work_stop、repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_low_light_small_cut | partial | DG-0546 插线板和延长线进水后：禁止通电与报废判断、DG-0714 管理多次接触对象、DG-0709 接触后风险复盘 | energy-basic-electrical-safety-index-001 基础电学安全索引、energy-water-damaged-electrical-no-power-001 电器进水后禁止通电、safety-safety-knowledge-009 长期互助关系的信任边界、safety-review-001 交换后复盘和物资清点 | 无 | 是 | 是 | 是 | Wiki 不可召回、query profile 缺失 |
| wave2_damaged_saw_to_lend | pass | DG-0840 工具破损后禁用标签、DG-0837 危险工具临时隔离、DG-0272 设备外借和维修前清理 | repair-damaged-tool-disable-tag-001 工具破损后禁用标签、repair-damaged-tool-report-isolation-001 工具损坏上报和隔离、repair-dangerous-tool-temporary-isolation-001 危险工具临时隔离、repair-dangerous-tool-borrow-permission-001 危险工具借用权限 | repair_tool_damage_isolation | 是 | 是 | 是 | 无 |
| wave2_borrowed_tool_found_damaged | pass | DG-0840 工具破损后禁用标签、DG-0150 工具借用和归还、DG-0837 危险工具临时隔离 | repair-damaged-tool-disable-tag-001 工具破损后禁用标签、repair-damaged-tool-report-isolation-001 工具损坏上报和隔离、repair-tool-borrow-return-log-001 工具借用归还记录、repair-tool-id-handover-log-001 手工具编号和交接记录 | repair_tool_damage_isolation、repair_tool_handover | 是 | 是 | 是 | 无 |
| wave2_child_watching_repair | pass | DG-0838 儿童远离工具区、DG-0095 儿童走失预防：先设计环境、DG-0343 蜡烛使用边界 | repair-children-away-tool-zone-001 儿童远离工具区、repair-child-nonoperator-safety-card-001 儿童和非操作人员安全提示卡、repair-work-zone-minimum-layout-001 维修工作区最小布置、repair-frequent-tool-quick-access-zone-001 常用工具快速取用区 | repair_child_tool_zone | 是 | 是 | 是 | 无 |
| wave2_bystander_handing_tools | pass | DG-0839 刀锯作业前清场、DG-0624 工具生锈清洁和封存、DG-0836 低光维修停止线 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-drinking-water-001 水桶漏水和饮用水容器降级 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_saw_floor_clutter | pass | DG-0839 刀锯作业前清场、DG-0834 锯切前固定与支撑、DG-0304 木板和木棍再利用 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-saw-cut-support-001 锯切前固定与支撑 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_one_saws_one_holds | pass | DG-0839 刀锯作业前清场、DG-0304 木板和木棍再利用、DG-0834 锯切前固定与支撑 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-wet-wood-strength-check-001 木料受潮后强度判断 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_no_workbench_platform | partial | DG-0839 刀锯作业前清场、DG-0834 锯切前固定与支撑、DG-0304 木板和木棍再利用 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-saw-cut-support-001 锯切前固定与支撑 | repair_cutting_fixture、repair_site_clearance_boundary | 是 | 是 | 是 | Wiki 不可召回 |
| wave2_wobbly_table_tools | partial | DG-0839 刀锯作业前清场、DG-0569 门窗松动：临时加固、DG-0624 工具生锈清洁和封存 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-fixing-and-support-001 固定与支撑 | repair_site_clearance_boundary | 是 | 是 | 是 | Selector 排序问题 |
| wave2_three_person_positions | pass | DG-0839 刀锯作业前清场、DG-0624 工具生锈清洁和封存、DG-0836 低光维修停止线 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-drinking-water-001 水桶漏水和饮用水容器降级 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_people_pass_repair_site | pass | DG-0839 刀锯作业前清场、DG-0836 低光维修停止线、DG-0624 工具生锈清洁和封存 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-low-light-stop-line-001 低光维修停止线 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_missing_tool_after_repair | pass | DG-0150 工具借用和归还、DG-0840 工具破损后禁用标签、DG-0624 工具生锈清洁和封存 | repair-tool-borrow-return-log-001 工具借用归还记录、repair-tool-id-handover-log-001 手工具编号和交接记录、repair-wear-tool-life-log-001 易损工具寿命记录、repair-knife-saw-use-log-001 刀具锯具使用登记 | repair_tool_handover | 是 | 是 | 是 | 无 |
| wave2_new_member_dangerous_tool | pass | DG-0837 危险工具临时隔离、DG-0512 危险工具权限、DG-0406 新成员三日教学 | repair-dangerous-tool-temporary-isolation-001 危险工具临时隔离、repair-dangerous-tool-borrow-permission-001 危险工具借用权限、repair-sharp-blunt-tool-zone-storage-001 锐器和钝器分区存放 | 无 | 是 | 是 | 是 | 无 |
| wave2_mixed_repair_materials | pass | DG-0839 刀锯作业前清场、DG-0517 清洁材料替代、DG-0836 低光维修停止线 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-low-light-stop-line-001 低光维修停止线 | repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_screws_blades_small_parts | partial | DG-0309 临时小零件盒、DG-0376 螺丝钉钉子小件库存、DG-0372 常用工具每日归位 | repair-fastener-washer-storage-001 螺丝钉垫片分类保存、repair-screws-and-nuts-001 螺丝和螺母、repair-screwdriver-camout-protection-001 螺丝刀打滑与螺丝头保护、repair-blade-dull-chip-check-001 刀片变钝与崩口判断 | 无 | 是 | 是 | 是 | Selector 排序问题 |
| wave2_no_light_no_workbench | pass | DG-0839 刀锯作业前清场、DG-0836 低光维修停止线、DG-0834 锯切前固定与支撑 | repair-knife-saw-clear-zone-001 刀锯作业前清场、repair-bystander-position-boundary-001 维修现场旁人站位边界、repair-floor-clutter-trip-check-001 地面杂物和绊倒风险检查、repair-low-light-stop-line-001 低光维修停止线 | repair_cutting_fixture、repair_low_light_work_stop、repair_site_clearance_boundary | 是 | 是 | 是 | 无 |
| wave2_damaged_tool_temporary_use | pass | DG-0840 工具破损后禁用标签、DG-0837 危险工具临时隔离、DG-0546 插线板和延长线进水后：禁止通电与报废判断 | repair-damaged-tool-disable-tag-001 工具破损后禁用标签、repair-damaged-tool-report-isolation-001 工具损坏上报和隔离、repair-dangerous-tool-temporary-isolation-001 危险工具临时隔离、repair-dangerous-tool-borrow-permission-001 危险工具借用权限 | repair_tool_damage_isolation | 是 | 是 | 是 | 无 |
| wave2_child_playing_nearby | pass | DG-0838 儿童远离工具区、DG-0837 危险工具临时隔离、DG-0509 工具用途说明卡 | repair-children-away-tool-zone-001 儿童远离工具区、repair-child-nonoperator-safety-card-001 儿童和非操作人员安全提示卡、repair-dangerous-tool-temporary-isolation-001 危险工具临时隔离、repair-dangerous-tool-borrow-permission-001 危险工具借用权限 | repair_child_tool_zone | 是 | 是 | 是 | 无 |
| wave2_uncertain_tool_urgent_task | pass | DG-0840 工具破损后禁用标签、DG-0837 危险工具临时隔离、DG-0272 设备外借和维修前清理 | repair-damaged-tool-disable-tag-001 工具破损后禁用标签、repair-damaged-tool-report-isolation-001 工具损坏上报和隔离、repair-dangerous-tool-temporary-isolation-001 危险工具临时隔离、repair-dangerous-tool-borrow-permission-001 危险工具借用权限 | repair_tool_damage_isolation | 是 | 是 | 是 | 无 |

## 5. 失败案例 Root Cause 分类

### wave2_low_light_small_cut

- query：光线很暗但是只是切一点点，会不会没关系？
- verdict：partial
- failures：未命中预期 Guide、未命中预期 Wiki
- root cause：Wiki 不可召回、query profile 缺失
- selected guides：DG-0546、DG-0714、DG-0709
- selected wiki：energy-basic-electrical-safety-index-001、energy-water-damaged-electrical-no-power-001、safety-safety-knowledge-009、safety-review-001、safety-safety-knowledge-004

### wave2_no_workbench_platform

- query：没有工作台，临时拿几块木板搭个平台维修可以吗？
- verdict：partial
- failures：未命中预期 Wiki
- root cause：Wiki 不可召回
- selected guides：DG-0839、DG-0834、DG-0304
- selected wiki：repair-knife-saw-clear-zone-001、repair-bystander-position-boundary-001、repair-floor-clutter-trip-check-001、repair-saw-cut-support-001、repair-temporary-clamp-fixture-001、repair-wet-wood-strength-check-001

### wave2_wobbly_table_tools

- query：临时桌子有点晃，但是还能放工具，需要处理吗？
- verdict：partial
- failures：未命中预期 Wiki
- root cause：Selector 排序问题
- selected guides：DG-0839、DG-0569、DG-0624
- selected wiki：repair-knife-saw-clear-zone-001、repair-bystander-position-boundary-001、repair-floor-clutter-trip-check-001、repair-fixing-and-support-001、repair-drinking-water-001、repair-hand-tool-care-001

### wave2_screws_blades_small_parts

- query：螺丝、刀片、小零件放一起容易丢，怎么办？
- verdict：partial
- failures：未命中预期 Guide
- root cause：Selector 排序问题
- selected guides：DG-0309、DG-0376、DG-0372
- selected wiki：repair-fastener-washer-storage-001、repair-screws-and-nuts-001、repair-screwdriver-camout-protection-001、repair-blade-dull-chip-check-001、repair-used-screw-nail-reuse-check-001、repair-fastener-001

## 6. 重点新增 Wiki Evidence 检查

- `repair-low-light-stop-line-001`：wave2_low_light_headlamp_saw、wave2_people_pass_repair_site、wave2_mixed_repair_materials、wave2_no_light_no_workbench
- `repair-dangerous-tool-temporary-isolation-001`：wave2_damaged_saw_to_lend、wave2_new_member_dangerous_tool、wave2_damaged_tool_temporary_use、wave2_child_playing_nearby、wave2_uncertain_tool_urgent_task
- `repair-children-away-tool-zone-001`：wave2_child_watching_repair、wave2_child_playing_nearby
- `repair-knife-saw-clear-zone-001`：wave2_low_light_headlamp_saw、wave2_bystander_handing_tools、wave2_saw_floor_clutter、wave2_one_saws_one_holds、wave2_no_workbench_platform、wave2_wobbly_table_tools、wave2_three_person_positions、wave2_people_pass_repair_site、wave2_mixed_repair_materials、wave2_no_light_no_workbench
- `repair-damaged-tool-disable-tag-001`：wave2_damaged_saw_to_lend、wave2_borrowed_tool_found_damaged、wave2_damaged_tool_temporary_use、wave2_uncertain_tool_urgent_task

## 7. 跨域与 Kiwix 检查

- 工具问题误入非维修领域：无
- 首位或强竞争证据跨域：无
- 外部依赖违规：0
- Kiwix 越权：0

## 8. 是否建议进入 Batch4-O Apply

- 当前未达到 field test 验收线。建议进入 Batch4-O Review / Apply，但只根据本报告 root cause 做最小修复。

## 9. 验证命令

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
  tests/test_retrieval_traceability.py \
  tests/test_retrieval_root_contract.py
```

## 10. 不应修复的问题

- 不修改 Wiki/Guide 正文或 Guide-Wiki 关联。
- 不修改 Retrieval Pipeline、Prompt、query profile、top_k、selector_candidate_limit、ranking 参数或 fallback 逻辑。
- 不因为单个 query 未命中就新增重复 Wiki。
