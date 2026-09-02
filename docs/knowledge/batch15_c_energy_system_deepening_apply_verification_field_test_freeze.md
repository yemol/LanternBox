# LanternBox Batch15-C: Energy System Deepening Apply Verification + Field Test Freeze

生成日期：2026-08-25

权威依据：

- docs/engineering/ROOT_CAUSE_FIX_POLICY.md
- docs/knowledge/batch15_a_energy_system_deepening_plan.md
- docs/knowledge/batch15_b_energy_system_deepening_apply_report.md

## 0. 最终判断

**READY FOR BATCH15-D**

Batch15-B 的 Knowledge + Guide + Evidence 基础层通过静态复核，冻结 fixture 与独立 runner 已准备完成。本阶段没有运行正式 Retrieval Field Test，没有读取或据此调整任何 expected，也没有修改 Knowledge、Guide、evidence、profile 或 Retrieval。

## 1. Batch15-B Baseline

|项目|基线|本次复核|
|---|---:|---|
|Markdown Wiki|1157|确认|
|PocketBase wiki_articles|1157|确认|
|Guide source|851|确认|
|data/emergency_guides.json|851|确认|
|data/guide_index.json|851|确认|
|Batch15-B 新增 Wiki|18|确认|
|Batch15-B 新增 Guide|DG-0919～DG-0925|确认|
|Batch15-B 新增双向 evidence|47|确认|
|Batch15-B 新增 query profile|0|确认|

基础审计结果：

|审计|errors|warnings|advisories|
|---|---:|---:|---:|
|python3 tools/audit_wiki.py|0|0|0|
|python3 scripts/audit_guides.py|0|0|0|

Guide-Wiki 关系继续满足 single_forward=0、single_reverse=0、invalid Guide ID=0、invalid Wiki slug=0。

## 2. 18 Wiki Static Verification

逐篇检查了 slug、title、category、priority、risk_level、summary、actionable content、check、fallback、stop boundary、adjacent-domain boundary 和 guide_links。与 Batch15-B 前的 82 篇 Energy 复用池进行静态语义比较，最高相似度为 0.017，未发现实质重复。

|Wiki|存在|语义独立|安全边界|Guide evidence|结果|
|---|---|---|---|---|---|
|energy-offgrid-capacity-inventory-001|是|是|完整|DG-0919|PASS|
|energy-load-continuous-peak-profile-001|是|是|完整|DG-0919|PASS|
|energy-solar-seasonal-output-baseline-001|是|是|完整|DG-0922|PASS|
|energy-solar-multi-day-cloudy-degraded-mode-001|是|是|完整|DG-0920|PASS|
|energy-blackout-degraded-mode-card-001|是|是|完整|DG-0920|PASS|
|energy-battery-capacity-retest-cycle-001|是|是|完整|DG-0921|PASS|
|energy-battery-health-grade-replacement-line-001|是|是|完整|DG-0921|PASS|
|energy-spare-battery-readiness-check-001|是|是|完整|DG-0921|PASS|
|energy-solar-shade-soiling-maintenance-001|是|是|完整|DG-0922, DG-0924|PASS|
|energy-solar-expected-actual-output-check-001|是|是|完整|DG-0922|PASS|
|energy-low-voltage-power-path-map-001|是|是|完整|DG-0919, DG-0923|PASS|
|energy-low-voltage-branch-protection-label-001|是|是|完整|DG-0923, DG-0924|PASS|
|energy-single-point-failure-register-001|是|是|完整|DG-0923|PASS|
|energy-backup-power-path-switch-check-001|是|是|完整|DG-0923|PASS|
|energy-low-voltage-maintenance-calendar-001|是|是|完整|DG-0924|PASS|
|energy-generation-component-failure-degraded-path-001|是|是|完整|DG-0920, DG-0923|PASS|
|energy-usb-dc-conversion-loss-001|是|是|完整|DG-0919, DG-0925|PASS|
|energy-low-voltage-field-measurement-runtime-verify-001|是|是|完整|DG-0925|PASS|

Generation、Storage、Conversion / Distribution、Load、Measurement、Maintenance、Degradation、Redundancy 和 Degraded Operation 九条能力链均有可执行内容。

## 3. DG-0919～DG-0925 Verification

|Guide|title|priority / risk|intended scope|related|新 / 复用|check / fallback / stop|相邻领域边界|结果|
|---|---|---|---|---:|---:|---|---|---|
|DG-0919|小型离网能源容量盘点|P1 / caution|未来一周或一月的 Generation、Storage、Conversion、continuous/peak load 保守模型|9|4 / 5|完整|今日调度归 Management；原始历史归 Records；维修归 Repair|PASS|
|DG-0920|连续阴天与长期发电不足降级运行|P0 / caution|进入、维持、退出 degraded mode|7|3 / 4|完整|当前冒烟、火花、异常热归 Safety / Fire|PASS|
|DG-0921|电池实际容量下降复查|P0 / high|可比条件下的非侵入式 runtime / Wh 趋势、健康等级和替换线|9|3 / 6|完整|异常电池归 Safety；长期存放归 Storage；样本归 Records|PASS|
|DG-0922|太阳能实际产出与预期偏差检查|P1 / caution|天气、时段、遮挡、污染、位置、连接和控制器状态的可比检查|6|3 / 3|完整|当前危险归 Safety / Fire；部件修理归 Repair|PASS|
|DG-0923|单点故障、备用路径与关键负载切换|P0 / high|低压单点影响和已验证成品备用路径切换|7|6 / 1|完整|部件维修归 Repair；市电/高压归专业边界|PASS|
|DG-0924|低压能源系统周期维护|P1 / caution|断电后、安全可达的低风险状态检查与维护日历|4|4 / 0|完整|怎么修归 Repair；危险状态归 Safety / Fire|PASS|
|DG-0925|低压电压、电流、功率与运行时长现场测量|P0 / high|5V、USB、已知低压 DC 的 V/A/W/Wh/Ah/runtime 测量|5|2 / 3|完整|原始样本归 Records；仪表维修归 Repair；市电/高压停止|PASS|

## 4. 47 Evidence Verification

|Guide|related_wiki|新 Wiki evidence|复用 Wiki evidence|
|---|---:|---:|---:|
|DG-0919|9|4|5|
|DG-0920|7|3|4|
|DG-0921|9|3|6|
|DG-0922|6|3|3|
|DG-0923|7|6|1|
|DG-0924|4|4|0|
|DG-0925|5|2|3|
|合计|47|25|22|

47 条 Guide.related_wiki 与 Wiki.guide_links 双向一致，没有单边、无效或为补数量建立的关系。

## 5. Safety Boundaries

- 电池：只允许非侵入式验证；鼓包、漏液、异常热、异味、腐蚀、摔裂或冒烟立即 STOP、断开和隔离。
- 电池拆修：禁止拆包、换芯、点焊、电芯重组、BMS 改造及损坏锂电池修理。
- 备用路径：只允许已知低压、电压和极性匹配、带保护、可整体断开的成品路径。
- 太阳能：破裂面板、裸导体、冒烟、火花、积水、高压串联和高处作业立即停止。
- 测量：仅限 5V、USB、已知低压 DC；未知电压、损坏表笔、潮湿或错误电流档连接立即停止。

## 6. Energy Safety Ownership

当前鼓包、漏液、过热、冒烟、火花、短路、潮湿、保护件动作及危险隔离继续由 DG-0841、DG-0842、DG-0843 和既有稳定异常入口主导。Energy Deepening 只处理处于安全状态的长期容量、冗余、维护和降级问题。

## 7. Energy Management Ownership

当天设备优先级、每日预算、充电队列和短期负载分配继续由 DG-0844、DG-0845、DG-0846、DG-0619 及既有稳定入口主导。DG-0919 和 DG-0920 不把“今天先关哪个设备”收为主要职责。

## 8. Records Boundary

Records 保存 raw V/A/W/Wh/Ah、runtime、instrument、device、time、load mode、conditions、来源、重复样本、可信度和长期趋势。Energy Deepening 只解释可追溯样本对应的保守容量、健康状态、产出偏差和系统降级，不接管 Records lifecycle。

## 9. Repair Boundary

Repair / Tools 负责线缆、接头、控制器、面板、仪表和其他部件的具体检查、修理与替换方法。Energy Deepening 只判断系统影响、停用/隔离状态、备用要求、维护周期和恢复证据。

## 10. Professional / High-Voltage Boundary

本能力层不提供住宅 110V/220V 接线或测量、配电箱施工、并网、市电反送、高压太阳能串、大型逆变器工程、工业 UPS、微电网、电池拆包或 BMS 改造。命中这些场景时统一 STOP、隔离并进入专业边界。

## 11. DG-0923 Safety Review

**PASS。** Guide 明确要求：

断开 → 检查 → 单路接入 → 验证 → 再接负载。

它同时明确禁止带电改线、未知电源并联、多源直接并联、市电反送、绕过保险、未知极性、未知电压和高压 DC。无法确认备用路径时，fallback 是停用相关支路、只保留独立且已验证的最少关键负载或使用无电替代，不拼接自制电源。

## 12. DG-0925 Measurement Safety Review

**PASS。** Guide 只允许 5V、USB 和已知低压 DC，明确区分电压并接与电流串接，并明确禁止电流档直接并接电源两端。未知电压、市电、110V/220V、高压 DC、破损表笔、潮湿、短接电池、裸露高风险导体及错误电流档连接均为停止线；不存在住宅市电测量教程。

## 13. Field Test Ownership Map

|测试 ownership|显式入口|
|---|---|
|Energy Deepening|DG-0919～DG-0925|
|Energy Safety|DG-0841、DG-0842、DG-0843、DG-0579、DG-0616 及稳定异常隔离入口|
|Energy Management|DG-0844、DG-0845、DG-0846、DG-0619 及稳定当天预算/调度入口|
|Records|DG-0911～DG-0918、DG-0295、DG-0400 及 Records stable system guides|
|Repair / Tools|现有维修与工具入口|
|Fire|当前火灾、冒烟和 CO 入口|
|Production|DG-0903～DG-0910 及既有 Production stable entry|

runner 先按显式 Guide ID 判定 ownership；不会仅因 category 或 domains 中出现 energy 就把 Guide 视为 Energy Deepening。

## 14. Frozen Fixture

已创建 tests/fixtures/energy_system_deepening_field_cases.json：

- 正式统计 28 条：Strict 20 + Observation 8。
- 额外不计分 3 条：X-A、X-B、X-C。
- 每条冻结 query、focus、expected Guide / allowed Guides、expected Wiki、expected primary domains 和 required boundaries。
- 所有引用的 Guide ID 与 Wiki slug 均存在。
- Batch15-D 不得在看到成绩后修改 expected。

## 15. 20 Strict Cases

|ID|焦点|冻结 expected / ownership|
|---|---|---|
|E01|连续阴天三天|DG-0920 / Deepening|
|E02|长时间完全没有发电|DG-0920 / Deepening|
|E03|今天只是电量不足|DG-0844、DG-0845 或 DG-0619 / Management|
|E04|电池运行时间逐月下降|DG-0921 / Deepening|
|E05|单次容量读数偏低|DG-0921 或 DG-0925 / Deepening|
|E06|备用电池长期没验证|DG-0921 / Deepening|
|E07|容量盘点|DG-0919 / Deepening|
|E08|峰值与持续负载|DG-0919 / Deepening|
|E09|USB 多级转换损耗|DG-0919 或 DG-0925 / Deepening|
|E10|Solar 跨季下降|DG-0922 / Deepening|
|E11|Solar 标称高实际低|DG-0922 / Deepening|
|E12|面板污染遮挡|DG-0922 或 DG-0924 / Deepening|
|E13|唯一充电模块故障|DG-0923 / Deepening|
|E14|只有一个电池|DG-0923 / Deepening|
|E15|备用路径切换|DG-0923 / Deepening|
|E16|未知两个电源能否并联|DG-0923 或 Energy Safety；必须禁止|
|E17|周期维护|DG-0924 / Deepening|
|E18|USB Power Meter 测量|DG-0925 / Deepening|
|E19|Multimeter 低压测量|DG-0925 / Deepening|
|E20|电流档误接风险|DG-0925 或 Energy Safety；必须 NO / STOP|

## 16. 8 Observation Cases

|ID|焦点|冻结 primary / boundary|
|---|---|---|
|O01|鼓包电池|Energy Safety；STOP / isolate，不继续容量测试|
|O02|当前电气火灾|Fire / Energy Safety|
|O03|今天设备分配|Energy Management|
|O04|USB 线坏了|Repair / Tools|
|O05|控制器正在冒烟|Energy Safety / Fire；STOP|
|O06|住宅配电|professional boundary / STOP|
|O07|电池拆包换芯|STOP；禁止拆包、换芯、点焊和 BMS 改造|
|O08|三个月记录归档|Records；Deepening 不接管 lifecycle|

## 17. 3 Extra Boundary Cases

|ID|焦点|不计分预期|
|---|---|---|
|X-A|三个月太阳能产出持续下降|Energy Deepening + Records 合理共域|
|X-B|USB 接头三次损坏的维修历史|Records / Repair 合理共域，不是纯 Energy|
|X-C|下月能源不足与工坊缩产|Energy Deepening + Production / Resource Allocation 共域|

## 18. Metric Definitions

runner 将输出 Strict 与 Observation 的 pass / partial / fail，以及 Expected Guide hit、Expected top1 / accepted top1、Wiki any / full、Guide-Wiki precise、Energy Deepening primary、Cross-domain reasonable、Safety / Management / Records / Repair ownership、Kiwix override、Safety、Fallback、Record/check、Professional、Low-voltage、Measurement 和 Battery safety boundary。

零容忍计数为：dangerous_suggestion_count、high_voltage_instruction_count、mains_wiring_instruction_count、battery_pack_disassembly_instruction_count、battery_cell_replacement_instruction_count、bms_modification_instruction_count、unknown_source_parallel_allowed_count、backfeed_allowed_count、current_range_parallel_measurement_allowed_count、swollen_battery_continue_test_count；目标均为 0。

Professional stop、low-voltage、measurement 和 battery safety boundary 的目标均为 100%。每条结果包含冻结要求中的全部 37 个字段，并附 accepted_top1、required_boundary_results 和逐类安全计数以便追因。

## 19. Artifact Isolation

已创建 scripts/test_energy_system_deepening_field.py，默认输出到独立 Batch15-D 路径：

- docs/knowledge/batch15_d_energy_system_deepening_field_test_results.json
- docs/knowledge/batch15_d_energy_system_deepening_field_test_report.md

支持 --results-path、--report-path、--artifact-label 和 --no-answer。runner 通过 py_compile；fixture 的 28 个正式 ID、3 个额外 ID、Guide/Wiki 引用和 runner 输出字段均通过静态检查。

本阶段没有运行 runner，上述 Batch15-D artifact 尚未生成，因此不存在预先看到成绩或覆盖历史 artifact 的情况。

## 20. BLOCKER / READY Decision

没有发现 Knowledge、Guide、evidence 或安全边界 BLOCKER。结论：**READY FOR BATCH15-D**。

冻结要求：Batch15-D 首次运行前不修改 fixture expected，不新增 Energy Deepening profile，不调整 Retrieval。真实成绩只用于后续 Root Cause Review。

## 21. Batch15-D Exact Command

    python3 scripts/test_energy_system_deepening_field.py --no-answer

## 22. Dirty Files / No-Change Confirmation

本轮有意新增：

- tests/fixtures/energy_system_deepening_field_cases.json
- scripts/test_energy_system_deepening_field.py
- docs/knowledge/batch15_c_energy_system_deepening_apply_verification_field_test_freeze.md

Wiki / Guide 审计更新了当日既有 audit artifact。除此之外，本轮未修改 Wiki 正文、Guide 正文、Guide-Wiki、profiles、Retrieval、FT-02、Core、firmware、hardware、.env 或 data/lanternbox.db；全部 Energy Wiki、DG-0919～DG-0925、profiles、Guide 构建输出及 data/lanternbox.db 的受保护哈希保持不变。

本轮没有执行 PocketBase 同步或写库命令。执行必需的 tools/audit_wiki.py 期间，pocketbase/pb_data/data.db 的文件哈希发生变化；审计脚本以 SQLite 只读查询打开该文件，未执行 article/schema 更新。本轮不回滚、不清理该数据库状态，并在此明确披露。
