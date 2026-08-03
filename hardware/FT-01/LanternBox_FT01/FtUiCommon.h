#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>

namespace FtKey {
  bool hasLetter(const String& key, char lower, char upper);
  bool isEsc(const String& key, bool includeDelete = false);
  bool isEnter(const String& key);
  bool isLeft(const String& key);
  bool isRight(const String& key);
  bool isUp(const String& key);
  bool isDown(const String& key);
}

void ftDrawHeaderBase(const String& title);
void ftDrawCompactTitle(const String& title);
void ftDrawSdGnssStatus(int sdX, int gnssX);
void ftDrawClockHHMM(int x);
void ftDrawFooter(const String& text, int x = 8);
void ftDrawLabelValueRow(
  int y,
  const String& label,
  const String& value,
  int valueX,
  uint16_t color = WHITE,
  uint16_t labelColor = LIGHTGREY
);
