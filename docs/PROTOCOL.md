# LanternBox Protocol

Version: 1.0
Status: Draft
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 的通信与同步协议。协议不绑定具体通信介质。

支持介质包括 USB、Wi-Fi、LoRa、Mesh、蓝牙、短波预留。

---

## 2. Core Principles

- Core 是唯一可信数据源；
- 终端可以离线缓存；
- 所有同步最终回到 Core；
- 每个设备必须拥有唯一 Device ID；
- 敏感通信默认加密；
- 未登记设备默认不可信；
- 通信失败不得导致数据丢失；
- 所有消息必须可追踪、可校验、可重试。

---

## 3. Device Identity

设备字段：

- device_id
- device_name
- device_type
- public_key
- registered_at
- last_seen
- status

设备类型：

- Core
- Field Terminal
- Study Terminal
- Sensor Node
- Communication Node

---

## 4. Device Registration

注册流程：

```
Device
  ↓
Request Registration
  ↓
Core verifies user approval
  ↓
Core stores device_id and public_key
  ↓
Core returns registration token
  ↓
Device becomes trusted
```

新设备不得自动信任。

---

## 5. Message Format

统一消息结构：

```json
{
  "message_id": "uuid",
  "from": "device_id",
  "to": "device_id",
  "type": "message_type",
  "timestamp": "ISO-8601",
  "sequence": 1024,
  "payload": {},
  "signature": "optional"
}
```

---

## 6. Message Types

基础类型：

- ping
- sync_request
- sync_response
- task_update
- map_update
- resource_update
- log_update
- chat_message
- alert
- device_status

---

## 7. Sync Package

```json
{
  "package_id": "uuid",
  "device_id": "field-terminal-01",
  "created_at": "ISO-8601",
  "items": [
    {
      "entity": "Task",
      "operation": "update",
      "data": {}
    }
  ]
}
```

---

## 8. Sync Operations

支持：

- create
- update
- archive
- acknowledge

默认不物理删除，优先 archive。

---

## 9. Sync Direction

Terminal to Core：

- 任务记录；
- 地图点；
- 危险点；
- 资源点；
- 日志；
- 状态。

Core to Terminal：

- 任务；
- Guide 缓存；
- 地图数据；
- 配置；
- 消息。

Terminal to Terminal：

默认通过授权通信通道，并最终同步回 Core。

---

## 10. Offline Sync

离线流程：

```
Terminal
  ↓
Local Cache
  ↓
Generate Sync Package
  ↓
Send to Core
  ↓
Core validates
  ↓
Core applies changes
  ↓
Core returns result
  ↓
Terminal marks package synced
```

同步失败必须可重试。

---

## 11. Conflict Handling

冲突处理原则：

- 不直接覆盖；
- 保留两个版本；
- 标记 conflict；
- 交由用户或 Core 规则处理；
- 保留冲突日志。

---

## 12. Security

协议预留：

- Device ID；
- public_key；
- session key；
- nonce；
- sequence；
- timestamp；
- signature；
- AEAD 校验。

可选技术方向：

- X25519；
- Noise；
- HPKE；
- ChaCha20-Poly1305；
- AES-GCM。

---

## 13. Low Bandwidth Mode

LoRa / 短波等低带宽环境只同步：

- alert；
- task_update；
- map_update 摘要；
- device_status；
- short_message。

大文件、图片和长日志不通过低带宽链路同步。

---

## 14. Payload Rules

Payload 应尽量小。

大附件使用 Asset 引用，不直接放入消息。

所有消息应支持 checksum 或 signature。

---

## 15. Protocol Compatibility

协议升级应保持向前兼容。新增字段不得破坏旧设备解析。未知字段应被忽略而不是导致错误。

---

## 16. Summary

LanternBox Protocol 的目标不是高速通信，而是在外部网络失效时，保证团队仍能安全、可靠、可追踪地同步关键信息。
