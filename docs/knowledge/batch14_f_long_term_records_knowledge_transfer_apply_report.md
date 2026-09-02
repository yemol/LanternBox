# Batch14-F Long-Term Records / Knowledge Transfer Retrieval Minimal Apply Report

生成日期：2026-08-24

依据：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch14_e_long_term_records_knowledge_transfer_root_cause_review.md`

## 1. Apply 范围与结论

本阶段只实施 Batch14-E 确认的最小修复：新增 5 个复合 Records query profiles，修正 4 个 Field Test fixture / co-primary 判定、测试层 Guide ownership 和 lifecycle 指标，并新增对应 profile contract tests。

未新增或修改 Wiki、Guide、Guide-Wiki 关系；未修改旧 stable domain profiles、Retrieval Pipeline、Prompt、ranking、top_k、selector、fallback、evidence cap、schema、PocketBase、应用代码或 FT-02。

最终结果达到 **Long-Term Records / Knowledge Transfer Retrieval v0.1 stable candidate**：

- strict：20 pass / 0 partial / 0 fail
- observation：3 pass / 3 partial / 0 fail
- dangerous suggestion / Kiwix override：0 / 0
- lifecycle evidence retrieval：100.0%
- lifecycle semantic violation：0

## 2. 新增 5 个 Records Profiles

全部新增于 `data/retrieval_query_profiles.json`，Records profiles 总数严格为 5。

|profile|主 Guide|复合入口|主要保护边界|
|---|---|---|---|
|`records_capture_trust_summary`|`DG-0912`；日终摘要分支 `DG-0911`|最小字段、亲历/转述、设备来源、verification、冲突记录、跨域日终摘要|不靠“记录/日志/今天”单词触发；保护 Communication、Medical、Storage、Production、Energy|
|`records_post_event_review`|`DG-0913`；允许旧 `DG-0295`|“已经安全/处理完/恢复安全/事后”与失败、near miss、原因、整改、关闭验证组合|正在发生的 Medical、Fire、Safety、Production、Energy 事件不被接管|
|`records_long_term_trend`|`DG-0914`|多次、跨月/跨季、历史、跨批次与比较、找规律、长期趋势组合|当前 Repair、Agriculture、Energy、Production、Manufacturing 行动保持业务域优先|
|`records_archive_index`|`DG-0917`；secondary `DG-0916`|ARCHIVED、历史版本、长期索引、SD/纸本/Core、多介质和离线归档|不建立宽泛 version profile；CURRENT、SUPERSEDED、rollback 继续走既有入口|
|`records_skill_evidence`|`DG-0918`|技能过期、复训、SUPERVISED/INDEPENDENT、授权 evidence 与独立资格复核|“高风险任务/疲劳/成员”单词不触发；保护 Team Safety、Organization、Tools、Production|

没有新增第 6 个 profile，也没有调整既有 stable profiles。

## 3. Fixture / Co-primary 修正

只修改 `tests/fixtures/long_term_records_knowledge_transfer_field_cases.json` 中 Batch14-E 已确认的 4 条：

- S06：接受 `DG-0295` 或 `DG-0913`，并继续要求 failure-review Wiki evidence。实际为 `DG-0913` top1、`DG-0295` top2。
- S10：允许 Energy top1，但必须 `DG-0914` 进入 top3 且带入 Records trend Wiki。实际为 `DG-0914` top1，条件仍被完整验证。
- S11：接受 `DG-0915` 或 `DG-0916` top1；当 `DG-0916` top1 时要求 `DG-0915` 或 promotion evidence。实际为 `DG-0916` top1、`DG-0915` top2，promotion boundary 正确。
- S19：接受 `DG-0400` top1，但要求 `DG-0918` top3、skill evidence 和 authorization boundary。实际为 `DG-0400` top1、`DG-0918` top2，所有支撑条件成立。

## 4. Field Test Ownership 修正

修正仅存在于 `scripts/test_long_term_records_knowledge_transfer_field.py` 的测试 ownership / metrics 层，没有修改生产 Retrieval metadata：

|Guide|测试 ownership|
|---|---|
|`DG-0860`|Communication|
|`DG-0428`|External Observation / Safety|
|`DG-0845`|Energy|
|`DG-0859`|Communication / Energy|
|`DG-0401`|Organization / Training|
|`DG-0902`|Long-Term Storage|
|`DG-0295`|Records|
|`DG-0400`|Organization / Records co-domain|
|`DG-0911` - `DG-0918`|Records|

因此不再因为 Guide 的 secondary `domains` 含 `records` 就自动视为 Records system primary。

## 5. Lifecycle Metric 修正

原 `lifecycle_correctness` 已改名为 `lifecycle_evidence_retrieval_correctness`，并新增：

- `lifecycle_applicable_cases`：4
- `lifecycle_semantic_violation_count`：0

最终：

- lifecycle evidence retrieval correctness：100.0%
- lifecycle semantic violation：0
- RAW auto-promoted：0
- unverified -> CURRENT：0
- single log -> VERIFIED：0
- single experience -> CURRENT：0

该指标现在明确区分“边界 evidence 是否被检索到”和“evidence 是否实际给出违规语义”。

## 6. Profile Contract Tests

新增：

- `tests/test_long_term_records_knowledge_transfer_retrieval_profiles.py`

覆盖：

- 5 个 profile 各自的 positive + negative boundary
- Records profile 数量严格等于 5
- O01 Medical、O02 Manufacturing、O03 Repair、O04 Storage、O05 Production、O06 Energy 的现有 top1 保护
- S10 trend Wiki evidence
- S19 Organization / Records 共域支撑
- X-A 不被 Records profiles 接管

完整合同回归：

```text
31 passed
```

pytest 仅报告 `.pytest_cache` 无写权限 warning，不影响测试结果。

## 7. Batch14-D -> Batch14-F 指标变化

|指标|Batch14-D 基线|Batch14-F|
|---|---:|---:|
|全量 pass / partial / fail|7 / 5 / 14|23 / 3 / 0|
|strict pass / partial / fail|4 / 2 / 14|20 / 0 / 0|
|observation pass / partial / fail|3 / 3 / 0|3 / 3 / 0|
|Records primary rate|35.0%|100.0%|
|expected Guide / accepted co-primary|50.0%|100.0%|
|expected Guide top1 / accepted top1|20.0%|100.0%|
|Wiki any / full|50.0% / 25.0%|100.0% / 75.0%|
|Guide-Wiki precise|35.0%|100.0%|
|cross-domain reasonable|基线未达目标|100.0%|
|lifecycle evidence retrieval|0.0%|100.0%|
|lifecycle semantic violation|0|0|
|version correctness|100.0%|100.0%|
|promotion gate|100.0%|100.0%|
|skill authorization boundary|100.0%|100.0%|
|dangerous suggestion / Kiwix|0 / 0|0 / 0|
|safety / fallback / record-check|100% / 100% / 100%|100% / 100% / 100%|

Batch14-D 基线取自 Batch14-E Root Cause Review；本轮 Field Test 按脚本既有路径重新生成了 Batch14-D results/report 文件。

## 8. Strict / Observation 与跨域边界

Strict 20 条全部 pass。

Observation 正式统计：

- O01 Medical：pass，`DG-0010` top1。
- O02 Manufacturing：pass，`DG-0873` top1。
- O03 Repair / Tools：partial，`DG-0837` top1；未补 Records history evidence，按要求不修。
- O04 Long-Term Storage：partial，`DG-0902` top1；未补 Records evidence，按要求不修。
- O05 Production：partial，`DG-0910` top1；未补 Records evidence，按要求不修。
- O06 Energy：pass，`DG-0562` top1。

Records 没有吞掉 Medical、Manufacturing、Repair、Storage、Production 或 Energy。

## 9. X-A / X-B / X-C

- X-A Medical observation handover：仍为 fail；top3 未命中 Records Guide。按要求保留为 Medical / Records cross-domain debt，本批未修。
- X-B Repair history trend：pass；`records_long_term_trend` 触发并命中 `DG-0914` 与 maintenance trend evidence。没有修改 Repair stable profile。
- X-C Agriculture cross-season trend：pass；`records_long_term_trend` 触发并命中 `DG-0914` 与 Agriculture / Records trend evidence。没有修改 Agriculture stable profile。

X-A 不计入正式 26 条 strict / observation 验收。

## 10. 审计与验证

```text
python3 -m py_compile scripts/test_long_term_records_knowledge_transfer_field.py
PASS

python3 tools/audit_wiki.py
Articles: markdown=1139 pocketbase=1139 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 844
Issues: errors=0 warnings=0 advisories=0

env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
  tests/test_retrieval_traceability.py \
  tests/test_retrieval_root_contract.py \
  tests/test_long_term_records_knowledge_transfer_retrieval_profiles.py
31 passed

python3 scripts/test_long_term_records_knowledge_transfer_field.py --no-answer
strict: 20 / 0 / 0
observation: 3 / 3 / 0
full: 23 / 3 / 0
```

## 11. Stable Candidate 判断

**Long-Term Records / Knowledge Transfer Retrieval v0.1 stable candidate：达到。**

理由：

- strict 20/20，无 partial/fail
- observation 0 fail，且 O03/O04/O05 只保留明确允许的 supplemental evidence partial
- expected Guide / accepted co-primary、Wiki any、Guide-Wiki precise 均为 100%
- lifecycle、version、promotion、skill 边界均达标
- dangerous suggestion、Kiwix override、lifecycle semantic violation 均为 0
- safety、fallback、record-check 均为 100%

不进行第二轮 tuning，不扩 Records profiles。

## 12. 未处理 Cross-domain Debt

按任务边界保留：

1. X-A：Medical observation handover，Medical / Records cross-domain debt。
2. O03：Repair 当前维修正确主导，未补长期维修 Records evidence。
3. O04：Long-Term Storage 当前交接正确主导，未补 Records evidence。
4. O05：Production 当前 WIP 交接正确主导，未补 Records evidence。

这些项目不是 Records Retrieval v0.1 stable candidate 的阻断项。

## 13. Dirty 文件说明

本阶段有意变更：

- `data/retrieval_query_profiles.json`
- `tests/fixtures/long_term_records_knowledge_transfer_field_cases.json`
- `scripts/test_long_term_records_knowledge_transfer_field.py`
- `tests/test_long_term_records_knowledge_transfer_retrieval_profiles.py`
- `docs/knowledge/batch14_f_long_term_records_knowledge_transfer_apply_report.md`

要求的 Field Test 与审计还会按现有工具行为更新：

- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_results.json`
- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_report.md`
- `docs/knowledge/wiki_audit_2026-08-24.md`
- `docs/knowledge/guide_audit_2026-08-24.md`

`data/lanternbox.db` 在 Batch14-F 开始前已经 dirty；本批未处理、未清理、未提交，也没有运行 PocketBase 同步。其他既有 Wiki、Guide、`emergency_guides.json`、`guide_index.json`、FT-02 与无关 dirty 文件均未处理。
