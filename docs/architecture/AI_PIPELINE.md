# LanternBox AI Pipeline

Version: v0.1  
Status: Active  
Last Updated: 2026-06-27

---

## 一、定位

AI Pipeline 是 LanternBox AI 的总调度层。

它负责根据不同 AI 模式，决定一次请求应该经过哪些模块。

Pipeline 不直接承担具体业务逻辑，而是组织流程。

---

## 二、为什么需要 Pipeline

LanternBox 不是单一聊天机器人。

不同模式需要不同流程：

- 应急模式需要完整管线。
- 陪伴模式需要轻量 AI。
- 学习模式未来需要学习管线。
- 知识模式主要依赖检索。
- 地图、能源、团队模式未来会接入各自数据源。

因此，不能把所有 AI 流程写在 routes.py 或 ai.py 中。

---

## 三、总体关系

```text
routes.py
   │
   ▼
Pipeline Dispatcher
   │
   ├── Emergency Pipeline
   ├── Companion Pipeline
   ├── Study Pipeline（未来）
   ├── Knowledge Pipeline（未来）
   ├── Navigation Pipeline（未来）
   └── Energy Pipeline（未来）