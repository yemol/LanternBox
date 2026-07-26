#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>

#include "FT02_HomeUI.h"
#include "FT02_HelpUI.h"
#include "FT02_PageState.h"
#include "FT02_HomeCards.h"
#include "FT02_InputManager.h"
#include "FT02_StatusBar.h"
#include "FT02_Storage.h"
#include <stdio.h>
#include <stdint.h>

constexpr int EPD_PWR  = 18;
constexpr int EPD_BUSY = 3;
constexpr int EPD_RST  = 8;
constexpr int EPD_DC   = 9;
constexpr int EPD_CS   = 10;
constexpr int EPD_MOSI = 11;
constexpr int EPD_SCK  = 12;

constexpr int CARDKB_SDA = 4;
constexpr int CARDKB_SCL = 5;
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
static bool g_ft02RuntimeBannerPrinted = false;
static bool g_ft02StorageProbeStarted = false;

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
    FT02DateTime dt = g_ft02BootDateTime;

    uint32_t elapsedMinutes = (millis() - g_ft02BootMillis) / 60000UL;
    FT02_AddMinutes(
        dt,
        elapsedMinutes
    );

    return dt;
}

static void FT02_UpdateClockIfNeeded(bool force)
{
    FT02DateTime now = FT02_CurrentDateTime();
    int minuteKey = now.hour * 60 + now.minute;

    if(!force && minuteKey == g_ft02LastShownMinute)
    {
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



static void FT02_UpdateStorageTopStatus()
{
    char line1[16];
    char line2[16];

    if(FT02_StorageIsReady())
    {
        unsigned long freeGb = FT02_StorageFreeMB() / 1024UL;

        if(freeGb < 1 && FT02_StorageFreeMB() > 0)
        {
            freeGb = 1;
        }

        snprintf(
            line1,
            sizeof(line1),
            "%luG",
            freeGb
        );

        if(FT02_StorageRwResultCurrent() == FT02_STORAGE_RW_OK)
        {
            snprintf(
                line2,
                sizeof(line2),
                "RW OK"
            );
        }
        else
        {
            snprintf(
                line2,
                sizeof(line2),
                "SD剩余"
            );
        }
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_SCANNING)
    {
        snprintf(
            line1,
            sizeof(line1),
            "SD"
        );

        snprintf(
            line2,
            sizeof(line2),
            "SCAN"
        );
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_NO_CARD)
    {
        snprintf(
            line1,
            sizeof(line1),
            "NO"
        );

        snprintf(
            line2,
            sizeof(line2),
            "SD"
        );
    }
    else if(FT02_StorageStateCurrent() == FT02_STORAGE_STATE_ERROR)
    {
        snprintf(
            line1,
            sizeof(line1),
            "SD"
        );

        snprintf(
            line2,
            sizeof(line2),
            "ERR"
        );
    }
    else
    {
        snprintf(
            line1,
            sizeof(line1),
            "SD"
        );

        snprintf(
            line2,
            sizeof(line2),
            "INIT"
        );
    }

    FT02_DrawStatusBarStorage(
        display,
        line1,
        line2
    );
}

static void FT02_RunDelayedStorageProbeIfNeeded()
{
    if(g_ft02StorageProbeStarted)
    {
        return;
    }

    if(millis() < 5000)
    {
        return;
    }

    g_ft02StorageProbeStarted = true;

    Serial.println("FT-02 SDMMC 1-bit file R/W smoke v1.90 begins now");
    Serial.flush();

    FT02_DrawStatusBarStorage(
        display,
        "SD",
        "SCAN"
    );

    FT02_StorageBegin();

    FT02_UpdateStorageTopStatus();

    Serial.print("FT-02 storage top status: ");
    Serial.print(FT02_StorageStateText());
    Serial.print(" error=");
    Serial.print(FT02_StorageErrorText());
    Serial.print(" freeMB=");
    Serial.print(FT02_StorageFreeMB());
    Serial.print(" rw=");
    Serial.print(FT02_StorageRwResultText());
    Serial.print(" cd=");
    Serial.print(FT02_StorageCardDetectRaw());
    Serial.print(" d3=");
    Serial.println(FT02_StorageD3Raw());
    Serial.flush();
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

    Serial.println("FT-02 runtime alive: v1.91 Help restored, SD base v1.90, CardKB2 SDA=4 SCL=5");
    Serial.flush();
}

static void FT02_RedrawCurrentPage()
{
    if(g_ft02PageState == FT02_PAGE_HELP)
    {
        FT02_DrawHelpScreen(display);
    }
    else
    {
        FT02_DrawHomeScreen(
            display,
            g_ft02HomeSelectedCard
        );
    }

    // Full-page rendering uses boot placeholders in the status bar.
    // Restore the live clock and the current storage state immediately.
    FT02_UpdateClockIfNeeded(true);
    FT02_UpdateStorageTopStatus();
}

static void FT02_OpenHelpPage()
{
    if(g_ft02PageState == FT02_PAGE_HELP)
    {
        return;
    }

    g_ft02PageState = FT02_PAGE_HELP;
    Serial.println("Page: HOME -> HELP");
    FT02_RedrawCurrentPage();
}

static void FT02_ReturnHomePage()
{
    if(g_ft02PageState == FT02_PAGE_HOME)
    {
        return;
    }

    g_ft02PageState = FT02_PAGE_HOME;
    Serial.println("Page: HELP -> HOME");
    FT02_RedrawCurrentPage();
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
        FT02_ReturnHomePage();
        return true;
    }

    if(event.key == FT02_KEY_HELP)
    {
        Serial.println("Help page already open");
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

    return FT02_HandleHomeInput(event);
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("FT-02 HomeScreen v1.91 HelpRestored From v1.90 Start");

    FT02_InputBegin(
        CARDKB_SDA,
        CARDKB_SCL,
        CARDKB_ADDR
    );

    Serial.println("FT02 InputManager ready: fixed pins SDA=4 SCL=5, D=UP, Z=LEFT, X=DOWN, C=RIGHT");

    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH);

    SPI.begin(
        EPD_SCK,
        -1,
        EPD_MOSI,
        EPD_CS
    );

    display.init(115200);
    display.setRotation(2);

    FT02_DrawHomeScreen(
        display,
        g_ft02HomeSelectedCard
    );

    g_ft02BootDateTime = FT02_ReadBuildDateTime();
    g_ft02BootMillis = millis();

    FT02_UpdateClockIfNeeded(true);

    Serial.println("FT-02 HomeScreen v1.91 HelpRestored From v1.90 Done");
    Serial.println("SDMMC 1-bit file R/W no-retry probe will run after 5 seconds.");
    Serial.flush();
}

void loop()
{
    FT02_PrintRuntimeBannerIfNeeded();
    FT02_RunDelayedStorageProbeIfNeeded();

    // Real elapsed clock:
    // refreshes only when the displayed minute changes.
    // The date is refreshed in the same local window and rolls over at midnight.
    FT02InputEvent inputEvent = FT02_InputPoll();

    if(inputEvent.key != FT02_KEY_NONE)
    {
        FT02_HandleInput(inputEvent);
    }

    if(FT02_StoragePollCardDetectChanged())
    {
        FT02_UpdateStorageTopStatus();
    }

    FT02_UpdateClockIfNeeded(false);

    delay(20);
}

