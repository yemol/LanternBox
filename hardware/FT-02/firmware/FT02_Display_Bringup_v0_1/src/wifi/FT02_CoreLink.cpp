#include "FT02_CoreLink.h"

#include <WiFi.h>
#include <stdio.h>
#include <string.h>

#include "FT02_Storage.h"

namespace
{
static const char* FT02_CORE_LINK_ADDRESS_PATH = "/wifi/core_address.txt";
static const char* FT02_CORE_LINK_ADDRESS_TEMP = "/wifi/core_address.tmp";

static FT02CoreLinkState g_state = FT02_CORE_LINK_IDLE;
static uint32_t g_discoveryStartMs = 0;
static char g_status[80] = "Core地址未设置";
static char g_address[FT02_CORE_LINK_ADDRESS_LEN] = "--";
static char g_configuredAddress[FT02_CORE_LINK_ADDRESS_LEN] = "";
static char g_apiVersion[16] = "--";
static char g_coreVersion[24] = "--";

static bool isAddressChar(char c)
{
    const uint8_t code = static_cast<uint8_t>(c);
    return code >= 0x21 && code <= 0x7E;
}

static void sanitizeAddress(const char* input, char* output, size_t outputSize)
{
    if(output == nullptr || outputSize == 0) return;
    output[0] = '\0';
    if(input == nullptr) return;

    size_t out = 0;
    while(*input == ' ' || *input == '\t' || *input == '\r' || *input == '\n') ++input;
    for(const char* cursor = input; *cursor != '\0' && out + 1 < outputSize; ++cursor)
    {
        if(*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') break;
        if(!isAddressChar(*cursor)) continue;
        output[out++] = *cursor;
    }
    output[out] = '\0';
}

static bool loadConfiguredAddress()
{
    g_configuredAddress[0] = '\0';
    if(!FT02_StorageIsReady()) return false;

    FILE* file = FT02_StorageOpenReadFile(FT02_CORE_LINK_ADDRESS_PATH);
    if(file == nullptr) return false;

    char buffer[FT02_CORE_LINK_ADDRESS_LEN] = {};
    const size_t read = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[read] = '\0';
    sanitizeAddress(buffer, g_configuredAddress, sizeof(g_configuredAddress));
    return g_configuredAddress[0] != '\0';
}

static bool saveConfiguredAddress()
{
    if(!FT02_StorageIsReady()) return false;

    (void)FT02_StorageDeleteFile(FT02_CORE_LINK_ADDRESS_TEMP);
    FILE* file = FT02_StorageOpenWriteFile(FT02_CORE_LINK_ADDRESS_TEMP, true);
    if(file == nullptr) return false;

    bool ok = true;
    if(g_configuredAddress[0] != '\0')
    {
        ok = fwrite(g_configuredAddress, 1, strlen(g_configuredAddress), file) == strlen(g_configuredAddress);
        if(ok) ok = fwrite("\n", 1, 1, file) == 1;
    }
    if(ok) ok = FT02_StorageSyncFile(file);
    fclose(file);

    if(!ok)
    {
        (void)FT02_StorageDeleteFile(FT02_CORE_LINK_ADDRESS_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_CORE_LINK_ADDRESS_PATH);
    if(!FT02_StorageRenameFile(FT02_CORE_LINK_ADDRESS_TEMP, FT02_CORE_LINK_ADDRESS_PATH))
    {
        (void)FT02_StorageDeleteFile(FT02_CORE_LINK_ADDRESS_TEMP);
        return false;
    }
    return true;
}
}

void FT02_CoreLinkBegin()
{
    loadConfiguredAddress();
    FT02_CoreLinkReset();
}

void FT02_CoreLinkReset()
{
    g_state = FT02_CORE_LINK_IDLE;
    g_discoveryStartMs = 0;
    snprintf(g_status, sizeof(g_status), "%s", g_configuredAddress[0] != '\0' ? "Core地址已保存" : "Core地址未设置");
    snprintf(g_address, sizeof(g_address), "--");
    snprintf(g_apiVersion, sizeof(g_apiVersion), "--");
    snprintf(g_coreVersion, sizeof(g_coreVersion), "--");
}

void FT02_CoreLinkStartDiscovery()
{
    loadConfiguredAddress();
    if(WiFi.status() != WL_CONNECTED)
    {
        FT02_CoreLinkReset();
        g_state = FT02_CORE_LINK_ERROR;
        snprintf(g_status, sizeof(g_status), "WiFi未连接，无法连接Core");
        return;
    }

    if(g_configuredAddress[0] == '\0')
    {
        FT02_CoreLinkReset();
        g_state = FT02_CORE_LINK_ERROR;
        snprintf(g_status, sizeof(g_status), "Core地址未设置");
        return;
    }

    g_state = FT02_CORE_LINK_DISCOVERING;
    g_discoveryStartMs = millis();
    snprintf(g_address, sizeof(g_address), "%s", g_configuredAddress);
    snprintf(g_status, sizeof(g_status), "连接Core %s", g_address);
}

bool FT02_CoreLinkPoll(uint32_t nowMs)
{
    if(g_state != FT02_CORE_LINK_DISCOVERING) return false;
    if(nowMs - g_discoveryStartMs < 600u) return false;

    g_state = FT02_CORE_LINK_READY;
    snprintf(g_status, sizeof(g_status), "Core地址: %s", g_address);
    snprintf(g_apiVersion, sizeof(g_apiVersion), "pending");
    snprintf(g_coreVersion, sizeof(g_coreVersion), "pending");
    return true;
}

FT02CoreLinkState FT02_CoreLinkStateCurrent() { return g_state; }
const char* FT02_CoreLinkStatusText() { return g_status; }
const char* FT02_CoreLinkAddressText() { return g_address; }
const char* FT02_CoreLinkConfiguredAddressText() { return g_configuredAddress[0] != '\0' ? g_configuredAddress : "--"; }
bool FT02_CoreLinkHasConfiguredAddress() { return g_configuredAddress[0] != '\0'; }

bool FT02_CoreLinkSetConfiguredAddress(const char* address)
{
    char sanitized[FT02_CORE_LINK_ADDRESS_LEN] = {};
    sanitizeAddress(address, sanitized, sizeof(sanitized));
    snprintf(g_configuredAddress, sizeof(g_configuredAddress), "%s", sanitized);
    snprintf(g_status, sizeof(g_status), "%s", g_configuredAddress[0] != '\0' ? "Core地址已保存" : "Core地址已清空");
    if(g_configuredAddress[0] == '\0') snprintf(g_address, sizeof(g_address), "--");
    return saveConfiguredAddress();
}
