#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/Arduino.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stddef.h>
#define INPUT 1
#define OUTPUT 2
#define LOW 0
#define HIGH 1
#define RISING 3
#define FALLING 4
#define IRAM_ATTR
#define MSBFIRST 1
#define SPI_MODE0 0
#define HSPI 2
struct SerialMock { template<class... T> void printf(const char*, T...){} void println(const char*){} void print(const char*){} void write(char){} };
extern SerialMock Serial;
inline void pinMode(uint8_t,uint8_t){}
inline void digitalWrite(uint8_t,int){}
inline int digitalRead(uint8_t){return 0;}
inline void attachInterrupt(uint32_t, void(*)(void), int){}
inline void detachInterrupt(uint32_t){}
inline uint32_t digitalPinToInterrupt(uint32_t p){return p;}
inline void delay(uint32_t){}
inline void delayMicroseconds(uint32_t){}
inline uint32_t millis(){return 0;}
inline uint32_t micros(){return 0;}
inline long pulseIn(uint32_t,uint32_t,uint32_t){return 0;}
inline void yield(){}
inline void noInterrupts(){}
inline void interrupts(){}
EOF
cat > "$TMP/SPI.h" <<'EOF'
#pragma once
#include "Arduino.h"
class SPISettings { public: SPISettings(uint32_t=4000000, uint8_t=MSBFIRST, uint8_t=SPI_MODE0){} };
class SPIClass {
public:
  explicit SPIClass(int){}
  bool begin(int,int,int,int){return true;}
  void end(){}
};
EOF
cat > "$TMP/RadioLib.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "SPI.h"
#define RADIOLIB_NC 0xFFFFFFFFu
#define RADIOLIB_ERR_NONE 0
#define RADIOLIB_ERR_CRC_MISMATCH -7
#define RADIOLIB_ERR_RX_TIMEOUT -6
using RadioLibIrqFlags_t = uint32_t;
#define RADIOLIB_IRQ_PREAMBLE_DETECTED 2
#define RADIOLIB_IRQ_HEADER_VALID 4
#define RADIOLIB_IRQ_RX_DEFAULT_FLAGS ((RadioLibIrqFlags_t)0x0F)
#define RADIOLIB_IRQ_RX_DEFAULT_MASK ((RadioLibIrqFlags_t)0x01)
#define RADIOLIB_SX126X_RX_TIMEOUT_INF 0xFFFFFFu
#define RADIOLIB_SX126X_IRQ_RX_DONE 0x0002u
#define RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED 0x0004u
#define RADIOLIB_SX126X_IRQ_HEADER_VALID 0x0010u
#define RADIOLIB_SX126X_IRQ_HEADER_ERR 0x0020u
#define RADIOLIB_SX126X_IRQ_CRC_ERR 0x0040u
#define RADIOLIB_SX126X_IRQ_TIMEOUT 0x0200u
class Module {
public:
  Module(uint32_t,uint32_t,uint32_t,uint32_t,SPIClass&,SPISettings){}
  int16_t SPIsetRegValue(uint32_t,uint8_t,uint8_t=7,uint8_t=0,uint8_t=2,uint8_t=0xFF,bool=false){ return RADIOLIB_ERR_NONE; }
  int16_t SPIgetRegValue(uint32_t,uint8_t=7,uint8_t=0){ return 1; }
};
class SX1262 {
public:
  explicit SX1262(Module*){}
  int16_t begin(float,float,uint8_t,uint8_t,uint8_t,int8_t,uint16_t,float,bool){return 0;}
  int16_t setCurrentLimit(float){return 0;} int16_t setDio2AsRfSwitch(bool=true){return 0;} int16_t explicitHeader(){return 0;}
  int16_t setCRC(uint8_t,uint16_t=0x1D0F,uint16_t=0x1021,bool=true){return 0;} int16_t setRxBoostedGainMode(bool,bool=true){return 0;}
  void setPacketReceivedAction(void(*)(void)){}
  void clearPacketReceivedAction(){}
  int16_t startReceive(uint32_t, RadioLibIrqFlags_t, RadioLibIrqFlags_t, size_t){return 0;}
  uint32_t getIrqFlags(){return 0;}
  int16_t clearIrqFlags(uint32_t){return 0;}
  size_t getPacketLength(bool=true){return 1;}
  int16_t readData(uint8_t*,size_t){return 0;} int16_t transmit(const uint8_t*,size_t,uint8_t=0){return 0;}
  int16_t sleep(bool=true){return 0;} int16_t standby(){return 0;}
  float getRSSI(){return -100;} float getSNR(){return 0;} float getFrequencyError(){return 0;}
};
EOF
cat > "$TMP/stubs.cpp" <<'EOF'
#include "Arduino.h"
SerialMock Serial;
EOF
# Only current hardware-SPI backend path.
g++ -std=c++17 -Wall -Wextra -Werror -I"$TMP" -I"$ROOT/src" -I"$ROOT/include" -I"$ROOT/lib/FT02Log/src" -fsyntax-only \
  "$ROOT/src/FT02_LoRaHardwareSpi.cpp" "$ROOT/src/FT02_MeshRadioBackend.cpp" "$TMP/stubs.cpp"
echo "PASS Mesh HW-SPI A1 backend C++17 syntax"
