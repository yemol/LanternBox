# LanternBox AI Modes

Version: v0.1  
Status: Active  
Last Updated: 2026-06-27

---

# 一、设计原则

LanternBox 不应让所有 AI 交互都强制走同一套复杂流程。

不同模式有不同目标：

- 应急模式需要可靠、谨慎、可执行。
- 陪伴模式需要轻量、自然、低压力。
- 学习模式需要知识检索、解释和教学结构。
- 地图、能源、成员等模式未来会接入不同数据源。

因此，AI 架构必须是 **Mode-aware（按模式启用）** 的。

---

# 二、当前模式总览

```text
Emergency Mode      应急模式
Daily Assistant     日常助手模式
Knowledge Mode      百科 / 知识模式
Study Mode          教学 / 学习模式
Navigation Mode     导航 / 地图模式
Companion Mode      陪伴模式
Energy Mode         设备 / 能源模式
Team Mode           成员 / 团队模式
```

---

# 三、各模式定义

## 1. Emergency Mode / 应急模式

### 定位

用于断网、断电、断水、灾害、野外、避难、低资源或长期失去外部支持的场景。

### 是否使用完整 AI 架构

是。

```text
Context Engine
↓
Retrieval
↓
Planner
↓
Response
↓
LLM Client
```

### 核心要求

- 默认无外部支援。
- 优先本地可执行措施。
- 优先风险判断和行动步骤。
- 禁止把城市公共服务作为默认方案。
- 必须结合本地指南、Wiki、库存、成员、日志等资料。

---

## 2. Companion Mode / 陪伴模式

### 定位

用于低压力交流、情绪陪伴、休息提醒、简单对话。

### 是否使用完整 AI 架构

否。

陪伴模式保持轻量：

```text
User
↓
Simple Prompt
↓
LLM Client
```

### 核心要求

- 不强制走 Retrieval。
- 不强制 Planner。
- 不主动把轻松聊天升级成应急判断。
- 不做复杂资源分析。
- 保持自然、简洁、低负担。

---

## 3. Daily Assistant Mode / 日常助手模式

### 定位

用于普通日常记录、提醒、物资管理、简单建议。

### 是否使用完整 AI 架构

部分使用。

可以使用轻量 Context，但默认不需要完整 Planner。

```text
Light Context
↓
Optional Retrieval
↓
Response
↓
LLM Client
```

### 未来方向

适合接入：

- 日常记录
- 物资库存
- 简单提醒
- 低风险建议

---

## 4. Knowledge Mode / 百科 / 知识模式

### 定位

用于查询 Wiki、Kiwix、手册、工具百科、种植百科等离线知识。

### 是否使用完整 AI 架构

部分使用。

重点是 Retrieval，不一定需要 Planner。

```text
Context
↓
Retrieval
↓
Response
↓
LLM Client
```

### 核心要求

- 优先引用本地知识库。
- 区分 PocketBase 精选 Wiki 和 Kiwix / ZIM 大库。
- 不编造知识来源。
- 适合解释、总结、对比、翻译成本地可操作知识。

---

## 5. Study Mode / 教学 / 学习模式

### 定位

用于 Study Terminal、离线阅读、课程学习、手写笔记、语音笔记、教学式解释。

### 是否使用完整 AI 架构

未来需要。

学习模式很可能使用类似完整管线，但 Planner 会变成 Learning Planner。

```text
Context
↓
Retrieval
↓
Learning Planner
↓
Teaching Response
↓
LLM Client
```

### 未来方向

适合接入：

- 学习资料
- 笔记
- Wiki
- Kiwix
- 课程计划
- 复习提醒

---

## 6. Navigation Mode / 导航 / 地图模式

### 定位

用于离线地图、GPS / 北斗、资源点、风险点、路线规划。

### 是否使用完整 AI 架构

未来需要专用管线。

```text
Location Context
↓
Map Retrieval
↓
Route / Risk Planner
↓
Response
```

### 未来方向

适合接入：

- 离线地图
- Field Terminal
- GPS / 北斗
- 资源点
- 危险区域
- 路线记录

---

## 7. Energy Mode / 设备 / 能源模式

### 定位

用于电量、功耗、太阳能输入、设备耗电排行、节能建议。

### 是否使用完整 AI 架构

未来需要专用管线。

```text
Sensor / Power Context
↓
Energy Analysis
↓
Energy Planner
↓
Response
```

### 核心要求

- 能源优先。
- 最小可用能力优先。
- 低电量时降低 AI / OCR / Vision 等高耗能模块优先级。

---

## 8. Team Mode / 成员 / 团队模式

### 定位

用于成员档案、医疗信息、任务、权限、团队协作。

### 是否使用完整 AI 架构

未来需要，但必须谨慎。

### 核心要求

- 默认加密。
- 测试只使用假数据。
- 不在普通 Prompt 中暴露敏感成员信息。
- 成员数据访问必须有权限边界。

---

# 四、当前实施优先级

```text
P0 Emergency Mode
P1 Companion Mode
P2 Knowledge Mode
P3 Study Mode
P4 Daily Assistant Mode
P5 Energy Mode
P6 Navigation Mode
P7 Team Mode
```

当前阶段：

```text
应急模式：完整架构优先落地
陪伴模式：保持轻量
学习模式：未来接入架构
```

---

# 五、重要原则

## 1. 不要把完整架构硬套给所有模式

完整架构适合应急、学习、导航、能源等高价值场景。

不适合每一次轻量聊天。

---

## 2. Companion Mode 不能被应急模式污染

陪伴模式不应默认：

- 检索应急指南
- 输出风险分级
- 生成行动方案
- 强行追问物资和环境

---

## 3. Study Mode 不是 Companion Mode

学习模式需要知识结构、复习计划、讲解方式。

它未来应独立设计，而不是复用陪伴模式 Prompt。

---

## 4. Emergency Mode 永远遵守无外部支援假设

应急模式默认：

- 无公网
- 无云端
- 无公共服务
- 无稳定救援
- 无现代供应链

回答必须优先本地可执行。

---

# 六、后续路线

下一步：

1. 继续完善 Emergency Mode 分层架构。
2. 建立 Planner。
3. 将 Knowledge Mode 从应急模式中拆出。
4. 为 Study Mode 单独设计 Learning Planner。
5. 为 Energy Mode 设计能源上下文结构。
6. 为 Navigation Mode 设计地图上下文结构。
7. 为 Team Mode 设计权限与加密边界。

---

# 七、总结

LanternBox 的 AI 不是一个单一聊天入口。

它是多个模式组成的离线智能系统。

不同模式共享底层能力：

- Context Engine
- Retrieval
- LLM Client

但不一定共享完整管线。

当前核心原则：

```text
应急模式完整
陪伴模式轻量
学习模式未来专门设计
```