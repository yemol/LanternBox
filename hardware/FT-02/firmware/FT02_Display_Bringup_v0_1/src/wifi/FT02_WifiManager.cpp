#include "FT02_WifiManager.h"

#include <stdio.h>
#include <string.h>

#include "FT02_Storage.h"

namespace
{
static constexpr uint32_t FT02_WIFI_CONNECT_TIMEOUT_MS = 15000UL;

static FT02WifiOwner g_owner = FT02_WIFI_OWNER_NONE;
static FT02WifiState g_state = FT02_WIFI_STATE_OFF;
static FT02WifiNetwork g_networks[FT02_WIFI_MAX_NETWORKS] = {};
static uint8_t g_networkCount = 0;
static uint8_t g_savedConnectIndex = 0;
static uint32_t g_connectStartMs = 0;
static bool g_connectingSaved = false;
static char g_pendingSsid[FT02_WIFI_PROFILE_SSID_LEN] = {};
static char g_pendingPassword[FT02_WIFI_PROFILE_PASSWORD_LEN] = {};
static char g_connectedSsid[FT02_WIFI_PROFILE_SSID_LEN] = {};
static char g_ipText[24] = "--";
static char g_statusText[96] = "WiFi OFF";

static void setStatus(const char* text)
{
    snprintf(g_statusText, sizeof(g_statusText), "%s", text != nullptr ? text : "");
}

static void clearRuntime()
{
    g_networkCount = 0;
    g_savedConnectIndex = 0;
    g_connectStartMs = 0;
    g_connectingSaved = false;
    g_pendingSsid[0] = '\0';
    g_pendingPassword[0] = '\0';
    g_connectedSsid[0] = '\0';
    snprintf(g_ipText, sizeof(g_ipText), "--");
}

static bool beginConnect(const char* ssid, const char* password, bool saved)
{
    if(g_owner == FT02_WIFI_OWNER_NONE || ssid == nullptr || ssid[0] == '\0') return false;

    snprintf(g_pendingSsid, sizeof(g_pendingSsid), "%s", ssid);
    snprintf(g_pendingPassword, sizeof(g_pendingPassword), "%s", password != nullptr ? password : "");
    g_connectingSaved = saved;
    g_connectStartMs = millis();
    g_state = saved ? FT02_WIFI_STATE_CONNECTING_SAVED : FT02_WIFI_STATE_CONNECTING;
    snprintf(g_statusText, sizeof(g_statusText), "连接 %s ...", g_pendingSsid);

    WiFi.disconnect(false, false);
    WiFi.begin(g_pendingSsid, g_pendingPassword);
    return true;
}

static bool beginNextSavedConnect()
{
    if(!FT02_StorageIsReady())
    {
        setStatus("SD未就绪，开始扫描");
        return false;
    }
    if(!FT02_WifiProfileStoreLoad())
    {
        setStatus("无保存网络，开始扫描");
        return false;
    }

    FT02WifiProfile profile = {};
    while(g_savedConnectIndex < FT02_WifiProfileStoreCount())
    {
        if(!FT02_WifiProfileStoreGet(g_savedConnectIndex++, profile)) continue;
        if(!profile.autoConnect) continue;
        return beginConnect(profile.ssid, profile.password, true);
    }

    setStatus("保存网络连接失败，开始扫描");
    return false;
}

static void beginAsyncScan()
{
    g_networkCount = 0;
    WiFi.scanDelete();
    g_state = FT02_WIFI_STATE_SCANNING;
    setStatus("扫描热点...");
    const int startResult = WiFi.scanNetworks(true, true);
    if(startResult == WIFI_SCAN_FAILED)
    {
        setStatus("扫描启动失败");
        g_state = FT02_WIFI_STATE_ERROR;
    }
}

static bool finishScan(int found)
{
    if(found <= 0)
    {
        setStatus(found == 0 ? "未发现网络" : "扫描失败");
        g_state = FT02_WIFI_STATE_ERROR;
        WiFi.scanDelete();
        return true;
    }

    const uint8_t count = found < FT02_WIFI_MAX_NETWORKS ? found : FT02_WIFI_MAX_NETWORKS;
    for(uint8_t i = 0; i < count; ++i)
    {
        snprintf(g_networks[i].ssid, sizeof(g_networks[i].ssid), "%s", WiFi.SSID(i).c_str());
        g_networks[i].rssi = WiFi.RSSI(i);
        g_networks[i].auth = WiFi.encryptionType(i);
    }
    g_networkCount = count;
    snprintf(g_statusText, sizeof(g_statusText), "扫描完成：%u 个网络", static_cast<unsigned>(g_networkCount));
    g_state = FT02_WIFI_STATE_SELECTING;
    return true;
}

static bool finishConnect(bool success)
{
    if(success)
    {
        snprintf(g_connectedSsid, sizeof(g_connectedSsid), "%s", g_pendingSsid);
        snprintf(g_ipText, sizeof(g_ipText), "%s", WiFi.localIP().toString().c_str());
        (void)FT02_WifiProfileStoreUpsert(g_pendingSsid, g_pendingPassword, true);
        setStatus("CONNECTED");
        g_state = FT02_WIFI_STATE_CONNECTED;
        return true;
    }

    WiFi.disconnect(false, false);
    if(g_connectingSaved && beginNextSavedConnect()) return true;

    if(g_connectingSaved)
    {
        beginAsyncScan();
        return true;
    }

    snprintf(g_statusText, sizeof(g_statusText), "%s 连接失败", g_pendingSsid);
    g_state = g_networkCount > 0 ? FT02_WIFI_STATE_SELECTING : FT02_WIFI_STATE_ERROR;
    return true;
}
}

void FT02_WifiManagerBegin()
{
    FT02_WifiProfileStoreBegin();
    WiFi.persistent(false);
    WiFi.disconnect(false, false);
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    g_owner = FT02_WIFI_OWNER_NONE;
    g_state = FT02_WIFI_STATE_OFF;
    clearRuntime();
    setStatus("WiFi OFF");
}

bool FT02_WifiManagerRequest(FT02WifiOwner owner)
{
    if(owner != FT02_WIFI_OWNER_WIFI_UI && owner != FT02_WIFI_OWNER_SYNC) return false;
    if(g_owner != FT02_WIFI_OWNER_NONE && g_owner != owner) return false;
    g_owner = owner;
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);
    if(g_state == FT02_WIFI_STATE_OFF) g_state = FT02_WIFI_STATE_STARTING;
    return true;
}

void FT02_WifiManagerRelease(FT02WifiOwner owner)
{
    FT02_WifiManagerShutdown(owner);
}

void FT02_WifiManagerShutdown(FT02WifiOwner owner)
{
    if(g_owner != FT02_WIFI_OWNER_NONE && g_owner != owner) return;
    g_state = FT02_WIFI_STATE_SHUTDOWN;
    WiFi.disconnect(false, false);
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    g_owner = FT02_WIFI_OWNER_NONE;
    clearRuntime();
    g_state = FT02_WIFI_STATE_OFF;
    setStatus("WiFi OFF");
}

void FT02_WifiManagerStartUiSession()
{
    clearRuntime();
    setStatus("开启WiFi...");
    if(!FT02_WifiManagerRequest(FT02_WIFI_OWNER_WIFI_UI))
    {
        setStatus("WiFi owner busy");
        g_state = FT02_WIFI_STATE_ERROR;
        return;
    }
    g_savedConnectIndex = 0;
    if(!beginNextSavedConnect()) beginAsyncScan();
}

void FT02_WifiManagerStartFreshScan()
{
    if(!FT02_WifiManagerRequest(FT02_WIFI_OWNER_WIFI_UI))
    {
        setStatus("WiFi owner busy");
        g_state = FT02_WIFI_STATE_ERROR;
        return;
    }
    g_connectedSsid[0] = '\0';
    snprintf(g_ipText, sizeof(g_ipText), "--");
    WiFi.disconnect(false, false);
    beginAsyncScan();
}

bool FT02_WifiManagerConnectNetwork(uint8_t index, const char* password)
{
    if(index >= g_networkCount) return false;
    return beginConnect(g_networks[index].ssid, password, false);
}

bool FT02_WifiManagerPoll(uint32_t nowMs)
{
    bool changed = false;

    if(g_state == FT02_WIFI_STATE_SCANNING)
    {
        const int scanStatus = WiFi.scanComplete();
        if(scanStatus >= 0 || scanStatus == WIFI_SCAN_FAILED)
        {
            changed = finishScan(scanStatus);
        }
    }
    else if(g_state == FT02_WIFI_STATE_CONNECTING || g_state == FT02_WIFI_STATE_CONNECTING_SAVED)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            changed = finishConnect(true);
        }
        else if(nowMs - g_connectStartMs >= FT02_WIFI_CONNECT_TIMEOUT_MS)
        {
            changed = finishConnect(false);
        }
    }
    else if((g_state == FT02_WIFI_STATE_CONNECTED ||
             g_state == FT02_WIFI_STATE_CORE_DISCOVERY ||
             g_state == FT02_WIFI_STATE_CORE_CONNECTED ||
             g_state == FT02_WIFI_STATE_PORTAL) &&
            WiFi.status() != WL_CONNECTED)
    {
        setStatus("连接已断开");
        g_connectedSsid[0] = '\0';
        snprintf(g_ipText, sizeof(g_ipText), "--");
        g_state = g_networkCount > 0 ? FT02_WIFI_STATE_SELECTING : FT02_WIFI_STATE_ERROR;
        changed = true;
    }

    return changed;
}

FT02WifiOwner FT02_WifiManagerOwner() { return g_owner; }
FT02WifiState FT02_WifiManagerState() { return g_state; }
const char* FT02_WifiManagerStatusText() { return g_statusText; }
const char* FT02_WifiManagerConnectedSsid() { return g_connectedSsid; }
const char* FT02_WifiManagerIpText() { return g_ipText; }
uint8_t FT02_WifiManagerNetworkCount() { return g_networkCount; }

bool FT02_WifiManagerGetNetwork(uint8_t index, FT02WifiNetwork& network)
{
    if(index >= g_networkCount) return false;
    network = g_networks[index];
    return true;
}

const char* FT02_WifiAuthText(wifi_auth_mode_t auth)
{
    switch(auth)
    {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "UNKNOWN";
    }
}

bool FT02_WifiManagerIsConnected()
{
    return g_state == FT02_WIFI_STATE_CONNECTED ||
           g_state == FT02_WIFI_STATE_CORE_DISCOVERY ||
           g_state == FT02_WIFI_STATE_CORE_CONNECTED ||
           g_state == FT02_WIFI_STATE_PORTAL;
}

bool FT02_WifiManagerIsSelectableList()
{
    return g_state == FT02_WIFI_STATE_SELECTING && g_networkCount > 0;
}
