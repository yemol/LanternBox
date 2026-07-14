# Tools & Repair Retrieval Field Test Report

生成时间：2026-07-14T02:42:19.771310+00:00

## 参考文件

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`：已读取。
- `docs/knowledge/batch4_h_tools_repair_expansion_report.md`：当前工作区未发现该文件，测试改以 `wiki_import/repair/` 与 PocketBase 本地数据为准。
- `docs/knowledge/retrieval_pipeline_fix_report.md`：当前工作区未发现该文件。

## 汇总

- 用例总数：10
- pass / partial / fail：10 / 0 / 0
- Batch4-H 新 Wiki 命中率：100.0%
- Guide 命中率：100.0%
- Guide-Wiki 精准组合率：100.0%
- high/caution 安全边界表现：100.0%
- 替代方案 / 降级用途覆盖率：100.0%
- 记录 / 复查建议覆盖率：100.0%
- 外部依赖违规：0
- Kiwix 越权：0

## 目标达成

- pass_gte_70：达成
- fail_eq_0：达成
- batch4h_hit_gte_80：达成
- high_caution_safety_eq_100：达成
- external_eq_0：达成
- kiwix_override_eq_0：达成

## 用例结果

| 用例 | 结论 | 命中 Guide | 命中 Wiki | 新 Wiki | 安全边界 | 降级 | 记录 | 失败原因 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tools_repair_rope_wear_load | pass | DG-0156 简易绳结固定 (normal) | repair-knot-slip-check-001 绳结固定和滑脱检查、repair-rope-full-length-inspection-001 绳索承重前检查、repair-rope-load-limit-001 绳索承重 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_anchor_loose | pass | DG-0833 固定点失效判断 (high) | repair-anchor-point-failure-check-001 固定点失效判断、repair-simple-bracket-load-check-001 简易支架受力检查 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_wire_jacket_tape | pass | DG-0567 电线破皮：临时绝缘和停用判断 (high) | repair-common-material-properties-index-001 常见材料性质索引、repair-electric-shock-001 漏电和短路基础、repair-temporary-insulation-001 临时绝缘材料的边界、repair-wire-jacket-damage-stop-001 电线外皮破损停用边界 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_cracked_bucket_water | pass | DG-0568 水桶漏水：临时修补与降级用途 (high) | repair-container-repair-material-001 容器修补材料、repair-drinking-water-002 容器裂缝和饮用水污染、repair-cracked-container-leak-control-001 破裂容器临时止漏、repair-plastic-bucket-crack-load-reduction-001 塑料桶裂缝减载使用 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_saw_board_fixture | pass | DG-0834 锯切前固定与支撑 (high) | repair-saw-cut-support-001 锯切前固定与支撑、repair-temporary-clamp-fixture-001 简易夹具和临时固定 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_blade_chip_continue | pass | DG-0154 刀具和锐器管理 (caution) | repair-blade-dull-chip-check-001 刀片变钝与崩口判断、repair-knife-use-storage-001 刀具安全使用和收纳 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_used_fastener_reuse | pass | DG-0376 螺丝钉钉子小件库存 (normal) | repair-fastener-washer-storage-001 螺丝钉垫片分类保存、repair-screws-and-nuts-001 螺丝和螺母 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_zip_tie_load | pass | DG-0151 胶带和扎带临时固定 (caution) | repair-basic-knots-index-001 基础绳结索引、repair-zip-tie-light-duty-boundary-001 扎带临时固定边界、repair-tool-substitute-risk-check-001 工具替代品风险判断 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_glue_not_holding | pass | DG-0835 胶水粘接前表面处理 (caution) | repair-glue-surface-prep-001 胶水粘接前表面处理、repair-fabric-tape-tear-reinforcement-001 布料和胶带加固撕裂口 | 是 | 是 | 是 | 是 | 无 |
| tools_repair_tool_handover | pass | DG-0150 工具借用和归还 (caution) | repair-tool-borrow-return-log-001 工具借用归还记录、repair-tool-id-handover-log-001 手工具编号和交接记录、repair-wear-tool-life-log-001 易损工具寿命记录 | 是 | 是 | 是 | 是 | 无 |

## 失败与缺口归因

## 知识缺口列表

本批不修改 Wiki/Guide 正文。若用例未命中但本地 Wiki 已存在，优先视为检索、别名、排序或关联问题，而不是立即补写知识。

## query profile / alias 缺口列表

- 可能存在 alias/domain 缺口的用例：无

## Retrieval 排序缺口列表

- 可能存在排序问题的用例：无

## Guide-Wiki 关联缺口列表

- 可能存在关联或回答覆盖问题的用例：无

## 验证命令

以下命令需在本报告生成后继续执行，并把结果补充到最终说明：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
```

## 下一批最小建议

1. 若预期 Wiki 已存在但未进入 selected evidence，优先定位 Retrieval v2 候选召回与排序，而不是补写重复 Wiki。
2. 若命中 Guide 但未加载对应 Wiki，检查 Guide related_wiki 与 PocketBase slug / ID 合同。
3. 若回答缺少停用、降级或复查建议，检查 Guide/Wiki evidence 是否进入 Prompt，而不是直接改 Prompt 文案。

## 不应继续修复的问题

- 不为了单条测试硬编码 query alias。
- 不通过增大 top_k 掩盖召回策略问题。
- 不新增重复工具维修 Wiki。
- 不让 Kiwix 覆盖本地 Guide/Wiki 行动建议。
- 不修改 Retrieval Pipeline、Prompt 或 query profile，除非下一批明确进入根因修复阶段。
