# Batch5-J Shelter / Fire / WASH Retrieval Field Test Report

生成时间：2026-07-15T16:12:13.445312+00:00

## 1. 测试范围

本阶段只测试 Batch5-I 新增 Shelter / Fire / Clothing / WASH Guide/Wiki 是否进入本地 Retrieval evidence。脚本默认不调用 LLM，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。

覆盖：临时住所选址、防雨防潮、睡眠区保温、通风边界、室内燃烧和一氧化碳停止线、火源使用前检查、湿冷衣物和脚部保护、PPE 污染边界、小团队 WASH 分区、洗手水优先级、桶厕、病人用品和厨房污染隔离。

## 2. 汇总

- 用例总数：16
- strict / observation：14 / 2
- pass / partial / fail：16 / 0 / 0
- Guide 命中率（严格用例，含 allowed secondary）：100.0%
- 主 Guide 命中率（严格用例，仅 expected）：100.0%
- Wiki 命中率（严格用例）：100.0%
- Guide-Wiki 精准组合率（严格用例）：100.0%
- safety boundary 覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- dangerous suggestion 数量：0
- Kiwix 越权数量：0
- 跨域竞争数量：0

## 3. Case 明细

| case | type | verdict | Guide | Wiki | profiles | safety | fallback | record | cross domain | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| shelter_sleep_where_tonight | strict | pass | DG-0847 临时住所选址与防雨防潮、DG-0827 先在水源附近设一个临时取水点、DG-0111 电线和插线板安全检查 | shelter-site-selection-weather-exposure-001 临时住所选址的风雨和暴露判断、shelter-rain-leak-first-line-001 漏雨时先保护睡眠区和物资区、shelter-ground-moisture-barrier-001 潮湿地面的隔离层判断、shelter-roof-wall-floor-seepage-signs-001 屋顶墙面地面渗水信号、shelter-daily-habitability-check-001 长期居住点每日可住性检查、water-container-labeling-004 储水容器标签与编号 | 无 | 是 | 是 | 是 | 无 | 无 |
| shelter_rain_leak_stay_or_move | strict | pass | DG-0847 临时住所选址与防雨防潮、DG-0851 灰烬与余火处理、DG-0058 洪水退去后：别急着回家吃东西 | shelter-site-selection-weather-exposure-001 临时住所选址的风雨和暴露判断、shelter-rain-leak-first-line-001 漏雨时先保护睡眠区和物资区、shelter-ground-moisture-barrier-001 潮湿地面的隔离层判断、shelter-roof-wall-floor-seepage-signs-001 屋顶墙面地面渗水信号、shelter-daily-habitability-check-001 长期居住点每日可住性检查、fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理 | 无 | 是 | 是 | 是 | 无 | 无 |
| shelter_wet_ground_sleep | strict | pass | DG-0848 睡眠区保温和地面隔离、DG-0850 室内燃烧和一氧化碳停止线、DG-0594 潮湿寒冷：衣物分层和睡眠保温 | shelter-sleep-heat-loss-ground-001 睡眠区地面失温风险、shelter-ground-moisture-barrier-001 潮湿地面的隔离层判断、shelter-ventilation-heat-balance-001 保温和通风的冲突平衡、clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-layering-work-rest-adjustment-001 干活和休息时的分层调整、fire-indoor-combustion-no-go-zone-001 室内燃烧禁区和停止线 | shelter_ventilation_heat_balance | 是 | 是 | 是 | 无 | 无 |
| shelter_ventilation_keep_warm | strict | pass | DG-0848 睡眠区保温和地面隔离、DG-0850 室内燃烧和一氧化碳停止线、DG-0360 无火保暖方法 | shelter-sleep-heat-loss-ground-001 睡眠区地面失温风险、shelter-ground-moisture-barrier-001 潮湿地面的隔离层判断、shelter-ventilation-heat-balance-001 保温和通风的冲突平衡、clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-layering-work-rest-adjustment-001 干活和休息时的分层调整、fire-indoor-combustion-no-go-zone-001 室内燃烧禁区和停止线 | shelter_ventilation_heat_balance | 是 | 是 | 是 | 无 | 无 |
| fire_indoor_charcoal_heating | strict | pass | DG-0850 室内燃烧和一氧化碳停止线、DG-0848 睡眠区保温和地面隔离、DG-0591 室内取暖：通风和一氧化碳风险 | fire-indoor-combustion-no-go-zone-001 室内燃烧禁区和停止线、fire-carbon-monoxide-suspect-stop-001 疑似一氧化碳时停止取暖、fire-smoke-backdraft-room-response-001 烟雾反流时的开窗和撤离判断、shelter-ventilation-heat-balance-001 保温和通风的冲突平衡、fire-indoor-combustion-carbon-monoxide-001 室内燃烧一氧化碳风险、shelter-sleep-heat-loss-ground-001 睡眠区地面失温风险 | shelter_ventilation_heat_balance | 是 | 是 | 是 | 无 | 无 |
| fire_before_lighting_check | strict | pass | DG-0849 火源使用前检查、DG-0827 先在水源附近设一个临时取水点、DG-0602 洪水后取水点禁用判断 | fire-before-lighting-site-check-001 生火前场地和周边检查、fire-dry-wet-fuel-sorting-001 干湿燃料分级和禁用判断、fire-temporary-stove-stability-boundary-001 临时炉具稳定性和禁用边界、fire-children-bystander-clear-zone-001 儿童和旁人远离火源区、shelter-kitchen-fire-sleep-distance-001 厨房火源区和睡眠区距离、water-container-labeling-004 储水容器标签与编号 | 无 | 是 | 是 | 是 | 无 | 无 |
| fire_temporary_stove_unstable | strict | pass | DG-0849 火源使用前检查、DG-0224 临时炉具安全、DG-0851 灰烬与余火处理 | fire-before-lighting-site-check-001 生火前场地和周边检查、fire-dry-wet-fuel-sorting-001 干湿燃料分级和禁用判断、fire-temporary-stove-stability-boundary-001 临时炉具稳定性和禁用边界、fire-children-bystander-clear-zone-001 儿童和旁人远离火源区、shelter-kitchen-fire-sleep-distance-001 厨房火源区和睡眠区距离、fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理 | 无 | 是 | 是 | 是 | 无 | 无 |
| fire_smoke_backdraft_room | strict | pass | DG-0850 室内燃烧和一氧化碳停止线、DG-0849 火源使用前检查、DG-0489 洪水后进入检查 | fire-indoor-combustion-no-go-zone-001 室内燃烧禁区和停止线、fire-carbon-monoxide-suspect-stop-001 疑似一氧化碳时停止取暖、fire-smoke-backdraft-room-response-001 烟雾反流时的开窗和撤离判断、shelter-ventilation-heat-balance-001 保温和通风的冲突平衡、fire-indoor-combustion-carbon-monoxide-001 室内燃烧一氧化碳风险、fire-before-lighting-site-check-001 生火前场地和周边检查 | fire_smoke_combustion_stop | 是 | 是 | 是 | 无 | 无 |
| fire_hot_ash_trash_bag | observation | pass | DG-0851 灰烬与余火处理、DG-0488 灰烬和焦物处理、DG-0627 垃圾渗漏后的地面清理 | fire-ash-ember-cooling-disposal-001 灰烬和余火冷却处理、fire-night-final-extinguish-log-001 夜间火源熄灭记录、fire-small-fire-stop-001 初起小火处置停止线、fire-fire-response-001 火灾小火处理的停止线、hygiene-hand-hygiene-001 粪口传播和手卫生关键时刻、hygiene-handwashing-001 少水洗手的替代流程原理 | 无 | 是 | 是 | 是 | 无 | 无 |
| clothing_wet_socks_outing | strict | pass | DG-0852 湿冷衣物和脚部保护、DG-0316 袜子轮换和干燥、DG-0058 洪水退去后：别急着回家吃东西 | clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-foot-check-after-wet-work-001 湿作业后的脚部检查、clothing-shoe-sole-failure-outing-stop-001 鞋底损坏后的外出停止线、clothing-glove-contamination-cut-boundary-001 手套使用和破损污染边界、clothing-contaminated-laundry-zone-001 污染衣物临时存放区、food-post-flood-food-high-risk-001 洪水后食物默认高风险 | clothing_wet_cold_ppe | 是 | 是 | 是 | 无 | 无 |
| clothing_shivering_keep_working | strict | pass | DG-0852 湿冷衣物和脚部保护、DG-0018 失温：保温、干燥、缓慢复温、DG-0061 极寒断电过夜 | clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-foot-check-after-wet-work-001 湿作业后的脚部检查、clothing-shoe-sole-failure-outing-stop-001 鞋底损坏后的外出停止线、clothing-glove-contamination-cut-boundary-001 手套使用和破损污染边界、clothing-contaminated-laundry-zone-001 污染衣物临时存放区、fire-hypothermia-002 低温失温基础 | clothing_wet_cold_ppe | 是 | 是 | 是 | 无 | 无 |
| clothing_gloves_contaminated | strict | pass | DG-0852 湿冷衣物和脚部保护、DG-0478 照护者自我保护、DG-0058 洪水退去后：别急着回家吃东西 | clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-foot-check-after-wet-work-001 湿作业后的脚部检查、clothing-shoe-sole-failure-outing-stop-001 鞋底损坏后的外出停止线、clothing-glove-contamination-cut-boundary-001 手套使用和破损污染边界、clothing-contaminated-laundry-zone-001 污染衣物临时存放区、food-post-flood-food-high-risk-001 洪水后食物默认高风险 | clothing_wet_cold_ppe | 是 | 是 | 是 | 无 | 无 |
| wash_water_toilet_kitchen_layout | strict | pass | DG-0853 小团队 WASH 分区运行、DG-0626 桶厕满袋封存和更换、DG-0604 水桶异味后的降级处理 | hygiene-wash-zone-layout-minimum-001 饮水洗手厕所厨房最小分区、hygiene-handwater-priority-table-001 洗手水优先级表、hygiene-bucket-toilet-changeover-001 桶厕更换和封存流程、hygiene-contamination-zone-visible-marking-001 污染区可见标记方法、hygiene-daily-wash-round-checklist-001 每日 WASH 巡查表、hygiene-wash-abnormal-record-001 卫生异常记录和追溯 | wash_zone_layout_priority | 是 | 是 | 是 | 无 | 无 |
| wash_limited_handwater | strict | pass | DG-0853 小团队 WASH 分区运行、DG-0604 水桶异味后的降级处理、DG-0081 应急洗手站：少水也要能洗手 | hygiene-wash-zone-layout-minimum-001 饮水洗手厕所厨房最小分区、hygiene-handwater-priority-table-001 洗手水优先级表、hygiene-bucket-toilet-changeover-001 桶厕更换和封存流程、hygiene-contamination-zone-visible-marking-001 污染区可见标记方法、hygiene-daily-wash-round-checklist-001 每日 WASH 巡查表、hygiene-wash-abnormal-record-001 卫生异常记录和追溯 | wash_zone_layout_priority | 是 | 是 | 是 | 无 | 无 |
| wash_bucket_toilet_full | strict | pass | DG-0853 小团队 WASH 分区运行、DG-0626 桶厕满袋封存和更换、DG-0571 厕所不能冲水：桶厕和覆盖材料 | hygiene-wash-zone-layout-minimum-001 饮水洗手厕所厨房最小分区、hygiene-handwater-priority-table-001 洗手水优先级表、hygiene-bucket-toilet-changeover-001 桶厕更换和封存流程、hygiene-contamination-zone-visible-marking-001 污染区可见标记方法、hygiene-daily-wash-round-checklist-001 每日 WASH 巡查表、hygiene-wash-abnormal-record-001 卫生异常记录和追溯 | wash_zone_layout_priority | 是 | 是 | 是 | 无 | 无 |
| wash_patient_cup_kitchen | observation | pass | DG-0854 病人用品与厨房污染隔离、DG-0083 病人用品隔离：杯子、毛巾、餐具、DG-0820 先给病人和照护人员留更安全的水 | hygiene-patient-cup-towel-isolation-001 病人杯子毛巾餐具隔离、hygiene-kitchen-raw-cooked-contamination-line-001 厨房生熟和污染线、hygiene-wash-abnormal-record-001 卫生异常记录和追溯、hygiene-shared-items-001 多人共用毛巾和餐具风险、hygiene-contaminated-surface-001 食物接触污染表面的判断、water-drinking-priority-017 饮用水优先级与配水 | 无 | 是 | 是 | 是 | 无 | 无 |

## 4. 逐条复盘

### shelter_sleep_where_tonight

- query：今晚只能临时住在一个漏风的房间里，地上有点潮，应该先怎么判断睡哪里比较安全？
- 类型：strict
- focus：临时住所选址、防潮、睡眠区。
- verdict：pass
- expected Guide：DG-0847、DG-0848
- allowed secondary：无
- selected Guide：DG-0847、DG-0827、DG-0111
- selected Wiki：shelter-site-selection-weather-exposure-001、shelter-rain-leak-first-line-001、shelter-ground-moisture-barrier-001、shelter-roof-wall-floor-seepage-signs-001、shelter-daily-habitability-check-001、water-container-labeling-004
- profiles：无
- domains：shelter、water、hygiene、records、power、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### shelter_rain_leak_stay_or_move

- query：房间一角一直漏雨，东西有点湿了，今晚还能不能继续住这里？
- 类型：strict
- focus：Shelter 主导，evacuation 只作为停止线补充。
- verdict：pass
- expected Guide：DG-0847
- allowed secondary：DG-0068
- selected Guide：DG-0847、DG-0851、DG-0058
- selected Wiki：shelter-site-selection-weather-exposure-001、shelter-rain-leak-first-line-001、shelter-ground-moisture-barrier-001、shelter-roof-wall-floor-seepage-signs-001、shelter-daily-habitability-check-001、fire-ash-ember-cooling-disposal-001
- profiles：无
- domains：shelter、water、hygiene、records、fire、evacuation、security、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### shelter_wet_ground_sleep

- query：地面很冷很潮，没有床，晚上睡觉怎么垫才不容易失温？
- 类型：strict
- focus：地面隔离和睡眠保温。
- verdict：pass
- expected Guide：DG-0848
- allowed secondary：无
- selected Guide：DG-0848、DG-0850、DG-0594
- selected Wiki：shelter-sleep-heat-loss-ground-001、shelter-ground-moisture-barrier-001、shelter-ventilation-heat-balance-001、clothing-wet-cold-early-hypothermia-signs-001、clothing-layering-work-rest-adjustment-001、fire-indoor-combustion-no-go-zone-001
- profiles：shelter_ventilation_heat_balance
- domains：shelter、clothing、medical、fire
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### shelter_ventilation_keep_warm

- query：晚上很冷，大家想把门窗都堵死保暖，这样可以吗？
- 类型：strict
- focus：保温和通风边界。
- verdict：pass
- expected Guide：DG-0848
- allowed secondary：DG-0850
- selected Guide：DG-0848、DG-0850、DG-0360
- selected Wiki：shelter-sleep-heat-loss-ground-001、shelter-ground-moisture-barrier-001、shelter-ventilation-heat-balance-001、clothing-wet-cold-early-hypothermia-signs-001、clothing-layering-work-rest-adjustment-001、fire-indoor-combustion-no-go-zone-001
- profiles：shelter_ventilation_heat_balance
- domains：shelter、clothing、medical、fire、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### fire_indoor_charcoal_heating

- query：停电又很冷，能不能在室内烧炭取暖，开一点窗是不是就行？
- 类型：strict
- focus：室内燃烧和一氧化碳停止线。必须有明确禁用边界。
- verdict：pass
- expected Guide：DG-0850
- allowed secondary：无
- selected Guide：DG-0850、DG-0848、DG-0591
- selected Wiki：fire-indoor-combustion-no-go-zone-001、fire-carbon-monoxide-suspect-stop-001、fire-smoke-backdraft-room-response-001、shelter-ventilation-heat-balance-001、fire-indoor-combustion-carbon-monoxide-001、shelter-sleep-heat-loss-ground-001
- profiles：shelter_ventilation_heat_balance
- domains：shelter、medical、fire、clothing
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### fire_before_lighting_check

- query：准备在门口附近生火做点热水，点火前要先检查什么？
- 类型：strict
- focus：火源使用前检查。
- verdict：pass
- expected Guide：DG-0849
- allowed secondary：无
- selected Guide：DG-0849、DG-0827、DG-0602
- selected Wiki：fire-before-lighting-site-check-001、fire-dry-wet-fuel-sorting-001、fire-temporary-stove-stability-boundary-001、fire-children-bystander-clear-zone-001、shelter-kitchen-fire-sleep-distance-001、water-container-labeling-004
- profiles：无
- domains：shelter、food、clothing、fire、water
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### fire_temporary_stove_unstable

- query：临时炉具有点晃，但是还能烧，能不能先凑合用？
- 类型：strict
- focus：临时炉具稳定性和禁用边界。
- verdict：pass
- expected Guide：DG-0849
- allowed secondary：无
- selected Guide：DG-0849、DG-0224、DG-0851
- selected Wiki：fire-before-lighting-site-check-001、fire-dry-wet-fuel-sorting-001、fire-temporary-stove-stability-boundary-001、fire-children-bystander-clear-zone-001、shelter-kitchen-fire-sleep-distance-001、fire-ash-ember-cooling-disposal-001
- profiles：无
- domains：shelter、food、clothing、fire、water、security、hygiene
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### fire_smoke_backdraft_room

- query：做饭的时候烟一直往屋里倒灌，应该开窗还是赶紧灭火？
- 类型：strict
- focus：烟雾反流、通风、停止线。
- verdict：pass
- expected Guide：DG-0850
- allowed secondary：DG-0849
- selected Guide：DG-0850、DG-0849、DG-0489
- selected Wiki：fire-indoor-combustion-no-go-zone-001、fire-carbon-monoxide-suspect-stop-001、fire-smoke-backdraft-room-response-001、shelter-ventilation-heat-balance-001、fire-indoor-combustion-carbon-monoxide-001、fire-before-lighting-site-check-001
- profiles：fire_smoke_combustion_stop
- domains：shelter、medical、fire、food、clothing、water、evacuation、security、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### fire_hot_ash_trash_bag

- query：灰烬看起来灭了但还有点热，可以直接倒进垃圾袋吗？
- 类型：observation
- focus：验证 DG-0851 新增后，灰烬余火场景由灰烬与余火处理 Guide 主导，且 Fire Wiki evidence 进入证据链。
- verdict：pass
- expected Guide：DG-0851
- allowed secondary：DG-0223、DG-0488、DG-0849、DG-0850
- selected Guide：DG-0851、DG-0488、DG-0627
- selected Wiki：fire-ash-ember-cooling-disposal-001、fire-night-final-extinguish-log-001、fire-small-fire-stop-001、fire-fire-response-001、hygiene-hand-hygiene-001、hygiene-handwashing-001
- profiles：无
- domains：shelter、hygiene、fire、water、medical、security、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### clothing_wet_socks_outing

- query：袜子湿了，鞋里也有水，但还要出去找东西，能不能继续走？
- 类型：strict
- focus：湿袜、鞋内进水、外出停止线。
- verdict：pass
- expected Guide：DG-0852
- allowed secondary：无
- selected Guide：DG-0852、DG-0316、DG-0058
- selected Wiki：clothing-wet-cold-early-hypothermia-signs-001、clothing-foot-check-after-wet-work-001、clothing-shoe-sole-failure-outing-stop-001、clothing-glove-contamination-cut-boundary-001、clothing-contaminated-laundry-zone-001、food-post-flood-food-high-risk-001
- profiles：clothing_wet_cold_ppe
- domains：shelter、clothing、medical、hygiene、evacuation、security、disaster、general
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### clothing_shivering_keep_working

- query：人已经冷得一直发抖，但活还没干完，可以继续撑一下吗？
- 类型：strict
- focus：湿冷失温早期信号和停工边界。
- verdict：pass
- expected Guide：DG-0852
- allowed secondary：DG-0848
- selected Guide：DG-0852、DG-0018、DG-0061
- selected Wiki：clothing-wet-cold-early-hypothermia-signs-001、clothing-foot-check-after-wet-work-001、clothing-shoe-sole-failure-outing-stop-001、clothing-glove-contamination-cut-boundary-001、clothing-contaminated-laundry-zone-001、fire-hypothermia-002
- profiles：clothing_wet_cold_ppe
- domains：shelter、clothing、medical、hygiene、security、water、power、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### clothing_gloves_contaminated

- query：清理污染物的时候手套破了，但手还没碰到东西，可以继续用吗？
- 类型：strict
- focus：PPE 破损污染边界。
- verdict：pass
- expected Guide：DG-0852
- allowed secondary：无
- selected Guide：DG-0852、DG-0478、DG-0058
- selected Wiki：clothing-wet-cold-early-hypothermia-signs-001、clothing-foot-check-after-wet-work-001、clothing-shoe-sole-failure-outing-stop-001、clothing-glove-contamination-cut-boundary-001、clothing-contaminated-laundry-zone-001、food-post-flood-food-high-risk-001
- profiles：clothing_wet_cold_ppe
- domains：shelter、clothing、medical、hygiene、water、evacuation、security、disaster
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### wash_water_toilet_kitchen_layout

- query：地方很小，水桶、做饭的地方和桶厕都只能放在一个房间附近，怎么分区？
- 类型：strict
- focus：WASH 分区主导，water/food 只补充。
- verdict：pass
- expected Guide：DG-0853
- allowed secondary：无
- selected Guide：DG-0853、DG-0626、DG-0604
- selected Wiki：hygiene-wash-zone-layout-minimum-001、hygiene-handwater-priority-table-001、hygiene-bucket-toilet-changeover-001、hygiene-contamination-zone-visible-marking-001、hygiene-daily-wash-round-checklist-001、hygiene-wash-abnormal-record-001
- profiles：wash_zone_layout_priority
- domains：water、shelter、food、records、hygiene
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### wash_limited_handwater

- query：洗手水不够了，哪些时候必须洗手，哪些可以降级处理？
- 类型：strict
- focus：洗手水优先级。不要被普通水配给完全抢走。
- verdict：pass
- expected Guide：DG-0853
- allowed secondary：DG-0572
- selected Guide：DG-0853、DG-0604、DG-0081
- selected Wiki：hygiene-wash-zone-layout-minimum-001、hygiene-handwater-priority-table-001、hygiene-bucket-toilet-changeover-001、hygiene-contamination-zone-visible-marking-001、hygiene-daily-wash-round-checklist-001、hygiene-wash-abnormal-record-001
- profiles：wash_zone_layout_priority
- domains：water、shelter、food、records、hygiene、tools、medical
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### wash_bucket_toilet_full

- query：桶厕快满了，还有味道，什么时候必须更换或封起来？
- 类型：strict
- focus：桶厕更换、封存和记录。
- verdict：pass
- expected Guide：DG-0853
- allowed secondary：无
- selected Guide：DG-0853、DG-0626、DG-0571
- selected Wiki：hygiene-wash-zone-layout-minimum-001、hygiene-handwater-priority-table-001、hygiene-bucket-toilet-changeover-001、hygiene-contamination-zone-visible-marking-001、hygiene-daily-wash-round-checklist-001、hygiene-wash-abnormal-record-001
- profiles：wash_zone_layout_priority
- domains：water、shelter、food、records、hygiene
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### wash_patient_cup_kitchen

- query：病人用过的杯子和毛巾不小心放到厨房边上了，现在怎么处理？
- 类型：observation
- focus：观察未新增 DG-0854 时，病人用品与厨房污染 Wiki 能否进入 evidence。若 WASH 主 Guide 命中但 Wiki 不全，可 partial。
- verdict：pass
- expected Guide：DG-0853
- allowed secondary：DG-0083
- selected Guide：DG-0854、DG-0083、DG-0820
- selected Wiki：hygiene-patient-cup-towel-isolation-001、hygiene-kitchen-raw-cooked-contamination-line-001、hygiene-wash-abnormal-record-001、hygiene-shared-items-001、hygiene-contaminated-surface-001、water-drinking-priority-017
- profiles：无
- domains：water、medical、food、records、hygiene、shelter
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

## 5. Root Cause 初步分类

- 无 partial/fail root cause。

## 6. 跨域竞争统计

- 未发现 evacuation / disaster / energy / medical / water / food 完全抢主位。

## 7. 是否建议进入 Batch5-K Root Cause Review

暂不需要 Batch5-K 修复；可继续扩大 Shelter / Fire / WASH 场景覆盖。

## 8. 边界声明

- 本批没有修改 Wiki 正文、Guide 正文、Guide-Wiki 关系、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。
- 本批没有修复 partial/fail，也没有宣告 Shelter / Fire / WASH Retrieval stable。
- 本脚本只调用本地 Guide/Wiki fetchers，不调用 LLM；Kiwix 越权按未进入本地证据选择路径记录为 0。
