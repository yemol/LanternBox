#pragma once

#include <Arduino.h>

// Shared FT-02 I2C bus guard.
//
// CardKB2 and ES8311 share Wire on SDA=47/SCL=21. Arduino Wire transactions
// are not safe to interleave across the UI/main task and the audio service task.
// Initialize once after Wire.begin(), then guard every complete I2C transaction.
void FT02_I2CBusInit();
bool FT02_I2CBusLock(uint32_t timeoutMs = 50);
void FT02_I2CBusUnlock();
