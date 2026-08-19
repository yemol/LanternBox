#pragma once

#include <stddef.h>

// Shared firmware timing constants.
static constexpr unsigned long FT_AUTO_TRACK_INTERVAL_MS = 30000UL;

// A GNSS fix is only attached to an outgoing message while it is fresh.
// This prevents a stale position from being silently sent after signal loss.
static constexpr unsigned long FT_GNSS_MESSAGE_MAX_AGE_MS = 15000UL;

// The editor remains capped at 120 bytes. The over-the-air allowance is larger
// so the firmware can append a compact GPS footer without truncating the text.
static constexpr size_t FT_MESH_USER_TEXT_MAX_BYTES = 120;
static constexpr size_t FT_MESH_TEXT_PAYLOAD_MAX_BYTES = 160;
