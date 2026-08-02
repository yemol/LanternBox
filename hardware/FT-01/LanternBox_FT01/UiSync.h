#pragma once
#include <Arduino.h>

void drawSyncScreen();
void handleSyncKey(const String& key);
void syncShowPhase(const String& phase, const String& detail);
