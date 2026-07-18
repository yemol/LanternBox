# LanternBox Batch11-B: Waste / Recycling Knowledge Apply Report

生成日期：2026-07-18

本阶段执行 Waste / Recycling v0.1 第一阶段知识与行动入口建设。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`，未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 schema；未新增测试逻辑；未写危险化学处理、电池拆解再造、塑料熔融、金属冶炼、专业医疗废物替代或大型垃圾处理工程。

实现说明：当前 `tools/audit_wiki.py` 的 slug domain 白名单尚无 `waste` / `recycling`。为避免修改 schema / audit / profile，本批在新目录 `wiki_import/waste/` 下新增 Waste / Recycling 知识，但 slug 使用现有正式 domain 前缀：`hygiene-*`、`repair-*`、`fire-*`、`agriculture-*`、`general-*`。主能力仍按 Waste / Recycling 报告和 Guide 链组织。

## 1. 新增 Wiki 清单

本批新增 36 篇 Wiki：P0 12 篇，P1 20 篇，P2 4 篇；risk_level 为 high 18、caution 13、normal 5。

|slug|title|priority|risk_level|Guide|
|---|---|---|---|---|
|`hygiene-waste-basic-sorting-isolation-001`|废弃物基础分类与临时隔离|P0|high|DG-0887|
|`hygiene-waste-mixed-trash-stop-line-001`|混合垃圾禁止线|P0|high|DG-0887|
|`general-waste-source-label-minimum-001`|废弃物来源最小标签|P0|caution|DG-0887, DG-0894|
|`hygiene-waste-child-access-control-001`|儿童远离废弃物和材料池|P0|high|DG-0887, DG-0888|
|`hygiene-waste-sharp-glass-temporary-container-001`|碎玻璃和锋利物临时容器|P0|high|DG-0888|
|`hygiene-waste-metal-edge-scrap-isolation-001`|金属边角废料隔离|P0|high|DG-0888, DG-0892|
|`hygiene-waste-battery-leakage-boundary-001`|电池漏液废弃物边界|P0|high|DG-0887, DG-0889|
|`hygiene-waste-unknown-chemical-item-hold-001`|不明化学污染物暂存|P0|high|DG-0887, DG-0889|
|`hygiene-waste-patient-trash-double-bag-zone-001`|病人垃圾双层封存区|P0|high|DG-0889|
|`hygiene-waste-food-rot-wet-isolation-001`|腐败食物和湿垃圾隔离|P0|high|DG-0887, DG-0890|
|`fire-waste-hot-ash-not-trash-001`|热灰不得进入垃圾袋|P0|high|DG-0891|
|`hygiene-waste-contaminated-container-downgrade-001`|污染容器降级和禁用|P0|high|DG-0889, DG-0893|
|`repair-recycling-material-pool-zone-layout-001`|可再利用材料池分区|P1|caution|DG-0892|
|`repair-recycling-material-intake-checklist-001`|材料入池前检查清单|P1|caution|DG-0892|
|`repair-recycling-salvaged-wood-intake-check-001`|废木板入池判断|P1|caution|DG-0892|
|`repair-recycling-metal-sheet-intake-check-001`|金属片入池判断|P1|high|DG-0892, DG-0888|
|`repair-recycling-plastic-container-intake-check-001`|塑料容器入池判断|P1|caution|DG-0893|
|`repair-recycling-fabric-rope-intake-check-001`|布料和绳索入池判断|P1|caution|DG-0892|
|`repair-recycling-fasteners-small-parts-sort-001`|旧螺丝钉和小零件分类|P1|normal|DG-0892, DG-0894|
|`repair-recycling-cleanable-noncleanable-material-001`|可清洁材料和不可再用材料分界|P1|high|DG-0892, DG-0893|
|`repair-recycling-material-downgrade-label-001`|材料降级用途标签|P1|caution|DG-0892, DG-0893, DG-0894|
|`agriculture-waste-kitchen-scrap-before-compost-001`|厨余进堆肥前分拣|P1|high|DG-0890|
|`hygiene-waste-patient-leftover-no-compost-001`|病人剩饭不直接进堆肥|P1|high|DG-0889, DG-0890|
|`hygiene-waste-oil-meat-odor-organic-boundary-001`|油脂肉类和异味厨余边界|P1|caution|DG-0890|
|`agriculture-waste-compost-waiting-bin-distance-001`|堆肥等待桶距离边界|P1|caution|DG-0890|
|`hygiene-waste-organic-pest-odor-daily-check-001`|有机废弃物虫害异味日查|P1|caution|DG-0890, DG-0894|
|`fire-waste-cold-ash-storage-boundary-001`|冷灰保存和丢弃边界|P1|caution|DG-0891|
|`fire-waste-charcoal-residue-reuse-check-001`|炭渣再利用前检查|P1|caution|DG-0891|
|`agriculture-waste-ash-soil-use-interface-001`|灰烬进入土壤前转交判断|P1|high|DG-0891|
|`hygiene-waste-ash-trash-mixing-ban-001`|灰烬混入普通垃圾风险|P1|high|DG-0891|
|`hygiene-waste-temporary-overflow-plan-001`|废弃物临时满溢处理|P1|high|DG-0887|
|`hygiene-waste-recycling-batch-quarantine-001`|可疑回收材料批次隔离|P1|caution|DG-0892|
|`general-recycling-material-pool-ledger-001`|材料池台账最小字段|P2|normal|DG-0894|
|`general-waste-source-hazard-log-001`|废弃物来源和风险记录|P2|normal|DG-0894|
|`general-waste-disposal-handover-card-001`|废弃物处理交接卡|P2|normal|DG-0894|
|`general-waste-reuse-failure-record-001`|回收再利用失败记录|P2|normal|DG-0894, DG-0892|

分类分布：

|category|数量|
|---|---:|
|污染控制 / 隔离 / 清洁分区|14|
|基础制造与材料维修|7|
|信息保存与长期重建|5|
|火源 / 保温 / 通风 / 一氧化碳风险|3|
|种植与食物生产|3|
|卫生|3|
|维修 / 制作 / 替代 / 拆解再利用|1|

## 2. 新增 Guide 清单

本批新增 8 个 Guide，均位于 `data/guides/waste/`。Guide 总数从 812 增至 820。

|Guide|title|priority|risk_level|related_wiki|
|---|---|---|---|---:|
|DG-0887|废弃物基础分类与临时隔离|P0|high|8|
|DG-0888|锋利 / 破碎 / 金属边角废物处理|P0|high|4|
|DG-0889|病人垃圾与污染物分流|P0|high|5|
|DG-0890|厨余和有机物进入堆肥前判断|P0|high|6|
|DG-0891|灰烬与炭渣冷却后分流|P0|high|5|
|DG-0892|可再利用材料进入材料池前检查|P1|normal|11|
|DG-0893|塑料桶 / 容器再利用前判断|P1|normal|4|
|DG-0894|废弃物与材料池记录交接|P2|normal|8|

## 3. Guide-Wiki 双向关系

新增 Guide-Wiki 关系 51 条。所有关系都满足：

- Guide `related_wiki` 指向真实 Wiki slug。
- Wiki `guide_links` 反向包含对应 Guide ID。
- 不存在单边关系。
- 不存在无效 Guide ID。
- 不存在无效 Wiki slug。

能力链覆盖：

|链路|主 Guide|说明|
|---|---|---|
|分类 -> 临时隔离|DG-0887|普通、湿垃圾、污染物、尖锐物、热源后废弃物、可再利用材料先分流|
|尖锐 / 破碎 / 金属边角|DG-0888|硬质容器、包覆、标记、儿童隔离，避免进入软袋或普通材料池|
|病人垃圾 / 污染物|DG-0889|病人垃圾、病人剩饭、污染容器和不明污染物独立分流|
|厨余 -> 堆肥前判断|DG-0890|只负责厨余进入 Agriculture 前的分流，不判断堆肥成熟或入土|
|热灰 -> 冷灰 -> 分流|DG-0891|未冷却前仍由 Fire 停止线主导，冷却后进入 Waste 分流|
|材料池准入|DG-0892|废木、金属、布绳、小零件和待清洁材料进入材料池前检查|
|塑料桶 / 容器再利用|DG-0893|容器来源、异味、残留、用途降级和禁用边界|
|记录 / 交接|DG-0894|材料池台账、废弃物来源、处理交接和回收失败记录|

没有为了补数量把 WASH 的人体卫生、Fire 的余火处理、Agriculture 的堆肥成熟 / 土壤施用、Manufacturing 的加工制作硬并入 Waste。相邻领域只作为边界和转交条件。

## 4. PocketBase 同步结果

已将本批 36 篇新增 Wiki 同步到本地 PocketBase `wiki_articles`。

最终一致性：

|项目|数量|
|---|---:|
|Markdown Wiki|1026|
|PocketBase `wiki_articles`|1026|
|PocketBase `wiki_categories`|24|

同步后 `tools/audit_wiki.py` 校验 Markdown 与 PocketBase 的 title、category、summary、tags、risk_level、status、source 和 content 均一致。

## 5. Audit 结果

运行命令：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

|验证项|结果|
|---|---|
|Wiki audit|errors=0, warnings=0, advisories=0|
|Guide build|Generated 820 Guides; Generated 820 Guide Index Items|
|Guide audit|errors=0, warnings=0, advisories=0|
|Guide-Wiki 单边关系|0|
|无效 Guide ID|0|
|无效 Wiki slug|0|

报告文件：

- `docs/knowledge/wiki_audit_2026-07-18.md`
- `docs/knowledge/guide_audit_2026-07-18.md`

## 6. 未覆盖内容

本批刻意未覆盖：

- 危险化学品处理教程。
- 电池拆解、再造或化学修复。
- 塑料熔融加工。
- 金属冶炼或专业机加工。
- 专业医疗废物处置替代。
- 大型垃圾处理系统。
- 现代城市环卫依赖流程。
- Manufacturing 的具体切割、打孔、连接、承重制作流程。
- Agriculture 的堆肥成熟判断、入土比例或作物施用细节。
- WASH 的人体卫生、病人照护和清洁流程。
- Fire 的室内燃烧、余火扑灭和一氧化碳相关主流程。

## 7. 下一步 Field Test 建议

建议进入 Batch11-C Waste / Recycling Retrieval Field Test。测试只新增 fixture、测试脚本、结果和报告，不提前新增 profile。

建议 strict cases：

1. 混合垃圾里有碎玻璃、湿厨余和病人垃圾，先怎么分。
2. 碎玻璃能不能直接装进塑料袋。
3. 电池漏液物能不能和普通垃圾放一起。
4. 病人剩饭能不能倒进堆肥桶。
5. 热灰看起来没火了能不能倒进垃圾袋。
6. 废木板能不能直接进入材料池。
7. 旧塑料桶有异味还能不能当水桶。
8. 材料池交接至少记录什么。
9. 垃圾点满了但有污染物怎么办。
10. 冷灰能不能撒到菜地。

重点观察 cross-domain：

- WASH / Hygiene 是否完全抢病人垃圾和污染物分流主位。
- Fire 是否完全抢热灰后续分流主位。
- Agriculture 是否完全抢厨余进入堆肥前分流主位。
- Manufacturing / Repair 是否完全抢材料池准入主位。
- Medical / safety 是否抢碎玻璃、金属边角等物品处理主位。

验收目标建议：

- fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety boundary = 100%。
- fallback = 100%。
- record/check = 100%。
- 如出现 partial，只记录进入 Batch11-D Root Cause Review，不提前修 Retrieval 或 profile。
