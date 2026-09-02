# Guide Audit Report (2026-09-01)

## 结论
- 未发现阻断级问题：JSON 可解析、id/文件名/priority/最低结构检查通过。
- 发现 0 个 warning；发现 0 个 advisory。
- 未发现 canonical clone 重复组。

## 覆盖范围
- Guide JSON：856
- 目录数：30
- 分类数：45

## 目录分布
|directory|数量|
|---|---:|
|security|116|
|water|110|
|power|79|
|medical|75|
|hygiene|66|
|evacuation|58|
|tools|48|
|disaster|46|
|records|33|
|comms|23|
|food|21|
|external_contact|20|
|general|20|
|planting|20|
|psychology|20|
|shelter|14|
|fire|8|
|manufacturing|8|
|production|8|
|storage|8|
|waste|8|
|clothing|6|
|navigation|6|
|contamination|5|
|livestock|5|
|repair|5|
|resource_allocation|5|
|risk_decision|5|
|team|5|
|wild_food|5|

## Priority 分布
|priority|数量|
|---|---:|
|P0|626|
|P1|161|
|P2|69|

## Risk Level 分布
|risk_level|数量|
|---|---:|
|normal|113|
|caution|596|
|high|133|
|critical|14|

## 结构检查
- 通过：必填字段、id 格式、文件名一致性、priority 合法性和最低数组结构未发现 error/warning。

## 正文内容检查
- 通过：正文长度、外部依赖和绝对化表达未发现 error/warning。

## 重复与范围重叠
- 通过：未发现完整 JSON 重复、主体重复、canonical clone 或同目录高相似簇。

## Guide-Wiki 关联检查
- 通过：related_wiki 均为真实 Wiki slug，Guide-Wiki 前后向关系完全对称。
