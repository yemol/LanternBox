# Security

## 密钥文件

- `Ft01Secrets.h`：本地真实频道材料，禁止提交。
- `Ft01Secrets.example.h`：公开模板，可以提交。
- `.gitignore` 已排除 `Ft01Secrets.h`，但如果该文件曾被 Git 跟踪，必须先执行：

```bash
git rm --cached Ft01Secrets.h
```

随后立即轮换已经暴露过的 Meshtastic 频道 PSK。

## 当前加密层

### 频道广播

- Meshtastic Data payload 使用私有频道 32 字节 PSK进行 AES-256-CTR 加密。
- 适用于广播消息和频道 NodeInfo 流程。

### 点对点私信

- FT01 首次启动生成 X25519 密钥对。
- 私钥保存在 ESP32 NVS，不写入源码或串口日志。
- NodeInfo 广播 32 字节公钥。
- 共享秘密经 SHA-256 派生后用于 AES-256-CCM。
- CCM 认证失败时不显示正文，也不写入正常消息记录。

## 送达确认与重放边界

- 发给 FT01 且请求 ACK 的有效单播包会返回 `ROUTING_APP` 确认。
- 消息以 `(发送节点, Packet ID)` 去重，中继副本不会重复入箱。
- 重复副本仍允许触发必要 ACK，避免确认包丢失后无法恢复。

## 隐私边界

- 无线头中的来源节点、目标节点、Packet ID、信道哈希和路由信息并非全部加密。
- GNSS 坐标只有用户主动发送消息时才附加，并位于加密正文中。
- 串口日志不得输出消息正文、私钥、公钥原文或 GNSS 坐标。

## 发布前检查

```bash
git status --ignored
git ls-files Ft01Secrets.h
```

第二条命令必须没有输出。还应搜索仓库中是否存在 32 字节真实密钥副本、频道 URL、二维码截图或导出的频道配置。
