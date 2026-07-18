# LanternBox Batch10-B: Agriculture Second Stage Knowledge Apply Report

本批执行 Agriculture Second Stage 第一阶段知识扩容，目标是把 Planting v0.1 的“能种一次”推进到“小规模长期生产循环”。本批未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 schema；未加入农药配方、化学灭虫剂自制、高复杂农业工程或畜牧深水区内容。

## 1. 新增 Wiki 清单

新增 40 篇 Wiki，均位于 `wiki_import/planting/`，分类为 `种植与食物生产`。

|#|slug|title|priority|risk_level|
|---:|---|---|---|---|
|1|`agriculture-seed-batch-viability-ledger-001`|种子批次与发芽率复测台账|P0|caution|
|2|`agriculture-seed-drying-recheck-001`|种子干燥后的复查|P0|caution|
|3|`agriculture-seed-storage-moisture-failure-001`|种子受潮失效判断|P0|high|
|4|`agriculture-seed-reserve-use-line-001`|留种和食用种子的保底线|P0|caution|
|5|`agriculture-continuous-nursery-schedule-001`|连续育苗排期|P0|caution|
|6|`agriculture-seedling-damping-off-isolation-001`|苗期猝倒和隔离|P0|high|
|7|`agriculture-transplant-hardening-off-001`|移栽前炼苗和缓苗风险|P0|caution|
|8|`agriculture-nursery-capacity-limit-001`|育苗数量和照护上限|P0|caution|
|9|`agriculture-soil-poor-fertility-signs-001`|土壤贫瘠的现场信号|P0|caution|
|10|`agriculture-compost-maturity-second-check-001`|堆肥成熟二次确认|P0|high|
|11|`agriculture-immature-compost-stop-line-001`|未腐熟肥进入食用区停止线|P0|high|
|12|`agriculture-soil-compaction-recovery-001`|土壤板结恢复|P0|caution|
|13|`agriculture-salinity-crust-stop-line-001`|土表盐壳和盐分风险|P0|high|
|14|`agriculture-raised-bed-drainage-recovery-001`|积水地块的高畦恢复|P0|caution|
|15|`agriculture-soil-recovery-log-001`|土壤恢复记录|P0|caution|
|16|`agriculture-pest-early-scouting-routine-001`|虫害早期巡查流程|P0|caution|
|17|`agriculture-egg-larvae-leaf-back-check-001`|叶背虫卵和幼虫检查|P0|caution|
|18|`agriculture-manual-pest-removal-record-001`|人工除虫后的复查记录|P0|caution|
|19|`agriculture-diseased-tool-zone-separation-001`|带病害工具分区|P0|high|
|20|`agriculture-unknown-chemical-plot-stop-001`|不明化学污染地块停用|P0|high|
|21|`agriculture-manure-compost-food-zone-boundary-001`|粪肥与食用地块边界|P0|high|
|22|`agriculture-kitchen-waste-compost-boundary-001`|厨余堆肥进入种植区边界|P0|high|
|23|`agriculture-crop-family-rotation-card-001`|作物家族轮作卡|P1|normal|
|24|`agriculture-heavy-light-feeder-rotation-001`|高耗肥和低耗肥作物轮作|P1|normal|
|25|`agriculture-quick-leaf-long-cycle-balance-001`|快收叶菜和长周期作物搭配|P1|normal|
|26|`agriculture-seasonal-planting-calendar-001`|多季种植日历|P1|normal|
|27|`agriculture-staple-crop-small-plot-priority-001`|小地块粮食作物优先级|P1|caution|
|28|`agriculture-production-failure-backup-plan-001`|生产失败备用计划|P1|caution|
|29|`agriculture-harvest-drying-rack-check-001`|收获晾晒架检查|P1|caution|
|30|`agriculture-postharvest-drydown-record-001`|收获后干燥记录|P1|caution|
|31|`agriculture-seed-food-harvest-separation-001`|留种批次与食用批次分离|P1|caution|
|32|`agriculture-storage-container-dry-clean-check-001`|储藏容器干燥清洁检查|P1|caution|
|33|`agriculture-rodent-insect-storage-barrier-001`|储粮鼠虫防护|P1|caution|
|34|`agriculture-mold-batch-discard-line-001`|霉变批次丢弃边界|P1|high|
|35|`agriculture-root-crop-curing-basic-001`|根茎作物短期熟化和储藏|P1|normal|
|36|`agriculture-annual-garden-plan-card-001`|家庭菜地年度计划卡|P2|normal|
|37|`agriculture-yield-record-minimum-fields-001`|产量记录最小字段|P2|normal|
|38|`agriculture-seed-library-box-index-001`|小型种子库盒索引|P2|normal|
|39|`agriculture-water-fertility-budget-card-001`|水肥预算卡|P2|caution|
|40|`agriculture-team-garden-task-handover-001`|菜地任务交接|P2|normal|

## 2. 新增 Guide 清单

新增 8 个 Guide，均位于 `data/guides/planting/`。

|Guide|title|priority|risk_level|related_wiki 数量|
|---|---|---|---|---:|
|DG-0879|种子保存与发芽率复测|P0|high|10|
|DG-0880|连续育苗与失败复盘|P0|high|8|
|DG-0881|土壤贫瘠与板结恢复判断|P0|caution|9|
|DG-0882|堆肥成熟和未腐熟风险判断|P0|high|7|
|DG-0883|病虫害早期隔离与工具分流|P0|high|12|
|DG-0884|轮作和多季种植计划|P1|caution|12|
|DG-0885|收获后晾晒防霉与储藏|P1|high|10|
|DG-0886|小规模粮食生产优先级|P1|caution|10|

所有 Guide 均包含 `scenario`、`steps`、`check`、`stop_or_escalate`、`fallback`、`related_wiki` 和 `risk_level`。高风险 Guide 均包含停止、隔离、禁用或暂停边界。

## 3. Guide-Wiki 关系

本批新增 Guide 的 `related_wiki` 总数为 78 条。

关系设计原则：

- 新增 Wiki 默认直接挂到对应新 Guide。
- 对已有 Wiki 只做精准补链，例如种子保存、育苗、土壤、堆肥、病虫害、采收和记录相关条目。
- 未把 Food safety 完全并入 Agriculture。
- 未把 Hygiene / Contamination 的人身污染内容硬并入 Agriculture。
- 未把 Manufacturing 的晾晒架制作硬并入 Agriculture。

补充精准关联的已有 Wiki 包括：

- 种子与留种：`agriculture-seed-storage-001`、`agriculture-seed-germination-test-001`、`agriculture-seed-selection-for-saving-001`、`agriculture-seed-saving-001`
- 育苗：`agriculture-seedling-001`、`agriculture-seedling-transplant-001`、`agriculture-seedling-thinning-001`、`agriculture-planting-failure-review-001`
- 土壤：`agriculture-soil-texture-field-check-001`、`agriculture-soil-drainage-test-001`、`agriculture-soil-fertility-basics-index-001`
- 堆肥与污染边界：`agriculture-compost-001`、`agriculture-compost-pile-setup-001`、`agriculture-manure-growing-zone-separation-001`
- 病虫害：`agriculture-plant-pests-001`、`agriculture-manual-pest-removal-001`、`agriculture-crop-disease-isolation-001`、`agriculture-diseased-residue-separation-001`、`agriculture-contaminated-plot-ban-record-001`
- 轮作与记录：`agriculture-crop-rotation-basic-001`、`agriculture-seasonal-planting-plan-001`、`agriculture-growing-record-handover-001`、`agriculture-growing-log-001`
- 收获后处理：`agriculture-harvest-maturity-check-001`、`agriculture-post-harvest-mold-prevention-001`、`agriculture-harvest-001`
- 小规模粮食优先级：`agriculture-local-crop-profile-001`、`agriculture-planting-inventory-link-001`、`agriculture-seed-storage-002`、`agriculture-expectation-001`

Guide-Wiki 双向校验结果：

|项目|结果|
|---|---:|
|single_forward_without_reverse|0|
|single_reverse_without_forward|0|
|invalid Guide ID|0|
|invalid Wiki slug|0|

## 4. PocketBase 同步结果

已将新增 Wiki 和相关 guide_links 变更同步到本地 PocketBase SQLite 数据库。

|项目|结果|
|---|---:|
|Markdown wiki 条目|990|
|PocketBase wiki_articles|990|
|PocketBase wiki_categories|24|
|Markdown / PocketBase 内容一致性|通过|

说明：项目当前 audit 以 `pocketbase/pb_data/data.db` 为本地 PocketBase 数据源；本批按 Markdown frontmatter 的实际 category upsert，保持分类一致。

## 5. Audit 结果

已运行：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

|验证|结果|
|---|---|
|Wiki audit|errors=0, warnings=0, advisories=0|
|Guide build|Generated 812 Guides / 812 Guide Index Items|
|Guide audit|errors=0, warnings=0, advisories=0|

生成/更新：

- `data/emergency_guides.json`
- `data/guide_index.json`
- `docs/knowledge/wiki_audit_2026-07-18.md`
- `docs/knowledge/guide_audit_2026-07-18.md`

## 6. 未覆盖内容

本批仍未覆盖或仅建立边界：

- 高复杂育种和品种改良。
- 大型灌溉系统。
- 商业温室工程。
- 大型农机和机械化农业。
- 畜牧深水区。
- 农药配方和化学灭虫剂自制。
- 食品加工工业化。
- 农业团队排班系统和复杂组织管理。

这些内容不适合作为 Agriculture Second Stage v0.1 的第一批投入方向。

## 7. 下一步 Field Test 建议

建议进入 Batch10-C Agriculture Retrieval Field Test，本批不要提前修 Retrieval。

建议 strict cases：

1. 旧种子发芽率低，能不能直接大面积播种。
2. 种子受潮有霉味，还能不能混回种子库。
3. 留种和食用种子冲突时怎么保底。
4. 苗盘出现倒伏和腐烂，是否还能继续移栽。
5. 连续育苗如何避免采收断档。
6. 土壤板结和长势差，是否应继续施肥。
7. 土表白壳和叶缘焦枯，是否能继续撒灰或施肥。
8. 未腐熟厨余能不能埋到叶菜旁。
9. 粪肥能不能直接进入食用地块。
10. 叶背发现虫卵和幼虫怎么办。
11. 剪过病株的工具能不能继续剪健康苗。
12. 地块有刺鼻味或油膜，能不能种食用作物。
13. 同一小地块下一季怎么轮作。
14. 小地块先种叶菜还是粮食作物。
15. 收获后怎么晾晒防霉。
16. 储藏容器潮湿或有异味还能不能装粮。
17. 霉变批次能不能挑一挑混回主库。
18. 多人轮流照看菜地如何交接任务。

重点观察的跨域竞争：

- Food safety 是否抢收获后防霉和储藏主位。
- Hygiene / Contamination 是否抢堆肥、粪肥和污染地块主位。
- Water 是否抢积水地块和水肥预算主位。
- Manufacturing 是否抢晾晒架和储藏容器主位。
- Tools / Repair 是否抢带病害工具分区主位。

## 结论

Batch10-B 已完成 Agriculture Second Stage 第一阶段 Apply：新增 40 篇 Wiki、8 个行动 Guide，建立 78 条新 Guide 的 evidence 关系，并完成本地 PocketBase 同步、Guide 汇总构建和双审计清零。当前状态适合进入 Batch10-C Field Test，但不应提前修改 Retrieval/profile。
