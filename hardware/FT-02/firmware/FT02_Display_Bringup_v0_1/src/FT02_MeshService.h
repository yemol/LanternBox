#pragma once

#include <stddef.h>
#include <stdint.h>

bool FT02_MeshServiceBegin();
void FT02_MeshServicePoll();
void FT02_MeshServiceOnRadioFrame(const uint8_t *frame,
                                  size_t len,
                                  float rssi,
                                  float snr,
                                  bool crcOk,
                                  bool headerOk);
uint32_t FT02_MeshServiceRawFrameCount();
uint32_t FT02_MeshServiceDecodedFrameCount();

bool FT02_MeshServiceSendBroadcastText(const char* text, uint32_t packetId);
bool FT02_MeshServiceCanSendPrivate(uint32_t destination);
bool FT02_MeshServiceSendPrivateText(uint32_t destination, const char* text, uint32_t packetId);
bool FT02_MeshServiceRequestNodeInfo();
bool FT02_MeshServicePkiReady();
uint8_t FT02_MeshServicePkiPeerCount();

bool FT02_MeshServiceReady();
bool FT02_MeshServiceSleepAvailable();
void FT02_MeshServiceSleepEnter();
void FT02_MeshServiceSleepService();
void FT02_MeshServiceSleepExit();
