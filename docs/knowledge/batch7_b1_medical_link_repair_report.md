# Batch7-B1 Medical Guide-Wiki Link Repair Report

## 1. 修复范围

本阶段只修复 Batch7-A 指定的首批 high / critical 医疗相关 Guide evidence chain。

- 新增 Wiki：0
- 新增 Guide：0
- 修改 Wiki 正文：0
- 修改 Guide steps / check / fallback / stop_or_escalate：0
- 修改 Retrieval / Prompt / query profile / top_k / selector limit / ranking / fallback / schema：0
- 更新范围：Guide `related_wiki`、对应 Wiki frontmatter `guide_links`、生成文件 `data/emergency_guides.json`

## 2. 修改的 Guide

| Guide | title | risk_level | 新增 related_wiki |
|---|---|---:|---|
| DG-0002 | 伤者初筛：意识、呼吸、大出血 | critical | medical-bleeding-control-001, medical-bleeding-control-002 |
| DG-0004 | 严重出血：加压包扎与休克观察 | critical | medical-bleeding-control-001, medical-bleeding-control-002 |
| DG-0007 | 烧烫伤：冷却、覆盖、后续观察 | high | medical-burn-care-001, medical-burn-care-002, medical-burn-first-aid-001 |
| DG-0017 | 中暑：转移、降温、补液 | critical | medical-heat-activity-stop-001, medical-oral-rehydration-001, medical-dehydration-001, medical-dehydration-002 |
| DG-0064 | 疑似化学污染：远离、脱外层、冲洗 | critical | hygiene-contamination-zone-001, hygiene-contaminated-clothing-001, hygiene-hygiene-knowledge-001, hygiene-contamination-log-001, hygiene-isolation-supplies-001 |
| DG-0087 | 伤口避开污水：防水覆盖和事后清洗 | high | medical-wound-care-001, medical-wound-care-002, medical-clean-water-001, medical-cross-infection-001, hygiene-contamination-zone-001, hygiene-hygiene-knowledge-001 |
| DG-0112 | 电池漏液处理 | high | hygiene-contamination-zone-001, hygiene-hygiene-knowledge-001, hygiene-contamination-log-001, hygiene-isolation-supplies-001 |

## 3. 修改的 Wiki

仅修改以下 Wiki frontmatter 的 `guide_links`，未修改正文。

- wiki_import/medical/medical-bleeding-control-001.md
- wiki_import/medical/medical-bleeding-control-002.md
- wiki_import/medical/medical-burn-care-001.md
- wiki_import/medical/medical-burn-care-002.md
- wiki_import/medical/medical-burn-first-aid-001.md
- wiki_import/medical/medical-heat-activity-stop-001.md
- wiki_import/medical/medical-oral-rehydration-001.md
- wiki_import/medical/medical-dehydration-001.md
- wiki_import/medical/medical-dehydration-002.md
- wiki_import/medical/medical-wound-care-001.md
- wiki_import/medical/medical-wound-care-002.md
- wiki_import/medical/medical-clean-water-001.md
- wiki_import/medical/medical-cross-infection-001.md
- wiki_import/contamination/hygiene-contamination-zone-001.md
- wiki_import/contamination/hygiene-contaminated-clothing-001.md
- wiki_import/contamination/hygiene-hygiene-knowledge-001.md
- wiki_import/contamination/hygiene-contamination-log-001.md
- wiki_import/contamination/hygiene-isolation-supplies-001.md

## 4. 新增 Guide-Wiki 边

本批新增 26 条双向关系。

| Guide | Wiki |
|---|---|
| DG-0002 | medical-bleeding-control-001 |
| DG-0002 | medical-bleeding-control-002 |
| DG-0004 | medical-bleeding-control-001 |
| DG-0004 | medical-bleeding-control-002 |
| DG-0007 | medical-burn-care-001 |
| DG-0007 | medical-burn-care-002 |
| DG-0007 | medical-burn-first-aid-001 |
| DG-0017 | medical-heat-activity-stop-001 |
| DG-0017 | medical-oral-rehydration-001 |
| DG-0017 | medical-dehydration-001 |
| DG-0017 | medical-dehydration-002 |
| DG-0064 | hygiene-contamination-zone-001 |
| DG-0064 | hygiene-contaminated-clothing-001 |
| DG-0064 | hygiene-hygiene-knowledge-001 |
| DG-0064 | hygiene-contamination-log-001 |
| DG-0064 | hygiene-isolation-supplies-001 |
| DG-0087 | medical-wound-care-001 |
| DG-0087 | medical-wound-care-002 |
| DG-0087 | medical-clean-water-001 |
| DG-0087 | medical-cross-infection-001 |
| DG-0087 | hygiene-contamination-zone-001 |
| DG-0087 | hygiene-hygiene-knowledge-001 |
| DG-0112 | hygiene-contamination-zone-001 |
| DG-0112 | hygiene-hygiene-knowledge-001 |
| DG-0112 | hygiene-contamination-log-001 |
| DG-0112 | hygiene-isolation-supplies-001 |

## 5. 未处理 Guide 清单

以下 Guide 按 Batch7-B1 约束暂不硬修：

- DG-0009 疑似骨折：固定不复位
- DG-0010 头部受伤：24 小时观察
- DG-0011 疑似脊柱伤：少动、固定、轴线搬运
- DG-0013 噎住：背部拍击与腹部冲击
- DG-0014 昏迷但有呼吸：恢复体位
- DG-0015 无反应无正常呼吸：胸外按压
- DG-0016 癫痫发作：保护头部与计时
- DG-0215 误服药初步处理

## 6. 为什么未处理

这些场景目前缺少足够精准的 P0 Wiki evidence。强行链接现有泛医疗、卫生或污染 Wiki，会制造“看起来有来源”的假证据链，违反 `ROOT_CAUSE_FIX_POLICY.md` 中“不强行关联不匹配的 Guide/Wiki”和“不用百科背景替代行动卡”的要求。

后续应在 Batch7-B2 中先补少量 P0 Wiki，再回补 Guide-Wiki 双向关系。

## 7. Audit 结果

已运行：

- `python3 tools/audit_wiki.py`
  - Articles: markdown=871, pocketbase=871, categories=24
  - Issues: errors=0, warnings=0, advisories=0
  - Report: `docs/knowledge/wiki_audit_2026-07-16.md`
- `python3 tools/build_guides.py`
  - Generated 790 Guides
  - Generated 790 Guide Index Items
- `python3 scripts/audit_guides.py`
  - Guides: 790
  - Issues: errors=0, warnings=0, advisories=0
  - Report: `docs/knowledge/guide_audit_2026-07-16.md`

Guide-Wiki 双向一致性只读检查：

- forward_edges=1951
- reverse_edges=1951
- single_forward_without_reverse=0
- single_reverse_without_forward=0
- invalid_guide_id=0
- invalid_wiki_slug=0

## 8. 是否建议进入 Batch7-B2

建议进入 Batch7-B2。

理由：

- Batch7-B1 已完成首批可由现有 Wiki 支撑的 high / critical Guide evidence chain 修复。
- 剩余 high / critical 医疗 Guide 的主要瓶颈不是链接缺失，而是缺少精准 P0 Wiki。
- Batch7-B2 应聚焦少量新增 P0 Wiki：骨折固定、头部伤观察、疑似脊柱伤、噎住、恢复体位、胸外按压、癫痫计时保护、误服药停止线。
- Batch7-C 再统一进入 Medical Retrieval Field Test，避免在 evidence chain 尚未完整前提前调 profile。
