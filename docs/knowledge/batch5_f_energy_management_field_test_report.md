# Batch5-F Energy Management Retrieval Field Test Report

生成时间：2026-07-14T17:26:08.441270+00:00

## 1. 测试范围

本阶段只测试 Batch5-E 新增能源管理 Guide/Wiki 是否进入本地 Retrieval evidence。脚本默认不调用 LLM，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 PocketBase schema。

覆盖：每日能源预算、关键设备最低电量线、充电队列和排班、太阳能弱输出排程、低电量断负载、夜间照明、能源记录和交接。

## 2. 汇总

- 用例总数：15
- strict / observation：12 / 3
- pass / partial / fail：14 / 1 / 0
- Guide 命中率（严格用例，含 allowed secondary）：100.0%
- 主 Guide 命中率（严格用例，仅 expected）：91.7%
- Wiki 命中率（严格用例）：91.7%
- Guide-Wiki 精准组合率（严格用例）：100.0%
- safety boundary 覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- 危险建议数量：0
- P0 电气安全误触发：0
- 跨域竞争数量：0

## 3. Case 明细

| case | type | verdict | Guide | Wiki | profiles | safety | fallback | record | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| energy_three_day_blackout_charge_priority | strict | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |
| energy_daily_budget_no_percent | strict | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |
| energy_key_device_minimum_line | strict | pass | DG-0845 关键设备最低电量线、DG-0561 手机低电量：保通信模式、DG-0113 手机省电模式 | energy-critical-device-minimum-line-001 关键设备最低电量线、energy-essential-load-reserve-001 关键负载预留量、energy-low-battery-power-window-001 低电量设备开机窗口、energy-night-lighting-runtime-plan-001 夜间照明续航安排、energy-phone-002 手机省电为什么优先短信 | 无 | 是 | 是 | 是 | 无 |
| energy_charging_queue_many_devices | strict | pass | DG-0846 充电队列和排班、DG-0337 充电优先级、DG-0336 电池轮换使用 | energy-charging-queue-schedule-001 充电队列和排班、energy-charge-discharge-handover-log-001 充放电交接记录、energy-shared-power-bank-borrow-return-001 共享移动电源借用归还、energy-charging-port-rotation-rule-001 充电口轮换规则、energy-backup-battery-rotation-ledger-001 备用电池轮换台账、energy-battery-aging-capacity-note-001 电池老化和可用容量记录 | energy_charging_queue_management | 是 | 是 | 是 | 无 |
| energy_shared_power_bank_unknown_return | strict | pass | DG-0846 充电队列和排班、DG-0337 充电优先级、DG-0543 夜间守夜与异常记录 | energy-charging-queue-schedule-001 充电队列和排班、energy-charge-discharge-handover-log-001 充放电交接记录、energy-shared-power-bank-borrow-return-001 共享移动电源借用归还、energy-charging-port-rotation-rule-001 充电口轮换规则、safety-review-003 安全记录的复盘价值、energy-device-consumption-baseline-001 设备耗电基线记录 | energy_charging_queue_management | 是 | 是 | 是 | 无 |
| energy_port_occupied_too_long | strict | pass | DG-0846 充电队列和排班、DG-0337 充电优先级、DG-0616 充电宝鼓包发热停用隔离 | energy-charging-queue-schedule-001 充电队列和排班、energy-charge-discharge-handover-log-001 充放电交接记录、energy-shared-power-bank-borrow-return-001 共享移动电源借用归还、energy-charging-port-rotation-rule-001 充电口轮换规则、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题 | energy_charging_queue_management | 是 | 是 | 是 | 无 |
| energy_cloudy_solar_target | strict | pass | DG-0617 太阳能充电白天排程、DG-0564 太阳能充电：排程和保护、DG-0563 移动电源：关键设备优先级 | energy-solar-daytime-charge-schedule-001 太阳能白天补电排程、energy-cloudy-solar-downgrade-001 阴天太阳能降级安排、energy-solar-charge-target-selection-001 日照不足时充电目标选择、energy-solar-weather-pattern-log-001 太阳能天气产出记录、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题 | energy_solar_low_output_management | 是 | 是 | 是 | 无 |
| energy_solar_daytime_schedule | strict | pass | DG-0617 太阳能充电白天排程、DG-0564 太阳能充电：排程和保护、DG-0846 充电队列和排班 | energy-solar-daytime-charge-schedule-001 太阳能白天补电排程、energy-cloudy-solar-downgrade-001 阴天太阳能降级安排、energy-solar-charge-target-selection-001 日照不足时充电目标选择、energy-solar-weather-pattern-log-001 太阳能天气产出记录、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题 | energy_solar_low_output_management | 是 | 是 | 是 | 无 |
| energy_solar_two_days_low_output | strict | pass | DG-0617 太阳能充电白天排程、DG-0564 太阳能充电：排程和保护、DG-0562 电池不足：照明分级使用 | energy-solar-daytime-charge-schedule-001 太阳能白天补电排程、energy-cloudy-solar-downgrade-001 阴天太阳能降级安排、energy-solar-charge-target-selection-001 日照不足时充电目标选择、energy-solar-weather-pattern-log-001 太阳能天气产出记录、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题 | energy_solar_low_output_management | 是 | 是 | 是 | 无 |
| energy_low_power_disconnect_order | strict | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |
| energy_one_light_at_night | strict | partial | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | selector 问题 |
| energy_nonessential_suspend | strict | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |
| energy_day_end_review | observation | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |
| energy_battery_rotation_lost_track | observation | pass | DG-0336 电池轮换使用、DG-0706 交换内容记录、DG-0618 电池混放后的编号轮换 | energy-backup-battery-rotation-ledger-001 备用电池轮换台账、energy-battery-aging-capacity-note-001 电池老化和可用容量记录、safety-review-001 交换后复盘和物资清点、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题、energy-battery-003 备用电池封存和轮换原则 | 无 | 是 | 是 | 是 | 无 |
| energy_core_terminal_low_power_window | observation | pass | DG-0844 每日能源预算、DG-0619 负载过大时断开排序、DG-0845 关键设备最低电量线 | energy-daily-energy-budget-001 每日能源预算表、energy-device-power-priority-tier-001 设备供电优先级分层、energy-device-runtime-estimate-card-001 设备续航估算卡、energy-day-end-power-review-001 每日用电复盘、general-energy-handover-summary-001 能源交班摘要、energy-low-battery-load-disconnect-order-001 低电量断开负载顺序 | energy_daily_budget_priority | 是 | 是 | 是 | 无 |

## 4. 逐条复盘

### energy_three_day_blackout_charge_priority

- query：停电三天了，手机、手电、收音机和移动电源都没多少电，应该优先给什么设备充电？
- 类型：strict
- focus：能源预算主导，通信作为负载补充，不要让通信窗口完全抢主位。
- verdict：pass
- expected Guide：DG-0844
- allowed secondary：DG-0107、DG-0563、DG-0845、DG-0846
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_daily_budget_no_percent

- query：很多设备看不到准确电量百分比，今天怎么做能源预算？
- 类型：strict
- focus：低资源估算，不要求精密计算。
- verdict：pass
- expected Guide：DG-0844
- allowed secondary：无
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_key_device_minimum_line

- query：手机和照明都快没电了，最低要给它们留多少，什么时候不能再用了？
- 类型：strict
- focus：关键设备最低电量线。
- verdict：pass
- expected Guide：DG-0845
- allowed secondary：无
- selected Guide：DG-0845、DG-0561、DG-0113
- selected Wiki：energy-critical-device-minimum-line-001、energy-essential-load-reserve-001、energy-low-battery-power-window-001、energy-night-lighting-runtime-plan-001、energy-phone-002
- profiles：无
- domains：power、records、security、tools、comms
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_charging_queue_many_devices

- query：太阳能板只有一个口，手机、收音机、手电和移动电源都要充，怎么排队？
- 类型：strict
- focus：充电队列和排班。
- verdict：pass
- expected Guide：DG-0846
- allowed secondary：DG-0564、DG-0617、DG-0844
- selected Guide：DG-0846、DG-0337、DG-0336
- selected Wiki：energy-charging-queue-schedule-001、energy-charge-discharge-handover-log-001、energy-shared-power-bank-borrow-return-001、energy-charging-port-rotation-rule-001、energy-backup-battery-rotation-ledger-001、energy-battery-aging-capacity-note-001
- profiles：energy_charging_queue_management
- domains：power、records、security、comms、evacuation
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_shared_power_bank_unknown_return

- query：共享移动电源被借走后还回来，不知道剩多少电，也没人记录，怎么办？
- 类型：strict
- focus：借还、交接、复盘，不是电池故障。
- verdict：pass
- expected Guide：DG-0846
- allowed secondary：无
- selected Guide：DG-0846、DG-0337、DG-0543
- selected Wiki：energy-charging-queue-schedule-001、energy-charge-discharge-handover-log-001、energy-shared-power-bank-borrow-return-001、energy-charging-port-rotation-rule-001、safety-review-003、energy-device-consumption-baseline-001
- profiles：energy_charging_queue_management
- domains：power、records、security、comms、evacuation
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_port_occupied_too_long

- query：有人一直占着唯一的充电口给自己的设备充电，其他关键设备怎么办？
- 类型：strict
- focus：团队能源秩序和排班。
- verdict：pass
- expected Guide：DG-0846
- allowed secondary：DG-0844
- selected Guide：DG-0846、DG-0337、DG-0616
- selected Wiki：energy-charging-queue-schedule-001、energy-charge-discharge-handover-log-001、energy-shared-power-bank-borrow-return-001、energy-charging-port-rotation-rule-001、energy-battery-001、energy-battery-002
- profiles：energy_charging_queue_management
- domains：power、records、security、comms、evacuation
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_cloudy_solar_target

- query：今天阴天，太阳能输出很弱，应该先充手机还是移动电源？
- 类型：strict
- focus：太阳能降级和充电目标选择。
- verdict：pass
- expected Guide：DG-0617
- allowed secondary：DG-0564、DG-0844、DG-0845
- selected Guide：DG-0617、DG-0564、DG-0563
- selected Wiki：energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002
- profiles：energy_solar_low_output_management
- domains：power
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_solar_daytime_schedule

- query：今天有几小时太阳，早上中午下午分别应该怎么安排充电？
- 类型：strict
- focus：太阳能白天排程。
- verdict：pass
- expected Guide：DG-0617
- allowed secondary：DG-0564
- selected Guide：DG-0617、DG-0564、DG-0846
- selected Wiki：energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002
- profiles：energy_solar_low_output_management
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_solar_two_days_low_output

- query：太阳能连续两天都没怎么充进去，晚上照明和通信要怎么调整？
- 类型：strict
- focus：多日低产出后的能源预算调整。
- verdict：pass
- expected Guide：DG-0844
- allowed secondary：DG-0564、DG-0617、DG-0845
- selected Guide：DG-0617、DG-0564、DG-0562
- selected Wiki：energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002
- profiles：energy_solar_low_output_management
- domains：power
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_low_power_disconnect_order

- query：电量不够了，应该先关哪些设备，哪些必须留下？
- 类型：strict
- focus：低电量断开负载顺序。不要误判成电气故障。
- verdict：pass
- expected Guide：DG-0619
- allowed secondary：DG-0844、DG-0845
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_one_light_at_night

- query：晚上为了省电，只留一盏灯可以吗？哪些地方不能全黑？
- 类型：strict
- focus：夜间最低照明和安全边界。
- verdict：partial
- expected Guide：DG-0845
- allowed secondary：DG-0565、DG-0844
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：selector 问题
- failure reasons：未命中预期 Wiki

### energy_nonessential_suspend

- query：现在电量快不够了，哪些娱乐或舒服一点的设备应该先停掉？
- 类型：strict
- focus：非必要负载停用。
- verdict：pass
- expected Guide：DG-0619
- allowed secondary：DG-0844、DG-0845
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_day_end_review

- query：今天用电情况很乱，晚上怎么复盘明天怎么调整？
- 类型：observation
- focus：每日复盘。
- verdict：pass
- expected Guide：DG-0844
- allowed secondary：无
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_battery_rotation_lost_track

- query：备用电池轮换记录乱了，不知道哪几节最近用过，怎么重新整理？
- 类型：observation
- focus：备用电池轮换台账，不要误入电池故障。
- verdict：pass
- expected Guide：DG-0336
- allowed secondary：DG-0618、DG-0844
- selected Guide：DG-0336、DG-0706、DG-0618
- selected Wiki：energy-backup-battery-rotation-ledger-001、energy-battery-aging-capacity-note-001、safety-review-001、energy-battery-001、energy-battery-002、energy-battery-003
- profiles：无
- domains：power、external_contact
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

### energy_core_terminal_low_power_window

- query：Core 和随身终端都快没电了，应该怎么安排短时间开机和记录？
- 类型：observation
- focus：观察是否被终端同步、数据备份或通信窗口完全抢主位。本批没有新增 energy-low-power-core-terminal-window-001，所以不强制 pass。
- verdict：pass
- expected Guide：DG-0845
- allowed secondary：DG-0107、DG-0844
- selected Guide：DG-0844、DG-0619、DG-0845
- selected Wiki：energy-daily-energy-budget-001、energy-device-power-priority-tier-001、energy-device-runtime-estimate-card-001、energy-day-end-power-review-001、general-energy-handover-summary-001、energy-low-battery-load-disconnect-order-001
- profiles：energy_daily_budget_priority
- domains：power、records、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- cross domain：无
- P0 电气安全误触发：否
- root cause：无
- failure reasons：无

## 5. Root Cause 分类

- selector 问题：1

## 6. 跨域竞争统计

- 未发现通信、电脑、夜间安全、维修或 P0 电气安全抢主位。

## 7. Batch5-H Apply 后结论

Batch5-H Minimal Apply 后，field test 从 `5 / 8 / 2` 提升为 `14 / 1 / 0`。当前不再存在 fail、危险建议、P0 电气安全误触发或跨域抢位。

剩余 partial 为 `energy_one_light_at_night`：Guide 已进入能源管理组合，且 safety / fallback / record 均满足，但 `energy-night-lighting-runtime-plan-001` 未进入 selected Wiki 前 6。该问题属于 selector/evidence priority 边界，不建议继续扩 profile 或改 top_k。

## 8. 边界声明

- 本批没有修改 Wiki 正文、Guide 正文、Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback 或 PocketBase schema。
- Batch5-H 仅新增能源管理 query profile、前移 DG-0617 / DG-0619 的必要 related_wiki 顺序，并刷新本报告和结果 JSON。

## 9. 验证结果

- `python3 tools/audit_wiki.py`：errors=0 warnings=0 advisories=0；markdown=804，pocketbase=804，categories=24。
- `python3 tools/build_guides.py`：Generated 776 Guides / 776 Guide Index Items。
- `python3 scripts/audit_guides.py`：errors=0 warnings=0 advisories=0；Guides=776。
- `env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_energy_management_retrieval_profiles.py`：18 passed。
- `python3 -m py_compile scripts/test_energy_management_field.py`：通过。
