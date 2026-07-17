# LanternBox Batch9-F：Manufacturing Retrieval Minimal Apply Report

日期：2026-07-17

阶段性质：最小检索入口修复。  
遵守：`docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

本阶段未新增 Wiki，未新增 Guide，未修改 Wiki 正文，未修改 Guide 正文，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback、schema 或 PocketBase schema。

## 1. 新增 profile

在 `data/retrieval_query_profiles.json` 新增 4 个 manufacturing query profile：

|profile|目标|优先入口|边界|
|---|---|---|---|
|`manufacturing_workspace_safety`|工坊开工、低光、粉尘、火花、儿童隔离、收工封存|DG-0871、DG-0878|已经发生伤害后仍让 medical / fire / safety 补充或主导|
|`manufacturing_material_reuse`|废木板、废金属、废塑料、旧材料进入制作流程前判断|DG-0872、DG-0876|废弃物污染处理仍由 waste / hygiene 主导|
|`manufacturing_cutting_clamping`|切割、锯切、打孔、夹持、材料滑动、低光切割|DG-0873|工具本身损坏仍可由 tools / repair 主导|
|`manufacturing_connection_load_check`|连接方式、结构件、承重前检查、重复尺寸制作|DG-0874、DG-0875、DG-0877|修已有架子仍可由 repair 主导|

修复中对 profile 做了两处收窄：

- `manufacturing_material_reuse` 不使用裸的“能不能用”触发，改为“能不能拿来 / 能不能用来”，避免“能不能用脚踩”误触发材料再利用。
- `manufacturing_connection_load_check` 不使用 DG-0877 exact title 作为宽泛 title alias，避免连接方式 query 被承重检查抢主位。

## 2. Guide-Wiki 顺序调整

只调整以下 Guide 的 `related_wiki` 顺序，未新增或删除 Guide-Wiki 关系：

|Guide|调整目的|
|---|---|
|DG-0871 工坊开工前安全检查|前置工作区分区、开工检查、粉尘火花、切割清场、PPE、儿童隔离|
|DG-0872 材料再利用前安全判断|前置废旧材料总检查、木材选择、金属废料、塑料边界、材料预处理|
|DG-0873 木材切割前固定与支撑|前置夹持、锯切支撑、打孔定位、划线、清场|
|DG-0875 连接方式选择|前置连接方式、螺丝、旧紧固件、金属连接、软材料|
|DG-0876 废旧金属材料安全处理|前置金属废料、防割边、薄金属打孔、锈蚀判断|
|DG-0877 制作完成后的承重检查|前置承重测试、质量复查、架子支撑、金属支架、缺陷隔离|
|DG-0878 工坊收工与工具封存|前置收工清点、危险工具、儿童隔离、半成品/成品分区|

已运行 `python3 tools/build_guides.py` 刷新：

- `data/emergency_guides.json`
- `data/guide_index.json`

## 3. Contract test 结果

新增：

- `tests/test_manufacturing_retrieval_profiles.py`

覆盖 8 个入口：

1. 废木板做架子 -> DG-0872，DG-0877 进入 selected evidence。
2. 切木板固定 / 用脚踩 -> DG-0873。
3. 低光继续切割 -> DG-0871 或 DG-0873，DG-0871 必须进入前二。
4. 火花飞到布料 -> DG-0871。
5. 重复同尺寸小木块 -> DG-0874 或 DG-0877。
6. 做好的架子放重物 -> DG-0877 必须进入前二并带出承重 Wiki。
7. 胶水 / 螺丝 / 绳子连接选择 -> DG-0875。
8. 废金属片锋利做支架 -> DG-0876。

运行结果：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_manufacturing_retrieval_profiles.py

17 passed
```

## 4. Field Test 前后变化

参考 Batch9-D / Batch9-E 初始状态：

|指标|Batch9-D|Batch9-F|
|---|---:|---:|
|total|18|18|
|strict / observation|12 / 6|12 / 6|
|pass / partial / fail|11 / 2 / 5|16 / 2 / 0|
|strict pass / partial / fail|5 / 2 / 5|10 / 2 / 0|
|Guide 命中率|91.7%|100.0%|
|主 Guide 命中率|83.3%|100.0%|
|Wiki 全量命中率|50.0%|83.3%|
|Guide-Wiki 精准组合率|75.0%|100.0%|
|Manufacturing 主 Guide 进入率|50.0%|83.3%|
|dangerous suggestion|0|0|
|Kiwix 越权|0|0|
|safety / fallback / record-check|100% / 100% / 100%|100% / 100% / 100%|

Field Test 命令：

```text
python3 scripts/test_manufacturing_field.py --no-answer
```

结果文件：

- `docs/knowledge/batch9_d_manufacturing_field_test_results.json`
- `docs/knowledge/batch9_d_manufacturing_field_test_report.md`

## 5. 剩余 partial 分类

|case|状态|分类|说明|
|---|---|---|---|
|`manufacturing_scrap_wood_shelf_judgement`|partial|selector / top8 evidence 截断|DG-0872、DG-0877、DG-0876 已进入 selected Guide，Guide-Wiki precise 为 true；但 selected Wiki 前 8 被 DG-0872 的材料判断链占满，`repair-manufacturing-load-test-001` 未进入前 8。|
|`manufacturing_shutdown_child_tool_safety`|partial|selector / top8 evidence 截断|DG-0871、DG-0878 已进入 selected Guide；但 selected Wiki 前 8 被 DG-0871 开工安全链占满，DG-0878 的收工清点 Wiki 未进入前 8。|

不继续扩 profile。两项都不是 Guide 缺失、Wiki 缺失或危险建议问题，而是 selected Wiki 截断与 fixture 粒度问题。

## 6. 验证结果

```text
python3 tools/audit_wiki.py
Articles: markdown=950 pocketbase=950 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 804
Issues: errors=0 warnings=0 advisories=0
```

Retrieval contract：

```text
17 passed
```

Manufacturing Field Test：

```text
pass / partial / fail = 16 / 2 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
safety / fallback / record-check = 100% / 100% / 100%
```

## 7. 是否达到 Manufacturing Retrieval v0.1 stable candidate

结论：达到 Manufacturing Retrieval v0.1 stable candidate。

理由：

- fail 已从 5 降为 0。
- strict case Guide 命中率达到 100%。
- Guide-Wiki 精准组合率达到 100%。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety / fallback / record-check 全部 100%。

保留说明：

- 当前不是 final stable freeze。
- 仍有 2 个 strict partial，均为 selected Wiki top8 截断问题。
- 不建议继续增加 profile；后续如要清理 partial，应先讨论 Field Test fixture 粒度或 selector 的 evidence 分配策略，而不是扩知识或硬编码排序。
