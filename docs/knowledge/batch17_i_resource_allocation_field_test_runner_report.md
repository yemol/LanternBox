# Batch17-I Resource Allocation Retrieval Field Test Report

生成时间：2026-09-02T02:19:53.940229+00:00

## 1. Scope

使用 Batch17-C 冻结 fixture 调用生产 run_retrieval_v2 路径；--no-answer 不生成回答，不修改 Knowledge、Evidence、profile、Contract 或 Retrieval。

- fixture：tests/fixtures/resource_allocation_field_cases.json
- results：docs/knowledge/batch17_i_resource_allocation_field_test_results.json
- report：docs/knowledge/batch17_i_resource_allocation_field_test_runner_report.md

## 2. Strict / Observation

- total / strict / observation：28 / 20 / 8
- strict pass / partial / fail：10 / 3 / 7
- observation pass / partial / fail：2 / 1 / 5

## 3. Retrieval Metrics

- Guide hit / top1 / accepted：55.0% / 50.0% / 70.0%
- Wiki any / full：65.0% / 40.0%
- precise evidence：65.0%

## 4. Capability Metrics

- allocation_horizon：36.4%
- baseline：66.7%
- degraded_mode：20.0%
- inventory：75.0%
- priority_allocation：22.2%
- replenishment：40.0%
- reserve：36.4%
- review_loop：66.7%
- safety_minimum：37.5%
- substitution：50.0%
- trigger_threshold：66.7%
- uncertainty：16.7%

## 5. Boundary Metrics

- storage：54.5%
- energy_management：66.7%
- energy_deepening：75.0%
- records：70.0%
- production：33.3%
- manufacturing：0.0%
- repair：0.0%
- medical：50.0%
- team_operations：100.0%
- energy_safety：100.0%

## 6. Safety Metrics

- production path：28 / 28
- planner / selector fallback：28 / 28
- Safety Gate failure：0
- safety priority：61.1%
- stop condition：100.0%
- dangerous suggestion：0
- scarcity overrides safety：0
- human value ranking：0
- evidence classifier diagnostic hits：8
- evidence scarcity classifier diagnostic hits：0
- Kiwix override：0
- safety / fallback / record-check：100.0% / 100.0% / 96.4%

## 7. Case Results

|case|type|verdict|top1|owners|Guide hit|Wiki any|precise|reason|
|---|---|---|---|---|---|---|---|---|
|R01|strict|fail|DG-0860|records|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R02|strict|fail|DG-0470|other|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R03|strict|pass|DG-0927|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R04|strict|pass|DG-0927|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R05|strict|partial|DG-0845|energy_management|False|True|True|ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.|
|R06|strict|pass|DG-0928|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R07|strict|pass|DG-0927|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R08|strict|fail|DG-0599|food|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R09|strict|partial|DG-0927|resource_allocation|False|True|True|ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.|
|R10|strict|pass|DG-0930|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R11|strict|fail|DG-0545|food|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R12|strict|pass|DG-0929|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R13|strict|fail|DG-0564|energy_management|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R14|strict|fail|DG-0006|medical|False|False|False|frozen ownership and evidence did not enter the selected set.|
|R15|strict|pass|DG-0897|agriculture、storage|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R16|strict|pass|DG-0914|records|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R17|strict|fail|DG-0929|resource_allocation|True|True|True|top1 ownership matches a frozen forbidden primary domain.|
|R18|strict|pass|DG-0928|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R19|strict|pass|DG-0842|energy_safety、safety|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|R20|strict|partial|DG-0927|resource_allocation|False|False|False|ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.|
|O01|observation|fail|DG-0360|other|False|False|False|frozen ownership and evidence did not enter the selected set.|
|O02|observation|fail|DG-0916|records|False|False|False|frozen ownership and evidence did not enter the selected set.|
|O03|observation|pass|DG-0926|resource_allocation|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|
|O04|observation|fail|DG-0617|energy_management|False|False|False|frozen ownership and evidence did not enter the selected set.|
|O05|observation|fail|DG-0905|production|False|False|False|frozen ownership and evidence did not enter the selected set.|
|O06|observation|fail|DG-0662|contamination|False|False|False|frozen ownership and evidence did not enter the selected set.|
|O07|observation|partial|DG-0929|resource_allocation|False|False|False|ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.|
|O08|observation|pass|DG-0841|energy_safety、safety|True|True|True|accepted ownership, Guide, Wiki, and precise evidence chain are present.|

## 8. Root Cause Classification

- B. expected Guide candidate outside selected top3：9
- C. primary ownership competition：11
- E. expected Wiki outside candidates：10
- F. expected Wiki outside selected evidence：3
- A. expected Guide absent from candidates：6
- D. forbidden domain captured top1：1

## 9. Case Detail

### R01

- query：仓库记录有 40 份应急食品，但盘点只有 31 份，未来一周该按多少安排？
- expected primary / accepted secondary：resource_allocation / storage、records、food
- expected / accepted Guide：DG-0926 / DG-0926、DG-0927
- expected Wiki：resource-allocation-available-resource-status-001、resource-allocation-uncertainty-scenarios-001
- actual Guide：DG-0860
- selected Wiki：communication-emergency-net-opening-check-001、communication-message-priority-levels-001、communication-check-in-missed-window-001、communication-radio-log-minimum-fields-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：inventory_consumption
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、E. expected Wiki outside candidates

### R02

- query：有 20 升水，其中 6 升是紧急储备，今天能按 20 升全部使用吗？
- expected primary / accepted secondary：resource_allocation / water、storage
- expected / accepted Guide：DG-0928 / DG-0928
- expected Wiki：resource-allocation-reserve-class-policy-001、resource-allocation-safety-minimum-register-001
- actual Guide：DG-0470
- selected Wiki：无
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、F. expected Wiki outside selected evidence

### R03

- query：过去每天耗水 8 升，这两天增加到 11 升，库存还能撑多久？
- expected primary / accepted secondary：resource_allocation / water、records
- expected / accepted Guide：DG-0927 / DG-0927、DG-0930
- expected Wiki：resource-allocation-consumption-horizon-baseline-001、water-usage-log-023
- actual Guide：DG-0927
- selected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001、energy-device-consumption-baseline-001、energy-offgrid-capacity-inventory-001、energy-solar-seasonal-output-baseline-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：resource_allocation_consumption_horizon
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R04

- query：现有食物够 14 天，电池按现在用法只够 5 天，未来 7 天怎么安排？
- expected primary / accepted secondary：resource_allocation / food、energy_management、energy_deepening
- expected / accepted Guide：DG-0927 / DG-0927、DG-0928
- expected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001
- actual Guide：DG-0927
- selected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001、energy-device-consumption-baseline-001、energy-offgrid-capacity-inventory-001、energy-solar-seasonal-output-baseline-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R05

- query：电量很少，照明、通信和娱乐设备应该怎么留？
- expected primary / accepted secondary：resource_allocation / energy_management
- expected / accepted Guide：DG-0928 / DG-0928、DG-0845、DG-0844
- expected Wiki：resource-allocation-safety-minimum-register-001、energy-essential-load-reserve-001
- actual Guide：DG-0845
- selected Wiki：energy-critical-device-minimum-line-001、energy-essential-load-reserve-001、energy-low-battery-power-window-001、energy-night-lighting-runtime-plan-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：1
- profiles：无
- verdict：partial / ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.
- root cause：A. expected Guide absent from candidates

### R06

- query：备用燃料什么时候才应该动用？
- expected primary / accepted secondary：resource_allocation / storage、fire、safety
- expected / accepted Guide：DG-0928 / DG-0928、DG-0930
- expected Wiki：resource-allocation-reserve-class-policy-001
- actual Guide：DG-0928
- selected Wiki：resource-allocation-safety-minimum-register-001、resource-allocation-reserve-class-policy-001、resource-allocation-resource-specific-priority-001、energy-essential-load-reserve-001、energy-critical-device-minimum-line-001、energy-daily-energy-budget-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：3
- profiles：resource_allocation_emergency_reserve_release
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R07

- query：确认 5 天后会获得补给，现在的库存怎么分配？
- expected primary / accepted secondary：resource_allocation / storage、records
- expected / accepted Guide：DG-0927 / DG-0927
- expected Wiki：resource-allocation-horizon-replenishment-001
- actual Guide：DG-0927
- selected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001、energy-device-consumption-baseline-001、energy-offgrid-capacity-inventory-001、energy-solar-seasonal-output-baseline-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R08

- query：有人说一周后可能有补给，但并不确定，现在能提前多消耗一些吗？
- expected primary / accepted secondary：resource_allocation / records
- expected / accepted Guide：DG-0927 / DG-0927、DG-0926
- expected Wiki：resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001
- actual Guide：DG-0599
- selected Wiki：food-food-knowledge-004、food-staple-food-002
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、F. expected Wiki outside selected evidence

### R09

- query：未来一个月没有补给来源，当前库存应该怎么调整？
- expected primary / accepted secondary：resource_allocation / storage、food、water、energy_deepening
- expected / accepted Guide：DG-0929 / DG-0929、DG-0930、DG-0927
- expected Wiki：resource-allocation-substitution-degraded-mode-001、resource-allocation-horizon-replenishment-001
- actual Guide：DG-0927
- selected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001、energy-device-consumption-baseline-001、energy-offgrid-capacity-inventory-001、energy-solar-seasonal-output-baseline-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：partial / ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.
- root cause：B. expected Guide candidate outside selected top3

### R10

- query：资源消耗突然比计划快了 30%，应该什么时候重新调整？
- expected primary / accepted secondary：resource_allocation / records
- expected / accepted Guide：DG-0930 / DG-0930、DG-0927
- expected Wiki：resource-allocation-trigger-action-recheck-001、resource-allocation-review-loop-001
- actual Guide：DG-0930
- selected Wiki：resource-allocation-review-loop-001、resource-allocation-uncertainty-scenarios-001、resource-allocation-consumption-horizon-baseline-001、resource-allocation-trigger-action-recheck-001、general-inventory-consumption-log-001、records-energy-capacity-trend-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R11

- query：某种耗材只剩很少，还有功能相近的替代品，应该怎么安排？
- expected primary / accepted secondary：resource_allocation / repair、manufacturing、storage
- expected / accepted Guide：DG-0929 / DG-0929
- expected Wiki：resource-allocation-substitution-degraded-mode-001
- actual Guide：DG-0545
- selected Wiki：无
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、E. expected Wiki outside candidates

### R12

- query：今天计划启动一个高耗能生产任务，但未来三天发电不足，还应该启动吗？
- expected primary / accepted secondary：resource_allocation / production、energy_management、energy_deepening
- expected / accepted Guide：DG-0929 / DG-0929、DG-0927
- expected Wiki：resource-allocation-substitution-degraded-mode-001、production-workshop-daily-resource-budget-001
- actual Guide：DG-0929
- selected Wiki：resource-allocation-horizon-replenishment-001、resource-allocation-substitution-degraded-mode-001、resource-allocation-trigger-action-recheck-001、energy-solar-multi-day-cloudy-degraded-mode-001、energy-generation-component-failure-degraded-path-001、water-shortage-response-041
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：resource_allocation_high_energy_task_decision
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R13

- query：今天太阳能输出偏低，晚上有哪些负载应该关掉？
- expected primary / accepted secondary：energy_management / resource_allocation
- expected / accepted Guide：DG-0844、DG-0845 / DG-0844、DG-0845、DG-0619
- expected Wiki：energy-daily-energy-budget-001、energy-critical-device-minimum-line-001
- actual Guide：DG-0564
- selected Wiki：energy-solar-charging-003、energy-solar-system-basics-index-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：A. expected Guide absent from candidates、E. expected Wiki outside candidates

### R14

- query：过去三个月发电越来越低，长期供能能力是不是需要重新规划？
- expected primary / accepted secondary：energy_deepening / records、resource_allocation
- expected / accepted Guide：DG-0919、DG-0922 / DG-0919、DG-0922、DG-0914
- expected Wiki：records-energy-capacity-trend-001、energy-solar-seasonal-output-baseline-001
- actual Guide：DG-0006
- selected Wiki：medical-deep-wound-infection-risk-001、medical-trauma-complication-index-001、medical-wound-infection-002
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、E. expected Wiki outside candidates

### R15

- query：剩下的种子应该怎么防潮保存半年？
- expected primary / accepted secondary：storage / agriculture、records
- expected / accepted Guide：DG-0897 / DG-0897、DG-0513
- expected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-library-rotation-card-001
- actual Guide：DG-0897
- selected Wiki：agriculture-storage-seed-moisture-quarantine-001、agriculture-storage-seed-food-separation-shelf-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001、agriculture-storage-seed-box-dry-check-001
- Contract status / rule / primary：COMPARE_ONLY / biological_material_storage / storage
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：storage_seed_storage
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R16

- query：过去三个月的资源消耗记录怎么整理成可信趋势？
- expected primary / accepted secondary：records / resource_allocation
- expected / accepted Guide：DG-0914 / DG-0914、DG-0912
- expected Wiki：records-repeated-problem-trend-review-001、records-applicable-conditions-context-001
- actual Guide：DG-0914
- selected Wiki：records-applicable-conditions-context-001、records-success-review-minimum-001、records-repeated-problem-trend-review-001、records-pending-hypothesis-next-test-001、records-corrective-action-verification-001、records-maintenance-history-trend-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：inventory_consumption、records_long_term_trend
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R17

- query：唯一的水泵坏了，怎么修？
- expected primary / accepted secondary：repair / tools、resource_allocation
- expected / accepted Guide：无 / DG-0383
- expected Wiki：无
- actual Guide：DG-0929
- selected Wiki：resource-allocation-horizon-replenishment-001、resource-allocation-substitution-degraded-mode-001、resource-allocation-trigger-action-recheck-001、energy-solar-multi-day-cloudy-degraded-mode-001、energy-generation-component-failure-degraded-path-001、water-shortage-response-041
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / top1 ownership matches a frozen forbidden primary domain.
- root cause：C. primary ownership competition、D. forbidden domain captured top1

### R18

- query：医疗耗材库存不足，怎么安排库存和补给？
- expected primary / accepted secondary：resource_allocation / medical、storage
- expected / accepted Guide：DG-0928 / DG-0928、DG-0926、DG-0927
- expected Wiki：resource-allocation-safety-minimum-register-001、resource-allocation-reserve-class-policy-001
- actual Guide：DG-0928
- selected Wiki：resource-allocation-safety-minimum-register-001、resource-allocation-reserve-class-policy-001、resource-allocation-resource-specific-priority-001、energy-essential-load-reserve-001、energy-critical-device-minimum-line-001、energy-daily-energy-budget-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：3
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R19

- query：唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，能不能继续用？
- expected primary / accepted secondary：energy_safety / fire
- expected / accepted Guide：DG-0842 / DG-0842、DG-0579
- expected Wiki：energy-device-smell-heat-no-restart-001、energy-low-voltage-system-stop-boundary-001
- actual Guide：DG-0842
- selected Wiki：energy-low-voltage-system-stop-boundary-001、energy-dc-polarity-reverse-check-001、energy-short-circuit-warning-signs-001、energy-wire-heating-load-limit-001、energy-loose-connector-stop-use-001、energy-wet-low-voltage-device-no-test-001
- Contract status / rule / primary：CANARY_APPLIED / energy_device_abnormal / energy_safety
- Safety Gate：passed
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### R20

- query：制定了 7 天资源计划以后，什么情况下必须提前重算？
- expected primary / accepted secondary：resource_allocation / records
- expected / accepted Guide：DG-0930 / DG-0930
- expected Wiki：resource-allocation-review-loop-001、resource-allocation-trigger-action-recheck-001
- actual Guide：DG-0927
- selected Wiki：resource-allocation-consumption-horizon-baseline-001、resource-allocation-horizon-replenishment-001、resource-allocation-uncertainty-scenarios-001、energy-device-consumption-baseline-001、energy-offgrid-capacity-inventory-001、energy-solar-seasonal-output-baseline-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：partial / ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.
- root cause：B. expected Guide candidate outside selected top3、E. expected Wiki outside candidates

### O01

- query：食物多，但燃料不足，应该先减少哪类活动？
- expected primary / accepted secondary：resource_allocation / energy_management、food
- expected / accepted Guide：DG-0929 / DG-0929、DG-0927、DG-0928
- expected Wiki：resource-allocation-substitution-degraded-mode-001
- actual Guide：DG-0360
- selected Wiki：无
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：A. expected Guide absent from candidates、C. primary ownership competition、E. expected Wiki outside candidates

### O02

- query：只有一个备用零件，现在要用还是留给以后？
- expected primary / accepted secondary：resource_allocation / storage、repair
- expected / accepted Guide：DG-0928 / DG-0928、DG-0929
- expected Wiki：resource-allocation-reserve-class-policy-001
- actual Guide：DG-0916
- selected Wiki：records-lifecycle-state-model-001、records-sop-version-minimum-fields-001、records-current-version-selection-001、records-superseded-version-isolation-001、records-rollback-revalidation-001、records-high-risk-knowledge-review-gate-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：A. expected Guide absent from candidates、C. primary ownership competition、E. expected Wiki outside candidates

### O03

- query：库存记录和实物经常对不上，应该以哪个为准？
- expected primary / accepted secondary：resource_allocation / records、storage
- expected / accepted Guide：DG-0926 / DG-0926、DG-0126
- expected Wiki：resource-allocation-available-resource-status-001、general-storage-inventory-card-minimum-001
- actual Guide：DG-0926
- selected Wiki：resource-allocation-available-resource-status-001、resource-allocation-uncertainty-scenarios-001、general-storage-inventory-card-minimum-001、general-inventory-consumption-log-001、energy-offgrid-capacity-inventory-001、food-inventory-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

### O04

- query：未来两周可能连续阴天，现有太阳能系统怎么安排资源？
- expected primary / accepted secondary：energy_deepening / resource_allocation、energy_management
- expected / accepted Guide：DG-0920、DG-0919 / DG-0920、DG-0919、DG-0927、DG-0929
- expected Wiki：energy-solar-multi-day-cloudy-degraded-mode-001
- actual Guide：DG-0617
- selected Wiki：energy-solar-daytime-charge-schedule-001、energy-cloudy-solar-downgrade-001、energy-solar-charge-target-selection-001、energy-solar-weather-pattern-log-001、energy-battery-001、energy-battery-002
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：energy_solar_low_output_management
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：B. expected Guide candidate outside selected top3、C. primary ownership competition、E. expected Wiki outside candidates

### O05

- query：生产原料只够做一种东西，应该生产哪个？
- expected primary / accepted secondary：resource_allocation / production
- expected / accepted Guide：DG-0928 / DG-0928、DG-0929
- expected Wiki：resource-allocation-resource-specific-priority-001、production-workshop-material-reservation-line-001
- actual Guide：DG-0905
- selected Wiki：production-workshop-raw-material-intake-card-001、production-workshop-wip-zone-layout-001、production-workshop-wip-handover-tag-001、production-workshop-finished-goods-hold-zone-001、production-workshop-waste-transfer-card-001、production-workshop-batch-label-minimum-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：A. expected Guide absent from candidates、C. primary ownership competition、F. expected Wiki outside selected evidence

### O06

- query：卫生耗材只够半个月，需要减少使用吗？
- expected primary / accepted secondary：resource_allocation / hygiene、medical、storage
- expected / accepted Guide：DG-0927、DG-0928 / DG-0927、DG-0928、DG-0929
- expected Wiki：resource-allocation-safety-minimum-register-001、resource-allocation-consumption-horizon-baseline-001
- actual Guide：DG-0662
- selected Wiki：hygiene-animal-carcass-001、hygiene-blister-001、hygiene-blood-contamination-001、hygiene-boiling-001、hygiene-children-001、hygiene-cleaning-sequence-001
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：fail / frozen ownership and evidence did not enter the selected set.
- root cause：A. expected Guide absent from candidates、C. primary ownership competition、E. expected Wiki outside candidates

### O07

- query：资源不足时，是不是应该让最有技能的人优先获得更多资源？
- expected primary / accepted secondary：resource_allocation / team_operations、organization
- expected / accepted Guide：DG-0928 / DG-0928
- expected Wiki：resource-allocation-resource-specific-priority-001
- actual Guide：DG-0929
- selected Wiki：resource-allocation-horizon-replenishment-001、resource-allocation-substitution-degraded-mode-001、resource-allocation-trigger-action-recheck-001、energy-solar-multi-day-cloudy-degraded-mode-001、energy-generation-component-failure-degraded-path-001、water-shortage-response-041
- Contract status / rule / primary：COMPARE_ONLY /  / unassigned
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：0
- profiles：无
- verdict：partial / ownership is acceptable but one frozen Guide/Wiki precision condition is incomplete.
- root cause：B. expected Guide candidate outside selected top3、E. expected Wiki outside candidates

### O08

- query：电池鼓包了，但这是最后一块可用电池。
- expected primary / accepted secondary：energy_safety / fire
- expected / accepted Guide：DG-0841、DG-0616 / DG-0841、DG-0616
- expected Wiki：energy-damaged-power-bank-quarantine-001、energy-battery-leak-corrosion-isolation-001
- actual Guide：DG-0841
- selected Wiki：energy-battery-parallel-series-boundary-001、energy-battery-chemistry-mix-stop-001、energy-battery-leak-corrosion-isolation-001、energy-damaged-power-bank-quarantine-001、energy-battery-storage-temperature-boundary-001
- Contract status / rule / primary：COMPARE_ONLY / battery_abnormal / energy_safety
- Safety Gate：not_applicable
- answer generated / dangerous suggestion：False / 0
- evidence classifier diagnostic hits：1
- profiles：energy_battery_abnormal_isolation
- verdict：pass / accepted ownership, Guide, Wiki, and precise evidence chain are present.
- root cause：无

## 10. Artifact Isolation

Runner 只写显式 results/report 路径；默认使用 Batch17-D 独立 artifact，不覆盖 Batch17-C 冻结报告或历史结果。

## 11. Boundary Statement

本次运行不修改 Wiki、Guide、Guide-Wiki、profile、Contract、Retrieval、Prompt、ranking、top_k、selector、fallback、schema、PocketBase、FT-02、Core、.env 或 data/lanternbox.db。
