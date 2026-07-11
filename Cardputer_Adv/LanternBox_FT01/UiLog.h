// FT-01 Log UI module
// Responsibility:
// - Draw the audio log list and help entry.
// - Handle log screen keyboard events.
// - Delegate all audio/storage operations to AudioLogger.
//
// Keep SD/audio implementation details out of this module.

#pragma once
#include <Arduino.h>

void drawLogScreen();
void handleLogKey(const String& key);
