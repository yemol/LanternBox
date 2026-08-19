# FT-02 v2.75d LR01 RX Reliability + Compact Logging A2

## 目的
1. LR01 Host UART RX buffer 提升到 4096 bytes，避免 EPD 阻塞期间丢失 Host 协议行。
2. 默认日志等级 1 时，周期 NAV/RADIO/SYSTEM/PONG 不再刷屏。
3. 保留消息发送生命周期、DELIVERY ACK/TIMEOUT、MESH_RX、节点列表完成和错误。

## 启动确认
应看到：
`[LR01-HOST-A2] ready RX=7 TX=13 baud=115200 rxbuf=4096 log=1 ...`

## 私信验收
发送一条私信后重点观察：
- TX_ACCEPTED
- TX_SENT
- TX_RESULT
- DELIVERY ... ACK 或 TIMEOUT

正常情况下不应每秒输出 NAV_STATE / RADIO_STATE / SYSTEM_STATE / PONG。

如果仍出现 `[LR01] RX unhandled:`，请保留该行及前后日志，用于继续检查协议行是否被截断。
