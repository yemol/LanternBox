# ADR-001：LBX Knowledge Package（LanternBox 单文件知识包）

**状态：Accepted（已确认）**
**日期：2026-07-13**

---

## 背景

LanternBox 当前的知识体系主要由以下几部分组成：

* Guide（行动指南）
* PocketBase Wiki（精选知识）
* Kiwix / ZIM（大规模离线知识）

目前 PocketBase 负责知识的编辑、管理和维护。

随着 Field Terminal 等轻量终端的规划推进，我们需要一种更加适合部署和携带的知识格式。

PocketBase 更适合作为知识管理工具，而不是终端上的知识载体。

因此，LanternBox 需要一种单文件的知识包，用于终端部署和知识同步。

---

## 决策

LanternBox 引入 **LBX（LanternBox Knowledge Package）** 作为项目内部的标准知识包格式。

文件扩展名暂定为：

```text
.lbx
```

LBX 的作用是：

* 在终端上提供知识读取能力。
* 作为知识部署和分发格式。
* 方便复制、备份和携带。

LBX **不是知识编辑格式**。

知识仍然在 Core 中进行编辑和维护。

---

## 当前原则

目前确认以下原则：

1. Core 继续使用 PocketBase 管理知识。

2. PocketBase 与 LBX 的职责不同：

   * PocketBase 负责编辑。
   * LBX 负责部署。

3. 终端优先读取 LBX，而不是直接运行 PocketBase。

4. LBX 不替代 Kiwix / ZIM，两者承担不同职责。

---

## 目前已确定的事项

目前已经确定的只有以下几点：

* LBX 是 LanternBox 的单文件知识包。
* 它主要服务于终端部署。
* Core 仍然是唯一的知识编辑中心。
* 后续终端应急手册设计可以基于 LBX 进行组织。

除此之外，LBX 的具体文件结构、数据格式、构建流程和同步方式暂未确定，待后续设计。

---

## 后续工作

当终端知识系统开始开发时，再分别讨论：

* LBX 文件结构
* PocketBase 导出流程
* 终端读取方式
* 更新机制

这些内容将在确认方案后再形成新的架构文档，不在本决策中提前定义。
