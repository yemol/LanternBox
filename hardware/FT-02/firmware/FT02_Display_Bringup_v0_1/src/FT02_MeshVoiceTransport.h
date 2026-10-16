#pragma once

#include <stddef.h>
#include <stdint.h>

// Store-and-forward LanternBox voice transport. Audio capture/codec creation
// is a separate layer; this module owns Mesh fragmentation, reassembly,
// completion ACK/NACK and SD/history integration.
void FT02_MeshVoiceTransportBegin();
void FT02_MeshVoiceTransportPoll();

void FT02_MeshVoiceTransportOnPayload(uint32_t packetId,
                                      uint32_t from,
                                      uint32_t to,
                                      bool pkiEncrypted,
                                      float rssi,
                                      float snr,
                                      const uint8_t* payload,
                                      size_t payloadLen);

// Queue an already-created .lbv file for transmission. destination must be
// FT02_MESH_BROADCAST_NODE for broadcast or a PKI-capable peer for direct.
// logicalId=0 lets the transport allocate one; the resulting voiceId can be
// queried while active.
bool FT02_MeshVoiceTransportQueueFile(uint32_t destination,
                                      const char* voicePath,
                                      uint32_t logicalId = 0u);

enum FT02MeshVoiceTxOutcome : uint8_t
{
    FT02_MESH_VOICE_TX_NONE = 0,
    FT02_MESH_VOICE_TX_SENT,
    FT02_MESH_VOICE_TX_DELIVERED,
    FT02_MESH_VOICE_TX_FAILED,
    FT02_MESH_VOICE_TX_CANCELED
};

bool FT02_MeshVoiceTransportBusy();
uint32_t FT02_MeshVoiceTransportVoiceId();
uint8_t FT02_MeshVoiceTransportProgressPercent();
uint8_t FT02_MeshVoiceTransportChunksSent();
uint8_t FT02_MeshVoiceTransportTotalChunks();
uint32_t FT02_MeshVoiceTransportRevision();
bool FT02_MeshVoiceTransportAwaitingAck();
FT02MeshVoiceTxOutcome FT02_MeshVoiceTransportLastOutcome();
void FT02_MeshVoiceTransportCancel();
