# FT-02 v2.72c Stable Release

当前版本：`v2.72c`  
状态：**实机验证通过，作为当前稳定基线**。

本包保留 FT-02 完整固件工程，并将原先分散的测试、构建、引脚和格式说明合并到本文件。历史过程文档不再随发布包分发。

## 1. 当前稳定能力

### LoRa / Meshtastic

- Heltec Wireless Stick Lite V3 作为 LoRa 协处理器
- FT-02 TX `GPIO13` → WSL V3 RX `GPIO18`
- FT-02 RX `GPIO7` ← WSL V3 TX `GPIO17`
- FT-02 `GPIO6` → WSL V3 `RST / CHIP_PU`
- UART2 `115200`，RX buffer `8192 bytes`
- 启动时由 GPIO6 硬复位 LoRa 协处理器
- 使用已验证的 Meshtastic PROTO FullSync 流程
- NodeDB、后台收信、广播、PKI 私信、ACK / NAK
- 收件箱可直接 ENTER 回复原发送者
- 顶部状态栏第二行显示未读消息数
- 通讯页 `R` 可硬复位 LoRa 并重新同步
- 有有效定位时消息可附加 GNSS 坐标

### 中文输入法 Pinyin IME A4

- 完全离线拼音
- 413 个无声调基础拼音音节
- 连续拼音自动切分
- 231 条常用及现场通讯词组
- 词组优先、单字兜底
- 候选翻页
- 本地词频学习
- 词频学习结果保存到 SD，重启后恢复
- 中文 / 英文输入模式
- 中文标点自动转换
- 广播、私信、直接回复均支持中文输入

## 2. 中文输入操作

### 中文模式 `[中]`

- `a-z / A-Z`：输入拼音
- `1-5`：选择当前候选字 / 词
- `SPACE`：选择当前页首候选
- `6 / 7`：上一页 / 下一页
- `Fn + ← / →`：候选翻页
- `0`：活动拼音按英文原样上屏
- `DEL`：优先删除拼音；拼音为空后删除正文字符
- `ENTER`：有活动拼音时先上屏；拼音为空时发送
- `ESC`：先取消活动拼音，再取消整条消息
- `Sym + W`：切换 `[中]` / `[EN]`

连续拼音示例：

```text
nihao            → ni'hao            → 你好
shoudao          → shou'dao          → 收到
woyijingdaole    → wo'yi'jing'dao'le → 我已经到了
```

如果整词没有命中，会自动退回逐音节选择并保留剩余拼音，不丢输入内容。

### 中文标点

在 `[中]` 模式下：

```text
,  → ，
.  → 。
?  → ？
!  → ！
:  → ：
;  → ；
```

CardKB2 常用实际键：

```text
Sym + N → ，
Sym + M → 。
Sym + E → ？
Sym + 1 → ！
Sym + L → ：
Sym + K → ；
```

### 英文模式 `[EN]`

字母、数字和符号直接按 ASCII 上屏，不进入拼音候选，也不转换中文标点。

## 3. 拼音词频学习

学习文件：

```text
/lanternbox/input/pinyin_user_freq.dat
```

只保存候选学习信息，例如拼音键、候选、选择次数和最近使用时间，**不保存聊天正文**。

学习数据在 RAM 中累计，并在合适时机批量写入 SD。SD 不可用时不会影响正常拼音输入。

## 4. 通讯页面操作

### 收件箱

```text
T       编写广播
M       网络终端
S       通讯诊断
Z / D   较旧消息
C / X   较新消息
P / N   上一条 / 下一条
ENTER   直接回复当前消息发送者
B       返回首页
```

### 网络终端

```text
D / X   上下选择节点
ENTER   给选中节点发私信
T       编写广播
R       硬复位 LoRa + 重新 FullSync
B       回收件箱
```

回复广播消息时，只回复原发送者，不再次广播。目标缺少有效 PKI 公钥时不会偷偷降级成广播。

## 5. 顶部 LoRa 状态

无未读消息：

```text
LoRa
已连接
```

有未读消息：

```text
LoRa
1条消息
```

超过 9 条：

```text
LoRa
9+条消息
```

连接 / 同步阶段优先显示真实连接状态。

## 6. GNSS 提示

输入页面只使用简洁状态：

```text
GPS：已定位
```

或：

```text
GPS：无定位
```

不会显示“新鲜定位”等工程术语。

## 7. 关键硬件引脚

### CardKB2 / I2C

```text
SDA  GPIO47
SCL  GPIO21
ADDR 0x5F
```

### GNSS

```text
RX GPIO39
TX GPIO38
Baud 38400
```

### SD / FSPI

```text
SCK  GPIO42
MOSI GPIO2
MISO GPIO1
CS   GPIO41
```

### Audio / WM8960

```text
VCC   3V3_EXT
GND   GND
SDA   GPIO47
SCL   GPIO21
BCLK  GPIO14
WS    GPIO15
DOUT  GPIO16   ESP32 → WM8960
DIN   GPIO17   WM8960 → ESP32
```

WM8960 模块自带 24 MHz MCLK，FT-02 不连接 TXMCLK / RXMCLK。扬声器使用 `RP / RN` BTL 差分输出，`RN` 不接 GND。

CardKB2 与 WM8960 共用 I2C：

```text
CardKB2 0x5F
WM8960  0x1A
SDA     GPIO47
SCL     GPIO21
I2C     100 kHz
```

系统只初始化一次 `Wire`，音频驱动复用现有总线。

## 8. 语音日志格式

目录：

```text
/lanternbox/audio/
```

编号示例：

```text
AUD_FT02A_000001.wav
```

下一个编号：

```text
/lanternbox/audio/audio_sequence.txt
```

索引：

```text
/lanternbox/audio/audio_index.jsonl
```

WAV：

```text
PCM / 44.1 kHz / 16-bit / mono
```

GNSS 坐标在录音真正开始时获取一次：有有效定位则记录坐标；无定位则 `gnss_fix=false` 且坐标为 0。不会把旧位置绑定到新录音。

## 9. 构建与上传

工程使用 PlatformIO。进入工程根目录后：

```bash
pio run
```

上传：

```bash
pio run -t upload
```

当前 v2.72c 已完成 FT-02 实机编译、刷写和功能验收。

## 10. v2.72c 实机验收基线

以下功能已确认正常：

```text
LoRa GPIO6 硬复位 / FullSync
NodeDB / 广播 / 私信 / ACK
收件箱直接回复
顶部未读消息提示
R 键重新同步后状态恢复
nihao → 你好
shoudao → 收到
连续拼音自动切分
词组候选与候选翻页
0 英文原样上屏
DEL 删除
词频学习
SD 持久化与重启恢复
[中] / [EN] 模式切换
中文标点
中文消息发送与回复
```

## 11. 发布包文件

关键发布文件：

```text
VERSION.txt                当前版本号
platformio.ini             PlatformIO 配置
boards/                    FT-02 板级配置
src/                       主固件源码
lib/                       本地库
 tools/                    字体 / 地图索引等工程工具
README.md                  本说明
THIRD_PARTY_NOTICES.txt    第三方来源与许可说明
FILE_CHECKSUMS.sha256      发布包文件校验
```

地图数据不随普通固件包重复分发。

## 12. 当前开发边界

v2.72c 是当前稳定冻结点。下一阶段可在此基础上继续实现 LanternBox Reliable Messaging Layer，包括即时消息、可靠消息、持久通知、Outbox、TTL、ACK 状态和过期处理。通讯底层与当前已验证输入法基线不应无依据回退。
