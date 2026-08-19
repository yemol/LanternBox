#pragma once
#include <Arduino.h>

namespace FtHardware {
  // LR01 SX1262 / Heltec Wireless Stick Lite V3
  constexpr int LORA_NSS_PIN  = 8;
  constexpr int LORA_RST_PIN  = 12;
  constexpr int LORA_IRQ_PIN  = 14;  // DIO1
  constexpr int LORA_BUSY_PIN = 13;

  constexpr int LORA_SCK_PIN  = 9;
  constexpr int LORA_MISO_PIN = 11;
  constexpr int LORA_MOSI_PIN = 10;

  // K25+ GNSS
  constexpr int GNSS_RX_PIN = 6;
  constexpr int GNSS_TX_PIN = 7;
  constexpr uint32_t GNSS_BAUD = 115200UL;

  // QMC5883L
  constexpr int I2C_SDA_PIN = 4;
  constexpr int I2C_SCL_PIN = 5;
  constexpr uint8_t QMC_ADDR = 0x0D;

  // Host UART to FT02 Core
  constexpr int HOST_RX_PIN = 18;
  constexpr int HOST_TX_PIN = 17;
  constexpr uint32_t HOST_BAUD = 115200UL;

  void prepareSharedSpiIdle(uint32_t settleUs = 10000UL);
}
