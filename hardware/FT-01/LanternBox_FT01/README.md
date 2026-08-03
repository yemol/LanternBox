# LanternBox FT-01

FT-01 是 LanternBox 的便携式现场终端固件，当前开发基线为 **v0.5.2e**。工程运行于 M5Stack Cardputer-Adv，集成 GNSS、microSD、录音、任务、USB 同步以及基于 LoRa/Meshtastic 的加密通信。

## 当前能力

- CN35，478.875 MHz，BW250 / SF11 / CR5，SyncWord `0x2B`。
- 私有频道 AES-256-CTR 广播。
- X25519 + AES-256-CCM 点对点私信，支持送达 ACK。
- 手机端可识别 FT01 的真实 PKI 身份并显示绿色加密标记。
- 网络终端目录，最多缓存 64 个节点，支持滚动选择和定向私信。
- 50 条消息环形收件箱，`N/P` 首尾循环翻阅。
- 相同 `(发送节点, Packet ID)` 的中继副本只入箱一次。
- 节点名称显示顺序为 `short_name → long_name → 节点编号`。
- 发送时若 GNSS 定位有效且不超过 15 秒，坐标自动附加到同一条消息。
- LoRa 接收作为全局后台服务运行，离开通信页面仍可接收广播与私信。
- 首页显示 `M1` 至 `M9+` 未读消息提示。
- WAV 录音、日志回放、任务收取、路径记录和 USB Core 同步。

## 快速开始

1. 将 `Ft01Secrets.example.h` 复制为 `Ft01Secrets.h`。
2. 填入 LanternBox 私有 Meshtastic 频道的 Channel Hash 与 32 字节 PSK。
3. Arduino IDE 选择：
   - Board：已验证的 M5Stack Cardputer-Adv
   - ESP32 Core：M5Stack 3.2.2 基线
   - Flash Size：`8MB`
   - Partition Scheme：`8M with spiffs (3MB APP/1.5MB SPIFFS)`
4. 编译并记录真实的 `Sketch uses`、`Maximum` 和 RAM 输出。
5. 烧录后按 [测试清单](docs/TESTING.md) 完成回归。

> `Ft01Secrets.h` 已被 `.gitignore` 排除，正式仓库中只保留示例文件。真实频道密钥不得提交。

## 通信页按键

| 页面 | 按键 | 功能 |
|---|---|---|
| 消息收件箱 | `T` / Enter | 编写频道广播 |
| 消息收件箱 | `M` / `O` | 打开网络终端目录 |
| 消息收件箱 | `N` / `P` | 环形翻阅消息 |
| 消息收件箱 | `B` | 请求节点信息与公钥 |
| 消息收件箱 | `D` | 打开或关闭诊断页 |
| 网络终端 | 方向键 / `N` / `P` | 滚动选择节点 |
| 网络终端 | Enter | 给选中节点发送 PKI 私信 |
| 网络终端 | `T` | 编写频道广播 |
| 编辑页面 | Enter | 发送 |
| 编辑页面 | Del | 删除字符 |
| 任意子页面 | `` ` `` / Del | 返回或取消 |

## 文档

- [版本历史](CHANGELOG.md)
- [安全与密钥](SECURITY.md)
- [系统架构](docs/ARCHITECTURE.md)
- [通信协议与行为](docs/COMMUNICATION.md)
- [构建与发布](docs/BUILD_AND_RELEASE.md)
- [录音、任务、存储与同步](docs/SUBSYSTEMS.md)
- [测试与验收](docs/TESTING.md)

## 当前边界

- 50 条消息与 64 个节点目录主要保存在 RAM/NVS，消息记录重启后不保留。
- GNSS 坐标当前作为文本尾部发送，不是独立 `POSITION_APP` 数据包。
- Meshtastic 无线包头仍可被观察，内容由频道 PSK或 PKI 加密保护。
- v0.5.2e 的全局后台收包需要重点回归录音期间的 SPI/音频稳定性。
