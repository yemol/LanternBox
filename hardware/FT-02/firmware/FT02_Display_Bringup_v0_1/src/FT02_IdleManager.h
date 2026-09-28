#pragma once

#include <Arduino.h>

void FT02_IdleManagerBegin(uint32_t nowMs);
void FT02_IdleManagerNoteActivity(uint32_t nowMs);
bool FT02_IdleManagerShouldSleep(uint32_t nowMs);
uint32_t FT02_IdleManagerIdleMs(uint32_t nowMs);
