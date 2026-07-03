# LanternBox Terminals

Version: 1.0
Status: Frozen
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 的终端体系，包括 Core、Field Terminal、Study Terminal、Sensor Node 和 Communication Node。

---

## 2. Core

Core 是中央核心，不是便携终端。

职责：

- 保存正式知识和数据；
- 运行 AI；
- 统一检索；
- 管理地图、资源、药物、成员、任务；
- 负责同步；
- 负责备份；
- 负责配置。

Core 可运行于当前 Mac mini 原型，未来可迁移到更低功耗、更坚固的硬件。

Core 是角色，不是固定设备。

---

## 3. Field Terminal

Field Terminal 是现场终端。

目标：

- 低功耗；
- 便携；
- 可靠；
- 可离线；
- 可同步。

最低能力：

- 查看 Guide；
- 查看基础 Wiki；
- 查看地图；
- 接收任务；
- 记录资源点；
- 记录危险点；
- 记录日志；
- 与 Core 同步；
- 查看电量和通信状态。

Field Terminal 不需要完整 AI，不保存完整数据库。

---

## 4. Study Terminal

Study Terminal 是学习终端。

目标：

- 长时间阅读；
- 低功耗；
- 本地资料；
- 学习和培训；
- 知识传承。

能力：

- 阅读 Wiki / Kiwix；
- 学习计划；
- 笔记；
- 技能训练；
- 新成员培训；
- 与 Core 同步。

Study Terminal 重点不是现场，而是长期学习。

---

## 5. Sensor Node

Sensor Node 是传感器节点。

职责：

- 采集温度、湿度、气压、水位、电量、空气质量等；
- 间歇上报；
- 本地缓存；
- 低功耗运行；
- 加密通信预留。

Sensor Node 不保存知识，不运行 AI。

---

## 6. Communication Node

Communication Node 用于扩展通信能力。

职责：

- LoRa；
- Mesh；
- Wi-Fi；
- USB；
- 短波资料与接口预留；
- 中继；
- 离线消息转发；
- 设备状态同步。

Communication Node 不包含业务逻辑。

---

## 7. Voice Node

Voice Node 是低功耗语音交互节点，属于 Human Interface。

目标：

- 本地唤醒；
- 离线 ASR；
- 离线 TTS；
- 调用 Core AI；
- 查询 Guide / Wiki / Medicine / Map / Task；
- 低功耗常备或按键唤醒。

Voice Node 不保存长期知识，不替代 Core。

---

## 8. Terminal Sync

所有终端都应通过 Protocol 与 Core 同步。

终端可缓存：

- 当前任务；
- Guide 子集；
- 地图子集；
- 离线日志；
- 当前配置。

正式数据最终回到 Core。

---

## 9. Hardware Principles

终端硬件原则：

- 不绑定具体型号；
- 可替换；
- 低功耗优先；
- 有线同步优先支持；
- 无线通信可选；
- 能源状态可见；
- 外壳和接口尽量可靠；
- 关键数据可备份。

---

## 10. Field Terminal First Version

第一代 Field Terminal 不追求完整功能。

建议先实现：

- 任务列表；
- Guide 查看；
- 地图点查看；
- 资源点记录；
- 危险点记录；
- 日志记录；
- 与 Core 同步。

完成后再考虑 AI 和语音能力。

---

## 11. Study Terminal First Version

第一代 Study Terminal 建议先实现：

- Wiki 阅读；
- Kiwix 阅读；
- 课程目录；
- 学习笔记；
- 与 Core 同步。

---

## 12. Device Registration

所有终端接入 Core 前必须注册。未登记设备默认不可信。注册信息应包含 Device ID、公钥、设备类型和权限。

---

## 13. Summary

Core 保存知识和正式数据。Field Terminal 把知识带到现场。Study Terminal 负责学习和传承。Sensor Node 采集环境。Communication Node 负责连接。Voice Node 降低交互门槛。

终端体系的目标是让 LanternBox 从知识中心变成真正可被团队使用的现场系统。
