# Batch7-F Medical Retrieval Final Verification

生成时间：2026-07-16

## 1. Batch7-A 到 Batch7-E 演进总结

| 阶段 | 目标 | 结果 |
|---|---|---|
| Batch7-A | 审计医疗 Guide / Wiki / Evidence 覆盖 | 判断医疗领域主要问题不是内容总量，而是 high / critical Guide 的 Guide-Wiki evidence chain 与 Retrieval 入口稳定性。 |
| Batch7-B1 | Medical Guide-Wiki Link Repair | 修复首批 high / critical Guide 的 existing Wiki 双向证据链，不新增 Wiki / Guide。 |
| Batch7-B2 | Critical Medical P0 Wiki Patch | 为噎住、恢复体位、CPR、癫痫、头伤、脊柱伤、骨折、误服药、化学皮肤/眼暴露补齐少量 P0 Wiki，并完成双向关联。 |
| Batch7-C | Medical Retrieval Field Test | 建立 22 个 medical field cases；初始结果为 `pass / partial / fail = 7 / 10 / 5`，暴露 medical profile 缺失、相邻领域抢主位和局部 related_wiki 顺序问题。 |
| Batch7-D | Medical Retrieval Root Cause Review | 确认 5 个 fail 的目标 Guide / Wiki 均已存在，根因集中在 selected evidence 入口稳定性。 |
| Batch7-E | Medical Retrieval Minimal Apply | 新增少量 medical query profile，最小调整 DG-0064 / DG-0112 / DG-0020 / DG-0559 evidence chain；结果提升到 `19 / 3 / 0`。 |

本阶段 Batch7-F 只做最终验证和状态固化，未修改 Wiki、Guide、Guide-Wiki、profile、Retrieval、Prompt、top_k、selector limit、ranking 或 fallback。

## 2. Medical Retrieval 当前状态

当前状态：Medical Retrieval v0.1 stable。

核心判断：

- high / critical medical Guide 可以稳定进入 selected evidence。
- 本地 Wiki evidence 能随 selected Guide 进入 evidence。
- Kiwix 未越权替代本地 Guide / Wiki。
- hygiene、contamination、water、food、fire、shelter、clothing、energy 等相邻领域未抢走主位。
- safety boundary、fallback、record/check 均保持完整。

## 3. 完整验证结果

### Audit

```text
python3 tools/audit_wiki.py
Articles: markdown=880 pocketbase=880 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 790
Issues: errors=0 warnings=0 advisories=0
```

### Contract Tests

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_medical_retrieval_profiles.py

17 passed
```

### Medical Field Test

```text
python3 scripts/test_medical_field.py --no-answer

total = 22
strict / observation = 16 / 6
pass / partial / fail = 19 / 3 / 0
Guide hit rate = 93.8%
Wiki hit rate = 93.8%
Guide-Wiki precise rate = 93.8%
high / critical Guide hit rate = 93.3%
safety / fallback / record-check = 100.0% / 100.0% / 100.0%
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
```

## 4. Field Test 最终结果

| 指标 | 最终结果 | 验收 |
|---|---:|---|
| fail | 0 | 通过 |
| dangerous suggestion | 0 | 通过 |
| Kiwix 越权 | 0 | 通过 |
| cross domain | 0 | 通过 |
| safety | 100.0% | 通过 |
| fallback | 100.0% | 通过 |
| record/check | 100.0% | 通过 |
| Wiki audit | 0 / 0 / 0 | 通过 |
| Guide audit | 0 / 0 / 0 | 通过 |

## 5. 剩余 partial 分类

| case | 类型 | 当前状态 | 分类 | 处理建议 |
|---|---|---|---|---|
| medical_child_unknown_medicine_no_vomit | strict partial | `DG-0215` / `medical-accidental-medication-ingestion-001` 未进入 selected evidence | 数据缺口 / selector 问题 | 保留观察。若后续处理，应先审查 Field Test 对“不自行催吐”类否定句的 dangerous phrase 判定，避免误判驱动 profile 过拟合。 |
| medical_observe_vomit_caregiver_infection | observation partial | Guide 命中，期望 Wiki 未完全命中 | selector 问题 / 合理 partial | 保留观察。hygiene 与 medical 协同合理，当前 safety / fallback / record-check 已完整。 |
| medical_observe_hypothermia_no_dry_clothes | observation partial | medical 失温 Guide 进入 selected，但低体温专属 Wiki evidence 不够精准 | selector 问题 / 合理 partial | 保留观察。若后续进入内容补丁，应单独评估 medical hypothermia Wiki，而非继续扩 profile。 |

## 6. 是否达到 stable

结论：达到 stable。

建议标记：

```text
Medical Retrieval v0.1 stable
```

理由：

- 完整验证全部通过验收目标。
- Field Test fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- cross domain = 0。
- safety / fallback / record-check 均为 100%。
- 剩余 partial 均为已知 observation / fixture 边界或后续内容评估点，不阻塞 v0.1 stable。

## 7. 冻结建议

建议冻结 Medical Retrieval v0.1：

1. 冻结当前 medical query profile，不继续追加 profile。
2. 冻结当前 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback。
3. 后续不要用 profile 继续追逐剩余 partial。
4. 若进入 Batch7-G，应只做 final fixture interpretation review 或低体温 medical Wiki 专项规划，且必须重新分阶段。
5. Medical 领域后续扩展应优先通过 Field Test 暴露真实缺口后再决策，不直接泛化扩写内容。
