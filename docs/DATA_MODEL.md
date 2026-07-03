# LanternBox Data Model

Version: 1.0
Status: Active
Last Updated: 2026-07-03

---

## 1. Purpose

本文档定义 LanternBox 的数据模型。本文档不定义具体数据库实现，不绑定 PocketBase、SQLite、PostgreSQL 或 ORM。

任何数据库调整，包括新增字段、新增实体、字段改名、实体关系变化、状态枚举变化，都必须同步更新本文档。

---

## 2. Common Fields

公共字段：

- id
- created_at
- updated_at
- version
- status

常用 status：

- draft
- review
- published
- archived

---

## 3. Guide

字段：

- id
- slug
- title
- category
- summary
- content
- difficulty
- risk_level
- tags
- related_wiki
- related_medicine
- related_tools
- references
- status
- version

Guide 是行动知识。

---

## 4. Wiki

字段：

- id
- slug
- title
- category
- summary
- content
- tags
- related_guides
- related_wiki
- related_assets
- references
- status
- version

Wiki 是能力知识。

---

## 5. Medicine

字段：

- id
- generic_name
- trade_name
- aliases
- dosage_form
- indication
- contraindication
- side_effect
- dosage
- storage
- shelf_life
- replacement
- barcode
- qr_code
- related_disease
- related_guide
- related_wiki

---

## 6. Disease

字段：

- id
- name
- aliases
- symptoms
- diagnosis_hint
- emergency_level
- treatment
- related_medicine
- related_guide
- related_wiki

疾病实体用于知识查询，不替代专业医疗判断。

---

## 7. Inventory

字段：

- id
- name
- category
- quantity
- unit
- location
- source
- expire_date
- minimum
- note
- related_medicine
- related_guide

Inventory 记录已经拥有的物资。

---

## 8. Resource

字段：

- id
- type
- name
- latitude
- longitude
- description
- risk
- accessibility
- reliability
- related_map_point
- related_guide
- related_task

Resource 记录环境中的资源。

---

## 9. MapPoint

字段：

- id
- latitude
- longitude
- type
- title
- description
- danger_level
- related_resource
- related_task
- related_guide
- related_wiki
- created_by
- verified

---

## 10. Member

字段：

- id
- nickname
- role
- skills
- medical_info
- contact
- status
- permissions
- training_record

成员医疗信息属于敏感数据，应加密或权限控制。

---

## 11. Task

字段：

- id
- title
- description
- type
- priority
- status
- assigned_to
- related_location
- related_resource
- related_guide
- created_by
- due_time
- result
- experience

---

## 12. Experience

字段：

- id
- title
- author
- category
- content
- related_task
- related_resource
- related_map_point
- verified
- review_status
- target

Experience 审核后可进入 Guide、Wiki 或 Archive。

---

## 13. Knowledge Inbox

字段：

- id
- source
- type
- title
- summary
- content
- asset
- review_status
- target

Inbox 是待整理知识，不参与默认 AI 检索。

---

## 14. Assets

字段：

- id
- filename
- type
- size
- checksum
- storage_path
- related_entity
- description

Assets 包括图片、PDF、音频、视频、流程图等。

---

## 15. Runtime Settings

字段：

- key
- value
- category
- updated_at

用于 AI 开关、低功耗模式、默认地图、终端状态等。

---

## 16. Device

字段：

- id
- device_name
- device_type
- hardware
- firmware
- public_key
- last_online
- status

设备类型包括 Core、Field、Study、Sensor、Communication。

---

## 17. Communication Message

字段：

- id
- sender
- receiver
- type
- content
- timestamp
- encrypted
- sync_status

---

## 18. AI Conversation

字段：

- id
- session_id
- role
- content
- timestamp
- mode
- related_context
- related_knowledge

AI Conversation 是历史记录，不是长期知识。

---

## 19. Relationships

主要关系：

```
Guide ↔ Wiki
Guide ↔ Medicine
Guide ↔ Task
Wiki ↔ Wiki
Task ↔ Member
Task ↔ MapPoint
Resource ↔ MapPoint
Experience → Guide / Wiki / Archive
KnowledgeInbox → Guide / Wiki / Archive
Device ↔ CommunicationMessage
```

---

## 20. Design Principles

- Data Model 独立于数据库实现；
- Core 保存正式数据；
- 终端只保存缓存和离线数据；
- AI 不直接保存长期知识；
- 实体通过引用建立关系；
- 高风险数据需要权限和安全控制；
- 数据库调整必须同步更新本文档。
