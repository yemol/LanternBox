# Batch5-N Shelter / Fire / WASH v0.2 Final Verification

生成日期：2026-07-16

## 1. Batch5-M 后状态

Batch5-M 已完成 Shelter / Fire / WASH v0.2 小批次 Guide 补齐：

- 新增 DG-0851 灰烬与余火处理。
- 新增 DG-0854 病人用品与厨房污染隔离。
- Guide-Wiki 双向关系已完成。
- 未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking 或 fallback。

Batch5-M 后 field regression 状态：

```text
pass / partial / fail = 15 / 1 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
safety / fallback / record-check = 100.0% / 100.0% / 100.0%
```

唯一 partial 为 `fire_hot_ash_trash_bag`。复盘结论是检索并未失败：selected Guide 已为 DG-0851，Fire Wiki evidence 已进入；partial 来自 fixture expected 仍停留在 DG-0851 创建前的基线。

## 2. Fixture 更新说明

本阶段只同步测试基线，未修改生产逻辑。

更新文件：

- `tests/fixtures/shelter_fire_wash_field_cases.json`

更新 case：

- `fire_hot_ash_trash_bag`

调整内容：

|字段|更新前|更新后|
|---|---|---|
|expected primary Guide|DG-0849|DG-0851|
|allowed secondary|DG-0850|DG-0849、DG-0850、DG-0488、DG-0223|
|focus|观察未新增 DG-0851 时 evidence 能否进入|验证 DG-0851 新增后由灰烬与余火处理 Guide 主导，且 Fire Wiki evidence 进入证据链|

未修改：

- Wiki
- Guide
- Retrieval Pipeline
- Prompt
- query profile
- ranking
- top_k
- selector limit
- fallback

## 3. Field Test 前后变化

|指标|Batch5-M 后|Batch5-N 后|
|---|---:|---:|
|用例总数|16|16|
|strict / observation|14 / 2|14 / 2|
|pass / partial / fail|15 / 1 / 0|16 / 0 / 0|
|Guide 命中率|100.0%|100.0%|
|主 Guide 命中率|100.0%|100.0%|
|Wiki 命中率|100.0%|100.0%|
|Guide-Wiki 精准组合率|100.0%|100.0%|
|safety boundary|100.0%|100.0%|
|fallback|100.0%|100.0%|
|record/check|100.0%|100.0%|
|dangerous suggestion|0|0|
|Kiwix 越权|0|0|
|cross domain|0|0|

`fire_hot_ash_trash_bag` 当前结果：

- verdict：pass
- selected Guide：
  - DG-0851 灰烬与余火处理
  - DG-0488 灰烬和焦物处理
  - DG-0627 垃圾渗漏后的地面清理
- selected Wiki：
  - `fire-ash-ember-cooling-disposal-001`
  - `fire-night-final-extinguish-log-001`
  - `fire-small-fire-stop-001`
  - `fire-fire-response-001`
  - `hygiene-hand-hygiene-001`
  - `hygiene-handwashing-001`
- safety / fallback / record-check：true / true / true

## 4. 验证结果

目标验证命令：

```text
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_shelter_fire_wash_retrieval_profiles.py
venv/bin/python scripts/test_shelter_fire_wash_field.py --no-answer
```

执行说明：

- `python3 tools/audit_wiki.py` 已直接运行。
- `python3 scripts/audit_guides.py` 已直接运行。
- 当前工作区的 `venv/bin/python` symlink 指向缺失的 Homebrew Python 3.11 路径，直接执行会报 `No such file or directory`。
- 为避免修改 venv 或生产文件，pytest 与 field test 使用同版本残留 Python 3.11，并通过 `DYLD_FRAMEWORK_PATH` 与 `PYTHONPATH=venv/lib/python3.11/site-packages` 加载相同项目依赖完成等价验证。

结果：

```text
Wiki audit:
Articles: markdown=839 pocketbase=839 categories=24
Issues: errors=0 warnings=0 advisories=0

Guide audit:
Guides: 784
Issues: errors=0 warnings=0 advisories=0

Targeted pytest:
19 passed

Shelter / Fire / WASH field test:
pass / partial / fail = 16 / 0 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
safety / fallback / record-check = 100.0% / 100.0% / 100.0%
```

Field test 输出：

- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_results.json`
- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_report.md`

Audit 输出：

- `docs/knowledge/wiki_audit_2026-07-16.md`
- `docs/knowledge/guide_audit_2026-07-16.md`

## 5. 是否达到 Stable

建议标记为 Shelter / Fire / WASH v0.2 stable。

理由：

- 16 个 field cases 全部 pass。
- strict cases 全部 pass。
- observation cases 全部 pass。
- fail = 0，partial = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- cross domain = 0。
- safety / fallback / record-check 均为 100.0%。
- Wiki audit 0/0/0，Guide audit 0/0/0。
- 本阶段没有通过扩 profile、改 ranking、改 top_k、改 selector limit 或改 fallback 来制造结果。

## 6. 是否建议冻结

建议冻结 Shelter / Fire / WASH v0.2。

冻结范围：

- DG-0847 至 DG-0854 作为 Shelter / Fire / Clothing / WASH 当前稳定行动入口集合。
- Batch5-L 的 4 个 query profile 保持现状，不继续扩展。
- Batch5-M 新增的 DG-0851 / DG-0854 保持现状。
- Field fixture 当前 16 cases 作为 v0.2 stable regression baseline。

冻结后不建议继续做：

- 不继续扩展 profile。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不调整 top_k。
- 不调整 selector limit。
- 不修改 ranking。
- 不新增 Shelter / Fire / WASH Wiki。
- 不新增 Shelter / Fire / WASH Guide。

后续如进入新批次，应以新的能力域或新的 field failure 为触发条件，不在 Batch5-N 阶段继续扩展。
