#include "FT02_LR01HostRuntime.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "FT02_Gnss.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_LoRaNodeRuntime.h"

// v2.75c LR01 Compact Logging A1
#ifndef FT02_LR01_LOG_LEVEL
#define FT02_LR01_LOG_LEVEL 1
#endif

#define LR01_LOG_NORMAL(...) do { if (FT02_LR01_LOG_LEVEL >= 1) Serial.printf(__VA_ARGS__); } while (0)
#define LR01_LOG_VERBOSE(...) do { if (FT02_LR01_LOG_LEVEL >= 2) Serial.printf(__VA_ARGS__); } while (0)

static uint32_t s_lr01LastPongSeqPrinted = 0;
static int s_lr01LastNavFix = -1;
static int s_lr01LastNavCompass = -1;
static int s_lr01LastRadioReady = -1;
static int s_lr01LastRadioNodes = -1;
static int s_lr01LastRadioPki = -1;
static uint32_t s_lr01LastRadioResets = 0xFFFFFFFFu;
static int s_lr01LastSystemGnss = -1;
static int s_lr01LastSystemCompass = -1;
static int s_lr01LastSystemLora = -1;

namespace
{
constexpr int FT02_LR01_RX_PIN = 7;
constexpr int FT02_LR01_TX_PIN = 13;
constexpr uint32_t FT02_LR01_BAUD = 115200;
constexpr size_t FT02_LR01_RX_BUFFER_BYTES = 4096;
constexpr uint32_t FT02_LR01_PING_INTERVAL_MS = 1000;
constexpr uint32_t FT02_LR01_ONLINE_TIMEOUT_MS = 3500;
constexpr size_t FT02_LR01_LINE_CAPACITY = 320; // LR01 host input limit is 256 bytes; leave RX headroom.
constexpr size_t FT02_LR01_TX_TEXT_MAX_BYTES = 120;
constexpr uint32_t FT02_LR01_LOCAL_NODE = 0x4C423002u;

HardwareSerial g_lr01Serial(2);
FT02LR01State g_state;
char g_line[FT02_LR01_LINE_CAPACITY] = {};
size_t g_lineLength = 0;
bool g_dropUntilLf = false;
uint32_t g_lastPingMs = 0;

size_t safeUtf8Prefix(const char* text, size_t maxBytes)
{
    if(text == nullptr) return 0;
    const size_t len = strlen(text);
    size_t n = len < maxBytes ? len : maxBytes;
    if(n == len) return n;
    while(n > 0 && (static_cast<uint8_t>(text[n]) & 0xC0u) == 0x80u) --n;
    return n;
}

bool sendLine(const char* text)
{
    if(text == nullptr || text[0] == '\0') return false;
    g_lr01Serial.print(text);
    g_lr01Serial.print('\n');
    if(strncmp(text, "CORE_PING_", 10) != 0)
        Serial.printf("[LR01-TX] %s\n", text);
    return true;
}

const char* findKey(const char* line, const char* key)
{
    if(line == nullptr || key == nullptr) return nullptr;
    const size_t keyLen = strlen(key);
    const char* p = line;
    while((p = strstr(p, key)) != nullptr)
    {
        if((p == line || p[-1] == ' ') && p[keyLen] == '=') return p + keyLen + 1;
        p += keyLen;
    }
    return nullptr;
}

bool parseInt32(const char* line, const char* key, int32_t& out)
{
    const char* p = findKey(line, key);
    if(p == nullptr) return false;
    char* end = nullptr;
    const long v = strtol(p, &end, 10);
    if(end == p) return false;
    out = static_cast<int32_t>(v);
    return true;
}

bool parseUInt32(const char* line, const char* key, uint32_t& out)
{
    const char* p = findKey(line, key);
    if(p == nullptr) return false;
    char* end = nullptr;
    const unsigned long v = strtoul(p, &end, 10);
    if(end == p) return false;
    out = static_cast<uint32_t>(v);
    return true;
}

bool parseFloat(const char* line, const char* key, float& out)
{
    const char* p = findKey(line, key);
    if(p == nullptr) return false;
    char* end = nullptr;
    const float v = strtof(p, &end);
    if(end == p) return false;
    out = v;
    return true;
}

bool parseToken(const char* line, const char* key, char* out, size_t outSize)
{
    if(out == nullptr || outSize == 0) return false;
    const char* p = findKey(line, key);
    if(p == nullptr) return false;
    size_t n = 0;
    while(p[n] != '\0' && p[n] != ' ' && n + 1 < outSize) ++n;
    memcpy(out, p, n);
    out[n] = '\0';
    return n > 0;
}

bool parseQuoted(const char* line, const char* key, char* out, size_t outSize)
{
    if(out == nullptr || outSize == 0) return false;
    char needle[32];
    snprintf(needle, sizeof(needle), "%s=\"", key);
    const char* p = strstr(line, needle);
    if(p == nullptr) return false;
    p += strlen(needle);
    const char* end = strchr(p, '"');
    if(end == nullptr) return false;
    size_t n = static_cast<size_t>(end - p);
    if(n >= outSize) n = outSize - 1;
    memcpy(out, p, n);
    out[n] = '\0';
    return true;
}

uint32_t parseNodeIdToken(const char* token)
{
    if(token == nullptr) return 0;
    while(*token == '!') ++token;
    char* end = nullptr;
    const unsigned long value = strtoul(token, &end, 16);
    return end == token ? 0 : static_cast<uint32_t>(value);
}

void markRx()
{
    g_state.lastRxMs = millis();
    g_state.online = true;
    ++g_state.lineCount;
}

void applyNavigationToCore()
{
    FT02_GnssApplyLR01State(g_state.gnssReady,
                            g_state.fix,
                            g_state.fixType,
                            g_state.latitudeE7,
                            g_state.longitudeE7,
                            g_state.altitudeDm,
                            g_state.satUsed,
                            g_state.satVisible,
                            g_state.hdopX100,
                            g_state.speedCms,
                            g_state.headingX10,
                            g_state.compassValid,
                            g_state.compassQuality,
                            g_state.gnssBytes,
                            g_state.unixTime,
                            g_state.timeValid);
}

void parseBoot(const char* line)
{
    char node[24] = {};
    char id[24] = {};
    int32_t protocol = 0;
    (void)parseToken(line, "node", node, sizeof(node));
    (void)parseToken(line, "id", id, sizeof(id));
    (void)parseInt32(line, "protocol", protocol);
    if(protocol > 0 && protocol <= 255) g_state.protocolVersion = static_cast<uint8_t>(protocol);
    const uint32_t local = parseNodeIdToken(id);
    FT02_LoRaNodeRuntimeHostBegin(local != 0 ? local : FT02_LR01_LOCAL_NODE,
                                  "LanternBox FT-02",
                                  node[0] ? node : "FT02");
    Serial.printf("[LR01] BOOT %s\n", line);
}

void parseNav(const char* line)
{
    int32_t fix = 0, fixType = 0, lat = 0, lon = 0, alt = 0;
    int32_t sat = 0, satUsed = 0, satVisible = 0, hdop = 9999;
    int32_t speed = 0, heading = 0, compass = 0, compassQ = 0, timeValid = 0;
    uint32_t unixTime = 0;
    (void)parseInt32(line, "fix", fix);
    (void)parseInt32(line, "fix_type", fixType);
    (void)parseInt32(line, "lat", lat);
    (void)parseInt32(line, "lon", lon);
    (void)parseInt32(line, "alt", alt);
    (void)parseInt32(line, "sat", sat);
    const bool hasSatUsed = parseInt32(line, "sat_used", satUsed);
    const bool hasSatVisible = parseInt32(line, "sat_visible", satVisible);
    (void)parseInt32(line, "hdop", hdop);
    (void)parseInt32(line, "speed", speed);
    (void)parseInt32(line, "heading", heading);
    (void)parseInt32(line, "compass", compass);
    (void)parseInt32(line, "compass_q", compassQ);
    (void)parseUInt32(line, "unix", unixTime);
    (void)parseInt32(line, "time_valid", timeValid);

    if(!hasSatUsed) satUsed = sat;
    if(!hasSatVisible) satVisible = satUsed;

    g_state.fix = fix != 0;
    g_state.fixType = static_cast<uint8_t>(fixType < 0 ? 0 : (fixType > 255 ? 255 : fixType));
    g_state.latitudeE7 = lat;
    g_state.longitudeE7 = lon;
    g_state.altitudeDm = static_cast<int16_t>(alt < -32768 ? -32768 : (alt > 32767 ? 32767 : alt));
    g_state.satUsed = static_cast<uint8_t>(satUsed < 0 ? 0 : (satUsed > 255 ? 255 : satUsed));
    g_state.satVisible = static_cast<uint8_t>(satVisible < 0 ? 0 : (satVisible > 255 ? 255 : satVisible));
    g_state.satellites = g_state.satUsed;
    g_state.hdopX100 = static_cast<uint16_t>(hdop < 0 ? 0 : (hdop > 65535 ? 65535 : hdop));
    g_state.speedCms = static_cast<uint16_t>(speed < 0 ? 0 : (speed > 65535 ? 65535 : speed));
    g_state.headingX10 = static_cast<uint16_t>(heading < 0 ? 0 : (heading > 65535 ? 65535 : heading));
    g_state.compassValid = compass != 0;
    g_state.compassQuality = static_cast<uint8_t>(compassQ < 0 ? 0 : (compassQ > 3 ? 3 : compassQ));
    g_state.unixTime = unixTime;
    g_state.timeValid = timeValid != 0;
    g_state.lastNavMs = millis();

    applyNavigationToCore();

    LR01_LOG_VERBOSE("[LR01] NAV fix=%u type=%u lat=%ld lon=%ld used=%u visible=%u hdop=%.2f speed=%.2fkm/h heading=%.1f compass=%u q=%u time=%lu/%u\n",
                  g_state.fix ? 1u : 0u,
                  static_cast<unsigned>(g_state.fixType),
                  static_cast<long>(g_state.latitudeE7),
                  static_cast<long>(g_state.longitudeE7),
                  static_cast<unsigned>(g_state.satUsed),
                  static_cast<unsigned>(g_state.satVisible),
                  static_cast<double>(g_state.hdopX100) / 100.0,
                  static_cast<double>(g_state.speedCms) * 0.036,
                  static_cast<double>(g_state.headingX10) / 10.0,
                  g_state.compassValid ? 1u : 0u,
                  static_cast<unsigned>(g_state.compassQuality),
                  static_cast<unsigned long>(g_state.unixTime),
                  g_state.timeValid ? 1u : 0u);
}

void parseRadio(const char* line)
{
    int32_t ready = 0, nodes = 0, pki = 0, txQueue = 0;
    uint32_t rx = 0, dup = 0;
    float freq = 0.0f;
    char profile[16] = {};
    (void)parseInt32(line, "ready", ready);
    (void)parseToken(line, "profile", profile, sizeof(profile));
    (void)parseFloat(line, "freq", freq);
    (void)parseUInt32(line, "rx", rx);
    (void)parseInt32(line, "nodes", nodes);
    (void)parseInt32(line, "pki", pki);
    (void)parseUInt32(line, "dup", dup);
    (void)parseInt32(line, "tx_queue", txQueue);

    g_state.loraReady = ready != 0;
    if(profile[0]) snprintf(g_state.radioProfile, sizeof(g_state.radioProfile), "%s", profile);
    g_state.frequencyMHz = freq;
    g_state.rxCount = rx;
    g_state.nodeCount = static_cast<uint16_t>(nodes < 0 ? 0 : (nodes > 65535 ? 65535 : nodes));
    g_state.pkiPeerCount = static_cast<uint16_t>(pki < 0 ? 0 : (pki > 65535 ? 65535 : pki));
    g_state.duplicateCount = dup;
    g_state.txQueue = static_cast<uint16_t>(txQueue < 0 ? 0 : (txQueue > 65535 ? 65535 : txQueue));
    g_state.lastRadioMs = millis();

    FT02_LoRaNodeRuntimeHostSetStatus(g_state.online && g_state.loraReady,
                                      g_state.nodeCount,
                                      g_state.pkiPeerCount);

    LR01_LOG_VERBOSE("[LR01] RADIO ready=%u profile=%s freq=%.3f rx=%lu nodes=%u pki=%u dup=%lu txq=%u\n",
                  g_state.loraReady ? 1u : 0u,
                  g_state.radioProfile,
                  static_cast<double>(g_state.frequencyMHz),
                  static_cast<unsigned long>(g_state.rxCount),
                  static_cast<unsigned>(g_state.nodeCount),
                  static_cast<unsigned>(g_state.pkiPeerCount),
                  static_cast<unsigned long>(g_state.duplicateCount),
                  static_cast<unsigned>(g_state.txQueue));
}

void parseSystem(const char* line)
{
    int32_t gnss = 0, compass = 0, lora = 0;
    uint32_t uptime = 0, gnssBytes = 0, heapBytes = 0, psramBytes = 0;
    uint32_t rxErrors = 0, uartErrors = 0, radioResets = 0;
    (void)parseInt32(line, "gnss", gnss);
    (void)parseInt32(line, "compass", compass);
    (void)parseInt32(line, "lora", lora);
    (void)parseUInt32(line, "uptime", uptime);
    (void)parseUInt32(line, "gnss_bytes", gnssBytes);
    (void)parseUInt32(line, "heap", heapBytes);
    (void)parseUInt32(line, "psram", psramBytes);
    (void)parseUInt32(line, "rx_errors", rxErrors);
    (void)parseUInt32(line, "uart_errors", uartErrors);
    (void)parseUInt32(line, "radio_resets", radioResets);

    g_state.gnssReady = gnss != 0;
    g_state.compassReady = compass != 0;
    g_state.loraReady = lora != 0;
    g_state.uptime = uptime;
    g_state.gnssBytes = gnssBytes;
    g_state.heapBytes = heapBytes;
    g_state.psramBytes = psramBytes;
    g_state.rxErrors = rxErrors;
    g_state.uartErrors = uartErrors;
    g_state.radioResets = radioResets;
    g_state.lastSystemMs = millis();

    applyNavigationToCore();
    FT02_LoRaNodeRuntimeHostSetStatus(g_state.online && g_state.loraReady,
                                      g_state.nodeCount,
                                      g_state.pkiPeerCount);

    LR01_LOG_VERBOSE("[LR01] SYSTEM gnss=%u compass=%u lora=%u uptime=%lu gnss_bytes=%lu heap=%lu psram=%lu rxerr=%lu uarterr=%lu resets=%lu\n",
                  g_state.gnssReady ? 1u : 0u,
                  g_state.compassReady ? 1u : 0u,
                  g_state.loraReady ? 1u : 0u,
                  static_cast<unsigned long>(g_state.uptime),
                  static_cast<unsigned long>(g_state.gnssBytes),
                  static_cast<unsigned long>(g_state.heapBytes),
                  static_cast<unsigned long>(g_state.psramBytes),
                  static_cast<unsigned long>(g_state.rxErrors),
                  static_cast<unsigned long>(g_state.uartErrors),
                  static_cast<unsigned long>(g_state.radioResets));
}

void parseMeshRx(const char* line)
{
    uint32_t airId = 0;
    char fromToken[24] = {};
    char toToken[24] = {};
    char name[32] = {};
    char kind[24] = {};
    char text[FT02_LORA_MESSAGE_TEXT_BYTES] = {};
    float rssi = 0.0f, snr = 0.0f;
    (void)parseUInt32(line, "id", airId);
    (void)parseToken(line, "from", fromToken, sizeof(fromToken));
    (void)parseToken(line, "to", toToken, sizeof(toToken));
    (void)parseQuoted(line, "name", name, sizeof(name));
    (void)parseToken(line, "kind", kind, sizeof(kind));
    (void)parseFloat(line, "rssi", rssi);
    (void)parseFloat(line, "snr", snr);
    (void)parseQuoted(line, "text", text, sizeof(text));
    const uint32_t from = parseNodeIdToken(fromToken);
    const uint32_t to = parseNodeIdToken(toToken);
    const bool pkiText = strcmp(kind, "MESH_PKI_TEXT") == 0;
    const bool userText = strcmp(kind, "MESH_TEXT") == 0 || pkiText;

    if(from != 0)
        FT02_LoRaNodeRuntimeHostObservePeer(from, name, snr, pkiText);

    if(userText && text[0] != '\0')
        FT02_LoRaCommunicationHostReceive(airId, from, to, name, pkiText, rssi, snr, text);

    Serial.printf("[LR01] MESH_RX id=0x%08lX from=!%08lX to=!%08lX name=%s kind=%s rssi=%.1f snr=%.1f text=%s\n",
                  static_cast<unsigned long>(airId),
                  static_cast<unsigned long>(from),
                  static_cast<unsigned long>(to),
                  name, kind, static_cast<double>(rssi), static_cast<double>(snr), text);
}

void parseMeshNode(const char* line)
{
    char idToken[24] = {};
    char longName[32] = {};
    char shortName[16] = {};
    int32_t online = 0, hops = -1, pki = 0;
    uint32_t last = 0;
    float rssi = 0.0f, snr = 0.0f;
    (void)parseToken(line, "id", idToken, sizeof(idToken));
    (void)parseQuoted(line, "long", longName, sizeof(longName));
    (void)parseQuoted(line, "short", shortName, sizeof(shortName));
    (void)parseInt32(line, "online", online);
    (void)parseInt32(line, "hops", hops);
    (void)parseFloat(line, "rssi", rssi);
    (void)parseFloat(line, "snr", snr);
    (void)parseInt32(line, "pki", pki);
    (void)parseUInt32(line, "last", last);
    const uint32_t node = parseNodeIdToken(idToken);
    if(node != 0)
    {
        FT02_LoRaNodeRuntimeHostUpsertPeer(node, longName, shortName,
                                           online != 0, hops, rssi, snr,
                                           pki != 0, last);
    }
}

void parseTxLifecycle(const char* line)
{
    uint32_t id = 0;
    (void)parseUInt32(line, "id", id);
    if(strncmp(line, "MESH_TX_ACCEPTED ", 17) == 0)
    {
        FT02_LoRaCommunicationHostTxAccepted(id);
        Serial.printf("[LR01] TX_ACCEPTED id=0x%08lX\n", static_cast<unsigned long>(id));
        return;
    }
    if(strncmp(line, "MESH_TX_SENT ", 13) == 0)
    {
        FT02_LoRaCommunicationHostTxSent(id);
        Serial.printf("[LR01] TX_SENT id=0x%08lX\n", static_cast<unsigned long>(id));
        return;
    }
    if(strncmp(line, "MESH_TX_FAILED ", 15) == 0)
    {
        char reason[40] = {};
        (void)parseToken(line, "reason", reason, sizeof(reason));
        FT02_LoRaCommunicationHostTxFailed(id, reason);
        Serial.printf("[LR01] TX_FAILED id=0x%08lX reason=%s\n", static_cast<unsigned long>(id), reason);
        return;
    }
}

void parseTxResult(const char* line)
{
    uint32_t id = 0;
    char type[16] = {};
    char reason[40] = {};
    int32_t ok = 0;
    const bool hasId = parseUInt32(line, "id", id);
    (void)parseToken(line, "type", type, sizeof(type));
    (void)parseInt32(line, "ok", ok);
    (void)parseToken(line, "reason", reason, sizeof(reason));
    if(hasId)
        FT02_LoRaCommunicationHostTxResult(id, type, ok != 0, reason);
    else
        FT02_LoRaCommunicationHostTxResult(type, ok != 0); // A1 fallback
    Serial.printf("[LR01] TX_RESULT id=0x%08lX type=%s ok=%u reason=%s\n",
                  static_cast<unsigned long>(id), type, ok != 0 ? 1u : 0u, reason);
}

void parseDelivery(const char* line)
{
    uint32_t id = 0;
    char nodeToken[24] = {};
    char status[16] = {};
    (void)parseUInt32(line, "id", id);
    (void)parseToken(line, "node", nodeToken, sizeof(nodeToken));
    (void)parseToken(line, "status", status, sizeof(status));
    const uint32_t node = parseNodeIdToken(nodeToken);
    const bool ack = strcmp(status, "ACK") == 0;
    const bool timeout = strcmp(status, "TIMEOUT") == 0;
    if(ack || timeout)
        FT02_LoRaCommunicationHostDelivery(id, node, ack);
    Serial.printf("[LR01] DELIVERY id=0x%08lX node=!%08lX status=%s\n",
                  static_cast<unsigned long>(id), static_cast<unsigned long>(node), status);
}


FT02CompassCalState compassCalStateFromToken(const char* token)
{
    if(token == nullptr) return FT02_COMPASS_CAL_UNKNOWN;
    if(strcmp(token, "IDLE") == 0) return FT02_COMPASS_CAL_IDLE;
    if(strcmp(token, "RUNNING") == 0) return FT02_COMPASS_CAL_RUNNING;
    if(strcmp(token, "READY") == 0) return FT02_COMPASS_CAL_READY;
    if(strcmp(token, "SAVED") == 0) return FT02_COMPASS_CAL_SAVED;
    if(strcmp(token, "CANCELED") == 0) return FT02_COMPASS_CAL_CANCELED;
    if(strcmp(token, "FAILED") == 0) return FT02_COMPASS_CAL_FAILED;
    return FT02_COMPASS_CAL_UNKNOWN;
}

void parseCompassCalState(const char* line)
{
    char stateToken[16] = {};
    int32_t progress = 0;
    int32_t quality = 0;
    uint32_t samples = 0;
    int32_t calibrated = 0;
    int32_t minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;

    (void)parseToken(line, "state", stateToken, sizeof(stateToken));
    (void)parseInt32(line, "progress", progress);
    (void)parseInt32(line, "quality", quality);
    (void)parseUInt32(line, "samples", samples);
    (void)parseInt32(line, "calibrated", calibrated);
    (void)parseInt32(line, "min_x", minX);
    (void)parseInt32(line, "max_x", maxX);
    (void)parseInt32(line, "min_y", minY);
    (void)parseInt32(line, "max_y", maxY);
    (void)parseInt32(line, "min_z", minZ);
    (void)parseInt32(line, "max_z", maxZ);

    FT02CompassCalibrationState& cal = g_state.compassCalibration;
    cal.state = compassCalStateFromToken(stateToken);
    cal.progress = static_cast<uint8_t>(progress < 0 ? 0 : (progress > 100 ? 100 : progress));
    cal.quality = static_cast<uint8_t>(quality < 0 ? 0 : (quality > 3 ? 3 : quality));
    cal.samples = samples;
    cal.calibrated = calibrated != 0;
    cal.minX = minX;
    cal.maxX = maxX;
    cal.minY = minY;
    cal.maxY = maxY;
    cal.minZ = minZ;
    cal.maxZ = maxZ;
    cal.lastUpdateMs = millis();
    cal.lastErrorCode = 0;
    cal.lastErrorMessage[0] = '\0';
    ++cal.revision;

    Serial.printf("[LR01-CAL] state=%s progress=%u quality=%u samples=%lu calibrated=%u\\n",
                  stateToken,
                  static_cast<unsigned>(cal.progress),
                  static_cast<unsigned>(cal.quality),
                  static_cast<unsigned long>(cal.samples),
                  cal.calibrated ? 1u : 0u);
}

void parseLine(char* line)
{
    if(line == nullptr) return;
    while(*line == ' ' || *line == '\t') ++line;
    if(*line == '\0') return;
    markRx();

    if(strncmp(line, "LR01_BOOT ", 10) == 0) parseBoot(line);
    else if(strncmp(line, "LR01_READY", 10) == 0)
    {
        int32_t protocol = 0;
        (void)parseInt32(line, "protocol", protocol);
        if(protocol > 0 && protocol <= 255) g_state.protocolVersion = static_cast<uint8_t>(protocol);
        g_state.readySeen = true;
        g_state.online = true;
        Serial.printf("[LR01] READY protocol=%u\n", static_cast<unsigned>(g_state.protocolVersion));
        FT02_LoRaNodeRuntimeHostBegin(FT02_LR01_LOCAL_NODE, "LanternBox FT-02", "FT02");
        ++g_state.statusRequestSequence;
        (void)FT02_LR01HostRequestStatus(g_state.statusRequestSequence);
        (void)FT02_LR01HostRequestNodes();
    }
    else if(strncmp(line, "LR01_PONG_", 10) == 0)
    {
        const uint32_t seq = static_cast<uint32_t>(strtoul(line + 10, nullptr, 10));
        if(seq == g_state.pingSequence)
        {
            g_state.lastPongMs = millis();
            g_state.online = true;
        }
        if(seq != g_state.pingSequence)
        {
            LR01_LOG_NORMAL("[LR01] PONG mismatch seq=%lu expected=%lu\n",
                            static_cast<unsigned long>(seq),
                            static_cast<unsigned long>(g_state.pingSequence));
        }
        else
        {
            LR01_LOG_VERBOSE("[LR01] PONG seq=%lu\n",
                             static_cast<unsigned long>(seq));
        }
    }
    else if(strncmp(line, "NAV_STATE ", 10) == 0) parseNav(line);
    else if(strncmp(line, "RADIO_STATE ", 12) == 0) parseRadio(line);
    else if(strncmp(line, "SYSTEM_STATE ", 13) == 0) parseSystem(line);
    else if(strncmp(line, "COMPASS_CAL_STATE ", 18) == 0) parseCompassCalState(line);
    else if(strncmp(line, "MESH_RX ", 8) == 0) parseMeshRx(line);
    else if(strncmp(line, "MESH_NODE ", 10) == 0) parseMeshNode(line);
    else if(strncmp(line, "MESH_NODE_END ", 14) == 0)
    {
        int32_t count = 0;
        (void)parseInt32(line, "count", count);
        g_state.listedNodeCount = static_cast<uint16_t>(count < 0 ? 0 : (count > 65535 ? 65535 : count));
        g_state.lastNodeListMs = millis();
        Serial.printf("[LR01] NODE_LIST complete count=%u\n", static_cast<unsigned>(g_state.listedNodeCount));
    }
    else if(strncmp(line, "MESH_TX_ACCEPTED ", 17) == 0 ||
            strncmp(line, "MESH_TX_SENT ", 13) == 0 ||
            strncmp(line, "MESH_TX_FAILED ", 15) == 0) parseTxLifecycle(line);
    else if(strncmp(line, "MESH_TX_RESULT ", 15) == 0) parseTxResult(line);
    else if(strncmp(line, "MESH_DELIVERY ", 14) == 0) parseDelivery(line);
    else if(strncmp(line, "STATUS_END", 10) == 0)
    {
        uint32_t id = 0;
        (void)parseUInt32(line, "id", id);
        g_state.lastStatusEndId = id;
        g_state.lastStatusCompleteMs = millis();
        Serial.printf("[LR01] STATUS_END id=%lu\n", static_cast<unsigned long>(id));
    }
    else if(strncmp(line, "LR01_ERR ", 9) == 0)
    {
        int32_t code = 0;
        char message[64] = {};
        (void)parseInt32(line, "code", code);
        (void)parseQuoted(line, "message", message, sizeof(message));
        if(code >= 20 && code <= 25)
        {
            FT02CompassCalibrationState& cal = g_state.compassCalibration;
            cal.lastErrorCode = code;
            snprintf(cal.lastErrorMessage, sizeof(cal.lastErrorMessage), "%s", message);
            cal.lastUpdateMs = millis();
            ++cal.revision;
        }
        Serial.printf("[LR01] ERROR code=%ld message=%s\n", static_cast<long>(code), message);
    }
    else
    {
        Serial.printf("[LR01] RX unhandled: %s\n", line);
    }
}
}

void FT02_LR01HostBegin()
{
    memset(&g_state, 0, sizeof(g_state));
    snprintf(g_state.radioProfile, sizeof(g_state.radioProfile), "--");
    g_state.hdopX100 = 9999;
    if(!g_lr01Serial.setRxBufferSize(FT02_LR01_RX_BUFFER_BYTES))
    {
        Serial.printf("[LR01-HOST-A2] WARN RX buffer request failed bytes=%u\n",
                      static_cast<unsigned>(FT02_LR01_RX_BUFFER_BYTES));
    }
    g_lr01Serial.begin(FT02_LR01_BAUD, SERIAL_8N1, FT02_LR01_RX_PIN, FT02_LR01_TX_PIN);
    g_lineLength = 0;
    g_dropUntilLf = false;
    g_lastPingMs = millis();
    Serial.printf("[LR01-HOST-A2] ready RX=%d TX=%d baud=%lu rxbuf=%u log=%d protocol=ASCII/UTF8-LF\n",
                  FT02_LR01_RX_PIN, FT02_LR01_TX_PIN,
                  static_cast<unsigned long>(FT02_LR01_BAUD),
                  static_cast<unsigned>(FT02_LR01_RX_BUFFER_BYTES),
                  FT02_LR01_LOG_LEVEL);
}

void FT02_LR01HostPoll()
{
    while(g_lr01Serial.available() > 0)
    {
        const char c = static_cast<char>(g_lr01Serial.read());
        if(c == '\n')
        {
            if(!g_dropUntilLf)
            {
                while(g_lineLength > 0 && g_line[g_lineLength - 1] == '\r') --g_lineLength;
                g_line[g_lineLength] = '\0';
                if(g_lineLength > 0) parseLine(g_line);
            }
            g_lineLength = 0;
            g_dropUntilLf = false;
        }
        else if(c != '\r' && !g_dropUntilLf)
        {
            if(g_lineLength + 1 < sizeof(g_line)) g_line[g_lineLength++] = c;
            else
            {
                Serial.println("[LR01] RX line overflow; dropping until LF");
                g_lineLength = 0;
                g_dropUntilLf = true;
            }
        }
    }

    const uint32_t now = millis();
    if(static_cast<uint32_t>(now - g_lastPingMs) >= FT02_LR01_PING_INTERVAL_MS)
    {
        g_lastPingMs = now;
        ++g_state.pingSequence;
        char ping[40];
        snprintf(ping, sizeof(ping), "CORE_PING_%lu", static_cast<unsigned long>(g_state.pingSequence));
        (void)sendLine(ping);
    }

    if(g_state.online && g_state.lastRxMs != 0 &&
       static_cast<uint32_t>(now - g_state.lastRxMs) > FT02_LR01_ONLINE_TIMEOUT_MS)
    {
        g_state.online = false;
        g_state.loraReady = false;
        FT02_LoRaNodeRuntimeHostSetStatus(false, g_state.nodeCount, g_state.pkiPeerCount);
        Serial.println("[LR01] offline: host UART silent >3500ms");
    }
}

const FT02LR01State& FT02_LR01HostState() { return g_state; }
bool FT02_LR01HostOnline() { return g_state.online; }
bool FT02_LR01HostRadioReady() { return g_state.online && g_state.loraReady; }
uint32_t FT02_LR01HostRxLineCount() { return g_state.lineCount; }

bool FT02_LR01HostRequestStatus()
{
    ++g_state.statusRequestSequence;
    return FT02_LR01HostRequestStatus(g_state.statusRequestSequence);
}

bool FT02_LR01HostRequestStatus(uint32_t requestId)
{
    if(requestId == 0) return sendLine("CORE_STATUS?");
    char line[48];
    snprintf(line, sizeof(line), "CORE_STATUS? id=%lu", static_cast<unsigned long>(requestId));
    return sendLine(line);
}

bool FT02_LR01HostRequestNodeInfo() { return sendLine("MESH_NODEINFO"); }
bool FT02_LR01HostRequestNodes() { return sendLine("MESH_NODES?"); }

bool FT02_LR01HostSendBroadcast(uint32_t hostId, const char* utf8Text)
{
    if(hostId == 0 || utf8Text == nullptr || utf8Text[0] == '\0') return false;
    const size_t n = safeUtf8Prefix(utf8Text, FT02_LR01_TX_TEXT_MAX_BYTES);
    if(n == 0) return false;
    char line[224];
    const int prefix = snprintf(line, sizeof(line), "MESH_TX id=%lu ", static_cast<unsigned long>(hostId));
    if(prefix <= 0 || static_cast<size_t>(prefix) + n >= sizeof(line)) return false;
    memcpy(line + prefix, utf8Text, n);
    line[prefix + n] = '\0';
    return sendLine(line);
}

bool FT02_LR01HostSendPrivate(uint32_t hostId, uint32_t nodeId, const char* utf8Text)
{
    if(hostId == 0 || nodeId == 0 || utf8Text == nullptr || utf8Text[0] == '\0') return false;
    const size_t n = safeUtf8Prefix(utf8Text, FT02_LR01_TX_TEXT_MAX_BYTES);
    if(n == 0) return false;
    char line[240];
    const int prefix = snprintf(line, sizeof(line), "MESH_PRIVATE id=%lu !%08lX ",
                                static_cast<unsigned long>(hostId), static_cast<unsigned long>(nodeId));
    if(prefix <= 0 || static_cast<size_t>(prefix) + n >= sizeof(line)) return false;
    memcpy(line + prefix, utf8Text, n);
    line[prefix + n] = '\0';
    return sendLine(line);
}

bool FT02_LR01HostSendBroadcast(const char* utf8Text)
{
    if(utf8Text == nullptr || utf8Text[0] == '\0') return false;
    const size_t n = safeUtf8Prefix(utf8Text, FT02_LR01_TX_TEXT_MAX_BYTES);
    if(n == 0) return false;
    char line[192];
    const int prefix = snprintf(line, sizeof(line), "MESH_TX ");
    if(prefix <= 0 || static_cast<size_t>(prefix) + n >= sizeof(line)) return false;
    memcpy(line + prefix, utf8Text, n);
    line[prefix + n] = '\0';
    return sendLine(line);
}

bool FT02_LR01HostSendPrivate(uint32_t nodeId, const char* utf8Text)
{
    if(nodeId == 0 || utf8Text == nullptr || utf8Text[0] == '\0') return false;
    const size_t n = safeUtf8Prefix(utf8Text, FT02_LR01_TX_TEXT_MAX_BYTES);
    if(n == 0) return false;
    char line[220];
    const int prefix = snprintf(line, sizeof(line), "MESH_PRIVATE !%08lX ", static_cast<unsigned long>(nodeId));
    if(prefix <= 0 || static_cast<size_t>(prefix) + n >= sizeof(line)) return false;
    memcpy(line + prefix, utf8Text, n);
    line[prefix + n] = '\0';
    return sendLine(line);
}

const char* FT02_LR01HostCompassCalStateText(FT02CompassCalState state)
{
    switch(state)
    {
        case FT02_COMPASS_CAL_IDLE: return "IDLE";
        case FT02_COMPASS_CAL_RUNNING: return "RUNNING";
        case FT02_COMPASS_CAL_READY: return "READY";
        case FT02_COMPASS_CAL_SAVED: return "SAVED";
        case FT02_COMPASS_CAL_CANCELED: return "CANCELED";
        case FT02_COMPASS_CAL_FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

bool FT02_LR01HostCompassCalRequestStatus() { return sendLine("COMPASS_CAL_STATUS?"); }
bool FT02_LR01HostCompassCalStart() { return sendLine("COMPASS_CAL_START"); }
bool FT02_LR01HostCompassCalSave() { return sendLine("COMPASS_CAL_SAVE"); }
bool FT02_LR01HostCompassCalCancel() { return sendLine("COMPASS_CAL_CANCEL"); }
bool FT02_LR01HostCompassCalReset() { return sendLine("COMPASS_CAL_RESET"); }
