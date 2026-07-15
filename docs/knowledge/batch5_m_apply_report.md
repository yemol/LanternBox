# Batch5-M Shelter / Fire / WASH v0.2 Small Guide Expansion Apply Report

生成日期：2026-07-15

## 1. 新增 Guide

本批只新增两个已经在 Batch5-M Planning 中确认的高风险行动入口。

|Guide|title|category|priority|risk_level|位置|
|---|---|---|---|---|---|
|DG-0851|灰烬与余火处理|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|`data/guides/fire/DG-0851.json`|
|DG-0854|病人用品与厨房污染隔离|污染控制 / 隔离 / 清洁分区|P0|high|`data/guides/hygiene/DG-0854.json`|

DG-0851 覆盖做饭、取暖、蜡烛、炉具或炭火结束后的灰烬、热炭、余火、热炉具和焦物处理；核心停止线是未确认冷却前不进入塑料袋、普通垃圾、衣物旁、燃料旁或睡眠区。

DG-0854 覆盖病人杯子、毛巾、餐具、清洁用品或垃圾接近厨房、饮水桶、公共餐具或食物处理面后的停用、隔离、清洁、记录和恢复判断。

## 2. Guide-Wiki 关联

本批只做必要的双向关联，没有新增 Wiki，也没有修改 Wiki 正文。

DG-0851 related_wiki：

- `fire-ash-ember-cooling-disposal-001`
- `fire-night-final-extinguish-log-001`
- `fire-small-fire-stop-001`
- `fire-fire-response-001`

DG-0854 related_wiki：

- `hygiene-patient-cup-towel-isolation-001`
- `hygiene-kitchen-raw-cooked-contamination-line-001`
- `hygiene-wash-abnormal-record-001`
- `hygiene-shared-items-001`
- `hygiene-contaminated-surface-001`

同步修改的 Wiki frontmatter 仅限 `guide_links` 字段，用于保持 `audit_guides.py` 的双向关系合同。

PocketBase 检查：当前本地 PocketBase 只有 `wiki_articles`、`wiki_categories`、`wiki_media` 等表，`wiki_articles` 不包含 `guide_links` 字段，也没有 Guide 关系表。因此本批没有额外 PocketBase Guide/Wiki 关联可同步；Guide-Wiki 关系由 Guide JSON、Wiki frontmatter 和生成索引维护。

## 3. 是否新增 Wiki

否。

本批没有新增 Wiki，没有修改 Wiki 正文，只更新了 9 个已有 Wiki 的 frontmatter `guide_links`。

## 4. 是否修改 Retrieval

否。

未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking 或 fallback。未新增 profile，未压低旧 Guide，未删除旧 Guide，未硬编码测试答案。

## 5. 测试结果

已运行：

```text
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_shelter_fire_wash_retrieval_profiles.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_shelter_fire_wash_retrieval_profiles.py
venv/bin/python scripts/test_shelter_fire_wash_field.py --no-answer
```

结果：

```text
Wiki audit:
Articles: markdown=839 pocketbase=839 categories=24
Issues: errors=0 warnings=0 advisories=0

build_guides:
Generated 784 Guides
Generated 784 Guide Index Items

Guide audit:
Guides: 784
Issues: errors=0 warnings=0 advisories=0

Shelter / Fire / WASH contract tests:
10 passed

Targeted retrieval contract tests:
19 passed

Shelter / Fire / WASH field regression:
pass / partial / fail = 15 / 1 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
safety / fallback / record-check = 100.0% / 100.0% / 100.0%
```

新增 contract 覆盖：

- “灰烬看似灭了但仍热，是否可以倒垃圾袋？” -> DG-0851，且 DG-0851 related_wiki 包含 `fire-ash-ember-cooling-disposal-001` 和 `fire-night-final-extinguish-log-001`。
- “病人杯子毛巾进入厨房区域怎么办？” -> DG-0854，且 DG-0854 related_wiki 包含 `hygiene-patient-cup-towel-isolation-001`、`hygiene-kitchen-raw-cooked-contamination-line-001` 和 `hygiene-wash-abnormal-record-001`。

## 6. Batch5-L -> Batch5-M 变化

|指标|Batch5-L 后|Batch5-M 后|
|---|---:|---:|
|Guide 总数|782|784|
|新增 Guide|0|2|
|新增 Wiki|0|0|
|新增 profile|4|0|
|pass / partial / fail|14 / 2 / 0|15 / 1 / 0|
|strict cases|全部 pass|全部 pass|
|dangerous suggestion|0|0|
|Kiwix 越权|0|0|
|cross domain|0|0|
|safety boundary|100.0%|100.0%|
|fallback|100.0%|100.0%|
|record/check|100.0%|100.0%|

两个目标 observation 的变化：

|case|Batch5-L|Batch5-M|说明|
|---|---|---|---|
|`fire_hot_ash_trash_bag`|partial|partial|实际 selected Guide 已为 DG-0851，目标 Fire Wiki 已进入 evidence；仍为 partial 是因为 field fixture 仍以“未新增 DG-0851”时期的 DG-0849 / DG-0850 作为 expected / allowed Guide。按要求不继续扩 profile。|
|`wash_patient_cup_kitchen`|partial|pass|selected Guide 为 DG-0854，目标 patient / kitchen / abnormal-record Wiki 已进入 evidence。|

## 7. 剩余 Observation

剩余 1 个 observation partial：

- `fire_hot_ash_trash_bag`

当前实际 selected Guide：

- DG-0851 灰烬与余火处理
- DG-0488 灰烬和焦物处理
- DG-0627 垃圾渗漏后的地面清理

当前实际 selected Wiki：

- `fire-ash-ember-cooling-disposal-001`
- `fire-night-final-extinguish-log-001`
- `fire-small-fire-stop-001`
- `fire-fire-response-001`
- `hygiene-hand-hygiene-001`
- `hygiene-handwashing-001`

判断：这是合理 observation partial，不是安全缺口。DG-0851 已成为主入口，目标 Wiki 已进入 evidence，safety / fallback / record-check 均为 true；不建议为了消除该 partial 修改 profile、Retrieval、Prompt、top_k、selector limit、ranking 或 fallback。

## 8. 是否达到 Shelter / Fire / WASH v0.2 Candidate

建议标记为 Shelter / Fire / WASH v0.2 candidate。

理由：

- 两个计划中的高风险行动入口 DG-0851 / DG-0854 已补齐。
- 没有新增 Wiki，没有扩大 profile，没有修改 Retrieval Pipeline。
- Wiki audit 0/0/0，Guide audit 0/0/0。
- strict cases 全部 pass。
- field regression 为 15 / 1 / 0，fail = 0。
- dangerous suggestion = 0，Kiwix 越权 = 0，cross domain = 0。
- safety / fallback / record-check 全部 100.0%。
- 唯一 partial 是 observation 口径残留，实际证据链已满足 Guide -> Wiki -> Evidence -> Action。
