#pragma once

#include <stddef.h>
#include <stdint.h>

struct FT02MeshRadioFrame {
    uint8_t data[255];
    size_t len;
    float rssi;
    float snr;
    float frequencyErrorHz;
    int16_t radioState;
    bool crcOk;
};

bool FT02_MeshRadioBackendBegin();
void FT02_MeshRadioBackendPoll();
bool FT02_MeshRadioBackendTakeFrame(FT02MeshRadioFrame& out);

// Synchronous raw Meshtastic frame TX. Broadcast-only policy is enforced by MeshService.
bool FT02_MeshRadioBackendTransmit(const uint8_t* data, size_t len);
bool FT02_MeshRadioBackendReady();
uint32_t FT02_MeshRadioBackendRxCount();
uint32_t FT02_MeshRadioBackendCrcErrorCount();
int16_t FT02_MeshRadioBackendLastState();

// Compatibility hooks used only by the separately gated LoRa light-sleep trial.
bool FT02_MeshRadioBackendSleepAvailable();
bool FT02_MeshRadioBackendSleepEnter();
void FT02_MeshRadioBackendSleepService();
void FT02_MeshRadioBackendSleepExit();
