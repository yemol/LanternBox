# Batch5-B Energy Safety Retrieval Field Test Report

生成时间：2026-07-14T11:58:04.482040+00:00

## 1. 测试范围

本阶段只测试 Batch5-A1 新增能源安全 Guide/Wiki 是否进入本地 Retrieval evidence。脚本不调用 LLM，不修改 Wiki、Guide、关联、profile、Pipeline、Prompt、top_k、selector limit 或 fallback。

覆盖：电池异常隔离、低压设备异常停用、未知电源安全判断，以及长期能源/太阳能两个观察型场景。

## 2. 汇总

- 用例总数：10
- 严格用例：8
- 观察用例：2
- pass / partial / fail：8 / 2 / 0
- Guide 命中率：100.0%
- Wiki 命中率：87.5%
- Guide-Wiki 精准组合：100.0%
- 安全边界覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- 危险建议：0
- 跨域竞争：1

## 3. 测试用例列表

| case | verdict | Guide | Wiki | safety | fallback | record/check | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- |
| energy_battery_power_bank_swollen | pass | DG-0841 电池异常隔离、DG-0546 插线板和延长线进水后：禁止通电与报废判断、DG-0168 家禽饮水和粪便管理 | energy-battery-parallel-series-boundary-001 电池串并联禁用边界、energy-battery-chemistry-mix-stop-001 不同电池类型混用停用边界、energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离、energy-damaged-power-bank-quarantine-001 受摔移动电源隔离观察、energy-battery-storage-temperature-boundary-001 电池存放温度边界、energy-basic-electrical-safety-index-001 基础电学安全索引 | 是 | 是 | 是 | 无 |
| energy_spare_battery_hot_after_storage | pass | DG-0841 电池异常隔离、DG-0546 插线板和延长线进水后：禁止通电与报废判断、DG-0168 家禽饮水和粪便管理 | energy-battery-parallel-series-boundary-001 电池串并联禁用边界、energy-battery-chemistry-mix-stop-001 不同电池类型混用停用边界、energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离、energy-damaged-power-bank-quarantine-001 受摔移动电源隔离观察、energy-battery-storage-temperature-boundary-001 电池存放温度边界、energy-basic-electrical-safety-index-001 基础电学安全索引 | 是 | 是 | 是 | 无 |
| energy_battery_leak_handling | pass | DG-0841 电池异常隔离、DG-0168 家禽饮水和粪便管理、DG-0546 插线板和延长线进水后：禁止通电与报废判断 | energy-battery-parallel-series-boundary-001 电池串并联禁用边界、energy-battery-chemistry-mix-stop-001 不同电池类型混用停用边界、energy-battery-leak-corrosion-isolation-001 电池漏液和腐蚀隔离、energy-damaged-power-bank-quarantine-001 受摔移动电源隔离观察、energy-battery-storage-temperature-boundary-001 电池存放温度边界、energy-basic-electrical-safety-index-001 基础电学安全索引 | 是 | 是 | 是 | 无 |
| energy_usb_device_overheating | pass | DG-0842 低压设备异常停用、DG-0382 不熟悉设备不拆原则、DG-0111 电线和插线板安全检查 | energy-low-voltage-system-stop-boundary-001 低压直流系统停用边界、energy-dc-polarity-reverse-check-001 直流正负极接反检查、energy-short-circuit-warning-signs-001 短路前兆和立即断开顺序、energy-wire-heating-load-limit-001 线缆发热和负载过大判断、energy-loose-connector-stop-use-001 电源接头松动停用边界、energy-wet-low-voltage-device-no-test-001 潮湿低压设备也不盲目通电 | 是 | 是 | 是 | 无 |
| energy_wire_getting_hot_after_wiring | pass | DG-0842 低压设备异常停用、DG-0843 未知电源安全判断、DG-0110 小型发电机使用边界 | energy-low-voltage-system-stop-boundary-001 低压直流系统停用边界、energy-dc-polarity-reverse-check-001 直流正负极接反检查、energy-short-circuit-warning-signs-001 短路前兆和立即断开顺序、energy-wire-heating-load-limit-001 线缆发热和负载过大判断、energy-loose-connector-stop-use-001 电源接头松动停用边界、energy-wet-low-voltage-device-no-test-001 潮湿低压设备也不盲目通电 | 是 | 是 | 是 | 无 |
| energy_red_black_reverse_polarity | pass | DG-0842 低压设备异常停用、DG-0040 停电后冰箱食物处置、DG-0045 盐腌保存：盐量、容器、复查 | energy-low-voltage-system-stop-boundary-001 低压直流系统停用边界、energy-dc-polarity-reverse-check-001 直流正负极接反检查、energy-short-circuit-warning-signs-001 短路前兆和立即断开顺序、energy-wire-heating-load-limit-001 线缆发热和负载过大判断、energy-loose-connector-stop-use-001 电源接头松动停用边界、energy-wet-low-voltage-device-no-test-001 潮湿低压设备也不盲目通电 | 是 | 是 | 是 | 无 |
| energy_unknown_old_charger_voltage | pass | DG-0843 未知电源安全判断、DG-0616 充电宝鼓包发热停用隔离、DG-0109 太阳能板应急充电 | energy-dc-polarity-reverse-check-001 直流正负极接反检查、energy-fuse-protection-no-bypass-001 保险丝和保护件不可绕过、energy-temporary-wiring-precheck-001 临时接线前检查、energy-unattended-charging-stop-boundary-001 无人值守充电停止边界、energy-charging-zone-fire-isolation-001 集中充电区防火隔离、repair-power-cable-temporary-insulation-boundary-001 电源线临时绝缘边界 | 是 | 是 | 是 | 无 |
| energy_unknown_matching_port | partial | DG-0843 未知电源安全判断、DG-0546 插线板和延长线进水后：禁止通电与报废判断、DG-0842 低压设备异常停用 | energy-dc-polarity-reverse-check-001 直流正负极接反检查、energy-fuse-protection-no-bypass-001 保险丝和保护件不可绕过、energy-temporary-wiring-precheck-001 临时接线前检查、energy-unattended-charging-stop-boundary-001 无人值守充电停止边界、energy-charging-zone-fire-isolation-001 集中充电区防火隔离、repair-power-cable-temporary-insulation-boundary-001 电源线临时绝缘边界 | 是 | 是 | 是 | selector 问题 |
| energy_three_day_blackout_charge_priority | partial | DG-0107 设备充电优先级：先保命，再保信息、DG-0563 移动电源：关键设备优先级、DG-0564 太阳能充电：排程和保护 | energy-bank-battery-priority-001 移动电源低电量使用优先级、energy-lithium-battery-safety-index-001 锂电池安全索引、energy-power-outage-002 停电后的关键设备分级、energy-swollen-lithium-battery-stop-use-001 锂电池鼓包停止使用、energy-solar-charging-003 太阳能充电的光照因素、energy-solar-system-basics-index-001 太阳能系统基础索引 | 是 | 是 | 是 | 跨域竞争 |
| energy_cloudy_solar_panel | pass | DG-0564 太阳能充电：排程和保护、DG-0109 太阳能板应急充电、DG-0617 太阳能充电白天排程 | energy-solar-charging-003 太阳能充电的光照因素、energy-solar-system-basics-index-001 太阳能系统基础索引、energy-solar-panel-001 太阳能板基础使用、energy-battery-001 锂电池进水和短路风险、energy-battery-002 电池混用和新旧混放问题、energy-battery-003 备用电池封存和轮换原则 | 是 | 是 | 是 | 无 |

## 4. Case 明细

### energy_battery_power_bank_swollen

- query：移动电源鼓起来了还能不能继续充？
- observation：否
- verdict：pass
- selected Guide：DG-0841, DG-0546, DG-0168
- selected Wiki：energy-battery-parallel-series-boundary-001, energy-battery-chemistry-mix-stop-001, energy-battery-leak-corrosion-isolation-001, energy-damaged-power-bank-quarantine-001, energy-battery-storage-temperature-boundary-001, energy-basic-electrical-safety-index-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_spare_battery_hot_after_storage

- query：备用电池放了半年，现在发热还有必要测试吗？
- observation：否
- verdict：pass
- selected Guide：DG-0841, DG-0546, DG-0168
- selected Wiki：energy-battery-parallel-series-boundary-001, energy-battery-chemistry-mix-stop-001, energy-battery-leak-corrosion-isolation-001, energy-damaged-power-bank-quarantine-001, energy-battery-storage-temperature-boundary-001, energy-basic-electrical-safety-index-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_battery_leak_handling

- query：电池漏液了怎么处理？
- observation：否
- verdict：pass
- selected Guide：DG-0841, DG-0168, DG-0546
- selected Wiki：energy-battery-parallel-series-boundary-001, energy-battery-chemistry-mix-stop-001, energy-battery-leak-corrosion-isolation-001, energy-damaged-power-bank-quarantine-001, energy-battery-storage-temperature-boundary-001, energy-basic-electrical-safety-index-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_usb_device_overheating

- query：USB设备突然发烫还能继续用吗？
- observation：否
- verdict：pass
- selected Guide：DG-0842, DG-0382, DG-0111
- selected Wiki：energy-low-voltage-system-stop-boundary-001, energy-dc-polarity-reverse-check-001, energy-short-circuit-warning-signs-001, energy-wire-heating-load-limit-001, energy-loose-connector-stop-use-001, energy-wet-low-voltage-device-no-test-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_wire_getting_hot_after_wiring

- query：接线后线越来越热，是不是功率不够？
- observation：否
- verdict：pass
- selected Guide：DG-0842, DG-0843, DG-0110
- selected Wiki：energy-low-voltage-system-stop-boundary-001, energy-dc-polarity-reverse-check-001, energy-short-circuit-warning-signs-001, energy-wire-heating-load-limit-001, energy-loose-connector-stop-use-001, energy-wet-low-voltage-device-no-test-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_red_black_reverse_polarity

- query：红黑线接反会怎么样？
- observation：否
- verdict：pass
- selected Guide：DG-0842, DG-0040, DG-0045
- selected Wiki：energy-low-voltage-system-stop-boundary-001, energy-dc-polarity-reverse-check-001, energy-short-circuit-warning-signs-001, energy-wire-heating-load-limit-001, energy-loose-connector-stop-use-001, energy-wet-low-voltage-device-no-test-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_unknown_old_charger_voltage

- query：找到一个旧充电器，不知道是多少伏，可以试一下吗？
- observation：否
- verdict：pass
- selected Guide：DG-0843, DG-0616, DG-0109
- selected Wiki：energy-dc-polarity-reverse-check-001, energy-fuse-protection-no-bypass-001, energy-temporary-wiring-precheck-001, energy-unattended-charging-stop-boundary-001, energy-charging-zone-fire-isolation-001, repair-power-cable-temporary-insulation-boundary-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无

### energy_unknown_matching_port

- query：这个接口能插进去，但是不知道是不是匹配，可以通电看看吗？
- observation：否
- verdict：partial
- selected Guide：DG-0843, DG-0546, DG-0842
- selected Wiki：energy-dc-polarity-reverse-check-001, energy-fuse-protection-no-bypass-001, energy-temporary-wiring-precheck-001, energy-unattended-charging-stop-boundary-001, energy-charging-zone-fire-isolation-001, repair-power-cable-temporary-insulation-boundary-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：selector 问题
- failure reasons：未命中预期 Wiki

### energy_three_day_blackout_charge_priority

- query：停电三天，应该优先给什么设备充电？
- observation：是
- verdict：partial
- selected Guide：DG-0107, DG-0563, DG-0564
- selected Wiki：energy-bank-battery-priority-001, energy-lithium-battery-safety-index-001, energy-power-outage-002, energy-swollen-lithium-battery-stop-use-001, energy-solar-charging-003, energy-solar-system-basics-index-001
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：power_vs_communication
- root cause：跨域竞争
- failure reasons：跨域竞争
- note：长期能源 P1 尚未实现；观察是否错误命中通信或设备维护。

### energy_cloudy_solar_panel

- query：太阳能板今天阴天，还有必要展开吗？
- observation：是
- verdict：pass
- selected Guide：DG-0564, DG-0109, DG-0617
- selected Wiki：energy-solar-charging-003, energy-solar-system-basics-index-001, energy-solar-panel-001, energy-battery-001, energy-battery-002, energy-battery-003
- safety boundary：是
- fallback：是
- record/check：是
- dangerous suggestions：无
- cross domain：无
- root cause：无
- failure reasons：无
- note：太阳能 P1 尚未实现；观察现有太阳能 evidence。

## 5. Root Cause 分类

- selector 问题：1
- 跨域竞争：1

## 6. 跨域竞争统计

- power_vs_communication：1

## 7. 重点风险检查

- 危险建议（尝试继续使用 / 观察一下再说 / 可以测试看看）：0
- high/caution stop_or_escalate 覆盖：100.0%
- high/caution fallback 覆盖：100.0%

## 8. 结论

本阶段只记录结果，不修复 partial/fail。若存在 partial/fail，应进入 Batch5-C Root Cause Review，再决定是否属于数据缺口、profile 缺口、selector 问题、ranking 问题或 Guide 设计问题。
