#include "FtTimeUtil.h"

namespace {
  const uint8_t MONTH_DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  bool isLeapYear(int year) {
    return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
  }

  int daysInMonth(int year, int month) {
    if (month < 1 || month > 12) return 30;
    int days = MONTH_DAYS[month - 1];
    if (month == 2 && isLeapYear(year)) days++;
    return days;
  }

  int monthNameToNumber(const char* month) {
    static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char* found = strstr(names, month);
    return found ? ((found - names) / 3) + 1 : 1;
  }

  unsigned long daysSince1970(int year, int month, int day) {
    unsigned long days = 0;
    for (int y = 1970; y < year; y++) days += isLeapYear(y) ? 366 : 365;
    for (int m = 1; m < month; m++) days += daysInMonth(year, m);
    return days + day - 1;
  }
}

unsigned long makeEpochFromCompileTime() {
  char monthText[4];
  int day, year, hour, minute, second;
  sscanf(__DATE__, "%3s %d %d", monthText, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
  return makeEpochFromDateTime(year, monthNameToNumber(monthText), day, hour, minute, second);
}

unsigned long makeEpochFromDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month)) return 0;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return 0;

  unsigned long days = daysSince1970(year, month, day);
  return days * 86400UL + hour * 3600UL + minute * 60UL + second;
}

void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long secondsInDay = epoch % 86400UL;
  int hour = secondsInDay / 3600UL;
  int minute = (secondsInDay % 3600UL) / 60UL;
  int second = secondsInDay % 60UL;
  snprintf(buffer, bufferSize, "%02d:%02d:%02d", hour, minute, second);
}

void epochToShortTimeString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long secondsInDay = epoch % 86400UL;
  snprintf(buffer, bufferSize, "%02lu:%02lu", secondsInDay / 3600UL, (secondsInDay % 3600UL) / 60UL);
}

void epochToDateString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long days = epoch / 86400UL;
  int year = 1970;
  while (days >= (unsigned long)(isLeapYear(year) ? 366 : 365)) {
    days -= isLeapYear(year) ? 366 : 365;
    year++;
  }

  int month = 1;
  while (month <= 12 && days >= (unsigned long)daysInMonth(year, month)) {
    days -= daysInMonth(year, month);
    month++;
  }
  snprintf(buffer, bufferSize, "%04d-%02d-%02lu", year, month, days + 1);
}
