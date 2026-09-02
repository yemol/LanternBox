# Batch14-D Long-Term Records / Knowledge Transfer Retrieval Field Test Report

生成时间：2026-08-24T14:58:40.225143+00:00

## 1. 测试范围

本阶段只测试 Long-Term Records / Knowledge Transfer v0.1 的本地 Retrieval 表现。使用现有 fetcher 选择 top 3 Guide，并按既有 related_wiki 与独立 Wiki 候选合并前 8 条 evidence；不调用 LLM，不生成最终回答。

正式统计为 20 条 strict 与 6 条 observation，另有 3 条不计分跨域边界观察。未新增 Records query profile，未修改 selector、ranking、top_k 或 Guide-Wiki 顺序。

## 2. Fixture / Script

- fixture：`tests/fixtures/long_term_records_knowledge_transfer_field_cases.json`
- script：`scripts/test_long_term_records_knowledge_transfer_field.py`
- results：`docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_results.json`
- report：`docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_report.md`

## 3. Wiki / Guide Audit

- Wiki audit：Markdown/PocketBase=1139/1139；errors=0，warnings=0，advisories=0。
- Guide audit：Guides=844；errors=0，warnings=0，advisories=0。

## 4. Retrieval Contract

- `tests/test_retrieval_traceability.py` + `tests/test_retrieval_root_contract.py`：9 passed。
- 未新增 Records profile contract test，未修改既有 tests。

## 5. 26 条 Field Test 总表

- total：26；strict / observation：20 / 6
- pass / partial / fail：23 / 3 / 0

|case|type|verdict|top1 Guide|Wiki any|Records primary|lifecycle|version|promotion|reason|
|---|---|---|---|---|---|---|---|---|---|
|S01|strict|pass|DG-0912|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S02|strict|pass|DG-0912|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S03|strict|pass|DG-0912|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S04|strict|pass|DG-0912|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S05|strict|pass|DG-0911|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S06|strict|pass|DG-0913|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S07|strict|pass|DG-0913|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S08|strict|pass|DG-0914|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S09|strict|pass|DG-0914|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S10|strict|pass|DG-0914|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S11|strict|pass|DG-0916|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S12|strict|pass|DG-0912|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S13|strict|pass|DG-0915|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S14|strict|pass|DG-0916|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S15|strict|pass|DG-0916|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S16|strict|pass|DG-0916|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S17|strict|pass|DG-0917|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S18|strict|pass|DG-0917|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S19|strict|pass|DG-0400|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|S20|strict|pass|DG-0918|True|True|True|True|True|预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。|
|O01|observation|pass|DG-0010|False|False|True|True|True|当前业务域正确主导，Records 未抢主位。|
|O02|observation|pass|DG-0873|False|False|True|True|True|当前业务域正确主导，Records 未抢主位。|
|O03|observation|partial|DG-0837|False|False|True|True|True|业务域正确主导，但合理的 Records 补充 evidence 未进入。|
|O04|observation|partial|DG-0902|False|False|True|True|True|业务域正确主导，但合理的 Records 补充 evidence 未进入。|
|O05|observation|partial|DG-0910|False|False|True|True|True|业务域正确主导，但合理的 Records 补充 evidence 未进入。|
|O06|observation|pass|DG-0562|False|False|True|True|True|当前业务域正确主导，Records 未抢主位。|

## 6. Strict 结果

- pass / partial / fail：20 / 0 / 0
- Records primary rate：100.0%
- expected Guide hit / top1：100.0% / 100.0%
- Wiki any / full hit：100.0% / 75.0%
- Guide-Wiki precise：100.0%

## 7. Observation 结果

- pass / partial / fail：3 / 3 / 0
- 全量 cross-domain reasonable rate：100.0%
- observation 的 partial 只表示合理 Records 补充 evidence 未进入，不在 Batch14-D 修复。

## 8. Lifecycle Correctness

- lifecycle evidence retrieval correctness：100.0%
- lifecycle applicable cases：4
- lifecycle semantic violation count：0
- RAW auto-promoted count：0
- Unverified -> CURRENT count：0
- single-log -> VERIFIED count：0
- single-experience -> CURRENT count：0

## 9. Version Correctness

- CURRENT / SUPERSEDED / ARCHIVED / rollback correctness：100.0%
- SUPERSEDED used as default count：0
- ARCHIVED used as default count：0
- rollback without revalidation count：0

## 10. Knowledge Promotion Gate

- knowledge promotion gate correctness：100.0%
- 一次成功与个人急救经验都必须保持人工审核、验证和对应 domain reviewer 边界。

## 11. Skill Authorization Boundary

- skill authorization boundary correctness：100.0%
- automatic skill authorization count：0

## 12. Dangerous Suggestion / Kiwix

- dangerous suggestion count：0
- Kiwix override count：0

## 13. Safety / Fallback / Record-Check

- safety：100.0%
- fallback：100.0%
- record/check：100.0%

## 14. 额外跨域边界观察

|case|verdict|top1 Guide|Records primary|Wiki any|notes|
|---|---|---|---|---|---|
|X-A|fail|DG-0695|False|False|预期主域=medical,records；实际 top1 主域=other；辅助标签=other；Records top1=False；Records 补充=False。|
|X-B|pass|DG-0914|True|True|预期主域=records,repair；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。|
|X-C|pass|DG-0914|True|True|预期主域=agriculture,records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。|

## 15. 主要失败模式

- R. 合理 observation，不修：6

根因在本阶段只分类，不修改 Knowledge、Evidence 或 Retrieval。

## 16. 是否建议进入 Batch14-E Root Cause Review

当前不要求进入 Root Cause Review；继续观察即可。本阶段仍不新增 Records profile。

## 17. 逐条结果

### S01

- type：strict
- query：我刚记录了一件现场异常，最少应该写哪些信息？
- focus：原始现场记录最小字段，Records 主导。
- expected Guide：DG-0911、DG-0912
- expected Wiki any：general-event-log-001、records-lifecycle-state-model-001、records-raw-record-minimum-fields-001
- actual top Guides：DG-0912、DG-0911
- actual selected Wiki：records-lifecycle-state-model-001、records-raw-record-minimum-fields-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001、records-contradictory-evidence-handling-001、general-record-trust-label-001、records-medical-observation-handover-boundary-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S02

- type：strict
- query：这条记录是别人告诉我的，不是我亲眼看到的，应该怎么标？
- focus：亲历、转述与设备来源边界，Records 主导。
- expected Guide：DG-0912
- expected Wiki any：general-record-trust-label-001、records-source-direct-reported-boundary-001
- actual top Guides：DG-0912、DG-0911
- actual selected Wiki：records-lifecycle-state-model-001、records-raw-record-minimum-fields-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001、records-contradictory-evidence-handling-001、general-record-trust-label-001、records-medical-observation-handover-boundary-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S03

- type：strict
- query：设备自动记录了一个异常值，可以直接当成已经确认的事实吗？
- focus：设备 metadata 不等于事实，不能自动 VERIFIED。
- expected Guide：DG-0912
- expected Wiki any：records-lifecycle-state-model-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001
- actual top Guides：DG-0912、DG-0911
- actual selected Wiki：records-lifecycle-state-model-001、records-raw-record-minimum-fields-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001、records-contradictory-evidence-handling-001、general-record-trust-label-001、records-medical-observation-handover-boundary-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S04

- type：strict
- query：两份记录对同一件事说法不一样，应该删掉哪一份？
- focus：冲突记录并列保留，不通过删除制造一致。
- expected Guide：DG-0912
- expected Wiki any：records-contradictory-evidence-handling-001、records-review-verification-checklist-001
- actual top Guides：DG-0912、DG-0911
- actual selected Wiki：records-lifecycle-state-model-001、records-raw-record-minimum-fields-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001、records-contradictory-evidence-handling-001、general-record-trust-label-001、records-medical-observation-handover-boundary-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S05

- type：strict
- query：今天发生了很多事，我想整理一份跨领域的日终摘要，应该保留什么？
- focus：每日跨域摘要，Records 主导。
- expected Guide：DG-0911
- expected Wiki any：general-event-log-001、records-daily-cross-domain-summary-001、records-raw-record-minimum-fields-001
- actual top Guides：DG-0911、DG-0912
- actual selected Wiki：records-daily-cross-domain-summary-001、records-raw-record-minimum-fields-001、general-event-log-001、education-paper-notebook-001、records-lifecycle-state-model-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S06

- type：strict
- query：一个任务失败了，现场已经安全，之后应该怎么做失败复盘？
- focus：现场安全后的失败复盘，Records 主导。
- expected Guide：DG-0295、DG-0913
- expected Wiki any：general-failure-lesson-record-001、records-corrective-action-verification-001、records-failure-cause-classification-001
- actual top Guides：DG-0913、DG-0295、DG-0880
- actual selected Wiki：records-contradictory-evidence-handling-001、records-failure-cause-classification-001、records-corrective-action-verification-001、records-incident-near-miss-review-template-001、general-failure-lesson-record-001、agriculture-continuous-nursery-schedule-001、agriculture-seedling-damping-off-isolation-001、agriculture-transplant-hardening-off-001
- profiles triggered：records_post_event_review
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S07

- type：strict
- query：刚才发生了一次险情，但已经处理完并恢复安全，怎么记录原因和整改？
- focus：near miss 上层复盘，不回到现场 Safety 操作。
- expected Guide：DG-0913
- expected Wiki any：records-corrective-action-verification-001、records-incident-near-miss-review-template-001
- actual top Guides：DG-0913、DG-0295
- actual selected Wiki：records-contradictory-evidence-handling-001、records-failure-cause-classification-001、records-corrective-action-verification-001、records-incident-near-miss-review-template-001、general-failure-lesson-record-001
- profiles triggered：records_post_event_review
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S08

- type：strict
- query：同一个设备三个月里坏了四次，我想从历史记录判断是不是重复故障，怎么整理？
- focus：维修历史趋势由 Records 主导，Repair 可 secondary。
- expected Guide：DG-0914
- expected Wiki any：records-maintenance-history-trend-001、records-pending-hypothesis-next-test-001、records-repeated-problem-trend-review-001
- actual top Guides：DG-0914
- actual selected Wiki：records-applicable-conditions-context-001、records-success-review-minimum-001、records-repeated-problem-trend-review-001、records-pending-hypothesis-next-test-001、records-corrective-action-verification-001、records-maintenance-history-trend-001、records-production-batch-long-term-review-001、records-agriculture-season-comparison-001
- profiles triggered：records_long_term_trend
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S09

- type：strict
- query：过去三季种同一种作物都失败了，我想比较天气、地点和批次记录找规律。
- focus：Records 与 Agriculture 合理共域，不把相关性写成确定原因。
- expected Guide：DG-0914
- expected Wiki any：records-agriculture-season-comparison-001、records-applicable-conditions-context-001、records-repeated-problem-trend-review-001
- actual top Guides：DG-0914、DG-0902
- actual selected Wiki：records-applicable-conditions-context-001、records-success-review-minimum-001、records-repeated-problem-trend-review-001、records-pending-hypothesis-next-test-001、records-corrective-action-verification-001、records-maintenance-history-trend-001、records-production-batch-long-term-review-001、records-agriculture-season-comparison-001
- profiles triggered：storage_inventory_handover、records_long_term_trend
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=agriculture,records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S10

- type：strict
- query：我想比较过去几个月电池可用容量和太阳能产出的变化趋势。
- focus：Records 与 Energy 合理共域，不提供危险电池维修。
- expected Guide：DG-0914
- expected Wiki any：records-applicable-conditions-context-001、records-energy-capacity-trend-001、records-repeated-problem-trend-review-001
- actual top Guides：DG-0914
- actual selected Wiki：records-applicable-conditions-context-001、records-success-review-minimum-001、records-repeated-problem-trend-review-001、records-pending-hypothesis-next-test-001、records-corrective-action-verification-001、records-maintenance-history-trend-001、records-production-batch-long-term-review-001、records-agriculture-season-comparison-001
- profiles triggered：records_long_term_trend
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=energy,records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S11

- type：strict
- query：这个办法今天第一次成功了，可以直接把它写进正式 SOP 吗？
- focus：一次成功不能直接进入正式 SOP 或 CURRENT。
- expected Guide：DG-0915、DG-0916
- expected Wiki any：records-high-risk-knowledge-review-gate-001、records-knowledge-promotion-gate-001、records-lifecycle-state-model-001
- actual top Guides：DG-0916、DG-0915、DG-0115
- actual selected Wiki：records-lifecycle-state-model-001、records-sop-version-minimum-fields-001、records-current-version-selection-001、records-superseded-version-isolation-001、records-rollback-revalidation-001、records-high-risk-knowledge-review-gate-001、records-version-change-log-001、general-knowledge-card-archive-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S12

- type：strict
- query：我刚写完一条日志，能不能直接把状态设成 VERIFIED？
- focus：一次日志不得自动晋升 VERIFIED。
- expected Guide：DG-0912、DG-0915
- expected Wiki any：records-knowledge-promotion-gate-001、records-lifecycle-state-model-001、records-review-verification-checklist-001
- actual top Guides：DG-0912、DG-0911
- actual selected Wiki：records-lifecycle-state-model-001、records-raw-record-minimum-fields-001、records-review-verification-checklist-001、records-source-direct-reported-boundary-001、records-applicable-conditions-context-001、records-contradictory-evidence-handling-001、general-record-trust-label-001、records-medical-observation-handover-boundary-001
- profiles triggered：records_capture_trust_summary
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S13

- type：strict
- query：我自己试了一次新的急救方法感觉有效，能直接加入正式知识库吗？
- focus：高风险经验由 Records 执行晋升门禁，Medical 只作领域 reviewer。
- expected Guide：DG-0915
- expected Wiki any：records-high-risk-knowledge-review-gate-001、records-knowledge-promotion-gate-001
- actual top Guides：DG-0915、DG-0210、DG-0710
- actual selected Wiki：records-lifecycle-state-model-001、records-applicable-conditions-context-001、records-knowledge-promotion-gate-001、records-high-risk-knowledge-review-gate-001、general-knowledge-card-archive-001、medical-medication-storage-check-001、safety-route-planning-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S14

- type：strict
- query：同一个 SOP 有好几个版本，我现在应该默认用哪个版本？
- focus：只有 CURRENT 默认指导行动。
- expected Guide：DG-0916
- expected Wiki any：records-current-version-selection-001、records-sop-version-minimum-fields-001
- actual top Guides：DG-0916、DG-0915、DG-0277
- actual selected Wiki：records-lifecycle-state-model-001、records-sop-version-minimum-fields-001、records-current-version-selection-001、records-superseded-version-isolation-001、records-rollback-revalidation-001、records-high-risk-knowledge-review-gate-001、records-version-change-log-001、general-knowledge-card-archive-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S15

- type：strict
- query：我找到一份 SUPERSEDED 的旧 SOP，还可以直接照着执行吗？
- focus：SUPERSEDED 不得作为默认行动版本。
- expected Guide：DG-0916
- expected Wiki any：records-current-version-selection-001、records-superseded-version-isolation-001
- actual top Guides：DG-0916、DG-0915、DG-0556
- actual selected Wiki：records-lifecycle-state-model-001、records-sop-version-minimum-fields-001、records-current-version-selection-001、records-superseded-version-isolation-001、records-rollback-revalidation-001、records-high-risk-knowledge-review-gate-001、records-version-change-log-001、general-knowledge-card-archive-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S16

- type：strict
- query：新 SOP 有问题，能不能直接把旧版本改回 CURRENT？
- focus：回滚必须形成新 CURRENT version 并重新验证。
- expected Guide：DG-0916
- expected Wiki any：records-current-version-selection-001、records-rollback-revalidation-001、records-version-change-log-001
- actual top Guides：DG-0916、DG-0803、DG-0915
- actual selected Wiki：records-lifecycle-state-model-001、records-sop-version-minimum-fields-001、records-current-version-selection-001、records-superseded-version-isolation-001、records-rollback-revalidation-001、records-high-risk-knowledge-review-gate-001、records-version-change-log-001、general-knowledge-card-archive-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S17

- type：strict
- query：归档的 ARCHIVED 知识卡还能作为当前操作依据吗？
- focus：ARCHIVED 只用于历史查询。
- expected Guide：DG-0916、DG-0917
- expected Wiki any：general-knowledge-card-archive-001、general-offline-directory-index-001、records-superseded-version-isolation-001
- actual top Guides：DG-0917、DG-0916
- actual selected Wiki：records-superseded-version-isolation-001、records-retention-access-disposal-001、records-medical-observation-handover-boundary-001、education-paper-notebook-001、general-long-term-archive-check-001、general-offline-directory-index-001、education-offline-data-backup-001、general-member-info-backup-001
- profiles triggered：records_archive_index
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S18

- type：strict
- query：SD、纸本和 Core 上都有记录，我怎么做长期索引和备份？
- focus：长期记录归档与索引，Records 主导。
- expected Guide：DG-0917
- expected Wiki any：education-offline-data-backup-001、general-long-term-archive-check-001、general-offline-directory-index-001、records-retention-access-disposal-001
- actual top Guides：DG-0917、DG-0916
- actual selected Wiki：records-superseded-version-isolation-001、records-retention-access-disposal-001、records-medical-observation-handover-boundary-001、education-paper-notebook-001、general-long-term-archive-check-001、general-offline-directory-index-001、education-offline-data-backup-001、general-member-info-backup-001
- profiles triggered：records_archive_index
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S19

- type：strict
- query：一个成员只跟着做过几次，还没有独立完成过，现在应该算什么技能等级？
- focus：不得自动进入 INDEPENDENT。
- expected Guide：DG-0400、DG-0918
- expected Wiki any：records-skill-authorization-minimum-001、records-skill-level-boundaries-001、records-supervised-practice-evidence-001
- actual top Guides：DG-0400、DG-0918、DG-0401
- actual selected Wiki：records-skill-authorization-minimum-001、records-skill-level-boundaries-001、records-supervised-practice-evidence-001、records-skill-expiry-refresher-001、records-new-member-learning-path-001、records-knowledge-transfer-handover-summary-001、general-small-team-handoff-001、organization-onboarding-001
- profiles triggered：无
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=organization,records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### S20

- type：strict
- query：一个人很久没做这个高风险任务了，还能继续按 INDEPENDENT 独立操作吗？
- focus：技能过期后降级并复训，不自动保持长期授权。
- expected Guide：DG-0918
- expected Wiki any：records-skill-authorization-minimum-001、records-skill-expiry-refresher-001、records-skill-level-boundaries-001
- actual top Guides：DG-0918、DG-0689、DG-0394
- actual selected Wiki：records-skill-authorization-minimum-001、records-skill-level-boundaries-001、records-supervised-practice-evidence-001、records-skill-expiry-refresher-001、records-new-member-learning-path-001、records-knowledge-transfer-handover-summary-001、general-small-team-handoff-001、organization-onboarding-001
- profiles triggered：records_skill_evidence
- verdict / reason：pass / 预期 Guide 主导或允许的共主成立，且至少一篇预期 Wiki 进入 evidence。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=records；实际 top1 主域=records；辅助标签=records；Records top1=True；Records 补充=True。
- root cause：无

### O01

- type：observation
- query：有人手臂正在大量流血，现在应该先做什么？
- focus：当前急救必须由 Medical 主导。
- expected Guide：无
- expected Wiki any：无
- actual top Guides：DG-0010、DG-0011、DG-0574
- actual selected Wiki：medical-head-injury-24h-observation-001、medical-spine-injury-movement-boundary-001、hygiene-fecal-oral-001、hygiene-vomit-003
- profiles triggered：medical_trauma_bleeding_fracture
- verdict / reason：pass / 当前业务域正确主导，Records 未抢主位。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=medical；实际 top1 主域=medical；辅助标签=medical；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

### O02

- type：observation
- query：我要做一个木架，木板具体怎么切和连接？
- focus：当前制作必须由 Manufacturing 主导。
- expected Guide：无
- expected Wiki any：无
- actual top Guides：DG-0873
- actual selected Wiki：repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001、repair-manufacturing-wood-measure-mark-001、repair-manufacturing-cut-drill-clear-zone-001、repair-manufacturing-wood-selection-001
- profiles triggered：manufacturing_cutting_clamping
- verdict / reason：pass / 当前业务域正确主导，Records 未抢主位。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=manufacturing；实际 top1 主域=manufacturing；辅助标签=manufacturing,repair,tools；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

### O03

- type：observation
- query：锯子坏了，现在怎么检查和修？
- focus：当前维修由 Repair / Tools 主导，Records 最多补历史。
- expected Guide：无
- expected Wiki any：records-maintenance-history-trend-001
- actual top Guides：DG-0837、DG-0840、DG-0873
- actual selected Wiki：repair-dangerous-tool-temporary-isolation-001、repair-dangerous-tool-borrow-permission-001、repair-sharp-blunt-tool-zone-storage-001、repair-damaged-tool-disable-tag-001、repair-damaged-tool-report-isolation-001、repair-manufacturing-clamp-before-processing-001、repair-manufacturing-wood-saw-support-001、repair-manufacturing-wood-drill-position-001
- profiles triggered：repair_tool_damage_isolation、manufacturing_cutting_clamping
- verdict / reason：partial / 业务域正确主导，但合理的 Records 补充 evidence 未进入。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=repair,tools；实际 top1 主域=repair；辅助标签=repair,tools；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

### O04

- type：observation
- query：储藏区今天换班，库存和异常交接卡应该怎么写？
- focus：当前储藏交接由 Long-Term Storage 主导。
- expected Guide：无
- expected Wiki any：general-long-term-archive-check-001
- actual top Guides：DG-0902、DG-0895
- actual selected Wiki：general-storage-handover-card-001、general-storage-inventory-card-minimum-001、general-storage-check-calendar-001、general-storage-issue-log-001、general-storage-fifo-shelf-rule-001、general-storage-label-minimum-fields-001、agriculture-storage-seed-library-rotation-card-001、agriculture-storage-seed-bag-label-001
- profiles triggered：inventory_consumption、storage_basic_zone_labeling、storage_inventory_handover
- verdict / reason：partial / 业务域正确主导，但合理的 Records 补充 evidence 未进入。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=storage；实际 top1 主域=storage；辅助标签=agriculture,records,storage；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

### O05

- type：observation
- query：下一班要接手工坊里还没完成的半成品，应该怎么交接？
- focus：当前 WIP 交接由 Production System / Workshop 主导。
- expected Guide：无
- expected Wiki any：records-production-batch-long-term-review-001
- actual top Guides：DG-0910、DG-0905
- actual selected Wiki：production-workshop-shift-handover-card-001、production-workshop-tool-state-handover-001、production-workshop-next-action-handover-001、production-workshop-wip-handover-tag-001、production-workshop-unfinished-item-tag-001、production-workshop-task-board-minimum-001、production-workshop-end-of-day-status-log-001、production-workshop-fatigue-stop-line-001
- profiles triggered：inventory_consumption、production_shift_handover
- verdict / reason：partial / 业务域正确主导，但合理的 Records 补充 evidence 未进入。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=production；实际 top1 主域=production；辅助标签=production,repair,tools；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

### O06

- type：observation
- query：今天电量不够，哪些设备先停，电池怎么分配？
- focus：当前能源调度由 Energy Management 主导。
- expected Guide：无
- expected Wiki any：无
- actual top Guides：DG-0562、DG-0845、DG-0859
- actual selected Wiki：energy-battery-capacity-002、energy-critical-device-minimum-line-001、energy-essential-load-reserve-001、energy-low-battery-power-window-001、energy-night-lighting-runtime-plan-001、communication-battery-radio-runtime-budget-001、communication-qrp-power-budget-001、communication-qrp-contact-window-discipline-001
- profiles triggered：无
- verdict / reason：pass / 当前业务域正确主导，Records 未抢主位。
- lifecycle / version / promotion / skill：True / True / True / True
- dangerous / Kiwix：无 / False
- safety / fallback / record-check：True / True / True
- cross-domain：预期主域=energy；实际 top1 主域=energy；辅助标签=energy；Records top1=False；Records 补充=False。
- root cause：R. 合理 observation，不修

## 18. 边界声明

本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、PocketBase、`emergency_guides.json`、`guide_index.json`、既有 Retrieval tests、应用代码、FT-02 firmware、sync protocol、`.env` 或 `data/lanternbox.db`；未运行答案生成。
