#include <Arduino.h>
#include "FT02_Log.h"
#include <SPI.h>
#include <GxEPD2_BW.h>

#include "FT02_HomeUI.h"
#include "FT02_BuildInfo.h"
#include "FT02_PbfMapUI.h"
#include "FT02_PbfMapRuntime.h"
#include "FT02_HelpUI.h"
#include "FT02_KnowledgeUI.h"
#include "FT02_FieldManual.h"
#include "FT02_FieldLibrary.h"
#include "FT02_Gnss.h"
#include "FT02_GnssFieldTest.h"
#include "FT02_LocationRecorder.h"
#include "FT02_LocationRecorderUI.h"
#include "FT02_LocationLog.h"
#include "FT02_LocationLogUI.h"
#include "FT02_AudioLog.h"
#include "FT02_BatteryMonitor.h"
#include "FT02_AudioLogUI.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_FontPackRenderer.h"
#include "FT02_PageState.h"
#include "FT02_HomeCards.h"
#include "FT02_InputManager.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_StatusBar.h"
#include "FT02_Storage.h"
#include "FT02_MapSearch.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LoRaHwProbe.h"
#include "FT02_MeshService.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_CommunicationNodeUI.h"
#include "FT02_CompassCalibrationUI.h"
#include "FT02_CompassCore.h"
#include "FT02_CompassUI.h"
#include "FT02_GnssPowerControl.h"
#include "FT02_SystemSelfTest.h"
#include "FT02_WirelessManager.h"
#include "FT02_PinyinLearning.h"
#include "FT02_ModeManager.h"
#include "FT02_IdleManager.h"
#include "FT02_SleepUI.h"
#include "FT02_SleepDiagnostics.h"
#include "FT02_SystemSettings.h"
#include "FT02_SystemSettingsUI.h"
#include "wifi/FT02_WifiManager.h"
#include <esp32-hal-cpu.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// A1.5.3 isolation test: disable application services during sleep to isolate reset source.
#define FT02_SLEEP_ISOLATION_TEST 0
#include <esp_system.h>
#include <esp_rom_sys.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr int EPD_PWR  = 18;
constexpr int EPD_BUSY = 3;
constexpr int EPD_RST  = 8;
constexpr int EPD_DC   = 9;
constexpr int EPD_CS   = 10;
constexpr int EPD_MOSI = 11;
constexpr int EPD_SCK  = 12;
constexpr int CARDKB_SDA = 47;
constexpr int CARDKB_SCL = 21;
constexpr uint8_t CARDKB_ADDR = 0x5F;

FT02Display display(
    GxEPD2_426_GDEQ0426T82(
        EPD_CS,
        EPD_DC,
        EPD_RST,
        EPD_BUSY
    )
);

struct FT02DateTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
};

static FT02DateTime g_ft02BootDateTime;
static uint32_t g_ft02BootMillis = 0;
static int g_ft02LastShownMinute = -1;
static int g_ft02HomeSelectedCard = 0;
static FT02PageState g_ft02PageState = FT02_PAGE_HOME;
static FT02PageState g_ft02PageBeforeHelp = FT02_PAGE_HOME;
static FT02PageState g_ft02SleepReturnPage = FT02_PAGE_HOME;
static bool g_ft02WakeReleaseRequired = false;
static uint32_t g_ft02PreSleepCpuMhz = 240u;
static bool g_ft02HomePPressActive = false;
static bool g_ft02HomePLongTriggered = false;
static uint32_t g_ft02HomePPressStartMs = 0;
static uint32_t g_ft02HomePLastEventMs = 0;
static uint8_t g_ft02HomePEventCount = 0;
static uint32_t g_ft02WakeLastConsumedKeyMs = 0;
static uint32_t g_ft02SleepEnteredMs = 0;
static uint32_t g_ft02SleepSliceCount = 0;
static FT02BatterySnapshot g_ft02PreSleepBattery = {};
static bool g_ft02PreSleepBatteryValid = false;

// A1.5 crash-safe Sleep Exit RCA. RTC_NOINIT_ATTR survives watchdog/software
// resets and lets the next boot report the last exact checkpoint without
// touching USB while the S3 is in the light-sleep maintenance loop.
struct FT02SleepNoInitTrace
{
    uint32_t magic;
    uint32_t version;
    uint32_t phase;
    uint32_t sessionActive;
    uint32_t exitReason;      // 0=none, 1=keyboard accepted, 2=unexpected mode loss
    uint32_t sleepCount;
    uint32_t wakeCount;
    int32_t lastSleepErr;
    int32_t lastWakeCause;
    uint32_t wakeArmed;
    uint32_t inputEventCount;
    uint32_t invalidInputCount;
    uint32_t lastInputRaw;
    uint32_t lastInputKey;
    uint32_t lastInputCommand;
    uint32_t acceptedRaw;
    uint32_t acceptedKey;
    uint32_t acceptedCommand;
    uint32_t acceptedSlice;
    uint32_t wakeCompleted;
};

constexpr uint32_t FT02_SLEEP_NOINIT_MAGIC = 0x46543253u; // "FT2S"
constexpr uint32_t FT02_SLEEP_NOINIT_VERSION = 0x00010500u;
RTC_NOINIT_ATTR FT02SleepNoInitTrace g_ft02SleepNoInitTrace;
constexpr uint32_t FT02_HOME_P_LONG_PRESS_MS = 900u;
constexpr uint32_t FT02_HOME_P_SHORT_SETTLE_MS = 320u;
constexpr uint8_t FT02_HOME_P_LONG_MIN_EVENTS = 3u;
constexpr uint32_t FT02_WAKE_CONSUME_QUIET_MS = 450u;
constexpr uint32_t FT02_KEYBOARD_WAKE_SCL_RELEASE_TIMEOUT_MS = 500u;
static char g_ft02LastBatteryStatusLine1[16] = "";
static char g_ft02LastBatteryStatusLine2[16] = "";
static bool g_ft02CommunicationReadyPresented = false;
static bool g_ft02LastLoRaLinkState = false;
static bool g_ft02LastLoRaReadyState = false;
static uint32_t g_ft02LastCommunicationRevision = 0;
static uint32_t g_ft02LastCompassCalRevision = 0;
static uint32_t g_ft02LastCompassCalRedrawMs = 0;
static uint32_t g_ft02LastLoRaNodeRevision = 0;
static uint16_t g_ft02LastLoRaUnread = 0;
static uint32_t g_ft02RecorderLastUiGeneration = 0;
static uint32_t g_ft02RecorderLastGnssGeneration = 0;
static uint32_t g_ft02GnssFieldTestLastGeneration = 0;
static uint32_t g_ft02RecorderLastScreenRefreshMs = 0;
static uint16_t g_ft02LocationLogSelectedIndex = 0;
static uint32_t g_ft02LocationLogLastGeneration = 0;
static FT02PageState g_ft02LocationLogDeleteReturnPage = FT02_PAGE_LOCATION_LOG_LIST;
static uint16_t g_ft02AudioLogSelectedIndex = 0;
static uint32_t g_ft02AudioLogLastGeneration = 0;
static bool g_ft02AudioCommandReleaseRequired = false;
static uint32_t g_ft02AudioRecordRestartGuardUntilMs = 0;
static uint32_t g_ft02AudioLastShownRecordBucket = 0xFFFFFFFFu;
static uint8_t g_ft02AudioLastShownVolume = 0xFFu;
static bool g_ft02AudioVolumeRefreshPending = false;
static uint32_t g_ft02AudioVolumeRefreshDueMs = 0;
constexpr uint32_t FT02_AUDIO_VOLUME_REFRESH_COALESCE_MS = 220u;
constexpr uint32_t FT02_AUDIO_TIMER_REFRESH_MS = 5000u;
constexpr uint32_t FT02_RECORDER_UI_REFRESH_MS = 15000UL;
static char g_ft02LastGnssStatusLine1[16] = "";
static char g_ft02LastGnssStatusLine2[16] = "";
static bool g_ft02MapFollowGnss = true;
static bool g_ft02MapHasTrackedFix = false;
static bool g_ft02MapLastFixValid = false;
static double g_ft02MapLastFollowLat = 0.0;
static double g_ft02MapLastFollowLon = 0.0;
static bool g_ft02MapLastHeadingValid = false;
static float g_ft02MapLastHeadingDeg = 0.0f;
static uint8_t g_ft02MapLastHeadingSource = 0; // 0 none, 1 Core compass, 2 GNSS course
static uint32_t g_ft02MapLastFollowRefreshMs = 0;
static uint32_t g_ft02MapLastFollowCheckMs = 0;
static uint8_t g_ft02MapPartialNavRefreshCount = 0;
static uint32_t g_ft02MapReleaseZeroSinceMs = 0;
enum FT02MapRefreshKind : uint8_t
{
    FT02_MAP_REFRESH_NONE = 0,
    FT02_MAP_REFRESH_ZOOM,
    FT02_MAP_REFRESH_PAN
};

static bool g_ft02MapRefreshPending = false;
static bool g_ft02MapRequireNavRelease = false;
static uint32_t g_ft02MapRefreshDeadlineMs = 0;
static int g_ft02MapPendingZoomFrom = 0;
static FT02MapRefreshKind g_ft02MapRefreshKind = FT02_MAP_REFRESH_NONE;
constexpr uint32_t FT02_MAP_FOLLOW_CHECK_MS = 10000UL;
constexpr double FT02_MAP_FOLLOW_DISTANCE_METERS = 10.0;
constexpr float FT02_MAP_HEADING_REFRESH_DEGREES = 5.0f;
constexpr uint8_t FT02_MAP_PARTIAL_CLEANUP_COUNT = 20;
constexpr uint32_t FT02_MAP_NAV_RELEASE_STABLE_MS = 150UL;
constexpr int FT02_MAP_SAFE_MARGIN_X = 140; // central ~65% of 800 px map width
constexpr int FT02_MAP_SAFE_MARGIN_Y = 64;  // central ~65% of 364 px map height
constexpr uint32_t FT02_MAP_ZOOM_SETTLE_MS = 450UL;
constexpr uint32_t FT02_MAP_PAN_SETTLE_MS = 300UL;

static void FT02_RedrawCurrentPage();
static void FT02_ReinitializeEpdFullMode();
static void FT02_DrawKnowledgeAfterCleanTransition();
static void FT02_SyncGnssStatusBar(bool allowPartialRefresh);
static void FT02_SyncBatteryStatusBar(bool allowPartialRefresh);
static void FT02_UpdateMapGnssFollow();
static void FT02_RunDirectPbfMap(bool resetDefault, bool showLoading);
static void FT02_ScheduleMapZoomRefresh(int originalZoom);
static void FT02_ScheduleMapPanRefresh();
static void FT02_ProcessPendingMapRefresh();
static void FT02_CancelPendingMapRefresh();
static void FT02_ArmMapNavReleaseGate();
static void FT02_OpenAudioLogListPage(bool reload);
static void FT02_ClampAudioLogSelection();
static bool FT02_ArmAudioCommandAfterRelease(const FT02InputEvent& event);
static void FT02_OpenDeviceStatusPage();
static void FT02_OpenCompassPage();
static void FT02_OpenFieldLibraryPage();
static void FT02_OpenMapSearchPage();
static void FT02_OpenWirelessWiFiPage();
static void FT02_OpenSystemSettingsPage();
static bool FT02_HandleSystemSettingsInput(const FT02InputEvent& event);
static void FT02_EnterSleepMode(bool automatic);
static void FT02_WakeFromSleep();
static bool FT02_ServiceSleepMode();
static bool FT02_SleepBlocked(bool automatic, const char** reason = nullptr);
static void FT02_EpdSignalsEnterHighImpedance();
static void FT02_EpdSignalsRestoreForWake();
static bool FT02_SleepWaitForKeyboardSignal();
static void FT02_DisarmKeyboardSclWake();
static bool FT02_ServiceHomeModeShortcut(const FT02InputEvent& event);

static int FT02_MonthFromBuildString(const char* mon)
{
    if(mon[0] == 'J' && mon[1] == 'a') return 1;
    if(mon[0] == 'F') return 2;
    if(mon[0] == 'M' && mon[2] == 'r') return 3;
    if(mon[0] == 'A' && mon[1] == 'p') return 4;
    if(mon[0] == 'M' && mon[2] == 'y') return 5;
    if(mon[0] == 'J' && mon[1] == 'u' && mon[2] == 'n') return 6;
    if(mon[0] == 'J' && mon[1] == 'u' && mon[2] == 'l') return 7;
    if(mon[0] == 'A' && mon[1] == 'u') return 8;
    if(mon[0] == 'S') return 9;
    if(mon[0] == 'O') return 10;
    if(mon[0] == 'N') return 11;
    if(mon[0] == 'D') return 12;
    return 1;
}

static bool FT02_IsLeapYear(int year)
{
    if((year % 400) == 0) return true;
    if((year % 100) == 0) return false;
    return (year % 4) == 0;
}

static int FT02_DaysInMonth(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if(month == 2 && FT02_IsLeapYear(year))
    {
        return 29;
    }

    return days[month - 1];
}

static void FT02_AddMinutes(FT02DateTime& dt, uint32_t addMinutes)
{
    while(addMinutes > 0)
    {
        uint32_t roomInHour = 60 - dt.minute;
        uint32_t step = addMinutes < roomInHour ? addMinutes : roomInHour;

        dt.minute += step;
        addMinutes -= step;

        if(dt.minute >= 60)
        {
            dt.minute = 0;
            dt.hour += 1;

            if(dt.hour >= 24)
            {
                dt.hour = 0;
                dt.day += 1;

                int dim = FT02_DaysInMonth(dt.year, dt.month);
                if(dt.day > dim)
                {
                    dt.day = 1;
                    dt.month += 1;

                    if(dt.month > 12)
                    {
                        dt.month = 1;
                        dt.year += 1;
                    }
                }
            }
        }
    }
}

static FT02DateTime FT02_ReadBuildDateTime()
{
    FT02DateTime dt;

    const char* buildDate = __DATE__; // Example: "Jul 23 2026"
    const char* buildTime = __TIME__; // Example: "01:38:42"

    char mon[4];
    mon[0] = buildDate[0];
    mon[1] = buildDate[1];
    mon[2] = buildDate[2];
    mon[3] = 0;

    dt.month = FT02_MonthFromBuildString(mon);
    dt.day = atoi(buildDate + 4);
    dt.year = atoi(buildDate + 7);

    dt.hour = (buildTime[0] - '0') * 10 + (buildTime[1] - '0');
    dt.minute = (buildTime[3] - '0') * 10 + (buildTime[4] - '0');

    return dt;
}

static FT02DateTime FT02_CurrentDateTime()
{
    // Once GNSS has set the ESP32 UTC clock, localtime_r() applies the
    // fixed UTC+8 timezone configured by FT02_GnssBegin().  Before the first
    // valid GNSS date/time arrives, retain the proven build-time fallback.
    const time_t systemNow = time(nullptr);
    if(systemNow >= 1704067200) // 2024-01-01 UTC sanity threshold
    {
        struct tm localTm = {};
        localtime_r(&systemNow, &localTm);
        FT02DateTime dt;
        dt.year = localTm.tm_year + 1900;
        dt.month = localTm.tm_mon + 1;
        dt.day = localTm.tm_mday;
        dt.hour = localTm.tm_hour;
        dt.minute = localTm.tm_min;
        return dt;
    }

    FT02DateTime dt = g_ft02BootDateTime;
    const uint32_t elapsedMinutes = (millis() - g_ft02BootMillis) / 60000UL;
    FT02_AddMinutes(dt, elapsedMinutes);
    return dt;
}

static int FT02_DateTimeMinuteKey(const FT02DateTime& dt)
{
    return (((dt.month * 32 + dt.day) * 24 + dt.hour) * 60 + dt.minute);
}

static bool FT02_IsMapStatusPartialSuppressed()
{
    // The map page owns its own refresh cadence and rendering transaction.
    // Keep background status-bar partial updates out of it so they cannot
    // interrupt navigation redraw scheduling.
    return g_ft02PageState == FT02_PAGE_MAP;
}

static void FT02_UpdateClockCacheOnly()
{
    FT02DateTime now = FT02_CurrentDateTime();

    char hhmm[6];
    char mmdd[6];

    snprintf(hhmm, sizeof(hhmm), "%02d:%02d", now.hour, now.minute);
    snprintf(mmdd, sizeof(mmdd), "%02d/%02d", now.month, now.day);

    FT02_SetStatusBarClockCache(hhmm, mmdd);
    g_ft02LastShownMinute = FT02_DateTimeMinuteKey(now);

}

static void FT02_UpdateClockIfNeeded(bool force)
{
    FT02DateTime now = FT02_CurrentDateTime();
    int minuteKey = FT02_DateTimeMinuteKey(now);

    if(!force && minuteKey == g_ft02LastShownMinute)
    {
        return;
    }

    // The map page owns its own refresh cadence. Cache the new time while
    // navigating instead of injecting a separate status-bar transaction.
    if(FT02_IsMapStatusPartialSuppressed())
    {
        FT02_UpdateClockCacheOnly();
        return;
    }

    // Knowledge pages are deliberately kept free of status-bar partial
    // refreshes. A partial update before the next full knowledge frame can
    // leave the SSD1677/GDEQ0426T82 controller in a state that makes fixed
    // horizontal regions appear pale. Cache the new time only; the next
    // knowledge full-frame commit will draw it together with the page.
    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE || g_ft02PageState == FT02_PAGE_LIBRARY)
    {
        FT02_UpdateClockCacheOnly();
        return;
    }

    g_ft02LastShownMinute = minuteKey;

    char hhmm[6];
    char mmdd[6];

    snprintf(
        hhmm,
        sizeof(hhmm),
        "%02d:%02d",
        now.hour,
        now.minute
    );

    snprintf(
        mmdd,
        sizeof(mmdd),
        "%02d/%02d",
        now.month,
        now.day
    );

    FT02_DrawStatusBarClock(
        display,
        hhmm,
        mmdd
    );
}

static void FT02_SyncBatteryStatusBar(bool allowPartialRefresh)
{
    char line1[16];
    char line2[16];
    FT02_BatteryFormatStatus(line1, sizeof(line1), line2, sizeof(line2));

    const bool changed =
        strcmp(line1, g_ft02LastBatteryStatusLine1) != 0 ||
        strcmp(line2, g_ft02LastBatteryStatusLine2) != 0;

    FT02_SetStatusBarBatteryCache(line1, line2);
    snprintf(g_ft02LastBatteryStatusLine1, sizeof(g_ft02LastBatteryStatusLine1), "%s", line1);
    snprintf(g_ft02LastBatteryStatusLine2, sizeof(g_ft02LastBatteryStatusLine2), "%s", line2);

    if(!allowPartialRefresh || !changed) return;

    if(g_ft02PageState == FT02_PAGE_MAP ||
       g_ft02PageState == FT02_PAGE_KNOWLEDGE ||
       g_ft02PageState == FT02_PAGE_LIBRARY ||
       FT02_AudioLogIsBusy())
    {
        return;
    }

    FT02_DrawStatusBarBattery(display, line1, line2);
}

static void FT02_SyncGnssStatusBar(bool allowPartialRefresh)
{
    const FT02GnssSnapshot snapshot = FT02_GnssSnapshotCurrent();

    const char* line1 = "GPS";
    const char* line2 = "连接中";

    if(!FT02_GnssPowerDesiredEnabled())
    {
        // Indoor mode silently powers GNSS down. Do not expose the mode policy
        // as a warning-style status message; just report the device as off.
        line2 = "已关闭";
    }
    else if(FT02_GnssPowerWakeTransitionActive() ||
            (FT02_GnssPowerActualKnown() && !FT02_GnssPowerActualEnabled()))
    {
        line2 = "唤醒中";
    }
    else if(snapshot.communicationActive)
    {
        if(snapshot.fixValid)
        {
            if(snapshot.fixType >= 3) line2 = "3D定位";
            else if(snapshot.fixType == 2) line2 = "2D定位";
            else line2 = "已定位";
        }
        else
        {
            line2 = "搜索中";
        }
    }
    else if(snapshot.startAgeMs >= 60000)
    {
        line2 = "GNSS离线";
    }

    const bool changed =
        strcmp(g_ft02LastGnssStatusLine1, line1) != 0 ||
        strcmp(g_ft02LastGnssStatusLine2, line2) != 0;

    FT02_SetStatusBarGnssCache(line1, line2);

    if(!changed) return;

    snprintf(g_ft02LastGnssStatusLine1, sizeof(g_ft02LastGnssStatusLine1), "%s", line1);
    snprintf(g_ft02LastGnssStatusLine2, sizeof(g_ft02LastGnssStatusLine2), "%s", line2);

    // Knowledge pages must not receive status-bar partial updates because that
    // can reintroduce the pale-band controller state. The GNSS page already
    // performs a full page refresh for meaningful GNSS state changes.
    if(allowPartialRefresh &&
       g_ft02PageState != FT02_PAGE_KNOWLEDGE &&
       g_ft02PageState != FT02_PAGE_LIBRARY &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_LIST &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_DETAIL &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM &&
       g_ft02PageState != FT02_PAGE_AUDIO_LOG_LIST &&
       g_ft02PageState != FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM &&
       !FT02_IsMapStatusPartialSuppressed())
    {
        FT02_DrawStatusBarGnss(display, line1, line2);
    }
}

static void FT02_RefreshStorageStatusCache()
{
    char line1[16];
    char line2[16];

    if(FT02_StorageIsReady())
    {
        const unsigned long freeMb = FT02_StorageFreeMB();
        unsigned long freeGb = freeMb / 1024UL;
        if(freeGb < 1 && freeMb > 0) freeGb = 1;
        snprintf(line1, sizeof(line1), "%luG", freeGb);
        snprintf(line2, sizeof(line2), "SD剩余");
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_SCANNING)
    {
        snprintf(line1, sizeof(line1), "SD");
        snprintf(line2, sizeof(line2), "SCAN");
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_NO_CARD)
    {
        snprintf(line1, sizeof(line1), "NO");
        snprintf(line2, sizeof(line2), "SD");
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_ERROR)
    {
        snprintf(line1, sizeof(line1), "SD");
        snprintf(line2, sizeof(line2), "ERR");
    }
    else
    {
        snprintf(line1, sizeof(line1), "SD");
        snprintf(line2, sizeof(line2), "INIT");
    }

    FT02_SetStatusBarStorageCache(line1, line2);
}

static void FT02_ReinitializeEpdFullMode()
{
    // Hard-cycle the e-paper rail to clear stale partial-update state.
    display.epd2.powerOff();
    delay(30);

    digitalWrite(EPD_PWR, LOW);
    delay(160);
    digitalWrite(EPD_PWR, HIGH);
    delay(160);

    digitalWrite(EPD_RST, HIGH);
    digitalWrite(EPD_DC, LOW);
    digitalWrite(EPD_CS, HIGH);
    delay(30);

    display.epd2.enableWrite(EPD_SCK, EPD_MOSI);
    display.init(FT02_EPD_SERIAL_DIAG_BAUD, true, 10, false);
    display.setRotation(2);

}

static void FT02_DrawKnowledgeAfterCleanTransition()
{
    // Reinitialize the panel before the final knowledge-page commit.
    FT02_ReinitializeEpdFullMode();
    FT02_UpdateClockCacheOnly();
    FT02_SyncGnssStatusBar(false);
    FT02_DrawKnowledgeScreen(display);
}

static void FT02_RedrawCurrentPage()
{
    // While map pan/zoom is settling, the scheduler owns the next commit.
    if(g_ft02PageState == FT02_PAGE_MAP && g_ft02MapRefreshPending)
    {
        return;
    }

    // Prime the full-page status bar with live clock text before drawing.
    // This keeps each page transition to one hardware transaction.
    FT02_UpdateClockCacheOnly();
    FT02_SyncGnssStatusBar(false);

    if(g_ft02PageState == FT02_PAGE_MAP_SEARCH)
    {
        FT02_DrawMapSearchScreen(display);
        return;
    }

    if(g_ft02PageState == FT02_PAGE_MAP)
    {
        FT02_PbfMapSetFollowGnss(g_ft02MapFollowGnss);
        FT02_DrawPbfMapScreen(display);
        return;
    }

    if(g_ft02PageState == FT02_PAGE_LOCATION_RECORDER)
    {
        FT02_DrawLocationRecorderScreen(
            display,
            FT02_GnssSnapshotCurrent(),
            FT02_LocationRecorderSnapshotCurrent()
        );
    }
    else if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_LIST)
    {
        FT02_DrawLocationLogListScreen(display, g_ft02LocationLogSelectedIndex);
    }
    else if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_DETAIL)
    {
        FT02_DrawLocationLogDetailScreen(display, g_ft02LocationLogSelectedIndex);
    }
    else if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM)
    {
        FT02LocationLogEntry entry;
        if(FT02_LocationLogGetNewest(g_ft02LocationLogSelectedIndex, entry))
        {
            FT02_DrawLocationLogDeleteConfirmScreen(display, entry);
        }
        else
        {
            g_ft02PageState = FT02_PAGE_LOCATION_LOG_LIST;
            FT02_DrawLocationLogListScreen(display, g_ft02LocationLogSelectedIndex);
        }
    }
    else if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST)
    {
        FT02_DrawAudioLogListScreen(display, g_ft02AudioLogSelectedIndex);
    }
    else if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM)
    {
        FT02AudioLogEntry entry;
        if(FT02_AudioLogGetNewest(g_ft02AudioLogSelectedIndex, entry))
        {
            FT02_DrawAudioLogDeleteConfirmScreen(display, entry);
        }
        else
        {
            g_ft02PageState = FT02_PAGE_AUDIO_LOG_LIST;
            FT02_DrawAudioLogListScreen(display, g_ft02AudioLogSelectedIndex);
        }
    }
    else if(g_ft02PageState == FT02_PAGE_COMMUNICATION)
    {
        FT02_DrawCommunicationNodeScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)
    {
        FT02_DrawSystemSelfTestScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_COMPASS)
    {
        FT02_DrawCompassScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        FT02_DrawCompassCalibrationScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)
    {
        FT02_DrawWirelessWiFiScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_SYSTEM_SETTINGS)
    {
        FT02_DrawSystemSettingsScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_HELP)
    {
        FT02_DrawHelpScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        FT02_DrawKnowledgeScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_LIBRARY)
    {
        FT02_DrawFieldLibraryScreen(display);
    }
    else
    {
        FT02_DrawHomeScreen(display, g_ft02HomeSelectedCard);
    }

}

static void FT02_OpenHelpPage()
{
    if(g_ft02PageState == FT02_PAGE_HELP)
    {
        return;
    }

    g_ft02PageBeforeHelp = g_ft02PageState;
    if(g_ft02PageBeforeHelp == FT02_PAGE_MAP)
    {
        FT02_CancelPendingMapRefresh();
    }
    if(g_ft02PageBeforeHelp == FT02_PAGE_AUDIO_LOG_LIST ||
       g_ft02PageBeforeHelp == FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM)
    {
        FT02_AudioLogRelease(FT02_AUDIO_OWNER_JOURNAL);
    }
    else if(g_ft02PageBeforeHelp == FT02_PAGE_DEVICE_STATUS)
    {
        FT02_AudioLogRelease(FT02_AUDIO_OWNER_SYSTEM_TEST);
    }
    g_ft02PageState = FT02_PAGE_HELP;

    FT02_RedrawCurrentPage();
}

static void FT02_ReturnHomePage()
{
    if(g_ft02PageState == FT02_PAGE_HOME)
    {
        return;
    }

    const bool returningFromKnowledge =
        g_ft02PageState == FT02_PAGE_KNOWLEDGE;

    if(g_ft02PageState == FT02_PAGE_MAP)
    {
        FT02_CancelPendingMapRefresh();
        FT02_PbfMapUnload();
    }
    if(g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)
    {
        FT02_WirelessManagerCloseWiFi();
    }

    // Audio owners are idempotent. Releasing both here guarantees that no
    // unusual navigation path can carry ES8311/I2S back to the home page.
    FT02_AudioLogRelease(FT02_AUDIO_OWNER_JOURNAL);
    FT02_AudioLogRelease(FT02_AUDIO_OWNER_SYSTEM_TEST);

    FT02_RefreshStorageStatusCache();
    g_ft02PageState = FT02_PAGE_HOME;

    if(returningFromKnowledge)
    {
        // v2.48: use the same proven clean-controller transition as knowledge
        // entry, but submit the final home page as the first and only frame.
        // This preserves the pale-band fix without a separate white refresh.
        FT02_ReinitializeEpdFullMode();
        FT02_UpdateClockCacheOnly();
        FT02_SyncGnssStatusBar(false);

        FT02_DrawHomeScreen(display, g_ft02HomeSelectedCard);
        return;
    }

    FT02_RedrawCurrentPage();
}

static void FT02_ReturnFromHelpPage()
{
    if(g_ft02PageState != FT02_PAGE_HELP)
    {
        return;
    }

    g_ft02PageState = g_ft02PageBeforeHelp;
    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST ||
       g_ft02PageState == FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM)
    {
        (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_JOURNAL);
    }
    else if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)
    {
        (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST);
    }

    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        FT02_DrawKnowledgeAfterCleanTransition();
        return;
    }

    FT02_RedrawCurrentPage();
}

static double FT02_MapDistanceMeters(
    double lat1,
    double lon1,
    double lat2,
    double lon2
)
{
    constexpr double earthRadiusMeters = 6371000.0;
    const double toRadians = M_PI / 180.0;
    const double p1 = lat1 * toRadians;
    const double p2 = lat2 * toRadians;
    const double dp = (lat2 - lat1) * toRadians;
    const double dl = (lon2 - lon1) * toRadians;
    const double a = sin(dp * 0.5) * sin(dp * 0.5) +
        cos(p1) * cos(p2) * sin(dl * 0.5) * sin(dl * 0.5);
    const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return earthRadiusMeters * c;
}

static bool FT02_MapNavigationHeading(float& headingDeg, uint8_t& source)
{
    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    if(compass.ready && compass.valid)
    {
        headingDeg = static_cast<float>(compass.headingX10) / 10.0f;
        source = 1;
        return true;
    }

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(gnss.fixValid && gnss.courseValid)
    {
        headingDeg = gnss.courseDegrees;
        source = 2;
        return true;
    }

    headingDeg = 0.0f;
    source = 0;
    return false;
}

static bool FT02_MapCenterOnCurrentGnss()
{
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(!gnss.fixValid || !gnss.hasPosition) return false;

    FT02_PbfMapSetCenter(gnss.longitude, gnss.latitude);
    g_ft02MapHasTrackedFix = true;
    g_ft02MapLastFixValid = true;
    g_ft02MapLastFollowLat = gnss.latitude;
    g_ft02MapLastFollowLon = gnss.longitude;
    uint8_t headingSource = 0;
    float headingDeg = 0.0f;
    g_ft02MapLastHeadingValid = FT02_MapNavigationHeading(headingDeg, headingSource);
    g_ft02MapLastHeadingDeg = headingDeg;
    g_ft02MapLastHeadingSource = headingSource;
    const uint32_t now = millis();
    g_ft02MapLastFollowRefreshMs = now;
    g_ft02MapLastFollowCheckMs = now;
    return true;
}

static void FT02_ArmMapNavReleaseGate()
{
    g_ft02MapRequireNavRelease = true;
    g_ft02MapReleaseZeroSinceMs = 0;

}

static void FT02_CancelPendingMapRefresh()
{
    if(!g_ft02MapRefreshPending) return;

    g_ft02MapRefreshPending = false;
    g_ft02MapRefreshDeadlineMs = 0;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_NONE;
}

static void FT02_ScheduleMapZoomRefresh(int originalZoom)
{
    if(!g_ft02MapRefreshPending || g_ft02MapRefreshKind != FT02_MAP_REFRESH_ZOOM)
    {
        g_ft02MapPendingZoomFrom = originalZoom;
    }

    g_ft02MapRefreshPending = true;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_ZOOM;
    g_ft02MapRefreshDeadlineMs = millis() + FT02_MAP_ZOOM_SETTLE_MS;

}

static void FT02_ScheduleMapPanRefresh()
{
    g_ft02MapRefreshPending = true;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_PAN;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshDeadlineMs = millis() + FT02_MAP_PAN_SETTLE_MS;

}

static void FT02_ProcessPendingMapRefresh()
{
    if(!g_ft02MapRefreshPending) return;

    if(g_ft02PageState != FT02_PAGE_MAP)
    {
        FT02_CancelPendingMapRefresh();
        return;
    }

    const uint32_t now = millis();
    if(static_cast<int32_t>(now - g_ft02MapRefreshDeadlineMs) < 0)
    {
        return;
    }

    const FT02MapRefreshKind commitKind = g_ft02MapRefreshKind;
    g_ft02MapRefreshPending = false;
    g_ft02MapRefreshDeadlineMs = 0;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_NONE;

    // The following build + display commit is blocking. Disarm map navigation
    // before entering it so a held key cannot leak through as a new pan batch
    // when the loop resumes. The gate is released only after a zero raw key is
    // observed in loop().
    FT02_ArmMapNavReleaseGate();
    FT02_PbfMapSetLoadingReason(
        commitKind == FT02_MAP_REFRESH_ZOOM ? FT02_MAP_LOADING_ZOOM : FT02_MAP_LOADING_PAN
    );

    FT02_RunDirectPbfMap(false, true);
}

static void FT02_RunDirectPbfMap(bool resetDefault, bool showLoading)
{
    if(resetDefault)
    {
        FT02_PbfMapResetView();
        g_ft02MapHasTrackedFix = false;
    }

    FT02_PbfMapPrepare();
    if(showLoading)
    {

        FT02_RedrawCurrentPage();

    }

    FT02_PbfMapBuild();
    FT02_RedrawCurrentPage();
    g_ft02MapPartialNavRefreshCount = 0;
    FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_GENERIC);
}

static void FT02_UpdateMapGnssFollow()
{
    if(g_ft02PageState != FT02_PAGE_MAP || !g_ft02MapFollowGnss) return;
    if(g_ft02MapRefreshPending || g_ft02MapRequireNavRelease) return;

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    const bool fixChanged = gnss.fixValid != g_ft02MapLastFixValid;
    g_ft02MapLastFixValid = gnss.fixValid;

    if(!gnss.fixValid || !gnss.hasPosition)
    {
        // Keep the current map position. A lost fix must never snap the map
        // back to the default center. Redraw once only when fix state changes.
        if(fixChanged && FT02_PbfMapReportCurrent().state == FT02_PBF_MAP_READY)
        {
            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
        }
        return;
    }

    if(!g_ft02MapHasTrackedFix)
    {

        FT02_MapCenterOnCurrentGnss();
        FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_NAVIGATION);
        FT02_RunDirectPbfMap(false, true);
        return;
    }

    const uint32_t now = millis();
    if(now - g_ft02MapLastFollowCheckMs < FT02_MAP_FOLLOW_CHECK_MS)
    {
        return;
    }
    g_ft02MapLastFollowCheckMs = now;

    const double movedMeters = FT02_MapDistanceMeters(
        g_ft02MapLastFollowLat,
        g_ft02MapLastFollowLon,
        gnss.latitude,
        gnss.longitude
    );

    uint8_t headingSource = 0;
    float headingDeg = 0.0f;
    const bool headingValid = FT02_MapNavigationHeading(headingDeg, headingSource);
    const bool headingValidityChanged = headingValid != g_ft02MapLastHeadingValid;
    const bool headingSourceChanged = headingValid && g_ft02MapLastHeadingValid &&
        headingSource != g_ft02MapLastHeadingSource;
    float headingDelta = 0.0f;
    if(headingValid && g_ft02MapLastHeadingValid)
    {
        headingDelta = fabsf(headingDeg - g_ft02MapLastHeadingDeg);
        if(headingDelta > 180.0f) headingDelta = 360.0f - headingDelta;
    }
    const bool headingChanged = headingValidityChanged || headingSourceChanged ||
        (headingValid && g_ft02MapLastHeadingValid &&
         headingDelta >= FT02_MAP_HEADING_REFRESH_DEGREES);

    if(movedMeters < FT02_MAP_FOLLOW_DISTANCE_METERS)
    {
        if(!headingChanged)
        {
            return;
        }

        // Position is effectively unchanged, but GNSS course heading moved enough
        // to matter. Refresh only the marker patch so the arrow rotates without
        // rebuilding/recentering the map.
        const double stableLat = g_ft02MapLastFollowLat;
        const double stableLon = g_ft02MapLastFollowLon;

        if(g_ft02MapPartialNavRefreshCount + 1 >= FT02_MAP_PARTIAL_CLEANUP_COUNT)
        {
            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
        }
        else if(FT02_RefreshPbfMapNavigationPartial(
                    display,
                    stableLon,
                    stableLat,
                    stableLon,
                    stableLat))
        {
            g_ft02MapPartialNavRefreshCount++;
        }
        else
        {
            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
        }

        g_ft02MapLastHeadingValid = headingValid;
        g_ft02MapLastHeadingDeg = headingDeg;
        g_ft02MapLastHeadingSource = headingSource;
        g_ft02MapLastFollowRefreshMs = now;

        return;
    }

    int16_t localX = 0;
    int16_t localY = 0;
    const bool projected = FT02_PbfMapProjectCoordinate(
        gnss.longitude,
        gnss.latitude,
        localX,
        localY
    );

    const bool insideSafeBox = projected &&
        localX >= FT02_MAP_SAFE_MARGIN_X &&
        localX < (800 - FT02_MAP_SAFE_MARGIN_X) &&
        localY >= FT02_MAP_SAFE_MARGIN_Y &&
        localY < (364 - FT02_MAP_SAFE_MARGIN_Y);

    if(insideSafeBox)
    {
        const double oldLat = g_ft02MapLastFollowLat;
        const double oldLon = g_ft02MapLastFollowLon;
        g_ft02MapLastFollowLat = gnss.latitude;
        g_ft02MapLastFollowLon = gnss.longitude;
        g_ft02MapLastHeadingValid = headingValid;
        g_ft02MapLastHeadingDeg = headingDeg;
        g_ft02MapLastHeadingSource = headingSource;
        g_ft02MapLastFollowRefreshMs = now;

        if(g_ft02MapPartialNavRefreshCount + 1 >= FT02_MAP_PARTIAL_CLEANUP_COUNT)
        {

            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
            return;
        }

        if(FT02_RefreshPbfMapNavigationPartial(
            display,
            oldLon,
            oldLat,
            gnss.longitude,
            gnss.latitude
        ))
        {
            g_ft02MapPartialNavRefreshCount++;
        }
        else
        {

            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
        }
        return;
    }

    FT02_MapCenterOnCurrentGnss();
    FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_NAVIGATION);
    FT02_RunDirectPbfMap(false, true);
}

static void FT02_OpenMapPage()
{
    if(g_ft02PageState == FT02_PAGE_MAP)
    {
        return;
    }

    g_ft02PageState = FT02_PAGE_MAP;
    g_ft02MapRefreshPending = false;
    g_ft02MapRefreshDeadlineMs = 0;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_NONE;
    g_ft02MapRequireNavRelease = false;
    g_ft02MapReleaseZeroSinceMs = 0;
    g_ft02MapPartialNavRefreshCount = 0;
    g_ft02MapLastFollowCheckMs = 0;
    g_ft02MapFollowGnss = true;
    g_ft02MapHasTrackedFix = false;
    g_ft02MapLastFixValid = false;
    g_ft02MapLastHeadingValid = false;
    g_ft02MapLastHeadingDeg = 0.0f;
    g_ft02MapLastHeadingSource = 0;

    if(!FT02_MapCenterOnCurrentGnss())
    {
        FT02_PbfMapResetView();

    }

    FT02_RunDirectPbfMap(false, true);
}

static void FT02_OpenLocationRecorderPage()
{
    // Location-log SD loading is owned by a background task. Leaving the list
    // must not touch its FILE handle or wait for cancellation; let it finish
    // and keep the cache warm for the next visit.
    FT02_ReleaseLocationLogDetailMap();
    if(g_ft02PageState == FT02_PAGE_LOCATION_RECORDER)
    {
        return;
    }

    g_ft02PageState = FT02_PAGE_LOCATION_RECORDER;
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    const FT02LocationRecorderSnapshot recorder = FT02_LocationRecorderSnapshotCurrent();
    const FT02GnssFieldTestSnapshot fieldTest = FT02_GnssFieldTestSnapshotCurrent();
    g_ft02RecorderLastGnssGeneration = gnss.uiGeneration;
    g_ft02RecorderLastUiGeneration = recorder.uiGeneration;
    g_ft02GnssFieldTestLastGeneration = fieldTest.uiGeneration;

    FT02_RedrawCurrentPage();
    // Start the live-refresh interval only after the blocking e-paper commit
    // has completed. This prevents a long refresh from consuming the next
    // refresh window before input polling gets another turn.
    g_ft02RecorderLastScreenRefreshMs = millis();
}

static void FT02_ClampLocationLogSelection()
{
    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();
    if(status.count == 0)
    {
        g_ft02LocationLogSelectedIndex = 0;
    }
    else if(g_ft02LocationLogSelectedIndex >= status.count)
    {
        g_ft02LocationLogSelectedIndex = static_cast<uint16_t>(status.count - 1U);
    }
}

static void FT02_OpenLocationLogListPage(bool reload)
{
    FT02_ReleaseLocationLogDetailMap();
    g_ft02PageState = FT02_PAGE_LOCATION_LOG_LIST;
    g_ft02LocationLogSelectedIndex = 0;

    if(reload)
    {
        FT02_LocationLogStartReload();
    }
    FT02_ClampLocationLogSelection();
    g_ft02LocationLogLastGeneration = FT02_LocationLogStatusCurrent().generation;

    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();

    FT02_RedrawCurrentPage();
}

static bool FT02_OpenLocationLogDetailPage()
{
    FT02LocationLogEntry entry;
    if(!FT02_LocationLogGetNewest(g_ft02LocationLogSelectedIndex, entry))
    {
        return false;
    }

    FT02_PrepareLocationLogDetailMap(g_ft02LocationLogSelectedIndex);
    g_ft02PageState = FT02_PAGE_LOCATION_LOG_DETAIL;

    FT02_RedrawCurrentPage();
    return true;
}

static bool FT02_OpenLocationLogDeleteConfirmPage(FT02PageState returnPage)
{
    FT02LocationLogEntry entry;
    if(!FT02_LocationLogGetNewest(g_ft02LocationLogSelectedIndex, entry))
    {
        return false;
    }
    if(entry.active)
    {

        return false;
    }

    g_ft02LocationLogDeleteReturnPage = returnPage;
    g_ft02PageState = FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM;
    FT02_RedrawCurrentPage();
    return true;
}

static bool FT02_ArmAudioCommandAfterRelease(const FT02InputEvent& event)
{
    // EPD FIFO events are often replayed after the physical key has already
    // been released. Only require a release when this exact key is still down.
    const uint8_t currentRaw = static_cast<uint8_t>(FT02_InputCurrentRawKey());
    const uint8_t eventRaw = static_cast<uint8_t>(event.raw);
    const bool physicalKeyStillDown = currentRaw != 0 && currentRaw == eventRaw;

    if(!physicalKeyStillDown)
    {
        g_ft02AudioCommandReleaseRequired = false;
        return true;
    }
    if(g_ft02AudioCommandReleaseRequired)
    {

        return false;
    }
    g_ft02AudioCommandReleaseRequired = true;
    return true;
}

static void FT02_ClampAudioLogSelection()
{
    const FT02AudioLogStatus status = FT02_AudioLogStatusCurrent();
    if(status.count == 0)
    {
        g_ft02AudioLogSelectedIndex = 0;
    }
    else if(g_ft02AudioLogSelectedIndex >= status.count)
    {
        g_ft02AudioLogSelectedIndex = static_cast<uint16_t>(status.count - 1u);
    }
}

static void FT02_OpenAudioLogListPage(bool reload)
{
    // Voice logs are the primary audio owner. Bring ES8311/I2S up once on page
    // entry and keep them ready while the user records or listens.
    (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_JOURNAL);
    if(reload)
    {
        FT02_AudioLogReload();
    }
    FT02_ClampAudioLogSelection();
    g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
    g_ft02AudioLastShownRecordBucket = 0xFFFFFFFFu;
    g_ft02AudioLastShownVolume = 0xFFu;
    g_ft02PageState = FT02_PAGE_AUDIO_LOG_LIST;

    FT02_RedrawCurrentPage();
}

static bool FT02_OpenAudioLogDeleteConfirmPage()
{
    if(FT02_AudioLogIsBusy()) return false;
    FT02AudioLogEntry entry;
    if(!FT02_AudioLogGetNewest(g_ft02AudioLogSelectedIndex, entry)) return false;
    g_ft02PageState = FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM;
    FT02_RedrawCurrentPage();
    return true;
}

static void FT02_OpenFieldLibraryPage()
{
    if(g_ft02PageState == FT02_PAGE_LIBRARY) return;

    FT02_FieldLibraryBegin(false);
    FT02_FieldLibraryReset();
    g_ft02PageState = FT02_PAGE_LIBRARY;

    FT02_RedrawCurrentPage();
}

static void FT02_OpenKnowledgePage()
{
    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        return;
    }

    FT02_KnowledgeReset();
    g_ft02PageState = FT02_PAGE_KNOWLEDGE;

    // v2.48 optimization: preserve the proven controller power-cycle and
    // full-mode reinitialization, then submit the final knowledge page as the
    // first frame. This removes the separate visible white refresh.
    FT02_DrawKnowledgeAfterCleanTransition();
}

static void FT02_OpenCompassCalibrationPage();

static void FT02_OpenWirelessWiFiPage()
{
    g_ft02PageState = FT02_PAGE_WIRELESS_WIFI;
    FT02_WirelessManagerOpenWiFi();
    FT02_RedrawCurrentPage();
}

static bool FT02_HandleWirelessWiFiInput(const FT02InputEvent& event)
{
    const FT02WirelessAction action = FT02_WirelessManagerHandleWiFiInput(event);
    if(action == FT02_WIRELESS_ACTION_EXIT_TOOLS)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(action == FT02_WIRELESS_ACTION_OPEN_HELP)
    {
        FT02_WirelessManagerCloseWiFi();
        FT02_OpenHelpPage();
        return true;
    }
    if(action == FT02_WIRELESS_ACTION_PARTIAL_REDRAW)
    {
        FT02_DrawWirelessWiFiBodyPartial(display);
        return true;
    }
    if(action == FT02_WIRELESS_ACTION_REDRAW)
    {
        FT02_RedrawCurrentPage();
        return true;
    }
    return action != FT02_WIRELESS_ACTION_NONE;
}

static void FT02_OpenSystemSettingsPage()
{
    if(g_ft02PageState == FT02_PAGE_SYSTEM_SETTINGS) return;
    g_ft02PageState = FT02_PAGE_SYSTEM_SETTINGS;
    FT02_SystemSettingsUIOpen();
    FT02_RedrawCurrentPage();
}

static bool FT02_HandleSystemSettingsInput(const FT02InputEvent& event)
{
    const FT02SystemSettingsAction action = FT02_SystemSettingsUIHandleInput(event);
    if(action == FT02_SETTINGS_ACTION_EXIT_HOME)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(action == FT02_SETTINGS_ACTION_OPEN_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(action == FT02_SETTINGS_ACTION_REDRAW)
    {
        FT02_RedrawCurrentPage();
        return true;
    }
    return false;
}

static void FT02_OpenDeviceStatusPage()
{
    if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS) return;
    g_ft02PageState = FT02_PAGE_DEVICE_STATUS;
    // System self-test must validate the real codec/I2S path, so it owns audio
    // for the lifetime of this page.
    (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST);
    FT02_SystemSelfTestRun();

    FT02_RedrawCurrentPage();
}

static bool FT02_HandleDeviceStatusInput(const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP)
    {
        const uint8_t page = FT02_SystemSelfTestPage();
        if(page > 0)
        {
            FT02_SystemSelfTestSetPage(page - 1U);
            FT02_RedrawCurrentPage();
        }
        return true;
    }
    if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN)
    {
        const uint8_t page = FT02_SystemSelfTestPage();
        if(page + 1U < FT02_SystemSelfTestPageCount())
        {
            FT02_SystemSelfTestSetPage(page + 1U);
            FT02_RedrawCurrentPage();
        }
        return true;
    }
    if(event.key == FT02_KEY_SELECT)
    {
        const uint8_t page = FT02_SystemSelfTestPage();
        (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST);
        FT02_SystemSelfTestRun();
        FT02_SystemSelfTestSetPage(page);
        FT02_RedrawCurrentPage();
        return true;
    }
    return false;
}

static void FT02_OpenCompassPage()
{
    if(g_ft02PageState == FT02_PAGE_COMPASS) return;
    g_ft02PageState = FT02_PAGE_COMPASS;
    FT02_CompassUIOpen();

    FT02_RedrawCurrentPage();
}

static bool FT02_HandleCompassInput(const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(event.key == FT02_KEY_SELECT)
    {
        FT02_OpenCompassCalibrationPage();
        return true;
    }
    return false;
}

static void FT02_OpenCompassCalibrationPage()
{
    g_ft02PageState = FT02_PAGE_COMPASS_CALIBRATION;
    FT02_CompassCalibrationUIOpen();
    g_ft02LastCompassCalRevision = FT02_CompassCalibrationUIRevision();
    g_ft02LastCompassCalRedrawMs = millis();

    FT02_RedrawCurrentPage();
}

static bool FT02_HandleCompassCalibrationInput(const FT02InputEvent& event)
{
    const FT02CompassCalibrationUIAction action =
        FT02_CompassCalibrationUIHandleInput(event);

    if(action == FT02_COMPASS_CAL_UI_EXIT)
    {
        g_ft02PageState = FT02_PAGE_COMPASS;
        FT02_CompassUIOpen();
        FT02_RedrawCurrentPage();
        return true;
    }
    if(action == FT02_COMPASS_CAL_UI_REDRAW)
    {
        g_ft02LastCompassCalRevision = FT02_CompassCalibrationUIRevision();
        g_ft02LastCompassCalRedrawMs = millis();
        FT02_RedrawCurrentPage();
        return true;
    }
    return false;
}

static void FT02_OpenCommunicationPage()
{
    if(g_ft02PageState == FT02_PAGE_COMMUNICATION) return;

    g_ft02PageState = FT02_PAGE_COMMUNICATION;
    FT02_CommunicationUIOpen();
    g_ft02CommunicationReadyPresented = FT02_LoRaNodeRuntimeReady();
    g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
    g_ft02LastLoRaNodeRevision = FT02_LoRaNodeRuntimeRevision();
    g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();

    FT02_RedrawCurrentPage();
}

static bool FT02_HandleCommunicationInput(
    const FT02InputEvent& event
)
{
    const FT02CommunicationInputResult result = FT02_CommunicationUIHandleInput(event);
    if(result == FT02_COMM_INPUT_EXIT_HOME)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(result == FT02_COMM_INPUT_OPEN_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(result == FT02_COMM_INPUT_BROWSE_PARTIAL)
    {
        FT02_DrawCommunicationBrowsePartial(display);
        return true;
    }
    if(result == FT02_COMM_INPUT_REDRAW)
    {
        // Manual R resync resets NodeRuntime immediately. Mirror the real
        // readiness state here so the later READY transition is observable.
        // v2.70a left this flag true, which made the communication page remain
        // visually stuck at “同步中” even after NodeDB had recovered to READY.
        g_ft02CommunicationReadyPresented = FT02_LoRaNodeRuntimeReady();
        g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
        g_ft02LastLoRaNodeRevision = FT02_LoRaNodeRuntimeRevision();
        g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
        FT02_RedrawCurrentPage();
        return true;
    }
    return result != FT02_COMM_INPUT_NONE;
}

static bool FT02_HandleHomeInput(
    const FT02InputEvent& event
)
{
    int nextSelected = g_ft02HomeSelectedCard;

    if(event.key == FT02_KEY_LEFT)
    {
        nextSelected = FT02_MoveHomeCardSelection(
            g_ft02HomeSelectedCard,
            0,
            -1
        );
    }
    else if(event.key == FT02_KEY_RIGHT)
    {
        nextSelected = FT02_MoveHomeCardSelection(
            g_ft02HomeSelectedCard,
            0,
            1
        );
    }
    else if(event.key == FT02_KEY_UP)
    {
        nextSelected = FT02_MoveHomeCardSelection(
            g_ft02HomeSelectedCard,
            -1,
            0
        );
    }
    else if(event.key == FT02_KEY_DOWN)
    {
        nextSelected = FT02_MoveHomeCardSelection(
            g_ft02HomeSelectedCard,
            1,
            0
        );
    }
    else if(event.key == FT02_KEY_SELECT)
    {

        if(g_ft02HomeSelectedCard == 0)
        {
            FT02_OpenKnowledgePage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 1)
        {
            FT02_OpenMapPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 2)
        {
            FT02_OpenAudioLogListPage(true);
            return true;
        }

        if(g_ft02HomeSelectedCard == 3)
        {
            FT02_OpenLocationRecorderPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 4)
        {
            FT02_OpenCommunicationPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 5)
        {
            FT02_OpenFieldLibraryPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 6)
        {
            FT02_OpenDeviceStatusPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 7)
        {
            FT02_OpenWirelessWiFiPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 8)
        {
            FT02_OpenCompassPage();
            return true;
        }

        if(g_ft02HomeSelectedCard == 9)
        {
            FT02_OpenSystemSettingsPage();
            return true;
        }

        return false;
    }
    else if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    else if(event.key == FT02_KEY_BACK)
    {

        return false;
    }
    else
    {
        return false;
    }

    if(nextSelected == g_ft02HomeSelectedCard)
    {
        return false;
    }

    int oldSelectedCard = g_ft02HomeSelectedCard;
    g_ft02HomeSelectedCard = nextSelected;

    if(FT02_HomeCardPageForIndex(oldSelectedCard) !=
       FT02_HomeCardPageForIndex(g_ft02HomeSelectedCard))
    {
        FT02_DrawHomeScreen(display, g_ft02HomeSelectedCard);
    }
    else
    {
        FT02_RedrawHomeCardSelection(
            display,
            oldSelectedCard,
            g_ft02HomeSelectedCard
        );
    }

    return true;
}

static bool FT02_HandleHelpInput(
    const FT02InputEvent& event
)
{
    if(event.key == FT02_KEY_BACK)
    {
        FT02_ReturnFromHelpPage();
        return true;
    }

    if(event.key == FT02_KEY_HELP)
    {

        return false;
    }

    return false;
}

static bool FT02_HandleMapInput(
    const FT02InputEvent& event
)
{
    // A blocking e-paper map commit can outlive the key event that triggered it.
    // After search-jump or map commit, suppress every map command until the
    // keyboard has reported a real zero/raw release. This includes ENTER, not
    // just arrows: a stale search ENTER previously became FT02_KEY_SELECT on
    // the map page and caused repeated full map refreshes.
    if(g_ft02MapRequireNavRelease)
    {

        return true;
    }

    if(event.key == FT02_KEY_BACK)
    {
        FT02_CancelPendingMapRefresh();
        FT02_ReturnHomePage();
        return true;
    }

    if(event.key == FT02_KEY_HELP)
    {
        FT02_CancelPendingMapRefresh();
        FT02_OpenHelpPage();
        return true;
    }

    if(event.key == FT02_KEY_SELECT)
    {
        FT02_CancelPendingMapRefresh();
        FT02_RunDirectPbfMap(false, false);
        return true;
    }

    const int currentZoom = FT02_PbfMapZoomCurrent();
    const int panStep = currentZoom <= 16 ? 16 : (currentZoom == 17 ? 32 : 64);

    // A map commit blocks the main loop long enough for a held direction key
    // to look like a new press afterward. Require a real key release before
    // accepting another map move or zoom.
    if(event.key == FT02_KEY_LEFT)
    {
        g_ft02MapFollowGnss = false;
        FT02_PbfMapMovePixels(-panStep, 0);
        FT02_ScheduleMapPanRefresh();
        return true;
    }

    if(event.key == FT02_KEY_RIGHT)
    {
        g_ft02MapFollowGnss = false;
        FT02_PbfMapMovePixels(panStep, 0);
        FT02_ScheduleMapPanRefresh();
        return true;
    }

    if(event.key == FT02_KEY_UP)
    {
        g_ft02MapFollowGnss = false;
        FT02_PbfMapMovePixels(0, -panStep);
        FT02_ScheduleMapPanRefresh();
        return true;
    }

    if(event.key == FT02_KEY_DOWN)
    {
        g_ft02MapFollowGnss = false;
        FT02_PbfMapMovePixels(0, panStep);
        FT02_ScheduleMapPanRefresh();
        return true;
    }

    if(event.key == FT02_KEY_CHAR)
    {
        char raw = event.raw;
        if(raw == 'q' || raw == 'Q' || raw == '+' || raw == '=')
        {
            const int before = FT02_PbfMapZoomCurrent();

            if(FT02_PbfMapChangeZoom(1))
            {
                FT02_ScheduleMapZoomRefresh(before);
            }
            return true;
        }
        if(raw == 'e' || raw == 'E' || raw == '-' || raw == '_')
        {
            const int before = FT02_PbfMapZoomCurrent();

            if(FT02_PbfMapChangeZoom(-1))
            {
                FT02_ScheduleMapZoomRefresh(before);
            }
            return true;
        }
        if(event.command == 'r')
        {
            FT02_CancelPendingMapRefresh();
            // R means "return to my position". Without a live fix, never
            // destroy the user's current center/zoom by falling back to the
            // default map center.
            g_ft02MapFollowGnss = true;
            if(FT02_MapCenterOnCurrentGnss())
            {
                FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_NAVIGATION);
                FT02_RunDirectPbfMap(false, true);
            }
            return true;
        }
        if(event.command == 'f')
        {
            FT02_CancelPendingMapRefresh();
            FT02_PbfMapInvalidateCache();
            FT02_RunDirectPbfMap(false, true);
            return true;
        }
        if(event.command == 's')
        {
            FT02_OpenMapSearchPage();
            return true;
        }
    }

    return false;
}

static void FT02_OpenMapSearchPage()
{
    FT02_CancelPendingMapRefresh();
    g_ft02MapRequireNavRelease = false;
    g_ft02MapReleaseZeroSinceMs = 0;
    FT02_MapSearchOpen();
    g_ft02PageState = FT02_PAGE_MAP_SEARCH;

    FT02_RedrawCurrentPage();
}

static bool FT02_HandleMapSearchInput(const FT02InputEvent& event)
{
    const FT02MapSearchAction action = FT02_MapSearchHandleInput(event);
    if(action == FT02_MAP_SEARCH_ACTION_EXIT_MAP)
    {
        g_ft02PageState = FT02_PAGE_MAP;

        FT02_RedrawCurrentPage();
        return true;
    }
    if(action == FT02_MAP_SEARCH_ACTION_JUMP)
    {
        double lon = 0.0;
        double lat = 0.0;
        char name[72] = {};
        if(FT02_MapSearchTakeJump(lon, lat, name, sizeof(name)))
        {
            g_ft02MapFollowGnss = false;
            FT02_CancelPendingMapRefresh();
            // ENTER selected the result. The map build/display commit can block for
            // seconds (or ~1 minute on a cold regional-cache build), so arm the
            // release gate before changing page state. Any buffered/repeated
            // ENTER is then discarded when the loop resumes instead of being
            // reinterpreted as the map-page manual-refresh command.
            FT02_ArmMapNavReleaseGate();
            FT02_PbfMapSetCenter(lon, lat);
            g_ft02PageState = FT02_PAGE_MAP;

            // Show a loading state before the potentially blocking PBF build.
            FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_SEARCH);
            FT02_RunDirectPbfMap(false, true);
        }
        return true;
    }
    if(action == FT02_MAP_SEARCH_ACTION_SELECTION_PARTIAL)
    {
        FT02_DrawMapSearchSelectionPartial(display);
        return true;
    }
    if(action == FT02_MAP_SEARCH_ACTION_REDRAW)
    {
        FT02_RedrawCurrentPage();
        return true;
    }
    return action != FT02_MAP_SEARCH_ACTION_NONE;
}

static bool FT02_HandleLocationRecorderInput(
    const FT02InputEvent& event
)
{
    if(event.key == FT02_KEY_BACK)
    {
        // Leaving the recorder page must not terminate an active journey.
        // FT02_LocationRecorderPoll() runs from the global main loop, so the
        // session, elapsed time and automatic track points continue in the
        // background while the user visits Home, Map or Knowledge pages.
        const FT02LocationRecorderSnapshot recorder =
            FT02_LocationRecorderSnapshotCurrent();

        FT02_ReturnHomePage();
        return true;
    }

    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }

    bool handled = false;

    if(event.key == FT02_KEY_SELECT)
    {
        FT02_LocationRecorderToggleSession();
        handled = true;
    }
    else if(event.key == FT02_KEY_CHAR)
    {
        const char command = event.command;
        if(command == 'p')
        {
            FT02_LocationRecorderRecordManualPoint();
            handled = true;
        }
        else if(command == 'a')
        {
            FT02_LocationRecorderToggleAutoTrack();
            handled = true;
        }
        else if(command == 'r')
        {
            handled = true;
        }
        else if(command == 't')
        {
            FT02_GnssFieldTestToggle();
            handled = true;
        }
        else if(command == 'l')
        {
            FT02_OpenLocationLogListPage(true);
            return true;
        }
    }

    if(handled)
    {
        const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
        const FT02LocationRecorderSnapshot recorder = FT02_LocationRecorderSnapshotCurrent();
        const FT02GnssFieldTestSnapshot fieldTest = FT02_GnssFieldTestSnapshotCurrent();
        g_ft02RecorderLastGnssGeneration = gnss.uiGeneration;
        g_ft02RecorderLastUiGeneration = recorder.uiGeneration;
        g_ft02GnssFieldTestLastGeneration = fieldTest.uiGeneration;
        FT02_DrawLocationRecorderMiddlePartial(display, gnss, recorder);
        // Button-driven redraws are intentional. Restart the automatic UI
        // interval only after the e-paper operation has really finished.
        g_ft02RecorderLastScreenRefreshMs = millis();
        return true;
    }

    return false;
}

static bool FT02_HandleLocationLogListInput(
    const FT02InputEvent& event
)
{
    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();

    if(event.key == FT02_KEY_BACK)
    {
        FT02_OpenLocationRecorderPage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        // Keep any location-log background read alive while Help is open.
        // The worker never blocks CardKB/GNSS/UI.
        FT02_OpenHelpPage();
        return true;
    }

    // While the JSONL index is loading, keep BACK/HELP/R responsive but do
    // not navigate partially built entries.
    if(status.loading)
    {
        if(event.key == FT02_KEY_CHAR && event.command == 'r')
        {
            g_ft02LocationLogSelectedIndex = 0;
            FT02_LocationLogStartReload();
            g_ft02LocationLogLastGeneration = FT02_LocationLogStatusCurrent().generation;
            FT02_DrawLocationLogListBodyPartial(display, g_ft02LocationLogSelectedIndex);
            return true;
        }
        return false;
    }

    if(event.key == FT02_KEY_SELECT)
    {
        return FT02_OpenLocationLogDetailPage();
    }

    uint16_t next = g_ft02LocationLogSelectedIndex;
    if(status.count > 0 && event.key == FT02_KEY_UP)
    {
        next = FT02_LocationLogWrapIndex(next, status.count, -1);
    }
    else if(status.count > 0 && event.key == FT02_KEY_DOWN)
    {
        next = FT02_LocationLogWrapIndex(next, status.count, 1);
    }
    else if(event.key == FT02_KEY_LEFT && status.count > 0)
    {
        next = FT02_LocationLogWrapIndex(next, status.count, -5);
    }
    else if(event.key == FT02_KEY_RIGHT && status.count > 0)
    {
        next = FT02_LocationLogWrapIndex(next, status.count, 5);
    }
    else if(event.key == FT02_KEY_CHAR)
    {
        const char command = event.command;
        if(command == 'r')
        {
            g_ft02LocationLogSelectedIndex = 0;
            FT02_LocationLogStartReload();
            g_ft02LocationLogLastGeneration = FT02_LocationLogStatusCurrent().generation;
            FT02_DrawLocationLogListBodyPartial(display, g_ft02LocationLogSelectedIndex);
            return true;
        }
        if(command == 't')
        {
            return FT02_OpenLocationLogDeleteConfirmPage(FT02_PAGE_LOCATION_LOG_LIST);
        }
        return false;
    }
    else
    {
        return false;
    }

    if(next == g_ft02LocationLogSelectedIndex) return false;
    const uint16_t oldSelected = g_ft02LocationLogSelectedIndex;
    g_ft02LocationLogSelectedIndex = next;
    FT02_DrawLocationLogListSelectionPartial(
        display, oldSelected, g_ft02LocationLogSelectedIndex
    );
    return true;
}

static bool FT02_HandleLocationLogDetailInput(
    const FT02InputEvent& event
)
{
    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();

    if(event.key == FT02_KEY_BACK)
    {
        FT02_ReleaseLocationLogDetailMap();
        g_ft02PageState = FT02_PAGE_LOCATION_LOG_LIST;
        FT02_RedrawCurrentPage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }

    uint16_t next = g_ft02LocationLogSelectedIndex;
    if(status.count > 0 &&
       (event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP))
    {
        next = FT02_LocationLogWrapIndex(next, status.count, -1);
    }
    else if(status.count > 0 &&
            (event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN))
    {
        next = FT02_LocationLogWrapIndex(next, status.count, 1);
    }
    else if(event.key == FT02_KEY_CHAR &&
            event.command == 't')
    {
        return FT02_OpenLocationLogDeleteConfirmPage(FT02_PAGE_LOCATION_LOG_DETAIL);
    }
    else
    {
        return false;
    }

    if(next == g_ft02LocationLogSelectedIndex) return false;
    g_ft02LocationLogSelectedIndex = next;
    FT02_PrepareLocationLogDetailMap(g_ft02LocationLogSelectedIndex);
    FT02_DrawLocationLogDetailBodyPartial(display, g_ft02LocationLogSelectedIndex);
    return true;
}

static bool FT02_HandleLocationLogDeleteConfirmInput(
    const FT02InputEvent& event
)
{
    if(event.key == FT02_KEY_BACK)
    {
        g_ft02PageState = g_ft02LocationLogDeleteReturnPage;
        FT02_RedrawCurrentPage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(event.key != FT02_KEY_SELECT) return false;

    FT02LocationLogEntry entry;
    if(!FT02_LocationLogGetNewest(g_ft02LocationLogSelectedIndex, entry))
    {
        g_ft02PageState = FT02_PAGE_LOCATION_LOG_LIST;
        FT02_RedrawCurrentPage();
        return true;
    }

    const bool deleted = FT02_LocationLogDeleteSession(entry.sessionId);
    if(deleted) FT02_ReleaseLocationLogDetailMap();
    FT02_ClampLocationLogSelection();
    g_ft02PageState = deleted
        ? FT02_PAGE_LOCATION_LOG_LIST
        : g_ft02LocationLogDeleteReturnPage;
    FT02_RedrawCurrentPage();
    return true;
}

static bool FT02_HandleAudioLogListInput(
    const FT02InputEvent& event
)
{
    const FT02AudioLogStatus status = FT02_AudioLogStatusCurrent();
    const FT02AudioLogState state = FT02_AudioLogStateCurrent();

    if(state == FT02_AUDIO_LOG_PLAYING)
    {
        if(event.key == FT02_KEY_UP || event.key == FT02_KEY_DOWN)
        {
            const int8_t delta = event.key == FT02_KEY_UP ? 1 : -1;
            if(FT02_AudioLogAdjustPlaybackVolume(delta))
            {
                const FT02AudioLogStatus updated = FT02_AudioLogStatusCurrent();
                // Audible gain changes immediately. Coalesce rapid taps, then
                // refresh only the volume line under the audio EPD guard.
                g_ft02AudioVolumeRefreshPending = true;
                g_ft02AudioVolumeRefreshDueMs = millis() +
                    FT02_AUDIO_VOLUME_REFRESH_COALESCE_MS;

                return true;
            }
            return false;
        }

        const bool stopKey =
            event.key == FT02_KEY_BACK ||
            event.key == FT02_KEY_SELECT ||
            (event.key == FT02_KEY_CHAR &&
             event.command == 'p');
        if(stopKey)
        {
            if(!FT02_ArmAudioCommandAfterRelease(event)) return false;
            g_ft02AudioVolumeRefreshPending = false;
            g_ft02AudioVolumeRefreshDueMs = 0;
            const bool requested = FT02_AudioLogRequestPlaybackStop();
            if(requested)
            {
                // Do not energize the EPD while the PA is still active. The
                // audio task shuts down the speaker path first, then the list
                // refresh occurs from the normal generation update.

            }
            return requested;
        }

        return false;
    }

    if(state == FT02_AUDIO_LOG_RECORDING || state == FT02_AUDIO_LOG_POST_ROLL)
    {
        if(event.key == FT02_KEY_CHAR && event.command == 'r')
        {
            if(!FT02_ArmAudioCommandAfterRelease(event)) return false;
            const bool requested = FT02_AudioLogRequestStop();
            if(requested)
            {
                // A held/repeated R can survive the stop/save EPD work as a
                // deferred FIFO event. Do not let that stale event start a
                // second recording when the state returns to IDLE.
                g_ft02AudioRecordRestartGuardUntilMs = millis() + 2500u;
            }
            return requested;
        }

        return false;
    }

    if(event.key == FT02_KEY_BACK)
    {
        FT02_ReturnHomePage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }

    if(event.key == FT02_KEY_CHAR && event.command == 'r')
    {
        if(static_cast<int32_t>(millis() - g_ft02AudioRecordRestartGuardUntilMs) < 0)
        {

            return true;
        }
        if(!FT02_ArmAudioCommandAfterRelease(event)) return false;

        // Recording UX intentionally keeps the proven cue-before-capture order:
        // first commit "00:00 / recording" to e-paper, then begin ADC capture.
        // The ~1 s visual delay is acceptable and prevents the initial high-
        // voltage EPD transaction from contaminating the first captured audio.
        FT02AudioLogStatus cueStatus = status;
        cueStatus.recordingElapsedMs = 0;
        cueStatus.recordingSeconds = 0;
        cueStatus.stopRequested = false;
        cueStatus.activeAudioId[0] = '\0';
        FT02_DrawAudioLogRecordingBodyPartial(display, cueStatus);
        delay(40);
        const bool started = FT02_AudioLogStartRecording(FT02_GnssSnapshotCurrent());
        g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
        if(started)
        {
            g_ft02AudioLastShownRecordBucket = 0;

        }
        else
        {
            FT02_DrawAudioLogListBodyPartial(display, g_ft02AudioLogSelectedIndex);
        }
        return true;
    }

    if((event.key == FT02_KEY_SELECT) ||
       (event.key == FT02_KEY_CHAR && event.command == 'p'))
    {
        if(!FT02_ArmAudioCommandAfterRelease(event)) return false;
        FT02AudioLogEntry entry;
        if(!FT02_AudioLogGetNewest(g_ft02AudioLogSelectedIndex, entry)) return false;

        // Start opens and scans the WAV but streaming does not begin until the
        // background audio task runs, so the playback screen can be committed first.
        const bool started = FT02_AudioLogPlayNewest(g_ft02AudioLogSelectedIndex);
        g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
        if(started)
        {
            const FT02AudioLogStatus updated = FT02_AudioLogStatusCurrent();
            g_ft02AudioLastShownVolume = updated.playbackVolumeLevel;
            g_ft02AudioVolumeRefreshPending = false;
            g_ft02AudioVolumeRefreshDueMs = 0;
            FT02_DrawAudioLogPlayingBodyPartial(display, entry);
            FT02_AudioLogReleasePlaybackStart();
        }
        else
        {
            FT02_DrawAudioLogListBodyPartial(display, g_ft02AudioLogSelectedIndex);
        }
        return true;
    }

    if(event.key == FT02_KEY_CHAR && event.command == 't')
    {
        if(!FT02_ArmAudioCommandAfterRelease(event)) return false;
        return FT02_OpenAudioLogDeleteConfirmPage();
    }

    uint16_t next = g_ft02AudioLogSelectedIndex;
    if(status.count > 0 && event.key == FT02_KEY_UP)
    {
        next = FT02_AudioLogWrapIndex(next, status.count, -1);
    }
    else if(status.count > 0 && event.key == FT02_KEY_DOWN)
    {
        next = FT02_AudioLogWrapIndex(next, status.count, 1);
    }
    else if(status.count > 0 && event.key == FT02_KEY_LEFT)
    {
        next = FT02_AudioLogWrapIndex(next, status.count, -5);
    }
    else if(status.count > 0 && event.key == FT02_KEY_RIGHT)
    {
        next = FT02_AudioLogWrapIndex(next, status.count, 5);
    }
    else
    {
        return false;
    }

    if(next == g_ft02AudioLogSelectedIndex) return false;
    const uint16_t oldSelected = g_ft02AudioLogSelectedIndex;
    g_ft02AudioLogSelectedIndex = next;
    FT02_DrawAudioLogListSelectionPartial(
        display, oldSelected, g_ft02AudioLogSelectedIndex
    );
    return true;
}

static bool FT02_HandleAudioLogDeleteConfirmInput(
    const FT02InputEvent& event
)
{
    if(event.key == FT02_KEY_BACK)
    {
        g_ft02PageState = FT02_PAGE_AUDIO_LOG_LIST;
        FT02_RedrawCurrentPage();
        return true;
    }
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }
    if(event.key != FT02_KEY_SELECT) return false;
    if(!FT02_ArmAudioCommandAfterRelease(event)) return false;

    (void)FT02_AudioLogDeleteNewest(g_ft02AudioLogSelectedIndex);
    FT02_ClampAudioLogSelection();
    g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
    g_ft02PageState = FT02_PAGE_AUDIO_LOG_LIST;
    FT02_RedrawCurrentPage();
    return true;
}

static bool FT02_HandleKnowledgeInput(
    const FT02InputEvent& event
)
{
    // FieldManual search is a free-form text surface. InputManager still
    // classifies raw 'h' as the global HELP shortcut, but while search input
    // is active the raw letter must reach KnowledgeUI first (same principle
    // already used there for B/D/Z/X/C).
    if(event.key == FT02_KEY_HELP && !FT02_KnowledgeTextInputActive())
    {
        FT02_OpenHelpPage();
        return true;
    }

    const FT02KnowledgeAction action = FT02_HandleKnowledgeInput(
        display,
        event
    );

    if(action == FT02_KNOWLEDGE_ACTION_EXIT_HOME)
    {
        FT02_ReturnHomePage();
        return true;
    }

    if(event.key != FT02_KEY_NONE)
    {
        // Knowledge UI already committed the current page, including the live
        // cached clock. Do not issue a second status-bar refresh here.
        return true;
    }

    return false;
}

static bool FT02_HandleFieldLibraryInputMain(
    const FT02InputEvent& event
)
{
    if(event.key == FT02_KEY_HELP)
    {
        FT02_OpenHelpPage();
        return true;
    }

    const FT02FieldLibraryAction action =
        FT02_HandleFieldLibraryInput(display, event);
    if(action == FT02_LIBRARY_ACTION_EXIT_HOME)
    {
        FT02_ReturnHomePage();
        return true;
    }
    return event.key != FT02_KEY_NONE;
}

static bool FT02_HandleInput(
    const FT02InputEvent& event
)
{
    if(g_ft02PageState == FT02_PAGE_HELP)
    {
        return FT02_HandleHelpInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_MAP)
    {
        return FT02_HandleMapInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_MAP_SEARCH)
    {
        return FT02_HandleMapSearchInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        return FT02_HandleKnowledgeInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_LIBRARY)
    {
        return FT02_HandleFieldLibraryInputMain(event);
    }

    if(g_ft02PageState == FT02_PAGE_COMMUNICATION)
    {
        return FT02_HandleCommunicationInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)
    {
        return FT02_HandleDeviceStatusInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_COMPASS)
    {
        return FT02_HandleCompassInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        return FT02_HandleCompassCalibrationInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)
    {
        return FT02_HandleWirelessWiFiInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_SYSTEM_SETTINGS)
    {
        return FT02_HandleSystemSettingsInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_LOCATION_RECORDER)
    {
        return FT02_HandleLocationRecorderInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_LIST)
    {
        return FT02_HandleLocationLogListInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_DETAIL)
    {
        return FT02_HandleLocationLogDetailInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM)
    {
        return FT02_HandleLocationLogDeleteConfirmInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST)
    {
        return FT02_HandleAudioLogListInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM)
    {
        return FT02_HandleAudioLogDeleteConfirmInput(event);
    }

    return FT02_HandleHomeInput(event);
}

static void FT02_EpdSignalsEnterHighImpedance()
{
    // EPD uses software SPI in this validation build, so there is no hardware
    // SPI peripheral to stop. Release SCK/MOSI directly before cutting power.
    // Charging/source transitions can momentarily disturb the EPD rail. The
    // old A1.4 path left RST floating after power-off; if the panel/controller
    // received even partial power again, floating control lines could make the
    // display visibly leave the retained sleep frame while the FT-02 firmware
    // was still asleep. Keep RESET actively LOW for the entire sleep state.
    pinMode(EPD_RST, OUTPUT);
    digitalWrite(EPD_RST, LOW);

    // The remaining host-driven lines are released to high impedance to avoid
    // phantom-powering an unpowered panel. RESET LOW is sufficient to prevent
    // accidental controller activity if the panel rail is disturbed.
    pinMode(EPD_CS, INPUT);
    pinMode(EPD_DC, INPUT);
    pinMode(EPD_MOSI, INPUT);
    pinMode(EPD_SCK, INPUT);
}

static void FT02_EpdSignalsRestoreForWake()
{
    // Restore deterministic idle levels before the panel is reinitialized.
    pinMode(EPD_CS, OUTPUT);
    pinMode(EPD_DC, OUTPUT);
    pinMode(EPD_RST, OUTPUT);
    pinMode(EPD_MOSI, OUTPUT);
    pinMode(EPD_SCK, OUTPUT);

    digitalWrite(EPD_CS, HIGH);
    digitalWrite(EPD_DC, LOW);
    digitalWrite(EPD_RST, LOW);
    digitalWrite(EPD_MOSI, LOW);
    digitalWrite(EPD_SCK, LOW);
}

static bool g_ft02KeyboardSclWakeArmed = false;
static_assert(CARDKB_SCL >= 0 && CARDKB_SCL <= 21, "CardKB2 SCL must remain RTC/EXT1 capable on ESP32-S3");
static constexpr uint64_t FT02_CARDKB_SCL_EXT1_MASK = (uint64_t{1} << CARDKB_SCL);

static void FT02_DisarmKeyboardSclWake()
{
    if(!g_ft02KeyboardSclWakeArmed) return;

    // Remove only CardKB2/SCL from EXT1. Do not touch unrelated GPIO wake users
    // such as the optional LoRa light-sleep bridge.
    (void)esp_sleep_disable_ext1_wakeup_io(FT02_CARDKB_SCL_EXT1_MASK);
    g_ft02KeyboardSclWakeArmed = false;
}

static bool FT02_SleepWaitForKeyboardSignal()
{
    // GPIO21/SCL is already an RTC input here. EXT1 ANY_LOW is the only
    // CardKB2 wake source; no digital GPIO interrupt is installed for SCL.
    g_ft02SleepNoInitTrace.phase = 20u; // BEFORE_EXT1_WAKE_CONFIG
    ++g_ft02SleepNoInitTrace.sleepCount;
    ++g_ft02SleepSliceCount;

    FT02_DisarmKeyboardSclWake();

    if(!FT02_InputSleepSclRtcReady())
    {
        g_ft02SleepNoInitTrace.lastSleepErr = static_cast<int32_t>(ESP_ERR_INVALID_STATE);
        g_ft02SleepNoInitTrace.phase = 22u; // RTC_SCL_OWNERSHIP_LOST
        return false;
    }

    // Close the pre-arm race. CardKB2 may already be pulling SCL LOW.
    if(FT02_InputSleepSclIsLow())
    {
        g_ft02SleepNoInitTrace.phase = 35u; // EARLY_SCL_LOW
        g_ft02SleepNoInitTrace.lastWakeCause = static_cast<int32_t>(ESP_SLEEP_WAKEUP_EXT1);
        return true;
    }

    const esp_err_t wakeErr = esp_sleep_enable_ext1_wakeup_io(
        FT02_CARDKB_SCL_EXT1_MASK,
        ESP_EXT1_WAKEUP_ANY_LOW
    );
    if(wakeErr != ESP_OK)
    {
        g_ft02SleepNoInitTrace.lastSleepErr = static_cast<int32_t>(wakeErr);
        g_ft02SleepNoInitTrace.phase = 21u; // EXT1_WAKE_CONFIG_FAILED
        return false;
    }

    g_ft02KeyboardSclWakeArmed = true;

    // Close the final arm race once more before entering Light Sleep.
    if(FT02_InputSleepSclIsLow())
    {
        g_ft02SleepNoInitTrace.phase = 36u; // ARMED_SCL_LOW
        FT02_DisarmKeyboardSclWake();
        g_ft02SleepNoInitTrace.lastWakeCause = static_cast<int32_t>(ESP_SLEEP_WAKEUP_EXT1);
        return true;
    }

    g_ft02SleepNoInitTrace.phase = 30u; // BEFORE_LIGHT_SLEEP
    FT02_SleepTruthBeforeLightSleep();
    const esp_err_t err = esp_light_sleep_start();

    g_ft02SleepNoInitTrace.phase = 40u; // AFTER_LIGHT_SLEEP
    ++g_ft02SleepNoInitTrace.wakeCount;
    g_ft02SleepNoInitTrace.lastSleepErr = static_cast<int32_t>(err);
    const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    g_ft02SleepNoInitTrace.lastWakeCause = static_cast<int32_t>(cause);
    g_ft02SleepNoInitTrace.phase = 50u; // WAKE_CAUSE_RECORDED

    const uint64_t ext1Status =
        cause == ESP_SLEEP_WAKEUP_EXT1 ? esp_sleep_get_ext1_wakeup_status() : 0u;

    FT02_SleepTruthAfterLightSleep(
        static_cast<int32_t>(err),
        static_cast<int32_t>(cause),
        ext1Status
    );

    FT02_DisarmKeyboardSclWake();

    return err == ESP_OK &&
           cause == ESP_SLEEP_WAKEUP_EXT1 &&
           (ext1Status & FT02_CARDKB_SCL_EXT1_MASK) != 0u;
}

static bool FT02_SleepBlocked(bool automatic, const char** reason)
{
    const char* why = nullptr;
    if(FT02_AudioLogIsBusy()) why = "audio_busy";
    else if(FT02_LocationRecorderSnapshotCurrent().sessionActive) why = "track_active";
    else if(FT02_GnssFieldTestSnapshotCurrent().active) why = "gnss_test_active";
    else if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION) why = "compass_calibration";
    else if(g_ft02MapRefreshPending) why = "map_refresh";
    else if(FT02_MeshServiceReady() && !FT02_MeshServiceSleepAvailable()) why = "mesh_busy";
    else if(!FT02_InputCardKbAvailable()) why = "keyboard_unavailable";
    else if(automatic && g_ft02HomePPressActive) why = "home_p_active";
    else if(automatic && FT02_EpdInteractiveQueuedInputCount() > 0u) why = "input_pending";
    if(reason) *reason = why;
    return why != nullptr;
}

static void FT02_EnterSleepMode(bool automatic)
{
    if(FT02_ModeIsSleeping()) return;
    if(automatic && !FT02_ModeAutoSleepAllowed()) return;
    const char* sleepBlockReason = nullptr;
    if(FT02_SleepBlocked(automatic, &sleepBlockReason))
    {
        if(!automatic)
            FT02_LOGW("[WARN][SLEEP] blocked reason=%s\n", sleepBlockReason ? sleepBlockReason : "busy");
        return;
    }

    g_ft02SleepReturnPage = g_ft02PageState;
    g_ft02HomePPressActive = false;
    g_ft02HomePLongTriggered = false;
    g_ft02HomePEventCount = 0u;
    g_ft02HomePLastEventMs = 0u;

    if(g_ft02PageState == FT02_PAGE_MAP) FT02_CancelPendingMapRefresh();

    // Sleep owns the low-power policy. Active audio work is blocked above;
    // idle page owners can be released safely and are reacquired on wake.
    FT02_AudioLogRelease(FT02_AUDIO_OWNER_JOURNAL);
    FT02_AudioLogRelease(FT02_AUDIO_OWNER_SYSTEM_TEST);

    const FT02WifiOwner wifiOwner = FT02_WifiManagerOwner();
    if(wifiOwner != FT02_WIFI_OWNER_NONE) FT02_WifiManagerShutdown(wifiOwner);

    // Indoor/outdoor policy is remembered, but sleep always forces GNSS off.
    // Sleep Power A3 also closes the Core UART transport through that policy.
    FT02_ModeEnterSleep();

    const bool compassSleepOk = FT02_CompassCoreSleepEnter();
    FT02_MeshServiceSleepEnter();
    FT02_LOGD("[SLEEP-PWR] enter compass=%d mesh_available=%d gnss_runtime=%d\n",
                  compassSleepOk ? 1 : 0,
                  FT02_MeshServiceSleepAvailable() ? 1 : 0,
                  FT02_GnssRuntimeEnabled() ? 1 : 0);

    // Commit one static sleep frame, then remove the e-paper power rail. The
    // bistable panel keeps the image without refresh power.
    FT02_DrawSleepScreen(display);
    delay(30);

    // Cut the panel rail, then clamp EPD RESET low before the Core starts its
    // periodic light-sleep slices. This makes the retained image immune to the
    // charger/source-switch disturbance seen in A1.4.3.
    digitalWrite(EPD_PWR, LOW);
    FT02_EpdSignalsEnterHighImpedance();

    g_ft02PreSleepCpuMhz = getCpuFrequencyMhz();
    if(g_ft02PreSleepCpuMhz > 80u) (void)setCpuFrequencyMhz(80u);

    // A manual long-P sleep is entered while P is still physically held.
    // Wake is armed only after a stable release, preventing instant bounce-back.
    g_ft02WakeReleaseRequired = false;
    g_ft02WakeLastConsumedKeyMs = 0u;
    g_ft02SleepEnteredMs = millis();
    g_ft02SleepSliceCount = 0u;
    FT02_EpdInteractiveReset();
    FT02_InputSetLowNoiseMode(true);

    // A5B diagnostics use only the already-cached sample here. Do not insert
    // a new shared-I2C battery transaction into the sleep-entry path.
    g_ft02PreSleepBattery = FT02_BatterySnapshotCurrent();
    g_ft02PreSleepBatteryValid = g_ft02PreSleepBattery.valid;
    FT02_BatteryMonitorSleepPause();
    FT02_LOGD_PRINTLN("[BAT] sleep pause");

    // The SD card has no controllable power gate on the current FT-02. A5B
    // therefore removes host SPI clocks only: sync, CS HIGH, FSPI end, pins
    // high-impedance. FAT remains mounted and protocol state is retained.
    if(!FT02_StorageSleepEnter())
    {
        FT02_LOGW_PRINTLN("[WARN][SLEEP-A5B] SD FSPI quiesce failed");
        FT02_WakeFromSleep();
        return;
    }

    // Start a fresh crash-safe RCA session before the first sleep entry.
    memset(&g_ft02SleepNoInitTrace, 0, sizeof(g_ft02SleepNoInitTrace));
    g_ft02SleepNoInitTrace.magic = FT02_SLEEP_NOINIT_MAGIC;
    g_ft02SleepNoInitTrace.version = FT02_SLEEP_NOINIT_VERSION;
    g_ft02SleepNoInitTrace.phase = 10u; // SLEEP_SESSION_ENTER
    g_ft02SleepNoInitTrace.sessionActive = 1u;

    // Arm CardKB2 only after all other sleep-entry work is complete. From this
    // command onward FT02 performs no shared-I2C peripheral work before Wire is
    // released, minimizing the handoff race between the two MCUs.
    if(!FT02_InputRequestHostSleep())
    {
        FT02_LOGW_PRINTLN("[WARN][SLEEP-A4] CardKB2 two-phase sleep handshake failed");
        FT02_WakeFromSleep();
        return;
    }

    // Native USB CDC is not a valid dependency while light sleep gates the USB
    // clock. Disable input diagnostics before releasing the shared Wire bus.
    FT02_InputSetDiagnosticsEnabled(false);

    // No shared-I2C transaction is allowed after this point until wake.
    // COMMIT transfers GPIO21/SCL to RTC input ownership for EXT1 ANY_LOW wake.
    if(!FT02_InputSuspendBusForSleep())
    {
        FT02_InputSetDiagnosticsEnabled(true);
        FT02_LOGW_PRINTLN("[WARN][SLEEP-A4] failed to suspend shared I2C bus");
        FT02_WakeFromSleep();
        return;
    }

    FT02_SleepDiagnosticsMarkEnter(automatic);
    FT02_SleepTruthSessionEnter(automatic);
    FT02_SleepTruthSetBatteryStart(
        g_ft02PreSleepBatteryValid,
        g_ft02PreSleepBattery.percent,
        g_ft02PreSleepBattery.voltage
    );

    FT02_LOGI("[SLEEP] enter source=%s\n", automatic ? "idle" : "manual");
    // Drain debug output once before sleeping. Never flush on each periodic wake.
    Serial.flush();
}

static void FT02_WakeFromSleep()
{
    if(!FT02_ModeIsSleeping()) return;

    FT02_LOGD_PRINTLN("[WAKE] begin");
    g_ft02SleepNoInitTrace.phase = 80u; // WAKE_FUNCTION_ENTER

    FT02_DisarmKeyboardSclWake();
    g_ft02SleepNoInitTrace.phase = 81u; // EXT1_WAKE_DISABLED

    if(g_ft02PreSleepCpuMhz >= 80u) (void)setCpuFrequencyMhz(g_ft02PreSleepCpuMhz);

    // CardKB2 has released its open-drain SCL wake pulse before restoring its
    // I2C slave. Bring Wire back before compass/battery resume touches the bus.
    const bool keyboardBusWakeOk =
        FT02_InputResumeBusAfterSleep(FT02_KEYBOARD_WAKE_SCL_RELEASE_TIMEOUT_MS);
    g_ft02SleepNoInitTrace.phase = 811u; // SHARED_I2C_RESTORED
    FT02_LOGD_PRINTLN("[WAKE] usb restore begin");
    Serial.end();
    delay(100);
    Serial.begin(115200);
    delay(300);
    FT02_LOGI_PRINTLN("[SLEEP-A4] wake source=CardKB2 SCL/EXT1");
    FT02_LOGD("[SLEEP-A4] shared-i2c restore keyboard=%d\n", keyboardBusWakeOk ? 1 : 0);
    FT02_LOGD_PRINTLN("[WAKE] usb restore done");

    const bool storageWakeOk = FT02_StorageSleepExit();
    FT02_LOGD("[SD-SLEEP] wake restore=%d\n", storageWakeOk ? 1 : 0);

    // Restore radio and compass before the normal application loop resumes.
    // GNSS transport is restored later by FT02_ModeWake() only for Outdoor mode.
    FT02_MeshServiceSleepExit();
    const bool compassWakeOk = FT02_CompassCoreSleepExit();
    FT02_LOGD("[SLEEP-PWR] wake compass=%d mesh_ready=%d\n",
                  compassWakeOk ? 1 : 0, FT02_MeshServiceReady() ? 1 : 0);

    FT02_InputSetLowNoiseMode(false);
    const bool batteryWakeOk = FT02_BatteryMonitorWakeResume();
    const FT02BatterySnapshot batteryWake = FT02_BatterySnapshotCurrent();
    FT02_LOGD(
        "[BAT] wake resume ok=%u valid=%u soc=%.1f voltage=%.3f failures=%u\n",
        batteryWakeOk ? 1u : 0u,
        batteryWake.valid ? 1u : 0u,
        static_cast<double>(batteryWake.percent),
        static_cast<double>(batteryWake.voltage),
        static_cast<unsigned>(batteryWake.consecutiveFailures));

    FT02_SleepTruthSetBatteryWake(
        batteryWake.valid,
        batteryWake.percent,
        batteryWake.voltage
    );
    g_ft02PreSleepBatteryValid = false;
    g_ft02SleepNoInitTrace.phase = 82u; // CPU_INPUT_BATTERY_RESTORED

    FT02_EpdSignalsRestoreForWake();
    digitalWrite(EPD_PWR, HIGH);
    delay(120);
    digitalWrite(EPD_RST, HIGH);
    g_ft02SleepNoInitTrace.phase = 83u; // EPD_POWER_RESTORED

    display.epd2.enableWrite(EPD_SCK, EPD_MOSI);
    const bool epdSpiBeginOk = true;
    g_ft02SleepNoInitTrace.phase = 84u; // EPD_SW_SPI_RESTORED

    FT02_ReinitializeEpdFullMode();
    FT02_EpdInteractiveReset();
    g_ft02SleepNoInitTrace.phase = 85u; // EPD_REINIT_DONE

    FT02_ModeWake();
    g_ft02SleepNoInitTrace.phase = 86u; // MODE_WAKE_DONE

    FT02_SyncGnssStatusBar(false);
    FT02_SyncBatteryStatusBar(false);
    g_ft02SleepNoInitTrace.phase = 87u; // STATUS_SYNC_DONE

    if(FT02_SystemSettingsWakeReturnPolicy() == FT02_WAKE_RETURN_HOME)
        g_ft02PageState = FT02_PAGE_HOME;
    else
        g_ft02PageState = g_ft02SleepReturnPage;

    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST ||
       g_ft02PageState == FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM)
    {
        (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_JOURNAL);
    }
    else if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)
    {
        (void)FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST);
        FT02_SystemSelfTestRun();
    }
    else if(g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)
    {
        // Wi-Fi remains OFF after sleep. Re-open only the UI state; the user
        // explicitly enables/scans from the page when needed.
        FT02_WirelessManagerOpenWiFi();
    }

    FT02_IdleManagerNoteActivity(millis());
    g_ft02WakeReleaseRequired = true;
    g_ft02WakeLastConsumedKeyMs = millis();

    FT02_RedrawCurrentPage();
    g_ft02SleepNoInitTrace.phase = 88u; // PAGE_REDRAW_DONE

    const uint32_t sleepDurationMs = static_cast<uint32_t>(millis() - g_ft02SleepEnteredMs);
    g_ft02SleepNoInitTrace.wakeCompleted = 1u;
    g_ft02SleepNoInitTrace.sessionActive = 0u;
    g_ft02SleepNoInitTrace.phase = 90u; // WAKE_COMPLETE

    FT02_InputSetDiagnosticsEnabled(true);

    FT02_LOGD(
        "[SLEEP-EXIT-RCA] exit=keyboard raw=0x%02lX key=%lu cmd=0x%02lX slept_ms=%lu slices=%lu input_events=%lu invalid=%lu\n",
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedRaw),
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedKey),
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedCommand),
        static_cast<unsigned long>(sleepDurationMs),
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedSlice),
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.inputEventCount),
        static_cast<unsigned long>(g_ft02SleepNoInitTrace.invalidInputCount));
    if(!epdSpiBeginOk)
        FT02_LOGE_PRINTLN("[ERROR][EPD] FAST SW-SPI restore failed during wake");
    Serial.flush();

    g_ft02SleepEnteredMs = 0u;
    g_ft02SleepSliceCount = 0u;
}

static bool FT02_ServiceSleepMode()
{
    if(!FT02_ModeIsSleeping()) return false;

    // Keep the panel physically asleep for the entire low-power interval.
    digitalWrite(EPD_PWR, LOW);
    digitalWrite(EPD_RST, LOW);
    FT02_InputSetLowNoiseMode(true);

    g_ft02SleepNoInitTrace.phase = 60u; // BEFORE_GPIO_SLEEP_WAIT
    const bool keyboardSignal = FT02_SleepWaitForKeyboardSignal();
    const uint32_t now = millis();

    if(keyboardSignal)
    {
        // SCL is an explicit CardKB2 host-wake pulse. The first physical key is
        // already queued inside CardKB2; normal post-wake input suppression
        // consumes it as the wake gesture instead of executing a UI action.
        g_ft02SleepNoInitTrace.exitReason = 1u;
        g_ft02SleepNoInitTrace.acceptedRaw = 0u;
        g_ft02SleepNoInitTrace.acceptedKey = static_cast<uint32_t>(FT02_KEY_NONE);
        g_ft02SleepNoInitTrace.acceptedCommand = 0u;
        g_ft02SleepNoInitTrace.acceptedSlice = g_ft02SleepSliceCount;
        g_ft02SleepNoInitTrace.phase = 70u; // CARDKB_SCL_WAKE_ACCEPTED

        const uint32_t acceptedSleepDurationMs =
            static_cast<uint32_t>(now - g_ft02SleepEnteredMs);
        FT02_SleepTruthMarkKeyboardWake();
        FT02_SleepDiagnosticsMarkKeyboardWake(
            0u,
            g_ft02SleepSliceCount,
            acceptedSleepDurationMs
        );
        g_ft02SleepNoInitTrace.phase = 71u; // NVS_WAKE_MARKED
        FT02_WakeFromSleep();
        return true;
    }

    // Fail safe: an unexpected Light-Sleep return or wake-config failure must
    // restore the terminal rather than leaving the shared I2C bus suspended.
    g_ft02SleepNoInitTrace.exitReason = 2u;
    g_ft02SleepNoInitTrace.phase = 72u; // UNEXPECTED_SLEEP_RETURN
    FT02_SleepTruthMarkUnexpectedReturn();
    FT02_WakeFromSleep();
    return true;
}

static bool FT02_ServiceHomeModeShortcut(const FT02InputEvent& event)
{
    if(g_ft02PageState != FT02_PAGE_HOME)
    {
        g_ft02HomePPressActive = false;
        g_ft02HomePLongTriggered = false;
        g_ft02HomePEventCount = 0u;
        g_ft02HomePLastEventMs = 0u;
        return false;
    }

    const uint32_t now = millis();
    const bool pEvent =
        event.key == FT02_KEY_CHAR &&
        (event.raw == 'p' || event.raw == 'P');

    if(pEvent)
    {
        if(!g_ft02HomePPressActive)
        {
            g_ft02HomePPressActive = true;
            g_ft02HomePLongTriggered = false;
            g_ft02HomePPressStartMs = now;
            g_ft02HomePEventCount = 1u;
        }
        else if(g_ft02HomePEventCount < 0xFFu)
        {
            ++g_ft02HomePEventCount;
        }
        g_ft02HomePLastEventMs = now;
        FT02_IdleManagerNoteActivity(now);

        // A long press is recognized from the repeated P event stream instead
        // of using zero samples as a physical release indication.
        if(!g_ft02HomePLongTriggered &&
           g_ft02HomePEventCount >= FT02_HOME_P_LONG_MIN_EVENTS &&
           static_cast<uint32_t>(now - g_ft02HomePPressStartMs) >= FT02_HOME_P_LONG_PRESS_MS)
        {
            g_ft02HomePLongTriggered = true;
            FT02_EnterSleepMode(false);
        }
        return true;
    }

    if(!g_ft02HomePPressActive) return false;

    // A different key resolves the pending P as a short press, then the new
    // key is allowed to continue through the normal page dispatcher.
    if(event.key != FT02_KEY_NONE)
    {
        if(!g_ft02HomePLongTriggered)
        {
            FT02_ModeToggleIndoorOutdoor();
            FT02_IdleManagerNoteActivity(now);
            FT02_SyncGnssStatusBar(false);
            FT02_DrawHomeScreen(display, g_ft02HomeSelectedCard);
        }
        g_ft02HomePPressActive = false;
        g_ft02HomePLongTriggered = false;
        g_ft02HomePEventCount = 0u;
        g_ft02HomePLastEventMs = 0u;
        return false;
    }

    // A short press is confirmed only after no more P events arrive for a
    // window longer than CardKB2's repeat cadence.
    if(!g_ft02HomePLongTriggered &&
       static_cast<uint32_t>(now - g_ft02HomePLastEventMs) >= FT02_HOME_P_SHORT_SETTLE_MS)
    {
        FT02_ModeToggleIndoorOutdoor();
        FT02_IdleManagerNoteActivity(now);
        FT02_SyncGnssStatusBar(false);
        FT02_DrawHomeScreen(display, g_ft02HomeSelectedCard);
        g_ft02HomePPressActive = false;
        g_ft02HomePEventCount = 0u;
        g_ft02HomePLastEventMs = 0u;
        return true;
    }

    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    FT02_LOGI("[FT02] boot %s\n", FT02_FIRMWARE_BUILD_LABEL);
    const int resetHigh = (int)esp_reset_reason();
    const unsigned resetRom0 = (unsigned)esp_rom_get_reset_reason(0);
    const unsigned resetRom1 = (unsigned)esp_rom_get_reset_reason(1);
    FT02_LOGD("[FT02] reset reason=%d\n", resetHigh);
    FT02_LOGD("[RESET-TRACE] esp=%d rom0=0x%02X rom1=0x%02X\n",
                  resetHigh, resetRom0, resetRom1);

    const bool noInitValid =
        g_ft02SleepNoInitTrace.magic == FT02_SLEEP_NOINIT_MAGIC &&
        g_ft02SleepNoInitTrace.version == FT02_SLEEP_NOINIT_VERSION;
    if(noInitValid)
    {
        FT02_LOGD(
            "[SLEEP-RCA-BOOT] active=%lu phase=%lu exit=%lu sleep=%lu wake=%lu armed=%lu events=%lu invalid=%lu "
            "last_raw=0x%02lX last_key=%lu last_cmd=0x%02lX accepted_raw=0x%02lX accepted_key=%lu accepted_cmd=0x%02lX "
            "accepted_slice=%lu completed=%lu err=%ld cause=%ld\n",
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.sessionActive),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.phase),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.exitReason),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.sleepCount),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.wakeCount),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.wakeArmed),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.inputEventCount),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.invalidInputCount),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.lastInputRaw),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.lastInputKey),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.lastInputCommand),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedRaw),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedKey),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedCommand),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.acceptedSlice),
            static_cast<unsigned long>(g_ft02SleepNoInitTrace.wakeCompleted),
            static_cast<long>(g_ft02SleepNoInitTrace.lastSleepErr),
            static_cast<long>(g_ft02SleepNoInitTrace.lastWakeCause));
    }
    else
    {
        memset(&g_ft02SleepNoInitTrace, 0, sizeof(g_ft02SleepNoInitTrace));
        g_ft02SleepNoInitTrace.magic = FT02_SLEEP_NOINIT_MAGIC;
        g_ft02SleepNoInitTrace.version = FT02_SLEEP_NOINIT_VERSION;
        FT02_LOGD_PRINTLN("[SLEEP-RCA-BOOT] initialized");
    }

    FT02_InputBegin(
        CARDKB_SDA,
        CARDKB_SCL,
        CARDKB_ADDR
    );
    FT02_EpdInteractiveReset();

    FT02_BatteryMonitorBegin();
    FT02_SyncBatteryStatusBar(false);

    FT02_GnssBegin();
    FT02_CompassCoreBegin();
    FT02_LoRaHwProbeBegin(); // AW9523 + SX1262: read-only RF hardware probe
    FT02_MeshServiceBegin(); // A2: RadioLib SX1262 backend owns the active RX path
    FT02_SystemSettingsBegin();
    FT02_GnssPowerControlBegin();
    FT02_SleepDiagnosticsBegin();
    FT02_ModeManagerBegin();
    FT02_SyncGnssStatusBar(false);

    FT02_StorageBegin();
    FT02_RefreshStorageStatusCache();
    FT02_GnssFieldTestBegin();
    FT02_PinyinLearningBegin();
    FT02_LoRaMessageDeliveryBegin();
    FT02_WirelessManagerBegin();
    FT02_LocationRecorderBegin();
    FT02_LocationLogBegin();
    FT02_AudioLogBegin();
    g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
    FT02_FieldManualBegin(true);

    pinMode(EPD_CS, OUTPUT);
    pinMode(EPD_RST, OUTPUT);
    pinMode(EPD_DC, OUTPUT);
    pinMode(EPD_BUSY, INPUT);
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH);
    digitalWrite(EPD_RST, HIGH);
    digitalWrite(EPD_DC, LOW);
    digitalWrite(EPD_CS, HIGH);
    delay(50);

    display.epd2.enableWrite(EPD_SCK, EPD_MOSI);
    FT02_LOGD("[EPD] backend=FAST-GPIO-SW-SPI sck=%d mosi=%d cs=%d hspi=RADIO\n", EPD_SCK, EPD_MOSI, EPD_CS);
    display.init(FT02_EPD_SERIAL_DIAG_BAUD, true, 10, false);
    display.setRotation(2);

    g_ft02BootDateTime = FT02_ReadBuildDateTime();
    g_ft02BootMillis = millis();
    FT02_IdleManagerBegin(g_ft02BootMillis);
    FT02_UpdateClockCacheOnly();

    FT02_DrawHomeScreen(
        display,
        g_ft02HomeSelectedCard
    );

    FT02_LOGI_PRINTLN("[FT02] ready");

    Serial.flush();
}

void loop()
{
    if(FT02_ServiceSleepMode()) return;

    // Reduce keyboard polling during capture to limit mic-side EMI.
    FT02_InputSetLowNoiseMode(FT02_AudioLogIsCapturing());

    // Service queued input before live polling to preserve key order across EPD commits.
    bool queuedInputHandledByKnowledge = false;
    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE &&
       FT02_EpdInteractiveQueuedInputCount() > 0)
    {
        const FT02KnowledgeAction queuedKnowledgeAction =
            FT02_KnowledgeProcessQueuedInput(display, &queuedInputHandledByKnowledge);
        if(queuedInputHandledByKnowledge) FT02_IdleManagerNoteActivity(millis());
        if(queuedKnowledgeAction == FT02_KNOWLEDGE_ACTION_EXIT_HOME)
        {
            FT02_ReturnHomePage();
        }
    }

    FT02InputEvent inputEvent = {};
    if(!queuedInputHandledByKnowledge)
    {
        if(!FT02_EpdInteractiveTakeQueuedInput(inputEvent))
        {
            inputEvent = FT02_InputPoll();
        }
    }
    const uint8_t currentRawKey = FT02_InputCurrentRawKey();
    if(currentRawKey == 0)
    {
        g_ft02AudioCommandReleaseRequired = false;
        FT02_AudioLogNotifyKeyRelease();
        if(g_ft02MapRequireNavRelease)
        {
            const uint32_t now = millis();
            if(g_ft02MapReleaseZeroSinceMs == 0)
            {
                g_ft02MapReleaseZeroSinceMs = now;
            }
            else if(now - g_ft02MapReleaseZeroSinceMs >= FT02_MAP_NAV_RELEASE_STABLE_MS)
            {
                g_ft02MapRequireNavRelease = false;
                g_ft02MapReleaseZeroSinceMs = 0;

            }
        }
    }
    else if(g_ft02MapRequireNavRelease)
    {
        // Any non-zero sample breaks the release qualification. This prevents
        // a transient zero between CardKB2 repeat/combination events from
        // re-arming the map too early.
        g_ft02MapReleaseZeroSinceMs = 0;
    }

    if(g_ft02WakeReleaseRequired)
    {
        // CardKB2 can emit zero samples between repeat events. Consume the wake
        // key stream until it has been quiet long enough to count as released.
        const uint32_t wakeNow = millis();
        if(inputEvent.key != FT02_KEY_NONE || FT02_InputCurrentRawKey() != 0)
            g_ft02WakeLastConsumedKeyMs = wakeNow;

        if(FT02_InputCurrentRawKey() == 0 &&
           static_cast<uint32_t>(wakeNow - g_ft02WakeLastConsumedKeyMs) >= FT02_WAKE_CONSUME_QUIET_MS)
            g_ft02WakeReleaseRequired = false;

        inputEvent.key = FT02_KEY_NONE;
        inputEvent.raw = 0;
        inputEvent.command = 0;
    }

    const bool modeShortcutConsumed =
        FT02_ServiceHomeModeShortcut(inputEvent);

    if(!modeShortcutConsumed && inputEvent.key != FT02_KEY_NONE)
    {
        FT02_IdleManagerNoteActivity(millis());
        FT02_HandleInput(inputEvent);
    }

    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        const FT02KnowledgeAction deferredKnowledgeAction =
            FT02_KnowledgeService(display, millis());
        if(deferredKnowledgeAction == FT02_KNOWLEDGE_ACTION_EXIT_HOME)
        {
            FT02_ReturnHomePage();
        }
    }

    // Global runtime owns the e-paper hot/power-off lifecycle for every
    // interactive page, including FieldManual.
    FT02_EpdInteractiveService(display, millis());

    if(g_ft02PageState == FT02_PAGE_MAP_SEARCH && FT02_MapSearchTakeDeferredRedraw(millis()))
    {
        FT02_DrawMapSearchInputPartial(display);
    }

    FT02_LoRaHwProbePoll();
    FT02_MeshServicePoll();
    FT02_CompassCorePoll();
    FT02_GnssPowerControlPoll();
    if(g_ft02PageState == FT02_PAGE_COMPASS)
    {
        FT02_CompassUIService(display, millis());
    }
    if(FT02_WirelessManagerPoll(millis()) && g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)
    {
        FT02_RedrawCurrentPage();
    }

    if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        const uint32_t calRevision = FT02_CompassCalibrationUIRevision();
        const uint32_t nowCal = millis();
        // Compass sampling runs in the main loop at 100 ms. A full e-paper
        // redraw takes well over a second, so measuring the refresh interval
        // from before the redraw can starve calibration sampling: by the time
        // the redraw finishes the old 1.5 s interval has already expired and
        // the next loop immediately redraws again. Keep the proven
        // calibration thresholds, but guarantee a real sampling window after
        // each display commit.
        if(calRevision != g_ft02LastCompassCalRevision &&
           static_cast<uint32_t>(nowCal - g_ft02LastCompassCalRedrawMs) >= 3000u)
        {
            g_ft02LastCompassCalRevision = calRevision;
            FT02_RedrawCurrentPage();
            // Start the throttle after the blocking e-paper refresh completes.
            g_ft02LastCompassCalRedrawMs = millis();
        }
    }

    FT02_GnssPoll();
    FT02_GnssFieldTestPoll();
    FT02_LocationRecorderPoll();
    const bool locationLogLoadCompleted = FT02_LocationLogPollReload();
    FT02_AudioLogPoll();

    if(locationLogLoadCompleted && g_ft02PageState == FT02_PAGE_LOCATION_LOG_LIST)
    {
        FT02_ClampLocationLogSelection();
        g_ft02LocationLogLastGeneration = FT02_LocationLogStatusCurrent().generation;
        FT02_DrawLocationLogListBodyPartial(display, g_ft02LocationLogSelectedIndex);
    }

    const bool audioBusy = FT02_AudioLogIsBusy();
    const bool audioCapturing = FT02_AudioLogIsCapturing();
    if(!audioBusy)
    {
        // IME learning persistence shares the SD backend with audio logging.
        // Never start a background learning snapshot while audio I/O is busy.
        FT02_PinyinLearningPoll();
        FT02_LoRaMessageDeliveryPoll();

        const bool loraLink = FT02_MeshServiceReady();
        const bool loraReady = FT02_LoRaNodeRuntimeReady();
        const uint32_t commRevision = FT02_LoRaCommunicationRevision();
        const uint16_t loraUnread = FT02_LoRaCommunicationUnreadCount();
        bool communicationPageRedrawn = false;

        if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
           !loraReady && g_ft02CommunicationReadyPresented)
        {
            // MeshService reported a readiness transition. Present it once.
            g_ft02CommunicationReadyPresented = false;
            FT02_CommunicationUIOnSyncStarted("Mesh 正在重新同步状态...");
            g_ft02LastLoRaLinkState = loraLink;
            g_ft02LastLoRaReadyState = loraReady;
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            if(!FT02_CommunicationUIIsCompose())
            {
                FT02_RedrawCurrentPage();
                communicationPageRedrawn = true;
            }
        }
        else if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
                loraReady && !g_ft02CommunicationReadyPresented)
        {
            // Redraw after MeshService reports radio readiness and clear the transient sync notice.
            g_ft02CommunicationReadyPresented = true;
            FT02_CommunicationUIOnSyncReady();
            g_ft02LastLoRaLinkState = loraLink;
            g_ft02LastLoRaReadyState = loraReady;
            if(FT02_CommunicationUIIsInbox()) FT02_LoRaCommunicationMarkAllRead();
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            FT02_RedrawCurrentPage();
            communicationPageRedrawn = true;
        }
        else if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
                FT02_CommunicationUIIsNodes() &&
                FT02_LoRaNodeRuntimeRevision() != g_ft02LastLoRaNodeRevision)
        {
            // MeshService learns names and public keys asynchronously from NODEINFO_APP.
            g_ft02LastLoRaNodeRevision = FT02_LoRaNodeRuntimeRevision();
            FT02_CommunicationUIOnNodeListUpdated(static_cast<uint16_t>(FT02_LoRaNodeRuntimeNodeCount()));
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            FT02_RedrawCurrentPage();
            communicationPageRedrawn = true;
        }
        else if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
                !FT02_CommunicationUIIsCompose() &&
                commRevision != g_ft02LastCommunicationRevision)
        {
            // Background receive is always live. If the user is already in the
            // inbox, rendering the new message also counts as reading it.
            if(FT02_CommunicationUIIsInbox()) FT02_LoRaCommunicationMarkAllRead();
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            FT02_RedrawCurrentPage();
            communicationPageRedrawn = true;
        }
        else if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
                FT02_CommunicationUITakeDeferredRedraw(millis()))
        {
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            FT02_DrawCommunicationComposePartial(display);
            communicationPageRedrawn = true;
        }

        if(!communicationPageRedrawn && !FT02_IsMapStatusPartialSuppressed() &&
           g_ft02PageState != FT02_PAGE_LIBRARY &&
           (loraLink != g_ft02LastLoRaLinkState ||
            loraReady != g_ft02LastLoRaReadyState ||
            loraUnread != g_ft02LastLoRaUnread))
        {
            g_ft02LastLoRaLinkState = loraLink;
            g_ft02LastLoRaReadyState = loraReady;
            g_ft02LastLoRaUnread = loraUnread;
            FT02_DrawStatusBarLoRa(
                display,
                loraReady ? "已连接" : (loraLink ? "同步中" : "连接中")
            );
        }

        FT02_SyncGnssStatusBar(true);
        FT02_UpdateMapGnssFollow();
    }

    if(!audioBusy && g_ft02PageState == FT02_PAGE_LOCATION_RECORDER)
    {
        const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
        const FT02LocationRecorderSnapshot recorder = FT02_LocationRecorderSnapshotCurrent();
        const FT02GnssFieldTestSnapshot fieldTest = FT02_GnssFieldTestSnapshotCurrent();
        const uint32_t now = millis();

        // GNSS can change several times per second, while this device uses an
        // e-paper panel. Treat the recorder screen as a low-rate dashboard:
        // background data remains live, but automatic presentation refreshes
        // are capped at 30 seconds. Button actions still redraw immediately.
        const bool changed =
            gnss.uiGeneration != g_ft02RecorderLastGnssGeneration ||
            recorder.uiGeneration != g_ft02RecorderLastUiGeneration ||
            fieldTest.uiGeneration != g_ft02GnssFieldTestLastGeneration;
        const bool periodicRecorderTick = recorder.sessionActive || fieldTest.active;

        if((changed || periodicRecorderTick) &&
           now - g_ft02RecorderLastScreenRefreshMs >= FT02_RECORDER_UI_REFRESH_MS)
        {
            g_ft02RecorderLastGnssGeneration = gnss.uiGeneration;
            g_ft02RecorderLastUiGeneration = recorder.uiGeneration;
            g_ft02GnssFieldTestLastGeneration = fieldTest.uiGeneration;
            FT02_DrawLocationRecorderMiddlePartial(display, gnss, recorder);
            // Measure the next interval from the end of the blocking display
            // commit, not from its beginning. This guarantees idle time for
            // input and background services between e-paper refreshes.
            g_ft02RecorderLastScreenRefreshMs = millis();
        }
    }

    // Real elapsed clock:
    // refreshes only when the displayed minute changes.
    // The date is refreshed in the same local window and rolls over at midnight.
    const FT02AudioLogStatus audioStatus = FT02_AudioLogStatusCurrent();
    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST)
    {
        const FT02AudioLogState audioState = FT02_AudioLogStateCurrent();
        if(audioState == FT02_AUDIO_LOG_RECORDING || audioState == FT02_AUDIO_LOG_POST_ROLL)
        {
            // Recording duration remains visible. Only the small timer window
            // is refreshed every 5 s; the audio task continues independently.
            // speaker playback remains logically gated throughout recording, so
            // the display update no longer shares an active class-D load.
            const uint32_t bucket = audioStatus.recordingElapsedMs /
                FT02_AUDIO_TIMER_REFRESH_MS;
            if(bucket != g_ft02AudioLastShownRecordBucket)
            {
                g_ft02AudioLastShownRecordBucket = bucket;

                FT02_DrawAudioLogRecordingTimerPartial(display, audioStatus);

            }
        }
        else if(audioState == FT02_AUDIO_LOG_PLAYING)
        {
            // Playback time stays static. The only in-play display update is
            // an explicit user volume change. Rapid
            // taps are coalesced, then the audio task freezes at zero and
            // shuts PA down before the tiny volume-line partial is committed.
            if(g_ft02AudioVolumeRefreshPending &&
               static_cast<int32_t>(millis() - g_ft02AudioVolumeRefreshDueMs) >= 0)
            {
                const FT02AudioLogStatus latest = FT02_AudioLogStatusCurrent();
                if(latest.playbackVolumeLevel == g_ft02AudioLastShownVolume)
                {
                    g_ft02AudioVolumeRefreshPending = false;
                }
                else if(FT02_AudioLogSetPlaybackEpdGuard(true))
                {

                    FT02_DrawAudioLogPlayingVolumePartial(display, latest);
                    g_ft02AudioLastShownVolume = latest.playbackVolumeLevel;
                    g_ft02AudioVolumeRefreshPending = false;
                    const bool resumed = FT02_AudioLogSetPlaybackEpdGuard(false);

                }
                else
                {
                    // Playback may be entering its pop-safe drain. Do not
                    // energize the EPD in that transition; the list redraw
                    // after playback shutdown will show the persisted final volume.
                    g_ft02AudioVolumeRefreshPending = false;

                }
            }
        }
    }
    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST && !FT02_AudioLogIsPlaying())
    {
        g_ft02AudioVolumeRefreshPending = false;
        g_ft02AudioVolumeRefreshDueMs = 0;
    }
    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST &&
       !FT02_AudioLogIsBusy() &&
       audioStatus.generation != g_ft02AudioLogLastGeneration)
    {
        g_ft02AudioLogLastGeneration = audioStatus.generation;
        g_ft02AudioLogSelectedIndex = 0;
        FT02_ClampAudioLogSelection();
        FT02_DrawAudioLogListBodyPartial(display, g_ft02AudioLogSelectedIndex);
    }

    // Map pan/zoom input only updates the desired view. Multiple inputs inside
    // the settle window are coalesced; the scheduler owns the single final
    // map build plus display refresh.
    FT02_ProcessPendingMapRefresh();

    // MAX17048 shares the CardKB2/ES8311 I2C bus. Measurements stay out of
    // active audio work; the status bar refreshes only when visible text changes.
    if(!FT02_AudioLogIsBusy())
    {
        if(FT02_BatteryMonitorPoll())
            FT02_SyncBatteryStatusBar(true);
    }

    if(!FT02_AudioLogIsBusy())
    {
        FT02_UpdateClockIfNeeded(false);
    }

    if(FT02_ModeAutoSleepAllowed() &&
       FT02_IdleManagerShouldSleep(millis()) &&
       !FT02_SleepBlocked(true))
    {
        FT02_EnterSleepMode(true);
        return;
    }

    // Low Power A2: keep the normal UI scheduler at 50 Hz. CardKB2 firmware
    // queues key bytes internally, so ordinary FT-02 pages do not need to poll
    // the shared I2C bus every 5 ms. Playback retains the proven 5 ms cadence
    // because its guard/volume path is time-sensitive and audio power is already
    // intentionally active in that state.
    const uint32_t loopDelayMs = FT02_AudioLogIsPlaying() ? 5u : 20u;
    delay(loopDelayMs);
}
