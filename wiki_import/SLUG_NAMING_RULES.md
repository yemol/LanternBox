Wiki Slug 命名规则

本文档用于统一 LanternBox Wiki 条目的 slug 命名方式。

⸻

1. 基本格式

所有 Wiki 条目的 slug 统一使用以下格式：

<domain>-<topic>-<sequence>

必要时可以增加一个子主题：

<domain>-<topic>-<subtopic>-<sequence>

示例：

water-boiling-001
food-storage-001
medical-wound-care-001
communication-radio-daily-check-001
clothing-cold-protection-001

⸻

2. 基本要求

slug 必须满足以下要求：

1. 全库唯一。
2. 全部使用小写英文。
3. 只使用英文字母、数字和短横线 -。
4. 不使用中文。
5. 不使用空格。
6. 不使用下划线 _。
7. 不使用大写字母。
8. 不使用无意义缩写。
9. 不使用临时前缀。
10. 末尾必须使用三位数字编号。

合法示例：

water-storage-001
medical-first-aid-001
energy-battery-maintenance-001

非法示例：

lt-clothing-001
Water-Storage-001
water_storage_001
water-storage
衣物防寒001

⸻

3. domain 规则

domain 表示一级知识领域，必须从以下固定列表中选择：

water
food
medical
energy
repair
hygiene
safety
communication
shelter
navigation
clothing
fire
tools
agriculture
wildlife
organization
psychology
education
general

常见含义如下：

domain	含义
water	饮水、净水、储水、取水
food	食物、储存、烹饪、配给
medical	急救、疾病、药品、护理
energy	电池、太阳能、发电、节能
repair	维修、拆解、替代制作
hygiene	卫生、清洁、消毒、污染控制
safety	安全、防护、风险规避
communication	通讯设备、无线电、信号、联络
shelter	避难所、营地、空间分区
navigation	地图、方向、路线、定位
clothing	衣物、鞋袜、保温、防雨
fire	火源、生火、燃料、通风
tools	工具用途、维护、安全使用
agriculture	种植、种子、土壤、灌溉
wildlife	野生植物、动物、蘑菇风险
organization	团队组织、纪律、分工、交接
psychology	情绪、压力、冲突缓解
education	教学、训练、知识传承
general	暂时无法归类的通用条目

⸻

4. topic 规则

topic 表示该条知识的主要主题。

命名要求：

1. 使用简短英文。
2. 不需要完整翻译中文标题。
3. 优先使用稳定、通用的词。
4. 多个英文单词之间使用短横线 - 连接。
5. 避免过长，一般控制在 1 到 3 个词。

示例：

storage
boiling
wound-care
daily-check
cold-protection
battery-maintenance

不推荐：

how-to-check-water-storage-container-in-long-term-emergency

推荐：

water-storage-check-001

⸻

5. sequence 规则

sequence 是三位数字编号。

格式固定为：

001
002
003

禁止使用：

1
01
0001

编号规则：

1. 在相同 domain-topic 下递增。
2. 不同 topic 可以分别从 001 开始。
3. 新增条目时，查找同类最大编号后继续递增。

示例：

water-boiling-001
water-boiling-002
water-boiling-003
water-storage-001
water-storage-002
medical-wound-care-001
medical-wound-care-002

⸻

6. 命名示例

推荐风格：

water-boiling-001
water-storage-container-001
water-rainwater-collection-001
food-rationing-001
food-spoilage-check-001
food-dry-storage-001
medical-wound-care-001
medical-bleeding-control-001
medical-fever-care-001
energy-battery-storage-001
energy-solar-panel-maintenance-001
energy-power-saving-001
communication-radio-daily-check-001
communication-device-battery-001
communication-message-protocol-001
clothing-cold-protection-001
clothing-rain-protection-001
clothing-foot-care-001
organization-shift-management-001
organization-inventory-record-001
organization-knowledge-transfer-001

⸻

7. 正则校验规则

推荐使用以下正则校验 slug：

^[a-z0-9]+(?:-[a-z0-9]+)*-[0-9]{3}$

该规则表示：

1. 只能使用小写字母、数字和短横线。
2. 不能以短横线开头。
3. 不能以短横线结尾。
4. 末尾必须是三位数字编号。

⸻

8. 最终原则

slug 是知识条目的稳定编号，不是完整标题。

标题可以详细，slug 应该：

短
稳
清楚
唯一
可长期维护