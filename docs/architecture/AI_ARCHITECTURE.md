# LanternBox AI Architecture

Version: v1.1  
Status: Active  
Last Updated: 2026-06-28

---

## 一、定位

LanternBox AI 不是普通聊天机器人，而是面向离线、低资源、极端环境和长期运行场景的本地智能系统。

它的目标不是单纯回答问题，而是：

> 理解环境，整合本地知识，检索可靠资料，制定行动方案，并帮助使用者执行。

因此，LanternBox AI 采用分层架构，而不是把所有逻辑堆在 `ai.py`、`routes.py` 或 `resources.py` 中。

---

## 二、总体架构

```text
Mode
  ↓
Pipeline Runtime
  ↓
Context Engine
  ↓
Services Layer
  ↓
Retrieval Layer
  ↓
Planner Layer（预留）
  ↓
Response Layer
  ↓
LLM Client
```

当前完整分层主要服务 **Emergency Mode / 应急模式**。

Companion Mode / 陪伴模式保持轻量，不强制进入完整 Context / Retrieval / Planner 管线。

Study Mode / 学习模式未来会接入类似架构，但会使用 Learning Planner 与 Teaching Response。

---

## 三、模式感知原则

LanternBox AI 是 Mode-aware 的。

不同模式不应强行共用同一条复杂流程。

当前原则：

```text
Emergency Mode：完整架构
Companion Mode：轻量 AI
Study Mode：未来专用学习管线
Knowledge Mode：偏知识检索与解释
Navigation Mode：未来地图 / 位置 / 路线管线
Energy Mode：未来能源分析与节能管线
Team Mode：未来成员 / 权限 / 加密管线
```

---

## 四、各层职责

### 1. Context Engine

位置：

```text
api/context/
```

职责：

- 理解用户输入
- 识别场景
- 识别风险
- 识别领域
- 提取观察信息
- 生成结构化 Lantern Context

主入口：

```python
analyze_context()
```

Context Engine 不负责：

- 检索资料
- 排序资料
- 拼 Prompt
- 调用 LLM
- 输出自然语言回答

---

### 2. Services Layer

位置：

```text
api/services/
```

职责：

- 访问本地业务数据
- 整理本地资料
- 隐藏底层数据来源
- 为 Pipeline / Retrieval 提供稳定数据接口

当前已落地：

```text
guide_service.py
wiki_service.py
```

未来扩展：

```text
inventory_service.py
member_service.py
map_service.py
sensor_service.py
log_service.py
```

Services 不负责：

- AI 调度
- Prompt 组织
- LLM 调用
- HTTP 路由
- Planner 决策

---

### 3. Retrieval Layer

位置：

```text
api/retrieval/
```

职责：

- 查询分析
- 领域识别
- 指南检索
- 来源过滤
- 来源排序
- 候选池构建
- 硬排除
- AI 重排
- 检索决策

当前结构：

```text
retrieval/
├── constants.py
├── query.py
├── domains.py
├── guide.py
├── references.py
├── runtime.py
├── reranker.py
├── apply.py
└── context_boost.py（过渡，未来 strategy）
```

#### Retrieval Runtime

`runtime.py` 是 Retrieval Layer 的运行时核心。

负责：

- Candidate Pool
- Candidate Source 标准化
- Hard Exclusion
- Selected / Excluded Sources
- Retrieval Decision
- Retrieval Debug Info
- Kiwix 候选预留

Runtime 不负责 Prompt，也不调用 LLM。

---

### 4. Planner Layer（预留）

位置：

```text
api/planner/
```

职责：

根据 Context、Retrieval、Inventory、Members、Sensors 等输入，生成行动计划。

未来输出：

```text
Goals
Immediate Actions
Forbidden Actions
Observations
Questions
Priority
```

Planner 不负责自然语言表达，也不直接调用 LLM。

---

### 5. Response Layer

位置：

```text
api/response/
```

当前结构：

```text
response/
├── prompts.py
├── context_blocks.py
├── safety.py
└── fallback.py
```

职责：

- 构造 Prompt Messages
- 管理对话历史
- 组织 Context Blocks
- 清洗不符合应急模式的输出
- 生成 fallback 回答

Response 不负责 Retrieval，不访问数据库，不做业务数据查询。

---

### 6. LLM Client

位置：

```text
api/llm/client.py
```

职责只有一个：

> 和模型通信。

当前主函数：

```python
call_ollama()
stream_ollama()
```

LLM Client 不知道 Guide、Wiki、Pipeline、Planner、Context、Retrieval。

---

### 7. Pipeline Runtime

位置：

```text
api/pipeline/
```

职责：

- 接收 PipelineRequest
- 根据 mode 分发到不同 Pipeline
- 组织 Preload / Dispatcher / Postprocess
- 支持 Hooks 扩展点

当前结构：

```text
pipeline/
├── schema.py
├── builder.py
├── preload.py
├── dispatcher.py
├── emergency.py
├── companion.py
├── hooks.py
└── postprocess.py
```

Pipeline 是调度层，不是业务数据层。

---

### 8. Resources

位置：

```text
api/resources.py
```

当前定位：资源协调层。

保留职责：

- 本地资源加载
- Resource Cache

Resources 不再承担 Guide Service、Wiki Service、Retrieval Runtime、Prompt、上下文拼装、LLM 等职责。

---

## 五、依赖方向

允许：

```text
routes → pipeline
pipeline → services
pipeline → retrieval
pipeline → response
pipeline → llm
retrieval → services
retrieval → context
response → context_blocks / safety / fallback
services → pocketbase_client / files / local data
```

禁止：

```text
services → pipeline
services → response
services → llm
services → routes
retrieval → response
response → retrieval
llm → services / retrieval / pipeline
routes → 直接组织 AI 流程
```

---

## 六、ai.py 定位

`api/ai.py` 是兼容门面。

它不承担业务逻辑。

目标：

```text
只保留 import / re-export / 兼容入口
```

---

## 七、核心工程原则

1. 模式决定 Pipeline。
2. Pipeline 决定模块。
3. Services 提供本地业务数据。
4. Retrieval 选择和判断来源。
5. Response 组织语言材料。
6. LLM 只负责生成。
7. Resources 只做资源协调。
8. 不做临时补丁，优先完整架构修复。
9. 不过度分层，只有当职责稳定后才新建模块。

---

## 八、后续路线

已完成：

```text
Context Engine
Retrieval Layer
Retrieval Runtime
Response Layer
Pipeline Runtime
Services 初版
Wiki Service
Guide Service
```

下一阶段：

```text
Planner
Inventory Service
Member Service
Map Service
Sensor Service
Study Mode
Knowledge Mode
```
