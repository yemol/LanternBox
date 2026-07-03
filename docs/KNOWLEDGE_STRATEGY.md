# LanternBox Knowledge Strategy

Version: 1.0
Status: Frozen
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 知识系统如何组织、命名、维护、审核和成长。

---

## 2. Knowledge Hierarchy

LanternBox 知识固定分为三层：

```
Guide
  ↓
Wiki
  ↓
Kiwix / ZIM
```

Guide 负责行动。Wiki 负责能力建设。Kiwix 负责广域知识保存。

---

## 3. Directory Structure

推荐目录：

```
knowledge/
  inbox/
  guide/
  wiki/
  kiwix/
  assets/
  templates/
```

Inbox 是待整理区，不参与 AI 默认检索。

---

## 4. Guide Organization

Guide 按能力分类：

```
guide/
  water/
  food/
  medical/
  medicine/
  shelter/
  energy/
  tools/
  maintenance/
  navigation/
  communication/
  agriculture/
  safety/
  team/
```

Guide 回答“现在怎么做”。Guide 应简短、明确、步骤化、可立即执行。

---

## 5. Wiki Organization

Wiki 按能力分类：

```
wiki/
  water/
  food/
  medical/
  medicine/
  shelter/
  engineering/
  agriculture/
  energy/
  tools/
  maintenance/
  communication/
  navigation/
  education/
  organization/
```

Wiki 回答“为什么这样做、如何长期实践、有哪些风险和替代方案”。

---

## 6. Slug Rule

统一格式：

```
category/topic-name
```

要求：

- 小写英文；
- 短横线连接；
- 不使用空格；
- 不使用中文；
- 保持稳定，不随标题轻易变化。

示例：

```
medical/bleeding-control
water/rainwater-collection
medicine/amoxicillin
navigation/resource-point
tools/hand-saw
```

---

## 7. Metadata

Guide / Wiki 推荐公共字段：

```yaml
title:
slug:
category:
tags:
summary:
difficulty:
risk_level:
related_guides:
related_wiki:
references:
status:
version:
updated_at:
```

AI、检索、引用和审核都依赖 Metadata。

---

## 8. Lifecycle

知识生命周期：

```
Idea → Draft → Review → Published → Updated → Archived
```

AI 默认只检索 Published。

---

## 9. Review Rules

知识审核关注：

- 真实性；
- 可执行性；
- 安全性；
- 是否重复；
- 是否属于 LanternBox 能力体系；
- 是否有可靠来源；
- 是否存在医疗、药物、工程、能源等高风险误导。

未经审核内容不得进入正式知识库。

---

## 10. AI Retrieval Priority

AI 检索顺序：

```
Guide
  ↓
Wiki
  ↓
Team Knowledge
  ↓
Kiwix
```

优先行动，其次理解，最后扩展阅读。

---

## 11. Team Knowledge

团队经验包括维修记录、种植记录、医疗观察、资源调查、巡逻记录、失败经验和任务复盘。

团队经验默认进入 Knowledge Inbox，审核后可进入 Guide、Wiki 或 Archive。

---

## 12. Quality Standard

正式知识至少满足：

- 来源可靠；
- 内容准确；
- 能够执行；
- 可以维护；
- 能够关联其它知识；
- 有明确适用边界；
- 高风险内容有风险提示。

目标不是知识越来越多，而是知识越来越准确、可用、可传承。

---

## 13. Maintenance Rule

知识维护包括新增、修订、归档、引用修正、分类调整和版本记录。

知识体系必须随项目持续更新，但三层结构不轻易改变。

---

## 14. Success Criteria

知识系统达成目标时：

- Guide 能帮助成员立即行动；
- Wiki 能帮助成员建立能力；
- Kiwix 能提供广域知识；
- AI 能准确检索和引用；
- 团队经验能沉淀；
- 知识能长期维护和传承。
