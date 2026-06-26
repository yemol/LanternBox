# LanternBox / 壳中灯

> **为人类的存续留下一个小小的火种。**

LanternBox
是一个完全离线、封闭、自有网络、自主供电的个人/团队知识与生存系统。项目目标是在长期失去外部支持的环境下，保存知识、积累经验、辅助决策，并帮助个体和团队维持持续生存能力。

## 系统架构

``` text
                 LanternBox

        ┌────────────────────────┐
        │ Core (Mac mini)        │
        │ FastAPI                │
        │ PocketBase             │
        │ Web UI                 │
        │ RAG                    │
        └──────────┬─────────────┘
                   │ HTTP API
                   ▼
        ┌────────────────────────┐
        │ AI Node                │
        │ Ollama                 │
        │ Whisper（规划）        │
        │ Piper（规划）          │
        │ OCR（规划）            │
        │ Vision（规划）         │
        └────────────────────────┘
```

## 三层知识体系

1.  应急指南库：快速、结构化处置。
2.  PocketBase 精选 Wiki：高价值知识索引。
3.  Kiwix / ZIM：大型离线知识底座。

## 快速启动

1.  启动 AI Node（Ollama）
2.  启动 PocketBase
3.  启动 Voice Service
4.  启动 Core

## 文档

-   docs/architecture.md
-   docs/deployment.md

## 当前里程碑

-   Core 与 AI Node 解耦
-   支持远程 Ollama
-   独立 Voice Service
-   PocketBase 精选 Wiki
