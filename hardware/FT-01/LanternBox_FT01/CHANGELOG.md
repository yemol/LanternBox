# Changelog

本文件合并原工程中分散的版本说明和测试报告。只记录可长期追踪的功能变化，细碎试验过程由 Git 历史承担。

## v0.5.2e

### Background Radio Service

- LoRa 初始化与非阻塞接收提升为全局后台服务。
- 首页、地图、录音、日志、任务、同步和帮助页面均可继续收包。
- 通信页面只负责展示与操作，不再拥有无线接收生命周期。
- 新增未读计数与首页 `M1` 至 `M9+` 提示。
- LoRa 初始化失败后每 30 秒后台重试。

## v0.5.2d

### GNSS Message Attachment

- 广播和 PKI 私信共用同一套位置附加逻辑。
- 仅在 GNSS FIX 有效、坐标合法且最近 15 秒内更新时附加位置。
- 坐标格式：`[GPS:纬度,经度]`。
- GNSS 不可用时仍发送纯文字，不阻塞通信。

## v0.5.2c

### Circular Message Navigation

- 50 条消息的用户浏览也改为环形。
- 最新消息继续按 `N` 跳到最旧消息。
- 最旧消息继续按 `P` 跳回最新消息。

## v0.5.2b

### Short Name Priority

- 所有通信页面统一采用 `short_name → long_name → 节点编号`。

## v0.5.2a

### Node Directory and Direct Message

- 新增网络终端目录，最多缓存 64 个节点。
- 每屏 5 个节点，支持方向键与 `N/P` 滚动。
- 选中具备公钥的节点后可发送 X25519 + AES-256-CCM 私信。
- 保留 AES-256 私有频道广播入口。

## v0.5.1g

### 50-Record History

- 消息环形缓冲区扩展至 50 条。
- 第 51 条覆盖最旧记录，不动态分配内存。

## v0.5.1f

### Node Identity and De-duplication

- 以 `(from, Packet ID)` 作为消息去重键。
- 中继副本仍可完成 ACK，但不重复写入收件箱。
- 解析并校验 NodeInfo 中的 `User.id`、名称和公钥。
- 学习名称后刷新现有消息的来源显示。

## v0.5.1e

### ACK, Non-blocking RX and Node Names

- 实现标准 Meshtastic `ROUTING_APP` 送达确认。
- 阻塞式 `receive()` 替换为 DIO1 中断与 `startReceive()/pollReceive()`。
- 修复通信页面按键数秒无响应的问题。
- 节点名称与公钥持久化到 NVS。

## v0.5.1d

### PKI Inbox

- NodeInfo 设置 `want_response=true`，请求其他节点回传资料。
- 私信先到、公钥后到时暂存密文，公钥到达后自动补解密。
- 主通信页面从射频调试仪改为消息正文收件箱。
- 底层 From/To/Packet/RSSI/SNR 搬入诊断页。

## v0.5.1a

### PKI Identity

- 生成并持久化 X25519 身份密钥。
- NodeInfo 广播真实 `public_key`，并声明 `is_unmessagable=false`。
- 支持 X25519 → SHA-256 → AES-256-CCM 的 PKI 私信解密。
- 手机端 FT01 节点可显示真实绿色加密标记。

## v0.5.0a 至 v0.5.0e

### Live Input and Structural Refactor

- 固定测试消息替换为 Cardputer 键盘真实输入。
- 私有频道正文继续使用 AES-256-CTR。
- 公共硬件、UI、文本、时间和音频存储逻辑抽取为独立模块。
- 清理约 11.5% 的重复源码。
- 修复 ESP32 Core 3.2.2 `String(double, uint8_t)` 重载冲突。
- 修复 `autoNextText()` 链接缺失，建立完整对象链接门禁。

## v0.4.5b 至 v0.4.9e

### Meshtastic RF and Native TX Milestones

- 确认 CN35 / 478.875 MHz / BW250 / SF11 / CR5 / SyncWord `0x2B`。
- 完成原始帧抓取、Mesh 头解析、帧收件箱和重复包观察。
- 从 LBX1 试验协议迁移到原生 Meshtastic RF 包与 Data protobuf。
- 手机实机收到 FT01 原生加密频道消息和 NodeInfo。
- 去除串口与 UI 中的明文预览。

## v0.3.x 至 v0.4.4c

### Stable Subsystems

- WAV 录音、保存、回放、删除和音频索引维护稳定。
- 任务详情、任务列表与任务报告完成。
- USB 任务下行从突发发送演进为逐行 ACK、可恢复和保存后确认。
- Core 确认上传后才清理路径、日志、任务报告和已上传音频。
- 音频文件删除采用显式确认，避免误清空音频索引。
