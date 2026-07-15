# Batch5-G Energy Management Retrieval Root Cause Review

生成日期：2026-07-15

## 1. 范围

本阶段只复盘 Batch5-F Energy Management Retrieval Field Test 的检索证据表现，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema、测试 fixture 或测试脚本。

参考文件：

- `docs/knowledge/batch5_f_energy_management_field_test_report.md`
- `docs/knowledge/batch5_f_energy_management_field_test_results.json`
- `docs/knowledge/batch5_e_energy_management_apply_report.md`
- `docs/knowledge/batch5_e_energy_management_plan.md`
- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

## 2. Batch5-F 汇总

- 用例总数：15
- strict / observation：12 / 3
- pass / partial / fail：5 / 8 / 2
- Guide 命中率：83.3%（含 allowed secondary）
- 主 Guide 命中率：33.3%
- Wiki 命中率：33.3%
- Guide-Wiki 精准组合率：50.0%
- safety / fallback / record：100% / 100% / 100%
- dangerous suggestion：0
- P0 电气安全误触发：0
- 跨域竞争：2

核心结论：Batch5-E 的内容本身已经同步，并且安全边界、降级方案、记录复查在进入 evidence 时表现稳定；未达 stable 的主因是能源管理 query profile 缺口和正确 Guide/Wiki evidence 没有稳定进入 selected evidence，不是 Wiki 正文质量问题。

## 3. Partial / Fail Case 复盘

| case | 当前 selected Guide | 当前 selected Wiki 摘要 | 预期 Guide | 是否命中 allowed secondary | 结论 | 是否建议 Apply | 最小 Apply |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `energy_three_day_blackout_charge_priority` | DG-0563, DG-0846, DG-0338 | 旧移动电源优先级、旧停电分级、DG-0846 相关 Wiki | DG-0844 | 是，DG-0846/DG-0563 | partial。问题主体是三天断电后的每日能源预算，但旧“移动电源关键设备优先级”抢主位；DG-0844 仅候选第 6。 | 是 | 新增 `energy_daily_budget_priority` profile，把“停电多日/设备都没电/优先充什么”拉向 DG-0844；必要时让 DG-0844 相关 Wiki 优先。 |
| `energy_charging_queue_many_devices` | DG-0564, DG-0563, DG-0109 | 旧太阳能基础、旧移动电源安全 Wiki | DG-0846 | 是，DG-0564/DG-0617 属允许补充；DG-0846 候选第 4 | partial。query 明确是“一个口、多设备、怎么排队”，但太阳能和移动电源旧 Guide 压住充电队列。 | 是 | 新增 `energy_charging_queue_management` profile，使“一个口/排队/多个设备/轮流充”优先 DG-0846。 |
| `energy_port_occupied_too_long` | DG-0563, DG-0107, DG-0845 | 旧移动电源优先级、关键设备最低线 | DG-0846 | 否；DG-0846 候选第 4 | partial。正确 Guide 已在候选但掉出 selected top3；预期 Wiki 只能靠 DG-0846 related_wiki 进入。 | 是 | 同 `energy_charging_queue_management`；不建议新 Guide。 |
| `energy_cloudy_solar_target` | DG-0564, DG-0563, DG-0846 | 旧太阳能基础、旧移动电源安全、充电队列 Wiki | DG-0617 | 是，DG-0564/DG-0844/DG-0845 允许；DG-0617 候选第 5 | partial。主题是阴天弱输出下的太阳能降级和目标选择；DG-0617 未进 top3，新增太阳能管理 Wiki 被旧 DG-0564 的 related_wiki 压住。 | 是 | 新增 `energy_solar_low_output_management` profile；同时评估 DG-0564 是否应补关联少量 Batch5-E 太阳能管理 Wiki。 |
| `energy_solar_daytime_schedule` | DG-0564, DG-0617, DG-0846 | 旧太阳能基础、旧电池基础 | DG-0617 | 是，DG-0564 允许 | partial。DG-0617 已选中，但其 related_wiki 中旧基础条目排在前面，Batch5-E 太阳能排程 Wiki 在 related 列表后段，未进入 selected 6。 | 是 | Guide-Wiki evidence priority 问题：不扩大 top_k，优先调整 DG-0617 相关 Wiki 顺序或最小关联策略。 |
| `energy_solar_two_days_low_output` | DG-0617, DG-0564, DG-0562 | 旧电池基础、旧充电线、旧能量记录 | DG-0844 | 是，DG-0617/DG-0564 允许 | partial。多日低产出后需要“预算复盘 + 夜间照明调整”，但太阳能旧基础 related_wiki 先进入，`energy-cloudy-solar-downgrade-001` 虽在 DG-0617 related 中但未进 selected 6。 | 是 | `energy_solar_low_output_management` profile + DG-0617 相关 Wiki evidence priority。 |
| `energy_one_light_at_night` | DG-0561, DG-0113, DG-0541 | 手机省电 Wiki | DG-0845 | 否 | partial。query 是夜间最低照明和不能全黑的能源安全边界；被手机低电量和夜间异响 Guide 抢走。 | 是 | `energy_daily_budget_priority` 可覆盖“省电/只留一盏灯/不能全黑”；或单独在同 profile 中加入夜间照明最低线 anchor，优先 DG-0845。不建议新增 Guide。 |
| `energy_nonessential_suspend` | DG-0845, DG-0561, DG-0563 | 关键设备最低线、手机省电、旧移动电源优先级 | DG-0619 | 是，DG-0844/DG-0845 允许 | partial。方向安全，但“娱乐/舒服一点的设备先停掉”应触发低电量断负载；DG-0619 未进入 top8。 | 是 | 新增或扩展 `energy_daily_budget_priority` 的负载断开词，或独立但不超过 3 个 profile 的 `energy_load_disconnect_priority`；优先复用 DG-0619，不新建 DG-0848。 |
| `energy_day_end_review` | DG-0690, DG-0709, DG-0729 | 团队复盘、接触复盘、心理复盘 Wiki | DG-0844 | 否 | fail。query 明确“用电情况很乱/晚上复盘/明天调整”，但没有能源管理 profile，通用团队复盘完全抢主位。数据已存在，不是数据缺口。 | 是 | `energy_daily_budget_priority` 加入“用电复盘/明天调整/能源复盘/交班摘要”，优先 DG-0844。 |
| `energy_core_terminal_low_power_window` | DG-0582, DG-0636, DG-0548 | 通信固定开机窗口、断网通信、撤离准备 | DG-0845 | 否；DG-0107 候选第 18 | fail，但属于观察型合理 partial。query 中 Core/终端/短时间开机/记录会自然触发通信与设备窗口；Batch5-E 明确暂未新增 `energy-low-power-core-terminal-window-001`。 | 暂不建议作为 Batch5-H 主修 | 记录为后续 P1/P2 缺口。若未来要修，应新增专门 Core/终端低电量 Wiki 或 Guide，而不是压低通信 Guide。 |

## 4. Pass Case 备注

以下 case 通过，但仍暴露边界：

- `energy_daily_budget_no_percent`：DG-0844 稳定命中，说明“能源预算”显式词有效；无 profile 也能靠文本匹配进入。
- `energy_key_device_minimum_line`：DG-0845 稳定命中，最低电量线表达清楚。
- `energy_shared_power_bank_unknown_return`：DG-0846 稳定命中，说明“共享移动电源 + 借还 + 没记录”已经足够触发新 Guide。
- `energy_low_power_disconnect_order`：脚本判定 pass，因为命中 allowed DG-0845 且有 `energy-essential-load-reserve-001`，但主语义仍应更接近 DG-0619。后续 Apply 可以顺带用负载断开词改善，不建议单独扩大修复范围。
- `energy_battery_rotation_lost_track`：DG-0336 / DG-0618 和新增电池轮换 Wiki 组合准确，不需要修。

## 5. 新增 Wiki Evidence 可达性检查

23 篇 Batch5-E 新增 Wiki 均已存在于 Markdown 和 PocketBase，且可读取 content。分类未新增，主要落在“能源”和“信息保存与长期重建”。因此本轮不能把未命中简单归为数据缺口。

| Wiki | Markdown/PB | Guide 关联 | 当前问题类型 |
| --- | --- | --- | --- |
| `energy-daily-energy-budget-001` | 存在 / 已同步 | DG-0844 | 可通过 DG-0844 进入；DG-0844 未被选中时不可达。 |
| `energy-critical-device-minimum-line-001` | 存在 / 已同步 | DG-0845 | DG-0845 命中时稳定进入。 |
| `energy-device-power-priority-tier-001` | 存在 / 已同步 | DG-0844 | DG-0844 未进 top3 时缺失。 |
| `energy-charging-queue-schedule-001` | 存在 / 已同步 | DG-0846 | DG-0846 命中时可进入；排队类 query 中 DG-0846 排名不够稳定。 |
| `energy-low-battery-power-window-001` | 存在 / 已同步 | DG-0845 | DG-0845 命中时可进入；Core/终端场景被通信 Guide 压住。 |
| `energy-solar-daytime-charge-schedule-001` | 存在 / 已同步 | DG-0617 | 在 DG-0617 related_wiki 后段，常被旧太阳能基础条目挤出 selected 6。 |
| `energy-cloudy-solar-downgrade-001` | 存在 / 已同步 | DG-0617 | 同上；阴天低输出 query 中 DG-0617 排名也不稳定。 |
| `energy-night-lighting-runtime-plan-001` | 存在 / 已同步 | DG-0845 | 夜间照明 query 未稳定触发 DG-0845。 |
| `energy-backup-battery-rotation-ledger-001` | 存在 / 已同步 | DG-0336, DG-0618 | 命中准确。 |
| `energy-charge-discharge-handover-log-001` | 存在 / 已同步 | DG-0846 | DG-0846 命中时可进入。 |
| `energy-low-battery-load-disconnect-order-001` | 存在 / 已同步 | DG-0619 | DG-0619 未稳定进入候选前列；不是缺关联。 |
| `energy-shared-power-bank-borrow-return-001` | 存在 / 已同步 | DG-0846 | 命中准确。 |
| `energy-device-runtime-estimate-card-001` | 存在 / 已同步 | DG-0844 | 命中准确。 |
| `energy-essential-load-reserve-001` | 存在 / 已同步 | DG-0845 | 命中准确，但会让 DG-0845 覆盖部分本该属于 DG-0619 的场景。 |
| `energy-nonessential-device-suspend-list-001` | 存在 / 已同步 | DG-0619 | DG-0619 未稳定进入候选前列。 |
| `energy-day-end-power-review-001` | 存在 / 已同步 | DG-0844 | 能源复盘 query 被通用团队复盘抢主位。 |
| `energy-solar-charge-target-selection-001` | 存在 / 已同步 | DG-0617 | 在 DG-0617 related_wiki 后段，未进入 selected 6。 |
| `energy-charging-port-rotation-rule-001` | 存在 / 已同步 | DG-0846 | DG-0846 未进 top3 时不可达。 |
| `energy-wh-mah-local-estimate-001` | 存在 / 已同步 | 无 | 作为背景估算单元合理；无需强行关联。 |
| `energy-device-consumption-baseline-001` | 存在 / 已同步 | 无 | 记录背景单元，未进入本批主要 evidence；暂不修。 |
| `energy-solar-weather-pattern-log-001` | 存在 / 已同步 | DG-0617 | 在 DG-0617 related_wiki 后段，未进入 selected 6。 |
| `energy-battery-aging-capacity-note-001` | 存在 / 已同步 | DG-0336, DG-0618 | 命中准确。 |
| `general-energy-handover-summary-001` | 存在 / 已同步 | DG-0844 | DG-0844 未命中时不可达。 |

独立 Wiki 候选层面，多数新增 Wiki 在 Batch5-F query 下没有进入 `fetch_wiki_candidates` top50；这说明它们主要依赖 Guide related_wiki 进入 evidence。下一步不能靠扩大 top_k 掩盖，应优先让正确 Guide 被选中，并处理已选 Guide 内 related_wiki 的 evidence priority。

## 6. Guide 归属与重叠分析

### DG-0844 每日能源预算

适合承接：

- 停电多日后“手机、手电、收音机、移动电源先充谁”
- 看不到百分比时的低资源预算
- 多日太阳能低输出后的明日调整
- 用电复盘和交班摘要

当前问题：显式“能源预算”命中稳定；没有“预算”字样时，旧 DG-0563、DG-0107、DG-0690 更容易抢位。建议新增能源预算 profile，而不是新 Guide。

### DG-0845 关键设备最低电量线

适合承接：

- 手机、照明最低线
- 低电量开机窗口
- 夜间最低照明
- Core/终端短时开机的基础策略

当前问题：低电量和手机词会把 query 推向 DG-0561、DG-0582、DG-0636；夜间词会推向夜间安全 Guide。需要 profile 帮助“最低电量线 / 不能全黑 / 保留照明”进入 DG-0845。Core/终端观察 case 暂不应硬修。

### DG-0846 充电队列和排班

适合承接：

- 一个充电口、多设备排队
- 共享移动电源借还
- 充电口占用时间过长
- 充放电交接记录

当前问题：共享移动电源借还已经稳定；“太阳能板只有一个口”会被 DG-0564/DG-0109 太阳能旧 Guide 抢位；“关键设备怎么办”会被 DG-0563/DG-0107 抢位。建议 profile 覆盖“一个口 / 占着口 / 排队 / 轮换 / 多设备”。

### DG-0617 太阳能充电白天排程

Batch5-E 已经把以下 Wiki 关联到 DG-0617：

- `energy-solar-daytime-charge-schedule-001`
- `energy-cloudy-solar-downgrade-001`
- `energy-solar-charge-target-selection-001`
- `energy-solar-weather-pattern-log-001`

当前问题不是缺关联，而是 DG-0617 related_wiki 旧基础条目太多，新管理条目排在后段，selected evidence 前 6 常被旧条目占满。建议 Batch5-H 只做最小 evidence priority 调整：把这 4 个管理条目在 DG-0617 的 related_wiki 中前移，或通过 profile 让 DG-0617 在低输出管理 query 中成为主 Guide。

### DG-0619 负载过大时断开排序

Batch5-E 已经把以下 Wiki 关联到 DG-0619：

- `energy-low-battery-load-disconnect-order-001`
- `energy-nonessential-device-suspend-list-001`

当前问题不是缺关联，而是 DG-0619 很少进入候选前列；“电量不够/哪些必须留下”被 DG-0845 合理吸收，“娱乐设备先停掉”也未触发 DG-0619。建议在 profile 中加入“非必要设备 / 娱乐设备 / 舒适设备 / 先停掉 / 断开负载”的 anchor。暂不新增 DG-0848。

## 7. 跨域竞争分析

### power / communication

`energy_core_terminal_low_power_window` 被 DG-0582、DG-0636 等通信固定开机窗口抢主位。由于该 case 是 observation，且 Batch5-E 明确没有新增 Core/终端低电量专门 Wiki，当前属于合理 partial。不要把通信窗口 Guide 降权；未来如果要修，应新增或规划专门的 Core/终端低电量能源管理单元。

### power / computer

本轮没有明确 computer Guide 抢主位，但 Core/终端词会把 query 推向通信/设备运行窗口。当前不建议为了观察 case 修改 computer 或 terminal 领域。

### power / safety

`energy_one_light_at_night` 被夜间安全相关 Guide 参与竞争，但本轮没有出现危险建议。安全应作为补充，不应压倒“夜间最低照明续航安排”。建议通过 DG-0845 profile 处理，而不是压低夜间安全。

### power / solar

太阳能场景的主要竞争不是跨域，而是旧太阳能 Guide 与新太阳能管理 Wiki 的 evidence priority。DG-0617 复用方向正确，暂不需要新增 DG-0847；优先调整 profile 和 related_wiki 前后顺序。

## 8. 合理 Partial 清单

- `energy_core_terminal_low_power_window`：观察型 case。本批没有新增 `energy-low-power-core-terminal-window-001`，通信固定开机窗口抢主位可解释。暂不进入 Batch5-H 主修范围。
- `energy_low_power_disconnect_order`：脚本已判 pass，但主 Guide 不是 DG-0619。可随负载断开 profile 一并改善，不单独修。
- `energy_battery_rotation_lost_track`：通过，不修。

## 9. Batch5-H 最小 Apply 建议

建议进入 Batch5-H Apply，但范围要小。

### 9.1 建议新增 profile，最多 3 个

1. `energy_daily_budget_priority`

覆盖：

- 停电多日
- 设备都没电
- 今天/明天怎么安排用电
- 优先给什么充电
- 用电复盘
- 晚上复盘明天调整
- 只留一盏灯 / 不能全黑 / 最低照明

优先 Guide：

- DG-0844
- DG-0845

优先 Wiki：

- `energy-daily-energy-budget-001`
- `energy-device-power-priority-tier-001`
- `energy-essential-load-reserve-001`
- `energy-day-end-power-review-001`
- `general-energy-handover-summary-001`
- `energy-night-lighting-runtime-plan-001`

2. `energy_charging_queue_management`

覆盖：

- 只有一个充电口
- 多设备排队
- 一直占着充电口
- 轮换
- 共享移动电源借还
- 充放电交接

优先 Guide：

- DG-0846

优先 Wiki：

- `energy-charging-queue-schedule-001`
- `energy-charging-port-rotation-rule-001`
- `energy-charge-discharge-handover-log-001`
- `energy-shared-power-bank-borrow-return-001`

3. `energy_solar_low_output_management`

覆盖：

- 阴天
- 输出很弱
- 连续两天没充进去
- 有几小时太阳
- 先充手机还是移动电源
- 太阳能白天排程

优先 Guide：

- DG-0617
- DG-0564 作为补充
- DG-0844 / DG-0845 作为多日低输出后的预算补充

优先 Wiki：

- `energy-cloudy-solar-downgrade-001`
- `energy-solar-charge-target-selection-001`
- `energy-solar-daytime-charge-schedule-001`
- `energy-solar-weather-pattern-log-001`
- `energy-night-lighting-runtime-plan-001`

负载断开可以先并入 `energy_daily_budget_priority` 或在不超过 3 个 profile 的前提下扩展该 profile，不建议新增第四个 profile。

### 9.2 建议 Guide-Wiki 关系 / 顺序调整

只建议必要项：

- DG-0617 已有关联 4 个 Batch5-E 太阳能管理 Wiki，不需要新增关联；建议把这 4 个从 related_wiki 后段前移到旧基础条目前面。
- DG-0619 已有关联 `energy-low-battery-load-disconnect-order-001` 和 `energy-nonessential-device-suspend-list-001`，不需要新增关联；如 Apply 阶段确认 selected evidence 仍挤不进，再考虑前移这 2 个。
- DG-0844 / DG-0845 / DG-0846 的关联数量和主题边界当前合理，不建议扩成大而全。

### 9.3 是否需要新增 DG-0847 / DG-0848

暂不建议。

- 太阳能排程可由 DG-0617 承接，问题是 profile 和 related_wiki 顺序。
- 低电量断负载可由 DG-0619 承接，问题是入口词没有稳定触发。
- 新增 Guide 会增加 Guide 设计重叠，不符合最小修复原则。

## 10. 不建议修改内容

- 不扩大 top_k。
- 不扩大 selector limit。
- 不修改 Prompt。
- 不修改 Retrieval Pipeline。
- 不新增大量 Guide。
- 不新增重复 Wiki。
- 不把通信窗口 Guide 降权。
- 不把 P0 电气安全 profile 扩到能源管理问题。
- 不用 Kiwix 或最终回答补丁掩盖 Guide/Wiki evidence 缺失。
- 不为观察型 Core/终端低电量 case 硬编码特殊规则。

## 11. 是否建议进入 Apply

建议进入 Batch5-H Minimal Apply。

最大修改范围建议：

- `data/retrieval_query_profiles.json`
- 如确认为 evidence priority 必要：`data/guides/power/DG-0617.json`，可能包括 `data/guides/power/DG-0619.json`
- 对应 profile contract test 文件
- Apply 报告和回归结果文件

预计新增 profile 数量：3 个以内。

预计新增 Guide 数量：0。

预计新增 Wiki 数量：0。

预计新增关联数量：0；如需调整，只做 related_wiki 顺序前移，不新增大批关联。

Energy Management v0.1 架构可以保持不变。下一步应修“能源管理入口”和“已关联 Wiki 的 evidence priority”，不是扩内容或改 Pipeline。
