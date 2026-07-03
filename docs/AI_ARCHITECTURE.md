# LanternBox AI Architecture

Version: 1.0
Status: Frozen
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox AI 如何工作。本文档不绑定具体模型。

---

## 2. AI Role

AI 负责理解用户、分析上下文、检索知识、组织信息、辅助规划和生成回答。

AI 不保存知识，不直接修改知识，不直接写数据库。

---

## 3. Pipeline

统一 AI Pipeline：

```
User Input
  ↓
Input Parser
  ↓
Context Engine
  ↓
Retrieval Engine
  ↓
Planner optional
  ↓
LLM
  ↓
Response Builder
  ↓
User
```

所有 AI Mode 共用 Pipeline。

---

## 4. Input Parser

输入包括文本、语音、二维码、条码、OCR、图片预留。Input Parser 输出统一 Query。

---

## 5. Context Engine

Context Engine 负责分析：

- 当前模式；
- 当前终端；
- 当前任务；
- 当前位置；
- 能源状态；
- 用户问题；
- 可用知识源。

Context Engine 不生成最终回答。

---

## 6. Retrieval Engine

检索顺序：

```
Guide → Wiki → Team Knowledge → Kiwix
```

LLM 不直接访问数据库，只接收 Retrieval Engine 生成的 Knowledge Package。

---

## 7. Planner

Planner 可选，用于复杂任务拆解、行动计划、长期流程、风险步骤和任务建议。普通问答可跳过 Planner。

---

## 8. LLM

LLM 可替换。可使用本地模型或未来其它模型。LLM 输入包括 Context、Knowledge Package 和 Prompt。LLM 输出 Raw Response。

---

## 9. Response Builder

Response Builder 负责最终整理：

- 格式；
- 引用；
- 风险提示；
- 来源；
- 相关 Guide / Wiki；
- 不确定性说明。

---

## 10. Modes

当前模式：

- Emergency：应急，优先 Guide、风险和行动。
- Assistant：日常助手，轻量。
- Study：学习，解释和教学。

模式只改变 Prompt、Context 和 Planner，不改变 Pipeline。

---

## 11. Memory

LanternBox 默认不采用永久 AI Memory。长期知识进入 Knowledge System。AI Memory 仅负责当前会话、当前任务和当前上下文。

---

## 12. Tool Interface

AI 可调用工具：

- knowledge_search；
- inventory；
- medicine；
- navigation；
- map；
- task；
- member；
- communication；
- settings；
- voice。

AI 只调用工具，不直接操作数据库。

---

## 13. Voice Interface

Voice Interface 属于 Human Interface。它负责语音输入、语音识别、语音输出和语音播报，并调用统一 AI Pipeline。

语音节点应低功耗、默认离线、可替换硬件、不依赖云服务。

---

## 14. Runtime Modes

AI 支持：

- Full Mode：完整能力；
- Standard Mode：常规能力；
- Low Power Mode：基础检索；
- Survival Mode：Guide / Wiki 优先，暂停高耗能功能。

---

## 15. Error Handling

AI 故障不得影响 Guide、Wiki、Map、Inventory、Task 等核心功能。AI 是增强能力，不是唯一入口。

---

## 16. API Suggestion

建议接口：

```python
analyze_context()
retrieve_knowledge()
run_planner()
generate_response()
call_tool()
run_mode()
voice_input()
voice_output()
```

名称可调整，职责应保持一致。

---

## 17. Summary

AI 是入口，Knowledge 是核心，Core 是中心。AI 不替代知识，只帮助人更快、更安全、更准确地使用知识。
