# Guide Audit Report (2026-07-02)

## 结论
- 未发现阻断级问题：JSON 可解析、id/文件名/priority/最低结构检查通过。
- 发现 2 个 warning；发现 49 个 advisory。
- 未发现 canonical clone 重复组。

## 覆盖范围
- Guide JSON：699
- 目录数：24
- 分类数：40

## 目录分布
|directory|数量|
|---|---:|
|security|116|
|water|80|
|medical|75|
|power|66|
|hygiene|64|
|evacuation|58|
|disaster|46|
|tools|40|
|records|25|
|food|21|
|general|20|
|comms|17|
|shelter|12|
|planting|9|
|clothing|5|
|contamination|5|
|external_contact|5|
|fire|5|
|livestock|5|
|psychology|5|
|repair|5|
|risk_decision|5|
|team|5|
|wild_food|5|

## Priority 分布
|priority|数量|
|---|---:|
|P0|521|
|P1|111|
|P2|67|

## 结构检查
- 通过：必填字段、id 格式、文件名一致性、priority 合法性和最低数组结构未发现 error/warning。

## 正文内容检查
|级别|文件|问题|
|---|---|---|
|warning|`data/guides/power/DG-0196.json`|含绝对化表达：完全安全|
|warning|`data/guides/security/DG-0429.json`|含绝对化表达：保证安全|

### 内容增强建议
- 49 条 advisory，主要提示模板化语句或标题短语重复。

## 重复与范围重叠
- 通过：未发现完整 JSON 重复、主体重复、canonical clone 或同目录高相似簇。
