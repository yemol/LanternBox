#pragma once

#include <Arduino.h>

// FT-02 central serial logging policy.
// Keep these numeric constants usable from compiler -D flags.
#define FT02_LOG_LEVEL_ERROR   1
#define FT02_LOG_LEVEL_WARN    2
#define FT02_LOG_LEVEL_INFO    3
#define FT02_LOG_LEVEL_DEBUG   4
#define FT02_LOG_LEVEL_VERBOSE 5

#ifndef FT02_LOG_LEVEL
#define FT02_LOG_LEVEL FT02_LOG_LEVEL_INFO
#endif

#define FT02_LOG_ENABLED(level) ((FT02_LOG_LEVEL) >= (level))

#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_ERROR
#define FT02_LOGE(...)          do { Serial.printf(__VA_ARGS__); } while (0)
#define FT02_LOGE_PRINT(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define FT02_LOGE_PRINTLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#define FT02_LOGE_WRITE(...)    do { Serial.write(__VA_ARGS__); } while (0)
#else
#define FT02_LOGE(...)          do { } while (0)
#define FT02_LOGE_PRINT(...)    do { } while (0)
#define FT02_LOGE_PRINTLN(...)  do { } while (0)
#define FT02_LOGE_WRITE(...)    do { } while (0)
#endif

#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_WARN
#define FT02_LOGW(...)          do { Serial.printf(__VA_ARGS__); } while (0)
#define FT02_LOGW_PRINT(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define FT02_LOGW_PRINTLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#define FT02_LOGW_WRITE(...)    do { Serial.write(__VA_ARGS__); } while (0)
#else
#define FT02_LOGW(...)          do { } while (0)
#define FT02_LOGW_PRINT(...)    do { } while (0)
#define FT02_LOGW_PRINTLN(...)  do { } while (0)
#define FT02_LOGW_WRITE(...)    do { } while (0)
#endif

#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_INFO
#define FT02_LOGI(...)          do { Serial.printf(__VA_ARGS__); } while (0)
#define FT02_LOGI_PRINT(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define FT02_LOGI_PRINTLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#define FT02_LOGI_WRITE(...)    do { Serial.write(__VA_ARGS__); } while (0)
#else
#define FT02_LOGI(...)          do { } while (0)
#define FT02_LOGI_PRINT(...)    do { } while (0)
#define FT02_LOGI_PRINTLN(...)  do { } while (0)
#define FT02_LOGI_WRITE(...)    do { } while (0)
#endif

#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_DEBUG
#define FT02_LOGD(...)          do { Serial.printf(__VA_ARGS__); } while (0)
#define FT02_LOGD_PRINT(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define FT02_LOGD_PRINTLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#define FT02_LOGD_WRITE(...)    do { Serial.write(__VA_ARGS__); } while (0)
#define FT02_EPD_SERIAL_DIAG_BAUD 115200u
#else
#define FT02_LOGD(...)          do { } while (0)
#define FT02_LOGD_PRINT(...)    do { } while (0)
#define FT02_LOGD_PRINTLN(...)  do { } while (0)
#define FT02_LOGD_WRITE(...)    do { } while (0)
#define FT02_EPD_SERIAL_DIAG_BAUD 0u
#endif

#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_VERBOSE
#define FT02_LOGV(...)          do { Serial.printf(__VA_ARGS__); } while (0)
#define FT02_LOGV_PRINT(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define FT02_LOGV_PRINTLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#define FT02_LOGV_WRITE(...)    do { Serial.write(__VA_ARGS__); } while (0)
#else
#define FT02_LOGV(...)          do { } while (0)
#define FT02_LOGV_PRINT(...)    do { } while (0)
#define FT02_LOGV_PRINTLN(...)  do { } while (0)
#define FT02_LOGV_WRITE(...)    do { } while (0)
#endif
