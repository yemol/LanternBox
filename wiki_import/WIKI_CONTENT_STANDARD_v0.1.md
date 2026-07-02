# 壳中灯 Wiki 内容规范 v0.1

每篇 Wiki 原稿使用 Markdown + front matter。

示例：

```markdown
---
title: 伤口清洁与观察基础
slug: medical-wound-cleaning-observation-001
category: 医疗急救
summary: 介绍小型割伤、擦伤和浅表伤口的基础清洁、覆盖和观察方法。
tags: 伤口, 清洁, 急救
risk_level: caution
status: published
source: CDC；Mayo Clinic
---

# 伤口清洁与观察基础

## 适用场景

正文从这里开始。
```

字段要求：

- title：短、明确、可检索。
- slug：唯一标识，按 `SLUG_NAMING_RULES.md` 使用 `<domain>-<topic>-<sequence>` 格式，用于导入时判断新建或更新。
- category：必须与 PocketBase 的 wiki_categories.name 一致。
- summary：40 到 80 字左右，适合列表和卡片显示。summary 不应重复 title 开头；列表卡片已单独显示标题，因此 summary 应直接说明本条的实际动作、判断边界或停止条件。
- tags：中文关键词，逗号分隔，至少 3 个。
- risk_level：normal / caution / high。
- status：draft / published。
- source：必须填写来源。

正文建议结构：

```markdown
# 标题

## 适用场景

## 核心原则

## 操作步骤

## 检查点

## 常见错误

## 停止条件

## 备注
```
