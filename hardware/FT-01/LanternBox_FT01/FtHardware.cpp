#include "FtHardware.h"

void FtHardware::prepareSharedSpiIdle(uint32_t settleUs) {
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  pinMode(LORA_NSS_PIN, OUTPUT);
  digitalWrite(LORA_NSS_PIN, HIGH);

  pinMode(LORA_RST_PIN, OUTPUT);
  digitalWrite(LORA_RST_PIN, HIGH);

  pinMode(LORA_IRQ_PIN, INPUT);
  pinMode(LORA_BUSY_PIN, INPUT);

  if (settleUs > 0) delayMicroseconds(settleUs);
}
