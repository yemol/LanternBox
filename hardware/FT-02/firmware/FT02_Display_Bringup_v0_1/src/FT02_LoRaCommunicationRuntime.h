#pragma once

#include <Arduino.h>

constexpr size_t FT02_LORA_MESSAGE_TEXT_BYTES = 192;
constexpr size_t FT02_LORA_USER_TEXT_MAX_BYTES = 120;

enum FT02LoRaTxState : uint8_t
{
    FT02_LORA_TX_NONE = 0,
    FT02_LORA_TX_SENT,
    FT02_LORA_TX_ACKED,
    FT02_LORA_TX_NAKED,
    FT02_LORA_TX_FAILED
};

struct FT02LoRaMessageView
{
    bool valid;
    uint32_t from;
    uint32_t to;
    uint32_t packetId;
    bool broadcast;
    bool pkiEncrypted;
    char text[FT02_LORA_MESSAGE_TEXT_BYTES];
    bool hasRxTime;
    uint32_t rxTimeEpoch;
    bool hasRssi;
    int32_t rssi;
    bool hasSnr;
    float snr;
    bool hasHops;
    uint8_t hops;
};

struct FT02LoRaTxStatusView
{
    bool valid;
    uint32_t packetId;
    uint32_t destination;
    bool broadcast;
    bool pkiEncrypted;
    FT02LoRaTxState state;
    uint32_t sentAtMs;
    uint32_t completedAtMs;
    uint32_t routingError;
    char preview[64];
};

// Called once for every complete FromRadio protobuf frame. This is a pure
// upper-layer parser; UART/framing/reset remain owned by FT02_LoRaTransport.
void FT02_LoRaCommunicationRuntimeOnFromRadio(const uint8_t* payload, uint16_t length);
void FT02_LoRaCommunicationRuntimeResetSession();

bool FT02_LoRaCommunicationSendBroadcast(const char* userText, bool attachFreshGnss);
bool FT02_LoRaCommunicationSendPrivate(uint32_t destination, const char* userText, bool attachFreshGnss);

size_t FT02_LoRaCommunicationMessageCount();
bool FT02_LoRaCommunicationGetMessageNewest(size_t newestIndex, FT02LoRaMessageView& out);
uint16_t FT02_LoRaCommunicationUnreadCount();
void FT02_LoRaCommunicationMarkAllRead();

uint32_t FT02_LoRaCommunicationRevision();
uint32_t FT02_LoRaCommunicationRxTextCount();
uint32_t FT02_LoRaCommunicationTxCount();
uint32_t FT02_LoRaCommunicationDuplicateCount();
uint32_t FT02_LoRaCommunicationAckCount();
uint32_t FT02_LoRaCommunicationNakCount();
uint32_t FT02_LoRaCommunicationLastRxPacketId();
bool FT02_LoRaCommunicationGetLastTx(FT02LoRaTxStatusView& out);

const char* FT02_LoRaCommunicationTxStateText(FT02LoRaTxState state);
const char* FT02_LoRaCommunicationRoutingErrorText(uint32_t error);
