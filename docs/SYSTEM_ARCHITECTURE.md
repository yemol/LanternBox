# LanternBox System Architecture

Version: 1.0
Status: Frozen
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 的整体系统架构。Blueprint 说明项目为什么存在，本文档说明系统如何组成。

---

## 2. Overall Architecture

LanternBox 采用 Core + Multi Terminal + Knowledge + AI 架构。

```
User
  ↓
Human Interface
  ↓
Field Terminal / Study Terminal / Voice Interface
  ↓
Communication Layer
  ↓
Core
  ↓
Application Modules
  ↓
Knowledge / AI / Navigation / Resource / Team / Data
```

Core 是唯一可信数据源，终端是 Core 的能力延伸。

---

## 3. Core

Core 保存正式数据和核心能力：

- Guide、Wiki、Kiwix；
- AI 引擎与检索；
- 地图与资源；
- 药物与库存；
- 成员、任务、日志；
- 配置、索引、备份。

Core 负责统一管理、同步、备份和 AI 推理。Core 不负责长期便携和现场交互。

---

## 4. Terminal

终端包括 Field Terminal 和 Study Terminal。

Field Terminal 用于外出、巡查、采集、任务、导航和通信。它应低功耗、便携、可离线缓存。

Study Terminal 用于阅读、教学、培训和知识传承。它强调低功耗、长时间阅读和学习体验。

终端不能长期保存完整数据库，正式数据最终同步回 Core。

---

## 5. Knowledge Layer

Knowledge Layer 包括 Guide、Wiki、Kiwix 和 Team Knowledge。

知识层必须独立于 AI。AI 不可用时，知识仍可查询。

---

## 6. AI Layer

AI 通过 Context Engine、Retrieval Engine、Planner 和 Response Builder 工作。

AI 不直接写数据库，不直接修改知识，只通过工具调用业务模块。

---

## 7. Navigation Layer

Navigation 管理空间信息：

- 地图；
- 资源点；
- 危险点；
- 路线；
- 营地；
- 任务区域。

地图点应能关联 Guide、Wiki、Resource、Task 和日志。

---

## 8. Communication Layer

Communication 负责消息、同步、任务包、地图点、资源点、日志和设备状态传输。

支持介质包括 USB、Wi-Fi、LoRa、Mesh、蓝牙和短波预留。通信层不包含业务逻辑。

---

## 9. Resource Layer

资源包括水、食物、药物、工具、能源、材料和消耗品。资源管理应记录数量、位置、来源、保质期、使用记录和替代方案。

---

## 10. Team Layer

团队层负责成员、技能、任务、值班、培训、日志和经验。团队经验经过审核后可进入 Knowledge。

---

## 11. Module Boundary

模块之间通过公开接口交互。不得直接修改其它模块内部数据。不得形成循环依赖。

推荐依赖方向：

```
Human Interface
  ↓
Application Modules
  ↓
Knowledge / AI / Navigation / Resource / Team
  ↓
Data Layer
  ↓
Infrastructure
```

---

## 12. Data Flow

正式数据流：

```
Input
  ↓
Validation
  ↓
Business Module
  ↓
Data Layer
  ↓
Synchronization
  ↓
Core
```

终端可缓存数据，但最终以 Core 为准。

---

## 13. Failure Strategy

任何单一模块故障不应导致系统整体不可用。

- AI 不可用：Guide、Wiki、地图仍可用；
- 通信中断：终端可离线缓存；
- 地图不可用：Guide 与 Wiki 不受影响；
- 高耗能任务暂停：核心知识仍可查。

---

## 14. Deployment

基础部署：

```
Core
  ├── FastAPI
  ├── Knowledge
  ├── AI
  ├── Data
  ├── Map
  └── Communication

Terminals
  ├── Field Terminal
  └── Study Terminal
```

未来可增加 Sensor Node 和 Communication Node。

---

## 15. Software Structure

推荐目录：

```
api/
  ai/
  context/
  retrieval/
  knowledge/
  guide/
  wiki/
  inventory/
  map/
  communication/
  member/
  task/
  medicine/
  system/

frontend/
data/
docs/
tests/
```

---

## 16. Architecture Constraints

- Core 只有一个正式可信源；
- Knowledge 不依赖 AI；
- AI 不直接写数据库；
- Terminal 只缓存必要数据；
- Communication 不包含业务逻辑；
- 数据库不直接暴露给 UI。

---

## 17. Summary

LanternBox 的系统架构可概括为：

Knowledge 是核心。  
Core 是中心。  
AI 是入口。  
Terminal 是延伸。  
Navigation 是空间组织。  
Communication 是连接。  
Documentation 是设计依据。
