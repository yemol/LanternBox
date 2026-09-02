# Batch14-G Long-Term Records / Knowledge Transfer Final Verification

生成日期：2026-08-24

权威输入：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch14_b_long_term_records_knowledge_transfer_plan.md`
- `docs/knowledge/batch14_c_long_term_records_knowledge_transfer_apply_report.md`
- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_report.md`
- `docs/knowledge/batch14_e_long_term_records_knowledge_transfer_root_cause_review.md`
- `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_apply_report.md`

## 1. Final Verification Scope

本阶段只进行 Long-Term Records / Knowledge Transfer Retrieval v0.1 最终验证与冻结判断。

除 Field Test 历史 artifact 已被覆盖所需的最小可追溯性修复外，没有修改 Wiki、Guide、Guide-Wiki、fixture、Records profiles、Retrieval Pipeline、Prompt、ranking、top_k、selector、fallback、evidence cap、schema、PocketBase、application code、FT-02、sync protocol、`.env` 或 `data/lanternbox.db`。

本次 Field Test 使用 `--no-answer`，不调用 LLM。

## 2. Batch14-D 历史产物完整性检查

检查结果：**原 Batch14-D artifact 已被 Batch14-F 运行覆盖。**

当前以下文件保存的是 Batch14-F 指标，不是原 Batch14-D baseline：

- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_results.json`
- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_report.md`

当前文件内容为：

- full：23 pass / 3 partial / 0 fail
- strict：20 / 0 / 0
- observation：3 / 3 / 0

原 Batch14-D 已知 baseline 由 Batch14-E Root Cause Review 和 Batch14-F Apply Report 共同证明：

- full：7 pass / 5 partial / 14 fail
- strict：4 / 2 / 14
- observation：3 / 3 / 0

仓库、Git 历史和现有附件中没有找到可恢复的原始 Batch14-D JSON/report。由于原文件未被 Git 跟踪，不能可靠恢复逐字段 artifact；本阶段没有根据汇总数字伪造历史文件。

## 3. 历史覆盖处理

实施了任务允许的唯一修复：

1. Field Test 默认输出改为独立 Batch14-F artifact：
   - `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_results.json`
   - `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_report.md`
2. 脚本新增 `--results-path`、`--report-path` 和 `--artifact-label`，可显式隔离未来 Batch 输出。
3. 生成报告的 artifact 标签、路径和 final profile/contract 描述不再错误写成 Batch14-D。
4. 没有修改任何 case、verdict、fixture 或 Retrieval 行为。

Batch14-G 验证前后，被覆盖的 D 文件 SHA-256 保持不变：

- results：`ed74b8bfb49753f480346837409d6d50ba9b9f0509730a76f3731744c915f870`
- report：`73ddb97472f238e975c2bed6c6efc0a285758708fb5cbf5cd5627d0d82d0f660`

结论：原始 D artifact 无法恢复，但覆盖事实、可信 baseline 来源和当前 final artifact 已明确分离。该缺口属于历史可追溯性债务，不是 Retrieval correctness 问题。

## 4. Knowledge / Guide Counts

最终基线：

|项目|数量|
|---|---:|
|Markdown Wiki|1139|
|PocketBase `wiki_articles`|1139|
|Wiki categories|24|
|Guide|844|
|Records Wiki|32|
|Batch14 Records Guide（DG-0911 至 DG-0918）|8|

冻结对象为 Batch14-C 建立的 32 Records Wiki、8 Records Guide 及其当前 evidence 关系。

## 5. Wiki / Guide Audits

```text
python3 tools/audit_wiki.py
Articles: markdown=1139 pocketbase=1139 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 844
Issues: errors=0 warnings=0 advisories=0
```

Guide audit 明确确认：

- 所有 `related_wiki` 均为真实 Wiki slug。
- Guide-Wiki 前向与反向关系完全对称。
- invalid Guide ID：0。
- invalid Wiki slug：0。
- single forward / reverse：0 / 0。

## 6. Five Records Profiles

确认当前恰好存在以下 5 个 Records profiles：

1. `records_capture_trust_summary`
2. `records_post_event_review`
3. `records_long_term_trend`
4. `records_archive_index`
5. `records_skill_evidence`

没有第 6 个 Records profile。

`data/retrieval_query_profiles.json` 相对 committed baseline 的 diff 只有尾部新增，没有旧 profile 删除或改写；Batch14-G 未修改该文件。旧 stable domain profiles 保持不变。

## 7. Retrieval Contracts

运行：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 \
venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_long_term_records_knowledge_transfer_retrieval_profiles.py
```

结果：

```text
31 passed
```

只有既有 `.pytest_cache` permission warning；没有 fail、xfail 或通过修改测试隐藏失败。

## 8. Final Field Test

运行最终脚本：

```text
python3 scripts/test_long_term_records_knowledge_transfer_field.py --no-answer
```

输出已隔离到 Batch14-F final artifact，没有覆盖 Batch14-D 路径。

结果：

|范围|pass|partial|fail|
|---|---:|---:|---:|
|Strict|20|0|0|
|Observation|3|3|0|
|Full|23|3|0|

O03/O04/O05 是冻结边界明确允许的 partial。

## 9. Expected Guide / Wiki / Precise

|指标|结果|
|---|---:|
|Expected Guide / accepted co-primary|100.0%|
|Expected top1 / accepted top1|100.0%|
|Wiki any|100.0%|
|Wiki full|75.0%|
|Guide-Wiki precise|100.0%|
|Cross-domain reasonable|100.0%|
|Records primary rate（strict）|100.0%|

Wiki full 不是本阶段冻结门槛；Wiki any 与 Guide-Wiki precise 均达到 100%。

## 10. Lifecycle Metrics

|指标|结果|
|---|---:|
|Lifecycle applicable cases|4|
|Lifecycle evidence retrieval correctness|100.0%|
|Lifecycle semantic violation count|0|
|RAW auto-promoted|0|
|Unverified -> CURRENT|0|
|Single log -> VERIFIED|0|
|Single experience -> CURRENT|0|

Lifecycle evidence retrieval 与 semantic violation 已分离统计，没有把 evidence miss 误写成错误知识建议。

## 11. Version / Promotion / Skill

|指标|结果|
|---|---:|
|Version correctness|100.0%|
|Knowledge promotion gate|100.0%|
|Skill authorization boundary|100.0%|
|SUPERSEDED used as default|0|
|ARCHIVED used as default|0|
|Rollback without revalidation|0|
|Automatic skill authorization|0|

CURRENT、SUPERSEDED、ARCHIVED、rollback、SOP promotion 和技能授权边界全部保持。

## 12. Dangerous / Kiwix

- Dangerous suggestion：0
- Kiwix override：0

Kiwix 没有覆盖 Guide/Wiki 的行动证据。

## 13. Safety / Fallback / Record-Check

- Safety：100.0%
- Fallback：100.0%
- Record/check：100.0%

这些数值是 **Retrieval / Guide / Evidence structural metrics**，表示 selected evidence 中存在对应结构与边界；由于使用 `--no-answer`，它们不是 LLM 最终自然语言回答安全率。

## 14. Stable Domain Boundary Regression

|case|应主导领域|实际 top1|结果|
|---|---|---|---|
|O01|Medical|`DG-0010`|pass|
|O02|Manufacturing|`DG-0873`|pass|
|O03|Repair / Tools|`DG-0837`|partial，允许|
|O04|Long-Term Storage|`DG-0902`|partial，允许|
|O05|Production|`DG-0910`|partial，允许|
|O06|Energy|`DG-0562`|pass|

Records 没有吞掉任何当前行动域。

S10：

- Fixture 允许 Energy 合理 top1。
- 本轮实际为 `DG-0914` top1。
- `records_long_term_trend` 命中，Records trend Wiki evidence 正确进入，case pass。

S19：

- `DG-0400` top1。
- `DG-0918` top2。
- Skill evidence、co-primary support 与 authorization boundary 全部正确，case pass。

## 15. X-A / X-B / X-C

- X-A Medical observation handover：仍为 fail；保留为 Medical / Records cross-domain debt，不计入正式 26 条验收，本批未修。
- X-B Repair history trend：pass；`DG-0914` 与 maintenance trend evidence 正确进入。
- X-C Agriculture multi-season trend：pass；`DG-0914` 与 Agriculture / Records trend evidence 正确进入。

没有修改 Repair 或 Agriculture stable profiles。

## 16. Known Cross-Domain Debt

继续保留，不在 Batch14-G 重开：

1. X-A：Medical observation handover，Medical / Records cross-domain debt。
2. `salvaged_wood`：Waste / Manufacturing 旧入口语义漂移。
3. `workshop_injury`：Medical / Workshop injury cross-domain gap。
4. O03/O04/O05：当前业务域正确主导，但 Records supplemental evidence 未进入的允许 partial。

这些债务均不是 Records Retrieval v0.1 冻结阻断项。

## 17. Dirty Files

Batch14-G 有意修改或新增：

- `scripts/test_long_term_records_knowledge_transfer_field.py`：仅历史 artifact 输出路径与标签隔离。
- `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_results.json`
- `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_field_test_report.md`
- `docs/knowledge/batch14_g_long_term_records_knowledge_transfer_final_verification.md`
- Wiki / Guide audit 报告由要求的审计命令重新生成。

Batch14-G 明确未修改：

- `data/retrieval_query_profiles.json`
- Field Test fixture
- Wiki / Guide / Guide-Wiki
- 被覆盖的 Batch14-D artifacts
- `data/lanternbox.db`
- `data/emergency_guides.json`
- `data/guide_index.json`
- FT-02 与其他无关 dirty 文件

`data/lanternbox.db` 和大量无关文件在本阶段开始前已经 dirty；没有处理、清理或提交。

## 18. Final Decision

# Long-Term Records / Knowledge Transfer Retrieval v0.1 STABLE / FROZEN

所有 Retrieval correctness、evidence、lifecycle、version、promotion、skill、安全结构与 stable-domain boundary 验收均通过。

Batch14-D 原始 artifact 缺失是已记录并被隔离的历史可追溯性债务；它不改变 Batch14-E 的可信 baseline 证据，也不影响当前 final verification 结果。

## 19. Freeze Recommendation

正式冻结：

- 32 Records Wiki
- 8 Records Guide
- 5 Records profiles
- 当前 Guide-Wiki evidence
- 当前 fixture / contract baseline

冻结后不再：

- 增加 Records profile
- 调整 `related_wiki`
- 增加 Records Wiki / Guide
- 为 O03/O04/O05 清 partial
- 为 X-A 扩 profile

只有未来独立 Cross-Domain Root Cause Review 证明系统性问题时，才重新评估对应跨域边界。

## 20. v1.0 / v1.5 Route Status

`v1.0 Personal Long-Term Survival Node`：

**COMPLETE / FROZEN**

v1.5 已完成 stable 主链：

- Agriculture Second Stage
- Manufacturing Foundation
- Waste / Recycling
- Long-Term Storage
- Production System / Workshop
- Long-Term Records / Knowledge Transfer

## 21. Next Route

下一主域：

**Batch15 Energy System Deepening Coverage Planning**

Batch14-G 不开始 Batch15 Apply。
