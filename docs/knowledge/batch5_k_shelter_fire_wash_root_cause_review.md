# Batch5-K Shelter / Fire / WASH Retrieval Root Cause Review

生成日期：2026-07-15

## 1. 范围

本阶段只分析 Batch5-J Shelter / Fire / Clothing / WASH Retrieval Field Test 的 partial / fail 根因，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema、测试 fixture、测试脚本或任何生产数据。

参考文件：

- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_report.md`
- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_results.json`
- `docs/knowledge/batch5_i_shelter_fire_wash_apply_report.md`
- `docs/knowledge/batch5_i_shelter_fire_wash_plan.md`
- `docs/knowledge/knowledge_coverage_map_v0_2.md`

本报告没有运行 audit。只做只读检查：读取 Batch5-J results、Guide JSON、Wiki Markdown、PocketBase `wiki_articles` / `wiki_categories`，并通过现有 fetcher 做候选可达性观察。

## 2. Batch5-J 汇总

|指标|结果|
|---|---:|
|用例总数|16|
|strict / observation|14 / 2|
|pass / partial / fail|7 / 6 / 3|
|Guide 命中率（strict，含 allowed secondary）|50.0%|
|主 Guide 命中率（strict，仅 expected）|50.0%|
|Wiki 命中率（strict）|50.0%|
|Guide-Wiki 精准组合率（strict）|50.0%|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|跨域竞争|3|

核心判断：安全边界、fallback、记录复查并没有掉；未达 stable 的主因是正确入口没有稳定进 selected Guide，导致 Batch5-I 新 Wiki 失去 related_wiki 进入 evidence 的机会。

Batch5-J 脚本初步 root cause 中出现的“数据缺口”，在本 Review 中需校正：重点检查的 Batch5-I Wiki 均存在于 Markdown 和 PocketBase，content 可读，slug 在 related_wiki normalize 后保留。因此大多数不是数据不存在，而是 profile 缺口、selector/ranking 竞争或 Guide-Wiki evidence priority。

## 3. Partial / Fail Case 复盘

|case|verdict|当前 selected Guide|当前 selected Wiki|预期 / allowed Guide|是否命中 allowed secondary|Review root cause|是否合理 partial|是否建议进入 Apply|最小 Apply 方式|
|---|---|---|---|---|---|---|---|---|---|
|`shelter_ventilation_keep_warm`|fail|DG-0621、DG-0569、DG-0580|repair-bag、repair-door-window 等维修 Wiki|expected DG-0848；allowed DG-0850|否|profile 缺口 + Shelter/Repair 跨域竞争|否|是|新增 `shelter_ventilation_heat_balance` profile，触发“门窗堵死 + 保暖/通风/冷/睡眠/火源”时偏向 shelter/fire，而不是 repair。|
|`fire_smoke_backdraft_room`|fail|DG-0226、DG-0220、DG-0579|food-cooking-fuel-management、safety-fire-response|expected DG-0850；allowed DG-0849|否；DG-0850 在 top8 第 6|selector/ranking 问题 + Fire/Food/Water 跨域竞争|否|是|新增 `fire_smoke_combustion_stop` profile；必要时增强 DG-0850 smoke/backdraft anchor，不压低 cooking Guide。|
|`fire_hot_ash_trash_bag`|partial observation|DG-0488、DG-0627、DG-0223|旧 hand hygiene / hygiene knowledge|expected DG-0849；allowed DG-0850|否|Guide 设计缺口 + profile 缺口；本批暂缓 DG-0851|是|暂缓主修|不进入 Batch5-L 主修；可在第二批评估 DG-0851 灰烬与余火处理。|
|`clothing_wet_socks_outing`|partial|DG-0316、DG-0058、DG-0830|food-post-flood、agriculture-wood-ash|expected DG-0852|否|profile 缺口；旧湿袜 Guide 抢局部主题，但未承接外出停止线|否|是|新增 `clothing_wet_cold_ppe` profile，覆盖湿袜、鞋里有水、继续走、外出、鞋底、脚部检查。|
|`clothing_shivering_keep_working`|partial|DG-0018、DG-0541、DG-0544|fire-hypothermia、safety-low-profile|expected DG-0852；allowed DG-0848|否|profile 缺口；medical/safety 旧 Guide 合理但缺衣物/工作停工链|否|是|同一 `clothing_wet_cold_ppe` profile，覆盖发抖、冷、继续干活、停工、湿冷。|
|`clothing_gloves_contaminated`|fail|DG-0661、DG-0662、DG-0478|hygiene-blood / cleaning / contaminated surface|expected DG-0852|否|profile 缺口 + Clothing/Hygiene/Medical 跨域竞争|否|是|`clothing_wet_cold_ppe` profile 加入手套破损、PPE、污染衣物、清理污染物；保持 DG-0852 主导，hygiene/medical 作补充。|
|`wash_water_toilet_kitchen_layout`|partial|DG-0626、DG-0568、DG-0604|旧 hand hygiene / handwashing knowledge|expected DG-0853|否；DG-0853 在 top8 第 5|selector/ranking 问题 + WASH profile 缺口 + evidence priority|否|是|新增 `wash_zone_layout_priority` profile；给 DG-0853 补/前置分区和可见标记相关 Wiki。|
|`wash_bucket_toilet_full`|partial|DG-0626、DG-0571、DG-0651|旧 hand hygiene / handwashing knowledge|expected DG-0853|否；DG-0853 在 top8 第 6|selector/ranking 问题 + WASH evidence priority|否|是|`wash_zone_layout_priority` profile 覆盖桶厕 + 更换/封存/异味；给 DG-0853 补异常记录相关 Wiki。|
|`wash_patient_cup_kitchen`|partial observation|DG-0083、DG-0820、DG-0477|water priority Wiki|expected DG-0853；allowed DG-0083|是|Guide 设计缺口 / Guide-Wiki evidence priority；本批暂缓 DG-0854|是|暂缓主修|不进入 Batch5-L 主修；第二批评估 DG-0854 或给 DG-0083 / DG-0853 加病人用品与厨房污染 Wiki。|

## 4. 新增 Wiki Evidence 可达性检查

### 4.1 总体结论

重点检查的 Batch5-I Wiki 均满足：

- Markdown 文件存在。
- PocketBase `wiki_articles` 记录存在。
- `content` 可读取。
- Markdown category / risk_level 与 PocketBase category / risk_level 一致。
- 通过 `fetch_related_wiki_candidates` 读取 Guide `related_wiki` 时，slug 保留。

但也观察到：

- 用 slug 字符串直接调用 `fetch_wiki_candidates(query=slug)` 未返回对应 Wiki。当前 Wiki fetcher 更像自然语言候选召回，不是 slug lookup；这不等于数据缺失。
- partial/fail cases 中，大多数 `fetch_wiki_candidates` 自然查询没有召回新 Wiki；新 Wiki 主要依赖命中 Guide 后由 `related_wiki` 确定性进入 evidence。
- 因此如果正确 Guide 未进 selected top3，Batch5-I 新 Wiki 基本不会进入 selected evidence。

### 4.2 全 35 篇 Wiki 检查表

全量只读检查结果：

- Markdown 存在：35/35。
- PocketBase 存在：35/35。
- content 可读取：35/35。
- metadata 正确：35/35。
- normalize 后 related_wiki 保留 slug：35/35。
- 直接 slug 查询进入 `fetch_wiki_candidates` top8：0/35。
- title 自然查询进入 `fetch_wiki_candidates` top8：34/35。
- 只能依赖 Guide related_wiki 稳定进入 selected evidence：是，尤其是 field case 中正确 Guide 未进入 selected top3 时。

|slug|数据状态|guide_links|slug 直查|title 查询|related slug 保留|判断|
|---|---|---|---|---|---|---|
|`shelter-site-selection-weather-exposure-001`|MD/PB/content/metadata 正常|DG-0847|否|是|是|正常，已在 pass case 进入 evidence。|
|`shelter-rain-leak-first-line-001`|正常|DG-0847|否|是|是|正常，已在 pass case 进入 evidence。|
|`shelter-ground-moisture-barrier-001`|正常|DG-0847, DG-0848|否|是|是|正常。|
|`shelter-sleep-heat-loss-ground-001`|正常|DG-0848|否|是|是|正常。|
|`shelter-ventilation-heat-balance-001`|正常|DG-0848, DG-0850|否|是|是|正常；`shelter_ventilation_keep_warm` 中入口被 repair 抢走。|
|`shelter-kitchen-fire-sleep-distance-001`|正常|DG-0849|否|是|是|正常。|
|`shelter-daily-habitability-check-001`|正常|DG-0847|否|是|是|正常。|
|`shelter-roof-wall-floor-seepage-signs-001`|正常|DG-0847|否|是|是|正常。|
|`fire-before-lighting-site-check-001`|正常|DG-0849|否|是|是|正常，已在 fire pass case 进入 evidence。|
|`fire-dry-wet-fuel-sorting-001`|正常|DG-0849|否|是|是|正常。|
|`fire-indoor-combustion-no-go-zone-001`|正常|DG-0850|否|是|是|正常。|
|`fire-carbon-monoxide-suspect-stop-001`|正常|DG-0850|否|是|是|正常。|
|`fire-smoke-backdraft-room-response-001`|正常|DG-0850|否|是|是|正常；DG-0850 在 smoke case top8 第 6，但未进 selected top3。|
|`fire-ash-ember-cooling-disposal-001`|正常|无|否|是|是|数据存在但未接入新 Guide；观察 case 合理 partial。|
|`fire-temporary-stove-stability-boundary-001`|正常|DG-0849|否|是|是|正常。|
|`fire-children-bystander-clear-zone-001`|正常|DG-0849|否|是|是|正常。|
|`fire-night-final-extinguish-log-001`|正常|无|否|是|是|数据存在但未接入新 Guide；观察 case 合理 partial。|
|`clothing-wet-cold-early-hypothermia-signs-001`|正常|DG-0848, DG-0852|否|是|是|正常；DG-0852 未被湿冷/发抖 case 触发。|
|`clothing-foot-check-after-wet-work-001`|正常|DG-0852|否|是|是|正常。|
|`clothing-shoe-sole-failure-outing-stop-001`|正常|DG-0852|否|是|是|正常。|
|`clothing-layering-work-rest-adjustment-001`|正常|DG-0848|否|是|是|正常。|
|`clothing-glove-contamination-cut-boundary-001`|正常|DG-0852|否|是|是|正常；被 hygiene/medical 主导压住。|
|`clothing-eye-protection-low-resource-001`|正常|无|否|是|是|本次 field case 未覆盖，数据正常。|
|`clothing-mouth-nose-dust-smoke-limit-001`|正常|无|否|是|是|本次 field case 未覆盖，数据正常。|
|`clothing-contaminated-laundry-zone-001`|正常|DG-0852|否|是|是|正常；被 hygiene/medical 主导压住。|
|`hygiene-wash-zone-layout-minimum-001`|正常|DG-0853|否|是|是|正常；DG-0853 在 layout case top8 第 5。|
|`hygiene-handwater-priority-table-001`|正常|DG-0853|否|是|是|正常；handwater case 已 pass。|
|`hygiene-bucket-toilet-changeover-001`|正常|DG-0853|否|是|是|正常；DG-0853 在 bucket case top8 第 6。|
|`hygiene-patient-cup-towel-isolation-001`|正常|无|否|是|是|数据存在但未接入 DG-0853 / DG-0083；观察 case 合理 partial。|
|`hygiene-kitchen-raw-cooked-contamination-line-001`|正常|无|否|是|是|数据存在但未接入 DG-0853 / DG-0083；观察 case 合理 partial。|
|`hygiene-contamination-zone-visible-marking-001`|正常|无|否|是|是|数据正常；建议少量关联 DG-0853。|
|`hygiene-daily-wash-round-checklist-001`|正常|DG-0853|否|是|是|正常。|
|`hygiene-wash-abnormal-record-001`|正常|无|否|是|是|数据存在但未接入 DG-0853；影响 bucket / patient observation。|
|`hygiene-food-water-toilet-distance-review-001`|正常|DG-0853|否|是|是|正常；自然 Wiki top8 可召回，但 selected evidence 仍依赖 Guide。|
|`hygiene-simple-team-wash-handover-001`|正常|DG-0853|否|否|是|title 查询也未进 top8；但可通过 DG-0853 related_wiki 进入 evidence，且本次非关键缺口。|

### 4.3 数据缺口判定

本 Review 不确认“生产数据缺口”。更准确的分类是：

- true data missing：0。
- Guide 未命中导致 related_wiki 不加载：主要问题。
- Wiki 存在但没有 Guide link / related_wiki 承接：`fire-ash-ember-cooling-disposal-001`、`fire-night-final-extinguish-log-001`、`hygiene-patient-cup-towel-isolation-001`、`hygiene-kitchen-raw-cooked-contamination-line-001`、`hygiene-wash-abnormal-record-001`。
- Wiki 自然召回弱：普遍存在；不能用扩大 top_k 掩盖，应优先 profile / evidence priority。

## 5. Guide 归属与重叠分析

|Guide|当前 related_wiki 顺序|判断|
|---|---|---|
|DG-0847 临时住所选址与防雨防潮|site selection、rain leak、ground barrier、seepage、daily check|顺序合理。通过 shelter 睡哪里/漏雨类问题已稳定命中。|
|DG-0848 睡眠区保温和地面隔离|sleep heat loss、ground barrier、ventilation heat balance、hypothermia signs、layering|顺序基本合理；`shelter_ventilation_keep_warm` 的问题不是顺序，而是 DG-0848 没进候选 top8。|
|DG-0849 火源使用前检查|before lighting、fuel sorting、temporary stove、children clear zone、kitchen/fire/sleep distance|顺序合理。点火前和炉具不稳已 pass。灰烬余火不是 DG-0849 的自然主职责。|
|DG-0850 室内燃烧和一氧化碳停止线|indoor no-go、CO stop、smoke backdraft、ventilation balance、existing CO wiki|顺序合理。`fire_smoke_backdraft_room` 中 DG-0850 已进 top8 第 6，属于 selector/ranking/profile 问题。|
|DG-0852 湿冷衣物和脚部保护|hypothermia signs、foot check、shoe sole、glove boundary、contaminated laundry|顺序合理。湿袜、发抖、手套污染均未触发 DG-0852；主因是 profile 缺口。|
|DG-0853 小团队 WASH 分区运行|zone layout、handwater、bucket toilet、daily round、food-water-toilet distance、team handover|顺序基本合理，但 layout/bucket cases 被旧 Guide 压住。可考虑把 `hygiene-contamination-zone-visible-marking-001`、`hygiene-wash-abnormal-record-001` 加入或前置，提升 WASH 运行证据完整度。|

逐项回答：

1. 每个 fail/partial 是否已有合适 Guide 承接：除 ash 和 patient-cup-kitchen 两个 observation 外，其余均已有合适 Guide 承接。
2. 是否需要调整 related_wiki 顺序：DG-0848 / DG-0850 / DG-0852 暂不需要；DG-0853 可做小幅 evidence priority 补强。
3. 是否需要新增少量 Guide-Wiki 关联：是，优先 DG-0853 关联 `hygiene-contamination-zone-visible-marking-001`、`hygiene-wash-abnormal-record-001`。观察项如进入第二批，可再处理 ash / patient-kitchen。
4. 是否需要新增 DG-0851 / DG-0854：不建议作为 Batch5-L 主修；两者是合理 partial 的后续批次候选。
5. 是否需要新增 query profile：是，最多 4 个，且是本批最小 Apply 的主修方向。

## 6. 跨域竞争分析

### 6.1 Shelter / Repair

case：`shelter_ventilation_keep_warm`

现象：“门窗都堵死保暖”被 DG-0621 / DG-0569 / DG-0580 等门窗维修/加固 Guide 抢主位。

判断：

- 这是 profile 缺口，不是 repair Guide 必然错误。旧 repair Guide 对“门窗”词敏感是合理的。
- 但 query 的关键语义不是修门窗，而是“封闭通风 + 保暖 + 夜间睡眠 + 可能火源/多人空间”的风险边界。
- 应新增 `shelter_ventilation_heat_balance` profile，让“门窗堵死、保暖、通风、睡觉、很冷、闷、火源”等组合触发 shelter/fire 目标域。
- 不建议删除、压低或改写维修 Guide。

### 6.2 Fire / Food / Water

case：`fire_smoke_backdraft_room`

现象：“做饭 / 烟 / 开窗 / 灭火”被烹饪燃料、初起火灾、水/安全旧证据抢主位；DG-0850 在 guide top8 第 6，但没有进入 selected top3。

判断：

- 这是 selector/ranking 问题叠加 profile 缺口。
- Food cooking Guide 不应降权，因为“做饭燃料”语义本身成立；问题是“烟倒灌 + 开窗还是灭火”应由 Fire/CO/通风停止线主导。
- 建议新增 `fire_smoke_combustion_stop` profile，触发“烟倒灌、烟往屋里、开窗、灭火、做饭烟、通风、头晕”等组合，目标域 fire/shelter/medical。
- DG-0850 已含 `fire-smoke-backdraft-room-response-001`，顺序第三，足够进入 related evidence；重点是让 DG-0850 进 selected top3。

### 6.3 Clothing / Hygiene / Medical

case：`clothing_gloves_contaminated`

现象：“污染物 / 手套破了”被血液污染、呕吐物、照护者自我保护抢主位。

判断：

- 这是 profile 缺口，不是 hygiene / medical Guide 错误。
- query 的动作入口是 PPE 破损后的继续/停止边界，DG-0852 应主导；hygiene/medical 应作为污染类型补充。
- 建议 `clothing_wet_cold_ppe` 同时覆盖湿冷、脚部、鞋袜、手套破损、PPE、污染衣物，目标域 clothing/shelter/hygiene/medical。
- 不建议把 `clothing-glove-contamination-cut-boundary-001` 关联到 DG-0853 作为主路；否则会让 WASH 抢走 PPE 行动入口。

### 6.4 WASH / Water / Old Hygiene

cases：`wash_water_toilet_kitchen_layout`、`wash_bucket_toilet_full`、`wash_patient_cup_kitchen`

判断：

- DG-0853 在 layout case top8 第 5、bucket case top8 第 6，说明不是数据缺失，而是 selector/ranking/profile 不足。
- 旧 bucket toilet Guide 作为 secondary 是合理的，但它会把 selected wiki 带回旧 handwashing / hygiene knowledge，挤掉 WASH 运行类新 Wiki。
- `wash_zone_layout_priority` profile 应覆盖“水桶 + 做饭/厨房 + 桶厕 + 分区”、“洗手水 + 降级”、“桶厕 + 满/异味/封存/更换”、“厨房 + 病人用品 + 污染”等组合。
- `wash_patient_cup_kitchen` 已命中 allowed secondary DG-0083，属于合理 partial。若第二批要强化，可新增 DG-0854 或给 DG-0083 / DG-0853 关联 patient/kitchen 新 Wiki。

## 7. 合理 Partial 清单

|case|原因|建议|
|---|---|---|
|`fire_hot_ash_trash_bag`|observation；Batch5-I 暂缓 DG-0851，且现有 DG-0488 / DG-0223 能给出安全边界，但不能把 Batch5-I ash Wiki 带入 evidence。|不作为 Batch5-L 主修。第二批再判断是否新增 DG-0851 灰烬与余火处理。|
|`wash_patient_cup_kitchen`|observation；Batch5-I 暂缓 DG-0854，当前命中 allowed secondary DG-0083，但 selected Wiki 偏水资源优先级，没有带入 patient/kitchen 污染新 Wiki。|不作为 Batch5-L 主修。第二批再判断 DG-0854 或少量关联。|

## 8. Batch5-L 最小 Apply 建议

### A. 新增 query profile，最多 4 个

1. `shelter_ventilation_heat_balance`
   - 触发：门窗堵死、保暖、通风、很冷、睡觉、闷、火源、多人。
   - 目标：shelter / fire / medical；slug domain shelter/fire。
   - 目的：避免“门窗”单词直接推向 repair。

2. `fire_smoke_combustion_stop`
   - 触发：烟倒灌、烟往屋里、做饭烟大、开窗还是灭火、通风、停火、一氧化碳。
   - 目标：fire / shelter / medical。
   - 目的：让 DG-0850 从 top8 进入 selected top3，不压低 cooking Guide。

3. `clothing_wet_cold_ppe`
   - 触发：湿袜、鞋里有水、继续走、发抖、冷得发抖、继续干活、手套破、PPE、污染衣物。
   - 目标：clothing / shelter / hygiene / medical。
   - 目的：DG-0852 主导湿冷与 PPE 停止线，medical/hygiene 作补充。

4. `wash_zone_layout_priority`
   - 触发：水桶、桶厕、厨房、做饭、厕所、分区、洗手水不够、封存、异味、病人用品。
   - 目标：hygiene / water / food / shelter / records。
   - 目的：让 DG-0853 主导 WASH 运行表，不被旧 handwashing / bucket toilet / water container 完全占满。

### B. related_wiki 顺序调整

- DG-0848：暂不建议调整。`shelter-ventilation-heat-balance-001` 已在第 3，若 Guide 被选中即可进入 selected evidence。
- DG-0850：暂不建议调整。`fire-smoke-backdraft-room-response-001` 已在第 3，问题是 Guide 未进 selected top3。
- DG-0852：暂不建议调整。PPE / wet-cold Wiki 已在第 1 至第 5。
- DG-0853：建议小幅补强 WASH 运行 evidence priority。可以考虑将 `hygiene-contamination-zone-visible-marking-001`、`hygiene-wash-abnormal-record-001` 加入 related_wiki，并把运行/记录类 Wiki 保持在前部。

### C. 少量 Guide-Wiki 关联

优先建议：

- DG-0853 + `hygiene-contamination-zone-visible-marking-001`
- DG-0853 + `hygiene-wash-abnormal-record-001`

可选但不作为主修：

- DG-0083 或未来 DG-0854 + `hygiene-patient-cup-towel-isolation-001`
- DG-0083 / DG-0084 或未来 DG-0854 + `hygiene-kitchen-raw-cooked-contamination-line-001`
- 未来 DG-0851 + `fire-ash-ember-cooling-disposal-001`
- 未来 DG-0851 + `fire-night-final-extinguish-log-001`

### D. 是否新增 Guide

- DG-0851 灰烬与余火处理：不建议作为 Batch5-L 主修。当前 case 是 observation，旧 Guide 有安全边界；可留到第二批，以避免为单个观察题新增 Guide。
- DG-0854 病人用品与厨房污染隔离：不建议作为 Batch5-L 主修。当前命中 allowed secondary DG-0083；先通过 WASH profile 和少量关联观察是否足够，再决定是否新增。

### E. 不进入 Apply 的合理 partial

- `fire_hot_ash_trash_bag`
- `wash_patient_cup_kitchen`

这两项应在 Batch5-L 后继续观察，不作为最小 Apply 的通过条件核心。

## 9. 不建议修改内容

- 不扩大 top_k。
- 不扩大 selector limit。
- 不修改 Prompt。
- 不修改 Retrieval Pipeline。
- 不新增大量 Guide。
- 不新增大量 Wiki。
- 不压低 repair / food / medical / hygiene 旧 Guide。
- 不为单个测试题硬编码 Guide ID 或 Wiki slug。
- 不把 Kiwix 作为行动建议补丁。
- 不删除旧 Guide 的有效语义入口。
- 不通过扩大 evidence 数量掩盖正确 Guide 未被选中的问题。

## 10. 是否建议进入 Apply

建议进入 Batch5-L Minimal Apply。

建议范围：

1. 新增 4 个 query profile：
   - `shelter_ventilation_heat_balance`
   - `fire_smoke_combustion_stop`
   - `clothing_wet_cold_ppe`
   - `wash_zone_layout_priority`
2. 少量补强 DG-0853 related_wiki：
   - `hygiene-contamination-zone-visible-marking-001`
   - `hygiene-wash-abnormal-record-001`
3. 不新增大量 Wiki。
4. 不新增大量 Guide。
5. 不修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。
6. Apply 后必须重新运行 Batch5-J Field Test，并重点观察：
   - `shelter_ventilation_keep_warm`
   - `fire_smoke_backdraft_room`
   - `clothing_wet_socks_outing`
   - `clothing_shivering_keep_working`
   - `clothing_gloves_contaminated`
   - `wash_water_toilet_kitchen_layout`
   - `wash_bucket_toilet_full`

稳定目标不应要求两个 observation case 必然 pass；它们应作为第二批 Guide 设计观察项。
