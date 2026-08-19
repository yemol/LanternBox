#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>

#include "FT02_HomeUI.h"
#include "FT02_BuildInfo.h"
#include "FT02_PbfMapUI.h"
#include "FT02_PbfMapRuntime.h"
#include "FT02_HelpUI.h"
#include "FT02_KnowledgeUI.h"
#include "FT02_FieldManual.h"
#include "FT02_Gnss.h"
#include "FT02_GnssFieldTest.h"
#include "FT02_LocationRecorder.h"
#include "FT02_LocationRecorderUI.h"
#include "FT02_LocationLog.h"
#include "FT02_LocationLogUI.h"
#include "FT02_AudioLog.h"
#include "FT02_AudioLogUI.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_FontPackRenderer.h"
#include "FT02_PageState.h"
#include "FT02_HomeCards.h"
#include "FT02_InputManager.h"
#include "FT02_StatusBar.h"
#include "FT02_Storage.h"
#include "FT02_GrayMapUI.h"
#include "FT02_MapSearch.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LR01HostRuntime.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_CommunicationNodeUI.h"
#include "FT02_CompassCalibrationUI.h"
#include "FT02_SystemSelfTest.h"
#include "FT02_PinyinLearning.h"
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
constexpr uint32_t EPD_SPI_HZ = 4000000UL;

SPIClass g_ft02EpdSpi(HSPI);

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
static bool g_ft02MapGrayActive = false;
static bool g_ft02RuntimeBannerPrinted = false;
static bool g_ft02CommunicationReadyPresented = false;
static bool g_ft02LastLoRaLinkState = false;
static bool g_ft02LastLoRaReadyState = false;
static uint32_t g_ft02LastCommunicationRevision = 0;
static uint32_t g_ft02LastCompassCalRevision = 0;
static uint32_t g_ft02LastCompassCalRedrawMs = 0;
static uint32_t g_ft02LastLR01NodeListMs = 0;
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
static uint32_t g_ft02AudioLastShownRecordBucket = 0xFFFFFFFFu;
static uint32_t g_ft02AudioLastShownPlayBucket = 0xFFFFFFFFu;
static uint8_t g_ft02AudioLastShownVolume = 0xFFu;
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
static void FT02_ReinitializeEpdFullMode(const char* source);
static void FT02_DrawKnowledgeAfterCleanTransition(const char* source);
static void FT02_SyncGnssStatusBar(bool allowPartialRefresh);
static void FT02_UpdateMapGnssFollow();
static void FT02_RunDirectPbfMap(bool resetDefault, bool showLoading);
static bool FT02_CommitGrayMap(const char* source);
static void FT02_EnsureBwAfterGrayMap(const char* source);
static void FT02_ScheduleMapZoomRefresh(int originalZoom);
static void FT02_ScheduleMapPanRefresh();
static void FT02_ProcessPendingMapRefresh();
static void FT02_CancelPendingMapRefresh(const char* source);
static void FT02_ArmMapNavReleaseGate(const char* source);
static void FT02_OpenAudioLogListPage(bool reload);
static void FT02_ClampAudioLogSelection();
static bool FT02_ArmAudioCommandAfterRelease();
static void FT02_OpenDeviceStatusPage();
static void FT02_OpenMapSearchPage();

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

static bool FT02_IsNativeGrayPage()
{
    // The production map owns the SSD1677 four-gray LUT and both RAM planes.
    // Never mix one-bit partial updates into it.
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

    Serial.print("Clock cache: ");
    Serial.print(hhmm);
    Serial.print(" ");
    Serial.println(mmdd);
}

static void FT02_UpdateClockIfNeeded(bool force)
{
    FT02DateTime now = FT02_CurrentDateTime();
    int minuteKey = FT02_DateTimeMinuteKey(now);

    if(!force && minuteKey == g_ft02LastShownMinute)
    {
        return;
    }

    // The production four-gray map owns the SSD1677 LUT and both RAM
    // bit planes. Never let the normal GxEPD2 one-bit status-bar path write
    // into the controller until the page has been exited and reinitialized.
    if(FT02_IsNativeGrayPage())
    {
        FT02_UpdateClockCacheOnly();
        Serial.println("[GRAY4] clock cache updated; one-bit partial refresh suppressed");
        return;
    }

    // Knowledge pages are deliberately kept free of status-bar partial
    // refreshes. A partial update before the next full knowledge frame can
    // leave the SSD1677/GDEQ0426T82 controller in a state that makes fixed
    // horizontal regions appear pale. Cache the new time only; the next
    // knowledge full-frame commit will draw it together with the page.
    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        FT02_UpdateClockCacheOnly();
        Serial.println("[EPD] static-page clock cache updated; partial refresh suppressed");
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

    Serial.print("Clock update: ");
    Serial.print(hhmm);
    Serial.print(" ");
    Serial.println(mmdd);

    FT02_DrawStatusBarClock(
        display,
        hhmm,
        mmdd
    );
}



static void FT02_SyncGnssStatusBar(bool allowPartialRefresh)
{
    const FT02GnssSnapshot snapshot = FT02_GnssSnapshotCurrent();

    const char* line1 = "GPS";
    const char* line2 = "连接中";

    if(snapshot.communicationActive)
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
    else if(snapshot.startAgeMs >= 3000)
    {
        line2 = FT02_LR01HostOnline() ? "GNSS离线" : "LR01离线";
    }

    const bool changed =
        strcmp(g_ft02LastGnssStatusLine1, line1) != 0 ||
        strcmp(g_ft02LastGnssStatusLine2, line2) != 0;

    FT02_SetStatusBarGnssCache(line1, line2);

    if(!changed) return;

    snprintf(g_ft02LastGnssStatusLine1, sizeof(g_ft02LastGnssStatusLine1), "%s", line1);
    snprintf(g_ft02LastGnssStatusLine2, sizeof(g_ft02LastGnssStatusLine2), "%s", line2);

    Serial.print("GNSS status cache: ");
    Serial.print(line1);
    Serial.print(" / ");
    Serial.println(line2);

    // Knowledge pages must not receive status-bar partial updates because that
    // can reintroduce the pale-band controller state. The GNSS page already
    // performs a full page refresh for meaningful GNSS state changes.
    if(allowPartialRefresh &&
       g_ft02PageState != FT02_PAGE_KNOWLEDGE &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_LIST &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_DETAIL &&
       g_ft02PageState != FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM &&
       g_ft02PageState != FT02_PAGE_AUDIO_LOG_LIST &&
       g_ft02PageState != FT02_PAGE_AUDIO_LOG_DELETE_CONFIRM &&
       !FT02_IsNativeGrayPage())
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
    Serial.print("Storage status cache: ");
    Serial.print(line1);
    Serial.print(" / ");
    Serial.println(line2);
}

static void FT02_PrintRuntimeBannerIfNeeded()
{
    if(g_ft02RuntimeBannerPrinted)
    {
        return;
    }

    if(millis() < 5000)
    {
        return;
    }

    g_ft02RuntimeBannerPrinted = true;

    Serial.print("FT-02 runtime alive: ");
    Serial.print(FT02_FIRMWARE_BUILD_LABEL);
    Serial.print(" + Location Recorder A3 + FieldManualRuntime + PBF Map UI A3.14 SPI40, ");
    Serial.print(FT02_StorageProfileText());
    Serial.println(", LR01 single-authority navigation/radio, Help restored, CardKB2 SDA=47 SCL=21");
    Serial.flush();
}

static void FT02_ReinitializeEpdFullMode(const char* source)
{
    Serial.print("[EPD] clean full-mode reinit begin source=");
    Serial.println(source != nullptr ? source : "unknown");

    // End any previous display lifecycle, then hard-cycle only the e-paper
    // power rail. This is the sequence proven by v2.44 to clear the stale
    // partial-update/LUT state that caused one horizontal Card band to fade.
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

    display.init(
        115200,
        true,
        10,
        false,
        g_ft02EpdSpi,
        SPISettings(EPD_SPI_HZ, MSBFIRST, SPI_MODE0)
    );
    display.setRotation(2);

    Serial.println("[EPD] clean full-mode reinit complete");
}

static void FT02_PrepareCleanGrayTransition(const char* source)
{
    Serial.print("[GRAY4] clean transition begin source=");
    Serial.println(source != nullptr ? source : "unknown");

    // A raw four-gray refresh does not inherit GxEPD2's previous/current RAM
    // history. Restore the proven full-refresh driver first and drive the
    // entire panel to white before loading the grayscale LUT. This prevents
    // the previous black-white page from remaining as an inverted ghost.
    FT02_ReinitializeEpdFullMode("gray4-preclear-reinit");
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
    } while(display.nextPage());
    display.epd2.powerOff();
    delay(40);

    Serial.println("[GRAY4] clean transition white frame complete");
}

static bool FT02_CommitGrayMap(const char* source)
{
    const bool continuingGraySession = g_ft02MapGrayActive;

    Serial.print("[GRAY4-MAP] production commit begin source=");
    Serial.print(source != nullptr ? source : "unknown");
    Serial.print(" transition=");
    Serial.println(continuingGraySession ? "gray-to-gray-single" : "bw-to-gray-clean");

    // Entering the production map from a normal one-bit page still needs one
    // clean white transition, otherwise the old status bar can remain in the
    // first four-gray frame. Once a valid four-gray map is already displayed,
    // do not submit another visible white frame for pan/zoom/follow updates.
    // The raw driver writes the complete 800x480 two-plane frame, so a direct
    // gray-to-gray full update is the single visible transaction we need.
    if(!continuingGraySession)
    {
        FT02_PrepareCleanGrayTransition(source);
    }
    else
    {
        Serial.println("[GRAY4-MAP] retained gray session; white preclear skipped");
    }

    // Block all one-bit status writes while the panel is being re-powered and
    // the complete four-gray frame is committed.
    g_ft02MapGrayActive = false;
    digitalWrite(EPD_PWR, LOW);
    delay(160);
    digitalWrite(EPD_PWR, HIGH);
    delay(160);

    const FT02GrayMapReport report = FT02_DrawGrayMapScreen(
        g_ft02EpdSpi,
        EPD_SPI_HZ,
        EPD_PWR,
        EPD_BUSY,
        EPD_RST,
        EPD_DC,
        EPD_CS
    );

    g_ft02MapGrayActive = report.success;
    Serial.printf(
        "[GRAY4-MAP] production commit success=%s init=%s refresh=%s sleep=%s transaction=%s aa_font=yes buildings=outline-only render=%lums total=%lums message=%s\n",
        report.success ? "yes" : "no",
        report.panelInitialized ? "yes" : "no",
        report.refreshCompleted ? "yes" : "no",
        report.panelQuiesced ? "yes" : "no",
        continuingGraySession ? "single-gray" : "clean-plus-gray",
        static_cast<unsigned long>(report.renderMs),
        static_cast<unsigned long>(report.elapsedMs),
        report.message != nullptr ? report.message : "--"
    );
    return report.success;
}

static void FT02_EnsureBwAfterGrayMap(const char* source)
{
    if(!g_ft02MapGrayActive) return;
    Serial.print("[GRAY4-MAP] leave production gray mode source=");
    Serial.println(source != nullptr ? source : "unknown");
    g_ft02MapGrayActive = false;
    FT02_ReinitializeEpdFullMode(source);
}

static void FT02_DrawKnowledgeAfterCleanTransition(const char* source)
{
    // v2.48: keep the controller power-cycle/full-mode reinitialization that
    // eliminated the pale-band issue, but do not submit a separate visible
    // white frame. The final knowledge page is the first and only refresh.
    FT02_ReinitializeEpdFullMode(source);
    FT02_UpdateClockCacheOnly();
    FT02_SyncGnssStatusBar(false);
    Serial.println("[EPD] single-refresh entry: drawing final knowledge frame");
    FT02_DrawKnowledgeScreen(display);
}

static void FT02_RedrawCurrentPage()
{
    // Map UX A4: while a pan/zoom target is still settling, the scheduler is
    // the sole owner of the next map-page commit.  Any unrelated redraw
    // request is intentionally ignored here so the old map cannot be
    // refreshed once immediately before the final target frame.  The
    // scheduler clears g_ft02MapRefreshPending before calling the normal
    // build/redraw path, so the final target still commits exactly once.
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
        const FT02PbfMapReport& mapReport = FT02_PbfMapReportCurrent();
        if(mapReport.state == FT02_PBF_MAP_READY)
        {
            // Map Navigation A1: production map is now intentionally 1-bit
            // black/white. Road hierarchy is encoded by line width/style and
            // buildings remain outline-only. This avoids the ~6.7 s Gray4
            // commit and keeps navigation compatible with fast partial updates.
            FT02_EnsureBwAfterGrayMap("map-bw-production");
            Serial.println("[MAP-BW-A1] production BW commit; Gray4 disabled for navigation");
            FT02_PbfMapSetFollowGnss(g_ft02MapFollowGnss);
            FT02_DrawPbfMapScreen(display);
        }
        else
        {
            FT02_EnsureBwAfterGrayMap("map-loading-or-error");
            FT02_PbfMapSetFollowGnss(g_ft02MapFollowGnss);
            FT02_DrawPbfMapScreen(display);
        }
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
    else if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        FT02_DrawCompassCalibrationScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_HELP)
    {
        FT02_DrawHelpScreen(display);
    }
    else if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        FT02_DrawKnowledgeScreen(display);
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
        FT02_CancelPendingMapRefresh("map-to-help");
        FT02_EnsureBwAfterGrayMap("map-to-help");
    }
    g_ft02PageState = FT02_PAGE_HELP;

    Serial.print("Page: ");
    Serial.print((int)g_ft02PageBeforeHelp);
    Serial.println(" -> HELP");

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
        FT02_CancelPendingMapRefresh("map-to-home");
        FT02_EnsureBwAfterGrayMap("map-to-home");
        FT02_PbfMapUnload();
    }

    FT02_RefreshStorageStatusCache();
    g_ft02PageState = FT02_PAGE_HOME;
    Serial.println("Page: current -> HOME");

    if(returningFromKnowledge)
    {
        // v2.48: use the same proven clean-controller transition as knowledge
        // entry, but submit the final home page as the first and only frame.
        // This preserves the pale-band fix without a separate white refresh.
        FT02_ReinitializeEpdFullMode("knowledge-exit-home");
        FT02_UpdateClockCacheOnly();
        FT02_SyncGnssStatusBar(false);
        Serial.println("[EPD] single-refresh exit: drawing final home frame");
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

    Serial.print("Page: HELP -> ");
    Serial.println((int)g_ft02PageState);

    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        FT02_DrawKnowledgeAfterCleanTransition("help-return-knowledge");
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

static bool FT02_MapCenterOnCurrentGnss()
{
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(!gnss.fixValid || !gnss.hasPosition) return false;

    FT02_PbfMapSetCenter(gnss.longitude, gnss.latitude);
    g_ft02MapHasTrackedFix = true;
    g_ft02MapLastFixValid = true;
    g_ft02MapLastFollowLat = gnss.latitude;
    g_ft02MapLastFollowLon = gnss.longitude;
    g_ft02MapLastHeadingValid = gnss.courseValid;
    g_ft02MapLastHeadingDeg = gnss.courseDegrees;
    const uint32_t now = millis();
    g_ft02MapLastFollowRefreshMs = now;
    g_ft02MapLastFollowCheckMs = now;
    return true;
}

static const char* FT02_MapRefreshKindText(FT02MapRefreshKind kind)
{
    switch(kind)
    {
        case FT02_MAP_REFRESH_ZOOM: return "zoom";
        case FT02_MAP_REFRESH_PAN: return "pan";
        default: return "none";
    }
}

static void FT02_ArmMapNavReleaseGate(const char* source)
{
    g_ft02MapRequireNavRelease = true;
    g_ft02MapReleaseZeroSinceMs = 0;
    Serial.printf("[MAP-INPUT-A5] release gate armed source=%s stable=%lums\n",
                  source != nullptr ? source : "unknown",
                  static_cast<unsigned long>(FT02_MAP_NAV_RELEASE_STABLE_MS));
}

static void FT02_CancelPendingMapRefresh(const char* source)
{
    if(!g_ft02MapRefreshPending) return;

    Serial.printf(
        "[MAP-UX-A3] pending %s refresh cancelled source=%s\n",
        FT02_MapRefreshKindText(g_ft02MapRefreshKind),
        source != nullptr ? source : "unknown"
    );
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
    Serial.printf(
        "[MAP-UX-A3] zoom refresh queued from=Z%d target=Z%d settle=%lums\n",
        g_ft02MapPendingZoomFrom,
        FT02_PbfMapZoomCurrent(),
        static_cast<unsigned long>(FT02_MAP_ZOOM_SETTLE_MS)
    );
}

static void FT02_ScheduleMapPanRefresh()
{
    g_ft02MapRefreshPending = true;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_PAN;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshDeadlineMs = millis() + FT02_MAP_PAN_SETTLE_MS;
    Serial.printf(
        "[MAP-UX-A3] pan refresh queued zoom=Z%d settle=%lums\n",
        FT02_PbfMapZoomCurrent(),
        static_cast<unsigned long>(FT02_MAP_PAN_SETTLE_MS)
    );
}

static void FT02_ProcessPendingMapRefresh()
{
    if(!g_ft02MapRefreshPending) return;

    if(g_ft02PageState != FT02_PAGE_MAP)
    {
        FT02_CancelPendingMapRefresh("page-changed");
        return;
    }

    const uint32_t now = millis();
    if(static_cast<int32_t>(now - g_ft02MapRefreshDeadlineMs) < 0)
    {
        return;
    }

    const FT02MapRefreshKind commitKind = g_ft02MapRefreshKind;
    const int fromZoom = g_ft02MapPendingZoomFrom;
    const int targetZoom = FT02_PbfMapZoomCurrent();
    g_ft02MapRefreshPending = false;
    g_ft02MapRefreshDeadlineMs = 0;
    g_ft02MapPendingZoomFrom = 0;
    g_ft02MapRefreshKind = FT02_MAP_REFRESH_NONE;

    if(commitKind == FT02_MAP_REFRESH_ZOOM)
    {
        Serial.printf(
            "[MAP-UX-A3] queued zoom commit from=Z%d target=Z%d one-render=yes\n",
            fromZoom,
            targetZoom
        );
    }
    else
    {
        const FT02PbfMapReport& report = FT02_PbfMapReportCurrent();
        Serial.printf(
            "[MAP-UX-A3] queued pan commit center=%.7f,%.7f zoom=Z%d one-render=yes\n",
            report.centerLon,
            report.centerLat,
            targetZoom
        );
    }
    // The following build + gray commit is blocking. Disarm map navigation
    // before entering it so a held key cannot leak through as a new pan batch
    // when the loop resumes. The gate is released only after a zero raw key is
    // observed in loop().
    FT02_ArmMapNavReleaseGate("pan-zoom-commit");
    FT02_PbfMapSetLoadingReason(
        commitKind == FT02_MAP_REFRESH_ZOOM ? FT02_MAP_LOADING_ZOOM : FT02_MAP_LOADING_PAN
    );
    Serial.printf("[MAP-LOAD-A2] operation=%s visible acknowledgement before blocking build\n",
                  commitKind == FT02_MAP_REFRESH_ZOOM ? "zoom" : "pan");
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
        Serial.println("[MAP-LOAD-A1] visible loading acknowledgement commit begin");
        FT02_RedrawCurrentPage();
        Serial.println("[MAP-LOAD-A1] loading acknowledgement visible; blocking map build may begin");
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
        Serial.println("[MAP-NAV-A1] first GNSS fix acquired; centering map");
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

    const bool headingValidityChanged = gnss.courseValid != g_ft02MapLastHeadingValid;
    float headingDelta = 0.0f;
    if(gnss.courseValid && g_ft02MapLastHeadingValid)
    {
        headingDelta = fabsf(gnss.courseDegrees - g_ft02MapLastHeadingDeg);
        if(headingDelta > 180.0f) headingDelta = 360.0f - headingDelta;
    }
    const bool headingChanged = headingValidityChanged ||
        (gnss.courseValid && g_ft02MapLastHeadingValid &&
         headingDelta >= FT02_MAP_HEADING_REFRESH_DEGREES);

    Serial.printf(
        "[MAP-NAV-A2] 10s check moved=%.1fm heading_delta=%.1fdeg heading_valid=%u threshold=%.1fdeg partial_count=%u\n",
        movedMeters,
        static_cast<double>(headingDelta),
        gnss.courseValid ? 1u : 0u,
        static_cast<double>(FT02_MAP_HEADING_REFRESH_DEGREES),
        static_cast<unsigned>(g_ft02MapPartialNavRefreshCount)
    );

    if(movedMeters < FT02_MAP_FOLLOW_DISTANCE_METERS)
    {
        if(!headingChanged)
        {
            return;
        }

        // Position is effectively unchanged, but LR01 compass heading moved enough
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

        g_ft02MapLastHeadingValid = gnss.courseValid;
        g_ft02MapLastHeadingDeg = gnss.courseDegrees;
        g_ft02MapLastFollowRefreshMs = now;
        Serial.printf("[MAP-NAV-A2] heading marker refresh heading=%.1f delta=%.1f\n",
                      static_cast<double>(gnss.courseDegrees),
                      static_cast<double>(headingDelta));
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
        g_ft02MapLastHeadingValid = gnss.courseValid;
        g_ft02MapLastHeadingDeg = gnss.courseDegrees;
        g_ft02MapLastFollowRefreshMs = now;

        if(g_ft02MapPartialNavRefreshCount + 1 >= FT02_MAP_PARTIAL_CLEANUP_COUNT)
        {
            Serial.printf(
                "[MAP-NAV-A1] partial cleanup threshold reached (%u); BW full refresh without PBF rebuild\n",
                static_cast<unsigned>(FT02_MAP_PARTIAL_CLEANUP_COUNT)
            );
            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
            return;
        }

        Serial.printf(
            "[MAP-NAV-A1] marker partial old=%.6f,%.6f new=%.6f,%.6f screen=%d,%d\n",
            oldLat,
            oldLon,
            gnss.latitude,
            gnss.longitude,
            static_cast<int>(localX),
            static_cast<int>(localY)
        );
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
            Serial.println("[MAP-NAV-A1] partial refresh unavailable; using BW full refresh without PBF rebuild");
            FT02_RedrawCurrentPage();
            g_ft02MapPartialNavRefreshCount = 0;
        }
        return;
    }

    Serial.printf(
        "[MAP-NAV-A1] safe-box exit screen=%d,%d; recentering map at %.6f,%.6f\n",
        static_cast<int>(localX),
        static_cast<int>(localY),
        gnss.latitude,
        gnss.longitude
    );
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

    if(!FT02_MapCenterOnCurrentGnss())
    {
        FT02_PbfMapResetView();
        Serial.println("[MAP-A3.14] no live GNSS fix; opening at default center");
    }

    Serial.println("Page: HOME -> DIRECT PBF MAP A3.14 GNSS FOLLOW");
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
    Serial.println("Page: HOME -> LOCATION RECORDER A4 source=LR01 NAV_STATE");
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
    Serial.printf(
        "Page: LOCATION RECORDER -> LOCATION LOG LIST loading=%d count=%u selected=%u\n",
        status.loading ? 1 : 0,
        static_cast<unsigned int>(status.count),
        static_cast<unsigned int>(g_ft02LocationLogSelectedIndex)
    );
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
    Serial.printf(
        "Page: LOCATION LOG LIST -> DETAIL session=%s\n",
        entry.sessionId
    );
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
        Serial.printf(
            "[LOCATION-LOG] delete blocked for active session=%s\n",
            entry.sessionId
        );
        return false;
    }

    g_ft02LocationLogDeleteReturnPage = returnPage;
    g_ft02PageState = FT02_PAGE_LOCATION_LOG_DELETE_CONFIRM;
    FT02_RedrawCurrentPage();
    return true;
}


static bool FT02_ArmAudioCommandAfterRelease()
{
    if(g_ft02AudioCommandReleaseRequired)
    {
        Serial.println("[AUDIO-UI] command ignored until key release");
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
    if(reload)
    {
        FT02_AudioLogReload();
    }
    FT02_ClampAudioLogSelection();
    g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
    g_ft02AudioLastShownRecordBucket = 0xFFFFFFFFu;
    g_ft02AudioLastShownPlayBucket = 0xFFFFFFFFu;
    g_ft02AudioLastShownVolume = 0xFFu;
    g_ft02PageState = FT02_PAGE_AUDIO_LOG_LIST;
    Serial.printf(
        "Page: HOME -> AUDIO LOG LIST count=%u selected=%u\n",
        static_cast<unsigned int>(FT02_AudioLogStatusCurrent().count),
        static_cast<unsigned int>(g_ft02AudioLogSelectedIndex)
    );
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


static void FT02_OpenKnowledgePage()
{
    if(g_ft02PageState == FT02_PAGE_KNOWLEDGE)
    {
        return;
    }

    FT02_KnowledgeReset();
    g_ft02PageState = FT02_PAGE_KNOWLEDGE;
    Serial.print("Page: HOME -> FIELD MANUAL RUNTIME A1 ");
    Serial.println(FT02_FIRMWARE_VERSION);

    // v2.48 optimization: preserve the proven controller power-cycle and
    // full-mode reinitialization, then submit the final knowledge page as the
    // first frame. This removes the separate visible white refresh.
    FT02_DrawKnowledgeAfterCleanTransition("home-enter-knowledge");
}

static void FT02_OpenCompassCalibrationPage();

static void FT02_OpenDeviceStatusPage()
{
    if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS) return;
    g_ft02PageState = FT02_PAGE_DEVICE_STATUS;
    FT02_SystemSelfTestRun();
    Serial.println("Page: HOME -> DEVICE STATUS / SYSTEM SELF-TEST A1");
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
    if(event.key == FT02_KEY_CHAR && event.command == 'k')
    {
        FT02_OpenCompassCalibrationPage();
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
        FT02_SystemSelfTestRun();
        FT02_RedrawCurrentPage();
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
    Serial.println("Page: DEVICE STATUS -> COMPASS CALIBRATION A1");
    FT02_RedrawCurrentPage();
}

static bool FT02_HandleCompassCalibrationInput(const FT02InputEvent& event)
{
    const FT02CompassCalibrationUIAction action =
        FT02_CompassCalibrationUIHandleInput(event);

    if(action == FT02_COMPASS_CAL_UI_EXIT)
    {
        g_ft02PageState = FT02_PAGE_DEVICE_STATUS;
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
    g_ft02LastLR01NodeListMs = FT02_LR01HostState().lastNodeListMs;
    g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
    Serial.printf(
        "Page: HOME -> COMMUNICATION Complete Runtime A1 ready=%d nodes=%u\n",
        g_ft02CommunicationReadyPresented ? 1 : 0,
        static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount())
    );
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
    if(result == FT02_COMM_INPUT_REDRAW)
    {
        // Manual R resync resets NodeRuntime immediately. Mirror the real
        // readiness state here so the later READY transition is observable.
        // v2.70a left this flag true, which made the communication page remain
        // visually stuck at “同步中” even after NodeDB had recovered to READY.
        g_ft02CommunicationReadyPresented = FT02_LoRaNodeRuntimeReady();
        g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
        g_ft02LastLR01NodeListMs = FT02_LR01HostState().lastNodeListMs;
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
        Serial.print("Home card select: ");
        Serial.println(g_ft02HomeSelectedCard);

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
            FT02_OpenDeviceStatusPage();
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
        Serial.println("Home back requested");
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

    FT02_RedrawHomeCardSelection(
        display,
        oldSelectedCard,
        g_ft02HomeSelectedCard
    );

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
        Serial.println("Help page already open");
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
        Serial.printf("[MAP-UX-A4] stale map key suppressed key=%s raw=0x%02X; waiting release\n",
                      FT02_InputKeyName(event.key),
                      static_cast<unsigned>(static_cast<uint8_t>(event.raw)));
        return true;
    }

    if(event.key == FT02_KEY_BACK)
    {
        FT02_CancelPendingMapRefresh("back");
        FT02_ReturnHomePage();
        return true;
    }

    if(event.key == FT02_KEY_HELP)
    {
        FT02_CancelPendingMapRefresh("help");
        FT02_OpenHelpPage();
        return true;
    }

    if(event.key == FT02_KEY_SELECT)
    {
        FT02_CancelPendingMapRefresh("manual-refresh");
        FT02_RunDirectPbfMap(false, false);
        return true;
    }

    const int currentZoom = FT02_PbfMapZoomCurrent();
    const int panStep = currentZoom <= 16 ? 16 : (currentZoom == 17 ? 32 : 64);

    // Map UX A5: an e-paper map commit blocks the main loop for a noticeable
    // seconds. A direction key that remains held across that commit would be
    // seen as a fresh repeat immediately afterwards and schedule a second
    // screen refresh. After every committed map move/zoom, navigation input
    // stays disarmed until CardKB2 has reported a real key release once.
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
            Serial.printf("[MAP-A3.14] zoom-in key raw=0x%02X before=%d\n", (unsigned char)raw, before);
            if(FT02_PbfMapChangeZoom(1))
            {
                FT02_ScheduleMapZoomRefresh(before);
            }
            return true;
        }
        if(raw == 'e' || raw == 'E' || raw == '-' || raw == '_')
        {
            const int before = FT02_PbfMapZoomCurrent();
            Serial.printf("[MAP-A3.14] zoom-out key raw=0x%02X before=%d\n", (unsigned char)raw, before);
            if(FT02_PbfMapChangeZoom(-1))
            {
                FT02_ScheduleMapZoomRefresh(before);
            }
            return true;
        }
        if(event.command == 'r')
        {
            FT02_CancelPendingMapRefresh("recenter");
            // R means "return to my position". Without a live fix, never
            // destroy the user's current center/zoom by falling back to the
            // default map center.
            g_ft02MapFollowGnss = true;
            if(FT02_MapCenterOnCurrentGnss())
            {
                Serial.println("[MAP-NAV-A1] R recenter to live GNSS position");
                FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_NAVIGATION);
                FT02_RunDirectPbfMap(false, true);
            }
            else
            {
                Serial.println("[MAP-NAV-A1] R ignored: no valid GNSS fix; center/zoom preserved");
            }
            return true;
        }
        if(event.command == 'f')
        {
            FT02_CancelPendingMapRefresh("force-cache-rebuild");
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
    FT02_CancelPendingMapRefresh("search-open");
    FT02_EnsureBwAfterGrayMap("map-to-search");
    g_ft02MapRequireNavRelease = false;
    g_ft02MapReleaseZeroSinceMs = 0;
    FT02_MapSearchOpen();
    g_ft02PageState = FT02_PAGE_MAP_SEARCH;
    Serial.println("Page: MAP -> MAP SEARCH A2");
    FT02_RedrawCurrentPage();
}

static bool FT02_HandleMapSearchInput(const FT02InputEvent& event)
{
    const FT02MapSearchAction action = FT02_MapSearchHandleInput(event);
    if(action == FT02_MAP_SEARCH_ACTION_EXIT_MAP)
    {
        g_ft02PageState = FT02_PAGE_MAP;
        Serial.println("Page: MAP SEARCH -> MAP");
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
            FT02_CancelPendingMapRefresh("search-jump");
            // ENTER selected the result. The map build/gray commit can block for
            // seconds (or ~1 minute on a cold regional-cache build), so arm the
            // release gate before changing page state. Any buffered/repeated
            // ENTER is then discarded when the loop resumes instead of being
            // reinterpreted as the map-page manual-refresh command.
            FT02_ArmMapNavReleaseGate("search-jump");
            FT02_PbfMapSetCenter(lon, lat);
            g_ft02PageState = FT02_PAGE_MAP;
            Serial.printf("[MAP-SEARCH-A4] jump name=\"%s\" center=%.7f,%.7f page=MAP one-commit=yes\n", name, lon, lat);
            Serial.println("Page: MAP SEARCH -> MAP (search jump)");
            // Map Load UX A2: acknowledge the selection with a contextual
            // loading-state partial refresh before the potentially blocking PBF
            // build. The final destination map remains the only gray-map commit.
            FT02_PbfMapSetLoadingReason(FT02_MAP_LOADING_SEARCH);
            FT02_RunDirectPbfMap(false, true);
        }
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
    // Event-only trace: no periodic spam, but every deliberate recorder-page
    // key gives us a hard checkpoint if a future real-device stall reappears.
    Serial.printf(
        "[RECORDER-INPUT] key=%s raw=0x%02X cmd=%c\n",
        FT02_InputKeyName(event.key),
        static_cast<unsigned int>(static_cast<uint8_t>(event.raw)),
        event.command >= 0x20 && event.command <= 0x7E ? event.command : '-'
    );
    if(event.key == FT02_KEY_BACK)
    {
        // Leaving the recorder page must not terminate an active journey.
        // FT02_LocationRecorderPoll() runs from the global main loop, so the
        // session, elapsed time and automatic track points continue in the
        // background while the user visits Home, Map or Knowledge pages.
        const FT02LocationRecorderSnapshot recorder =
            FT02_LocationRecorderSnapshotCurrent();
        Serial.printf(
            "[RECORDER] leave page; session=%s background=%s auto=%s points=%lu\n",
            recorder.sessionActive ? recorder.sessionId : "--",
            recorder.sessionActive ? "continue" : "idle",
            recorder.autoTrackEnabled ? "on" : "off",
            (unsigned long)recorder.pointCount
        );
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
            (void)FT02_LR01HostRequestStatus();
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
    g_ft02LocationLogSelectedIndex = next;
    FT02_DrawLocationLogListBodyPartial(display, g_ft02LocationLogSelectedIndex);
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
                g_ft02AudioLastShownVolume = updated.playbackVolumeLevel;
                g_ft02AudioLastShownPlayBucket = updated.playbackElapsedMs /
                    FT02_AUDIO_TIMER_REFRESH_MS;
                FT02_DrawAudioLogPlayingStatusPartial(display, updated);
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
            if(!FT02_ArmAudioCommandAfterRelease()) return false;
            const bool requested = FT02_AudioLogRequestPlaybackStop();
            if(requested)
            {
                const FT02AudioLogStatus updated = FT02_AudioLogStatusCurrent();
                FT02_DrawAudioLogPlayingStatusPartial(display, updated);
            }
            return requested;
        }

        Serial.println("[AUDIO-UI] input ignored while playing; UP/DOWN volume, B/ENTER/P stop");
        return false;
    }

    if(state == FT02_AUDIO_LOG_RECORDING || state == FT02_AUDIO_LOG_POST_ROLL)
    {
        if(event.key == FT02_KEY_CHAR && event.command == 'r')
        {
            if(!FT02_ArmAudioCommandAfterRelease()) return false;
            return FT02_AudioLogRequestStop();
        }
        Serial.println("[AUDIO-UI] input ignored while recording; press R to stop");
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
        if(!FT02_ArmAudioCommandAfterRelease()) return false;
        // Draw the initial 00:00 cue before capture starts. Later timer updates
        // are isolated to a small window while streaming continues in the audio task.
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
        if(!FT02_ArmAudioCommandAfterRelease()) return false;
        FT02AudioLogEntry entry;
        if(!FT02_AudioLogGetNewest(g_ft02AudioLogSelectedIndex, entry)) return false;

        // Start opens and scans the WAV but streaming does not begin until the
        // background audio task runs, so the playback screen can be committed first.
        const bool started = FT02_AudioLogPlayNewest(g_ft02AudioLogSelectedIndex);
        g_ft02AudioLogLastGeneration = FT02_AudioLogStatusCurrent().generation;
        if(started)
        {
            const FT02AudioLogStatus updated = FT02_AudioLogStatusCurrent();
            g_ft02AudioLastShownPlayBucket = 0;
            g_ft02AudioLastShownVolume = updated.playbackVolumeLevel;
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
        if(!FT02_ArmAudioCommandAfterRelease()) return false;
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
    g_ft02AudioLogSelectedIndex = next;
    FT02_DrawAudioLogListBodyPartial(display, g_ft02AudioLogSelectedIndex);
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
    if(!FT02_ArmAudioCommandAfterRelease()) return false;

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
    if(event.key == FT02_KEY_HELP)
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

    if(g_ft02PageState == FT02_PAGE_COMMUNICATION)
    {
        return FT02_HandleCommunicationInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)
    {
        return FT02_HandleDeviceStatusInput(event);
    }

    if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        return FT02_HandleCompassCalibrationInput(event);
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

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.print("FT-02 ");
    Serial.print(FT02_FIRMWARE_BUILD_LABEL);
    Serial.print(" Production BW Fast Navigation A2 Arrow + Voice Log A1 + Location Recorder A4 + FieldManualRuntime + PBF Map UI A3.14 + Map UX A5 + Map Search A5 SPI40 ");
    Serial.print(FT02_StorageProfileText());
    Serial.println(" Start");

    Serial.print("[BOOT] flash_bytes=");
    Serial.print(ESP.getFlashChipSize());
    Serial.print(" heap_free=");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" psram_bytes=");
    Serial.print(ESP.getPsramSize());
    Serial.print(" psram_free=");
    Serial.println(ESP.getFreePsram());

    const bool fontSelfTest =
        FT02_HasGlyphPack(ft02_cjk_24r, 0x58F3) && // 壳
        FT02_HasGlyphPack(ft02_cjk_24r, 0x77E5) && // 知
        FT02_HasGlyphPack(ft02_cjk_24r, 0x8BC6) && // 识
        FT02_HasGlyphPack(ft02_cjk_24r, 0x5E93);   // 库
    Serial.print("[FONT] global_cjk_24r glyphs=");
    Serial.print(FT02_CJK_24R_GLYPH_COUNT);
    Serial.print(" self_test=");
    Serial.println(fontSelfTest ? "PASS" : "FAIL");

    const bool smallFontSelfTest =
        FT02_HasGlyphPack(ft02_cjk_20r, 0x8BF4) && // 说
        FT02_HasGlyphPack(ft02_cjk_20r, 0x660E) && // 明
        FT02_HasGlyphPack(ft02_cjk_20r, 0x6458) && // 摘
        FT02_HasGlyphPack(ft02_cjk_20r, 0x8981);   // 要
    Serial.print("[FONT] global_cjk_20r glyphs=");
    Serial.print(FT02_CJK_20R_GLYPH_COUNT);
    Serial.print(" self_test=");
    Serial.println(smallFontSelfTest ? "PASS" : "FAIL");


    const bool boldFontSelfTest =
        FT02_HasGlyphPack(ft02_cjk_24b, 0x58F3) && // 壳
        FT02_HasGlyphPack(ft02_cjk_24b, 0x77E5) && // 知
        FT02_HasGlyphPack(ft02_cjk_24b, 0x8BC6) && // 识
        FT02_HasGlyphPack(ft02_cjk_24b, 0x5E93);   // 库
    Serial.print("[FONT] global_cjk_24b glyphs=");
    Serial.print(FT02_CJK_24B_GLYPH_COUNT);
    Serial.print(" self_test=");
    Serial.println(boldFontSelfTest ? "PASS" : "FAIL");

    FT02_InputBegin(
        CARDKB_SDA,
        CARDKB_SCL,
        CARDKB_ADDR
    );

    Serial.println("FT02 InputManager ready: fixed pins SDA=47 SCL=21, D=UP, Z=LEFT, X=DOWN, C=RIGHT");

    FT02_GnssBegin();
    FT02_LR01HostBegin();
    FT02_SyncGnssStatusBar(false);

    FT02_StorageBegin();
    FT02_RefreshStorageStatusCache();
    FT02_GnssFieldTestBegin();
    FT02_PinyinLearningBegin();
    FT02_LoRaMessageDeliveryBegin();
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

    if(!g_ft02EpdSpi.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS))
    {
        Serial.println("[EPD] HSPI begin failed");
    }

    display.init(
        115200,
        true,
        10,
        false,
        g_ft02EpdSpi,
        SPISettings(EPD_SPI_HZ, MSBFIRST, SPI_MODE0)
    );
    display.setRotation(2);

    g_ft02BootDateTime = FT02_ReadBuildDateTime();
    g_ft02BootMillis = millis();
    FT02_UpdateClockCacheOnly();

    FT02_DrawHomeScreen(
        display,
        g_ft02HomeSelectedCard
    );

    Serial.print("FT-02 ");
    Serial.print(FT02_FIRMWARE_BUILD_LABEL);
    Serial.print(" Production BW Fast Navigation A2 Arrow + Voice Log A1 + Location Recorder A4 + FieldManualRuntime + PBF Map UI A3.14 + Map UX A5 + Map Search A5 SPI40 ");
    Serial.print(FT02_StorageProfileText());
    Serial.println(" ready");
    Serial.flush();
}

void loop()
{
    FT02_PrintRuntimeBannerIfNeeded();

    // Input is a top-priority service. In particular, never place a blocking
    // e-paper refresh ahead of CardKB polling: a long display commit can make
    // the recorder page appear frozen and can starve short key presses.
    FT02InputEvent inputEvent = FT02_InputPoll();
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
                Serial.printf("[MAP-INPUT-A5] navigation re-armed after stable release %lums\n",
                              static_cast<unsigned long>(FT02_MAP_NAV_RELEASE_STABLE_MS));
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

    if(inputEvent.key != FT02_KEY_NONE)
    {
        FT02_HandleInput(inputEvent);
    }

    if(g_ft02PageState == FT02_PAGE_MAP_SEARCH && FT02_MapSearchTakeDeferredRedraw(millis()))
    {
        FT02_RedrawCurrentPage();
    }

    FT02_LR01HostPoll();

    if(g_ft02PageState == FT02_PAGE_COMPASS_CALIBRATION)
    {
        const uint32_t calRevision = FT02_CompassCalibrationUIRevision();
        const uint32_t nowCal = millis();
        if(calRevision != g_ft02LastCompassCalRevision &&
           static_cast<uint32_t>(nowCal - g_ft02LastCompassCalRedrawMs) >= 1500u)
        {
            g_ft02LastCompassCalRevision = calRevision;
            g_ft02LastCompassCalRedrawMs = nowCal;
            FT02_RedrawCurrentPage();
            Serial.printf("[COMPASS-CAL-UI-A1] redraw revision=%lu\n",
                          static_cast<unsigned long>(calRevision));
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

        const bool loraLink = FT02_LR01HostOnline();
        const bool loraReady = FT02_LoRaNodeRuntimeReady();
        const uint32_t commRevision = FT02_LoRaCommunicationRevision();
        const uint16_t loraUnread = FT02_LoRaCommunicationUnreadCount();
        bool communicationPageRedrawn = false;

        if(g_ft02PageState == FT02_PAGE_COMMUNICATION &&
           !loraReady && g_ft02CommunicationReadyPresented)
        {
            // LR01 reported a readiness transition. Present it once without
            // invoking any Core-side radio recovery.
            g_ft02CommunicationReadyPresented = false;
            FT02_CommunicationUIOnSyncStarted("LR01 正在重新同步状态...");
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
            // Redraw after LR01 reports radio readiness and clear only the
            // transient status-sync notice. Node discovery remains LR01-owned.
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
                FT02_LR01HostState().lastNodeListMs != 0 &&
                FT02_LR01HostState().lastNodeListMs != g_ft02LastLR01NodeListMs)
        {
            // MESH_NODES? is asynchronous. The first draw happens immediately
            // after the request, while the LR01 node list is still in flight.
            // Redraw exactly once when MESH_NODE_END arrives so the first page
            // entry shows the completed list without requiring exit/re-entry.
            g_ft02LastLR01NodeListMs = FT02_LR01HostState().lastNodeListMs;
            FT02_CommunicationUIOnNodeListUpdated(FT02_LR01HostState().listedNodeCount);
            g_ft02LastCommunicationRevision = FT02_LoRaCommunicationRevision();
            g_ft02LastLoRaUnread = FT02_LoRaCommunicationUnreadCount();
            FT02_RedrawCurrentPage();
            communicationPageRedrawn = true;
            Serial.printf("[LR01-NODE-UI-A1] node list commit redraw count=%u\n",
                          static_cast<unsigned>(FT02_LR01HostState().listedNodeCount));
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
            FT02_RedrawCurrentPage();
            communicationPageRedrawn = true;
        }

        if(!communicationPageRedrawn && !FT02_IsNativeGrayPage() &&
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
            const uint32_t bucket = audioStatus.playbackElapsedMs /
                FT02_AUDIO_TIMER_REFRESH_MS;
            if(bucket != g_ft02AudioLastShownPlayBucket ||
               audioStatus.playbackVolumeLevel != g_ft02AudioLastShownVolume)
            {
                g_ft02AudioLastShownPlayBucket = bucket;
                g_ft02AudioLastShownVolume = audioStatus.playbackVolumeLevel;
                FT02_DrawAudioLogPlayingStatusPartial(display, audioStatus);
            }
        }
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
    // map build plus gray refresh.
    FT02_ProcessPendingMapRefresh();

    if(!FT02_AudioLogIsBusy())
    {
        FT02_UpdateClockIfNeeded(false);
    }

    delay(FT02_AudioLogIsPlaying() ? 5 : 20);
}
