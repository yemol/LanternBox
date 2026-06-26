# LanternBox AI Architecture

Version: v1.0
Status: Active
Last Updated: 2026-06-27

---

# 一、为什么要重新设计 AI 架构

LanternBox 并不是一个普通的大语言模型聊天程序。

它的目标是在：

- 完全离线
- 长期运行
- 极端环境
- 多终端协同
- 本地知识库
- 本地传感器
- 本地成员管理
- 有限能源

等条件下，持续帮助使用者完成判断、规划和执行。

因此，它不能采用传统的：

```
User
 ↓
Prompt
 ↓
LLM
 ↓
Answer
```

这种简单聊天架构。

随着项目的发展，AI 将逐步接入：

- 本地 Wiki
- 应急指南
- 库存系统
- 成员信息
- 地图
- GPS
- LoRa
- 传感器
- OCR
- Vision
- Speech
- Study Terminal
- Field Terminal

如果所有逻辑继续堆积在一个 ai.py 中，系统最终将无法维护。

因此 LanternBox 采用分层架构。

整个 AI 的目标不是：

> 回答问题。

而是：

> 理解环境、整合知识、制定行动方案，并帮助使用者执行。

---

# 二、总体架构

```
                 User Input
                      │
                      ▼
          Context Engine（理解）
                      │
                      ▼
         Retrieval Layer（找资料）
                      │
                      ▼
         Planner Layer（制定方案）
                      │
                      ▼
        Response Layer（组织表达）
                      │
                      ▼
          LLM Client（模型通信）
```

整个系统的数据流永远保持单向。

任何模块都不能形成循环依赖。

---

# 三、各层职责

## 1. Context Engine（感知层）

### 作用

Context Engine 是整个 AI 的第一层。

它负责：

> 理解用户正在发生什么。

它并不回答问题。

也不负责寻找答案。

它只负责：

把用户的一句话，转换成机器可以理解的 Context。

例如：

```
水只剩一桶了，而且今天很热。
```

转换成：

```
input_type:
    situation

domains:
    water
    weather
    health

signals:
    water_shortage
    heat_exposure

risk_level:
    HIGH

risks:
    dehydration
    heatstroke

retrieval_plan:
    节水
    饮水优先级
    中暑预防
```

### 输入

用户输入

历史上下文

（未来）

成员状态

库存状态

地图

传感器

天气

时间

位置

### 输出

统一的数据结构：

```
LanternContext
```

以后所有模块都读取这个对象。

而不是重新理解用户输入。

---

### Context Engine 不负责

❌ 检索知识

❌ 排序资料

❌ 组织 Prompt

❌ 调用 LLM

❌ 输出自然语言

---

# 2. Retrieval（检索层）

### 作用

Retrieval 负责：

> 找资料。

它永远不会回答问题。

它只负责：

"有哪些资料最值得给 Planner。"

例如：

用户：

```
水只剩一桶了。
```

Retrieval 会：

找到：

- 应急指南
- Wiki
- 库存信息
- 成员资料
- 历史记录

然后进行：

过滤

排序

重排

最终返回：

```
Candidate References
```

---

### Retrieval 包含

query

domains

references

reranker

strategy（未来）

---

### Retrieval 输入

LanternContext

Guide

Wiki

Inventory

History

---

### Retrieval 输出

```
Selected References
```

包括：

Guide

Wiki

Inventory

Member

History

等。

---

### Retrieval 不负责

❌ 推理

❌ 回答

❌ Prompt

❌ LLM

---

# 关于 Context Boost

目前 Retrieval 中存在：

```
context_boost.py
```

它属于：

过渡方案。

当前作用：

利用 Context Engine 的结果，对候选资料进行加权。

未来：

它将升级为：

```
strategy.py
```

负责：

根据 LanternContext

生成完整 Retrieval Strategy。

而不是：

检索完成以后临时加分。

这是 Retrieval 的长期演进方向。

---

# 3. Planner（规划层）

Planner 是整个 LanternBox AI 的核心。

也是普通 RAG 没有的一层。

它负责：

根据：

Context

Retrieval

Inventory

Members

Sensors

制定：

Action Plan。

Planner 不关心语言。

它输出的是：

```
Goals

Immediate Actions

Forbidden Actions

Observations

Questions

Priority
```

Planner 永远不要写：

```
if 水不够：
```

而应该基于：

通用行动原则。

例如：

资源不足

↓

降低消耗

↓

保证生命支持

↓

寻找替代来源

↓

建立观察指标

Planner 将来会逐渐演化成：

LanternBox 的真正"大脑"。

---

# 4. Response（响应层）

Response 的职责只有一个：

把已有的信息组织成适合 LLM 理解的 Prompt。

Response 不进行推理。

它只是：

整理。

组合。

表达。

Response 当前负责：

Prompt

History

Fallback

Safety

Context Block

Response 输入：

Planner

Retrieval

Context

输出：

Prompt Messages。

---

# 5. LLM Client（模型层）

LLM Client 是整个系统最底层。

职责只有一个：

和模型通信。

例如：

```
call_ollama()

stream_ollama()
```

它不知道：

Guide

Wiki

Planner

Inventory

Members

Context

等等。

以后即使模型换成：

OpenAI

vLLM

TensorRT

MLX

LM Studio

也只需要修改这一层。

---

# 四、依赖原则

允许：

```
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

禁止：

```
Retrieval
↓
Response
```

禁止：

```
Response
↓
Retrieval
```

禁止：

```
Planner
↓
LLM
```

禁止：

```
Context
↓
Response
```

所有模块只能向下调用。

不能反向调用。

---

# 五、当前目录结构

```
api/

context/
    schema.py
    rules.py
    engine.py

retrieval/
    constants.py
    query.py
    domains.py
    references.py
    reranker.py
    context_boost.py

planner/
    （预留）

response/
    prompts.py
    context_blocks.py
    safety.py
    fallback.py

llm/
    client.py

ai.py
```

---

# 六、ai.py 的定位

ai.py 不再承担业务逻辑。

它只是：

兼容层。

统一入口。

重新导出旧接口。

最终目标：

控制在 100 行以内。

以后任何新增业务，

原则上都不能写进 ai.py。

---

# 七、如何判断代码应该放哪一层？

开发新功能时，请先问自己：

### 它是在理解用户吗？

放：

Context。

---

### 它是在找资料吗？

放：

Retrieval。

---

### 它是在决定下一步行动吗？

放：

Planner。

---

### 它是在组织 Prompt 吗？

放：

Response。

---

### 它只是调用模型吗？

放：

LLM。

---

如果无法回答，

说明职责还没有划分清楚。

不要开始写代码。

---

# 八、未来路线

当前：

✅ Context

✅ Retrieval

✅ Response

⬜ Planner

⬜ Memory

⬜ Agent

⬜ Multi-Agent

未来 LanternBox 将逐步演化为：

```
Input

↓

Perception

↓

Knowledge

↓

Planning

↓

Reasoning

↓

Execution

↓

Learning
```

最终目标不是：

聊天机器人。

而是：

一个能够长期运行、持续学习、辅助决策、帮助团队生存与重建的离线智能系统。