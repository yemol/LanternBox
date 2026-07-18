# LanternBox Batch11-E: Waste / Recycling Retrieval Minimal Apply Report

生成日期：2026-07-18

本阶段执行 Waste / Recycling v0.1 Retrieval evidence 入口最小修复。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。未新增 Wiki，未新增 Guide，未修改 Wiki 正文或 Guide 正文，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback 或 schema。

参考：

- `docs/knowledge/batch11_d_waste_recycling_root_cause_review.md`
- `docs/knowledge/batch11_c_waste_recycling_field_test_report.md`
- `docs/knowledge/batch11_b_waste_recycling_apply_report.md`

## 1. 新增 Profile

本批新增 6 个 Waste / Recycling query profile，只保护“分类 -> 隔离 -> 材料池 -> 交接记录”主行动链。

|profile|目标|primary Guide|边界|
|---|---|---|---|
|`waste_basic_sorting`|混合垃圾、基础分类、临时隔离、污染垃圾桶位置、远离生活区|DG-0887|人体已接触污染时让 WASH / Medical / Contamination 主导。|
|`waste_sharp_broken_material`|碎玻璃、锋利物、金属边角、钉子、未受伤前的收集和标记|DG-0888|已经割伤 / 扎伤时由 Medical 主导。|
|`waste_contaminated_trash`|病人垃圾、不明刺鼻废弃物、电池漏液物、污染垃圾临时隔离和标记|DG-0889 / DG-0887|人体暴露由 Medical / Contamination 主导；电池使用和储能风险由 Energy 主导。|
|`waste_kitchen_scrap_boundary`|厨余、腐败厨余、病人剩饭、油脂肉类进入堆肥前分流|DG-0890|堆肥成熟和进入食用地块由 Agriculture 主导。|
|`waste_ash_char_boundary`|热灰、冷灰、炭渣、灰烬进入普通垃圾前判断和分流|DG-0891|火源是否安全、复燃风险由 Fire 主导。|
|`waste_material_pool_reuse`|废木板、旧金属片、塑料桶、旧容器、材料池、材料等级、记录、交接|DG-0892 / DG-0893 / DG-0894|加工制作由 Manufacturing 主导；材料准入、残留污染、等级和交接由 Waste 主导。|

未新增 `waste_record_handover` 独立 profile；记录/交接语义被收进 `waste_material_pool_reuse`，避免一次性扩到 7-8 个 profile。

## 2. Guide-Wiki 顺序调整

只调整 Waste Guide 的 `related_wiki` 顺序，没有改 Guide 正文。

|Guide|调整重点|
|---|---|
|DG-0887|将基础分类、来源标签、混合垃圾禁止线、电池漏液、不明污染物放在前段。|
|DG-0888|保持碎玻璃/锋利物和金属边角为前两位。|
|DG-0889|将病人垃圾、不明污染物、电池漏液、污染容器放在前段。|
|DG-0890|将腐败湿垃圾、病人剩饭、厨余进堆肥前分拣放在前段。|
|DG-0891|将热灰、冷灰、灰烬混入普通垃圾风险、炭渣放在前段。|
|DG-0892|将废木板、金属片、入池清单、降级标签、材料池分区放在前段。|
|DG-0893|保持塑料容器和污染容器边界为前两位。|
|DG-0894|将材料池台账、回收失败、废弃物交接、来源风险记录放在前段。|

## 3. Contract Test 结果

新增：

- `tests/test_waste_recycling_retrieval_profiles.py`

覆盖 11 条 contract cases：

1. 混合垃圾分类与临时隔离 -> DG-0887
2. 碎玻璃收集和标记 -> DG-0888
3. 金属边角和钉子 -> DG-0888
4. 电池漏液垃圾混普通垃圾 -> DG-0889 / DG-0887
5. 病人纸巾垃圾 -> DG-0889
6. 腐败厨余渗液进堆肥 -> DG-0890
7. 热灰倒垃圾桶 -> DG-0891
8. 废木板进材料池 -> DG-0892
9. 旧塑料桶异味 -> DG-0893
10. 材料池登记 -> DG-0894
11. 废弃物交接 -> DG-0894

运行结果：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_waste_recycling_retrieval_profiles.py

20 passed in 2.73s
```

## 4. Field Test 前后变化

|指标|Batch11-C|Batch11-E|
|---|---:|---:|
|total|24|24|
|strict / observation|18 / 6|18 / 6|
|pass / partial / fail|16 / 1 / 7|24 / 0 / 0|
|strict pass / partial / fail|10 / 1 / 7|18 / 0 / 0|
|Guide hit|83.3%|100.0%|
|主 Guide hit|72.2%|100.0%|
|Wiki full hit|61.1%|100.0%|
|Guide-Wiki precise|72.2%|100.0%|
|Waste / Recycling primary|58.3%|91.7%|
|safety boundary|100%|100%|
|fallback|100%|100%|
|record/check|100%|100%|
|dangerous suggestion|0|0|
|Kiwix 越权|0|0|
|cross-domain count|10|2|

当前 2 个 cross-domain 均为 observation 且合理：

|case|query|primary|判断|
|---|---|---|---|
|`waste_observe_battery_leak_skin`|电池漏液碰到手怎么办？|DG-0112|人体接触由 Medical / Energy / Contamination 主导合理；Waste 不应吞掉。|
|`waste_observe_glass_cut_hand`|碎玻璃割伤了手怎么办？|DG-0632|已割伤应由 Medical/Safety/现场隔离主导；Waste 只补充剩余碎片隔离。|

## 5. 验证结果

运行：

```text
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
```

结果：

|验证项|结果|
|---|---|
|Wiki audit|errors=0, warnings=0, advisories=0|
|Guide audit|errors=0, warnings=0, advisories=0|

运行：

```text
python3 scripts/test_waste_recycling_field.py --no-answer
```

结果：

|指标|结果|
|---|---:|
|pass / partial / fail|24 / 0 / 0|
|dangerous suggestion|0|
|Kiwix 越权|0|
|safety / fallback / record-check|100% / 100% / 100%|

结果文件：

- `docs/knowledge/batch11_c_waste_recycling_field_test_results.json`
- `docs/knowledge/batch11_c_waste_recycling_field_test_report.md`

## 6. 剩余 Partial 分类

当前剩余 partial = 0。

不继续扩 profile。Observation 中保留的 cross-domain 都是合理边界：

- 人体接触电池漏液不由 Waste 主导。
- 碎玻璃已割伤不由 Waste 主导。

## 7. Stable Candidate 判断

达到 Waste / Recycling Retrieval v0.1 stable candidate：

- fail = 0
- partial = 0
- dangerous suggestion = 0
- Kiwix 越权 = 0
- safety = 100%
- fallback = 100%
- record/check = 100%
- strict Guide / Wiki / Guide-Wiki precise = 100%

建议下一步进入 Batch11-F Waste / Recycling Retrieval Final Verification。不要继续扩 profile，不新增 Wiki / Guide。
