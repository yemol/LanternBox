#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <stdint.h>

#include "FT02_WifiProfileStore.h"

static constexpr uint8_t FT02_WIFI_MAX_NETWORKS = 12;

struct FT02WifiNetwork
{
    char ssid[FT02_WIFI_PROFILE_SSID_LEN];
    int32_t rssi;
    wifi_auth_mode_t auth;
};

enum FT02WifiOwner : uint8_t
{
    FT02_WIFI_OWNER_NONE = 0,
    FT02_WIFI_OWNER_WIFI_UI,
    FT02_WIFI_OWNER_SYNC
};

enum FT02WifiState : uint8_t
{
    FT02_WIFI_STATE_OFF = 0,
    FT02_WIFI_STATE_STARTING,
    FT02_WIFI_STATE_CONNECTING_SAVED,
    FT02_WIFI_STATE_SCANNING,
    FT02_WIFI_STATE_SELECTING,
    FT02_WIFI_STATE_CONNECTING,
    FT02_WIFI_STATE_CONNECTED,
    FT02_WIFI_STATE_CORE_DISCOVERY,
    FT02_WIFI_STATE_CORE_CONNECTED,
    FT02_WIFI_STATE_PORTAL,
    FT02_WIFI_STATE_ERROR,
    FT02_WIFI_STATE_SHUTDOWN
};

void FT02_WifiManagerBegin();
bool FT02_WifiManagerRequest(FT02WifiOwner owner);
void FT02_WifiManagerRelease(FT02WifiOwner owner);
void FT02_WifiManagerShutdown(FT02WifiOwner owner);
void FT02_WifiManagerStartUiSession();
void FT02_WifiManagerStartFreshScan();
bool FT02_WifiManagerConnectNetwork(uint8_t index, const char* password);
bool FT02_WifiManagerPoll(uint32_t nowMs);

FT02WifiOwner FT02_WifiManagerOwner();
FT02WifiState FT02_WifiManagerState();
const char* FT02_WifiManagerStatusText();
const char* FT02_WifiManagerConnectedSsid();
const char* FT02_WifiManagerIpText();
uint8_t FT02_WifiManagerNetworkCount();
bool FT02_WifiManagerGetNetwork(uint8_t index, FT02WifiNetwork& network);
const char* FT02_WifiAuthText(wifi_auth_mode_t auth);
bool FT02_WifiManagerIsConnected();
bool FT02_WifiManagerIsSelectableList();
