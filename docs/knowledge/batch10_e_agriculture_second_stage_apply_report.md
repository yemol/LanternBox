# LanternBox Batch10-E: Agriculture Second Stage Retrieval Minimal Apply Report

生成时间：2026-07-18

本阶段遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`，只做 Agriculture Second Stage Retrieval evidence 入口稳定性修复。未新增 Wiki，未新增 Guide，未修改 Wiki 正文，未修改 Guide steps/check/fallback/stop_or_escalate，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback、schema 或 PocketBase schema。

## 1. 新增 profile

在 `data/retrieval_query_profiles.json` 新增 4 个 agriculture query profile：

|profile|目标|优先入口|secondary / 边界|
|---|---|---|---|
|`agriculture_seed_library`|种子库、种子批次、发芽率复测、受潮失效、留种保底线|DG-0879|DG-0886、Food；已入口食用的霉变粮食仍由 Food safety 主导|
|`agriculture_pest_disease_control`|病虫害周期管理、病害工具分流、虫卵、病株、跨区传播|DG-0883|Tools / hygiene / contamination；工具损坏仍由 Tools / Repair 主导|
|`agriculture_postharvest_storage`|收获后晾晒、防霉、留种食用批次分离、储藏容器、鼠虫防护|DG-0885|DG-0879、Food、storage、manufacturing|
|`agriculture_contaminated_plot_boundary`|不明化学污染地块、食用种植区停用、污染材料进入菜地|DG-0883 / DG-0882|Contamination / medical / water 保留人体接触、饮水和污染区主入口|

这些 profile 只强化 Batch10-D 已确认的 Agriculture Second Stage 入口，不改变 retrieval pipeline 或 selector 机制。

## 2. Guide-Wiki 顺序调整

只调整少量 `related_wiki` 顺序，未改 Guide 正文：

|Guide|调整|
|---|---|
|DG-0879 种子保存与发芽率复测|前排固定为种子批次台账、种子干燥复查、种子受潮失效、留种保底线、种子库盒索引、留种/食用批次分离|
|DG-0883 病虫害早期隔离与工具分流|将 `agriculture-diseased-tool-zone-separation-001` 和 `agriculture-unknown-chemical-plot-stop-001` 前移到核心 evidence 区|
|DG-0885 收获后晾晒防霉与储藏|复核后不需要调整，前排已经覆盖晾晒、防霉、批次分离、容器检查、鼠虫防护、霉变丢弃|

已运行 `python3 tools/build_guides.py` 刷新 `data/emergency_guides.json` 和 `data/guide_index.json`，生成 812 Guides / 812 Guide Index Items。

## 3. Contract test 结果

新增 `tests/test_agriculture_second_stage_retrieval_profiles.py`，覆盖 9 个 profile 合同场景：

- 旧种子发芽率低是否可大面积播种
- 种子受潮霉味是否可混回种子库
- 剪过病株的剪刀是否可继续跨区使用
- 疑似化学污染地块是否可种食用作物
- 发臭厨余堆肥是否可倒在叶菜旁边
- 粪肥是否可直接用于食用菜地
- 收获豆子晾晒防霉
- 留种和食用批次分离
- 收成少时是否吃掉留种

回归命令：

```bash
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_agriculture_second_stage_retrieval_profiles.py
```

结果：

```text
18 passed in 2.12s
```

## 4. Field Test 前后变化

Batch10-C / Batch10-D 基线：

|指标|Batch10-C / D|
|---|---:|
|total|24|
|strict / observation|18 / 6|
|pass / partial / fail|20 / 0 / 4|
|Guide hit|94.4%|
|Wiki hit|94.4%|
|Guide-Wiki precise|94.4%|
|Agriculture Second Stage top1 rate|70.8%|
|dangerous suggestion|0|
|Kiwix 越权|0|

Batch10-E 刷新后：

|指标|Batch10-E|
|---|---:|
|total|24|
|strict / observation|18 / 6|
|pass / partial / fail|24 / 0 / 0|
|strict pass / partial / fail|18 / 0 / 0|
|Guide hit|100.0%|
|expected Guide hit|94.4%|
|Wiki full hit|100.0%|
|Guide-Wiki precise|100.0%|
|Agriculture Second Stage primary rate|95.8%|
|safety / fallback / record-check|100% / 100% / 100%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|cross domain|1|

4 个原 fail 已全部修复：

|case|Batch10-D 根因|Batch10-E 结果|
|---|---|---|
|`agri_seed_low_germination_large_sowing`|旧 Planting Guide 抢 top1|DG-0879 top1，pass|
|`agri_diseased_pruner_cross_use`|旧 Planting Guide 抢 top1，工具分区 Wiki 靠后|DG-0883 top1，pass|
|`agri_seed_food_batch_separation`|旧留种 Guide 抢 top1|DG-0885 top1，DG-0879 secondary，pass|
|`agri_unknown_chemical_plot_food`|Agriculture profile 缺失，contamination / medical / water 抢主位|DG-0883 top1，expected Wiki 进入，pass|

Field Test 命令：

```bash
python3 scripts/test_agriculture_second_stage_field.py --no-answer
```

输出文件：

- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_results.json`
- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_report.md`

说明：现有 Field Test 脚本沿用 Batch10-C 输出路径，本次内容已刷新为 Batch10-E 后结果。

## 5. 剩余 partial 分类

剩余 partial：0。

剩余 observation cross-domain：1。

|case|现象|判断|
|---|---|---|
|`agri_observe_moldy_grain_eat`|Food safety top1，DG-0886 进入 selected|合理跨域。query 是“粮食有霉味还能不能吃”，入口应由 Food safety 主导，Agriculture evidence 作为种源和储粮管理补充|

不继续扩 profile。该 observation 不属于 Agriculture Retrieval 失败，也不需要压低 Food safety。

## 6. 审计结果

```text
python3 tools/audit_wiki.py
Articles: markdown=990 pocketbase=990 categories=24
Issues: errors=0 warnings=0 advisories=0
```

```text
python3 scripts/audit_guides.py
Guides: 812
Issues: errors=0 warnings=0 advisories=0
```

## 7. 未修改项

- 未新增 Wiki
- 未新增 Guide
- 未修改 Wiki 正文
- 未修改 Guide 正文
- 未修改 Retrieval Pipeline
- 未修改 Prompt
- 未修改 top_k
- 未修改 selector limit
- 未修改 ranking
- 未修改 fallback
- 未修改 schema
- 未同步 PocketBase schema
- 未做农药配方、化学灭虫剂自制、高复杂农业工程或畜牧深水区内容

## 8. 是否达到 stable candidate

达到 Agriculture Second Stage Retrieval v0.1 stable candidate。

理由：

1. strict cases 全部 pass，fail = 0。
2. Guide hit、Wiki hit、Guide-Wiki precise 均达到 100%。
3. dangerous suggestion = 0，Kiwix 越权 = 0。
4. safety / fallback / record-check 保持 100%。
5. 唯一 cross-domain 为合理 observation：已霉变粮食入口食用判断由 Food safety 主导。

建议进入 Batch10-F Final Verification。Batch10-F 只做最终验证和冻结报告，不继续扩 profile。
