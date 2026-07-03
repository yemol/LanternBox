# Kiwix Integration Policy V1

Version: 1.0
Status: Draft
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 接入 Kiwix / ZIM 后的知识优先级、风险边界和调试要求。

Kiwix 是广域离线知识库，用于补充背景知识、术语、教材、百科和长期学习资料。Kiwix 不是 LanternBox 的应急行动第一来源，也不能覆盖 LanternBox 本地 Guide 和精选 Wiki 已经定义的高风险边界。

---

## 2. Source Priority

LanternBox 的知识优先级固定为：

```text
Guide
  ↓
Wiki
  ↓
Kiwix / ZIM
```

Guide 优先于 Wiki。Guide 是行动卡，负责“当前怎么做、先做什么、何时停止、如何降级”。

Wiki 优先于 Kiwix。Wiki 是精选结构化知识，负责解释判断标准、适用边界、常见误区和本地能力体系。

Kiwix 只能补充背景知识和广域知识。Kiwix 适合提供概念、术语、图示、基础教材、百科上下文和长期学习材料。

---

## 3. High Risk Boundary

Kiwix 不能覆盖高风险 Guide。

当 Kiwix 内容与本地 Guide 或高风险边界 Wiki 冲突时，必须以本地 Guide / Wiki 为准。典型场景包括但不限于：

- 燃气味环境禁止点火；
- 室内燃烧的一氧化碳风险；
- 电器进水后禁止通电；
- 锂电池鼓包停止使用；
- 不明蘑菇默认不食用；
- 病死动物默认不食用；
- 未确认水源不直接饮用；
- 化学容器不得装饮用水；
- 深伤口感染、高热意识异常、儿童老人脱水风险。

Kiwix 不作为应急行动第一来源。在 Emergency / Survival 场景中，Kiwix 只能作为补充解释，不得替代本地行动步骤。

---

## 4. Evidence Selection

Kiwix 结果必须经过 Retrieval v2 Evidence Selector。

Retrieval v2 可以召回 Kiwix 候选，但最终进入回答上下文前，必须由 Evidence Selector 判断其是否：

- 与用户问题相关；
- 不覆盖本地 Guide；
- 不违反高风险边界；
- 不引入无法离线执行的优先建议；
- 适合作为背景补充而非行动指令。

如果 Guide 已经给出明确停止条件，Kiwix 只能解释原因，不能建议继续尝试。

---

## 5. Debug Traceability

Kiwix 结果必须在 debug 中可追踪。

系统至少应能在调试信息中区分：

- source_type 是否为 `kiwix`；
- 来源 ZIM 文件或 Kiwix 服务标识；
- 查询词；
- 候选标题；
- 片段或摘要；
- 是否被 Evidence Selector 选中；
- 被选中或排除的理由。

未被选中的 Kiwix 候选不应进入最终回答上下文。

---

## 6. Offline And Failure Behavior

未配置 Kiwix 服务时系统必须正常运行。

Kiwix 未安装、ZIM 文件缺失、服务未启动、查询超时或索引损坏时：

- Guide 查询必须继续可用；
- Wiki 查询必须继续可用；
- Retrieval v2 不应整体失败；
- 回答可以说明未使用 Kiwix 背景；
- debug 应记录 Kiwix 不可用原因。

Kiwix 是增强能力，不是 LanternBox 核心可用性的前提。

---

## 7. No External Dependency Drift

Kiwix 不得引入现代外部依赖优先建议。

如果 Kiwix 内容建议依赖互联网、现代物流、专业医院、实时云服务、在线地图、在线购买、持续电网或外部组织支援，LanternBox 必须将其视为背景信息，而不是默认行动方案。

Kiwix 不得绕过 LanternBox 无外部支援假设。回答应优先保持：

- 离线可执行；
- 低功耗可执行；
- 本地资源可执行；
- 可降级；
- 可记录；
- 可停止。

---

## 8. Answering Rule

最终回答中，Kiwix 内容应使用以下方式出现：

- 作为背景解释；
- 作为进一步学习方向；
- 作为术语和原理补充；
- 作为非紧急场景下的拓展资料。

最终回答中，Kiwix 内容不应：

- 直接给高风险操作步骤；
- 覆盖 Guide 的停止条件；
- 鼓励试吃、试喝、试通电、试点火；
- 要求用户依赖外部救援或在线服务；
- 引入攻击性、对抗性或伤害性内容。

---

## 9. Summary

Kiwix 的位置是第三层知识源。

Guide 决定行动。  
Wiki 决定判断边界。  
Kiwix 补充广域背景。

Kiwix 接入的目标不是让 LanternBox 变成百科搜索器，而是在本地 Guide / Wiki 已经建立安全边界后，提供更广、更深、可离线保存的学习和解释能力。
