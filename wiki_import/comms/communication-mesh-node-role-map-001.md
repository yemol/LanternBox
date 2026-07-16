---
title: Mesh 网络基础节点角色
slug: communication-mesh-node-role-map-001
category: 通讯
priority: P1
summary: 区分固定节点、移动节点、备用节点和记录点，避免所有节点同时移动导致网络失稳。
tags: Mesh, 节点角色, LoRa, 中继, 网络
guide_links: DG-0857, DG-0856
risk_level: caution
status: published
source: LanternBox internal curated knowledge
---
# Mesh 网络基础节点角色

## 用途

用于小团队理解简易 Mesh 或 LoRa 节点的角色。目标不是部署复杂网络，而是避免所有节点同时移动、断电或无记录。

## 操作步骤

1. 把节点分为固定、中继、移动和备用。
2. 在纸图上标出节点编号、角色、位置和负责人。
3. 固定节点不随外出队移动，移动节点必须登记离开时间。
4. 中继节点优先保持电量和稳定位置。
5. 节点角色变化后更新通信日志。

## 判断标准

- 可继续：至少有一个固定记录点和一条备用路径。
- 需调整：多个关键节点同时低电或同时移动。
- 停止条件：节点位置暴露、身份不清、连续失联或无人负责时，不把它作为关键中继。

## 风险提示

- 不要把 Mesh 当作自动可靠网络。
- 不要让外出队带走唯一中继。
- 不要把节点角色只存在某个人记忆里。

