#include "FT02_CompassCore.h"
#include "FT02_Log.h"

#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "CompassCalibration.h"
#include "FT02_I2CBus.h"

namespace
{
constexpr uint8_t QMC_ADDR = 0x0D;
constexpr uint32_t POLL_MS = 100u;
constexpr uint32_t RETRY_MS = 3000u;
constexpr uint32_t STALE_MS = 3000u;
constexpr uint32_t RESUME_VERIFY_TIMEOUT_MS = 120u;
constexpr uint8_t RESUME_DISCARD_SAMPLES = 2u;
constexpr uint8_t QMC_STATUS_DRDY = 0x01u;
constexpr uint8_t QMC_STATUS_OVL = 0x02u;
constexpr float INSTALL_YAW_OFFSET_DEG = 90.0f;

CompassCalibration g_cal;
FT02CoreCompassSnapshot g_state = {};
uint32_t g_lastPollMs = 0u;
uint32_t g_lastProbeMs = 0u;
uint32_t g_lastGoodMs = 0u;
bool g_calStoreOk = false;
bool g_sleepSuspended = false;

void changed() { ++g_state.uiGeneration; }

bool qmcProbe()
{
    if(!FT02_I2CBusLock(50)) return false;
    Wire.beginTransmission(QMC_ADDR);
    const bool ok = Wire.endTransmission() == 0;
    FT02_I2CBusUnlock();
    return ok;
}

bool qmcWrite(uint8_t reg, uint8_t value)
{
    if(!FT02_I2CBusLock(50)) return false;
    Wire.beginTransmission(QMC_ADDR);
    Wire.write(reg);
    Wire.write(value);
    const bool ok = Wire.endTransmission() == 0;
    FT02_I2CBusUnlock();
    return ok;
}

enum class QmcReadResult : uint8_t
{
    OK = 0,
    NOT_READY,
    OVERFLOW,
    IO_ERROR
};

QmcReadResult qmcReadFreshRaw(int16_t &x, int16_t &y, int16_t &z)
{
    if(!FT02_I2CBusLock(30)) return QmcReadResult::IO_ERROR;

    // QMC5883L retains output registers while in standby. Reading 00H..05H
    // successfully therefore does NOT prove that the measurement engine has
    // resumed. Gate every accepted sample on STATUS.DRDY so stale pre-sleep
    // output can never become a valid post-wake heading.
    Wire.beginTransmission(QMC_ADDR);
    Wire.write(0x06);
    if(Wire.endTransmission(false) != 0 || Wire.requestFrom((int)QMC_ADDR, 1) != 1)
    {
        FT02_I2CBusUnlock();
        return QmcReadResult::IO_ERROR;
    }
    const uint8_t status = Wire.read();
    if((status & QMC_STATUS_OVL) != 0u)
    {
        FT02_I2CBusUnlock();
        return QmcReadResult::OVERFLOW;
    }
    if((status & QMC_STATUS_DRDY) == 0u)
    {
        FT02_I2CBusUnlock();
        return QmcReadResult::NOT_READY;
    }

    Wire.beginTransmission(QMC_ADDR);
    Wire.write(0x00);
    if(Wire.endTransmission(false) != 0)
    {
        FT02_I2CBusUnlock();
        return QmcReadResult::IO_ERROR;
    }
    if(Wire.requestFrom((int)QMC_ADDR, 6) != 6)
    {
        FT02_I2CBusUnlock();
        return QmcReadResult::IO_ERROR;
    }
    const uint8_t xl = Wire.read(), xh = Wire.read();
    const uint8_t yl = Wire.read(), yh = Wire.read();
    const uint8_t zl = Wire.read(), zh = Wire.read();
    FT02_I2CBusUnlock();

    x = (int16_t)((xh << 8) | xl);
    y = (int16_t)((yh << 8) | yl);
    z = (int16_t)((zh << 8) | zl);
    return QmcReadResult::OK;
}

bool qmcWaitFreshSamples(uint8_t samples, uint32_t timeoutMs)
{
    const uint32_t started = millis();
    uint8_t accepted = 0u;
    while(static_cast<uint32_t>(millis() - started) < timeoutMs)
    {
        int16_t x=0, y=0, z=0;
        const QmcReadResult result = qmcReadFreshRaw(x, y, z);
        if(result == QmcReadResult::OK)
        {
            if(++accepted >= samples) return true;
        }
        else if(result == QmcReadResult::IO_ERROR)
        {
            return false;
        }
        delay(5);
    }
    return false;
}

bool qmcBeginOnce()
{
    if(!qmcProbe()) return false;

    // A wake is treated as a real sensor re-bring-up, not merely a register
    // write. Soft reset clears any half-resumed internal state, then the
    // documented set/reset period and continuous profile are restored. Host
    // calibration remains untouched because it lives in RAM/NVS.
    if(!qmcWrite(0x0A, 0x80)) return false;
    delay(10);
    if(!qmcWrite(0x0B, 0x01)) return false;
    delay(5);
    if(!qmcWrite(0x09, 0x1D)) return false;

    // Verify that the measurement engine is producing NEW data. Discard the
    // first samples after reset so the UI never publishes a stale/transition
    // sample as the post-wake heading.
    return qmcWaitFreshSamples(RESUME_DISCARD_SAMPLES, RESUME_VERIFY_TIMEOUT_MS);
}

FT02CoreCompassCalState mapCalState(CompassCalibration::State s)
{
    switch(s)
    {
        case CompassCalibration::State::IDLE: return FT02_CORE_COMPASS_CAL_IDLE;
        case CompassCalibration::State::RUNNING: return FT02_CORE_COMPASS_CAL_RUNNING;
        case CompassCalibration::State::READY: return FT02_CORE_COMPASS_CAL_READY;
        case CompassCalibration::State::SAVED: return FT02_CORE_COMPASS_CAL_SAVED;
        case CompassCalibration::State::CANCELED: return FT02_CORE_COMPASS_CAL_CANCELED;
        case CompassCalibration::State::FAILED: return FT02_CORE_COMPASS_CAL_FAILED;
        default: return FT02_CORE_COMPASS_CAL_UNKNOWN;
    }
}

void syncCalibration(bool bump)
{
    FT02CoreCompassCalibrationState &c = g_state.calibration;
    const FT02CoreCompassCalState st = mapCalState(g_cal.state());
    const uint8_t q = g_cal.isSessionActive() ? g_cal.sessionQuality() : g_cal.savedQuality();
    bool any = false;
#define SET_CAL(field, value) do { auto _v=(value); if(c.field != _v) { c.field=_v; any=true; } } while(0)
    SET_CAL(state, st);
    SET_CAL(progress, g_cal.progress());
    SET_CAL(quality, q);
    SET_CAL(samples, g_cal.samples());
    SET_CAL(calibrated, g_cal.isCalibrated());
    SET_CAL(minX, g_cal.minX());
    SET_CAL(maxX, g_cal.maxX());
    SET_CAL(minY, g_cal.minY());
    SET_CAL(maxY, g_cal.maxY());
    SET_CAL(minZ, g_cal.minZ());
    SET_CAL(maxZ, g_cal.maxZ());
#undef SET_CAL
    if(any || bump) ++c.revision;
}

bool computeHeading(int16_t rawX, int16_t rawY, int16_t rawZ,
                    uint16_t &headingX10, uint8_t &quality)
{
    float x=0.0f, y=0.0f, z=0.0f;
    g_cal.apply((float)rawX, (float)rawY, (float)rawZ, x, y, z);

    float h = atan2f(-y, x) * 180.0f / PI;
    if(h < 0.0f) h += 360.0f;
    h += INSTALL_YAW_OFFSET_DEG;
    while(h >= 360.0f) h -= 360.0f;
    while(h < 0.0f) h += 360.0f;
    headingX10 = (uint16_t)lroundf(h * 10.0f);
    if(headingX10 >= 3600u) headingX10 -= 3600u;

    const float mag = sqrtf(x*x + y*y + z*z);
    if(!isfinite(mag) || mag < 50.0f || mag > 20000.0f)
    {
        quality = 0;
        return true;
    }
    quality = g_cal.isCalibrated() ? g_cal.savedQuality() : 1u;
    return true;
}

void setError(int code, const char *msg)
{
    g_state.calibration.lastErrorCode = code;
    snprintf(g_state.calibration.lastErrorMessage,
             sizeof(g_state.calibration.lastErrorMessage), "%s", msg ? msg : "");
    ++g_state.calibration.revision;
}

void clearError()
{
    if(g_state.calibration.lastErrorCode == 0 && g_state.calibration.lastErrorMessage[0] == '\0') return;
    g_state.calibration.lastErrorCode = 0;
    g_state.calibration.lastErrorMessage[0] = '\0';
    ++g_state.calibration.revision;
}
}

void FT02_CompassCoreBegin()
{
    const uint32_t gen = g_state.uiGeneration;
    memset(&g_state, 0, sizeof(g_state));
    g_state.uiGeneration = gen + 1u;
    g_calStoreOk = g_cal.begin();
    syncCalibration(true);
    g_lastPollMs = 0u;
    g_lastProbeMs = millis();
    g_lastGoodMs = 0u;
    g_state.hardwareKnown = true;
    g_state.ready = qmcBeginOnce();
    if(g_state.ready)
    {
        FT02_LOGI("[COMPASS] ready addr=0x%02X cal=%u store=%u\n",
                      QMC_ADDR, g_cal.isCalibrated()?1u:0u, g_calStoreOk?1u:0u);
    }
    else
    {
        FT02_LOGW("[WARN][COMPASS] not detected addr=0x%02X; retrying\n", QMC_ADDR);
    }
}

void FT02_CompassCorePoll()
{
    if(g_sleepSuspended) return;
    const uint32_t now = millis();
    if(!g_state.ready)
    {
        if(static_cast<uint32_t>(now - g_lastProbeMs) < RETRY_MS) return;
        g_lastProbeMs = now;
        if(!qmcBeginOnce()) return;
        g_state.ready = true;
        changed();
        FT02_LOGD("[COMPASS] restored addr=0x%02X cal=%u\n",
                      QMC_ADDR, g_cal.isCalibrated()?1u:0u);
    }

    if(static_cast<uint32_t>(now - g_lastPollMs) < POLL_MS) return;
    g_lastPollMs = now;

    int16_t x=0, y=0, z=0;
    const QmcReadResult readResult = qmcReadFreshRaw(x,y,z);
    if(readResult != QmcReadResult::OK)
    {
        // NOT_READY is normal for a short interval, especially directly after
        // resume. It becomes a lifecycle fault only if fresh samples do not
        // return within STALE_MS. IO/overflow follow the same recovery path
        // without ever publishing stale output as a valid heading.
        const uint32_t freshnessBase = g_lastGoodMs != 0u ? g_lastGoodMs : g_lastProbeMs;
        if(static_cast<uint32_t>(now - freshnessBase) >= STALE_MS)
        {
            if(g_state.ready || g_state.valid)
            {
                g_state.ready = false;
                g_state.valid = false;
                g_state.quality = 0;
                changed();
                FT02_LOGW("[WARN][COMPASS] fresh stream lost result=%u; reprobe\n",
                          static_cast<unsigned>(readResult));
            }
        }
        return;
    }

    g_lastGoodMs = now;
    g_state.ready = true;
    if(g_cal.isSessionActive()) g_cal.addSample(x,y,z);

    uint16_t h=0;
    uint8_t q=0;
    const bool valid = computeHeading(x,y,z,h,q);
    bool any = false;
#define SET_STATE(field, value) do { auto _v=(value); if(g_state.field != _v) { g_state.field=_v; any=true; } } while(0)
    SET_STATE(rawX, x);
    SET_STATE(rawY, y);
    SET_STATE(rawZ, z);
    SET_STATE(headingX10, h);
    SET_STATE(quality, q);
    SET_STATE(valid, valid);
#undef SET_STATE
    g_state.lastUpdateMs = now;
    syncCalibration(g_cal.isSessionActive());
    if(any) changed();
}

FT02CoreCompassSnapshot FT02_CompassCoreSnapshotCurrent()
{
    return g_state;
}

bool FT02_CompassCoreCalStart()
{
    clearError();
    if(!g_state.ready) { setError(25, "compass_not_ready"); return false; }
    const bool ok = g_cal.start();
    if(!ok) setError(20, "compass_cal_busy");
    syncCalibration(true);
    return ok;
}

bool FT02_CompassCoreCalSave()
{
    clearError();
    if(!g_cal.isSessionActive()) { setError(21, "compass_cal_not_running"); return false; }
    if(g_cal.sessionQuality() < 2u) { setError(22, "compass_cal_quality_low"); syncCalibration(true); return false; }
    const bool ok = g_cal.save();
    if(!ok) setError(23, "compass_cal_save_failed");
    syncCalibration(true);
    return ok;
}

bool FT02_CompassCoreCalCancel()
{
    clearError();
    const bool ok = g_cal.cancel();
    if(!ok) setError(21, "compass_cal_not_running");
    syncCalibration(true);
    return ok;
}

bool FT02_CompassCoreCalReset()
{
    clearError();
    if(g_cal.isSessionActive()) { setError(20, "compass_cal_busy"); return false; }
    const bool ok = g_cal.reset();
    if(!ok) setError(24, "compass_cal_reset_failed");
    syncCalibration(true);
    return ok;
}

const char* FT02_CompassCoreCalStateText(FT02CoreCompassCalState state)
{
    switch(state)
    {
        case FT02_CORE_COMPASS_CAL_IDLE: return "IDLE";
        case FT02_CORE_COMPASS_CAL_RUNNING: return "RUNNING";
        case FT02_CORE_COMPASS_CAL_READY: return "READY";
        case FT02_CORE_COMPASS_CAL_SAVED: return "SAVED";
        case FT02_CORE_COMPASS_CAL_CANCELED: return "CANCELED";
        case FT02_CORE_COMPASS_CAL_FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}


bool FT02_CompassCoreSleepEnter()
{
    if(g_sleepSuspended) return true;
    if(g_cal.isSessionActive()) return false;

    // QMC5883L control register 0x09 = 0x00 enters standby. Calibration data
    // lives in host memory/NVS and is not affected by this sensor power state.
    const bool ok = qmcWrite(0x09, 0x00);
    if(ok)
    {
        g_sleepSuspended = true;
        g_state.valid = false;
        g_state.quality = 0u;
        changed();
        FT02_LOGD_PRINTLN("[COMPASS-SLEEP] enter standby=1");
    }
    else
    {
        FT02_LOGE_PRINTLN("[ERROR][COMPASS] sleep standby failed");
    }
    return ok;
}

bool FT02_CompassCoreSleepExit()
{
    if(!g_sleepSuspended) return true;

    const bool ok = qmcBeginOnce();
    g_sleepSuspended = false;
    g_lastPollMs = 0u;
    g_lastProbeMs = millis();
    g_lastGoodMs = 0u;
    g_state.ready = ok;
    g_state.valid = false;
    g_state.quality = 0u;
    changed();

    if(ok) FT02_LOGD_PRINTLN("[COMPASS] restored continuous=200Hz fresh=verified");
    else FT02_LOGW_PRINTLN("[WARN][COMPASS] wake restore pending reprobe=1");
    return ok;
}
