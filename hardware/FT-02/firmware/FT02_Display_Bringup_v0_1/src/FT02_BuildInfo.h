#pragma once

// FT-02 single source of truth for firmware identity.
// Mesh Messaging A1.x is developed as a feature branch on the frozen A5C R1.5
// firmware identity so the existing hardware regression gates remain valid.
#define FT02_FIRMWARE_VERSION "v2.84-sleep-power-a5c-stable-r1.5"
#define FT02_FIRMWARE_VERSION_LABEL "版本：" FT02_FIRMWARE_VERSION
#define FT02_FIRMWARE_UI_VERSION "v2.84 A5C R1.5"
#define FT02_FIRMWARE_FEATURE_LABEL "SLEEP POWER A5C STABLE R1.5 / SLEEP HEALTH / SD QUIESCE / GNSS STANDBY / SCL2PHASE RC3 EXT1"
#define FT02_FIRMWARE_BUILD_LABEL FT02_FIRMWARE_VERSION " / " FT02_FIRMWARE_FEATURE_LABEL

#define FT02_MESH_MESSAGING_VERSION "A1.5.15e"
