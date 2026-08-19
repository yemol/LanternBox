# LR01 Hardware Reference

## 1. 当前原型角色

当前 LR01 原型基于 ESP32-S3 + SX1262，并作为 FT-02 的 Navigation + Communication Coprocessor。

## 2. 引脚分配

| GPIO | 功能 | 所属 |
|---:|---|---|
| 4 | SDA | QMC5883L |
| 5 | SCL | QMC5883L |
| 6 | RX | K25+ GNSS |
| 7 | TX | K25+ GNSS |
| 8 | NSS | SX1262 |
| 9 | SCK | SX1262 |
| 10 | MOSI | SX1262 |
| 11 | MISO | SX1262 |
| 12 | RESET | SX1262 |
| 13 | BUSY | SX1262 |
| 14 | DIO1 | SX1262 |
| 17 | TX | Host UART to Core |
| 18 | RX | Host UART from Core |

QMC5883L I2C address：`0x0D`  
GNSS baud：`115200`  
Host UART：`115200 8N1`

## 3. RF 基线

```text
Profile: CN35
Frequency: 478.875 MHz
Bandwidth: 250 kHz
Spreading Factor: 11
Coding Rate: 4/5 (CR5)
Sync Word: 0x2B
TX Power: 19 dBm
Hop Limit: 3
```

## 4. 权限边界

LR01 独占：

- GNSS UART
- Compass I2C
- LoRa SPI / IRQ
- Meshtastic Native packet handling

Core 不应直接控制这些硬件。
