#include "FT02_MeshRadioBackend.h"
#include "FT02_Log.h"

#include <Arduino.h>
#include <RadioLib.h>
#include <string.h>

#include "FT02_LoRaHwProbe.h"
#include "FT02_LoRaHardwareSpi.h"

namespace {
constexpr float MESH_FREQ_MHZ = 478.875f; // CN35: already proven by FT01/Meshtastic network
constexpr float MESH_BW_KHZ = 250.0f;
constexpr uint8_t MESH_SF = 11;
constexpr uint8_t MESH_CR = 5;
constexpr uint8_t MESH_SYNC_WORD = 0x2B;
constexpr int8_t MESH_POWER_DBM = 19;      // Matches supplied Meshtastic CN profile for broadcast TX
constexpr uint16_t MESH_PREAMBLE = 16;     // Meshtastic RadioInterface::preambleLengthDefault
#ifndef FT02_LORA_TCXO_DIO3_TENTHS
#define FT02_LORA_TCXO_DIO3_TENTHS 17
#endif
constexpr float MESH_TCXO_VOLTAGE = static_cast<float>(FT02_LORA_TCXO_DIO3_TENTHS) / 10.0f;
constexpr bool MESH_USE_LDO = false;       // Core1262 schematic includes the 15uH DCDC network; keep Meshtastic DCDC mode.
constexpr uint32_t MESH_IRQ_POLL_INTERVAL_MS = 20;

Module* g_module = nullptr;
SX1262* g_radio = nullptr;
volatile bool g_packetFlag = false;
bool g_ready = false;
bool g_frameReady = false;
bool g_sleepSession = false;
FT02MeshRadioFrame g_frame{};
uint32_t g_rxCount = 0;
uint32_t g_crcErrors = 0;
uint32_t g_otherErrors = 0;
int16_t g_lastState = RADIOLIB_ERR_NONE;
uint32_t g_lastStatsMs = 0;
uint32_t g_lastIrqPollMs = 0;
uint32_t g_irqPollCount = 0;
uint32_t g_preambleCount = 0;
uint32_t g_headerValidCount = 0;
uint32_t g_headerErrorCount = 0;
uint32_t g_timeoutCount = 0;
uint32_t g_rxDoneCount = 0;
uint16_t g_lastIrqFlags = 0;
bool g_txInProgress = false;
bool g_packetActionAttached = false;
uint32_t g_txCount = 0;
uint32_t g_txFailCount = 0;

#if defined(ESP32)
IRAM_ATTR
#endif
void onPacketReceived() {
    g_packetFlag = true;
}

void detachPacketReceivedActionForSleep() {
    if (!g_radio || !g_packetActionAttached) return;

    // RadioLib's SX126x packet callback is the DIO1 GPIO ISR.  It must not
    // remain registered while the radio is in warm sleep and FT02 is
    // reconfiguring unrelated GPIOs for the keyboard sleep handshake.
    g_radio->clearPacketReceivedAction();
    g_packetActionAttached = false;

    noInterrupts();
    g_packetFlag = false;
    interrupts();
}

void attachPacketReceivedActionAfterWake() {
    if (!g_radio || g_packetActionAttached) return;

    g_radio->setPacketReceivedAction(onPacketReceived);
    g_packetActionAttached = true;

    // A packet can complete in the tiny interval between startReceive() and
    // reattaching the DIO1 ISR. SX1262 IRQ/DIO1 stays latched, so seed the
    // software flag once after attachment instead of losing that event.
    if (digitalRead(FT02_LoRaHardwareSpi::DIO1) == HIGH) {
        noInterrupts();
        g_packetFlag = true;
        interrupts();
    }
}

void printProfile() {
    FT02_LOGD("[MESH-RADIO] PROFILE backend=RadioLib-SX1262-HW-SPI meshtastic=LongFast irq_poll=Meshtastic-style freq=%.3f bw=%.1f sf=%u cr=4/%u sync=0x%02X preamble=%u header=explicit crc=on tcxo=%.1fV regulator=DCDC rxboost=1 spi=HW-HSPI-4MHZ tx=enabled-pki\n",
                  static_cast<double>(MESH_FREQ_MHZ), static_cast<double>(MESH_BW_KHZ), MESH_SF, MESH_CR,
                  MESH_SYNC_WORD, MESH_PREAMBLE, static_cast<double>(MESH_TCXO_VOLTAGE));
}

bool startListening() {
    if (!g_radio) return false;

    // Match Meshtastic SX126x receive semantics: monitor terminal RX errors and
    // preamble/header milestones in the SX1262 IRQ register, while keeping DIO1
    // dedicated to RX_DONE.  A 20 ms software poll catches errors/missed edges.
    const RadioLibIrqFlags_t irqFlags =
        RADIOLIB_IRQ_RX_DEFAULT_FLAGS |
        (static_cast<RadioLibIrqFlags_t>(1) << RADIOLIB_IRQ_PREAMBLE_DETECTED) |
        (static_cast<RadioLibIrqFlags_t>(1) << RADIOLIB_IRQ_HEADER_VALID);
    const int16_t state = g_radio->startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF,
                                                irqFlags,
                                                RADIOLIB_IRQ_RX_DEFAULT_MASK,
                                                0);
    g_lastState = state;
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] startReceive failed state=%d\n", state);
        return false;
    }
    g_lastIrqPollMs = millis();
    FT02_LOGD_PRINTLN("[MESH-RADIO] startReceive PASS irq_poll=20ms");
    return true;
}

bool processTerminalIrq(uint16_t irq) {
    if (!g_radio) return false;

    const bool rxDone = (irq & RADIOLIB_SX126X_IRQ_RX_DONE) != 0;
    const bool crcErr = (irq & RADIOLIB_SX126X_IRQ_CRC_ERR) != 0;
    const bool headerErr = (irq & RADIOLIB_SX126X_IRQ_HEADER_ERR) != 0;
    const bool timeout = (irq & RADIOLIB_SX126X_IRQ_TIMEOUT) != 0;

    if (timeout && !rxDone && !crcErr && !headerErr) {
        ++g_timeoutCount;
        g_lastState = RADIOLIB_ERR_RX_TIMEOUT;
        g_radio->clearIrqFlags(irq);
        FT02_LOGW("[WARN][MESH] IRQ timeout irq=0x%04X count=%lu action=restart\n",
                      irq, static_cast<unsigned long>(g_timeoutCount));
        return startListening();
    }

    if (!(rxDone || crcErr || headerErr)) return false;
    if (rxDone) ++g_rxDoneCount;
    if (crcErr) ++g_crcErrors;
    if (headerErr) ++g_headerErrorCount;

    FT02MeshRadioFrame frame{};
    frame.len = g_radio->getPacketLength();
    if (frame.len > sizeof(frame.data)) {
        ++g_otherErrors;
        FT02_LOGW("[WARN][MESH] bad RX length irq=0x%04X len=%u action=clear_restart\n",
                      irq, static_cast<unsigned>(frame.len));
        g_radio->clearIrqFlags(irq);
        return startListening();
    }

    const int16_t state = g_radio->readData(frame.data, frame.len);
    frame.radioState = state;
    frame.rssi = g_radio->getRSSI();
    frame.snr = g_radio->getSNR();
    frame.frequencyErrorHz = g_radio->getFrequencyError();
    frame.crcOk = (state == RADIOLIB_ERR_NONE);
    g_lastState = state;

    // readData() clears the SX1262 IRQ register after copying the FIFO.  Preserve
    // bad frames too, exactly so MeshService can show the real over-the-air bytes.
    if (state == RADIOLIB_ERR_NONE) {
        ++g_rxCount;
        g_frame = frame;
        g_frameReady = true;
        FT02_LOGD("[MESH-RADIO] RX_DONE n=%lu irq=0x%04X len=%u rssi=%.1f snr=%.2f ferr=%.0f state=%d\n",
                      static_cast<unsigned long>(g_rxCount), irq, static_cast<unsigned>(frame.len),
                      static_cast<double>(frame.rssi), static_cast<double>(frame.snr),
                      static_cast<double>(frame.frequencyErrorHz), state);
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        g_frame = frame;
        g_frameReady = true;
        FT02_LOGW("[WARN][MESH] RX_BAD irq=0x%04X crc_err=%d hdr_err=%d len=%u rssi=%.1f snr=%.2f ferr=%.0f state=%d\n",
                      irq, crcErr ? 1 : 0, headerErr ? 1 : 0, static_cast<unsigned>(frame.len),
                      static_cast<double>(frame.rssi), static_cast<double>(frame.snr),
                      static_cast<double>(frame.frequencyErrorHz), state);
    } else {
        ++g_otherErrors;
        FT02_LOGE("[ERROR][MESH] RX_ERROR irq=0x%04X n=%lu state=%d len=%u\n",
                      irq, static_cast<unsigned long>(g_otherErrors), state, static_cast<unsigned>(frame.len));
    }

    // Meshtastic restarts RX after every terminal receive event.
    return startListening();
}
}

bool FT02_MeshRadioBackendBegin() {
    if (g_ready) return true;
    if (!FT02_LoRaHwProbeReady()) {
        FT02_LOGE_PRINTLN("[ERROR][MESH] begin blocked: Core1262 hardware probe not ready");
        return false;
    }

    // Hardware probe has already applied the AW9523 reset pulse and fixed RX path:
    // RESET=1, RXEN=1, TXEN=0. RadioLib must not attempt to drive AW9523 pins.
    if (!FT02_LoRaHardwareSpi::begin()) {
        FT02_LOGE_PRINTLN("[ERROR][MESH] HW-SPI begin failed host=HSPI pins=38/40/39/7");
        return false;
    }

    // RadioLib's extended Arduino Module constructor uses the provided SPIClass
    // and SPISettings and intentionally does not initialize the bus itself.
    g_module = new Module(FT02_LoRaHardwareSpi::NSS,
                          FT02_LoRaHardwareSpi::DIO1,
                          RADIOLIB_NC,
                          FT02_LoRaHardwareSpi::BUSY,
                          FT02_LoRaHardwareSpi::bus(),
                          FT02_LoRaHardwareSpi::settings());
    g_radio = new SX1262(g_module);

    FT02_LOGD_PRINTLN("[MESH-RADIO] backend=HW-SPI-A1 owner=RadioLib SX1262 old_diag=inactive");
    FT02_LOGD("[MESH-RADIO] HW-SPI host=HSPI sck=%d miso=%d mosi=%d cs=%d hz=%lu mode=0\n",
                  FT02_LoRaHardwareSpi::SCK, FT02_LoRaHardwareSpi::MISO,
                  FT02_LoRaHardwareSpi::MOSI, FT02_LoRaHardwareSpi::NSS,
                  static_cast<unsigned long>(FT02_LoRaHardwareSpi::CLOCK_HZ));
    printProfile();

    int16_t state = g_radio->begin(MESH_FREQ_MHZ, MESH_BW_KHZ, MESH_SF, MESH_CR,
                                   MESH_SYNC_WORD, MESH_POWER_DBM, MESH_PREAMBLE,
                                   MESH_TCXO_VOLTAGE, MESH_USE_LDO);
    g_lastState = state;
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] radio begin failed state=%d\n", state);
        return false;
    }

    // Match the supplied Meshtastic SX126xInterface behavior after RadioLib begin().
    state = g_radio->setCurrentLimit(140.0f);
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] currentLimit failed state=%d\n", state);
        return false;
    }
    state = g_radio->setDio2AsRfSwitch(false);
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] dio2RfSwitch failed state=%d\n", state);
        return false;
    }
    state = g_radio->explicitHeader();
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] explicitHeader failed state=%d\n", state);
        return false;
    }
    state = g_radio->setCRC(2);
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] CRC setup failed state=%d\n", state);
        return false;
    }
    state = g_radio->setRxBoostedGainMode(true);
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] rxBoost failed state=%d\n", state);
        return false;
    }

    // Meshtastic SX126xInterface applies the Semtech/Heltec RX sensitivity patch
    // after selecting the RX gain mode. A2/A3 missed this exact step. Historical
    // FT02 m13-m17 diagnostics also had bit0 of 0x08B5 set when HeaderValid was
    // observed on real hardware, so keep this as a single evidence-backed delta.
    const int16_t rxPatchState = g_module->SPIsetRegValue(0x08B5, 0x01, 0, 0);
#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_DEBUG
    const int16_t rxPatchRead = g_module->SPIgetRegValue(0x08B5, 0, 0);
    FT02_LOGD("[MESH-RADIO] RX_PATCH reg08B5_bit0 set_state=%d read=%d\n",
                  rxPatchState, rxPatchRead);
#endif
    if (rxPatchState != RADIOLIB_ERR_NONE) {
        FT02_LOGW_PRINTLN("[WARN][MESH] RX sensitivity patch apply failed");
    }

    // DIO1 is the only active packet interrupt. The ISR merely sets a flag.
    // Attach exactly once; the sleep lifecycle explicitly detaches/re-attaches it.
    g_radio->setPacketReceivedAction(onPacketReceived);
    g_packetActionAttached = true;
    if (!startListening()) {
        detachPacketReceivedActionForSleep();
        return false;
    }

    // RadioLib now owns all SX1262 traffic on the dedicated HSPI hardware controller. The HW probe keeps reporting AW state
    // but must stop issuing GetStatus commands in its periodic poll.
    FT02_LoRaHwProbeSetRadioOwned(true);
    g_ready = true;
    g_lastStatsMs = millis();
    FT02_LOGD_PRINTLN("[MESH-RADIO] READY rx=enabled tx=enabled-pki");
    return true;
}

void FT02_MeshRadioBackendPoll() {
    if (!g_ready || !g_radio || g_sleepSession || g_txInProgress) return;

    const uint32_t now = millis();

    // Keep the normal RadioLib RX_DONE edge path.  Meshtastic also polls the
    // SX1262 IRQ register as a fallback, because HEADER_ERR/CRC_ERR need not
    // produce the same DIO1 edge as a good packet.
    if (!g_packetFlag && digitalRead(FT02_LoRaHardwareSpi::DIO1) == HIGH) g_packetFlag = true;

    const bool pollDue = static_cast<uint32_t>(now - g_lastIrqPollMs) >= MESH_IRQ_POLL_INTERVAL_MS;
    if ((g_packetFlag || pollDue) && !g_frameReady) {
        if (g_packetFlag) {
            noInterrupts();
            g_packetFlag = false;
            interrupts();
        }
        if (pollDue) g_lastIrqPollMs = now;

        ++g_irqPollCount;
        const uint16_t irq = static_cast<uint16_t>(g_radio->getIrqFlags());
        g_lastIrqFlags = irq;

        const uint16_t noisyMask = RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED | RADIOLIB_SX126X_IRQ_HEADER_VALID;
        const uint16_t terminalMask = RADIOLIB_SX126X_IRQ_RX_DONE | RADIOLIB_SX126X_IRQ_TIMEOUT |
                                      RADIOLIB_SX126X_IRQ_CRC_ERR | RADIOLIB_SX126X_IRQ_HEADER_ERR;

        if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED) ++g_preambleCount;
        if (irq & RADIOLIB_SX126X_IRQ_HEADER_VALID) ++g_headerValidCount;

        if ((irq & noisyMask) && ((irq & terminalMask) == 0U)) {
            // Exact behavior used by Meshtastic's SX126x software-IRQ fallback:
            // these bits are milestones, not complete packets. Clear them so a
            // noisy preamble cannot leave the IRQ register latched forever.
            g_radio->clearIrqFlags(irq & noisyMask);
            FT02_LOGD("[MESH-RADIO] IRQ_PROGRESS irq=0x%04X pre=%lu hdr_ok=%lu dio1=%d\n",
                          irq, static_cast<unsigned long>(g_preambleCount),
                          static_cast<unsigned long>(g_headerValidCount),
                          digitalRead(FT02_LoRaHardwareSpi::DIO1));
        } else if (irq & terminalMask) {
            processTerminalIrq(irq);
        }
    }

    if (static_cast<uint32_t>(now - g_lastStatsMs) >= 15000u) {
        g_lastStatsMs = now;
        FT02_LOGD("[MESH-RADIO] STATS ready=%d rx=%lu rx_done=%lu tx=%lu tx_fail=%lu pre=%lu hdr_ok=%lu hdr_err=%lu crc_err=%lu timeout=%lu other_err=%lu irq_poll=%lu last_irq=0x%04X frame_pending=%d dio1=%d busy=%d last_state=%d\n",
                      g_ready ? 1 : 0, static_cast<unsigned long>(g_rxCount),
                      static_cast<unsigned long>(g_rxDoneCount), static_cast<unsigned long>(g_txCount),
                      static_cast<unsigned long>(g_txFailCount), static_cast<unsigned long>(g_preambleCount),
                      static_cast<unsigned long>(g_headerValidCount), static_cast<unsigned long>(g_headerErrorCount),
                      static_cast<unsigned long>(g_crcErrors), static_cast<unsigned long>(g_timeoutCount),
                      static_cast<unsigned long>(g_otherErrors), static_cast<unsigned long>(g_irqPollCount),
                      g_lastIrqFlags, g_frameReady ? 1 : 0, digitalRead(FT02_LoRaHardwareSpi::DIO1),
                      digitalRead(FT02_LoRaHardwareSpi::BUSY), g_lastState);
    }
}

bool FT02_MeshRadioBackendTakeFrame(FT02MeshRadioFrame& out) {
    if (!g_frameReady) return false;
    out = g_frame;
    g_frameReady = false;
    return true;
}

bool FT02_MeshRadioBackendTransmit(const uint8_t* data, size_t len) {
    if (!g_ready || !g_radio || !data || len == 0u || len > 240u || g_sleepSession || g_txInProgress) {
        FT02_LOGW("[WARN][MESH] TX_REJECT ready=%d len=%u sleep=%d busy_tx=%d\n",
                      g_ready ? 1 : 0, static_cast<unsigned>(len), g_sleepSession ? 1 : 0, g_txInProgress ? 1 : 0);
        return false;
    }

    g_txInProgress = true;
    // Ignore stale RX milestones before switching the front-end to PA output.
    noInterrupts();
    g_packetFlag = false;
    interrupts();
    (void)g_radio->clearIrqFlags(0x03FFu);

    if (!FT02_LoRaHwProbeSetRfTransmit()) {
        ++g_txFailCount;
        (void)FT02_LoRaHwProbeSetRfReceive();
        g_txInProgress = false;
        FT02_LOGE_PRINTLN("[ERROR][MESH] TX failed stage=rf_path_tx");
        return false;
    }

    // The AW9523 I2C transaction already exceeds the RF switch settling time.
    const uint32_t startedUs = micros();
    const int16_t state = g_radio->transmit(data, len);
    const uint32_t elapsedUs = static_cast<uint32_t>(micros() - startedUs);
    g_lastState = state;
    // TX_DONE uses DIO1 too, so discard the callback flag generated by the
    // blocking transmit before arming RX_DONE again.
    noInterrupts();
    g_packetFlag = false;
    interrupts();

    // RX recovery is mandatory even after a RadioLib error.
    const bool rxPathOk = FT02_LoRaHwProbeSetRfReceive();
    const bool rxRestartOk = rxPathOk && startListening();

    if (state == RADIOLIB_ERR_NONE) {
        ++g_txCount;
        FT02_LOGD("[MESH-RADIO] TX_DONE n=%lu len=%u elapsed_us=%lu rx_restore=%d state=%d\n",
                      static_cast<unsigned long>(g_txCount), static_cast<unsigned>(len),
                      static_cast<unsigned long>(elapsedUs), rxRestartOk ? 1 : 0, state);
    } else {
        ++g_txFailCount;
        FT02_LOGE("[ERROR][MESH] TX_FAIL n=%lu len=%u elapsed_us=%lu rx_restore=%d state=%d\n",
                      static_cast<unsigned long>(g_txFailCount), static_cast<unsigned>(len),
                      static_cast<unsigned long>(elapsedUs), rxRestartOk ? 1 : 0, state);
    }

    if (!rxRestartOk) {
        // Do not retry a packet that RadioLib already transmitted successfully.
        // Instead mark the backend unavailable so the operator sees the RX
        // recovery fault and no further TX is attempted on an unknown RF path.
        g_ready = false;
        FT02_LOGE_PRINTLN("[ERROR][MESH] RX recovery failed backend_disabled=1");
    }
    g_txInProgress = false;
    return state == RADIOLIB_ERR_NONE;
}

bool FT02_MeshRadioBackendReady() { return g_ready; }
uint32_t FT02_MeshRadioBackendRxCount() { return g_rxCount; }
uint32_t FT02_MeshRadioBackendCrcErrorCount() { return g_crcErrors; }
int16_t FT02_MeshRadioBackendLastState() { return g_lastState; }

bool FT02_MeshRadioBackendSleepAvailable() {
    return g_ready && g_radio && !g_txInProgress && !g_frameReady;
}

void FT02_MeshRadioBackendSleepEnter() {
    if (!FT02_MeshRadioBackendSleepAvailable()) {
        g_sleepSession = false;
        FT02_LOGD("[MESH-SLEEP] enter skipped ready=%d tx=%d frame_pending=%d\n",
                      g_ready ? 1 : 0, g_txInProgress ? 1 : 0, g_frameReady ? 1 : 0);
        return;
    }

    // Quarantine the backend before touching DIO1. Poll/TX paths now refuse
    // new work while the receive GPIO interrupt is dismantled.
    g_sleepSession = true;
    detachPacketReceivedActionForSleep();

    noInterrupts();
    g_packetFlag = false;
    interrupts();
    (void)g_radio->clearIrqFlags(0x03FFu);

    const int16_t state = g_radio->sleep(true);
    g_lastState = state;
    if (state != RADIOLIB_ERR_NONE) {
        FT02_LOGE("[ERROR][MESH] sleep enter failed stage=radio_sleep state=%d action=rx_rollback\n", state);

        // Do not leave a half-suspended Mesh backend behind. Return to a known
        // RX state and re-arm DIO1 before allowing the application to continue.
        (void)g_radio->standby();
        (void)g_radio->clearIrqFlags(0x03FFu);
        const bool rxPathOk = FT02_LoRaHwProbeSetRfReceive();
        const bool rxRestartOk = rxPathOk && startListening();
        if (rxRestartOk) {
            attachPacketReceivedActionAfterWake();
        } else {
            g_ready = false;
            FT02_LOGE("[ERROR][MESH] sleep rollback failed rf=%d backend_disabled=1\n",
                      rxPathOk ? 1 : 0);
        }
        g_sleepSession = false;
        return;
    }

    const bool rfIdle = FT02_LoRaHwProbeSetRfIdle();
    if(!rfIdle) FT02_LOGW_PRINTLN("[WARN][MESH] sleep RF idle path failed");
    FT02_LOGD("[MESH-SLEEP] enter radio=warm-sleep rf_idle=%d dio1_irq=detached\n",
              rfIdle ? 1 : 0);
}

void FT02_MeshRadioBackendSleepService() {
    // SX1262 is intentionally asleep and its RF front-end is disabled.
    // CardKB2's timer slice remains the only sleep-service wake source in A3.
}

void FT02_MeshRadioBackendSleepExit() {
    if (!g_sleepSession) return;

    if (!g_radio) {
        g_sleepSession = false;
        g_ready = false;
        g_packetActionAttached = false;
        FT02_LOGE_PRINTLN("[ERROR][MESH] wake restore failed stage=radio_missing backend_disabled=1");
        return;
    }

    // DIO1 ISR must still be detached here. Wake the radio and establish RX
    // first; only then re-arm the GPIO interrupt.
    detachPacketReceivedActionForSleep();

    const int16_t standbyState = g_radio->standby();
    g_lastState = standbyState;
    if (standbyState != RADIOLIB_ERR_NONE) {
        g_sleepSession = false;
        g_ready = false;
        FT02_LOGE("[ERROR][MESH] wake restore failed stage=standby state=%d backend_disabled=1\n", standbyState);
        return;
    }

    (void)g_radio->clearIrqFlags(0x03FFu);
    noInterrupts();
    g_packetFlag = false;
    interrupts();

    const bool rxPathOk = FT02_LoRaHwProbeSetRfReceive();
    const bool rxRestartOk = rxPathOk && startListening();

    if (!rxRestartOk) {
        g_sleepSession = false;
        g_ready = false;
        FT02_LOGE("[ERROR][MESH] wake restore failed stage=rx_restore rf=%d backend_disabled=1\n", rxPathOk ? 1 : 0);
        return;
    }

    attachPacketReceivedActionAfterWake();
    g_sleepSession = false;

    FT02_LOGD_PRINTLN("[MESH] restored dio1_irq=attached");
}
