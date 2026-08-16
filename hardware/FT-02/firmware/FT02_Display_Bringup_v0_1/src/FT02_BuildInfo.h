#pragma once

// FT-02 single source of truth for firmware identity.
// Update only this file when creating a new firmware package.
#define FT02_FIRMWARE_VERSION "v2.74g"
#define FT02_FIRMWARE_VERSION_LABEL "版本：" FT02_FIRMWARE_VERSION
#define FT02_FIRMWARE_FEATURE_LABEL "Production + Reliable Messaging Runtime A1 + Persistent Outbox + Meshtastic Complete Communication Runtime + Offline Pinyin IME A4 + System Self-Test A1"
#define FT02_FIRMWARE_BUILD_LABEL FT02_FIRMWARE_VERSION " " FT02_FIRMWARE_FEATURE_LABEL
