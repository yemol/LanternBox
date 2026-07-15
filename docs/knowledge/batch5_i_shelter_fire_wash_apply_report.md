# Batch5-I Shelter / Fire / Clothing / WASH Integration Apply Report

生成日期：2026-07-15

本批执行环境与居住核心补洞 Apply。未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、fallback、schema 或测试；未新增 category；未写现代物业/消防/电力依赖建议、复杂烟囱施工、复杂炉具制造、高风险室内燃烧技巧、医疗治疗替代、化学消毒剂配方大全或采购推荐。

## 1. 新增 Wiki 清单

本批新增 35 篇 Wiki。

|方向|slug|title|category|priority|risk_level|Guide|
|---|---|---|---|---|---|---|
|Shelter|shelter-site-selection-weather-exposure-001|临时住所选址的风雨和暴露判断|庇护空间分区|P0|caution|DG-0847|
|Shelter|shelter-rain-leak-first-line-001|漏雨时先保护睡眠区和物资区|庇护空间分区|P0|caution|DG-0847|
|Shelter|shelter-ground-moisture-barrier-001|潮湿地面的隔离层判断|庇护空间分区|P0|caution|DG-0847, DG-0848|
|Shelter|shelter-sleep-heat-loss-ground-001|睡眠区地面失温风险|庇护空间分区|P0|high|DG-0848|
|Shelter|shelter-ventilation-heat-balance-001|保温和通风的冲突平衡|庇护空间分区|P0|high|DG-0848, DG-0850|
|Shelter|shelter-kitchen-fire-sleep-distance-001|厨房火源区和睡眠区距离|庇护空间分区|P0|high|DG-0849|
|Shelter|shelter-daily-habitability-check-001|长期居住点每日可住性检查|庇护空间分区|P1|caution|DG-0847|
|Shelter|shelter-roof-wall-floor-seepage-signs-001|屋顶墙面地面渗水信号|庇护空间分区|P1|caution|DG-0847|
|Fire|fire-before-lighting-site-check-001|生火前场地和周边检查|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0849|
|Fire|fire-dry-wet-fuel-sorting-001|干湿燃料分级和禁用判断|火源 / 保温 / 通风 / 一氧化碳风险|P0|caution|DG-0849|
|Fire|fire-indoor-combustion-no-go-zone-001|室内燃烧禁区和停止线|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0850|
|Fire|fire-carbon-monoxide-suspect-stop-001|疑似一氧化碳时停止取暖|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0850|
|Fire|fire-smoke-backdraft-room-response-001|烟雾反流时的开窗和撤离判断|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0850|
|Fire|fire-ash-ember-cooling-disposal-001|灰烬和余火冷却处理|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|-|
|Fire|fire-temporary-stove-stability-boundary-001|临时炉具稳定性和禁用边界|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0849|
|Fire|fire-children-bystander-clear-zone-001|儿童和旁人远离火源区|火源 / 保温 / 通风 / 一氧化碳风险|P0|high|DG-0849|
|Fire|fire-night-final-extinguish-log-001|夜间火源熄灭记录|火源 / 保温 / 通风 / 一氧化碳风险|P1|caution|-|
|Clothing / PPE|clothing-wet-cold-early-hypothermia-signs-001|湿冷失温早期信号|衣物 / 鞋袜 / 体温防护|P0|high|DG-0848, DG-0852|
|Clothing / PPE|clothing-foot-check-after-wet-work-001|湿作业后的脚部检查|衣物 / 鞋袜 / 体温防护|P0|caution|DG-0852|
|Clothing / PPE|clothing-shoe-sole-failure-outing-stop-001|鞋底损坏后的外出停止线|衣物 / 鞋袜 / 体温防护|P0|caution|DG-0852|
|Clothing / PPE|clothing-layering-work-rest-adjustment-001|干活和休息时的分层调整|衣物 / 鞋袜 / 体温防护|P1|caution|DG-0848|
|Clothing / PPE|clothing-glove-contamination-cut-boundary-001|手套使用和破损污染边界|衣物 / 鞋袜 / 体温防护|P0|high|DG-0852|
|Clothing / PPE|clothing-eye-protection-low-resource-001|低资源护目防护边界|衣物 / 鞋袜 / 体温防护|P1|caution|-|
|Clothing / PPE|clothing-mouth-nose-dust-smoke-limit-001|口鼻防护的粉尘烟雾边界|衣物 / 鞋袜 / 体温防护|P1|high|-|
|Clothing / PPE|clothing-contaminated-laundry-zone-001|污染衣物临时存放区|衣物 / 鞋袜 / 体温防护|P0|high|DG-0852|
|WASH|hygiene-wash-zone-layout-minimum-001|饮水洗手厕所厨房最小分区|污染控制 / 隔离 / 清洁分区|P0|high|DG-0853|
|WASH|hygiene-handwater-priority-table-001|洗手水优先级表|卫生|P0|high|DG-0853|
|WASH|hygiene-bucket-toilet-changeover-001|桶厕更换和封存流程|卫生|P0|high|DG-0853|
|WASH|hygiene-patient-cup-towel-isolation-001|病人杯子毛巾餐具隔离|污染控制 / 隔离 / 清洁分区|P0|high|-|
|WASH|hygiene-kitchen-raw-cooked-contamination-line-001|厨房生熟和污染线|食物|P0|high|-|
|WASH|hygiene-contamination-zone-visible-marking-001|污染区可见标记方法|污染控制 / 隔离 / 清洁分区|P1|caution|-|
|WASH|hygiene-daily-wash-round-checklist-001|每日 WASH 巡查表|卫生|P1|caution|DG-0853|
|WASH|hygiene-wash-abnormal-record-001|卫生异常记录和追溯|卫生|P1|caution|-|
|WASH|hygiene-food-water-toilet-distance-review-001|食物水桶厕所距离复核|污染控制 / 隔离 / 清洁分区|P1|high|DG-0853|
|WASH|hygiene-simple-team-wash-handover-001|小团队 WASH 交接摘要|卫生|P2|normal|DG-0853|

## 2. 新增 Guide 清单

本批新增 6 个 Guide。

|Guide|title|目录|risk_level|related_wiki 数量|
|---|---|---|---|---:|
|DG-0847|临时住所选址与防雨防潮|data/guides/shelter|caution|5|
|DG-0848|睡眠区保温和地面隔离|data/guides/shelter|high|5|
|DG-0849|火源使用前检查|data/guides/fire|high|5|
|DG-0850|室内燃烧和一氧化碳停止线|data/guides/fire|high|5|
|DG-0852|湿冷衣物和脚部保护|data/guides/clothing|high|5|
|DG-0853|小团队 WASH 分区运行|data/guides/hygiene|high|6|

本批暂缓：DG-0851 灰烬与余火处理、DG-0854 病人用品与厨房污染隔离。

## 3. 是否新增 category

未新增 category。使用的正式分类包括：

- 庇护空间分区
- 火源 / 保温 / 通风 / 一氧化碳风险
- 衣物 / 鞋袜 / 体温防护
- 卫生
- 污染控制 / 隔离 / 清洁分区
- 食物

PocketBase `wiki_categories` 数量仍为 24。

## 4. PocketBase 同步数量

|项目|数量|
|---|---:|
|新增 Markdown Wiki|35|
|新增 PocketBase `wiki_articles`|35|
|当前 Markdown Wiki 总数|839|
|当前 PocketBase `wiki_articles` 总数|839|
|Markdown / PocketBase 数量是否一致|是|

同步方式：新增 Markdown Wiki 写入 `wiki_import/*` 后，同步新增记录到 `pocketbase/pb_data/data.db` 的 `wiki_articles`，并写入正文 `content`。

## 5. Guide-Wiki 关系变化

|项目|执行后数量|
|---|---:|
|Wiki -> Guide guide_links 边|1864|
|Guide -> Wiki related_wiki 边|1864|
|Guide-Wiki 单边关系|0|
|无效 Guide ID|0|
|无效 Wiki slug|0|
|新增 Guide related_wiki 边|31|

为保持 DG-0850 与既有 Wiki 的双向关系，本批只对既有 `wiki_import/fire/fire-indoor-combustion-carbon-monoxide-001.md` 增加 `DG-0850` 的 `guide_links`。

## 6. Audit 结果

已运行：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

```text
Wiki audit:
Articles: markdown=839 pocketbase=839 categories=24
Issues: errors=0 warnings=0 advisories=0

build_guides:
Generated 782 Guides
Generated 782 Guide Index Items

Guide audit:
Guides: 782
Issues: errors=0 warnings=0 advisories=0
```

## 7. 未处理内容

- 未新增 DG-0851 灰烬与余火处理 Guide。
- 未新增 DG-0854 病人用品与厨房污染隔离 Guide。
- 未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit 或 fallback。
- 未修改测试。
- 未做 Shelter / Fire / WASH Retrieval Field Test。

## 8. 是否进入下一阶段 Field Test

是。下一阶段必须进入：Batch5-J Shelter / Fire / WASH Retrieval Field Test。

本批不能宣布 stable。当前只能说明 Wiki/Guide 结构、PocketBase 同步和 Guide-Wiki 双向关系已通过审计；是否能在真实检索场景中稳定进入 evidence，需要 Batch5-J 验证。
