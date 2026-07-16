# Batch7-B2 Critical Medical P0 Wiki Patch Report

## 1. 新增 Wiki 清单

本阶段新增 9 篇 P0 medical Wiki，均为 `category: 医疗急救`、`risk_level: high`、`status: published`。

| slug | title | priority | risk_level | guide_links |
|---|---|---:|---:|---|
| medical-choking-airway-obstruction-001 | 噎住和气道异物的危险边界 | P0 | high | DG-0013 |
| medical-recovery-position-breathing-observation-001 | 昏迷但有呼吸时的体位和呼吸观察 | P0 | high | DG-0014 |
| medical-cpr-no-normal-breathing-001 | 无反应且无正常呼吸的胸外按压边界 | P0 | high | DG-0015 |
| medical-seizure-protection-timing-001 | 癫痫发作保护与计时边界 | P0 | high | DG-0016 |
| medical-head-injury-24h-observation-001 | 头部受伤后的 24 小时观察 | P0 | high | DG-0010 |
| medical-spine-injury-movement-boundary-001 | 疑似脊柱伤的移动边界 | P0 | high | DG-0011 |
| medical-fracture-immobilize-no-reposition-001 | 疑似骨折固定不复位原则 | P0 | high | DG-0009 |
| medical-accidental-medication-ingestion-001 | 误服药和不明药物的初步边界 | P0 | high | DG-0215 |
| medical-chemical-skin-eye-exposure-001 | 化学物接触皮肤和眼睛的冲洗边界 | P0 | high | DG-0064, DG-0112 |

内容边界：

- 不写药物剂量。
- 不写处方替代。
- 不写侵入性操作教程。
- 不把复杂医学判断写成确定诊断。
- 每篇均包含停止条件、替代方案和记录建议。
- 不依赖现代急救电话或医院作为唯一行动。

## 2. 关联 Guide

本批补齐以下 Guide `related_wiki`：

| Guide | title | 新增 related_wiki |
|---|---|---|
| DG-0013 | 噎住：背部拍击与腹部冲击 | medical-choking-airway-obstruction-001 |
| DG-0014 | 昏迷但有呼吸：恢复体位 | medical-recovery-position-breathing-observation-001 |
| DG-0015 | 无反应无正常呼吸：胸外按压 | medical-cpr-no-normal-breathing-001 |
| DG-0016 | 癫痫发作：保护头部与计时 | medical-seizure-protection-timing-001 |
| DG-0010 | 头部受伤：24 小时观察 | medical-head-injury-24h-observation-001 |
| DG-0011 | 疑似脊柱伤：少动、固定、轴线搬运 | medical-spine-injury-movement-boundary-001 |
| DG-0009 | 疑似骨折：固定不复位 | medical-fracture-immobilize-no-reposition-001 |
| DG-0215 | 误服药初步处理 | medical-accidental-medication-ingestion-001 |
| DG-0064 | 疑似化学污染：远离、脱外层、冲洗 | medical-chemical-skin-eye-exposure-001 |
| DG-0112 | 电池漏液处理 | medical-chemical-skin-eye-exposure-001 |

未修改任何已有 Guide 的 `steps`、`check`、`fallback`、`stop_or_escalate`。

## 3. 双向关系结果

新增 Guide-Wiki 边 10 条：

- DG-0013 <-> medical-choking-airway-obstruction-001
- DG-0014 <-> medical-recovery-position-breathing-observation-001
- DG-0015 <-> medical-cpr-no-normal-breathing-001
- DG-0016 <-> medical-seizure-protection-timing-001
- DG-0010 <-> medical-head-injury-24h-observation-001
- DG-0011 <-> medical-spine-injury-movement-boundary-001
- DG-0009 <-> medical-fracture-immobilize-no-reposition-001
- DG-0215 <-> medical-accidental-medication-ingestion-001
- DG-0064 <-> medical-chemical-skin-eye-exposure-001
- DG-0112 <-> medical-chemical-skin-eye-exposure-001

双向一致性只读检查：

- guides=790
- wiki=880
- forward_edges=1961
- reverse_edges=1961
- single_forward_without_reverse=0
- single_reverse_without_forward=0
- invalid_guide_id=0
- invalid_wiki_slug=0

## 4. PocketBase 同步结果

同步方式：本地 SQLite upsert 到 `pocketbase/pb_data/data.db` 的 `wiki_articles`，只写入新增 Wiki 的 metadata 与 content，未修改 PocketBase schema。

- 新增 `wiki_articles`：9
- 更新 `wiki_articles`：0
- 同步前 `wiki_articles`：871
- 同步后 `wiki_articles`：880
- Markdown Wiki：880
- Markdown / PocketBase 数量一致：是
- PocketBase `wiki_categories`：24

新增记录已确认存在于 PocketBase：

- medical-accidental-medication-ingestion-001
- medical-chemical-skin-eye-exposure-001
- medical-choking-airway-obstruction-001
- medical-cpr-no-normal-breathing-001
- medical-fracture-immobilize-no-reposition-001
- medical-head-injury-24h-observation-001
- medical-recovery-position-breathing-observation-001
- medical-seizure-protection-timing-001
- medical-spine-injury-movement-boundary-001

## 5. Audit 结果

已运行：

- `python3 tools/audit_wiki.py`
  - Articles: markdown=880, pocketbase=880, categories=24
  - Issues: errors=0, warnings=0, advisories=0
  - Report: `docs/knowledge/wiki_audit_2026-07-16.md`
- `python3 tools/build_guides.py`
  - Generated 790 Guides
  - Generated 790 Guide Index Items
- `python3 scripts/audit_guides.py`
  - Guides: 790
  - Issues: errors=0, warnings=0, advisories=0
  - Report: `docs/knowledge/guide_audit_2026-07-16.md`

## 6. 未修改项

本阶段未修改：

- 既有 Wiki 正文
- Guide steps / check / fallback / stop_or_escalate
- Retrieval Pipeline
- Prompt
- query profile
- top_k
- selector limit
- ranking
- fallback
- schema
- tests

本阶段未新增 Guide，未做 Retrieval Field Test。

## 7. 是否建议进入 Batch7-C Medical Retrieval Field Test

建议进入 Batch7-C。

原因：

- Batch7-B1 已修复现有 Wiki 可支撑的 high / critical Guide evidence chain。
- Batch7-B2 已补齐此前明确缺少 P0 Wiki 的 critical / high 医疗入口。
- 当前 Markdown / PocketBase / Guide-Wiki 双向关系均通过 0/0/0 验收。
- 下一步应通过 Medical Retrieval Field Test 验证这些 Guide/Wiki 是否稳定进入 evidence，并观察 hygiene、contamination、water、fire、shelter、Kiwix background 是否抢主位。
