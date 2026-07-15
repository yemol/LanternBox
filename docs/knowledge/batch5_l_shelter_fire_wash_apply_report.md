# Batch5-L Shelter / Fire / WASH Retrieval Minimal Apply Report

生成日期：2026-07-15

## 1. 修改文件列表

本批只执行 Batch5-K 确认的 retrieval entry point 最小修复和 DG-0853 WASH evidence priority 补强。

修改 / 新增文件：

- `data/retrieval_query_profiles.json`
- `data/guides/hygiene/DG-0853.json`
- `wiki_import/hygiene/hygiene-contamination-zone-visible-marking-001.md`
- `wiki_import/hygiene/hygiene-wash-abnormal-record-001.md`
- `data/emergency_guides.json`
- `data/guide_index.json`
- `tests/test_shelter_fire_wash_retrieval_profiles.py`
- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_results.json`
- `docs/knowledge/batch5_j_shelter_fire_wash_field_test_report.md`
- `docs/knowledge/batch5_l_shelter_fire_wash_apply_report.md`

未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback、PocketBase schema，也没有新增 Wiki 或新增 Guide。

## 2. 新增 Profile 列表

新增 4 个 query profile：

|profile|目标|主导 Guide|
|---|---|---|
|`shelter_ventilation_heat_balance`|门窗堵死 + 保暖 / 通风 / 睡眠 / 火源 / 多人语义时，避免被 repair 门窗加固主导。|DG-0848 / DG-0850|
|`fire_smoke_combustion_stop`|烟倒灌、烟往屋里、开窗还是灭火、室内燃烧、一氧化碳停止线。|DG-0850，DG-0849 补充|
|`clothing_wet_cold_ppe`|湿袜、鞋内进水、发抖、继续干活、手套破损、PPE、污染衣物。|DG-0852，DG-0848 补充|
|`wash_zone_layout_priority`|饮水 / 洗手 / 厕所 / 厨房 / 病人用品 / 污染区综合运行。|DG-0853，旧 hygiene Guide 补充|

这些 profile 使用对象 + 状态双触发，不为单个测试题硬编码 Guide ID 或 Wiki slug，也不压低 repair / food / medical / hygiene 旧 Guide。

## 3. DG-0853 Related Wiki 补强

仅对 DG-0853 做最小补强，新增 2 条 related_wiki：

- `hygiene-contamination-zone-visible-marking-001`
- `hygiene-wash-abnormal-record-001`

并同步对应 Wiki 的 `guide_links: DG-0853`，保持 Guide-Wiki 双向关系。

关系校验：

```text
forward_edges=1866
reverse_edges=1866
Guide-Wiki single_forward_without_reverse=0
Guide-Wiki single_reverse_without_forward=0
invalid_guide_id=0
invalid_wiki_slug=0
```

## 4. 是否新增 Wiki / Guide

- 新增 Wiki：否。
- 新增 Guide：否。
- 未新增 DG-0851 灰烬与余火处理。
- 未新增 DG-0854 病人用品与厨房污染隔离。

DG-0851 / DG-0854 继续作为第二批 Guide 设计观察项。

## 5. Contract Test 结果

新增 `tests/test_shelter_fire_wash_retrieval_profiles.py`，覆盖 8 个入口：

1. 门窗堵死保暖 -> DG-0848 / DG-0850，不由 repair 主导。
2. 烟倒灌开窗还是灭火 -> DG-0850，DG-0849 补充，不由 food cooking 主导。
3. 湿袜鞋内进水外出 -> DG-0852。
4. 冷得发抖还要干活 -> DG-0852 / DG-0848。
5. 手套破损清理污染物 -> DG-0852，hygiene / medical 只补充。
6. 水桶 / 厨房 / 桶厕空间分区 -> DG-0853。
7. 桶厕快满异味 -> DG-0853，DG-0626 补充。
8. 洗手水不够 -> DG-0853，保持原 pass 不退化。

结果：

```text
tests/test_shelter_fire_wash_retrieval_profiles.py
8 passed
```

完整 targeted pytest：

```text
tests/test_retrieval_traceability.py
tests/test_retrieval_root_contract.py
tests/test_shelter_fire_wash_retrieval_profiles.py
17 passed
```

## 6. Batch5-J 前后变化

|指标|Batch5-J 前|Batch5-L 后|
|---|---:|---:|
|用例总数|16|16|
|strict / observation|14 / 2|14 / 2|
|pass / partial / fail|7 / 6 / 3|14 / 2 / 0|
|Guide 命中率（strict，含 allowed secondary）|50.0%|100.0%|
|主 Guide 命中率（strict，仅 expected）|50.0%|100.0%|
|Wiki 命中率（strict）|50.0%|100.0%|
|Guide-Wiki 精准组合率（strict）|50.0%|100.0%|
|safety boundary|100.0%|100.0%|
|fallback|100.0%|100.0%|
|record/check|100.0%|100.0%|
|dangerous suggestion|0|0|
|Kiwix 越权|0|0|
|跨域竞争|3|0|

重点修复 case：

- `shelter_ventilation_keep_warm`：fail -> pass。
- `fire_smoke_backdraft_room`：fail -> pass。
- `clothing_wet_socks_outing`：partial -> pass。
- `clothing_shivering_keep_working`：partial -> pass。
- `clothing_gloves_contaminated`：fail -> pass。
- `wash_water_toilet_kitchen_layout`：partial -> pass。
- `wash_bucket_toilet_full`：partial -> pass。
- `wash_limited_handwater`：保持 pass。

## 7. 剩余 Partial / Fail 分类

剩余：

|case|verdict|分类|说明|
|---|---|---|---|
|`fire_hot_ash_trash_bag`|partial|合理 partial / 后续 Guide 设计候选|observation case。本批未新增 DG-0851；旧 DG-0488 / DG-0223 有安全边界，但 Batch5-I ash Wiki 不进入 selected evidence。|
|`wash_patient_cup_kitchen`|partial|合理 partial / 后续 Guide 设计候选|observation case。本批未新增 DG-0854；当前命中 allowed secondary DG-0083，但 patient/kitchen 新 Wiki 未进入 selected evidence。|

fail：0。

## 8. Stable Candidate 判断

建议标记为 Shelter / Fire / WASH Retrieval v0.1 stable candidate。

理由：

- strict cases 已全部 pass。
- fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety / fallback / record-check 均为 100.0%。
- 跨域竞争降为 0。
- 剩余 2 个 partial 均为 observation，且对应 DG-0851 / DG-0854 已明确留作第二批 Guide 设计观察项。

## 9. 验证结果

已运行：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_shelter_fire_wash_retrieval_profiles.py
python3 -m py_compile scripts/test_shelter_fire_wash_field.py
venv/bin/python scripts/test_shelter_fire_wash_field.py --no-answer
```

结果：

```text
Wiki audit:
Articles: markdown=839 pocketbase=839 categories=24
Issues: errors=0 warnings=0 advisories=0

build_guides:
Generated 782 Guides
Generated 782 Guide Index Items

Guide audit:
Guides: 782
Issues: errors=0 warnings=0 advisories=0

pytest:
17 passed

py_compile:
passed

Batch5-J field regression:
pass / partial / fail = 14 / 2 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
```

## 10. 不建议继续修复的内容

本批不建议继续扩 profile 或扩大 evidence。特别不建议：

- 不扩大 top_k。
- 不扩大 selector limit。
- 不修改 Prompt。
- 不修改 Retrieval Pipeline。
- 不修改 ranking / fallback。
- 不新增大量 Guide。
- 不新增大量 Wiki。
- 不压低 repair / food / medical / hygiene 旧 Guide。
- 不为单个测试题硬编码 Guide ID 或 Wiki slug。
- 不把 Kiwix 作为行动建议补丁。
- 不在本批新增 DG-0851 / DG-0854。

后续第二批候选：

- DG-0851 灰烬与余火处理。
- DG-0854 病人用品与厨房污染隔离。
