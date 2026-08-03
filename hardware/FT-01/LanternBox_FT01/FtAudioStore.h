#pragma once

#include <Arduino.h>

struct FtAudioDeleteResult {
  bool wavDeleted = false;
  bool wavMissing = false;
  bool indexOk = false;
  int indexRemoved = 0;
  String error = "";
};

bool ftIsAudioWavFilename(const String& filename);
bool ftIsSafeAudioDeleteFilename(const String& filename);
bool ftRewriteAudioIndexWithoutFile(const String& filename, int& removedOut, String& errorOut);
bool ftDeleteAudioAndIndex(const String& filename, FtAudioDeleteResult& result);
