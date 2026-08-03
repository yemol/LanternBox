# 通信协议与界面

## RF 基线

| 参数 | 值 |
|---|---|
| 区域/信道 | CN35 |
| 频率 | 478.875 MHz |
| 带宽 | 250 kHz |
| 扩频因子 | SF11 |
| 编码率 | CR5 |
| SyncWord | `0x2B` |
| Hop Limit | 3 |

该配置已经在 FT01 与 Heltec V3 / FT02 Meshtastic 网络之间完成实机接收与发送验证。

## 消息类型

### 频道广播

```text
用户文字
→ 可选 GNSS 尾部
→ Data(TEXT_MESSAGE_APP)
→ 私有频道 AES-256-CTR
→ Broadcast
```

### 点对点私信

```text
选择网络节点
→ 用户文字 + 可选 GNSS 尾部
→ Data(TEXT_MESSAGE_APP)
→ X25519 共享秘密
→ SHA-256 密钥派生
→ AES-256-CCM
→ 指定节点 + want_ack
```

### 送达 ACK

FT01 成功认证并处理请求确认的单播包后返回：

- `ROUTING_APP`
- `Routing.error_reason = NONE`
- `Data.request_id = 原 Packet ID`
- 目标为原发送节点

ACK 的 ACK 不继续请求确认，避免确认回环。

## NodeInfo 与名称

FT01 NodeInfo 包含：

- `User.id`
- `long_name`
- `short_name`
- `public_key`
- `is_unmessagable=false`

收到其他节点 NodeInfo 时校验 `User.id` 与无线包来源。显示顺序为：

```text
short_name → long_name → 节点编号
```

未知节点或缺少公钥时可按 `B` 请求 NodeInfo。私信先到、公钥后到时，FT01 暂存密文，公钥到达后自动补解密。

## 去重

Meshtastic 广播可能同时以直达包和中继副本抵达。FT01 使用：

```text
(发送节点, Packet ID)
```

作为去重键。重复包不再次写入消息收件箱，但仍可完成必要的 ACK 处理。

## 后台接收

`serviceLoRaBackground()` 在主循环持续执行：

- 不进入通信页面也能收广播与私信。
- 控制帧不增加未读计数。
- 正文消息增加未读计数，首页显示 `M1` 至 `M9+`。
- 进入通信页后标记当前消息已读。
- 无线初始化失败时每 30 秒重试。

## GNSS 附加

坐标仅在以下条件同时满足时附加：

1. 当前 GNSS FIX 有效。
2. 最后有效定位距发送时不超过 15 秒。
3. 纬度位于 `[-90, 90]`。
4. 经度位于 `[-180, 180]`。
5. 坐标不同时为 `0,0`。

格式：

```text
<正文> [GPS:31.210449,121.381342]
```

位置属于同一条 `TEXT_MESSAGE_APP`，不会额外生成第二条收件箱记录。

## 页面

### 消息收件箱

只显示发送者、加密类型、正文、序号、时间和链路摘要。`N/P` 首尾循环。

### 网络终端

- 最多 64 个节点。
- 每屏 5 个节点。
- 方向键、`N/P` 滚动。
- `PKI` 表示可发送私信，`---` 表示尚无公钥。

### 诊断页

显示 RF 配置、PKI Peers、Pending、Decrypt OK、ACK、Dup、Packet ID、RSSI 与 SNR。诊断信息不占用主消息页面。
