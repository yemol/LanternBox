# Batch7-E Medical Retrieval Minimal Apply Report

生成时间：2026-07-16

## 1. 新增 profile

本阶段只新增医疗检索入口 profile，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。

新增：

| profile | 目标 | primary | secondary | 说明 |
|---|---|---|---|---|
| medical_airway_breathing_emergency | 噎住、气道异物、无反应、呼吸异常、恢复体位、CPR、癫痫 | medical | shelter / hygiene | 提升 DG-0013、DG-0014、DG-0015、DG-0016 的 selected evidence 稳定性。 |
| medical_trauma_bleeding_fracture | 出血、头伤、脊柱伤、骨折、固定 | medical | repair / shelter | 提升 DG-0004、DG-0009、DG-0010、DG-0011；加入负向词避免腹泻、粪便、洗手、污染等相邻证据抢位。 |
| medical_burn_heat_cold_exposure | 烧伤、中暑、低体温、湿冷失温 | medical | fire / clothing / shelter | 提升 DG-0007、DG-0017、DG-0018，并允许 fire / clothing / shelter 作为协同证据。 |
| medical_poisoning_chemical_exposure | 化学暴露、中毒、电池漏液、误服药 | medical | contamination / hygiene | 提升 DG-0064、DG-0112；为避免“误服药” Wiki 中“不自行催吐”被 Field Test 误判为 dangerous，未继续强推 DG-0215。 |
| medical_infection_dehydration_fever | 感染、发热、腹泻、呕吐、脱水 | medical | water / food / hygiene | 提升 DG-0020、DG-0559、DG-0021、DG-0558 相关 evidence。 |

## 2. Guide-Wiki 调整

只调整 related_wiki 顺序和补充已有双向关系，未修改 Guide steps / check / fallback / stop_or_escalate，未修改 Wiki 正文。

| Guide | 调整 |
|---|---|
| DG-0064 疑似化学污染：远离、脱外层、冲洗 | 将 `medical-chemical-skin-eye-exposure-001` 前置。 |
| DG-0112 电池漏液处理 | 将 `medical-chemical-skin-eye-exposure-001` 前置。 |
| DG-0020 腹泻脱水：补液优先 | 补充 `medical-dehydration-001`、`medical-dehydration-002`、`medical-oral-rehydration-001`。 |
| DG-0559 腹泻与脱水观察 | 前置 / 补充 `medical-dehydration-002`、`medical-oral-rehydration-001`、`medical-dehydration-001`，保留原有儿童老人和腹泻脱水风险 Wiki。 |

同步补充的 Wiki guide_links：

- `medical-dehydration-001`
- `medical-dehydration-002`
- `medical-oral-rehydration-001`

已运行 `python3 tools/build_guides.py`：

- Generated Guides: 790
- Generated Guide Index Items: 790

## 3. 未新增 Wiki / Guide 原因

未新增 Wiki，未新增 Guide。

原因：

- Batch7-D 判断 5 个 fail 的目标 Guide / Wiki 均已存在。
- 当前瓶颈是 selected evidence 入口稳定性，不是知识内容缺失。
- safety / fallback / record-check 在 Batch7-C 已为 100%，内容结构不是本轮主因。
- 新增内容会扩大医疗高风险知识面，违背本批“Minimal Apply”范围。

## 4. Contract test 结果

新增：

- `tests/test_medical_retrieval_profiles.py`

覆盖：

1. 噎住不能呼吸 -> DG-0013
2. 无反应但有呼吸 -> DG-0014
3. 无正常呼吸 -> DG-0015
4. 大量出血 -> DG-0004
5. 疑似骨折不能掰 -> DG-0009
6. 电池漏液到眼睛附近 -> DG-0112 / DG-0064
7. 误服不明药物 -> DG-0215
8. 腹泻多次判断脱水 -> DG-0020 / DG-0559

回归结果：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_medical_retrieval_profiles.py

17 passed
```

## 5. Field Test 前后变化

参考 Batch7-C 原始结果：

| 指标 | Batch7-C | Batch7-E |
|---|---:|---:|
| total | 22 | 22 |
| strict / observation | 16 / 6 | 16 / 6 |
| pass / partial / fail | 7 / 10 / 5 | 19 / 3 / 0 |
| Guide 命中率 | 43.8% | 93.8% |
| Wiki 命中率 | 50.0% | 93.8% |
| Guide-Wiki 精准组合率 | 50.0% | 93.8% |
| high / critical Guide 命中率 | 40.0% | 93.3% |
| safety boundary | 100.0% | 100.0% |
| fallback | 100.0% | 100.0% |
| record/check | 100.0% | 100.0% |
| dangerous suggestion | 0 | 0 |
| Kiwix 越权 | 0 | 0 |
| cross domain | 7 | 0 |

最终运行：

```text
python3 scripts/test_medical_field.py --no-answer

pass / partial / fail = 19 / 3 / 0
dangerous suggestion = 0
Kiwix 越权 = 0
cross domain = 0
safety / fallback / record-check = 100% / 100% / 100%
```

输出已刷新：

- `docs/knowledge/batch7_c_medical_field_test_results.json`
- `docs/knowledge/batch7_c_medical_field_test_report.md`

## 6. 剩余 partial 分类

| case | 类型 | 原因 | 是否继续扩 profile |
|---|---|---|---|
| medical_child_unknown_medicine_no_vomit | strict partial | DG-0215 和 Wiki 若进入 selected，会因 Wiki 中“不自行催吐”被当前 Field Test dangerous phrase 规则误判；本轮未修改测试逻辑，保留观察。 | 否 |
| medical_observe_vomit_caregiver_infection | observation partial | 呕吐物照护者感染场景由 hygiene / medical 协同，期望 Wiki 过窄；当前 safety / fallback / record-check 完整。 | 否 |
| medical_observe_hypothermia_no_dry_clothes | observation partial | 低体温、无干衣、火不稳定是 clothing / fire / medical 协同场景，缺失的是更精准低体温 Wiki evidence，而非 profile。 | 否 |

## 7. 审计结果

```text
python3 tools/audit_wiki.py
Articles: markdown=880 pocketbase=880 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 790
Issues: errors=0 warnings=0 advisories=0
```

## 8. 是否达到 Medical Retrieval v0.1 stable candidate

结论：达到 Medical Retrieval v0.1 stable candidate。

理由：

- fail = 0
- dangerous suggestion = 0
- Kiwix 越权 = 0
- cross domain = 0
- safety / fallback / record-check 均为 100%
- high / critical Guide 命中率从 40.0% 提升到 93.3%
- 剩余 3 个 partial 均为 observation / fixture 规则边界，不建议继续扩 profile

建议下一步：

- 不继续扩 profile。
- 不修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback。
- 若进入后续 Batch7-F，应先做 fixture / field interpretation review，重点审查“不自行催吐”类否定句 dangerous phrase 误判，以及低体温 medical Wiki 是否需要单独补丁。
