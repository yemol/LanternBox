# Batch7-A Medical Evidence Link & Retrieval Coverage Review

生成日期：2026-07-16

本阶段只做医疗急救领域 Guide / Wiki / Evidence 覆盖审计与规划判断。未修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase 或 tests。

## 1. 当前覆盖统计

项目实际目录：

- Guide：`data/guides/medical`、`data/guides/hygiene`、`data/guides/contamination` 存在；未发现 `data/guides/health`。
- Wiki：`wiki_import/medical`、`wiki_import/hygiene`、`wiki_import/contamination` 存在；未发现 `wiki_import/health`。

统计口径：

- 核心医疗：`data/guides/medical` 与 `wiki_import/medical`。
- 三目录医疗卫生污染：`medical + hygiene + contamination`。
- 广义医疗相关：目录、domain 或 category 命中 `medical / first_aid / hygiene / contamination / 医疗 / 急救 / 卫生 / 污染 / 隔离`。

|范围|Guide|有 related_wiki|无 related_wiki|Guide-Wiki 边|high/critical Guide|high/critical 缺链|
|---|---:|---:|---:|---:|---:|---:|
|核心 medical 目录|75|18|57|101|21|12|
|hygiene 目录|66|15|51|90|3|1|
|contamination 目录|5|5|0|66|2|0|
|三目录合计|146|38|108|257|26|13|
|广义医疗相关|248|64|184|318|38|19|

|范围|Wiki|有 guide_links|无 guide_links|high/critical Wiki|
|---|---:|---:|---:|---:|
|medical 目录|42|42|0|27 high|
|hygiene 目录|42|42|0|27 high|
|contamination 目录|21|21|0|21 high|
|三目录合计|105|105|0|75 high|
|广义医疗相关|106|106|0|75 high|

核心判断：

- 医疗 Wiki 并不缺基础库存：核心 medical 42 篇，且全部有 `guide_links`。
- 真正断点在 Guide 侧：核心 medical 75 个 Guide 中 57 个没有 `related_wiki`。
- high / critical 医疗场景尤其危险：广义医疗相关 high/critical Guide 38 个，其中 19 个缺 `related_wiki`。
- 高风险 Guide 的 `stop_or_escalate` 和 `fallback` 字段结构上都存在；问题不是字段为空，而是缺少确定性 Wiki evidence 和主题精准性。

## 2. High / Critical Guide 缺证据链清单

### 2.1 核心 Medical 缺 related_wiki

|Guide|title|risk_level|问题|建议|
|---|---|---|---|---|
|DG-0002|伤者初筛：意识、呼吸、大出血|critical|P0 初筛入口无 related_wiki|优先链接出血、呼吸、记录类现有 Wiki；若无精准呼吸/初筛 Wiki，再规划少量 P0 Wiki。|
|DG-0004|严重出血：加压包扎与休克观察|critical|大出血/休克入口无 related_wiki|优先链接 `medical-bleeding-control-*`；检查是否需要休克观察 Wiki。|
|DG-0007|烧烫伤：冷却、覆盖、后续观察|high|烧烫伤老 Guide 无 related_wiki|可链接 `medical-burn-care-*`、`medical-burn-first-aid-001`。|
|DG-0009|疑似骨折：固定不复位|high|骨折固定入口无 related_wiki|现有 Wiki 偏扭伤，可能需要新增骨折固定/循环检查 Wiki。|
|DG-0010|头部受伤：24 小时观察|high|头部受伤观察入口无 related_wiki|现有 Wiki 不足，可能需要新增头伤观察停止线 Wiki。|
|DG-0011|疑似脊柱伤：少动、固定、轴线搬运|high|脊柱伤搬运入口无 related_wiki|现有 Wiki 不足，可能需要新增脊柱搬运停止线 Wiki。|
|DG-0013|噎住：背部拍击与腹部冲击|critical|窒息/异物入口无 related_wiki|现有 Wiki 不足，建议新增窒息异物 P0 Wiki 后链接。|
|DG-0014|昏迷但有呼吸：恢复体位|critical|意识/呼吸入口无 related_wiki|现有 Wiki 不足，建议新增恢复体位/呼吸观察 Wiki。|
|DG-0015|无反应无正常呼吸：胸外按压|critical|CPR 入口无 related_wiki|现有 Wiki 不足，建议新增本地化 CPR 边界 Wiki。|
|DG-0016|癫痫发作：保护头部与计时|critical|癫痫发作入口无 related_wiki|现有 Wiki 不足，建议新增癫痫保护与计时 Wiki。|
|DG-0017|中暑：转移、降温、补液|critical|中暑入口无 related_wiki|可部分链接 `medical-heat-activity-stop-001`、`medical-oral-rehydration-001`；仍可能需要中暑急救 Wiki。|
|DG-0215|误服药初步处理|high|误服药入口无 related_wiki|现有药品记录/储存 Wiki 不足，建议新增误服药/不明药停止线 Wiki。|

### 2.2 相邻医疗卫生污染缺 related_wiki

|Guide|title|risk_level|目录|问题|建议|
|---|---|---|---|---|---|
|DG-0059|火场返回前判断|high|disaster|火场返回含 medical / water / shelter，但无 evidence|若作为医疗吸入/烧伤风险入口，需链接烟尘、烧伤、撤离判断 Wiki。|
|DG-0064|疑似化学污染：远离、脱外层、冲洗|critical|disaster|化学暴露高风险入口无 evidence|优先链接污染控制/化学污染 Wiki；若无皮肤眼暴露 Wiki，需新增少量 P0。|
|DG-0369|烟尘进入室内处理|high|disaster|烟尘吸入/室内污染入口无 evidence|链接 hygiene-mold/air/smoke 或规划烟尘吸入观察 Wiki。|
|DG-0087|伤口避开污水：防水覆盖和事后清洗|high|hygiene|伤口污染高风险入口无 evidence|可链接 `medical-wound-care-*`、`medical-clean-water-*`、hygiene 污水分区 Wiki。|
|DG-0112|电池漏液处理|high|power|化学接触 medical / hygiene 风险无 evidence|链接污染控制、化学暴露、眼/皮肤冲洗相关 Wiki；若无精准 Wiki 需新增。|
|DG-0208|危险区域标记|high|power|medical / water / security 多域但无 evidence|更像 general safety，不应作为 Batch7-B 医疗首批主目标。|
|DG-0170|动物尸体处理|high|security|动物尸体污染 medical / hygiene 风险无 evidence|可链接 `hygiene-animal-carcass-001`、污染隔离 Wiki。|

## 3. 医疗主题分组覆盖

|主题|优先级|Guide 覆盖|Wiki 覆盖|证据链状态|判断|
|---|---|---|---|---|---|
|大出血 / 止血|P0|DG-0002、DG-0003、DG-0004、DG-0556|`medical-bleeding-control-001/002`|新 Guide DG-0556 有链；老 critical DG-0002/DG-0004 无链|Yellow：先修链接。|
|烧伤 / 烫伤|P0|DG-0007、DG-0026、DG-0560|`medical-burn-care-*`、`medical-burn-first-aid-001`|DG-0560 有链，DG-0007/DG-0026 无链|Yellow：先修链接。|
|骨折 / 扭伤 / 固定|P0|DG-0008、DG-0009、DG-0025、DG-0614|`medical-sprain-*`|扭伤有 Wiki，骨折/循环检查/不复位 Wiki 不足|Yellow：需要少量 Wiki + 链接。|
|中暑 / 低体温|P0|DG-0017、DG-0018、DG-0505、DG-0506、DG-0507、DG-0508|`medical-heat-activity-stop-001`、`medical-cold-injury-frostbite-001`、`fire-hypothermia-002`|失温/冻伤有部分链，中暑 critical DG-0017 无链|Yellow：中暑需补链或补 Wiki。|
|脱水 / 腹泻 / 呕吐|P0|DG-0020、DG-0023、DG-0472、DG-0559、DG-0615|`medical-dehydration-*`、`medical-oral-rehydration-001`、hygiene vomit Wiki|DG-0559/DG-0615 有链，老 DG-0020 和隔离流程无链|Yellow：先修链接。|
|发热 / 感染迹象|P0|DG-0021、DG-0024、DG-0473、DG-0558、DG-0611|`medical-fever-*`、`medical-high-fever-consciousness-risk-001`|新发热 Guide 链强，老记录/咳嗽照护链弱|Yellow：链接可修。|
|创口清洁 / 感染预警|P0|DG-0005、DG-0006、DG-0022、DG-0612、DG-0087|`medical-wound-*`、`medical-clean-water-*`、`medical-cross-infection-*`|新 DG-0612 链强但过宽；老小伤口/观察表/污水伤口无链|Yellow：链接优先，需收窄过宽证据。|
|呼吸困难 / 窒息 / 异物|P0|DG-0013、DG-0014、DG-0015|无明确核心 medical Wiki|三个 critical Guide 全无 related_wiki|Red/Yellow：需要少量 P0 Wiki。|
|过敏 / 休克风险|P0|DG-0004、DG-0211 等间接覆盖|药品/过敏记录弱|缺明确过敏/休克行动证据|Red/Yellow：需规划 P0/P1 Wiki。|
|中毒 / 不明药物 / 化学暴露|P0|DG-0064、DG-0112、DG-0215|污染控制 Wiki 有一部分；误服药 Wiki 不足|三个高风险入口无 related_wiki|Yellow：化学可先链，误服药可能需新增 Wiki。|
|长期护理|P1|DG-0103、DG-0104、DG-0106、DG-0393|慢病、医疗记录、药品记录 Wiki 有基础|老 Guide 多无链|Yellow：Batch7-B 后续。|
|病人隔离|P1|DG-0471-0481、DG-0854|hygiene-patient、shared items、WASH 记录 Wiki 强|Batch5-M 已补 DG-0854；旧 medical 隔离 Guide 多无链|Yellow：先不扩内容，补链。|
|药品记录|P1|DG-0209-0216|`medical-medication-*`、`medical-medication-storage-check-001`|DG-0210 有链，其他药品 Guide 多无链|Yellow：补链，误服药需 Wiki。|
|疼痛与休息|P1|DG-0028、DG-0101 等|`medical-pain-001` 有但关联偏新 Guide|旧疼痛判断无链|Yellow：低于创伤优先级。|
|儿童 / 老人 / 脆弱成员照护|P1|DG-0095-0106、DG-0611|儿童发热、慢病、疼痛、脱水 Wiki 有基础|旧 Guide 多无链，新 DG-0611 链过宽|Yellow：后续收窄。|
|心理恶化观察|P1|DG-0106、psychology 目录 Guide|psychology Wiki 不在医疗目录|可能被 psychology 主导|Yellow：Field Test 后再定。|
|医疗记录 / 症状日志|P2|DG-0022、DG-0024、DG-0480、DG-0499|`medical-medical-record-001`、fever record Wiki|记录 Wiki 有基础，老 Guide 无链|Yellow：补链。|
|药品库存 / 慢病管理|P2|DG-0097、DG-0103、DG-0209-0216|`medical-chronic-disease-001`、`medical-medication-*`|药品状态有链，库存和访问规则弱|Yellow：后续。|

## 4. Guide-Wiki 问题表

|Guide|title|risk_level|current_related_wiki|问题|建议|
|---|---|---|---|---|---|
|DG-0002|伤者初筛：意识、呼吸、大出血|critical|空|高风险入口无 Wiki evidence；可能只靠 Guide 本体|Batch7-B 首批补链；若缺初筛 Wiki，规划新增。|
|DG-0004|严重出血：加压包扎与休克观察|critical|空|大出血 critical 无 evidence|链接 `medical-bleeding-control-*`；检查是否需休克 Wiki。|
|DG-0007|烧烫伤：冷却、覆盖、后续观察|high|空|已有烧伤 Wiki 但老 Guide 未链接|链接 `medical-burn-care-*`、`medical-burn-first-aid-001`。|
|DG-0009|疑似骨折：固定不复位|high|空|骨折无精准 Wiki evidence|可能需要新增骨折固定/不复位 Wiki。|
|DG-0010|头部受伤：24 小时观察|high|空|头伤观察无 evidence|规划新增头伤观察停止线 Wiki。|
|DG-0011|疑似脊柱伤：少动、固定、轴线搬运|high|空|脊柱伤无 evidence|规划新增脊柱搬运边界 Wiki。|
|DG-0013|噎住：背部拍击与腹部冲击|critical|空|窒息 critical 无 evidence|规划新增窒息/异物 Wiki。|
|DG-0014|昏迷但有呼吸：恢复体位|critical|空|意识/呼吸 critical 无 evidence|规划新增恢复体位 Wiki。|
|DG-0015|无反应无正常呼吸：胸外按压|critical|空|CPR critical 无 evidence|规划新增胸外按压边界 Wiki。|
|DG-0016|癫痫发作：保护头部与计时|critical|空|癫痫 critical 无 evidence|规划新增癫痫发作保护与记录 Wiki。|
|DG-0017|中暑：转移、降温、补液|critical|空|中暑 critical 无 evidence|链接现有热环境/补液 Wiki；不足时新增中暑 Wiki。|
|DG-0215|误服药初步处理|high|空|误服药无 evidence|规划新增误服药/不明药停止线 Wiki。|
|DG-0064|疑似化学污染：远离、脱外层、冲洗|critical|空|化学暴露 critical 无 evidence|链接 contamination/hygiene Wiki；必要时新增皮肤眼暴露 Wiki。|
|DG-0087|伤口避开污水：防水覆盖和事后清洗|high|空|伤口污染 high 无 evidence|链接 wound / clean water / sewage Wiki。|
|DG-0112|电池漏液处理|high|空|化学接触 high 无 evidence|链接污染隔离和皮肤/眼暴露 Wiki；必要时新增。|
|DG-0018|失温：保温、干燥、缓慢复温|critical|`fire-hypothermia-002`|只有 fire 领域 Wiki，医疗证据偏薄|补充 clothing/medical 低体温 Wiki 或确认现有 Wiki 足够精准。|
|DG-0611|儿童发热咳嗽观察卡|caution|23 个 Wiki|证据链过宽，混入出血、烧伤、鼻出血、扭伤等不精准 Wiki|Batch7-B 不先改；后续收窄 related_wiki。|
|DG-0612|小伤口少水清洁卡|high|24 个 Wiki|证据链过宽，混入慢病、烧伤、脱水、鼻出血等主题|保留核心 wound/clean-water/cross-infection，后续收窄。|
|DG-0613|鼻出血不仰头处理|caution|22 个 Wiki|证据链过宽，混入烧伤、慢病、扭伤等|后续收窄到 nosebleed/bleeding/cross infection。|
|DG-0615|照护者处理呕吐物后防交叉感染|high|5 个 Wiki|含出血/伤口 Wiki，主题略散|补强 vomit/fecal/hygiene 证据，减少伤口类干扰。|

## 5. Retrieval 风险预测

|场景|可能抢主位领域|风险|建议观察点|
|---|---|---|---|
|发热、咳嗽、儿童发热|hygiene / WASH / general medical|被泛卫生隔离覆盖，缺体温、尿量、精神状态、停止线|Field Test 检查 DG-0558 / DG-0611 与 fever Wiki 是否进入。|
|腹泻、呕吐、脱水|water / food / hygiene|水配给或食物中毒抢主位，脱水补液和隔离证据不稳|检查 DG-0559 / DG-0023 / DG-0472。|
|烧伤、烫伤|fire / cooking|火源安全抢主位，烧伤冷却和覆盖 evidence 缺失|检查 DG-0560 / DG-0007。|
|中毒、误服药、化学暴露|contamination / power / food|污染隔离或电池安全主导，医疗观察与不催吐边界缺失|检查 DG-0064 / DG-0112 / DG-0215。|
|创口感染、伤口污水接触|hygiene / contamination|清洁区污染区主导，伤口红肿热痛、渗液、红线停止线弱|检查 DG-0006 / DG-0087 / DG-0612。|
|低体温、冻伤|clothing / shelter / fire|保温/火源主导，医疗复温和禁揉搓边界被稀释|检查 DG-0018 / DG-0019 / DG-0505。|
|窒息、昏迷、无呼吸|general safety / Kiwix background|本地 critical Guide 无 related_wiki，Kiwix 背景可能越权|必须先补本地 evidence，再做 Field Test。|

Kiwix 风险判断：

- 当前本地医疗 critical Guide 存在，但多个无 related_wiki。
- 如果 Retrieval 没选中正确 Guide 或没有加载 related Wiki，百科背景可能在医学名词解释上显得“更丰富”，这不符合 Guide > related Wiki > independent Wiki > Kiwix 的行动优先级。
- 因此 Batch7-B 不应先扩 prompt 或 Kiwix policy，应先修本地 Guide-Wiki 链。

## 6. 是否需要新增 Wiki

结论：选择 **D：混合小批次，但必须拆阶段**。第一阶段以链接修复为主，少量新增 P0 Wiki 只用于现有 Wiki 明显不存在的 critical 场景。

判断：

|判断项|结论|
|---|---|
|A. 现有 Wiki 足够，只需补 Guide-Wiki 关联|部分成立。出血、烧伤、发热、脱水、伤口感染、药品记录、病人隔离已有 Wiki，可先补链。|
|B. 现有 Wiki 不足，需要少量新增医疗 Wiki|成立。窒息/异物、恢复体位、CPR、癫痫、头伤、脊柱伤、骨折不复位、误服药、化学皮肤/眼暴露需要少量 P0 Wiki。|
|C. Guide 本身过旧，需要后续重写|部分成立。DG-0002-DG-0017 结构字段完整，但证据链缺失；先补链和 Field Test，再决定是否重写。|
|D. Retrieval profile 缺失，需要 Field Test 后再定|成立。医疗跨 hygiene/water/fire/contamination/clothing 的冲突很多，应在证据链修复后做 Field Test，再决定 profile。|

不建议现在大规模扩内容：

- medical Wiki 已有 42 篇，且全部有 guide_links。
- 盲目新增会进一步增加旧 Guide 与新 Wiki 的链路债。
- 先补 high/critical Guide 的 evidence chain 更符合 ROOT_CAUSE_FIX_POLICY。

## 7. Batch7-B 推荐方案

推荐：**方案 D，拆阶段混合小批次**。

### Batch7-B1：Medical Guide-Wiki Link Repair

范围：

- 不新增 Guide。
- 尽量不新增 Wiki。
- 先给 high / critical Guide 补 related_wiki。
- 首批目标：DG-0002、DG-0004、DG-0007、DG-0009、DG-0010、DG-0011、DG-0013、DG-0014、DG-0015、DG-0016、DG-0017、DG-0215、DG-0064、DG-0087、DG-0112。

优先链到已有 Wiki：

- 出血：`medical-bleeding-control-001/002`
- 烧伤：`medical-burn-care-*`、`medical-burn-first-aid-001`
- 发热/意识：`medical-fever-*`、`medical-high-fever-consciousness-risk-001`
- 脱水：`medical-dehydration-*`、`medical-oral-rehydration-001`
- 伤口感染：`medical-wound-*`、`medical-clean-water-*`
- 污染隔离：`hygiene-*` / `contamination-*` 现有 Wiki

### Batch7-B2：只补确实缺失的 P0 Wiki

候选新增少量 Wiki：

- `medical-choking-airway-obstruction-001`
- `medical-recovery-position-breathing-observation-001`
- `medical-cpr-no-normal-breathing-001`
- `medical-seizure-protection-timing-001`
- `medical-head-injury-24h-observation-001`
- `medical-spine-injury-movement-boundary-001`
- `medical-fracture-immobilize-no-reposition-001`
- `medical-accidental-medication-ingestion-001`
- `medical-chemical-skin-eye-exposure-001`

### Batch7-C：Medical Retrieval Field Test

Field Test 应覆盖：

- 大出血 / 止血
- 烧烫伤
- 骨折 / 脊柱 / 头伤
- 噎住 / 昏迷 / 无呼吸
- 中暑 / 低体温 / 冻伤
- 腹泻脱水 / 发热
- 伤口感染 / 污水接触
- 误服药 / 化学暴露
- 病人隔离 / 照护者防护

验收重点：

- high / critical Guide 必须进入 selected Guide。
- related Wiki 必须进入 evidence。
- Kiwix 不得越权。
- safety boundary / fallback / record-check 100%。

## 8. 不建议修改项

本阶段和 Batch7-B 初期都不建议：

- 不改 Retrieval Pipeline。
- 不改 Prompt。
- 不新增或修改 query profile，等 Field Test 后再定。
- 不改 top_k。
- 不改 selector limit。
- 不改 ranking。
- 不改 fallback。
- 不改 schema。
- 不同步 PocketBase，除非后续 Apply 阶段确实新增 Wiki。
- 不做医疗治疗替代，不写药物剂量指南，不写侵入性操作教程。
- 不让 Kiwix 或百科背景替代本地 Guide/Wiki 行动建议。
