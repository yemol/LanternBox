#include "FT02_LoRaHwProbe.h"
#include "FT02_Log.h"

#include <Arduino.h>
#include <Wire.h>
#include "FT02_I2CBus.h"
#include "FT02_LoRaHardwareSpi.h"

namespace {
// Wiring agreed for FT-02 Core + Waveshare Core1262-LF.
constexpr int RADIO_SCK = FT02_LoRaHardwareSpi::SCK;
constexpr int RADIO_MOSI = FT02_LoRaHardwareSpi::MOSI;
constexpr int RADIO_MISO = FT02_LoRaHardwareSpi::MISO;
constexpr int RADIO_NSS = FT02_LoRaHardwareSpi::NSS;
constexpr int RADIO_BUSY = FT02_LoRaHardwareSpi::BUSY;
constexpr int RADIO_DIO1 = FT02_LoRaHardwareSpi::DIO1;

// AW9523 P1_0: nRESET; P1_1: RXEN; P1_2: TXEN.
constexpr uint8_t AW_ID = 0x10;
constexpr uint8_t AW_OUTPUT_P1 = 0x03;
constexpr uint8_t AW_CONFIG_P1 = 0x05;
constexpr uint8_t AW_MODE_P1 = 0x13;
constexpr uint8_t AW_CHIP_ID = 0x23;
constexpr uint8_t AW_P1_CONTROL_MASK = 0x07;
constexpr uint8_t AW_RADIO_RESET_ASSERTED = 0x02; // Reset LOW, RXEN HIGH, TXEN LOW: RF2=RX (verified schematic)
constexpr uint8_t AW_RADIO_RF_IDLE = 0x01;        // Reset HIGH, RXEN LOW, TXEN LOW: front-end disabled for sleep
constexpr uint8_t AW_RADIO_RX_STANDBY = 0x03;     // Reset HIGH, RXEN HIGH, TXEN LOW: RF2=RX (verified schematic)
constexpr uint8_t AW_RADIO_TX_ACTIVE = 0x05;      // Reset HIGH, RXEN LOW, TXEN HIGH: RFC=TX path

bool g_awPresent = false;
bool g_radioPreflightReady = false;
uint8_t g_awAddress = 0;
uint8_t g_p1UnrelatedBits = 0;
uint32_t g_lastReportMs = 0;
bool g_radioOwned = false;
bool g_rfRxActive = false;
bool g_rfTxActive = false;

// Every complete AW9523 register transaction holds FT-02's shared I2C lock.
bool readAw(uint8_t addr, uint8_t reg, uint8_t &value) {
    if (!FT02_I2CBusLock(80)) return false;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    const bool sent = Wire.endTransmission(false) == 0;
    bool ok = false;
    if (sent && Wire.requestFrom(addr, static_cast<uint8_t>(1)) == 1 && Wire.available()) {
        value = static_cast<uint8_t>(Wire.read());
        ok = true;
    }
    FT02_I2CBusUnlock();
    return ok;
}

bool writeAw(uint8_t addr, uint8_t reg, uint8_t value) {
    if (!FT02_I2CBusLock(80)) return false;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    const bool ok = Wire.endTransmission(true) == 0;
    FT02_I2CBusUnlock();
    return ok;
}

bool setRfControl(uint8_t control, const char* mode) {
    if (!g_awPresent || g_awAddress == 0) return false;
    const uint8_t expected = static_cast<uint8_t>(control & AW_P1_CONTROL_MASK);
    const uint8_t value = static_cast<uint8_t>(g_p1UnrelatedBits | expected);
    if (!writeAw(g_awAddress, AW_OUTPUT_P1, value)) return false;
    uint8_t latch = 0;
    if (!readAw(g_awAddress, AW_OUTPUT_P1, latch)) return false;
    if ((latch & AW_P1_CONTROL_MASK) != expected) {
        FT02_LOGE("[ERROR][LORA-HW] RF_PATH failed mode=%s latch=0x%02X expected=0x%02X\n", mode, latch, expected);
        return false;
    }
    g_rfRxActive = (expected == AW_RADIO_RX_STANDBY);
    g_rfTxActive = (expected == AW_RADIO_TX_ACTIVE);
    FT02_LOGD("[LORA-HW] RF_PATH mode=%s rxen=%d txen=%d latch=0x%02X\n",
                  mode, g_rfRxActive ? 1 : 0, g_rfTxActive ? 1 : 0, latch);
    return true;
}

bool waitBusyLow(uint32_t timeoutMs) {
    const uint32_t startMs = millis();
    while (digitalRead(RADIO_BUSY) == HIGH) {
        if (static_cast<uint32_t>(millis() - startMs) >= timeoutMs) return false;
        delay(1);
    }
    return true;
}

void printResult() {
    FT02_LOGD("[LORA-HW] RESULT aw=%s addr=0x%02X sx1262=%s busy=%d dio1=%d spi=HW-HSPI-PENDING rxen=1 txen=0 tx=PENDING_BACKEND\n",
                  g_awPresent ? "PASS" : "FAIL", g_awAddress,
                  g_radioPreflightReady ? "PREFLIGHT" : "NOT_READY",
                  digitalRead(RADIO_BUSY), digitalRead(RADIO_DIO1));
}
}

void FT02_LoRaHwProbeBegin() {
    // EPD has released HSPI in this build. The radio backend will initialize
    // HSPI on GPIO38/40/39/7 after this AW9523 reset/RF-path preflight.
    pinMode(RADIO_BUSY, INPUT);
    pinMode(RADIO_DIO1, INPUT);

    FT02_LOGD_PRINTLN("[LORA-HW] probe begin: Core1262-LF / AW9523 / no RF transmit");
    for (uint8_t addr = 0x58; addr <= 0x5B; ++addr) {
        uint8_t chipId = 0;
        if (readAw(addr, AW_ID, chipId) && chipId == AW_CHIP_ID) {
            g_awPresent = true;
            g_awAddress = addr;
            FT02_LOGD("[LORA-HW] AW9523 detected addr=0x%02X id=0x%02X\n", addr, chipId);
            break;
        }
    }
    if (!g_awPresent) {
        FT02_LOGE_PRINTLN("[ERROR][LORA-HW] AW9523 not found (expected ID 0x23 at 0x58..0x5B); no reset or SPI probe attempted");
        printResult();
        g_lastReportMs = millis();
        return;
    }

    uint8_t outputP1 = 0, configP1 = 0, modeP1 = 0;
    if (!readAw(g_awAddress, AW_OUTPUT_P1, outputP1) ||
        !readAw(g_awAddress, AW_CONFIG_P1, configP1) ||
        !readAw(g_awAddress, AW_MODE_P1, modeP1)) {
        FT02_LOGE_PRINTLN("[ERROR][LORA-HW] AW9523 control registers unreadable; no reset pulse attempted");
        printResult();
        g_lastReportMs = millis();
        return;
    }

    // Preserve other P1 bits. Any future P1 users must share an atomic AW owner.
    g_p1UnrelatedBits = static_cast<uint8_t>(outputP1 & ~AW_P1_CONTROL_MASK);
    const bool configured =
        writeAw(g_awAddress, AW_OUTPUT_P1, static_cast<uint8_t>(g_p1UnrelatedBits | AW_RADIO_RESET_ASSERTED)) &&
        writeAw(g_awAddress, AW_MODE_P1, static_cast<uint8_t>(modeP1 | AW_P1_CONTROL_MASK)) &&
        writeAw(g_awAddress, AW_CONFIG_P1, static_cast<uint8_t>(configP1 & ~AW_P1_CONTROL_MASK));
    if (!configured) {
        FT02_LOGE_PRINTLN("[ERROR][LORA-HW] AW9523 output configuration; radio RESET remains asserted if output write succeeded");
        printResult();
        g_lastReportMs = millis();
        return;
    }
    delay(5); // SX1262 hardware nRESET low; >100us required.
    if (!writeAw(g_awAddress, AW_OUTPUT_P1,
                 static_cast<uint8_t>(g_p1UnrelatedBits | AW_RADIO_RX_STANDBY))) {
        FT02_LOGE_PRINTLN("[ERROR][LORA-HW] cannot release RESET through AW9523");
        printResult();
        g_lastReportMs = millis();
        return;
    }
    uint8_t latch = 0;
    if (!readAw(g_awAddress, AW_OUTPUT_P1, latch) ||
        (latch & AW_P1_CONTROL_MASK) != AW_RADIO_RX_STANDBY) {
        FT02_LOGE_PRINTLN("[ERROR][LORA-HW] AW9523 output latch mismatch; verify P1_0/P1_1/P1_2 mapping");
        printResult();
        g_lastReportMs = millis();
        return;
    }
    g_rfRxActive = true;
    g_rfTxActive = false;
    FT02_LOGD("[LORA-HW] AW outputs: RESET=1 RXEN=1 TXEN=0 p1_latch=0x%02X\n", latch);
    delay(20);
    g_radioPreflightReady = waitBusyLow(250);
    if (!g_radioPreflightReady) {
        FT02_LOGE("[ERROR][LORA-HW] SX1262 BUSY stuck HIGH gpio=%d; check 3.3V/RESET/BUSY\n", RADIO_BUSY);
        printResult();
        g_lastReportMs = millis();
        return;
    }
    FT02_LOGD_PRINTLN("[LORA-HW] SX1262 preflight PASS busy=0; SPI identity check delegated to RadioLib HW-SPI begin");
    printResult();
    g_lastReportMs = millis();
}

void FT02_LoRaHwProbePoll() {
    // Deliberately no work during light sleep: main.loop exits before Poll.
    // Repeat a concise status every 15 s so USB monitor reconnect does not
    // swallow the boot-time probe. Never reconfigure or switch RF to TX.
    if (static_cast<uint32_t>(millis() - g_lastReportMs) < 15000u) return;
    g_lastReportMs = millis();
    if (g_radioOwned) {
        FT02_LOGD("[LORA-HW] RESULT aw=%s addr=0x%02X sx1262=RADIO_BACKEND_OWNED busy=%d dio1=%d spi=HW-HSPI-4MHZ rxen=%d txen=%d tx=ENABLED_PKI\n",
                      g_awPresent ? "PASS" : "FAIL", g_awAddress, digitalRead(RADIO_BUSY), digitalRead(RADIO_DIO1),
                      g_rfRxActive ? 1 : 0, g_rfTxActive ? 1 : 0);
    } else {
        printResult();
    }
}

// Current hardware probe remains the sole AW9523 reset/RF-path owner.
bool FT02_LoRaHwProbeReady() { return g_awPresent && g_radioPreflightReady; }

void FT02_LoRaHwProbeSetRadioOwned(bool owned) { g_radioOwned = owned; }


bool FT02_LoRaHwProbeSetRfIdle() {
    return setRfControl(AW_RADIO_RF_IDLE, "IDLE");
}

bool FT02_LoRaHwProbeSetRfReceive() {
    return setRfControl(AW_RADIO_RX_STANDBY, "RX");
}

bool FT02_LoRaHwProbeSetRfTransmit() {
    return setRfControl(AW_RADIO_TX_ACTIVE, "TX");
}

bool FT02_LoRaHwProbeRfTransmitActive() { return g_rfTxActive; }
