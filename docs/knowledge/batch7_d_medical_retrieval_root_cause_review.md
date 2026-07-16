# Batch7-D Medical Retrieval Root Cause Review

## 1. Field Test 总结

参考 `docs/knowledge/batch7_c_medical_field_test_results.json`：

| 指标 | 结果 |
|---|---:|
| total | 22 |
| strict / observation | 16 / 6 |
| pass / partial / fail | 7 / 10 / 5 |
| Guide 命中率 | 43.8% |
| Wiki 命中率 | 50.0% |
| Guide-Wiki 精准组合率 | 50.0% |
| high / critical Guide 命中率 | 40.0% |
| safety / fallback / record-check | 100% / 100% / 100% |
| dangerous suggestion | 0 |
| Kiwix 越权 | 0 |
| cross domain | 7 |

总体判断：

1. fail 不是由安全字段缺失造成。所有 case 的 safety / fallback / record-check 均为 100%。
2. fail 不是由 Kiwix 越权造成。当前 Kiwix 越权为 0。
3. fail 大多不是新增 Wiki 本身缺失。Batch7-B2 的 9 篇 Wiki 均已存在并完成双向关系。
4. 主要根因是 selected evidence 入口不稳定：正确 Guide 没进 selected top3 后，related_wiki 无法被确定性加载。
5. medical query profile 缺失明显，尤其是癫痫、头伤、脊柱伤、误服药、热伤害、化学皮肤/眼暴露和 CPR 背景查询。
6. 相邻领域抢主位明显：repair / safety、disaster / shelter / energy、fire / planting / food、energy 等在部分医疗高危场景中抢到 top1。
7. Guide-Wiki 缺链不是主因，但有局部顺序和关联问题：DG-0064 / DG-0112 的 `medical-chemical-skin-eye-exposure-001` 位置靠后；DG-0020 仍缺可支撑脱水判断的 related_wiki。

## 2. 5 个 Fail 根因

| case | query | expected Guide / Wiki | actual selected Guide | actual selected Wiki | high / critical 命中 | 新补 P0 Wiki 命中 | cross domain | root cause | 最小修复建议 |
|---|---|---|---|---|---|---|---|---|---|
| medical_seizure_do_not_hold | 有人癫痫发作，旁边人想按住他，可以吗？ | DG-0016 / medical-seizure-protection-timing-001 | DG-0839, DG-0533, DG-0635 | repair/safety Wiki | 否 | 否 | off_domain_primary | A, C, E | 新增 medical_airway_breathing_emergency profile，覆盖癫痫/抽搐/按住/塞嘴；避免 repair_site_clearance_boundary 抢主位。 |
| medical_head_hit_vomit_sleepy | 头撞了一下，当时清醒，后面开始想吐和犯困，怎么办？ | DG-0010 / medical-head-injury-24h-observation-001 | DG-0057, DG-0392, DG-0239 | 无 | 否 | 否 | contamination / shelter / energy | A, C, E | 新增 medical_trauma_bleeding_fracture profile，覆盖头撞、呕吐、犯困、意识变化；不要改 Wiki。 |
| medical_fall_neck_pain_numb | 摔下来后脖子痛，手脚有点麻，能不能扶起来走？ | DG-0011 / medical-spine-injury-movement-boundary-001 | DG-0851, DG-0830, DG-0040 | fire / agriculture Wiki | 否 | 否 | shelter / fire / food | A, C, E | medical_trauma_bleeding_fracture profile 加入坠落、脖子痛、手脚麻、扶起来走、少动。 |
| medical_battery_leak_skin_eye | 电池漏液弄到手上和眼睛附近，怎么处理？ | DG-0112 或 DG-0064 / medical-chemical-skin-eye-exposure-001 | DG-0841, DG-0168, DG-0546 | energy Wiki | DG-0112 仅在 candidate 第 4，未 selected | 否 | energy | A, C, D, E | 新增 medical_poisoning_chemical_exposure profile；把 DG-0112 / DG-0064 中 `medical-chemical-skin-eye-exposure-001` 前置。 |
| medical_heat_slow_response | 天气很热，人头晕出汗，开始反应慢，怎么处理？ | DG-0017 / medical-heat-activity-stop-001, medical-oral-rehydration-001 | DG-0179, DG-0015, DG-0187 | medical-cpr-no-normal-breathing-001 | DG-0017 仅在 candidate 第 6，未 selected | 否 | shelter / weather-adjacent | A, C, E | 新增 medical_burn_heat_cold_exposure profile，覆盖高温、头晕出汗、反应慢、中暑、补液。 |

根因分类说明：

- A. medical profile 缺失：5/5 fail 均需要。
- B. correct Guide exists but not selected：5/5 fail 均成立；其中 DG-0112 和 DG-0017 已在 candidate 但未进 top3。
- C. correct Wiki exists but not selected：5/5 fail 均成立；原因多为正确 Guide 未进 top3，related_wiki 无法加载。
- D. Guide-Wiki related_wiki 顺序问题：主要见化学皮肤/眼暴露。
- E. 相邻领域抢主位：5/5 fail 均成立。
- G. 确实缺 Wiki / Guide：不成立；本批 fail 的目标 Wiki/Guide 均存在。

## 3. 10 个 Partial 分类

| case | 问题类型 | actual | expected | root cause | 建议 |
|---|---|---|---|---|---|
| medical_major_bleeding_soaked_cloth | 正确 Wiki 已进入，但 high/critical Guide 不稳定 | DG-0556, DG-0613, DG-0003；Wiki 命中 bleeding 001/002 | DG-0004 / bleeding 001/002 | B, C 局部，selector priority | medical_trauma_bleeding_fracture profile；可考虑让 DG-0004 在严重出血、浸透敷料语义下优先于普通止血卡。 |
| medical_child_unknown_medicine_no_vomit | high Guide 没进 selected top3 | DG-0021, DG-0071, DG-0075 | DG-0215 / medical-accidental-medication-ingestion-001 | A, C | medical_poisoning_chemical_exposure profile；覆盖误服药、不明药、催吐、儿童误服。 |
| medical_diarrhea_vomit_dehydration | 正确 Guide 已进入，但 Wiki evidence 不足 | DG-0472, DG-0559, DG-0020；Wiki 为 child-elder / diarrhea risk | DG-0020 或 DG-0559；Wiki 期望 dehydration / ORS | D / Guide-Wiki 关联不足 | 不新增 Wiki；给 DG-0020 补 existing related_wiki：medical-dehydration-001、medical-dehydration-002、medical-oral-rehydration-001；DG-0559 可前置 dehydration/ORS 证据。 |
| medical_hypothermia_wet_clothes_fire | 相邻领域合理补充但 medical 失温 Guide 不稳定 | DG-0852, DG-0556, DG-0317；clothing Wiki 主导 | DG-0018 / fire-hypothermia-002 | A, B, E | medical_burn_heat_cold_exposure profile；fixture 可允许 DG-0852 secondary，但 DG-0018 应进入 selected。 |
| medical_fever_confusion | 正确 Wiki 已进入，但 fever Guide 不稳定 | DG-0002, DG-0027, DG-0106；medical-high-fever-consciousness-risk-001 命中 | DG-0021 或 DG-0558 | B, F | Fixture 预期可扩展：DG-0002 / DG-0027 是合理急性入口；同时 medical_infection_dehydration_fever profile 可提升 DG-0021/DG-0558。 |
| medical_observe_vomit_caregiver_infection | observation 合理 partial；Wiki 预期过窄 | DG-0615 命中；actual medical-cross-infection-002 而非 expected -001 | DG-0615 / medical-cross-infection-001, medical-diarrhea-dehydration-risk-001 | F, H | 不建议修检索；fixture 可允许 medical-cross-infection-002。 |
| medical_observe_chemical_eye_clothing | 正确 Guide 已进入，但 related_wiki 被前置污染/水证据挤出 selected top6 | DG-0665, DG-0466, DG-0064；medical-chemical 在 related 但未 selected | DG-0064 / medical-chemical-skin-eye-exposure-001 | D, E, H | DG-0064 中将 medical-chemical-skin-eye-exposure-001 前置；observation 不应强制 contamination 完全让位。 |
| medical_observe_burn_near_fire | observation 合理 partial | DG-0026 + fire Guide | DG-0007 / burn Wiki | A, H | medical_burn_heat_cold_exposure profile 可改善；fixture 可允许 DG-0026 和 fire Guide secondary。 |
| medical_observe_hypothermia_no_dry_clothes | observation 但 cross-domain 明显 | food / organization / fever evidence | DG-0018 / fire-hypothermia-002 | A, E, H | medical_burn_heat_cold_exposure profile；不新增 Wiki。 |
| medical_observe_cpr_background | fixture/query 缩写触发不足 | agriculture / psychology evidence | DG-0015 / medical-cpr-no-normal-breathing-001 | A, F, H | medical_airway_breathing_emergency profile 加入 CPR、心肺复苏、胸外按压；本 observation 不作为紧急行动失败修复。 |

## 4. High / Critical Guide 审查

| Guide | title | Field Test 表现 | 是否召回但未选 | 是否被相邻领域抢位 | related_wiki 是否进入 evidence | 判断 |
|---|---|---|---|---|---|---|
| DG-0002 | 伤者初筛：意识、呼吸、大出血 | 多个呼吸/意识 case selected；大出血 case candidate 第 4 | 是 | 否 | bleeding Wiki 多次进入 | 可用，但严重出血入口不够强。 |
| DG-0004 | 严重出血：加压包扎与休克观察 | 大出血 case candidate 第 5，未 selected | 是 | 否 | bleeding Wiki 进入 | selector/profile 问题，不是缺链。 |
| DG-0007 | 烧烫伤：冷却、覆盖、后续观察 | 烫伤偏方 strict selected；火源旁烧伤 observation 未 selected | 局部 | fire 可抢 | burn Wiki 局部进入 | 需要 burn/heat/cold profile；Wiki 链可用。 |
| DG-0009 | 疑似骨折：固定不复位 | strict pass | 否 | 否 | 新补 fracture Wiki 进入 | 稳定。 |
| DG-0010 | 头部受伤：24 小时观察 | fail，未进 candidate top8 | 否 | disaster/shelter/energy 抢位 | 新补 head injury Wiki 未进入 | 需要 trauma profile。 |
| DG-0011 | 疑似脊柱伤：少动、固定、轴线搬运 | fail，未进 candidate top8 | 否 | fire/shelter/food 抢位 | 新补 spine Wiki 未进入 | 需要 trauma profile。 |
| DG-0013 | 噎住：背部拍击与腹部冲击 | strict pass | 否 | 否 | 新补 choking Wiki 进入 | 稳定。 |
| DG-0014 | 昏迷但有呼吸：恢复体位 | strict pass；CPR case secondary | 否 | 否 | 新补 recovery Wiki 进入 | 稳定。 |
| DG-0015 | 无反应无正常呼吸：胸外按压 | emergency strict pass；CPR 背景 observation fail | 局部 | 非医学背景查询被 agriculture/psychology 抢 | 新补 CPR Wiki 进入 emergency case | 加 CPR acronym profile alias。 |
| DG-0016 | 癫痫发作：保护头部与计时 | fail，未进 candidate top8 | 否 | repair/safety 抢位 | 新补 seizure Wiki 未进入 | 需要 airway/breathing 或 neuro-emergency 触发。 |
| DG-0017 | 中暑：转移、降温、补液 | heat case candidate 第 6，未 selected | 是 | shelter/weather/clothing 邻域 | heat/ORS Wiki 未进入 | 需要 burn/heat/cold exposure profile。 |
| DG-0018 | 失温：保温、干燥、缓慢复温 | strict candidate 第 4；observation 未进 | 是 | clothing/food/shelter/fire | fire-hypothermia-002 未进入 | 需要 cold exposure profile；fixture 可允许 clothing secondary。 |
| DG-0021 | 发热：降温、补水、记录 | fever isolation pass；fever confusion candidate 第 5；误服药误入 | 是 | 否 | fever consciousness Wiki 多次进入 | fixture 需扩展 allowed；profile 可改善。 |
| DG-0064 | 疑似化学污染：远离、脱外层、冲洗 | chemical observation selected 第 3；battery strict 未 selected | 局部 | contamination/energy 抢位 | medical chemical Wiki 在 related 但未 selected | 需要 profile + related_wiki 前置。 |
| DG-0087 | 伤口避开污水：防水覆盖和事后清洗 | wound sewage strict pass | 否 | water/hygiene 协同 | wound/clean water Wiki 进入 | 稳定。 |
| DG-0112 | 电池漏液处理 | battery strict candidate 第 4，未 selected | 是 | energy 抢位 | medical chemical Wiki 未进入 | 需要 poisoning/chemical profile + related_wiki 前置。 |
| DG-0215 | 误服药初步处理 | strict partial，未进 candidate top8 | 否 | 发热/evacuation 语义干扰 | 新补 medication Wiki 未进入 | 需要 poisoning/chemical profile。 |

## 5. 新补 9 篇 Wiki Evidence 审查

| Wiki | expected Guide | 是否进入 evidence | 未进入原因 | 建议 |
|---|---|---|---|---|
| medical-choking-airway-obstruction-001 | DG-0013 | 是，medical_choking_cannot_speak | Guide selected 后正常加载 | 保持。 |
| medical-recovery-position-breathing-observation-001 | DG-0014 | 是，recovery 和 CPR strict | Guide selected 后正常加载 | 保持。 |
| medical-cpr-no-normal-breathing-001 | DG-0015 | 是，CPR emergency；误入 heat case | DG-0015 被 heat case 错选时带入 | 保持；profile 避免 heat 错进 CPR。 |
| medical-seizure-protection-timing-001 | DG-0016 | 否 | DG-0016 未进 candidate top8 | medical_airway_breathing_emergency profile。 |
| medical-head-injury-24h-observation-001 | DG-0010 | 否 | DG-0010 未进 candidate top8 | medical_trauma_bleeding_fracture profile。 |
| medical-spine-injury-movement-boundary-001 | DG-0011 | 否 | DG-0011 未进 candidate top8 | medical_trauma_bleeding_fracture profile。 |
| medical-fracture-immobilize-no-reposition-001 | DG-0009 | 是，fracture strict | Guide selected 后正常加载 | 保持。 |
| medical-accidental-medication-ingestion-001 | DG-0215 | 否 | DG-0215 未进 candidate top8 | medical_poisoning_chemical_exposure profile。 |
| medical-chemical-skin-eye-exposure-001 | DG-0064, DG-0112 | 否；chemical observation 中在 `guide_related_wiki`，但未进 selected top6 | DG-0064 进入 top3 但 related_wiki 顺序靠后；battery case DG-0112 未 selected | 前置到 DG-0064 / DG-0112 related_wiki 第一位，并新增 poisoning/chemical profile。 |

## 6. Cross Domain 根因

Field Test 记录 cross domain=7：

- off_domain_primary：7
- medical_vs_contamination：2
- medical_vs_shelter：3
- medical_vs_energy：2

按领域判断：

| 领域 | 现象 | 边界建议 |
|---|---|---|
| hygiene | 发热咳嗽、呕吐污染、污水伤口可以合理协同；本轮 hygiene 未完全抢掉 strict 医疗主位 | 若 query 是病人症状、意识、脱水、伤口感染，medical 应优先；若 query 是区域清洁、物品分区，hygiene 可优先。 |
| contamination | 化学味、污染衣物可主导 observation；但眼痛、皮肤接触、电池漏液到眼周时 medical/chemical exposure 必须进入 evidence | “皮肤/眼睛/呼吸刺激/症状” medical 优先；“容器隔离/区域标记” contamination 优先；两者应协同。 |
| water / food | 腹泻、呕吐、脱水可有 water/food 补充，但不能压过 medical dehydration | “尿少、意识、无法饮水、脱水判断” medical 优先；“水源是否污染/食物是否可吃” water/food 优先。 |
| fire | 烧伤发生在火源旁时 fire 可先处理现场危险，但烧伤创面处理必须进入 medical evidence | “现场仍着火/烟/CO” fire 优先；“皮肤烧烫伤、涂牙膏、冷却覆盖” medical 优先。 |
| clothing | 湿冷衣物和脚部保护可协同低体温，但失温症状本身需要 medical | “湿袜/鞋袜/干衣不足” clothing 可优先；“发抖、迟钝、低体温、停止发抖” medical 应进入。 |
| shelter | 避风、转移、保温空间是低体温/中暑辅助 evidence；不应抢走头伤/脊柱伤 | “居住点/避风/通风/空间” shelter 优先；“伤病症状” medical 优先。 |
| Kiwix | 本轮 Kiwix 越权为 0 | 不需要 Kiwix 修复；继续保持 Kiwix 背景补充，不替代本地 Guide/Wiki。 |
| general safety / repair | 癫痫 case 被刀锯清场/站位边界抢主位，属于 profile 误触发 | repair profile 应避免“旁边人/按住”这类非工具场景误触发。 |

## 7. 是否需要 Profile

| profile | 是否必要 | triggers | 可能误伤 | Batch7-E 是否实现 |
|---|---|---|---|---|
| medical_airway_breathing_emergency | 必要 | 噎住、不能说话、咳不出、无反应、无正常呼吸、CPR、心肺复苏、胸外按压、恢复体位、癫痫、抽搐、按住、塞嘴 | 可能误伤心理安抚、普通咳嗽、通信缩写 CPR 类无关文本 | 是。覆盖 CPR background 和 seizure fail，但避免普通发热咳嗽过度提升。 |
| medical_trauma_bleeding_fracture | 必要 | 大量出血、血浸透、敷料浸透、头撞、呕吐、犯困、坠落、脖子痛、手脚麻、骨折、复位、掰正、扶起来走 | 可能误伤 shelter evacuation、fire ash、repair injury prevention | 是。解决出血 selector、头伤、脊柱伤、骨折入口。 |
| medical_burn_heat_cold_exposure | 必要 | 烧伤、烫伤、牙膏、酱油、高温、头晕出汗、反应慢、中暑、低体温、失温、发抖、湿衣、直接靠火烤 | 可能误伤 fire heating、clothing PPE、shelter insulation | 是。目标是协同，不要完全压低 fire/clothing。 |
| medical_poisoning_chemical_exposure | 必要 | 误服药、不明药、催吐、儿童误服、药盒、化学味、眼睛刺痛、皮肤接触、电池漏液、漏液弄到手上/眼睛 | 可能误伤 energy battery safety、contamination container isolation | 是。要让 medical chemical exposure 进入 selected evidence，energy/contamination 可 secondary。 |
| medical_infection_dehydration_fever | 必要但优先级略低 | 发热、意识不清、腹泻、呕吐、脱水、尿少、不能饮水、伤口污水、红肿热痛 | 可能误伤 hygiene/WASH、water/food | 建议实现，但权重要保守；重点提升 symptom/record/dehydration，不覆盖 WASH 分区问题。 |

## 8. 是否需要 Guide-Wiki 顺序调整

| Guide | related_wiki 判断 | 是否过宽/顺序问题 | 建议 |
|---|---|---|---|
| DG-0004 | bleeding 001/002 精准 | 不过宽 | 不调整；靠 profile/selector 让 DG-0004 selected。 |
| DG-0007 | burn care 001/002 + first aid | 精准 | 可保持；若调整，`medical-burn-first-aid-001` 可前置。 |
| DG-0009 | 新补 fracture 单链 | 精准 | 不调整。 |
| DG-0010 | 新补 head injury 单链 | 精准 | 不调整。 |
| DG-0011 | 新补 spine 单链 | 精准 | 不调整。 |
| DG-0013 | 新补 choking 单链 | 精准 | 不调整。 |
| DG-0014 | 新补 recovery 单链 | 精准 | 不调整。 |
| DG-0015 | 新补 CPR 单链 | 精准 | 不调整。 |
| DG-0016 | 新补 seizure 单链 | 精准 | 不调整。 |
| DG-0017 | heat/ORS/dehydration 链接合理 | 不过宽 | 不调整；靠 profile 进入 selected。 |
| DG-0018 | 仅 fire-hypothermia-002 | 可能偏 fire，缺 medical hypothermia Wiki 但本阶段不建议新增 | 暂不新增 Wiki；先 profile。 |
| DG-0021 | fever consciousness Wiki 已在链路 | 顺序可接受 | fixture 可允许 DG-0002/DG-0027；profile 保守提升。 |
| DG-0064 | medical-chemical-skin-eye-exposure-001 在最后 | 顺序问题明显 | Batch7-E 前置 medical chemical Wiki，再接污染分区/衣物/记录。 |
| DG-0087 | medical wound/clean water 前置，hygiene 后置 | 合理 | 不调整。 |
| DG-0112 | medical-chemical-skin-eye-exposure-001 在最后 | 顺序问题明显 | Batch7-E 前置 medical chemical Wiki，再接 contamination zone/log/isolation。 |
| DG-0215 | 新补 medication 单链 | 精准 | 不调整。 |

额外发现：

- DG-0020 `related_wiki` 仍为空，导致腹泻脱水 case 虽命中 DG-0020/DG-0559，但没有进入用户期望的 `medical-dehydration-001` / `medical-oral-rehydration-001`。
- DG-0559 当前 related_wiki 为 `medical-child-elder-dehydration-risk-001`、`medical-diarrhea-dehydration-risk-001`，可以补充或前置 ORS / dehydration 证据。此项不需要新增 Wiki。

## 9. 是否需要新增 Wiki / Guide

不建议 Batch7-E 新增 Wiki 或 Guide。

理由：

- 5 个 fail 的 expected Guide 和 expected Wiki 均已存在。
- 新补 9 篇 Wiki 的未命中原因主要是 Guide 未进入 selected top3，或 related_wiki 顺序靠后。
- safety / fallback / record-check 已 100%，dangerous suggestion 为 0，说明内容安全结构不是当前瓶颈。
- 当前应修入口，不应继续扩内容。

唯一可记录的未来观察：DG-0018 只有 `fire-hypothermia-002`，如果后续低体温 field test 仍弱，可在更小范围新增 medical hypothermia Wiki。但不建议在 Batch7-E 做。

## 10. Batch7-E 最小 Apply 建议

建议选择：C + D + E。

具体范围：

1. 新增少量 medical query profile：
   - medical_airway_breathing_emergency
   - medical_trauma_bleeding_fracture
   - medical_burn_heat_cold_exposure
   - medical_poisoning_chemical_exposure
   - medical_infection_dehydration_fever
2. Guide-Wiki 顺序最小调整：
   - DG-0064：`medical-chemical-skin-eye-exposure-001` 前置。
   - DG-0112：`medical-chemical-skin-eye-exposure-001` 前置。
3. Guide-Wiki 关联补强，不新增 Wiki：
   - DG-0020：补 `medical-dehydration-001`、`medical-dehydration-002`、`medical-oral-rehydration-001`。
   - DG-0559：可补或前置 `medical-dehydration-001`、`medical-oral-rehydration-001`，保留现有 diarrhea/dehydration risk Wiki。
4. 少量修正 fixture expected：
   - `medical_fever_confusion`：允许 DG-0002 / DG-0027 作为 secondary，因为“发烧 + 意识不清”可合理先进入意识初筛或老人意识混乱观察。
   - `medical_observe_vomit_caregiver_infection`：允许 `medical-cross-infection-002`。
   - observation 中 fire/chemical/hypothermia 可保留 partial 观察，不强行追求全 pass。
5. 暂不修复某些 observation：
   - CPR 背景查询可用 profile alias 改善，但不应为了百科解释改 Kiwix 或 Prompt。
   - 火源旁烧伤、低体温无干衣服属于协同场景，允许相邻领域进入 secondary。

不建议：

- 不新增大量 Wiki。
- 不新增 Guide。
- 不扩 top_k。
- 不改 Prompt。
- 不改 selector limit。
- 不让 Kiwix 补行动建议。
- 不硬编码 case。

## 11. 不建议修改项

Batch7-E 不建议修改：

- Wiki 正文。
- Guide steps / check / fallback / stop_or_escalate。
- Retrieval Pipeline 架构。
- Prompt。
- top_k。
- selector limit。
- ranking 架构。
- fallback。
- PocketBase schema。
- Kiwix 权重或 Kiwix 行动建议能力。

Batch7-E 的最小目标应是：让正确 medical Guide 进入 selected top3，并让已存在的 related_wiki 通过确定性链路进入 selected evidence。
