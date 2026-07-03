# LanternBox Development Roadmap

Version: 1.0
Status: Active
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 当前开发路线。Roadmap 可随项目推进更新。

---

## 2. Current State

当前已具备：

- FastAPI 后端；
- 基础 Web UI；
- Guide / Wiki 基础体系；
- PocketBase Wiki；
- Ollama / 本地 AI；
- Hybrid RAG 基础；
- 日常记录；
- 物资库存；
- AI 助手页面。

---

## 3. Phase 1：Knowledge Foundation

目标：完成知识核心。

任务：

- 完善 Guide；
- 完善 Wiki；
- 固化 Metadata；
- 提升 AI 检索；
- 建立 Knowledge Inbox；
- 整理医疗、药物、工具、导航、通信等关键知识分类。

完成标准：

- AI 可稳定引用 Guide / Wiki；
- Guide 能覆盖高频场景；
- Wiki 可持续扩展；
- 知识分类与能力体系一致。

---

## 4. Phase 2：Navigation System

目标：让知识进入空间场景。

任务：

- 离线地图页面；
- 资源点；
- 危险点；
- 临时营地；
- 路线记录；
- 地图点详情；
- 地图点关联 Guide / Wiki / Task / Resource。

完成标准：

地图成为资源、风险和任务的入口。

---

## 5. Phase 3：Communication System

目标：完成 Core 与终端协作基础。

任务：

- Core ↔ Terminal 同步；
- 消息；
- 任务同步；
- 地图点同步；
- 日志同步；
- 设备身份；
- 加密预留；
- LoRa / USB / Mesh 接口预留。

完成标准：

终端可离线记录并同步回 Core。

---

## 6. Phase 4：Field Terminal

目标：完成第一代个人终端原型。

任务：

- 查看 Guide；
- 查看基础 Wiki；
- 查看地图；
- 接收任务；
- 记录资源点；
- 记录危险点；
- 回传日志；
- 与 Core 同步；
- 低功耗运行。

完成标准：

知识能跟随成员进入现场。

---

## 7. Phase 5：Voice Interface

目标：完成离线语音交互接口。

任务：

- 本地唤醒；
- ASR；
- TTS；
- AI 对话；
- Guide 查询；
- Wiki 查询；
- 药物查询；
- 地图查询；
- 低功耗语音节点原型。

完成标准：

成员可通过语音查询核心知识和 AI。

---

## 8. Phase 6：Study Terminal

目标：支持学习、培训、知识传承。

任务：

- 阅读界面；
- 学习计划；
- 课程资料；
- 技能训练；
- 新成员培训；
- 长时间低功耗学习。

---

## 9. Development Principles

- 先完成，再优化；
- 先稳定，再扩展；
- 先工程，再美化；
- 不为未来想象增加当前复杂度；
- 当前阶段完成后再进入下一阶段；
- 新功能必须服务长期独立运行能力。

---

## 10. Current Priority

当前优先级：

1. Knowledge Foundation
2. Navigation System
3. Communication System
4. Field Terminal
5. Voice Interface
6. Study Terminal
