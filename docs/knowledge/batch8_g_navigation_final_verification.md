# Batch8-G Navigation Retrieval Final Verification

生成日期：2026-07-17

本阶段只做 Navigation Retrieval v0.1 最终验证和报告。未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema，也未新增测试逻辑。

参考：

- `docs/knowledge/batch8_e_navigation_root_cause_review.md`
- `docs/knowledge/batch8_f_navigation_apply_report.md`
- `docs/knowledge/batch8_d_navigation_field_test_report.md`

## 1. Batch8-D -> Batch8-F -> Batch8-G 变化

|阶段|动作|pass / partial / fail|关键状态|
|---|---|---:|---|
|Batch8-D Field Test|验证 Navigation v0.1 初始 Retrieval|12 / 3 / 3|Guide hit 90.0%，Wiki full hit 50.0%，Guide-Wiki precise 70.0%。|
|Batch8-E Root Cause Review|只做根因分析|不适用|确认问题集中在 profile 缺失、相邻领域抢主位、少量 related_wiki 顺序截断。|
|Batch8-F Minimal Apply|新增 navigation profile，调整少量 evidence priority，新增 contract test|18 / 0 / 0|strict cases 全部 pass，dangerous suggestion = 0，Kiwix 越权 = 0。|
|Batch8-G Final Verification|只做最终验证|18 / 0 / 0|复跑 audit、pytest、Navigation Field Test，状态保持稳定。|

## 2. 最终 Field Test 结果

重新运行：

```text
python3 scripts/test_navigation_field.py --no-answer
```

结果文件：

- `docs/knowledge/batch8_d_navigation_field_test_results.json`
- `docs/knowledge/batch8_d_navigation_field_test_report.md`

最终 summary：

|指标|结果|
|---|---:|
|total|18|
|strict / observation|10 / 8|
|pass / partial / fail|18 / 0 / 0|
|strict pass / partial / fail|10 / 0 / 0|
|Guide hit|100.0%|
|主 Guide hit|100.0%|
|Wiki full hit|100.0%|
|Wiki any hit|100.0%|
|Guide-Wiki precise|100.0%|
|navigation primary rate|77.8%|
|cross domain count|4|

说明：剩余 cross domain 全部来自 observation cases，未造成 partial 或 fail。

## 3. Audit 结果

### Wiki audit

重新运行：

```text
python3 tools/audit_wiki.py
```

结果：

```text
Report: docs/knowledge/wiki_audit_2026-07-17.md
Articles: markdown=912 pocketbase=912 categories=24
Issues: errors=0 warnings=0 advisories=0
```

结论：Wiki audit = 0 / 0 / 0。

### Guide audit

重新运行：

```text
python3 scripts/audit_guides.py
```

结果：

```text
Report: docs/knowledge/guide_audit_2026-07-17.md
Guides: 796
Issues: errors=0 warnings=0 advisories=0
```

结论：Guide audit = 0 / 0 / 0。

## 4. Pytest 结果

重新运行：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_navigation_retrieval_profiles.py
```

结果：

```text
15 passed in 4.24s
```

结论：retrieval contract tests 通过。

## 5. Safety / Kiwix / Record 指标

|指标|结果|
|---|---:|
|dangerous suggestion|0|
|Kiwix 越权|0|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|

结论：Navigation Retrieval v0.1 的安全边界、降级方案和记录检查链在当前 field cases 中保持完整。

## 6. 是否达到 Navigation Retrieval v0.1 Stable

结论：达到。

标记：

```text
Navigation Retrieval v0.1 stable
```

判定依据：

- Wiki audit = 0 / 0 / 0。
- Guide audit = 0 / 0 / 0。
- retrieval contract tests 全部通过。
- Navigation Field Test fail = 0。
- strict cases 全部 pass。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety / fallback / record-check 均为 100%。

## 7. 冻结建议

建议冻结 Navigation Retrieval v0.1：

- 不继续新增 navigation profile。
- 不继续新增 Navigation Wiki / Guide。
- 不修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。
- 保留当前 observation cross-domain 作为合理协同观察，不进入修复。
- 后续如进入 Navigation v0.2，应另起阶段，聚焦团队移动、任务路线、FT-02 深度融合和更复杂地图显示，不在 v0.1 stable 范围内继续扩展。
