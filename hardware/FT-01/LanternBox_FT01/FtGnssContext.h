#pragma once

#include <Arduino.h>

struct FtGnssSnapshot {
  bool valid;
  double latitude;
  double longitude;
  int satellites;
  uint32_t ageMs;
};

// Returns true only when the terminal currently has a valid and fresh GNSS fix.
bool ftGetFreshGnssSnapshot(FtGnssSnapshot& out);

// Appends a compact, parseable location footer to the same text message:
//   [GPS:31.210449,121.381342]
// attached is true only when a fresh fix was available.
String ftBuildMessageWithGnss(const String& userText, bool& attached);
