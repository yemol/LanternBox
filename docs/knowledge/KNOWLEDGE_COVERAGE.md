# LanternBox 知识覆盖

本文件是知识覆盖入口文档。详细覆盖规划见：

- [知识覆盖 V1](./KNOWLEDGE_COVERAGE_V1.md)
- [生存知识基础预置包 V1](./SURVIVAL_BASELINE_PACK_V1.md)

## 生存基础预置状态

生存知识基础预置包 V1 定义 LanternBox 在离线、断供、灾害和长期自持环境下必须携带的最小知识集合。

当前状态：

- 基础预置文档：`docs/knowledge/SURVIVAL_BASELINE_PACK_V1.md`
- 核心领域：water, food, medical, shelter, evacuation, power, hygiene, security, tools, communication
- 扩展领域：planting, wild_food, repair, psychology
- 运行时扩展：禁止
- 运行时新增 ZIM：禁止
- 动态外部知识发现：禁止
- ZIM coverage：所有 baseline domain 均应为 Y
- Kiwix enabled：表示允许通过 Kiwix 访问本地 ZIM，不代表独立内容来源
- Gap Detection 依据：生存知识基础预置包 V1
- Kiwix selection 依据：生存知识基础预置包 V1

运行规则：

Guide 始终是行动层，Wiki 是精选解释层，ZIM 是离线内容层，Kiwix 是 ZIM 的本地访问层。ZIM 资源缺失不得阻断核心生存回答，只能降低背景覆盖置信度。
