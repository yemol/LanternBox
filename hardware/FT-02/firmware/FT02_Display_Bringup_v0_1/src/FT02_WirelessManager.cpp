#include "FT02_WirelessManager.h"

#include <stdio.h>
#include <string.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_StatusBar.h"
#include "wifi/FT02_CoreLink.h"
#include "wifi/FT02_CorePortalClient.h"
#include "wifi/FT02_CorePortalRenderer.h"
#include "wifi/FT02_WifiManager.h"

namespace
{
static constexpr uint8_t FT02_WIFI_VISIBLE_NETWORKS = 7;
static constexpr int FT02_WIFI_LIST_FIRST_BASELINE_Y = 182;
static constexpr int FT02_WIFI_LIST_ROW_H = 29;

static constexpr size_t FT02_WIFI_PASSWORD_LEN = FT02_WIFI_PROFILE_PASSWORD_LEN;

enum FT02WirelessWiFiUiMode : uint8_t
{
    FT02_WIRELESS_WIFI_MODE_NORMAL = 0,
    FT02_WIRELESS_WIFI_MODE_PASSWORD,
    FT02_WIRELESS_WIFI_MODE_CORE_ADDRESS
};

enum FT02WirelessWiFiCard : uint8_t
{
    FT02_WIRELESS_WIFI_CARD_ENABLE = 0,
    FT02_WIRELESS_WIFI_CARD_CORE_ADDRESS,
    FT02_WIRELESS_WIFI_CARD_COUNT
};

static uint8_t g_networkSelected = 0;
static uint8_t g_wifiCardSelected = FT02_WIRELESS_WIFI_CARD_ENABLE;
static FT02WirelessWiFiUiMode g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
static char g_selectedSsid[FT02_WIFI_PROFILE_SSID_LEN] = {};
static char g_passwordInput[FT02_WIFI_PASSWORD_LEN] = {};
static char g_coreAddressInput[FT02_CORE_LINK_ADDRESS_LEN] = {};
static uint8_t g_passwordLength = 0;
static uint8_t g_coreAddressLength = 0;
static bool g_portalOpened = false;
static uint32_t g_lastPollRedrawMs = 0;
static char g_portalActionMessage[96] = "";

static const FT02BottomBarItem WIFI_OFF_BOTTOM[3] = {
    {nullptr, "Z/D/X/C 选择"},
    {nullptr, "1/ENTER 执行"},
    {nullptr, "0/B 返回"}
};

static const FT02BottomBarItem WIFI_LIST_BOTTOM[3] = {
    {nullptr, "↑/↓ 选择SSID"},
    {nullptr, "ENTER 连接"},
    {nullptr, "R扫描 / 0返回"}
};

static const FT02BottomBarItem WIFI_PASSWORD_BOTTOM[3] = {
    {nullptr, "输入密码"},
    {nullptr, "ENTER 连接"},
    {nullptr, "退格删除 / B取消"}
};

static const FT02BottomBarItem WIFI_CORE_ADDRESS_BOTTOM[3] = {
    {nullptr, "输入Core地址"},
    {nullptr, "ENTER 保存"},
    {nullptr, "退格删除 / B取消"}
};

static const FT02BottomBarItem WIFI_PORTAL_BOTTOM[3] = {
    {nullptr, "Z/D/X/C 选择"},
    {nullptr, "ENTER 打开"},
    {nullptr, "R扫描 / B返回"}
};


static const char* screenTitleText()
{
    return FT02_WifiManagerIsConnected() &&
           g_wifiUiMode == FT02_WIRELESS_WIFI_MODE_NORMAL
        ? "壳中灯"
        : "WIFI";
}

static void drawScreenTitle(FT02Display& display)
{
    FT02_DrawStatusBar(display);
    FT02_DrawTextPack(display, ft02_cjk_24b, screenTitleText(), 48, 116);
    display.drawLine(48, 134, 752, 134, GxEPD_BLACK);
}

static void drawFooterStatus(FT02Display& display, const char* text)
{
    display.drawLine(48, 394, 752, 394, GxEPD_BLACK);
    FT02_DrawTextPackClipped(display, ft02_cjk_20r, text != nullptr ? text : "", 48, 421, 48, 399, 704, 28);
}

static void drawActionCard(FT02Display& display, int x, int y, int w, int h, const char* number, const char* label, bool selected)
{
    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
    }
    else
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, w - 4, h - 4, GxEPD_BLACK);
    }

    const int labelW = FT02_TextWidthPack(ft02_cjk_24b, label);
    const int labelX = x + (w - labelW) / 2;
    const bool hasNumber = number != nullptr && number[0] != '\0';
    if(selected)
    {
        if(hasNumber) FT02_DrawTextPackInvert(display, ft02_cjk_20r, number, x + 18, y + 32);
        FT02_DrawTextPackInvert(display, ft02_cjk_24b, label, labelX, y + 60);
    }
    else
    {
        if(hasNumber) FT02_DrawTextPack(display, ft02_cjk_20r, number, x + 18, y + 32);
        FT02_DrawTextPack(display, ft02_cjk_24b, label, labelX, y + 60);
    }
}

static bool isPrintableAscii(char c)
{
    const uint8_t code = static_cast<uint8_t>(c);
    return code >= 0x20 && code <= 0x7E;
}

static const char* wifiStateText()
{
    const FT02WifiState state = FT02_WifiManagerState();
    if(state == FT02_WIFI_STATE_OFF) return "OFF";
    if(FT02_WifiManagerIsConnected()) return "CONNECTED";
    if(state == FT02_WIFI_STATE_ERROR) return "ERROR";
    return "ON";
}

static void resetWifiUiRuntime()
{
    g_networkSelected = 0;
    g_wifiCardSelected = FT02_WIRELESS_WIFI_CARD_ENABLE;
    g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
    g_selectedSsid[0] = '\0';
    g_passwordInput[0] = '\0';
    g_coreAddressInput[0] = '\0';
    g_passwordLength = 0;
    g_coreAddressLength = 0;
    g_portalOpened = false;
    g_portalActionMessage[0] = '\0';
    FT02_CoreLinkReset();
    FT02_CorePortalClientReset();
}

static void openPortalIfNeeded()
{
    if(!FT02_WifiManagerIsConnected())
    {
        if(g_portalOpened)
        {
            g_portalOpened = false;
            FT02_CoreLinkReset();
            FT02_CorePortalClientReset();
        }
        return;
    }

    if(g_portalOpened) return;
    g_portalOpened = true;
    g_portalActionMessage[0] = '\0';
    FT02_CoreLinkStartDiscovery();
    FT02_CorePortalClientOpenHome();
}

static void beginPasswordForSelected()
{
    FT02WifiNetwork network = {};
    if(!FT02_WifiManagerGetNetwork(g_networkSelected, network)) return;
    snprintf(g_selectedSsid, sizeof(g_selectedSsid), "%s", network.ssid);
    g_passwordInput[0] = '\0';
    g_passwordLength = 0;
    g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_PASSWORD;
}

static void connectSelectedWithPassword()
{
    (void)FT02_WifiManagerConnectNetwork(g_networkSelected, g_passwordInput);
    g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
    g_portalOpened = false;
    g_portalActionMessage[0] = '\0';
}

static void connectSelectedOpenNetwork()
{
    g_passwordInput[0] = '\0';
    g_passwordLength = 0;
    connectSelectedWithPassword();
}

static void beginCoreAddressInput()
{
    snprintf(g_coreAddressInput, sizeof(g_coreAddressInput), "%s",
             FT02_CoreLinkHasConfiguredAddress() ? FT02_CoreLinkConfiguredAddressText() : "");
    g_coreAddressLength = static_cast<uint8_t>(strnlen(g_coreAddressInput, sizeof(g_coreAddressInput)));
    g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_CORE_ADDRESS;
}

static void saveCoreAddressInput()
{
    const bool saved = FT02_CoreLinkSetConfiguredAddress(g_coreAddressInput);
    snprintf(g_portalActionMessage, sizeof(g_portalActionMessage), "%s", saved ? "Core地址已保存" : "Core地址已记录，SD保存失败");
    g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
    if(FT02_WifiManagerIsConnected())
    {
        FT02_CoreLinkStartDiscovery();
        FT02_CorePortalClientRefreshFromCoreLink();
    }
}

static void startFreshScanFromUi()
{
    resetWifiUiRuntime();
    FT02_WifiManagerStartFreshScan();
}

static uint8_t firstVisibleNetworkIndex()
{
    const uint8_t count = FT02_WifiManagerNetworkCount();
    if(count <= FT02_WIFI_VISIBLE_NETWORKS) return 0;
    if(g_networkSelected < FT02_WIFI_VISIBLE_NETWORKS) return 0;
    uint8_t first = static_cast<uint8_t>(g_networkSelected - FT02_WIFI_VISIBLE_NETWORKS + 1);
    const uint8_t maxFirst = static_cast<uint8_t>(count - FT02_WIFI_VISIBLE_NETWORKS);
    return first > maxFirst ? maxFirst : first;
}

static void drawNetworkRow(FT02Display& display, uint8_t index, uint8_t visibleRow)
{
    FT02WifiNetwork network = {};
    if(!FT02_WifiManagerGetNetwork(index, network)) return;

    const int y = FT02_WIFI_LIST_FIRST_BASELINE_Y + static_cast<int>(visibleRow) * FT02_WIFI_LIST_ROW_H;
    const bool selected = index == g_networkSelected;
    if(selected)
    {
        display.fillRect(34, y - 21, 732, 25, GxEPD_BLACK);
    }

    char ssid[44];
    snprintf(ssid, sizeof(ssid), "%u %s", static_cast<unsigned>(index + 1), network.ssid);
    char info[64];
    snprintf(info, sizeof(info), "%s  %lddBm", FT02_WifiAuthText(network.auth), static_cast<long>(network.rssi));

    const int infoWidth = FT02_TextWidthPack(ft02_cjk_20r, info);
    const int infoX = display.width() - 48 - infoWidth;
    const int clipW = infoX > 70 ? infoX - 58 : 220;
    if(selected)
    {
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, ssid, 48, y, 42, y - 22, clipW, 27);
        FT02_DrawTextPackInvert(display, ft02_cjk_20r, info, infoX, y);
    }
    else
    {
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, ssid, 48, y, 42, y - 22, clipW, 27);
        FT02_DrawTextPack(display, ft02_cjk_20r, info, infoX, y);
    }
}

} // namespace

void FT02_WirelessManagerBegin()
{
    FT02_WifiManagerBegin();
    FT02_CoreLinkBegin();
    FT02_CorePortalClientBegin();
}

void FT02_WirelessManagerOpenWiFi()
{
    resetWifiUiRuntime();
    if(FT02_WifiManagerOwner() == FT02_WIFI_OWNER_WIFI_UI)
    {
        FT02_WifiManagerShutdown(FT02_WIFI_OWNER_WIFI_UI);
    }
}

void FT02_WirelessManagerCloseWiFi()
{
    resetWifiUiRuntime();
    FT02_WifiManagerShutdown(FT02_WIFI_OWNER_WIFI_UI);
}

bool FT02_WirelessManagerPoll(uint32_t nowMs)
{
    bool redraw = FT02_WifiManagerPoll(nowMs);
    const bool wasPortalOpen = g_portalOpened;
    openPortalIfNeeded();
    if(g_portalOpened != wasPortalOpen) redraw = true;

    if(g_portalOpened)
    {
        if(FT02_CoreLinkPoll(nowMs))
        {
            FT02_CorePortalClientRefreshFromCoreLink();
            redraw = true;
        }
    }

    if(redraw)
    {
        const uint32_t elapsed = nowMs - g_lastPollRedrawMs;
        if(g_lastPollRedrawMs != 0 && elapsed < 900u) return false;
        g_lastPollRedrawMs = nowMs;
    }
    return redraw;
}

FT02WirelessAction FT02_WirelessManagerHandleWiFiInput(const FT02InputEvent& event)
{
    if(g_wifiUiMode == FT02_WIRELESS_WIFI_MODE_CORE_ADDRESS)
    {
        if(event.key == FT02_KEY_SELECT)
        {
            saveCoreAddressInput();
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(static_cast<uint8_t>(event.raw) == 0x08)
        {
            if(g_coreAddressLength > 0)
            {
                g_coreAddressInput[--g_coreAddressLength] = '\0';
                return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
            }
            g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_BACK)
        {
            g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
            return FT02_WIRELESS_ACTION_REDRAW;
        }
        if(isPrintableAscii(event.raw) && event.raw != ' ' && g_coreAddressLength + 1 < FT02_CORE_LINK_ADDRESS_LEN)
        {
            g_coreAddressInput[g_coreAddressLength++] = event.raw;
            g_coreAddressInput[g_coreAddressLength] = '\0';
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        return FT02_WIRELESS_ACTION_NONE;
    }

    if(g_wifiUiMode == FT02_WIRELESS_WIFI_MODE_PASSWORD)
    {
        if(event.key == FT02_KEY_SELECT)
        {
            connectSelectedWithPassword();
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(static_cast<uint8_t>(event.raw) == 0x08)
        {
            if(g_passwordLength > 0)
            {
                g_passwordInput[--g_passwordLength] = '\0';
                return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
            }
            g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_BACK)
        {
            g_wifiUiMode = FT02_WIRELESS_WIFI_MODE_NORMAL;
            return FT02_WIRELESS_ACTION_REDRAW;
        }
        if(isPrintableAscii(event.raw) && g_passwordLength + 1 < FT02_WIFI_PASSWORD_LEN)
        {
            g_passwordInput[g_passwordLength++] = event.raw;
            g_passwordInput[g_passwordLength] = '\0';
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        return FT02_WIRELESS_ACTION_NONE;
    }

    if(event.key == FT02_KEY_BACK || (event.key == FT02_KEY_CHAR && event.command == '0'))
    {
        FT02_WirelessManagerCloseWiFi();
        return FT02_WIRELESS_ACTION_EXIT_TOOLS;
    }
    if(event.key == FT02_KEY_HELP) return FT02_WIRELESS_ACTION_OPEN_HELP;

    if(FT02_WifiManagerState() == FT02_WIFI_STATE_OFF)
    {
        if(event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP)
        {
            g_wifiCardSelected = FT02_WIRELESS_WIFI_CARD_ENABLE;
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN)
        {
            g_wifiCardSelected = FT02_WIRELESS_WIFI_CARD_CORE_ADDRESS;
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_CHAR && event.command == '1')
        {
            resetWifiUiRuntime();
            FT02_WifiManagerStartUiSession();
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_SELECT)
        {
            if(g_wifiCardSelected == FT02_WIRELESS_WIFI_CARD_CORE_ADDRESS)
            {
                beginCoreAddressInput();
            }
            else
            {
                resetWifiUiRuntime();
                FT02_WifiManagerStartUiSession();
            }
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
    }

    if(FT02_WifiManagerIsConnected())
    {
        if(event.key == FT02_KEY_CHAR && event.command == 'r')
        {
            startFreshScanFromUi();
            return FT02_WIRELESS_ACTION_REDRAW;
        }
        if(event.key == FT02_KEY_UP)
        {
            if(FT02_CorePortalClientMoveGrid(-1, 0)) return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_DOWN)
        {
            if(FT02_CorePortalClientMoveGrid(1, 0)) return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_LEFT)
        {
            if(FT02_CorePortalClientMoveGrid(0, -1)) return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_RIGHT)
        {
            if(FT02_CorePortalClientMoveGrid(0, 1)) return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_SELECT)
        {
            if(FT02_CorePortalClientSelectCurrent())
            {
                snprintf(g_portalActionMessage, sizeof(g_portalActionMessage), "%s", FT02_CorePortalClientMessage());
                return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
            }
        }
        return FT02_WIRELESS_ACTION_NONE;
    }

    if(FT02_WifiManagerIsSelectableList())
    {
        const uint8_t count = FT02_WifiManagerNetworkCount();
        if(g_networkSelected >= count) g_networkSelected = 0;
        if(event.key == FT02_KEY_UP)
        {
            g_networkSelected = g_networkSelected == 0
                ? static_cast<uint8_t>(count - 1)
                : static_cast<uint8_t>(g_networkSelected - 1);
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_DOWN)
        {
            g_networkSelected = static_cast<uint8_t>((g_networkSelected + 1) % count);
            return FT02_WIRELESS_ACTION_PARTIAL_REDRAW;
        }
        if(event.key == FT02_KEY_SELECT)
        {
            FT02WifiNetwork network = {};
            if(!FT02_WifiManagerGetNetwork(g_networkSelected, network)) return FT02_WIRELESS_ACTION_NONE;
            if(network.auth == WIFI_AUTH_OPEN) connectSelectedOpenNetwork();
            else beginPasswordForSelected();
            return FT02_WIRELESS_ACTION_REDRAW;
        }
        if(event.key == FT02_KEY_CHAR && event.command >= '1' && event.command <= '9')
        {
            const uint8_t selected = static_cast<uint8_t>(event.command - '1');
            if(selected < count)
            {
                g_networkSelected = selected;
                FT02WifiNetwork network = {};
                if(!FT02_WifiManagerGetNetwork(g_networkSelected, network)) return FT02_WIRELESS_ACTION_NONE;
                if(network.auth == WIFI_AUTH_OPEN) connectSelectedOpenNetwork();
                else beginPasswordForSelected();
                return FT02_WIRELESS_ACTION_REDRAW;
            }
        }
    }

    if((FT02_WifiManagerState() == FT02_WIFI_STATE_SELECTING || FT02_WifiManagerState() == FT02_WIFI_STATE_ERROR) &&
       event.key == FT02_KEY_CHAR && event.command == 'r')
    {
        startFreshScanFromUi();
        return FT02_WIRELESS_ACTION_REDRAW;
    }

    return FT02_WIRELESS_ACTION_NONE;
}

static void drawWirelessWiFiBody(FT02Display& display)
{
    display.fillRect(0, 136, display.width(), display.height() - 136, GxEPD_WHITE);

    const FT02WifiState state = FT02_WifiManagerState();
    char footer[180];
    snprintf(footer, sizeof(footer), "状态: %s  %s", wifiStateText(), FT02_WifiManagerStatusText());

    if(g_wifiUiMode == FT02_WIRELESS_WIFI_MODE_CORE_ADDRESS)
    {
        FT02_DrawTextPack(display, ft02_cjk_24b, "Core地址", 48, 188);
        FT02_DrawTextPackClipped(display, ft02_cjk_24r, g_coreAddressInput, 52, 246, 48, 214, 704, 42);
        display.drawRect(48, 214, 704, 52, GxEPD_BLACK);
        FT02_DrawTextPack(display, ft02_cjk_20r, "例如 192.168.71.20:8000", 48, 320);
        snprintf(footer, sizeof(footer), "ENTER保存  B取消  当前: %s", FT02_CoreLinkConfiguredAddressText());
        FT02_DrawBottomBarWithFont(display, WIFI_CORE_ADDRESS_BOTTOM, ft02_cjk_20r);
    }
    else if(g_wifiUiMode == FT02_WIRELESS_WIFI_MODE_PASSWORD)
    {
        char ssidLine[96];
        snprintf(ssidLine, sizeof(ssidLine), "SSID  %s", g_selectedSsid);
        FT02_DrawTextPackClipped(display, ft02_cjk_24b, ssidLine, 48, 188, 48, 162, 704, 36);
        FT02_DrawTextPack(display, ft02_cjk_20r, "Password", 48, 238);
        FT02_DrawTextPackClipped(display, ft02_cjk_24r, g_passwordInput, 52, 280, 48, 252, 704, 42);
        display.drawRect(48, 252, 704, 52, GxEPD_BLACK);
        snprintf(footer, sizeof(footer), "ENTER连接  退格删除  B取消");
        FT02_DrawBottomBarWithFont(display, WIFI_PASSWORD_BOTTOM, ft02_cjk_20r);
    }
    else if(state == FT02_WIFI_STATE_OFF)
    {
        drawActionCard(display, 48, 188, 328, 92, "", "开启网络", g_wifiCardSelected == FT02_WIRELESS_WIFI_CARD_ENABLE);
        drawActionCard(display, 424, 188, 328, 92, "", "Core地址", g_wifiCardSelected == FT02_WIRELESS_WIFI_CARD_CORE_ADDRESS);
        FT02_DrawTextPack(display, ft02_cjk_20r, "按需开启，退出即关闭；不会后台扫描或自动常驻。", 98, 350);
        if(g_portalActionMessage[0] != '\0')
        {
            snprintf(footer, sizeof(footer), "%s  Core: %s", g_portalActionMessage, FT02_CoreLinkConfiguredAddressText());
        }
        else
        {
            snprintf(footer, sizeof(footer), "状态: OFF  Core: %s", FT02_CoreLinkConfiguredAddressText());
        }
        FT02_DrawBottomBarWithFont(display, WIFI_OFF_BOTTOM, ft02_cjk_20r);
    }
    else if(FT02_WifiManagerIsConnected())
    {
        FT02_CorePortalRendererDraw(display, FT02_CorePortalClientPage(), 156);
        const FT02CorePortalPage& page = FT02_CorePortalClientPage();
        snprintf(footer, sizeof(footer), "CONNECTED  IP %s  %s",
                 FT02_WifiManagerIpText(), page.status);
        FT02_DrawBottomBarWithFont(display, WIFI_PORTAL_BOTTOM, ft02_cjk_20r);
    }
    else if(state == FT02_WIFI_STATE_SELECTING)
    {
        const uint8_t count = FT02_WifiManagerNetworkCount();
        if(g_networkSelected >= count) g_networkSelected = 0;
        const uint8_t first = firstVisibleNetworkIndex();
        const uint8_t visible = count - first < FT02_WIFI_VISIBLE_NETWORKS
            ? static_cast<uint8_t>(count - first)
            : FT02_WIFI_VISIBLE_NETWORKS;
        for(uint8_t row = 0; row < visible; ++row)
        {
            drawNetworkRow(display, static_cast<uint8_t>(first + row), row);
        }
        snprintf(footer, sizeof(footer), "状态: ON  扫描完成 %u个网络  %u-%u/%u",
                 static_cast<unsigned>(count),
                 static_cast<unsigned>(first + 1),
                 static_cast<unsigned>(first + visible),
                 static_cast<unsigned>(count));
        FT02_DrawBottomBarWithFont(display, WIFI_LIST_BOTTOM, ft02_cjk_20r);
    }
    else
    {
        drawActionCard(display, 236, 188, 328, 92, "", "重新扫描", state == FT02_WIFI_STATE_ERROR);
        snprintf(footer, sizeof(footer), "状态: %s  %s", wifiStateText(), FT02_WifiManagerStatusText());
        FT02_DrawBottomBarWithFont(display, WIFI_LIST_BOTTOM, ft02_cjk_20r);
    }

    drawFooterStatus(display, footer);
}

void FT02_DrawWirelessWiFiScreen(FT02Display& display)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        drawScreenTitle(display);
        drawWirelessWiFiBody(display);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "wireless-wifi-full");
}

void FT02_DrawWirelessWiFiBodyPartial(FT02Display& display)
{
    const uint32_t startedMs = millis();
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(0, 136, display.width(), display.height() - 136);
    display.firstPage();
    do
    {
        drawWirelessWiFiBody(display);
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display, "wireless-wifi-body-partial", startedMs);
}
