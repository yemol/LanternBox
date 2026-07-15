# Batch5-M Shelter / Fire / WASH Second-Batch Plan

生成日期：2026-07-15

## 1. 范围

本阶段只做 Shelter / Fire / WASH v0.2 小批次规划，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、fallback、PocketBase schema、测试或生产数据。

参考：

- `docs/knowledge/batch5_l_shelter_fire_wash_apply_report.md`
- `docs/knowledge/batch5_k_shelter_fire_wash_root_cause_review.md`
- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_report.md`
- `docs/knowledge/batch5_i_shelter_fire_wash_apply_report.md`

本报告只做只读复盘：读取 Batch5-J/L 结果、现有 Guide JSON 和候选 Wiki Markdown；未运行 audit。

## 2. Batch5-L Stable Candidate 状态

Batch5-L 已将 Shelter / Fire / WASH Retrieval 提升为 v0.1 stable candidate：

|指标|Batch5-L 后|
|---|---:|
|pass / partial / fail|14 / 2 / 0|
|strict cases|全部 pass|
|dangerous suggestion|0|
|Kiwix 越权|0|
|cross domain|0|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|

剩余 2 个 partial 均为 observation：

- `fire_hot_ash_trash_bag`
- `wash_patient_cup_kitchen`

这两个不阻止 v0.1 stable candidate，但可以作为 v0.2 小批次补齐点。

## 3. 两个 Observation Partial 复盘

### 3.1 fire_hot_ash_trash_bag

- query：灰烬看起来灭了但还有点热，可以直接倒进垃圾袋吗？
- 当前 selected Guide：DG-0488 灰烬和焦物处理；DG-0627 垃圾渗漏后的地面清理；DG-0223 灰烬处理。
- 当前 selected Wiki：`hygiene-hand-hygiene-001`、`hygiene-handwashing-001`、`hygiene-handwashing-002`、`hygiene-hygiene-knowledge-002`、`hygiene-hygiene-knowledge-003`、`hygiene-hygiene-knowledge-004`。
- 预期 Guide：DG-0849；allowed secondary：DG-0850。
- 预期 Wiki：`fire-ash-ember-cooling-disposal-001`、`fire-night-final-extinguish-log-001`。
- 当前状态：partial，未命中预期 Guide / Wiki，但 safety、fallback、record/check 已满足。

现有知识判断：

- `fire-ash-ember-cooling-disposal-001` 足够支撑行动入口：包含适用场景、冷却位置、不能倒入塑料袋/垃圾、停止条件、fallback、记录建议。
- `fire-night-final-extinguish-log-001` 需要 Guide 承接：它是夜间/睡前余火复查和交接记录，适合作为灰烬余火 Guide 的 record/check 支撑。
- 旧 DG-0488 / DG-0223 能继续承担安全边界，但不适合长期作为主入口：
  - DG-0488 归属 disaster/hygiene/medical/water，`related_wiki` 为空。
  - DG-0223 归属 water/food/security，`related_wiki` 为空。
  - 两者能说明热灰不能进垃圾袋，但不能把 Batch5-I 的 Fire 新 Wiki 带入 selected evidence。
- 是否需要新增 DG-0851：建议需要，但只作为 v0.2 小批次 Guide 设计，不新增 Wiki，不新增 profile。

### 3.2 wash_patient_cup_kitchen

- query：病人用过的杯子和毛巾不小心放到厨房边上了，现在怎么处理？
- 当前 selected Guide：DG-0083 病人用品隔离：杯子、毛巾、餐具；DG-0820 先给病人和照护人员留更安全的水；DG-0477 病人垃圾处理。
- 当前 selected Wiki：`water-drinking-priority-017`、`water-medical-priority-020`、`water-patient-priority-045`。
- 预期 Guide：DG-0853；allowed secondary：DG-0083。
- 预期 Wiki：`hygiene-patient-cup-towel-isolation-001`、`hygiene-kitchen-raw-cooked-contamination-line-001`、`hygiene-wash-abnormal-record-001`。
- 当前状态：partial，已命中 allowed secondary DG-0083，但未命中预期 Wiki。

现有知识判断：

- `hygiene-patient-cup-towel-isolation-001` 足够支撑病人用品隔离：包含标记、固定放置点、不进入厨房/饮水桶旁、照护手卫生、停止条件和记录建议。
- `hygiene-kitchen-raw-cooked-contamination-line-001` 需要 Guide 承接：它把厨房台面、生熟分区、病人用品、污染表面和食物入口边界串联起来。
- `hygiene-wash-abnormal-record-001` 已足够作为异常追溯支撑，并已在 Batch5-L 关联 DG-0853；但该 observation 当前由 DG-0083 / water priority 证据主导，未把它带入 selected Wiki。
- 旧 DG-0083 能继续承担主入口的一部分：病人杯子、毛巾、餐具隔离本身正确，且是 allowed secondary。
- 旧 DG-0083 不足以独立承担 v0.2：`related_wiki` 为空，不能把 Batch5-I 的 patient/kitchen 新 Wiki 带入 evidence；同时缺少厨房污染线和 WASH 异常记录的组合。
- 是否需要新增 DG-0854：建议需要，但只作为 v0.2 小批次 Guide 设计，不新增 Wiki，不新增 profile。

## 4. 是否需要 DG-0851

建议新增 DG-0851：灰烬与余火处理。

理由：

- 现有 Wiki 已足够，不需要新增 Wiki。
- 旧 Guide 能给安全边界，但域归属和 related_wiki 链路不适合作为 Fire v0.2 主入口。
- 该场景是高风险火源后处理：热灰、余火、复燃、垃圾袋引燃、夜间复查。它不应长期依赖 disaster/water 旧入口。
- 新 Guide 可以把灰烬余火 Wiki、夜间熄灭记录和小火停止线串成 Guide -> Wiki -> Evidence -> Action。

规划：

- id：DG-0851
- title：灰烬与余火处理
- category：火源 / 保温 / 通风 / 一氧化碳风险
- priority：P0
- risk_level：high
- scenario：做饭、取暖、蜡烛、炉具或炭火结束后，仍有灰烬、热炭、余火、热炉具或焦物，需要判断能否移动、封存或丢弃。
- steps：
  1. 先停止加燃料，清空儿童、旁人和可燃物。
  2. 判断是否仍有烟、火星、红热、热气或容器发烫。
  3. 仍热时放在耐热位置或金属容器中冷却，不进塑料袋、纸箱、普通垃圾或睡眠区。
  4. 冷却期间标记禁入，远离衣物、燃料、厨房通道和睡眠区。
  5. 冷却后再封存或转移，并记录处理人和复查时间。
- check：
  - 是否无烟、无火星、无明显热感。
  - 容器或地面是否耐热。
  - 周边是否没有纸、布、塑料、燃料、干草木和垃圾。
  - 夜间是否有复查人和复查时间。
- stop_or_escalate：
  - 灰烬冒烟、颜色发红、容器发热、复燃、风大扬灰、儿童无法隔离时停止搬运或丢弃。
  - 无法确认冷却时按仍有余火处理，不进入睡眠。
- fallback：
  - 没有金属容器时，延长冷却时间，用沙土或耐热地面隔离。
  - 没有表格时，用纸条记录火源名称、地点、处理人、复查时间。
- related_wiki：
  - `fire-ash-ember-cooling-disposal-001`
  - `fire-night-final-extinguish-log-001`
  - `fire-small-fire-stop-001`
  - `fire-fire-response-001`

## 5. 是否需要 DG-0854

建议新增 DG-0854：病人用品与厨房污染隔离。

理由：

- 现有 Wiki 已足够，不需要新增 Wiki。
- 旧 DG-0083 是合理 secondary，但没有 related_wiki，无法带出 Batch5-I 的病人用品 / 厨房污染 / WASH 异常记录证据。
- 当前 observation 的核心不是单纯病人杯子隔离，也不是单纯饮水优先级，而是病人用品误入厨房边界后的停用、隔离、清洁、记录和恢复使用判断。
- DG-0854 可以把 DG-0083 的病人用品隔离与 DG-0084 的厨房分区之间缺失的 WASH action bridge 补上。

规划：

- id：DG-0854
- title：病人用品与厨房污染隔离
- category：污染控制 / 隔离 / 清洁分区
- priority：P0
- risk_level：high
- scenario：病人杯子、毛巾、餐具、清洁用品或垃圾接近厨房、饮水桶、公共餐具或食物处理面，需要判断停用、隔离、清洁和记录。
- steps：
  1. 先停止使用受影响的厨房台面、餐具、水桶周边或公共用品堆。
  2. 标记病人用品和接触位置，避免继续移动造成污染扩散。
  3. 将病人用品移回固定病人用品区，不进入公共厨房和饮水桶旁。
  4. 判断食物、餐具、台面和毛巾是否直接接触；接触入口用品时按隔离、弃用或重新清洁判断。
  5. 照护者完成手卫生后，再恢复公共食物和饮水处理。
  6. 记录接触物、位置、处理人、停用范围和复查时间。
- check：
  - 病人用品是否专人专用、标记清楚、位置固定。
  - 厨房台面、餐具、饮水桶取水口是否被接触。
  - 是否完成照护前后手卫生。
  - 是否记录污染事件和恢复使用时间。
- stop_or_escalate：
  - 病人用品接触熟食、公共餐具、饮水桶取水口、厨房台面或多人出现腹泻呕吐时停止共用相关区域。
  - 无法确认接触范围时，先扩大隔离，不继续处理入口食物。
- fallback：
  - 容器不足时，用固定角落和明显标记隔离病人用品。
  - 没有足够清洁台面时，换位置或按时间分区处理，并中间清洁。
- related_wiki：
  - `hygiene-patient-cup-towel-isolation-001`
  - `hygiene-kitchen-raw-cooked-contamination-line-001`
  - `hygiene-wash-abnormal-record-001`
  - `hygiene-shared-items-001`
  - `hygiene-contaminated-surface-001`

## 6. 是否需要新增 Wiki

建议不新增 Wiki。

只读检查结论：

- `fire-ash-ember-cooling-disposal-001`：足够支撑灰烬、余火、热灰、垃圾袋禁用、冷却、复查和记录。
- `fire-night-final-extinguish-log-001`：足够支撑睡前/夜间熄灭确认、复查人和交接记录。
- `hygiene-patient-cup-towel-isolation-001`：足够支撑病人杯子、毛巾、餐具和固定放置点隔离。
- `hygiene-kitchen-raw-cooked-contamination-line-001`：足够支撑厨房生熟、污染表面、病人用品和入口食物停止线。
- `hygiene-wash-abnormal-record-001`：足够支撑异常记录、追溯、复查和交接。

现有 Wiki 的缺口不是内容不足，而是缺 Guide 承接和双向关联。

## 7. 是否需要新增 Profile

建议不新增 query profile。

理由：

- Batch5-L 已新增 4 个 profile，strict cases 全部 pass。
- 当前两个 remaining cases 均为 observation，不应继续扩大 profile。
- `fire_hot_ash_trash_bag` 的问题是缺 Fire v0.2 Guide 承接，而非 profile 必须修复。
- `wash_patient_cup_kitchen` 已命中 allowed secondary DG-0083，问题是 Guide-Wiki evidence 组合不足，而非 entry point 完全不可达。

因此不改 Retrieval、不改 Prompt、不改 top_k、不改 selector limit、不改 fallback。

## 8. 建议 Apply 范围

建议选择 B：Batch5-M Apply 小批次。

Apply 范围：

1. 新增 DG-0851 灰烬与余火处理。
2. 新增 DG-0854 病人用品与厨房污染隔离。
3. 不新增 Wiki。
4. 不新增 profile。
5. 只做 Guide-Wiki 双向关联。
6. 刷新 `data/emergency_guides.json` 和 `data/guide_index.json`。
7. 重新跑 Shelter / Fire / WASH Field Test。

建议关联：

- DG-0851：
  - `fire-ash-ember-cooling-disposal-001`
  - `fire-night-final-extinguish-log-001`
  - `fire-small-fire-stop-001`
  - `fire-fire-response-001`
- DG-0854：
  - `hygiene-patient-cup-towel-isolation-001`
  - `hygiene-kitchen-raw-cooked-contamination-line-001`
  - `hygiene-wash-abnormal-record-001`
  - `hygiene-shared-items-001`
  - `hygiene-contaminated-surface-001`

Field Test 预期：

- strict cases 不退化。
- `fire_hot_ash_trash_bag` 应至少 partial -> pass 或更强 evidence partial。
- `wash_patient_cup_kitchen` 应至少 partial -> pass 或 selected Wiki 明显改善。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety / fallback / record-check 保持 100.0%。

## 9. 不建议修改内容

- 不新增 Wiki。
- 不新增 query profile。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不修改 top_k。
- 不修改 selector limit。
- 不修改 fallback。
- 不修改 PocketBase schema。
- 不压低旧 DG-0488 / DG-0223 / DG-0083 / DG-0084。
- 不把旧 Guide 删除或降权。
- 不为 observation case 硬编码 Guide ID 或 Wiki slug。
- 不把 Kiwix 作为行动建议补丁。
- 不扩大 evidence 数量来掩盖 Guide-Wiki 链路问题。

## 10. 是否建议进入 Batch5-M Apply

建议进入 Batch5-M Apply，但保持小批次。

判断：

- 如果只追求 v0.1 stable candidate，可以不 Apply，保留两个 observation partial。
- 如果目标是 Shelter / Fire / WASH v0.2，把灰烬余火和病人用品-厨房污染两条高风险链补成 Guide -> Wiki -> Evidence -> Action，则建议 Apply。
- 推荐做法是新增 DG-0851 和 DG-0854 两个 Guide，不新增 Wiki、不新增 profile、不改 Retrieval。

本阶段不建议选择 C（只新增其中一个 Guide），因为两个 observation 都是高风险边界，且现有 Wiki 已就绪；一起做仍属于小批次，风险低、收益明确。
