# LanternBox Batch10-F: Agriculture Second Stage Retrieval Final Verification

生成时间：2026-07-18

本阶段只做最终验证和冻结报告。未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、profile、top_k、selector limit、ranking、fallback，未新增知识，未新增测试逻辑。

## 1. Batch10-B -> Batch10-F 演进总结

|阶段|目标|结果|
|---|---|---|
|Batch10-B|新增 Agriculture Second Stage 第一阶段 Wiki / Guide / evidence chain|新增长期农业生产循环知识入口，覆盖种子、育苗、土壤、堆肥、病虫害、轮作、收获后处理和小规模粮食优先级|
|Batch10-C|建立 Field Test，验证 evidence 是否稳定进入 selected|24 cases 中 pass / partial / fail = 20 / 0 / 4；安全、fallback、record/check 均为 100%，dangerous suggestion = 0，Kiwix 越权 = 0|
|Batch10-D|Root Cause Review|确认 4 个 fail 主要来自旧 Planting v0.1、Food / Hygiene / Contamination / Water 等相邻领域抢入口；不是知识缺失，也不是大规模 Guide-Wiki 缺链|
|Batch10-E|Minimal Apply|新增 4 个 agriculture query profile，调整 DG-0879 / DG-0883 少量 related_wiki 顺序，contract tests 通过，Field Test 刷新为 24 / 0 / 0|
|Batch10-F|Final Verification|重新运行 audit、pytest、Field Test，确认 Agriculture Second Stage Retrieval v0.1 stable|

## 2. 最终 Field Test 结果

运行命令：

```bash
python3 scripts/test_agriculture_second_stage_field.py --no-answer
```

结果：

|指标|结果|
|---|---:|
|total|24|
|strict / observation|18 / 6|
|pass / partial / fail|24 / 0 / 0|
|strict pass / partial / fail|18 / 0 / 0|
|Guide hit|100.0%|
|expected Guide hit|94.4%|
|Wiki full hit|100.0%|
|Wiki any hit|100.0%|
|Guide-Wiki precise|100.0%|
|Agriculture Second Stage primary rate|95.8%|
|cross domain|1|

Field Test 输出：

- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_results.json`
- `docs/knowledge/batch10_c_agriculture_second_stage_field_test_report.md`

说明：现有脚本沿用 Batch10-C 输出路径，本次内容为 Batch10-F 重新验证后的最新结果。

## 3. audit 结果

运行命令：

```bash
python3 tools/audit_wiki.py
```

结果：

```text
Articles: markdown=990 pocketbase=990 categories=24
Issues: errors=0 warnings=0 advisories=0
```

运行命令：

```bash
python3 scripts/audit_guides.py
```

结果：

```text
Guides: 812
Issues: errors=0 warnings=0 advisories=0
```

验收：

- Wiki audit：0 / 0 / 0
- Guide audit：0 / 0 / 0

## 4. pytest 结果

运行命令：

```bash
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_agriculture_second_stage_retrieval_profiles.py
```

结果：

```text
18 passed in 2.29s
```

## 5. 安全与越权

|指标|结果|
|---|---:|
|dangerous suggestion|0|
|Kiwix 越权|0|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|

当前唯一 cross-domain observation 是 `agri_observe_moldy_grain_eat`：霉味粮食入口食用判断由 Food safety 主导，Agriculture evidence 作为储粮、留种和批次管理补充。这是合理边界，不需要继续扩 profile。

## 6. 是否达到 stable

达到。

标记：

**Agriculture Second Stage Retrieval v0.1 stable**

判定依据：

1. Field Test fail = 0，partial = 0。
2. strict cases 全部 pass。
3. Guide / Wiki / Guide-Wiki precise 均为 100%。
4. dangerous suggestion = 0。
5. Kiwix 越权 = 0。
6. safety / fallback / record-check 全部 100%。
7. 审计和 retrieval contract tests 全部通过。

## 7. 冻结建议

建议冻结 Agriculture Second Stage Retrieval v0.1：

- 不继续新增 Agriculture profile。
- 不继续新增 Agriculture Wiki / Guide。
- 不调整 top_k、selector limit、ranking 或 fallback。
- 后续如进入 Agriculture v0.2，应另开 Batch，优先围绕更长期的生产系统主题规划，例如种子库年度轮换、作物组合产量评估、土壤恢复年度计划、收获后长期储存与团队交接。
