#pragma once

#include <Arduino.h>

namespace FtHardware {
  constexpr int SD_SCK_PIN = 40;
  constexpr int SD_MISO_PIN = 39;
  constexpr int SD_MOSI_PIN = 14;
  constexpr int SD_CS_PIN = 12;
  constexpr uint32_t SD_INIT_FREQ = 400000UL;

  constexpr int LORA_NSS_PIN = 5;
  constexpr int LORA_RST_PIN = 3;
  constexpr int LORA_IRQ_PIN = 4;
  constexpr int LORA_BUSY_PIN = 6;

  constexpr int GNSS_RX_PIN = 15;
  constexpr int GNSS_TX_PIN = 13;
  constexpr uint32_t GNSS_BAUD = 115200UL;

  void prepareSharedSpiIdle(uint32_t settleUs = 10000UL);
}
