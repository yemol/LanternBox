#include "FT02_LoRaTransport.h"
#include "FT02_LoRaCoprocessor.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LoRaCommunicationRuntime.h"

#include <HardwareSerial.h>

namespace
{
constexpr int FT02_LORA_RX_PIN = 7;
constexpr int FT02_LORA_TX_PIN = 13;
constexpr uint32_t FT02_LORA_BAUD = 115200u;
constexpr size_t FT02_LORA_RX_BUFFER = 8192u;
constexpr uint16_t FT02_LORA_MAX_PROTO = 512u;

constexpr uint8_t FT02_MT_START1 = 0x94u;
constexpr uint8_t FT02_MT_START2 = 0xC3u;

// Golden v2.66b full-config nonce. Keep this ordinary non-zero nonce so the
// radio returns its complete initial stream, including NodeInfo records.
constexpr uint32_t FT02_MT_CONFIG_ID = 0x4C420266u;

// WSL V3 needs time to boot after CHIP_PU is released.
constexpr uint32_t FT02_MT_BOOT_WAIT_MS = 5000u;
// A full stream is only ~1.2 KiB on the current three-node mesh. 15 seconds is
// deliberately generous and avoids reset loops during e-paper activity.
constexpr uint32_t FT02_MT_SYNC_TIMEOUT_MS = 15000u;
constexpr uint32_t FT02_MT_NODE_GRACE_MS = 2500u;
constexpr uint32_t FT02_MT_WAIT_LOG_MS = 10000u;
constexpr uint32_t FT02_MT_HEARTBEAT_MS = 20000u;
constexpr uint32_t FT02_MT_RUNTIME_SILENCE_RESET_MS = 70000u;

HardwareSerial g_loraSerial(2);

enum RxState : uint8_t
{
    RX_WAIT_START1 = 0,
    RX_WAIT_START2,
    RX_LEN_MSB,
    RX_LEN_LSB,
    RX_PAYLOAD
};

RxState g_rxState = RX_WAIT_START1;
uint16_t g_expectedLength = 0;
uint16_t g_payloadPos = 0;
uint8_t g_payload[FT02_LORA_MAX_PROTO];

uint32_t g_cycleStartMs = 0;
uint32_t g_lastRequestMs = 0;
uint32_t g_lastFrameMs = 0;
uint32_t g_lastWaitLogMs = 0;
uint32_t g_configCompleteMs = 0;
uint32_t g_frameCount = 0;
uint32_t g_lastHeartbeatMs = 0;
uint32_t g_heartbeatNonce = 2u;
uint32_t g_cycleCount = 0;
bool g_requestSent = false;
bool g_linkUp = false;
bool g_configComplete = false;

size_t encodeVarint32(uint32_t value, uint8_t* out, size_t capacity)
{
    size_t count = 0;
    do
    {
        if(count >= capacity) return 0;
        uint8_t b = static_cast<uint8_t>(value & 0x7Fu);
        value >>= 7;
        if(value != 0) b |= 0x80u;
        out[count++] = b;
    }
    while(value != 0);
    return count;
}

bool readVarint(const uint8_t* data, size_t length, size_t& offset, uint64_t& value)
{
    value = 0;
    uint8_t shift = 0;
    while(offset < length && shift < 64)
    {
        const uint8_t b = data[offset++];
        value |= static_cast<uint64_t>(b & 0x7Fu) << shift;
        if((b & 0x80u) == 0) return true;
        shift = static_cast<uint8_t>(shift + 7);
    }
    return false;
}

bool findTopLevelVarintField(
    const uint8_t* data,
    size_t length,
    uint32_t wantedField,
    uint64_t& fieldValue
)
{
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return false;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 0x07u);

        switch(wire)
        {
            case 0:
            {
                uint64_t value = 0;
                if(!readVarint(data, length, offset, value)) return false;
                if(field == wantedField)
                {
                    fieldValue = value;
                    return true;
                }
                break;
            }
            case 1:
                if(offset + 8 > length) return false;
                offset += 8;
                break;
            case 2:
            {
                uint64_t itemLength = 0;
                if(!readVarint(data, length, offset, itemLength)) return false;
                if(itemLength > static_cast<uint64_t>(length - offset)) return false;
                offset += static_cast<size_t>(itemLength);
                break;
            }
            case 5:
                if(offset + 4 > length) return false;
                offset += 4;
                break;
            default:
                return false;
        }
    }
    return false;
}

void resetParser()
{
    g_rxState = RX_WAIT_START1;
    g_expectedLength = 0;
    g_payloadPos = 0;
}

void drainRx()
{
    while(g_loraSerial.available() > 0)
    {
        (void)g_loraSerial.read();
    }
}

void startFreshRadioCycle(const char* reason)
{
    resetParser();
    drainRx();
    FT02_LoRaNodeRuntimeReset();
    FT02_LoRaCommunicationRuntimeResetSession();

    g_requestSent = false;
    g_linkUp = false;
    g_configComplete = false;
    g_frameCount = 0;
    g_lastRequestMs = 0;
    g_lastFrameMs = 0;
    g_configCompleteMs = 0;
    g_lastHeartbeatMs = 0;

    FT02_LoRaCoprocessorReset(reason);
    g_cycleStartMs = FT02_LoRaCoprocessorLastReleaseMs();
    g_lastWaitLogMs = g_cycleStartMs;
    ++g_cycleCount;

    Serial.printf(
        "[LORA] fresh radio cycle=%lu reason=%s boot_wait_ms=%lu\n",
        static_cast<unsigned long>(g_cycleCount),
        reason != nullptr ? reason : "unspecified",
        static_cast<unsigned long>(FT02_MT_BOOT_WAIT_MS)
    );
}

void sendWantConfig()
{
    // Exact v2.66b ToRadio.want_config_id protobuf: field 3, wire type varint.
    uint8_t protobuf[8];
    protobuf[0] = 0x18u;
    const size_t varintLength = encodeVarint32(
        FT02_MT_CONFIG_ID,
        protobuf + 1,
        sizeof(protobuf) - 1
    );
    if(varintLength == 0)
    {
        Serial.println("[LORA] ERROR want_config varint encode failed");
        return;
    }

    const uint16_t protobufLength = static_cast<uint16_t>(1 + varintLength);
    const uint8_t header[4] = {
        FT02_MT_START1,
        FT02_MT_START2,
        static_cast<uint8_t>((protobufLength >> 8) & 0xFFu),
        static_cast<uint8_t>(protobufLength & 0xFFu)
    };

    // Ignore boot/reboot notices accumulated before the request. Starting with
    // an empty local RX queue makes the full-sync statistics deterministic.
    drainRx();
    resetParser();

    g_loraSerial.write(header, sizeof(header));
    g_loraSerial.write(protobuf, protobufLength);
    g_loraSerial.flush();

    g_requestSent = true;
    g_lastRequestMs = millis();

    Serial.print("[LORA] PROTO full-sync want_config id=0x");
    Serial.print(FT02_MT_CONFIG_ID, HEX);
    Serial.println(" bytes=94 C3 00 06 18 E6 84 88 E2 04");
}

void handleFrame(const uint8_t* payload, uint16_t length)
{
    ++g_frameCount;
    g_lastFrameMs = millis();

    if(!g_linkUp)
    {
        g_linkUp = true;
        Serial.print("[LORA] PROTO LINK DETECTED first_frame_len=");
        Serial.println(length);
    }

    // Feed every complete FromRadio payload to the upper layer before handling
    // config_complete, so NodeDB has already consumed all preceding NodeInfo.
    FT02_LoRaNodeRuntimeOnFromRadio(payload, length);
    FT02_LoRaCommunicationRuntimeOnFromRadio(payload, length);

    // FromRadio.rebooted is field 8. It can legitimately appear immediately
    // after the hardware reset. It is informational here because this runtime
    // already owns the radio reset lifecycle.
    uint64_t rebooted = 0;
    if(findTopLevelVarintField(payload, length, 8u, rebooted) && rebooted != 0)
    {
        Serial.println("[LORA] radio rebooted notice received");
    }

    // FromRadio.config_complete_id is field 7.
    uint64_t configCompleteId = 0;
    if(findTopLevelVarintField(payload, length, 7u, configCompleteId) &&
       configCompleteId == FT02_MT_CONFIG_ID && !g_configComplete)
    {
        g_configComplete = true;
        g_configCompleteMs = millis();

        Serial.printf(
            "[LORA] PROTO full-sync complete id=0x%08lX frames=%lu nodes=%u expected=%lu\n",
            static_cast<unsigned long>(configCompleteId),
            static_cast<unsigned long>(g_frameCount),
            static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
            static_cast<unsigned long>(FT02_LoRaNodeRuntimeExpectedNodeCount())
        );

        if(FT02_LoRaNodeRuntimeReady())
        {
            Serial.printf(
                "[LORA] PROTO FULL-SYNC PASS frames=%lu radio_resets=%lu\n",
                static_cast<unsigned long>(g_frameCount),
                static_cast<unsigned long>(FT02_LoRaCoprocessorResetCount())
            );
        }
        else
        {
            Serial.println("[LORA] NodeDB incomplete at config_complete; grace period started");
        }
    }

    if(g_frameCount == 5u || g_frameCount == 10u ||
       g_frameCount == 20u || g_frameCount == 40u)
    {
        Serial.print("[LORA] PROTO frames_received=");
        Serial.println(g_frameCount);
    }
}

void sendHeartbeat()
{
    // ToRadio.heartbeat = field 7 (length-delimited), Heartbeat.nonce = field 1.
    // Nonce 1 is special in Meshtastic 2.7.26 and triggers a NodeInfo broadcast,
    // so normal keepalive starts at 2 and increments.
    uint8_t nested[8];
    nested[0] = 0x08u;
    const size_t n = encodeVarint32(g_heartbeatNonce, nested + 1, sizeof(nested) - 1);
    if(n == 0) return;
    const size_t nestedLen = 1 + n;
    uint8_t toRadio[12];
    size_t pos = 0;
    toRadio[pos++] = 0x3Au; // field 7, wire 2
    const size_t l = encodeVarint32(static_cast<uint32_t>(nestedLen), toRadio + pos, sizeof(toRadio) - pos);
    if(l == 0) return;
    pos += l;
    if(pos + nestedLen > sizeof(toRadio)) return;
    memcpy(toRadio + pos, nested, nestedLen);
    pos += nestedLen;
    if(FT02_LoRaTransportSendToRadio(toRadio, pos))
    {
        g_lastHeartbeatMs = millis();
        ++g_heartbeatNonce;
        if(g_heartbeatNonce == 0u || g_heartbeatNonce == 1u) g_heartbeatNonce = 2u;
    }
}

void consumeByte(uint8_t b)
{
    switch(g_rxState)
    {
        case RX_WAIT_START1:
            if(b == FT02_MT_START1) g_rxState = RX_WAIT_START2;
            break;

        case RX_WAIT_START2:
            if(b == FT02_MT_START2)
            {
                g_rxState = RX_LEN_MSB;
            }
            else if(b != FT02_MT_START1)
            {
                g_rxState = RX_WAIT_START1;
            }
            break;

        case RX_LEN_MSB:
            g_expectedLength = static_cast<uint16_t>(b) << 8;
            g_rxState = RX_LEN_LSB;
            break;

        case RX_LEN_LSB:
            g_expectedLength |= b;
            g_payloadPos = 0;
            if(g_expectedLength == 0 || g_expectedLength > FT02_LORA_MAX_PROTO)
            {
                Serial.print("[LORA] invalid PROTO frame length=");
                Serial.println(g_expectedLength);
                g_rxState = RX_WAIT_START1;
            }
            else
            {
                g_rxState = RX_PAYLOAD;
            }
            break;

        case RX_PAYLOAD:
            g_payload[g_payloadPos++] = b;
            if(g_payloadPos >= g_expectedLength)
            {
                handleFrame(g_payload, g_expectedLength);
                g_rxState = RX_WAIT_START1;
            }
            break;
    }
}
} // namespace

void FT02_LoRaTransportBegin()
{
    // HardwareSerial owns an interrupt-backed RX ring. 8 KiB comfortably holds
    // the measured ~1.2 KiB full-config burst while the e-paper blocks main.
    g_loraSerial.setRxBufferSize(FT02_LORA_RX_BUFFER);
    g_loraSerial.begin(
        FT02_LORA_BAUD,
        SERIAL_8N1,
        FT02_LORA_RX_PIN,
        FT02_LORA_TX_PIN
    );

    Serial.printf(
        "[LORA] UART2 started RX=%d TX=%d baud=%lu rxbuf=%u\n",
        FT02_LORA_RX_PIN,
        FT02_LORA_TX_PIN,
        static_cast<unsigned long>(FT02_LORA_BAUD),
        static_cast<unsigned>(FT02_LORA_RX_BUFFER)
    );

    FT02_LoRaCoprocessorBegin();
    startFreshRadioCycle("ft02-boot");
}

void FT02_LoRaTransportPoll()
{
    // Drain aggressively when main is available. The ISR-backed 8 KiB RX ring
    // protects data while e-paper or other peripherals temporarily block it.
    uint16_t budget = 4096u;
    while(budget-- > 0 && g_loraSerial.available() > 0)
    {
        consumeByte(static_cast<uint8_t>(g_loraSerial.read()));
    }

    const uint32_t now = millis();
    const uint32_t sinceCycle = now - g_cycleStartMs;

    if(!g_requestSent && sinceCycle >= FT02_MT_BOOT_WAIT_MS)
    {
        sendWantConfig();
        return;
    }

    if(g_requestSent && !g_configComplete &&
       now - g_lastRequestMs >= FT02_MT_SYNC_TIMEOUT_MS)
    {
        Serial.printf(
            "[LORA] sync timeout link=%u frames=%lu; resetting radio coprocessor\n",
            g_linkUp ? 1u : 0u,
            static_cast<unsigned long>(g_frameCount)
        );
        startFreshRadioCycle(g_linkUp ? "sync-incomplete" : "no-response");
        return;
    }

    if(g_configComplete && !FT02_LoRaNodeRuntimeReady() &&
       now - g_configCompleteMs >= FT02_MT_NODE_GRACE_MS)
    {
        Serial.printf(
            "[LORA] NodeDB incomplete nodes=%u expected=%lu; resetting radio and resyncing\n",
            static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
            static_cast<unsigned long>(FT02_LoRaNodeRuntimeExpectedNodeCount())
        );
        startFreshRadioCycle("nodedb-incomplete");
        return;
    }

    if(FT02_LoRaNodeRuntimeReady() &&
       g_lastFrameMs != 0u &&
       now - g_lastFrameMs >= FT02_MT_RUNTIME_SILENCE_RESET_MS)
    {
        Serial.printf(
            "[LORA] runtime heartbeat timeout silence_ms=%lu; resetting radio coprocessor\n",
            static_cast<unsigned long>(now - g_lastFrameMs)
        );
        startFreshRadioCycle("runtime-heartbeat-timeout");
        return;
    }

    if(FT02_LoRaNodeRuntimeReady() &&
       (g_lastHeartbeatMs == 0u || now - g_lastHeartbeatMs >= FT02_MT_HEARTBEAT_MS))
    {
        sendHeartbeat();
    }

    if(!FT02_LoRaNodeRuntimeReady() &&
       now - g_lastWaitLogMs >= FT02_MT_WAIT_LOG_MS)
    {
        g_lastWaitLogMs = now;
        Serial.printf(
            "[LORA] waiting cycle=%lu reset_count=%lu request=%u link=%u frames=%lu nodes=%u/%lu\n",
            static_cast<unsigned long>(g_cycleCount),
            static_cast<unsigned long>(FT02_LoRaCoprocessorResetCount()),
            g_requestSent ? 1u : 0u,
            g_linkUp ? 1u : 0u,
            static_cast<unsigned long>(g_frameCount),
            static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
            static_cast<unsigned long>(FT02_LoRaNodeRuntimeExpectedNodeCount())
        );
    }
}

bool FT02_LoRaTransportLinkUp()
{
    return g_linkUp;
}

bool FT02_LoRaTransportConfigComplete()
{
    return g_configComplete;
}

uint32_t FT02_LoRaTransportFrameCount()
{
    return g_frameCount;
}

uint32_t FT02_LoRaTransportResetCount()
{
    return FT02_LoRaCoprocessorResetCount();
}


bool FT02_LoRaTransportSendToRadio(const uint8_t* protobuf, size_t length)
{
    if(protobuf == nullptr || length == 0 || length > FT02_LORA_MAX_PROTO) return false;
    if(!g_linkUp || !FT02_LoRaNodeRuntimeReady()) return false;

    const uint8_t header[4] = {
        FT02_MT_START1,
        FT02_MT_START2,
        static_cast<uint8_t>((length >> 8) & 0xFFu),
        static_cast<uint8_t>(length & 0xFFu)
    };
    const size_t h = g_loraSerial.write(header, sizeof(header));
    const size_t p = g_loraSerial.write(protobuf, length);
    g_loraSerial.flush();
    if(h != sizeof(header) || p != length)
    {
        Serial.printf(
            "[LORA] ToRadio short write header=%u/%u payload=%u/%u\n",
            static_cast<unsigned>(h),
            static_cast<unsigned>(sizeof(header)),
            static_cast<unsigned>(p),
            static_cast<unsigned>(length)
        );
        return false;
    }
    return true;
}

void FT02_LoRaTransportForceResync(const char* reason)
{
    startFreshRadioCycle(reason != nullptr ? reason : "manual-resync");
}
