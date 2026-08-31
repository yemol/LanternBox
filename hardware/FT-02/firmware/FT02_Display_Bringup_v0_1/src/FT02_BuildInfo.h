#pragma once

// FT-02 single source of truth for firmware identity.
// Update only this file when creating a new firmware package.
#define FT02_FIRMWARE_VERSION "v2.75k57"
#define FT02_FIRMWARE_VERSION_LABEL "版本：" FT02_FIRMWARE_VERSION
#define FT02_FIRMWARE_FEATURE_LABEL "Post-Refresh 350ms A19 + Volume Base-Half Same-Steps A18 + Volume Safe-Refresh A17 + Reader Clean-Slate + Input Stable-Release A16 + ES8311 Static Total-Duration Playback + Pop-Safe Stop + PSRAM Clean Playback A12 + EPD-Settle + Burst Repair + PA-SD + Record Timer + S3 512x Clock + Speaker 2/3 + Field Library v0.8.2 Fast UI Partial + ELE-P0-01 Runtime + Reading Comfort + Fit-Page Figures + Persistent Resume + Hot Fast Reader + Sharp ScanPDF + Global Input FIFO A2 + FieldManual Runtime A3.15 Split Text Font Sync + LR01 Single Authority"
#define FT02_FIRMWARE_BUILD_LABEL FT02_FIRMWARE_VERSION " / " FT02_FIRMWARE_FEATURE_LABEL
