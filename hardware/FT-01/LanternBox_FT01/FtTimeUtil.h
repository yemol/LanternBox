#pragma once

#include <Arduino.h>

unsigned long makeEpochFromCompileTime();
unsigned long makeEpochFromDateTime(int year, int month, int day, int hour, int minute, int second);
void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize);
void epochToShortTimeString(unsigned long epoch, char* buffer, size_t bufferSize);
void epochToDateString(unsigned long epoch, char* buffer, size_t bufferSize);
