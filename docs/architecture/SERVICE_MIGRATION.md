# LanternBox Service Migration Record

Version: v1.1  
Status: In Progress  
Last Updated: 2026-06-28

---

## 一、目标

本文件记录 LanternBox AI 重构过程中，函数从 routes.py、ai.py、resources.py、wiki.py 等旧位置迁移到新分层结构的进度。

原则：

```text
先盘点
再分类
最后迁移
不做临时补丁
```

---

## 二、已完成迁移

| 能力 | 原位置 | 新位置 | 状态 |
|---|---|---|---|
| AI Prompt 构造 | ai.py | response/prompts.py | ✅ |
| Response Safety | ai.py | response/safety.py | ✅ |
| Fallback Answer | ai.py | response/fallback.py | ✅ |
| LLM 调用 | ai.py | llm/client.py | ✅ |
| Reference 排序 | ai.py / routes.py | retrieval/references.py | ✅ |
| AI Rerank | ai.py | retrieval/reranker.py | ✅ |
| Rerank 应用 | routes.py | retrieval/apply.py | ✅ |
| Context Boost | ai.py | retrieval/context_boost.py | ✅ 过渡 |
| Pipeline Builder | routes.py | pipeline/builder.py | ✅ |
| Pipeline Preload | routes.py | pipeline/preload.py | ✅ |
| Pipeline Dispatcher | routes.py | pipeline/dispatcher.py | ✅ |
| Pipeline Postprocess | routes.py | pipeline/postprocess.py | ✅ |
| Pipeline Hooks | 无 | pipeline/hooks.py | ✅ |
| Wiki 查询 | wiki.py / routes.py | services/wiki_service.py | ✅ |
| Wiki 过滤 | routes.py | services/wiki_service.py | ✅ |
| Wiki 分类查询 | routes.py | services/wiki_service.py | ✅ |
| PocketBase 请求 | wiki.py / routes.py | pocketbase_client.py | ✅ |
| Guide Trigger | resources.py | services/guide_service.py | ✅ |
| Guide 关联指南 | resources.py | services/guide_service.py | ✅ |
| Guide 文本规范化 | resources.py | services/guide_service.py | ✅ |
| Guide 检索打分 | resources.py | retrieval/guide.py | ✅ |
| Candidate Pool | resources.py | retrieval/runtime.py | ✅ |
| Retrieval Decision | resources.py | retrieval/runtime.py | ✅ |

---

## 三、当前保留位置

### resources.py

当前保留：

```text
load_local_resources
merge_guides
build_local_context
prepare_ai_context
Resource Cache
```

`resources.py` 是资源协调层，不再承载 Guide、Wiki、Retrieval Runtime、Prompt 或 LLM 逻辑。

---

## 四、待迁移能力

| 能力 | 目标位置 | 优先级 |
|---|---|---|
| Inventory Service | services/inventory_service.py | P1 |
| Member Service | services/member_service.py | P1 |
| Map Service | services/map_service.py | P2 |
| Sensor Service | services/sensor_service.py | P2 |
| Log Service | services/log_service.py | P2 |
| Planner | planner/ | P1 |
| Study Mode Pipeline | pipeline/study.py | P2 |
| Knowledge Mode Pipeline | pipeline/knowledge.py | P2 |

---

## 五、特殊说明

### context_boost.py

当前仍为过渡模块。

未来应升级为：

```text
retrieval/strategy.py
```

目标是从 Context Engine 输出生成正式 Retrieval Strategy，而不是在检索后做临时加分。

---

## 六、迁移完成标准

一个能力迁移完成必须满足：

1. 原文件不再保留重复定义。
2. 新文件职责清晰。
3. 导入方向符合架构。
4. `python -c "from api.app import app; print('ok')"` 通过。
5. 非流式和流式 AI 接口测试通过。
6. 文档同步更新。

---

## 七、当前阶段结论

LanternBox AI 第一阶段架构重构已经基本完成。

当前核心结构：

```text
Pipeline Runtime
Services Layer
Retrieval Runtime
Response Layer
LLM Client
```

下一阶段应进入 Planner、Inventory、Study Mode 等功能建设。
