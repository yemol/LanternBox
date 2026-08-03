#pragma once

#include <Arduino.h>

String ftJsonEscape(const String& value);
String ftJsonString(const String& line, const String& key);
int ftJsonInt(const String& line, const String& key, int fallback = 0);
double ftJsonDouble(const String& line, const String& key, double fallback = 0.0);
String ftBaseName(const String& path);
String ftNormalizePath(const String& path, const String& directory);
String ftFormatBytes(uint64_t bytes, unsigned int decimals = 1);
