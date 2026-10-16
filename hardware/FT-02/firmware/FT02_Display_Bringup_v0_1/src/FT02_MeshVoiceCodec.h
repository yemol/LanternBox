#pragma once

#include <stdint.h>

// Offline conversion used by asynchronous Mesh voice messages. Capture stays
// on the proven ES8311 44.1 kHz path, then this module converts to/from
// Opus VOIP 6 kbps / 16 kHz TX; legacy GSM-FR and Codec2-3200/1600/1300
// remain decodable after capture / before playback.
bool FT02_MeshVoiceEncodeWavToLbv(const char* wavPath,
                                  const char* lbvPath,
                                  uint32_t trimLeadMs,
                                  uint32_t& durationMs,
                                  uint32_t& encodedBytes);
bool FT02_MeshVoiceDecodeLbvToWav(const char* lbvPath,
                                  const char* wavPath,
                                  uint32_t& durationMs);

// RTC-retained crash breadcrumbs for offline speech-codec conversion.
void FT02_MeshVoiceCodecReportResetTrace();
void FT02_MeshVoiceCodecClearResetTrace();
