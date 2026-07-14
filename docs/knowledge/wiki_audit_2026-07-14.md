# Wiki Audit Report (2026-07-14)

## 结论
- 未发现阻断级问题：slug 唯一性、必填字段、分类引用、Markdown 与 PocketBase 内容一致性均通过。
- 发现 0 个 warning，需要人工复核；发现 0 个 advisory，多为内容可进一步增强项。
- 未发现正文完全重复或 title+summary 完全重复。

## 覆盖范围
- Markdown wiki 条目：763
- PocketBase wiki_articles：763
- PocketBase wiki_categories：24
- data/lanternbox.db 表：inventory, journal, sqlite_sequence, task_events, task_reports, terminal_devices, terminal_tasks
- 说明：`data/lanternbox.db` 不承载 wiki；当前 wiki 数据库为 `pocketbase/pb_data/data.db`。

## Slug Domain 分布
|domain|数量|
|---|---:|
|agriculture|75|
|clothing|20|
|communication|32|
|education|3|
|energy|40|
|fire|30|
|food|35|
|general|20|
|hygiene|53|
|medical|42|
|navigation|22|
|organization|23|
|psychology|20|
|repair|122|
|safety|75|
|shelter|44|
|tools|4|
|water|79|
|wildlife|24|

## 分类分布
|category|数量|
|---|---:|
|信息保存与长期重建|20|
|医疗急救|43|
|卫生|29|
|团队轮值与任务管理|25|
|地图地形与环境监测|22|
|基础制造与材料维修|23|
|外部接触与物资交换风险|22|
|安全|32|
|小规模养殖|22|
|庇护空间分区|21|
|心理韧性与冲突降温|20|
|水|79|
|污染控制 / 隔离 / 清洁分区|24|
|火源 / 保温 / 通风 / 一氧化碳风险|32|
|种植|22|
|种植与食物生产|31|
|维修 / 制作 / 替代 / 拆解再利用|103|
|能源|40|
|衣物 / 鞋袜 / 体温防护|20|
|通讯|32|
|避难转移|21|
|野外食物获取 / 狩猎捕捞 / 动物蛋白补充|24|
|风险决策|21|
|食物|35|

## 元数据与 Slug 检查
- 通过：必填字段、slug 正则、slug domain、文件名一致性、分类引用、risk_level/status、tag 数量均未发现问题。

## 正文内容检查
- 通过：每篇条目均覆盖用途、操作、判断标准、风险提示；中高风险条目均含停止条件或不适用边界；未发现外部依赖或绝对化表达。

## 文件与数据库一致性
- 通过：763 个 Markdown slug 与 PocketBase wiki_articles 一一对应，标题、分类、摘要、标签、风险等级、状态、来源和正文均一致。

## 重复与范围重叠
- 通过：未发现 slug 重复、正文完全重复、title+summary 完全重复或同 domain 高相似条目。

### 主题族边界复核
- 以下为同一 `domain-topic` 下有多个序号的主题族；这通常是合理拆分，不自动判为重复，但适合人工做范围边界抽检。
|topic key|数量|slugs|
|---|---:|---|
|`safety-safety-knowledge`|10|`safety-safety-knowledge-001`, `safety-safety-knowledge-002`, `safety-safety-knowledge-003`, `safety-safety-knowledge-004`, `safety-safety-knowledge-005`, `safety-safety-knowledge-006`, `safety-safety-knowledge-007`, `safety-safety-knowledge-008`, `safety-safety-knowledge-009`, `safety-safety-knowledge-010`|
|`energy-lighting`|5|`energy-lighting-001`, `energy-lighting-002`, `energy-lighting-003`, `energy-lighting-004`, `energy-lighting-005`|
|`medical-fever`|5|`medical-fever-001`, `medical-fever-002`, `medical-fever-003`, `medical-fever-004`, `medical-fever-005`|
|`communication-communication-knowledge`|4|`communication-communication-knowledge-001`, `communication-communication-knowledge-002`, `communication-communication-knowledge-003`, `communication-communication-knowledge-004`|
|`fire-heating`|4|`fire-heating-001`, `fire-heating-002`, `fire-heating-003`, `fire-heating-004`|
|`food-food-knowledge`|4|`food-food-knowledge-001`, `food-food-knowledge-002`, `food-food-knowledge-003`, `food-food-knowledge-004`|
|`hygiene-hygiene-knowledge`|4|`hygiene-hygiene-knowledge-001`, `hygiene-hygiene-knowledge-002`, `hygiene-hygiene-knowledge-003`, `hygiene-hygiene-knowledge-004`|
|`hygiene-waste`|4|`hygiene-waste-001`, `hygiene-waste-002`, `hygiene-waste-003`, `hygiene-waste-004`|
|`safety-conflict`|4|`safety-conflict-001`, `safety-conflict-002`, `safety-conflict-003`, `safety-conflict-004`|
|`shelter-evacuation`|4|`shelter-evacuation-001`, `shelter-evacuation-002`, `shelter-evacuation-003`, `shelter-evacuation-004`|
|`water-boiling`|4|`water-boiling-001`, `water-boiling-002`, `water-boiling-003`, `water-boiling-004`|
|`agriculture-container`|3|`agriculture-container-001`, `agriculture-container-002`, `agriculture-container-003`|
|`clothing-insulation`|3|`clothing-insulation-001`, `clothing-insulation-002`, `clothing-insulation-003`|
|`communication-written-message`|3|`communication-written-message-001`, `communication-written-message-002`, `communication-written-message-003`|
|`energy-battery`|3|`energy-battery-001`, `energy-battery-002`, `energy-battery-003`|
|`energy-fever`|3|`energy-fever-001`, `energy-fever-002`, `energy-fever-003`|
|`energy-solar-charging`|3|`energy-solar-charging-001`, `energy-solar-charging-002`, `energy-solar-charging-003`|
|`fire-fire-knowledge`|3|`fire-fire-knowledge-001`, `fire-fire-knowledge-002`, `fire-fire-knowledge-003`|
|`food-canned-food`|3|`food-canned-food-001`, `food-canned-food-002`, `food-canned-food-003`|
|`food-refrigeration`|3|`food-refrigeration-001`, `food-refrigeration-002`, `food-refrigeration-003`|
|`hygiene-handwashing`|3|`hygiene-handwashing-001`, `hygiene-handwashing-002`, `hygiene-handwashing-003`|
|`hygiene-odor`|3|`hygiene-odor-001`, `hygiene-odor-002`, `hygiene-odor-003`|
|`hygiene-toilet`|3|`hygiene-toilet-001`, `hygiene-toilet-002`, `hygiene-toilet-003`|
|`hygiene-vomit`|3|`hygiene-vomit-001`, `hygiene-vomit-002`, `hygiene-vomit-003`|
|`safety-children`|3|`safety-children-001`, `safety-children-002`, `safety-children-003`|
|`safety-night-safety`|3|`safety-night-safety-001`, `safety-night-safety-002`, `safety-night-safety-003`|
|`safety-outside-movement`|3|`safety-outside-movement-001`, `safety-outside-movement-002`, `safety-outside-movement-003`|
|`safety-review`|3|`safety-review-001`, `safety-review-002`, `safety-review-003`|
|`shelter-contamination-zone`|3|`shelter-contamination-zone-001`, `shelter-contamination-zone-002`, `shelter-contamination-zone-003`|
|`water-drinking-water`|3|`water-drinking-water-001`, `water-drinking-water-002`, `water-drinking-water-003`|
|`agriculture-animal-feed`|2|`agriculture-animal-feed-001`, `agriculture-animal-feed-002`|
|`agriculture-feces`|2|`agriculture-feces-001`, `agriculture-feces-002`|
|`agriculture-seed-storage`|2|`agriculture-seed-storage-001`, `agriculture-seed-storage-002`|
|`clothing-repair`|2|`clothing-repair-001`, `clothing-repair-002`|
|`communication-contact-list`|2|`communication-contact-list-001`, `communication-contact-list-002`|
|`communication-contact-window`|2|`communication-contact-window-001`, `communication-contact-window-002`|
|`communication-offline-communication`|2|`communication-offline-communication-001`, `communication-offline-communication-002`|
|`communication-separated-movement`|2|`communication-separated-movement-001`, `communication-separated-movement-002`|
|`communication-sms`|2|`communication-sms-001`, `communication-sms-002`|
|`energy-battery-capacity`|2|`energy-battery-capacity-001`, `energy-battery-capacity-002`|
|`energy-power-outage`|2|`energy-power-outage-001`, `energy-power-outage-002`|
|`fire-alcohol-stove`|2|`fire-alcohol-stove-001`, `fire-alcohol-stove-002`|
|`fire-candle`|2|`fire-candle-001`, `fire-candle-002`|
|`fire-hypothermia`|2|`fire-hypothermia-001`, `fire-hypothermia-002`|
|`fire-insulation`|2|`fire-insulation-001`, `fire-insulation-002`|
|`fire-open-flame`|2|`fire-open-flame-001`, `fire-open-flame-002`|
|`fire-smoke`|2|`fire-smoke-001`, `fire-smoke-002`|
|`food-food-waste`|2|`food-food-waste-001`, `food-food-waste-002`|
|`food-protein`|2|`food-protein-001`, `food-protein-002`|
|`food-spoilage`|2|`food-spoilage-001`, `food-spoilage-002`|
|`food-staple-food`|2|`food-staple-food-001`, `food-staple-food-002`|
|`hygiene-contamination-zone`|2|`hygiene-contamination-zone-001`, `hygiene-contamination-zone-002`|
|`hygiene-mold`|2|`hygiene-mold-001`, `hygiene-mold-002`|
|`hygiene-wet-waste`|2|`hygiene-wet-waste-001`, `hygiene-wet-waste-003`|
|`medical-bleeding-control`|2|`medical-bleeding-control-001`, `medical-bleeding-control-002`|
|`medical-burn-care`|2|`medical-burn-care-001`, `medical-burn-care-002`|
|`medical-clean-water`|2|`medical-clean-water-001`, `medical-clean-water-002`|
|`medical-cross-infection`|2|`medical-cross-infection-001`, `medical-cross-infection-002`|
|`medical-dehydration`|2|`medical-dehydration-001`, `medical-dehydration-002`|
|`medical-wound-care`|2|`medical-wound-care-001`, `medical-wound-care-002`|
|`medical-wound-infection`|2|`medical-wound-infection-001`, `medical-wound-infection-002`|
|`repair-bag-repair`|2|`repair-bag-repair-001`, `repair-bag-repair-002`|
|`repair-door-window`|2|`repair-door-window-001`, `repair-door-window-002`|
|`repair-drinking-water`|2|`repair-drinking-water-001`, `repair-drinking-water-002`|
|`repair-footwear-repair`|2|`repair-footwear-repair-001`, `repair-footwear-repair-002`|
|`repair-plastic-sheet`|2|`repair-plastic-sheet-001`, `repair-plastic-sheet-002`|
|`repair-repair-knowledge`|2|`repair-repair-knowledge-001`, `repair-repair-knowledge-002`|
|`repair-salvage`|2|`repair-salvage-001`, `repair-salvage-002`|
|`safety-barter`|2|`safety-barter-001`, `safety-barter-002`|
|`safety-door-window`|2|`safety-door-window-001`, `safety-door-window-002`|
|`safety-evacuation`|2|`safety-evacuation-001`, `safety-evacuation-002`|
|`safety-fire-response`|2|`safety-fire-response-001`, `safety-fire-response-002`|
|`safety-gas-leak`|2|`safety-gas-leak-001`, `safety-gas-leak-002`|
|`safety-inventory`|2|`safety-inventory-001`, `safety-inventory-002`|
|`safety-low-profile`|2|`safety-low-profile-001`, `safety-low-profile-002`|
|`safety-shelter-in-place`|2|`safety-shelter-in-place-001`, `safety-shelter-in-place-002`|
|`safety-stranger-contact`|2|`safety-stranger-contact-001`, `safety-stranger-contact-002`|
|`shelter-children`|2|`shelter-children-001`, `shelter-children-003`|
|`shelter-flood-water`|2|`shelter-flood-water-001`, `shelter-flood-water-002`|
|`shelter-lighting`|2|`shelter-lighting-001`, `shelter-lighting-002`|
|...|...|另有 10 个主题族|

## 复跑方式
```bash
python3 tools/audit_wiki.py
```
