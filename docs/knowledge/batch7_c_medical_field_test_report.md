# Batch7-C Medical Retrieval Field Test Report

生成时间：2026-07-16T10:50:21.806313+00:00

## 1. 测试范围

本阶段只测试 Batch7-B1 / Batch7-B2 修复后的 medical high / critical Guide-Wiki evidence 是否稳定进入本地 Retrieval selected evidence。脚本默认不调用 LLM，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。

覆盖：严重出血、噎住、昏迷但有呼吸、无正常呼吸、癫痫、头部伤、疑似脊柱伤、疑似骨折、误服药、化学皮肤眼暴露、烧烫伤、中暑、腹泻脱水、污水伤口、失温、发热意识变化，以及 6 个跨域 observation 场景。

## 2. 汇总

- 用例总数：22
- strict / observation：16 / 6
- pass / partial / fail：19 / 3 / 0
- Guide 命中率（严格用例，含 allowed secondary）：93.8%
- 主 Guide 命中率（严格用例，仅 expected）：93.8%
- Wiki 命中率（严格用例）：93.8%
- Guide-Wiki 精准组合率（严格用例）：93.8%
- high / critical Guide 命中率：93.3%
- safety boundary 覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- dangerous suggestion 数量：0
- Kiwix 越权数量：0
- cross domain 数量：0

## 3. Case 明细

| case | type | verdict | Guide | Wiki | profiles | safety | fallback | record | cross domain | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| medical_major_bleeding_soaked_cloth | strict | pass | DG-0010 头部受伤：24 小时观察、DG-0011 疑似脊柱伤：少动、固定、轴线搬运、DG-0004 严重出血：加压包扎与休克观察 | medical-head-injury-24h-observation-001 头部受伤后的 24 小时观察、medical-spine-injury-movement-boundary-001 疑似脊柱伤的移动边界、medical-bleeding-control-001 直接压迫止血为什么不能频繁查看、medical-bleeding-control-002 直接压迫止血原理、medical-fracture-immobilize-no-reposition-001 疑似骨折固定不复位原则、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察 | medical_trauma_bleeding_fracture | 是 | 是 | 是 | 无 | 无 |
| medical_choking_cannot_speak | strict | pass | DG-0013 噎住：背部拍击与腹部冲击、DG-0015 无反应无正常呼吸：胸外按压、DG-0014 昏迷但有呼吸：恢复体位 | medical-choking-airway-obstruction-001 噎住和气道异物的危险边界、medical-cpr-no-normal-breathing-001 无反应且无正常呼吸的胸外按压边界、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-seizure-protection-timing-001 癫痫发作保护与计时边界 | medical_airway_breathing_emergency | 是 | 是 | 是 | 无 | 无 |
| medical_unconscious_breathing_position | strict | pass | DG-0014 昏迷但有呼吸：恢复体位、DG-0015 无反应无正常呼吸：胸外按压、DG-0016 癫痫发作：保护头部与计时 | medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-cpr-no-normal-breathing-001 无反应且无正常呼吸的胸外按压边界、medical-seizure-protection-timing-001 癫痫发作保护与计时边界、medical-choking-airway-obstruction-001 噎住和气道异物的危险边界 | medical_airway_breathing_emergency | 是 | 是 | 是 | 无 | 无 |
| medical_no_response_no_normal_breathing | strict | pass | DG-0015 无反应无正常呼吸：胸外按压、DG-0014 昏迷但有呼吸：恢复体位、DG-0016 癫痫发作：保护头部与计时 | medical-cpr-no-normal-breathing-001 无反应且无正常呼吸的胸外按压边界、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-seizure-protection-timing-001 癫痫发作保护与计时边界、medical-choking-airway-obstruction-001 噎住和气道异物的危险边界 | medical_airway_breathing_emergency | 是 | 是 | 是 | 无 | 无 |
| medical_seizure_do_not_hold | strict | pass | DG-0016 癫痫发作：保护头部与计时、DG-0014 昏迷但有呼吸：恢复体位、DG-0015 无反应无正常呼吸：胸外按压 | medical-seizure-protection-timing-001 癫痫发作保护与计时边界、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-cpr-no-normal-breathing-001 无反应且无正常呼吸的胸外按压边界 | repair_site_clearance_boundary、medical_airway_breathing_emergency | 是 | 是 | 是 | 无 | 无 |
| medical_head_hit_vomit_sleepy | strict | pass | DG-0011 疑似脊柱伤：少动、固定、轴线搬运、DG-0010 头部受伤：24 小时观察、DG-0027 老人意识混乱观察 | medical-spine-injury-movement-boundary-001 疑似脊柱伤的移动边界、medical-head-injury-24h-observation-001 头部受伤后的 24 小时观察、medical-high-fever-consciousness-risk-001 高热伴意识异常风险、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-fracture-immobilize-no-reposition-001 疑似骨折固定不复位原则 | medical_trauma_bleeding_fracture | 是 | 是 | 是 | 无 | 无 |
| medical_fall_neck_pain_numb | strict | pass | DG-0011 疑似脊柱伤：少动、固定、轴线搬运、DG-0010 头部受伤：24 小时观察、DG-0008 扭伤：休息、冷敷、加压、抬高 | medical-spine-injury-movement-boundary-001 疑似脊柱伤的移动边界、medical-head-injury-24h-observation-001 头部受伤后的 24 小时观察、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-fracture-immobilize-no-reposition-001 疑似骨折固定不复位原则 | medical_trauma_bleeding_fracture | 是 | 是 | 是 | 无 | 无 |
| medical_fracture_no_reposition | strict | pass | DG-0011 疑似脊柱伤：少动、固定、轴线搬运、DG-0010 头部受伤：24 小时观察、DG-0009 疑似骨折：固定不复位 | medical-spine-injury-movement-boundary-001 疑似脊柱伤的移动边界、medical-head-injury-24h-observation-001 头部受伤后的 24 小时观察、medical-fracture-immobilize-no-reposition-001 疑似骨折固定不复位原则、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察 | medical_trauma_bleeding_fracture | 是 | 是 | 是 | 无 | 无 |
| medical_child_unknown_medicine_no_vomit | strict | partial | DG-0112 电池漏液处理、DG-0064 疑似化学污染：远离、脱外层、冲洗、DG-0311 不明来源容器处理 | medical-chemical-skin-eye-exposure-001 化学物接触皮肤和眼睛的冲洗边界、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁、hygiene-contamination-log-001 污染记录帮助追溯来源、hygiene-isolation-supplies-001 隔离用品不足时的替代、hygiene-contaminated-clothing-001 污染衣物和普通衣物分开 | medical_poisoning_chemical_exposure | 是 | 是 | 是 | 无 | 数据缺口、selector 问题 |
| medical_battery_leak_skin_eye | strict | pass | DG-0112 电池漏液处理、DG-0841 电池异常隔离、DG-0064 疑似化学污染：远离、脱外层、冲洗 | medical-chemical-skin-eye-exposure-001 化学物接触皮肤和眼睛的冲洗边界、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁、hygiene-contamination-log-001 污染记录帮助追溯来源、hygiene-isolation-supplies-001 隔离用品不足时的替代、energy-battery-parallel-series-boundary-001 电池串并联禁用边界 | energy_battery_abnormal_isolation、medical_poisoning_chemical_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_burn_toothpaste_soy_sauce | strict | pass | DG-0560 烧烫伤冷却与覆盖、DG-0007 烧烫伤：冷却、覆盖、后续观察、DG-0017 中暑：转移、降温、补液 | medical-burn-care-002 烧烫伤冷却原理、medical-burn-first-aid-001 烧烫伤初步处理与观察、medical-trauma-complication-index-001 常见外伤并发症索引、medical-burn-care-001 烧烫伤冷却为什么要优先、medical-heat-activity-stop-001 高温环境下何时停止外出和停下活动、medical-oral-rehydration-001 补液盐和普通饮水的差异 | medical_burn_heat_cold_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_heat_slow_response | strict | pass | DG-0560 烧烫伤冷却与覆盖、DG-0017 中暑：转移、降温、补液、DG-0007 烧烫伤：冷却、覆盖、后续观察 | medical-burn-care-002 烧烫伤冷却原理、medical-burn-first-aid-001 烧烫伤初步处理与观察、medical-trauma-complication-index-001 常见外伤并发症索引、medical-heat-activity-stop-001 高温环境下何时停止外出和停下活动、medical-oral-rehydration-001 补液盐和普通饮水的差异、medical-dehydration-001 老人发热脱水为什么更危险 | medical_burn_heat_cold_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_diarrhea_vomit_dehydration | strict | pass | DG-0559 腹泻与脱水观察、DG-0472 腹泻呕吐隔离流程、DG-0020 腹泻脱水：补液优先 | medical-dehydration-002 腹泻呕吐后的脱水分级判断、medical-oral-rehydration-001 补液盐和普通饮水的差异、medical-child-elder-dehydration-risk-001 儿童老人脱水风险、medical-diarrhea-dehydration-risk-001 腹泻与脱水风险观察、medical-dehydration-001 老人发热脱水为什么更危险、medical-high-fever-consciousness-risk-001 高热伴意识异常风险 | medical_infection_dehydration_fever | 是 | 是 | 是 | 无 | 无 |
| medical_wound_dirty_water_limited_clean | strict | pass | DG-0087 伤口避开污水：防水覆盖和事后清洗、DG-0559 腹泻与脱水观察、DG-0558 发热观察、补水和记录 | medical-wound-care-001 伤口红肿热痛和感染早期信号、medical-wound-care-002 污染伤口和破伤风风险概念、medical-clean-water-001 小伤口清洁水不足时的边界、medical-cross-infection-001 照护者交叉感染的关键时刻、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁 | medical_infection_dehydration_fever | 是 | 是 | 是 | 无 | 无 |
| medical_hypothermia_wet_clothes_fire | strict | pass | DG-0852 湿冷衣物和脚部保护、DG-0018 失温：保温、干燥、缓慢复温、DG-0560 烧烫伤冷却与覆盖 | clothing-wet-cold-early-hypothermia-signs-001 湿冷失温早期信号、clothing-foot-check-after-wet-work-001 湿作业后的脚部检查、clothing-shoe-sole-failure-outing-stop-001 鞋底损坏后的外出停止线、clothing-glove-contamination-cut-boundary-001 手套使用和破损污染边界、clothing-contaminated-laundry-zone-001 污染衣物临时存放区、fire-hypothermia-002 低温失温基础 | clothing_wet_cold_ppe、medical_burn_heat_cold_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_fever_confusion | strict | pass | DG-0558 发热观察、补水和记录、DG-0559 腹泻与脱水观察、DG-0021 发热：降温、补水、记录 | medical-fever-004 发热记录为什么重要、medical-fever-observation-record-001 发热观察与记录、medical-high-fever-consciousness-risk-001 高热伴意识异常风险、medical-dehydration-002 腹泻呕吐后的脱水分级判断、medical-oral-rehydration-001 补液盐和普通饮水的差异、medical-child-elder-dehydration-risk-001 儿童老人脱水风险 | medical_infection_dehydration_fever | 是 | 是 | 是 | 无 | 无 |
| medical_observe_fever_cough_isolation | observation | pass | DG-0021 发热：降温、补水、记录、DG-0473 咳嗽发热照护、DG-0558 发热观察、补水和记录 | medical-child-elder-dehydration-risk-001 儿童老人脱水风险、medical-common-infectious-disease-index-001 常见传染病基础索引、medical-high-fever-consciousness-risk-001 高热伴意识异常风险、medical-fever-004 发热记录为什么重要、medical-fever-observation-record-001 发热观察与记录、medical-oral-rehydration-001 补液盐和普通饮水的差异 | medical_infection_dehydration_fever | 是 | 是 | 是 | 无 | 无 |
| medical_observe_vomit_caregiver_infection | observation | partial | DG-0615 照护者处理呕吐物后防交叉感染、DG-0559 腹泻与脱水观察、DG-0558 发热观察、补水和记录 | hygiene-fecal-contamination-water-separation-001 粪便污染与饮水隔离、medical-bleeding-control-001 直接压迫止血为什么不能频繁查看、medical-clean-water-001 小伤口清洁水不足时的边界、medical-cross-infection-002 照护者手卫生与交叉感染、medical-wound-care-001 伤口红肿热痛和感染早期信号、medical-dehydration-002 腹泻呕吐后的脱水分级判断 | medical_infection_dehydration_fever | 是 | 是 | 是 | 无 | selector 问题、合理 partial |
| medical_observe_chemical_eye_clothing | observation | pass | DG-0064 疑似化学污染：远离、脱外层、冲洗、DG-0112 电池漏液处理、DG-0012 异物入眼：冲洗不揉 | medical-chemical-skin-eye-exposure-001 化学物接触皮肤和眼睛的冲洗边界、hygiene-contamination-zone-001 清洁区污染区的标记原则、hygiene-contaminated-clothing-001 污染衣物和普通衣物分开、hygiene-hygiene-knowledge-001 处理污染物后的手部清洁、hygiene-contamination-log-001 污染记录帮助追溯来源、hygiene-isolation-supplies-001 隔离用品不足时的替代 | medical_poisoning_chemical_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_observe_burn_near_fire | observation | pass | DG-0560 烧烫伤冷却与覆盖、DG-0017 中暑：转移、降温、补液、DG-0007 烧烫伤：冷却、覆盖、后续观察 | medical-burn-care-002 烧烫伤冷却原理、medical-burn-first-aid-001 烧烫伤初步处理与观察、medical-trauma-complication-index-001 常见外伤并发症索引、medical-heat-activity-stop-001 高温环境下何时停止外出和停下活动、medical-oral-rehydration-001 补液盐和普通饮水的差异、medical-dehydration-001 老人发热脱水为什么更危险 | medical_burn_heat_cold_exposure | 是 | 是 | 是 | 无 | 无 |
| medical_observe_hypothermia_no_dry_clothes | observation | partial | DG-0560 烧烫伤冷却与覆盖、DG-0017 中暑：转移、降温、补液、DG-0018 失温：保温、干燥、缓慢复温 | medical-burn-care-002 烧烫伤冷却原理、medical-burn-first-aid-001 烧烫伤初步处理与观察、medical-trauma-complication-index-001 常见外伤并发症索引、medical-heat-activity-stop-001 高温环境下何时停止外出和停下活动、medical-oral-rehydration-001 补液盐和普通饮水的差异、medical-dehydration-001 老人发热脱水为什么更危险 | medical_burn_heat_cold_exposure | 是 | 是 | 是 | 无 | selector 问题、合理 partial |
| medical_observe_cpr_background | observation | pass | DG-0015 无反应无正常呼吸：胸外按压、DG-0014 昏迷但有呼吸：恢复体位、DG-0016 癫痫发作：保护头部与计时 | medical-cpr-no-normal-breathing-001 无反应且无正常呼吸的胸外按压边界、medical-recovery-position-breathing-observation-001 昏迷但有呼吸时的体位和呼吸观察、medical-seizure-protection-timing-001 癫痫发作保护与计时边界、medical-choking-airway-obstruction-001 噎住和气道异物的危险边界 | medical_airway_breathing_emergency | 是 | 是 | 是 | 无 | 无 |

## 4. 逐条复盘

### medical_major_bleeding_soaked_cloth

- query：大量出血，布已经被血浸透，应该先做什么？
- 类型：strict
- focus：严重出血应由医疗 Guide 主导，直接压迫和持续观察进入 evidence。
- watch conflicts：hygiene
- verdict：pass
- expected Guide：DG-0004
- allowed secondary：DG-0002
- selected Guide：DG-0010、DG-0011、DG-0004
- selected Wiki：medical-head-injury-24h-observation-001、medical-spine-injury-movement-boundary-001、medical-bleeding-control-001、medical-bleeding-control-002、medical-fracture-immobilize-no-reposition-001、medical-recovery-position-breathing-observation-001
- profiles：medical_trauma_bleeding_fracture
- domains：hygiene、comms、water、medical、tools、shelter、security
- high / critical target：DG-0002、DG-0004
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_choking_cannot_speak

- query：有人噎住了，不能说话也咳不出来怎么办？
- 类型：strict
- focus：噎住和气道异物入口，不能由泛呼吸或水域抢主位。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0013
- allowed secondary：无
- selected Guide：DG-0013、DG-0015、DG-0014
- selected Wiki：medical-choking-airway-obstruction-001、medical-cpr-no-normal-breathing-001、medical-recovery-position-breathing-observation-001、medical-seizure-protection-timing-001
- profiles：medical_airway_breathing_emergency
- domains：water、medical、hygiene、security
- high / critical target：DG-0013
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_unconscious_breathing_position

- query：人昏过去了，但是还有呼吸，怎么放比较安全？
- 类型：strict
- focus：昏迷但有呼吸，恢复体位和呼吸观察。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0014
- allowed secondary：无
- selected Guide：DG-0014、DG-0015、DG-0016
- selected Wiki：medical-recovery-position-breathing-observation-001、medical-cpr-no-normal-breathing-001、medical-seizure-protection-timing-001、medical-choking-airway-obstruction-001
- profiles：medical_airway_breathing_emergency
- domains：hygiene、water、security、medical、records
- high / critical target：DG-0014
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_no_response_no_normal_breathing

- query：人没有反应，也没有正常呼吸，现在怎么办？
- 类型：strict
- focus：无反应无正常呼吸，胸外按压边界。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0015
- allowed secondary：无
- selected Guide：DG-0015、DG-0014、DG-0016
- selected Wiki：medical-cpr-no-normal-breathing-001、medical-recovery-position-breathing-observation-001、medical-seizure-protection-timing-001、medical-choking-airway-obstruction-001
- profiles：medical_airway_breathing_emergency
- domains：medical、hygiene、water、security、records
- high / critical target：DG-0015
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_seizure_do_not_hold

- query：有人癫痫发作，旁边人想按住他，可以吗？
- 类型：strict
- focus：癫痫发作保护和计时，不按压四肢、不塞嘴。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0016
- allowed secondary：无
- selected Guide：DG-0016、DG-0014、DG-0015
- selected Wiki：medical-seizure-protection-timing-001、medical-recovery-position-breathing-observation-001、medical-cpr-no-normal-breathing-001
- profiles：repair_site_clearance_boundary、medical_airway_breathing_emergency
- domains：records、medical、hygiene、water、security
- high / critical target：DG-0016
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_head_hit_vomit_sleepy

- query：头撞了一下，当时清醒，后面开始想吐和犯困，怎么办？
- 类型：strict
- focus：头部伤 24 小时观察，呕吐和嗜睡为危险信号。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0010
- allowed secondary：无
- selected Guide：DG-0011、DG-0010、DG-0027
- selected Wiki：medical-spine-injury-movement-boundary-001、medical-head-injury-24h-observation-001、medical-high-fever-consciousness-risk-001、medical-recovery-position-breathing-observation-001、medical-fracture-immobilize-no-reposition-001
- profiles：medical_trauma_bleeding_fracture
- domains：tools、shelter、security、medical、hygiene、comms、water、records
- high / critical target：DG-0010
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_fall_neck_pain_numb

- query：摔下来后脖子痛，手脚有点麻，能不能扶起来走？
- 类型：strict
- focus：疑似脊柱伤少动和移动边界。
- watch conflicts：shelter
- verdict：pass
- expected Guide：DG-0011
- allowed secondary：无
- selected Guide：DG-0011、DG-0010、DG-0008
- selected Wiki：medical-spine-injury-movement-boundary-001、medical-head-injury-24h-observation-001、medical-recovery-position-breathing-observation-001、medical-fracture-immobilize-no-reposition-001
- profiles：medical_trauma_bleeding_fracture
- domains：tools、shelter、security、medical、hygiene、comms、water
- high / critical target：DG-0011
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_fracture_no_reposition

- query：腿可能骨折了，能不能把它掰正？
- 类型：strict
- focus：疑似骨折固定不复位。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0009
- allowed secondary：无
- selected Guide：DG-0011、DG-0010、DG-0009
- selected Wiki：medical-spine-injury-movement-boundary-001、medical-head-injury-24h-observation-001、medical-fracture-immobilize-no-reposition-001、medical-recovery-position-breathing-observation-001
- profiles：medical_trauma_bleeding_fracture
- domains：tools、shelter、security、medical、hygiene、comms、water
- high / critical target：DG-0009
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_child_unknown_medicine_no_vomit

- query：孩子误吃了不知道什么药，现在要不要催吐？
- 类型：strict
- focus：误服药和不明药物，不自行催吐，保留包装和记录。
- watch conflicts：hygiene、contamination
- verdict：partial
- expected Guide：DG-0215
- allowed secondary：无
- selected Guide：DG-0112、DG-0064、DG-0311
- selected Wiki：medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001、hygiene-isolation-supplies-001、hygiene-contaminated-clothing-001
- profiles：medical_poisoning_chemical_exposure
- domains：power、hygiene、water、medical、disaster、security、tools
- high / critical target：DG-0215
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：数据缺口、selector 问题
- failure reasons：未命中预期或允许 Guide、未命中预期 Wiki

### medical_battery_leak_skin_eye

- query：电池漏液弄到手上和眼睛附近，怎么处理？
- 类型：strict
- focus：电池漏液导致皮肤和眼部暴露，能源/污染可进入，但医疗化学暴露 Wiki 必须进入。
- watch conflicts：contamination、energy
- verdict：pass
- expected Guide：DG-0064、DG-0112
- allowed secondary：无
- selected Guide：DG-0112、DG-0841、DG-0064
- selected Wiki：medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001、hygiene-isolation-supplies-001、energy-battery-parallel-series-boundary-001
- profiles：energy_battery_abnormal_isolation、medical_poisoning_chemical_exposure
- domains：power、hygiene、water、medical、records、security、disaster
- high / critical target：DG-0064、DG-0112
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_burn_toothpaste_soy_sauce

- query：烫伤后能不能涂牙膏或者酱油？
- 类型：strict
- focus：烧烫伤应由医疗主导，fire 不应抢主位。
- watch conflicts：fire
- verdict：pass
- expected Guide：DG-0007
- allowed secondary：无
- selected Guide：DG-0560、DG-0007、DG-0017
- selected Wiki：medical-burn-care-002、medical-burn-first-aid-001、medical-trauma-complication-index-001、medical-burn-care-001、medical-heat-activity-stop-001、medical-oral-rehydration-001
- profiles：medical_burn_heat_cold_exposure
- domains：medical、records、water、shelter
- high / critical target：DG-0007
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_heat_slow_response

- query：天气很热，人头晕出汗，开始反应慢，怎么处理？
- 类型：strict
- focus：中暑和热伤害，降温、补液和意识变化停止线。
- watch conflicts：water、shelter
- verdict：pass
- expected Guide：DG-0017
- allowed secondary：无
- selected Guide：DG-0560、DG-0017、DG-0007
- selected Wiki：medical-burn-care-002、medical-burn-first-aid-001、medical-trauma-complication-index-001、medical-heat-activity-stop-001、medical-oral-rehydration-001、medical-dehydration-001
- profiles：medical_burn_heat_cold_exposure
- domains：medical、water、shelter、records
- high / critical target：DG-0017
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_diarrhea_vomit_dehydration

- query：腹泻呕吐很多次，怎么判断是不是脱水？
- 类型：strict
- focus：腹泻呕吐后的脱水判断，water/food 可补充但 medical 应进入 evidence。
- watch conflicts：water、food、hygiene
- verdict：pass
- expected Guide：DG-0020、DG-0559
- allowed secondary：无
- selected Guide：DG-0559、DG-0472、DG-0020
- selected Wiki：medical-dehydration-002、medical-oral-rehydration-001、medical-child-elder-dehydration-risk-001、medical-diarrhea-dehydration-risk-001、medical-dehydration-001、medical-high-fever-consciousness-risk-001
- profiles：medical_infection_dehydration_fever
- domains：medical、hygiene、water、security
- high / critical target：无
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_wound_dirty_water_limited_clean

- query：伤口碰到污水了，已经没有很多干净水，怎么办？
- 类型：strict
- focus：伤口污水接触，卫生可协同但医疗伤口 evidence 应进入。
- watch conflicts：hygiene、water、contamination
- verdict：pass
- expected Guide：DG-0087、DG-0612
- allowed secondary：无
- selected Guide：DG-0087、DG-0559、DG-0558
- selected Wiki：medical-wound-care-001、medical-wound-care-002、medical-clean-water-001、medical-cross-infection-001、hygiene-contamination-zone-001、hygiene-hygiene-knowledge-001
- profiles：medical_infection_dehydration_fever
- domains：security、shelter、medical、tools、hygiene
- high / critical target：DG-0087、DG-0612
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_hypothermia_wet_clothes_fire

- query：人很冷、发抖、衣服湿了，能不能直接靠火烤？
- 类型：strict
- focus：失温应由医疗主导，clothing/fire/shelter 只能作为 secondary。
- watch conflicts：clothing、fire、shelter
- verdict：pass
- expected Guide：DG-0018
- allowed secondary：无
- selected Guide：DG-0852、DG-0018、DG-0560
- selected Wiki：clothing-wet-cold-early-hypothermia-signs-001、clothing-foot-check-after-wet-work-001、clothing-shoe-sole-failure-outing-stop-001、clothing-glove-contamination-cut-boundary-001、clothing-contaminated-laundry-zone-001、fire-hypothermia-002
- profiles：clothing_wet_cold_ppe、medical_burn_heat_cold_exposure
- domains：clothing、hygiene、shelter、medical、water、security
- high / critical target：DG-0018
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_fever_confusion

- query：发烧的人开始意识不清，怎么办？
- 类型：strict
- focus：发热伴意识不清，医疗危险信号和记录。
- watch conflicts：hygiene、water
- verdict：pass
- expected Guide：DG-0021、DG-0558
- allowed secondary：无
- selected Guide：DG-0558、DG-0559、DG-0021
- selected Wiki：medical-fever-004、medical-fever-observation-record-001、medical-high-fever-consciousness-risk-001、medical-dehydration-002、medical-oral-rehydration-001、medical-child-elder-dehydration-risk-001
- profiles：medical_infection_dehydration_fever
- domains：medical、shelter、comms、records、water
- high / critical target：DG-0558
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_observe_fever_cough_isolation

- query：咳嗽发热的人要不要和大家分开？
- 类型：observation
- focus：观察 hygiene/WASH 是否完全抢主位，medical 是否仍进入 evidence。
- watch conflicts：hygiene
- verdict：pass
- expected Guide：DG-0021、DG-0558
- allowed secondary：无
- selected Guide：DG-0021、DG-0473、DG-0558
- selected Wiki：medical-child-elder-dehydration-risk-001、medical-common-infectious-disease-index-001、medical-high-fever-consciousness-risk-001、medical-fever-004、medical-fever-observation-record-001、medical-oral-rehydration-001
- profiles：medical_infection_dehydration_fever
- domains：shelter、medical、comms、records、water
- high / critical target：DG-0558
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_observe_vomit_caregiver_infection

- query：呕吐物怎么处理，照护者怎么避免感染？
- 类型：observation
- focus：hygiene 可以主导，但 medical/dehydration evidence 应补充。
- watch conflicts：hygiene、contamination
- verdict：partial
- expected Guide：DG-0559、DG-0615
- allowed secondary：无
- selected Guide：DG-0615、DG-0559、DG-0558
- selected Wiki：hygiene-fecal-contamination-water-separation-001、medical-bleeding-control-001、medical-clean-water-001、medical-cross-infection-002、medical-wound-care-001、medical-dehydration-002
- profiles：medical_infection_dehydration_fever
- domains：medical
- high / critical target：DG-0615
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：selector 问题、合理 partial
- failure reasons：未命中预期 Wiki

### medical_observe_chemical_eye_clothing

- query：化学味很重，眼睛刺痛，衣服也沾到了。
- 类型：observation
- focus：contamination 可以主导，但 medical skin/eye exposure 应进入。
- watch conflicts：contamination
- verdict：pass
- expected Guide：DG-0064
- allowed secondary：DG-0112
- selected Guide：DG-0064、DG-0112、DG-0012
- selected Wiki：medical-chemical-skin-eye-exposure-001、hygiene-contamination-zone-001、hygiene-contaminated-clothing-001、hygiene-hygiene-knowledge-001、hygiene-contamination-log-001、hygiene-isolation-supplies-001
- profiles：medical_poisoning_chemical_exposure
- domains：disaster、hygiene、water、medical、power
- high / critical target：DG-0064、DG-0112
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_observe_burn_near_fire

- query：烧伤是在火源旁边发生的，是先灭火还是先处理伤？
- 类型：observation
- focus：fire 与 medical 的优先顺序是否合理。
- watch conflicts：fire
- verdict：pass
- expected Guide：DG-0007
- allowed secondary：DG-0488、DG-0848
- selected Guide：DG-0560、DG-0017、DG-0007
- selected Wiki：medical-burn-care-002、medical-burn-first-aid-001、medical-trauma-complication-index-001、medical-heat-activity-stop-001、medical-oral-rehydration-001、medical-dehydration-001
- profiles：medical_burn_heat_cold_exposure
- domains：medical、water、shelter、records
- high / critical target：DG-0007、DG-0848
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### medical_observe_hypothermia_no_dry_clothes

- query：低体温但也没有干衣服，火也不稳定。
- 类型：observation
- focus：clothing / shelter / fire / medical 是否协同。
- watch conflicts：clothing、shelter、fire
- verdict：partial
- expected Guide：DG-0018
- allowed secondary：无
- selected Guide：DG-0560、DG-0017、DG-0018
- selected Wiki：medical-burn-care-002、medical-burn-first-aid-001、medical-trauma-complication-index-001、medical-heat-activity-stop-001、medical-oral-rehydration-001、medical-dehydration-001
- profiles：medical_burn_heat_cold_exposure
- domains：medical、water、shelter、security
- high / critical target：DG-0018
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：selector 问题、合理 partial
- failure reasons：未命中预期 Wiki

### medical_observe_cpr_background

- query：我只想查一下 CPR 是什么。
- 类型：observation
- focus：Kiwix 可以背景解释，但不能替代本地 DG-0015。
- watch conflicts：kiwix
- verdict：pass
- expected Guide：DG-0015
- allowed secondary：无
- selected Guide：DG-0015、DG-0014、DG-0016
- selected Wiki：medical-cpr-no-normal-breathing-001、medical-recovery-position-breathing-observation-001、medical-seizure-protection-timing-001、medical-choking-airway-obstruction-001
- profiles：medical_airway_breathing_emergency
- domains：medical、hygiene、water、security、records
- high / critical target：DG-0015
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

## 5. Cross Domain 统计

- hygiene 冲突观察：0
- contamination 冲突观察：0
- water 冲突观察：0
- food 冲突观察：0
- fire 冲突观察：0
- shelter 冲突观察：0
- clothing 冲突观察：0
- energy 冲突观察：0
- kiwix 冲突观察：0

未发现 hygiene / contamination / water / food / fire / shelter / clothing 完全抢主位。

## 6. 重点风险分析

- hygiene 抢主位：0 个 case 触发 cross-domain 记录。
- contamination 抢主位：0 个 case 触发 cross-domain 记录。
- water / food 抢主位：0 个 case 触发 cross-domain 记录。
- fire 抢主位：0 个 case 触发 cross-domain 记录。
- shelter / clothing 抢主位：0 个 case 触发 cross-domain 记录。
- Kiwix 越权风险：0 个 case 触发 cross-domain 记录。
- Kiwix 越权：本脚本只测试本地 Guide/Wiki selected evidence，未发现 Kiwix 替代本地 Guide/Wiki 的路径。

## 7. Root Cause 初步分类

- 数据缺口：1
- selector 问题：3
- 合理 partial：2

## 8. 是否建议进入 Batch7-D Root Cause Review

建议进入 Batch7-D Medical Retrieval Root Cause Review：本批只记录 evidence 表现，不直接修复。Review 应判断问题属于 Guide/Wiki 证据链缺口、profile 缺口、selector/ranking 问题、跨域竞争或合理 observation。

## 9. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_medical_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_medical_field.py --no-answer
```

本轮已确认：脚本编译通过，Wiki audit 为 0/0/0，Guide audit 为 0/0/0，既有 retrieval traceability/root contract 测试为 9 passed，Medical Field Test 已生成 JSON 和 Markdown 报告。

## 10. 边界声明

- 本批没有修改 Wiki 正文、Guide 正文、Guide-Wiki 关系、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。
- 本批没有修复 partial/fail，也没有新增 medical query profile。
- 本脚本只调用本地 Guide/Wiki fetchers，不调用 LLM；Kiwix 越权按未进入本地证据选择路径记录为 0。
