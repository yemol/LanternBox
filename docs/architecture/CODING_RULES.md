# LanternBox Coding Rules

Version: v1.0  
Status: Active  
Last Updated: 2026-06-28

---

## 一、核心原则

任何新代码在编写前，必须先回答：

> 它属于哪一层？

如果无法判断，不应开始写代码。

---

## 二、分层归属规则

### 用户理解

放入：

```text
api/context/
```

### 本地业务数据

放入：

```text
api/services/
```

### 检索、排序、候选池、重排

放入：

```text
api/retrieval/
```

### 行动计划

放入：

```text
api/planner/
```

### Prompt、回答组织、安全清洗

放入：

```text
api/response/
```

### 模型通信

放入：

```text
api/llm/
```

### 请求调度和模式分发

放入：

```text
api/pipeline/
```

### HTTP 接口

放入：

```text
api/routes.py
```

但 routes.py 不得承载业务逻辑。

---

## 三、禁止事项

禁止：

- routes.py 组织 AI 流程
- routes.py 直接访问 PocketBase 业务数据
- services 调用 LLM
- services 调用 pipeline
- services 调用 response
- retrieval 调用 response
- response 调用 retrieval
- llm/client.py 调用任何业务模块
- resources.py 新增业务功能
- 为了跑通而加入临时补丁
- 同一函数保留多份定义
- 为小问题过度新增层级

---

## 四、解决问题原则

必须提供完整修复，不提供临时补丁。

当遇到错误时，优先判断：

1. 是职责放错了吗？
2. 是函数语义混淆了吗？
3. 是依赖方向错误了吗？
4. 是重复定义导致覆盖了吗？
5. 是模块迁移不完整吗？

不要只为了消除报错而兼容错误设计。

---

## 五、函数迁移规则

迁移函数时必须：

1. 成组迁移同一功能簇。
2. 删除旧位置重复定义。
3. 更新所有调用方导入。
4. 跑编译检查。
5. 跑接口测试。
6. 更新文档。

禁止：

```text
复制一份到新文件，旧文件继续保留
```

---

## 六、文件大小规则

超过 1000 行的文件，不允许盲目拆分。

必须先做：

1. 函数地图
2. 依赖地图
3. 职责分类
4. 迁移计划
5. 成组迁移

---

## 七、提交规则

Git commit 默认使用中文。

推荐前缀：

```text
新增
修复
优化
重构
文档
测试
配置
版本
```

每完成一个稳定阶段必须提交。

---

## 八、测试规则

架构改动后至少运行：

```bash
python -m py_compile <changed files>
python -c "from api.app import app; print('ok')"
```

AI 相关改动后必须测试：

```text
/api/ai/advice
/api/ai/advice/stream
```

---

## 九、LanternBox 特殊规则

Emergency Mode 默认无外部支援。

回答中不得默认依赖：

- 供水公司
- 供电公司
- 物业
- 客服
- 外卖
- 快递
- 在线搜索

除非用户明确说明处于正常城市环境。

---

## 十、架构座右铭

```text
代码服从架构。
架构服务目标。
不做临时补丁。
不过度分层。
```
