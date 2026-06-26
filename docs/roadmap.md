# LanternBox Roadmap / 近期路线

## 当前阶段目标

先把 Core-Proto-01 软件系统稳定下来，再逐步接入终端和传感器。

当前优先级：

1. 稳定 FastAPI 后端
2. 稳定前端页面
3. 稳定 PocketBase Wiki
4. 优化本地 AI 助手
5. 完善应急指南库
6. 建立本地知识引用卡片系统
7. 梳理迁移清单
8. 后续再推进终端同步和加密通信

## 近期开发重点

### P1 - 系统稳定

- 检查 FastAPI 路由注册
- 修复 /api/emergency 可能出现的 404
- 检查 /api/wiki
- 检查 /api/ai
- 统一项目启动方式
- 记录端口占用问题处理方式

### P2 - Wiki 与 RAG

- 维护 api/wiki.py 领域关键词
- 优化 AI 上下文检索
- 首页只展示轻量关联结果
- AI 助手页显示完整关联结果
- 后续加入引用卡片系统

### P3 - 应急指南库

- 深化应急指南内容
- 清洗去重
- 分类合并
- 支持展开查看
- 与 AI 助手联动

### P4 - 能源面板

未来需要能源面板，显示：

- 当前功耗
- 剩余电量
- 预计运行时间
- 模块耗电排行
- 太阳能 / 电池输入状态
- 节能建议

### P5 - 终端体系

后续接入：

- Field Terminal
- Study Terminal
- Sensor Node
- USB-C 有线同步
- Wi-Fi 内网同步
- LoRa 低速同步

### P6 - 安全通信

后续实现 LanternBox Protocol：

- Device ID
- 设备密钥
- 端到端加密
- nonce
- sequence_id
- timestamp
- AEAD / HMAC 校验
- 未登记设备拒绝通信