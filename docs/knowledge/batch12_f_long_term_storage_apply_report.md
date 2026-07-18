# LanternBox Batch12-F: Long-Term Storage Retrieval Minimal Apply Report

生成日期：2026-07-18

本阶段遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`，只做 Long-Term Storage Retrieval evidence 入口稳定性最小修复。未新增 Wiki，未新增 Guide，未修改 Wiki 正文，未修改 Guide 正文，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback 或 schema，未同步 PocketBase schema，未处理 `.env` 或无关 dirty 文件。

## 1. 新增 Profile

新增 8 个 Long-Term Storage query profile，用于保护 Storage 的主行动链：保存 -> 标签 -> 复查 -> 轮换 -> 隔离 -> 交接。

|profile|主保护入口|边界|
|---|---|---|
|`storage_basic_zone_labeling`|DG-0895 储藏区基础分区与标签|制作货架由 Manufacturing 主导；Storage 只负责分区、标签、正常库存和隔离区。|
|`storage_food_grain_rotation`|DG-0896 / DG-0901 / DG-0902|入口食用、食物中毒由 Food 主导；未入口前的储粮保存、隔离、FIFO 由 Storage 主导。|
|`storage_seed_storage`|DG-0897|种子能不能播、发芽率和生产计划由 Agriculture 主导；种子保存、受潮复查、种食分离、轮换记录由 Storage 主导。|
|`storage_medical_supply_storage`|DG-0898 / DG-0901|治疗、药品使用判断由 Medical 主导；药箱保存、包装破损隔离、急救包复查由 Storage 主导。|
|`storage_tools_materials_storage`|DG-0899 / DG-0902|工具使用维修由 Tools/Repair 主导；工具材料保存、分类、领用记录由 Storage 主导。|
|`storage_energy_fuel_storage`|DG-0900|电池使用、充放电、火源操作由 Energy/Fire 主导；长期保存、分区、标签、复查由 Storage 主导。|
|`storage_suspicious_item_isolation`|DG-0901|人体接触污染由 Medical/Contamination 主导；可疑物品不进正常库存、待复查标签由 Storage 主导。|
|`storage_inventory_handover`|DG-0902|通信/任务交接由相邻领域主导；储藏库存卡、批次、领用、复查日、FIFO 交接由 Storage 主导。|

实现过程中对 `storage_inventory_handover` 做了收窄处理：使用 `trigger_any` 只覆盖库存卡、储藏交接、先入先出、轮换记录等记录语义，避免在“鼠咬批次隔离”中抢 DG-0901 / DG-0896 主位。

## 2. Guide-Wiki 顺序调整

只调整 Storage Guide 的 `related_wiki` 顺序，未新增或删除 Guide-Wiki 边。

|Guide|调整目的|
|---|---|
|DG-0896|让干粮分区、受潮结块、鼠咬批次隔离、虫鼠巡查优先进入 evidence。|
|DG-0897|让受潮隔离、留种/食用分架、种子库轮换卡优先进入 evidence。|
|DG-0898|让敷料绷带干燥封存、过期/不明药品暂存、急救包复查优先进入 evidence。|
|DG-0900|让电池干燥阴凉储藏、充电宝轮换、火柴防潮、燃料远离生活区优先进入 evidence。|
|DG-0901|让漏液异味、可疑批次、过期暂存、污染容器优先进入 evidence。|
|DG-0902|让储藏区交接卡、库存卡、复查日历、异常记录、FIFO 优先进入 evidence。|

## 3. Contract Test 结果

新增：`tests/test_long_term_storage_retrieval_profiles.py`

覆盖 12 条核心查询，包括储藏区分区、干粮防潮防虫、鼠咬/霉味批次隔离、FIFO、种子受潮与分架、急救包复查、药品包装破损、电池/充电宝长期保存、漏液异味隔离和储藏交接。

运行结果：

```text
21 passed
```

命令：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_long_term_storage_retrieval_profiles.py
```

## 4. Field Test 前后变化

|阶段|total|strict / observation|pass / partial / fail|Guide hit|Wiki full hit|Guide-Wiki precise|Storage primary|
|---|---:|---:|---:|---:|---:|---:|---:|
|Batch12-D|26|20 / 6|16 / 1 / 9|80.0%|70.0%|80.0%|50.0%|
|Batch12-F|26|20 / 6|26 / 0 / 0|100.0%|100.0%|100.0%|88.5%|

刷新后的 Field Test：

```text
pass / partial / fail = 26 / 0 / 0
strict pass / partial / fail = 20 / 0 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
safety / fallback / record-check = 100.0% / 100.0% / 100.0%
cross domain = 3
```

Cross-domain 剩余信号均来自 observation 场景，属于合理边界：

|observation|主域判断|
|---|---|
|粮食有霉味还能不能吃？|Food 主导入口食用判断合理，Storage 不应吞掉。|
|电池漏液碰到手怎么办？|Medical / Energy / Contamination 主导人体接触和电池异常合理。|
|旧容器有不明残留，还能不能继续用来储藏？|Waste / Recycling 主导容器再利用边界合理，Storage 可作为后续储藏补充。|

## 5. 审计结果

```text
Wiki audit: errors=0 warnings=0 advisories=0
Guide audit: errors=0 warnings=0 advisories=0
```

审计输出：

- Markdown Wiki：1066
- PocketBase wiki_articles：1066
- Guide：828

## 6. 剩余 Partial 分类

无剩余 partial。

本轮不继续扩 profile，不新增 Storage Wiki/Guide，不再调整 Guide-Wiki 顺序。

## 7. Stable Candidate 判断

Long-Term Storage Retrieval v0.1 已达到 stable candidate：

- strict cases 全部 pass
- observation cases 全部 pass
- fail = 0
- dangerous suggestion = 0
- Kiwix 越权 = 0
- safety / fallback / record-check = 100%
- Guide / Wiki / Guide-Wiki precise = 100%

建议进入 Batch12-G Final Verification，只做最终审计、contract test 和 Field Test 冻结验证。
