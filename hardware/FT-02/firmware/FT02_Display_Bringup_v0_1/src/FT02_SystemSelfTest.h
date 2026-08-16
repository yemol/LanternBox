#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"

enum FT02SelfTestState : uint8_t
{
    FT02_SELFTEST_PASS = 0,
    FT02_SELFTEST_WARN,
    FT02_SELFTEST_FAIL,
    FT02_SELFTEST_NA
};

struct FT02SelfTestItem
{
    const char* name;
    FT02SelfTestState state;
    char detail[96];
};

struct FT02SelfTestReport
{
    static constexpr uint8_t MAX_ITEMS = 16;
    FT02SelfTestItem items[MAX_ITEMS];
    uint8_t count;
    uint8_t passed;
    uint8_t warned;
    uint8_t failed;
    uint8_t skipped;
    uint32_t generation;
    uint32_t elapsedMs;
};

void FT02_SystemSelfTestRun();
const FT02SelfTestReport& FT02_SystemSelfTestReportCurrent();
void FT02_SystemSelfTestSetPage(uint8_t pageIndex);
uint8_t FT02_SystemSelfTestPage();
uint8_t FT02_SystemSelfTestPageCount();
void FT02_DrawSystemSelfTestScreen(FT02Display& display);
