# Wiki Schema V1

Wiki 是 LanternBox 的“精选结构化知识条目”。它回答的是：为什么、怎么判断、边界是什么。Wiki 不替代 Guide 的行动步骤，而是帮助用户理解风险、做判断、复盘和学习。

## 字段定义

- `id`：稳定 ID，可使用系统生成 ID 或人工命名 ID。
- `title`：知识条目标题，清楚说明主题。
- `category`：知识域，例如 `水`、`医疗急救`、`维修`。
- `priority`：知识优先级，建议使用 `P0`、`P1`、`P2`。
- `risk_level`：风险等级，例如 `low`、`medium`、`high`、`critical`。
- `summary`：一段短摘要，说明这条知识解决什么判断问题。
- `tags`：检索标签数组，覆盖术语、口语、风险词和别名。
- `content`：结构化正文，建议包含原理、判断标准、边界、误区和示例。
- `guide_links`：关联 Guide ID，用于从知识跳转到行动卡。
- `kiwix_topics`：建议关联的 Kiwix/ZIM 主题，用于广域背景扩展。
- `source`：来源说明，可写内部整理、公开资料、离线百科主题等。
- `last_reviewed`：最后复核日期，格式 `YYYY-MM-DD`。

## 编写原则

- Wiki 解释判断标准，不直接替代当前行动。
- 内容必须结构化，便于 Retrieval v2 摘取和前端展示。
- 对高风险主题必须写清楚“不适用条件”和“需要升级”的边界。
- 与 Guide 保持互补：Guide 短而可执行，Wiki 稍长但仍应精选。
- Kiwix/ZIM 只作为背景和深挖入口，不覆盖本地更具体的 Guide。

## 完整示例

```json
{
  "id": "WIKI-P0-POWER-WATER-DAMAGED-OUTLET-001",
  "title": "插线板和延长线进水后的漏电与短路风险",
  "category": "维修 / 制作 / 替代 / 拆解再利用",
  "priority": "P0",
  "risk_level": "high",
  "summary": "插线板、排插和延长线进水后，即使外表变干，内部金属件和绝缘材料仍可能残留水分、锈蚀或污染，继续通电会增加漏电、短路、发热和火灾风险。",
  "tags": [
    "插线板",
    "排插",
    "延长线",
    "进水",
    "泡水",
    "雨水",
    "潮湿",
    "漏电",
    "短路",
    "禁止通电",
    "报废"
  ],
  "content": {
    "principle": [
      "水分、泥沙和盐分会降低绝缘性能。",
      "内部金属件锈蚀后可能导致接触不良和发热。",
      "外壳表面干燥不代表内部完全干燥。"
    ],
    "judgement": [
      "外壳、插孔、线缆或插头潮湿时不得使用。",
      "出现焦味、变形、锈蚀、发热或跳闸，应直接停用。",
      "无法确认内部完全干燥和绝缘状态时，按报废处理。"
    ],
    "boundaries": [
      "不要插电测试。",
      "不要用高负载电器试用。",
      "不要把进水设备留作夜间无人看管供电。"
    ],
    "examples": [
      "雨水打湿的延长线，即使晒干，也应优先替换。",
      "洪水泡过的排插应从关键供电链路中移除。"
    ]
  },
  "guide_links": [
    "DG-0546"
  ],
  "kiwix_topics": [
    "Electrical safety",
    "Short circuit",
    "Insulation resistance"
  ],
  "source": "LanternBox internal curated knowledge",
  "last_reviewed": "2026-07-01"
}
```
