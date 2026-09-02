#pragma once

#include <Arduino.h>
#include <stdint.h>

static constexpr uint8_t FT02_WIFI_PROFILE_MAX_COUNT = 8;
static constexpr size_t FT02_WIFI_PROFILE_SSID_LEN = 33;
static constexpr size_t FT02_WIFI_PROFILE_PASSWORD_LEN = 65;

struct FT02WifiProfile
{
    char ssid[FT02_WIFI_PROFILE_SSID_LEN];
    char password[FT02_WIFI_PROFILE_PASSWORD_LEN];
    int16_t priority;
    uint32_t lastConnected;
    uint32_t lastSuccess;
    bool autoConnect;
};

void FT02_WifiProfileStoreBegin();
bool FT02_WifiProfileStoreLoad();
bool FT02_WifiProfileStoreSave();
bool FT02_WifiProfileStoreIsLoaded();
uint8_t FT02_WifiProfileStoreCount();
bool FT02_WifiProfileStoreGet(uint8_t index, FT02WifiProfile& profile);
int FT02_WifiProfileStoreFind(const char* ssid);
bool FT02_WifiProfileStoreUpsert(const char* ssid, const char* password, bool connectedNow);
bool FT02_WifiProfileStoreDelete(const char* ssid);
void FT02_WifiProfileStoreSort();
