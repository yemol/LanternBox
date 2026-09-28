#pragma once

#include <stddef.h>

// Routine diagnostic traces are disabled in the production build. Error, recovery,
// startup-health and explicit console-command output remain available.
static constexpr bool FT_VERBOSE_DIAGNOSTICS = false;

// Shared firmware timing constants.
static constexpr unsigned long FT_AUTO_TRACK_INTERVAL_MS = 30000UL;

// Compass body-frame transform. Four-point bench measurements on the installed
// FT-02 show the raw calibrated horizontal frame rotates opposite to the device
// heading. Correct handedness first in qmcComputeHeading(), then apply the
// measured device-front yaw offset.
static constexpr float FT_COMPASS_INSTALL_YAW_OFFSET_DEG = 90.0f;

// A GNSS fix is only attached to an outgoing message while it is fresh.
// This prevents a stale position from being silently sent after signal loss.
static constexpr unsigned long FT_GNSS_MESSAGE_MAX_AGE_MS = 15000UL;

// The editor remains capped at 120 bytes. The over-the-air allowance is larger
// so the firmware can append a compact GPS footer without truncating the text.
static constexpr size_t FT_MESH_USER_TEXT_MAX_BYTES = 120;
static constexpr size_t FT_MESH_TEXT_PAYLOAD_MAX_BYTES = 160;

// GNSS receive-path health and recovery.
// The K25+ can emit dense multi-constellation NMEA bursts at 115200 baud.
// Keep enough RX headroom so LoRa/compass/host work cannot starve the GNSS UART.
static constexpr size_t FT_GNSS_RX_BUFFER_BYTES = 4096;

// Freshness windows. Old GSV/GGA-derived values must never masquerade as live data.
static constexpr unsigned long FT_GNSS_STREAM_FRESH_MS = 3000UL;
static constexpr unsigned long FT_GNSS_FIELD_FRESH_MS = 5000UL;
static constexpr unsigned long FT_GNSS_GSV_FRESH_MS = 5000UL;

// Recovery is intentionally conservative: healthy NMEA with no fix is normally
// allowed to continue acquiring. We only restart the host-side GNSS receive path
// when the stream/parser is unhealthy, or after a prolonged outdoor-looking no-fix.
static constexpr unsigned long FT_GNSS_NO_NMEA_RECOVERY_MS = 10000UL;
static constexpr unsigned long FT_GNSS_STREAM_STALL_RECOVERY_MS = 8000UL;
static constexpr unsigned long FT_GNSS_RECOVERY_COOLDOWN_MS = 15000UL;
// Receiver-side acquisition watchdog. A deliberate cold restart is allowed only
// after sustained strong-sky evidence with no ephemeris/used-SV progress, followed
// by a long reacquisition lockout.
static constexpr uint8_t FT_GNSS_A4_STRONG_CNO_MAX = 30;       // dB-Hz
static constexpr uint8_t FT_GNSS_A4_MIN_CNO20_SV = 6;          // satellites >=20 dB-Hz
static constexpr uint8_t FT_GNSS_A4_MIN_VISIBLE_SV = 8;
static constexpr uint8_t FT_GNSS_A4_WEAK_CNO_MAX = 25;         // partial sky evidence
static constexpr uint8_t FT_GNSS_A4_WEAK_CNO20_SV = 3;
static constexpr uint8_t FT_GNSS_A4_WEAK_VISIBLE_SV = 6;
static constexpr unsigned long FT_GNSS_A4_EVIDENCE_TRIGGER_MS = 90000UL;    // weighted strong-sky evidence
static constexpr unsigned long FT_GNSS_A4_EVIDENCE_CAP_MS = 180000UL;
static constexpr unsigned long FT_GNSS_A4_NO_PROGRESS_MS = 120000UL;        // 2 min no eph/used progress
static constexpr unsigned long FT_GNSS_A4_POST_COLD_LOCKOUT_MS = 480000UL;  // 8 min undisturbed acquire
static constexpr unsigned long FT_GNSS_A4_COLD_RETRY_MS = 1800000UL;        // max one auto cold / 30 min
static constexpr unsigned long FT_GNSS_RECEIVER_RESET_GRACE_MS = 10000UL;


// Persistent GNSS black-box recorder. Detailed sampling is used while acquiring
// a fix and briefly after transitions; stable-fix logging is intentionally slow
// to limit flash wear. Critical transitions/recoveries are flushed immediately.
static constexpr unsigned long FT_GNSS_LOG_ACQUIRE_SAMPLE_MS = 15000UL;
static constexpr unsigned long FT_GNSS_LOG_STABLE_SAMPLE_MS = 300000UL;
static constexpr unsigned long FT_GNSS_LOG_TRANSITION_DETAIL_MS = 120000UL;
static constexpr unsigned long FT_GNSS_LOG_ACQUIRE_FLUSH_MS = 60000UL;
static constexpr unsigned long FT_GNSS_LOG_STABLE_FLUSH_MS = 900000UL;
static constexpr unsigned long FT_GNSS_LOG_HIGH_RATE_NOFIX_MS = 1800000UL;
static constexpr unsigned long FT_GNSS_LOG_LONG_NOFIX_SAMPLE_MS = 60000UL;
static constexpr unsigned long FT_GNSS_LOG_LONG_NOFIX_FLUSH_MS = 600000UL;
