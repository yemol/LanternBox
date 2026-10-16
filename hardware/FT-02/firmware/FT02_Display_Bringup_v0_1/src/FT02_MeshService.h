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
uint32_t FT02_MeshServiceNodeInfoRxCount();

bool FT02_MeshServiceSendBroadcastText(const char* text, uint32_t packetId);
bool FT02_MeshServiceCanSendPrivate(uint32_t destination);
bool FT02_MeshServiceSendPrivateText(uint32_t destination, const char* text, uint32_t packetId);

// Generic private-app transport used by LanternBox extensions such as voice.
// The port number must be a valid Meshtastic Data.portnum. Voice uses private
// application port 256; fragments intentionally avoid per-packet routing ACKs
// and use their own compact completion/NACK protocol.
uint32_t FT02_MeshServiceAllocatePacketId();
// Largest generic application payload that is guaranteed to fit the current
// private PKI air-frame envelope.  Callers with their own fragmentation layer
// (voice) should keep every wire packet at or below this value.
size_t FT02_MeshServiceMaxPrivateAppPayload();
bool FT02_MeshServiceSendBroadcastApp(uint32_t portnum,
                                       const uint8_t* payload,
                                       size_t payloadLen,
                                       uint32_t packetId);
bool FT02_MeshServiceSendPrivateApp(uint32_t destination,
                                     uint32_t portnum,
                                     const uint8_t* payload,
                                     size_t payloadLen,
                                     uint32_t packetId,
                                     bool wantAck = false);
bool FT02_MeshServiceRequestNodeInfo();
bool FT02_MeshServicePkiReady();
uint8_t FT02_MeshServicePkiPeerCount();

bool FT02_MeshServiceReady();
bool FT02_MeshServiceSleepAvailable();
bool FT02_MeshServiceSleepEnter();
void FT02_MeshServiceSleepService();
void FT02_MeshServiceSleepExit();
