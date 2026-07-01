# LanternBox AI Pipeline

Version: v1.0  
Status: Active  
Last Updated: 2026-06-28

---

## 一、定位

AI Pipeline 是 LanternBox AI 的运行时调度层。

它负责根据不同 mode 决定一次 AI 请求应该经过哪些模块。

Pipeline 不直接承担业务数据处理，不直接访问数据库，不直接写复杂检索规则。

---

## 二、总体流程

```text
routes.py
  ↓
Pipeline Builder
  ↓
Pipeline Preload
  ↓
Pipeline Dispatcher
  ↓
Emergency / Companion / Future Pipelines
  ↓
Pipeline Postprocess
  ↓
HTTP Response
```

---

## 三、当前目录结构

```text
api/pipeline/
├── __init__.py
├── schema.py
├── builder.py
├── preload.py
├── dispatcher.py
├── emergency.py
├── companion.py
├── hooks.py
└── postprocess.py
```

---

## 四、Builder

文件：

```text
api/pipeline/builder.py
```

职责：

- 将 FastAPI payload 转换为 PipelineRequest
- 规整 message、mode、history
- 处理 ChatHistoryItem / Pydantic model / dict 的 history 兼容
- 接收 preload 后的业务输入

Builder 不负责：

- 查 Wiki
- 查 Guide
- 调用 LLM
- 检索排序
- 拼 Prompt

---

## 五、Preload

文件：

```text
api/pipeline/preload.py
```

职责：

- 组织 AI 预处理流程
- 调用 Retrieval v2 Orchestrator
- 生成 PipelineRequest 所需业务输入

当前 Preload 已接管原本散落在 routes.py 中的 AI 预处理逻辑。

Preload 不负责：

- HTTP 响应
- Prompt 组织
- LLM 调用
- 数据源底层访问

---

## 六、Dispatcher

文件：

```text
api/pipeline/dispatcher.py
```

职责：

- 根据 mode 选择 pipeline
- 处理 stream / non-stream 两类入口

当前支持：

```text
emergency
companion
```

未来支持：

```text
study
knowledge
navigation
energy
team
```

---

## 七、Emergency Pipeline

文件：

```text
api/pipeline/emergency.py
```

当前流程：

```text
PipelineRequest
  ↓
build_emergency_messages
  ↓
call_ollama / stream_ollama
  ↓
PipelineResult / stream messages
```

未来流程：

```text
Context
  ↓
Retrieval
  ↓
Planner
  ↓
Response
  ↓
LLM
```

应急模式必须遵守：

- 默认无外部支援
- 优先本地资料
- 优先行动步骤
- 禁止默认依赖城市服务
- 适合离线、低资源和极端环境

---

## 八、Companion Pipeline

文件：

```text
api/pipeline/companion.py
```

流程：

```text
PipelineRequest
  ↓
build_companion_messages
  ↓
LLM Client
```

陪伴模式不强制进入：

- Context Engine
- Retrieval
- Planner
- 应急指南检索
- 风险分级

---

## 九、Hooks

文件：

```text
api/pipeline/hooks.py
```

当前扩展点：

```text
before_preload
after_preload
before_dispatch
after_dispatch
before_response
after_response
```

Hooks 用于轻量扩展，不应滥用。

暂不引入复杂 Plugin Manager、Event Bus 或 DI 容器。

---

## 十、Postprocess

文件：

```text
api/pipeline/postprocess.py
```

职责：

- 将 PipelineResult 转换为 API JSON
- 构造 fallback PipelineResult
- 保持 routes.py 不直接构造 PipelineResult

---

## 十一、routes.py 的职责

routes.py 只负责：

- HTTP 请求
- 参数接收
- 调用 Pipeline
- 返回 HTTP 响应
- 非 AI 的普通接口入口

routes.py 不负责：

- AI 预处理
- Guide / Wiki 检索
- Rerank
- Prompt 构造
- LLM 调用
- 直接查询 PocketBase 业务数据

---

## 十二、设计原则

```text
Mode 决定 Pipeline
Pipeline 决定模块
Preload 准备输入
Dispatcher 分发模式
Postprocess 统一输出
```
