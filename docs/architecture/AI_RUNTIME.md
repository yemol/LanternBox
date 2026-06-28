# LanternBox AI Runtime

Version: v1.0  
Status: Active  
Last Updated: 2026-06-28

---

## 一、定位

AI Runtime 是 LanternBox AI 从请求进入到响应返回的完整运行机制。

它不是单个文件，而是一组协作层：

```text
Routes
  ↓
Pipeline Runtime
  ↓
Context Engine
  ↓
Services
  ↓
Retrieval Runtime
  ↓
Response
  ↓
LLM Client
```

---

## 二、运行流程

Emergency Mode 当前主流程：

```text
HTTP Request
  ↓
routes.py
  ↓
build_pipeline_request
  ↓
prepare_ai_pipeline_context
  ↓
run_ai_pipeline / run_ai_stream_pipeline
  ↓
Emergency Pipeline
  ↓
Response Messages
  ↓
LLM Client
  ↓
PipelineResult
  ↓
build_ai_advice_response
  ↓
HTTP Response
```

---

## 三、Runtime 中的关键对象

### PipelineRequest

统一 AI Pipeline 输入。

包含：

```text
message
mode
history
matched_triggers
related_guides
related_wikis
detected_domains
stream
metadata
```

---

### PipelineResult

统一非流式 Pipeline 输出。

包含：

```text
mode
answer
messages
debug
```

---

### Lantern Context

由 Context Engine 生成。

用于表达用户输入的结构化理解。

---

### Candidate Source

由 Retrieval Runtime 生成。

用于统一 Guide、Wiki、Kiwix、未来 Inventory / Map 等来源。

---

## 四、Retrieval Runtime

位置：

```text
api/retrieval/runtime.py
```

职责：

- 构建 Candidate Pool
- 标准化 Candidate Source
- 执行 Hard Exclusion
- 拆分 selected_sources / excluded_sources
- 生成 retrieval_decision
- 输出 debug 信息

当前支持来源：

```text
guide
wiki
kiwix（占位）
```

未来扩展：

```text
inventory
member
map
sensor
manual
log
```

---

## 五、Runtime 与 Services 的关系

Services 提供业务数据。

Runtime 组织流程和候选决策。

```text
Services：数据从哪里来
Retrieval Runtime：哪些数据进入回答
Pipeline Runtime：本次请求走哪些模块
```

---

## 六、Runtime 与 Response 的关系

Retrieval Runtime 不写 Prompt。

Response 读取 Retrieval 输出，并组织成 LLM 可理解的消息。

```text
Retrieval Runtime → selected_sources / excluded_sources / decision
Response → Prompt Messages
```

---

## 七、Fallback

Fallback 不应绕开 PipelineResult。

应通过：

```python
build_fallback_pipeline_result()
```

生成统一结果结构。

---

## 八、扩展原则

新增能力时，优先判断它属于哪类：

```text
业务数据 → Services
候选来源 → Retrieval Runtime
模式流程 → Pipeline
语言表达 → Response
模型调用 → LLM Client
```

不要把新功能直接塞入 routes.py 或 resources.py。
