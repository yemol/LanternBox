# LanternBox AI Services Layer

Version: v1.0  
Status: Active  
Last Updated: 2026-06-28

---

## 一、定位

Services Layer 是 LanternBox AI 的本地业务数据访问层。

它负责从本地数据源读取、整理、规范化业务数据。

Services 是 Pipeline / Retrieval 与底层数据源之间的缓冲层。

---

## 二、总体关系

```text
Pipeline Preload
  ↓
Services Layer
  ↓
PocketBase / JSON / Files / SQLite / Future Data Sources
```

---

## 三、当前目录结构

```text
api/services/
├── __init__.py
├── guide_service.py
└── wiki_service.py
```

未来扩展：

```text
inventory_service.py
member_service.py
map_service.py
sensor_service.py
log_service.py
```

---

## 四、Guide Service

文件：

```text
api/services/guide_service.py
```

职责：

- Trigger 匹配
- 关联指南查找
- 指南文本规范化
- 指南领域提取
- 指南兼容性判断
- 指南序列化

当前关键能力：

```text
match_triggers
find_related_guides
guide_core_text
guide_full_text
guide_domains
guide_compatible_with_domains
```

Guide Service 已接管触发规则匹配、关联指南查找、指南文本规范化、领域兼容判断与基础指南结构处理。

指南检索与打分属于：

```text
api/retrieval/guide.py
```

---

## 五、Wiki Service

文件：

```text
api/services/wiki_service.py
```

职责：

- Wiki AI 检索
- Wiki 词表扩展
- Wiki 文章规范化
- Wiki 过滤
- Wiki 分类查询
- 分类文章查询

当前关键能力：

```text
search_wiki_for_ai
search_wiki_articles
normalize_wiki_article
normalize_wiki_articles_for_ai
build_wiki_search_terms
build_wiki_or_filter_for_terms
clean_wiki_search_term
filter_related_wikis_for_query
get_wiki_categories_records
get_wiki_articles_by_category_records
```

`WIKI_DOMAIN_TERMS` 属于 Wiki Service 的业务词表。

如果未来词表变大，可迁移到：

```text
api/services/wiki_terms.py
```

---

## 六、PocketBase Client

文件：

```text
api/pocketbase_client.py
```

职责：

- 统一 PocketBase HTTP 访问
- 提供底层数据请求方法

当前建议接口：

```text
pb_get
pocketbase_get_records
pocketbase_get_record
```

routes.py 和 wiki.py 不应维护自己的 PocketBase 请求逻辑。

---

## 七、Services 不是什么

Services 不负责：

- Pipeline 调度
- Prompt 组织
- LLM 调用
- HTTP 路由
- Planner 决策
- AI Response 清洗

---

## 八、依赖原则

允许：

```text
pipeline/preload.py → services/*
retrieval/* → services/*
services/* → pocketbase_client / files / local data
```

禁止：

```text
services/* → pipeline/*
services/* → response/*
services/* → llm/*
services/* → routes.py
```

---

## 九、未来服务

### Inventory Service

负责：

- 物资库存快照
- 低库存预警
- 资源配给参考

### Member Service

负责：

- 成员状态
- 医疗注意事项
- 特殊照护需求
- 权限边界

### Map Service

负责：

- 离线地图
- 资源点
- 风险点
- 路线信息

### Sensor Service

负责：

- 温度
- 湿度
- 气压
- 电量
- GPS / LoRa 状态

### Log Service

负责：

- 日志查询
- 事件记录
- 时间线
- 复盘数据

---

## 十、核心原则

```text
Pipeline 组织流程
Services 提供数据
Retrieval 选择来源
Response 组织表达
LLM 生成语言
```
