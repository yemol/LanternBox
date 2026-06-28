# LanternBox Service Migration Plan

Version: v0.1  
Status: Draft  
Last Updated: 2026-06-28

---

## 一、目标

本文件用于记录 LanternBox AI 重构过程中，现有函数从 routes.py、ai.py、resources.py、wiki.py、retrieval 等位置迁移到 Services Layer 的计划。

当前原则：

```text
先盘点
再分类
最后迁移