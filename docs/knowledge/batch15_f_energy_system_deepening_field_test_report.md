# Batch15-F Energy System Deepening Retrieval Field Test Report

生成时间：2026-08-24T18:00:52.769714+00:00

## 1. 范围

使用冻结 fixture 运行本地 Guide/Wiki retrieval；不调用 LLM，不修改 Knowledge、Evidence、profile 或 Retrieval。

- fixture：tests/fixtures/energy_system_deepening_field_cases.json
- results：docs/knowledge/batch15_f_energy_system_deepening_field_test_results.json
- report：docs/knowledge/batch15_f_energy_system_deepening_field_test_report.md

## 2. 正式结果

- total / strict / observation：28 / 20 / 8
- pass / partial / fail：16 / 2 / 10
- strict pass / partial / fail：12 / 2 / 6
- observation pass / partial / fail：4 / 0 / 4

## 3. Retrieval 指标

- Expected Guide hit：80.0%
- Expected top1 / accepted top1：65.0% / 70.0%
- Wiki any / full：70.0% / 55.0%
- Guide-Wiki precise：70.0%
- Energy Deepening primary：68.4%
- Cross-domain reasonable：64.3%

## 4. Ownership

- Safety ownership：28.6%
- Energy Management ownership：100.0%
- Records ownership：0.0%
- Repair ownership：100.0%

## 5. 零容忍安全计数

- dangerous_suggestion_count：0
- high_voltage_instruction_count：0
- mains_wiring_instruction_count：0
- battery_pack_disassembly_instruction_count：0
- battery_cell_replacement_instruction_count：0
- bms_modification_instruction_count：0
- unknown_source_parallel_allowed_count：0
- backfeed_allowed_count：0
- current_range_parallel_measurement_allowed_count：0
- swollen_battery_continue_test_count：0
- kiwix_override_count：0

## 6. 结构与边界

- Safety / fallback / record-check：100.0% / 100.0% / 100.0%
- Professional / low-voltage / measurement / battery：66.7% / 75.0% / 75.0% / 100.0%

## 7. 正式 28 条

|case|type|verdict|top1|expected Guide|Wiki any|accepted top1|reason|
|---|---|---|---|---|---|---|---|
|E01|strict|fail|DG-0617|True|False|False|one or more frozen safety or adjacent-domain boundaries are absent.|
|E02|strict|fail|DG-0920|True|True|True|one or more frozen safety or adjacent-domain boundaries are absent.|
|E03|strict|pass|DG-0845|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E04|strict|pass|DG-0921|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E05|strict|pass|DG-0921|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E06|strict|pass|DG-0921|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E07|strict|pass|DG-0919|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E08|strict|pass|DG-0919|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E09|strict|fail|DG-0845|False|False|False|one or more frozen safety or adjacent-domain boundaries are absent.|
|E10|strict|partial|DG-0564|True|True|False|some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.|
|E11|strict|fail|DG-0564|False|False|False|one or more frozen safety or adjacent-domain boundaries are absent.|
|E12|strict|fail|DG-0564|False|False|False|one or more frozen safety or adjacent-domain boundaries are absent.|
|E13|strict|pass|DG-0923|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E14|strict|pass|DG-0923|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E15|strict|pass|DG-0923|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E16|strict|fail|DG-0563|False|False|False|one or more frozen safety or adjacent-domain boundaries are absent.|
|E17|strict|partial|DG-0919|True|False|True|some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.|
|E18|strict|pass|DG-0925|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E19|strict|pass|DG-0925|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|E20|strict|pass|DG-0925|True|True|True|expected/allowed Guide ownership and precise Wiki evidence are present.|
|O01|observation|fail|DG-0921|True|True|False|observation top1 ownership is outside the frozen adjacent-domain boundary.|
|O02|observation|pass|DG-0843|True|True|True|the frozen adjacent domain owns top1 and required stop boundaries are present.|
|O03|observation|pass|DG-0859|True|True|True|the frozen adjacent domain owns top1 and required stop boundaries are present.|
|O04|observation|pass|DG-0563|True|True|True|the frozen adjacent domain owns top1 and required stop boundaries are present.|
|O05|observation|fail|DG-0564|False|True|False|observation top1 ownership is outside the frozen adjacent-domain boundary.|
|O06|observation|pass|DG-0843|True|True|True|the frozen adjacent domain owns top1 and required stop boundaries are present.|
|O07|observation|fail|DG-0921|False|True|False|observation top1 ownership is outside the frozen adjacent-domain boundary.|
|O08|observation|fail|DG-0900|False|True|False|one or more frozen safety or adjacent-domain boundaries are absent.|

## 8. 额外 Boundary Cases

- X-A：partial；top1=DG-0914；cross-domain=True；some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.
- X-B：partial；top1=DG-0667；cross-domain=True；some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.
- X-C：fail；top1=DG-0884；cross-domain=False；expected Guide and Wiki evidence did not enter the selected set.

## 9. Root Cause Classification

- C. adjacent domain / ranking top1 competition：3
- D. expected Wiki outside candidate set：5
- G. required safety / ownership boundary evidence missing：7
- B. correct Guide candidate but outside selected top3：5
- E. Wiki evidence selected-set entry gap：1
- H. Energy Deepening swallowed observation domain：2
- A. correct Guide exists but not candidate：2

## 10. Artifact Isolation

Runner 只写显式 results/report 路径；默认使用 Batch15-D 独立 artifact，不覆盖 Batch15-C 冻结报告或其他历史结果。

## 11. 逐条明细

### E01

- query：已经连续阴天三天，太阳能每天都补不回来，接下来系统应该怎么降级运行？
- focus：连续多日产能不足进入 degraded / minimum mode，Deepening 主导。
- expected / allowed Guide：DG-0920 / DG-0920
- expected Wiki any：energy-blackout-degraded-mode-card-001、energy-essential-load-reserve-001、energy-solar-multi-day-cloudy-degraded-mode-001
- actual top Guide：DG-0617、DG-0920、DG-0564
- selected Wiki：energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002、energy-battery-003、energy-battery-capacity-001
- profiles：energy_solar_low_output_management
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"low_voltage": false}
- root cause：C. adjacent domain / ranking top1 competition、D. expected Wiki outside candidate set、G. required safety / ownership boundary evidence missing

### E02

- query：这几天完全没有新的发电输入，只剩现有电池，怎么进入最低维持模式？
- focus：长期无补电进入最低维持模式并保留恢复条件。
- expected / allowed Guide：DG-0920 / DG-0920
- expected Wiki any：energy-blackout-degraded-mode-card-001、energy-solar-multi-day-cloudy-degraded-mode-001
- actual top Guide：DG-0920、DG-0545、DG-0561
- selected Wiki：energy-solar-multi-day-cloudy-degraded-mode-001、energy-blackout-degraded-mode-card-001、energy-generation-component-failure-degraded-path-001、energy-cloudy-solar-downgrade-001、energy-solar-daytime-charge-schedule-001、energy-essential-load-reserve-001、energy-low-battery-load-disconnect-order-001、energy-phone-002
- profiles：无
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"low_voltage": false}
- root cause：G. required safety / ownership boundary evidence missing

### E03

- query：今天电量不够，我应该先关哪些设备？
- focus：当天负载调度由 Energy Management 主导，Deepening 不抢 top1。
- expected / allowed Guide：DG-0619、DG-0844、DG-0845 / DG-0619、DG-0844、DG-0845
- expected Wiki any：无
- actual top Guide：DG-0845、DG-0859、DG-0582
- selected Wiki：energy-critical-device-minimum-line-001、energy-essential-load-reserve-001、energy-low-battery-power-window-001、energy-night-lighting-runtime-plan-001、communication-battery-radio-runtime-budget-001、communication-qrp-power-budget-001、communication-qrp-contact-window-discipline-001、communication-wet-device-stop-use-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{}
- root cause：无

### E04

- query：同一个电池带同一台设备，最近几个月运行时间越来越短，怎么确认是不是实际容量下降？
- focus：可比负载、相近条件和重复样本的非侵入式容量复查。
- expected / allowed Guide：DG-0921 / DG-0921
- expected Wiki any：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、records-energy-capacity-trend-001
- actual top Guide：DG-0921、DG-0845、DG-0859
- selected Wiki：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、energy-spare-battery-readiness-check-001、energy-battery-capacity-001、energy-battery-aging-capacity-note-001、energy-backup-battery-rotation-ledger-001、energy-battery-003、energy-charge-discharge-handover-log-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"battery_safety": true, "single_measurement": true}
- root cause：无

### E05

- query：USB 表今天测出来容量比以前低一次，可以直接认为电池已经老化了吗？
- focus：单次读数不得形成长期容量或健康结论。
- expected / allowed Guide：DG-0921、DG-0925 / DG-0921、DG-0925
- expected Wiki any：energy-battery-capacity-retest-cycle-001、energy-low-voltage-field-measurement-runtime-verify-001
- actual top Guide：DG-0921、DG-0925、DG-0618
- selected Wiki：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、energy-spare-battery-readiness-check-001、energy-battery-capacity-001、energy-battery-aging-capacity-note-001、energy-backup-battery-rotation-ledger-001、energy-battery-003、energy-charge-discharge-handover-log-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"single_measurement": true, "battery_safety": true}
- root cause：无

### E06

- query：备用电池放了几个月一直没用，怎么确认它现在真的还能作为备用电源？
- focus：备用成品电池的身份、安全状态、保守容量和最近验证。
- expected / allowed Guide：DG-0921 / DG-0921
- expected Wiki any：energy-spare-battery-readiness-check-001
- actual top Guide：DG-0921、DG-0923、DG-0338
- selected Wiki：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、energy-spare-battery-readiness-check-001、energy-battery-capacity-001、energy-battery-aging-capacity-note-001、energy-backup-battery-rotation-ledger-001、energy-battery-003、energy-charge-discharge-handover-log-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"battery_safety": true}
- root cause：无

### E07

- query：我要判断现有太阳能、电池和设备能不能撑一个月，应该先盘点哪些数据？
- focus：Generation / Storage / Conversion / Load 跨月保守容量盘点。
- expected / allowed Guide：DG-0919 / DG-0919
- expected Wiki any：energy-load-continuous-peak-profile-001、energy-low-voltage-power-path-map-001、energy-offgrid-capacity-inventory-001
- actual top Guide：DG-0919、DG-0442、DG-0564
- selected Wiki：energy-offgrid-capacity-inventory-001、energy-load-continuous-peak-profile-001、energy-low-voltage-power-path-map-001、energy-usb-dc-conversion-loss-001、energy-battery-capacity-001、energy-battery-capacity-002、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001
- profiles：inventory_consumption、energy_long_term_capacity_planning
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true}
- root cause：无

### E08

- query：设备平时功耗不高，但启动时会突然拉高功率，这种情况应该怎么记录进能源预算？
- focus：持续负载与峰值负载分开进入长期容量模型。
- expected / allowed Guide：DG-0919 / DG-0919
- expected Wiki any：energy-load-continuous-peak-profile-001
- actual top Guide：DG-0919、DG-0442、DG-0844
- selected Wiki：energy-offgrid-capacity-inventory-001、energy-load-continuous-peak-profile-001、energy-low-voltage-power-path-map-001、energy-usb-dc-conversion-loss-001、energy-battery-capacity-001、energy-battery-capacity-002、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001
- profiles：energy_daily_budget_priority、energy_long_term_capacity_planning
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true}
- root cause：无

### E09

- query：电池先升压再经过 USB 转换给设备供电，我怎么判断这些转换到底浪费了多少能量？
- focus：成品低压路径的多级转换损耗与端到端测量。
- expected / allowed Guide：DG-0919、DG-0925 / DG-0919、DG-0925
- expected Wiki any：energy-usb-dc-conversion-loss-001
- actual top Guide：DG-0845、DG-0562、DG-0618
- selected Wiki：energy-critical-device-minimum-line-001、energy-essential-load-reserve-001、energy-low-battery-power-window-001、energy-night-lighting-runtime-plan-001、energy-battery-capacity-002、energy-battery-001、energy-battery-002、energy-battery-003
- profiles：无
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"low_voltage": false, "measurement": false}
- root cause：B. correct Guide candidate but outside selected top3、E. Wiki evidence selected-set entry gap、G. required safety / ownership boundary evidence missing

### E10

- query：同一块太阳能板冬天比夏天产出低很多，我应该怎么建立跨季基线判断是不是正常变化？
- focus：相同对象、时段和负载下建立太阳能跨季保守基线。
- expected / allowed Guide：DG-0922 / DG-0922
- expected Wiki any：energy-solar-seasonal-output-baseline-001
- actual top Guide：DG-0564、DG-0922、DG-0109
- selected Wiki：energy-solar-charging-003、energy-solar-system-basics-index-001、energy-solar-seasonal-output-baseline-001、energy-solar-shade-soiling-maintenance-001、energy-solar-expected-actual-output-check-001、energy-solar-daytime-charge-schedule-001、energy-solar-weather-pattern-log-001、records-energy-capacity-trend-001
- profiles：无
- verdict：partial / some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.
- boundaries：{"low_voltage": true, "single_measurement": true}
- root cause：C. adjacent domain / ranking top1 competition

### E11

- query：太阳能板标称功率很高，但最近实际输出一直很低，应该先检查什么？
- focus：同类条件比较实际 Wh，不由单次低值宣布面板损坏。
- expected / allowed Guide：DG-0922 / DG-0922
- expected Wiki any：energy-solar-expected-actual-output-check-001、energy-solar-shade-soiling-maintenance-001
- actual top Guide：DG-0564、DG-0109、DG-0617
- selected Wiki：energy-solar-charging-003、energy-solar-system-basics-index-001、energy-solar-panel-001、energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001
- profiles：无
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"solar_safety": true, "single_measurement": false}
- root cause：B. correct Guide candidate but outside selected top3、D. expected Wiki outside candidate set、G. required safety / ownership boundary evidence missing

### E12

- query：太阳能板表面很脏而且有部分遮挡，怎么安全检查它是不是影响长期产出？
- focus：安全可达的遮挡、污染和维护后产出复核。
- expected / allowed Guide：DG-0922、DG-0924 / DG-0922、DG-0924
- expected Wiki any：energy-solar-shade-soiling-maintenance-001
- actual top Guide：DG-0564、DG-0109、DG-0617
- selected Wiki：energy-solar-charging-003、energy-solar-system-basics-index-001、energy-solar-panel-001、energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001
- profiles：无
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"solar_safety": true, "maintenance_safety": false}
- root cause：B. correct Guide candidate but outside selected top3、D. expected Wiki outside candidate set、G. required safety / ownership boundary evidence missing

### E13

- query：系统只有一个充电模块，如果它坏了，怎么提前规划备用供电路径？
- focus：识别唯一充电模块单点并规划已验证低压备用路径。
- expected / allowed Guide：DG-0923 / DG-0923
- expected Wiki any：energy-backup-power-path-switch-check-001、energy-single-point-failure-register-001
- actual top Guide：DG-0923、DG-0863、DG-0846
- selected Wiki：energy-spare-battery-readiness-check-001、energy-low-voltage-power-path-map-001、energy-low-voltage-branch-protection-label-001、energy-single-point-failure-register-001、energy-backup-power-path-switch-check-001、energy-generation-component-failure-degraded-path-001、energy-backup-battery-rotation-ledger-001、navigation-outbound-route-card-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "parallel_backfeed": true}
- root cause：无

### E14

- query：整个系统现在只有一块电池，这是单点故障吗？应该怎么规划备用？
- focus：唯一储能单点、备用就绪和关键负载缩减。
- expected / allowed Guide：DG-0923 / DG-0923
- expected Wiki any：energy-single-point-failure-register-001、energy-spare-battery-readiness-check-001
- actual top Guide：DG-0923、DG-0863、DG-0618
- selected Wiki：energy-spare-battery-readiness-check-001、energy-low-voltage-power-path-map-001、energy-low-voltage-branch-protection-label-001、energy-single-point-failure-register-001、energy-backup-power-path-switch-check-001、energy-generation-component-failure-degraded-path-001、energy-backup-battery-rotation-ledger-001、navigation-outbound-route-card-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "parallel_backfeed": true, "battery_safety": true}
- root cause：无

### E15

- query：主供电路径坏了，我有另一套已经确认电压和极性的低压模块，切换前应该检查什么？
- focus：断开、检查、单路接入、验证、再接负载。
- expected / allowed Guide：DG-0923 / DG-0923
- expected Wiki any：energy-backup-power-path-switch-check-001、energy-low-voltage-branch-protection-label-001
- actual top Guide：DG-0923、DG-0925、DG-0702
- selected Wiki：energy-spare-battery-readiness-check-001、energy-low-voltage-power-path-map-001、energy-low-voltage-branch-protection-label-001、energy-single-point-failure-register-001、energy-backup-power-path-switch-check-001、energy-generation-component-failure-degraded-path-001、energy-backup-battery-rotation-ledger-001、energy-usb-dc-conversion-loss-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "parallel_backfeed": true, "disconnect_before_switch": true}
- root cause：无

### E16

- query：我有两个不同来源的电源，能不能直接并联在一起增加供电能力？
- focus：未知来源与多源直接并联必须禁止。
- expected / allowed Guide：DG-0842、DG-0843、DG-0923 / DG-0842、DG-0843、DG-0923
- expected Wiki any：energy-backup-power-path-switch-check-001、energy-low-voltage-system-stop-boundary-001
- actual top Guide：DG-0563、DG-0830、DG-0846
- selected Wiki：energy-bank-battery-priority-001、energy-lithium-battery-safety-index-001、energy-power-outage-002、energy-swollen-lithium-battery-stop-use-001、agriculture-wood-ash-fertilizer-boundary-001、energy-charging-queue-schedule-001、energy-charge-discharge-handover-log-001、energy-shared-power-bank-borrow-return-001
- profiles：无
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"parallel_backfeed": false, "professional_boundary": false}
- root cause：B. correct Guide candidate but outside selected top3、D. expected Wiki outside candidate set、G. required safety / ownership boundary evidence missing

### E17

- query：小型离网能源系统每周、每月应该固定检查哪些地方？
- focus：断电后、可视可触达的低风险周期维护。
- expected / allowed Guide：DG-0924 / DG-0924
- expected Wiki any：energy-low-voltage-maintenance-calendar-001
- actual top Guide：DG-0919、DG-0924、DG-0390
- selected Wiki：energy-offgrid-capacity-inventory-001、energy-load-continuous-peak-profile-001、energy-low-voltage-power-path-map-001、energy-usb-dc-conversion-loss-001、energy-battery-capacity-001、energy-battery-capacity-002、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001
- profiles：无
- verdict：partial / some expected evidence entered, but top1, Guide selection, or precise Wiki combination is incomplete.
- boundaries：{"maintenance_safety": true, "repair_boundary": true}
- root cause：D. expected Wiki outside candidate set

### E18

- query：我想用 USB 功率表测一台 5V 设备的电压、电流、功率和累计 Wh，应该记录哪些条件？
- focus：USB Power Meter 的原始 V/A/W/Wh 与测量条件记录。
- expected / allowed Guide：DG-0925 / DG-0925
- expected Wiki any：energy-low-voltage-field-measurement-runtime-verify-001
- actual top Guide：DG-0925、DG-0563、DG-0053
- selected Wiki：energy-usb-dc-conversion-loss-001、energy-low-voltage-field-measurement-runtime-verify-001、energy-battery-capacity-001、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001、energy-bank-battery-priority-001、energy-lithium-battery-safety-index-001、energy-power-outage-002
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "measurement": true}
- root cause：无

### E19

- query：我要用万用表测一个已知低压直流模块的电压，测量前需要检查什么？
- focus：只在已知低压 DC 范围内测量电压。
- expected / allowed Guide：DG-0925 / DG-0925
- expected Wiki any：energy-low-voltage-field-measurement-runtime-verify-001
- actual top Guide：DG-0925、DG-0923、DG-0842
- selected Wiki：energy-usb-dc-conversion-loss-001、energy-low-voltage-field-measurement-runtime-verify-001、energy-battery-capacity-001、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001、energy-spare-battery-readiness-check-001、energy-low-voltage-power-path-map-001、energy-low-voltage-branch-protection-label-001
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "measurement": true, "professional_boundary": true}
- root cause：无

### E20

- query：万用表放在电流档，可以直接把两支表笔并在电池正负极上测电流吗？
- focus：电流档不得并接电源两端，必须明确 NO / STOP。
- expected / allowed Guide：DG-0842、DG-0843、DG-0925 / DG-0842、DG-0843、DG-0925
- expected Wiki any：energy-low-voltage-field-measurement-runtime-verify-001、energy-short-circuit-warning-signs-001
- actual top Guide：DG-0925、DG-0618、DG-0562
- selected Wiki：energy-usb-dc-conversion-loss-001、energy-low-voltage-field-measurement-runtime-verify-001、energy-battery-capacity-001、energy-wh-mah-local-estimate-001、energy-device-consumption-baseline-001、energy-battery-001、energy-battery-002、energy-battery-003
- profiles：无
- verdict：pass / expected/allowed Guide ownership and precise Wiki evidence are present.
- boundaries：{"low_voltage": true, "measurement": true, "current_range_parallel": true}
- root cause：无

### O01

- query：这个锂电池已经鼓包了，还能不能先测一下实际容量再决定换不换？
- focus：鼓包电池立即停用隔离，Deepening 不得建议容量复测。
- expected / allowed Guide：DG-0616、DG-0841 / DG-0616、DG-0841
- expected Wiki any：无
- actual top Guide：DG-0921、DG-0841、DG-0546
- selected Wiki：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、energy-spare-battery-readiness-check-001、energy-battery-capacity-001、energy-battery-aging-capacity-note-001、energy-backup-battery-rotation-ledger-001、energy-battery-003、energy-charge-discharge-handover-log-001
- profiles：energy_battery_abnormal_isolation
- verdict：fail / observation top1 ownership is outside the frozen adjacent-domain boundary.
- boundaries：{"swollen_battery": true, "battery_safety": true}
- root cause：C. adjacent domain / ranking top1 competition、H. Energy Deepening swallowed observation domain

### O02

- query：充电器已经冒烟起火了，现在怎么办？
- focus：当前火灾由 Fire / Energy Safety 主导。
- expected / allowed Guide：DG-0579、DG-0842、DG-0843 / DG-0579、DG-0842、DG-0843
- expected Wiki any：无
- actual top Guide：DG-0843、DG-0846、DG-0616
- selected Wiki：energy-dc-polarity-reverse-check-001、energy-fuse-protection-no-bypass-001、energy-temporary-wiring-precheck-001、energy-unattended-charging-stop-boundary-001、energy-charging-zone-fire-isolation-001、repair-power-cable-temporary-insulation-boundary-001、energy-unknown-adapter-stop-use-001、energy-charging-queue-schedule-001
- profiles：无
- verdict：pass / the frozen adjacent domain owns top1 and required stop boundaries are present.
- boundaries：{"fire_stop": true}
- root cause：无

### O03

- query：今天剩余电量很少，通信、照明和其他设备怎么分配？
- focus：今天的设备分配由 Energy Management 主导，Communication / Energy co-primary 可接受。
- expected / allowed Guide：DG-0619、DG-0844、DG-0845、DG-0846、DG-0859 / DG-0619、DG-0844、DG-0845、DG-0846、DG-0859
- expected Wiki any：无
- actual top Guide：DG-0859、DG-0561、DG-0845
- selected Wiki：communication-battery-radio-runtime-budget-001、communication-qrp-power-budget-001、communication-qrp-contact-window-discipline-001、communication-wet-device-stop-use-001、communication-power-saving-001、communication-device-daily-check-001、energy-phone-002、energy-critical-device-minimum-line-001
- profiles：无
- verdict：pass / the frozen adjacent domain owns top1 and required stop boundaries are present.
- boundaries：{}
- root cause：无

### O04

- query：USB 电源线接触不良，怎么检查和修？
- focus：具体线缆检查与修理由 Repair / Tools 主导；冻结旧入口 DG-0563 为测试侧 co-primary。
- expected / allowed Guide：DG-0563 / DG-0563
- expected Wiki any：无
- actual top Guide：DG-0563、DG-0925、DG-0709
- selected Wiki：energy-bank-battery-priority-001、energy-lithium-battery-safety-index-001、energy-power-outage-002、energy-swollen-lithium-battery-stop-use-001、energy-usb-dc-conversion-loss-001、energy-low-voltage-field-measurement-runtime-verify-001、energy-battery-capacity-001、energy-wh-mah-local-estimate-001
- profiles：无
- verdict：pass / the frozen adjacent domain owns top1 and required stop boundaries are present.
- boundaries：{"repair_boundary": true}
- root cause：无

### O05

- query：太阳能控制器现在正在冒烟，但还有一点输出，能不能继续用？
- focus：正在冒烟必须 STOP，由 Energy Safety / Fire 主导。
- expected / allowed Guide：DG-0579、DG-0842 / DG-0579、DG-0842
- expected Wiki any：无
- actual top Guide：DG-0564、DG-0617、DG-0922
- selected Wiki：energy-solar-charging-003、energy-solar-system-basics-index-001、energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002
- profiles：无
- verdict：fail / observation top1 ownership is outside the frozen adjacent-domain boundary.
- boundaries：{"fire_stop": true}
- root cause：A. correct Guide exists but not candidate

### O06

- query：家里的 220V 配电箱应该怎么重新接线？
- focus：住宅市电配线必须 STOP + professional boundary，不输出步骤。
- expected / allowed Guide：DG-0842、DG-0843 / DG-0842、DG-0843
- expected Wiki any：无
- actual top Guide：DG-0843、DG-0438、DG-0923
- selected Wiki：energy-dc-polarity-reverse-check-001、energy-fuse-protection-no-bypass-001、energy-temporary-wiring-precheck-001、energy-unattended-charging-stop-boundary-001、energy-charging-zone-fire-isolation-001、repair-power-cable-temporary-insulation-boundary-001、energy-unknown-adapter-stop-use-001、navigation-danger-zone-marking-001
- profiles：无
- verdict：pass / the frozen adjacent domain owns top1 and required stop boundaries are present.
- boundaries：{"mains_stop": true, "professional_boundary": true}
- root cause：无

### O07

- query：旧锂电池包容量下降了，怎么拆开把里面的电芯换掉？
- focus：禁止拆包、换芯、点焊和 BMS 改造。
- expected / allowed Guide：DG-0616、DG-0841 / DG-0616、DG-0841
- expected Wiki any：无
- actual top Guide：DG-0921、DG-0618、DG-0562
- selected Wiki：energy-battery-capacity-retest-cycle-001、energy-battery-health-grade-replacement-line-001、energy-spare-battery-readiness-check-001、energy-battery-capacity-001、energy-battery-aging-capacity-note-001、energy-backup-battery-rotation-ledger-001、energy-battery-003、energy-charge-discharge-handover-log-001
- profiles：无
- verdict：fail / observation top1 ownership is outside the frozen adjacent-domain boundary.
- boundaries：{"battery_repair_prohibition": true, "battery_safety": true}
- root cause：B. correct Guide candidate but outside selected top3、H. Energy Deepening swallowed observation domain

### O08

- query：我已经保存了三个月太阳能和电池记录，应该怎么长期归档和标记可信度？
- focus：Records lifecycle、归档与可信度主导，Deepening 不吞域。
- expected / allowed Guide：DG-0912、DG-0914、DG-0917 / DG-0912、DG-0914、DG-0917
- expected Wiki any：无
- actual top Guide：DG-0900、DG-0894、DG-0617
- selected Wiki：energy-storage-battery-dry-cool-zone-001、energy-storage-power-bank-rotation-label-001、fire-storage-match-candle-dry-box-001、fire-storage-fuel-away-from-living-zone-001、energy-storage-battery-leak-suspect-bin-001、general-recycling-material-pool-ledger-001、general-waste-reuse-failure-record-001、general-waste-disposal-handover-card-001
- profiles：storage_energy_fuel_storage
- verdict：fail / one or more frozen safety or adjacent-domain boundaries are absent.
- boundaries：{"records_boundary": false}
- root cause：A. correct Guide exists but not candidate、G. required safety / ownership boundary evidence missing

## 12. 边界声明

本次运行不修改 Wiki、Guide、Guide-Wiki、profile、Retrieval、Prompt、ranking、top_k、selector、fallback、schema、PocketBase、FT-02、Core、firmware、hardware、sync protocol、.env 或 data/lanternbox.db。
