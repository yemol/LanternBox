#include "FT02_LoRaCoprocessor.h"

namespace
{
constexpr int FT02_LORA_RESET_PIN = 6;
constexpr uint32_t FT02_LORA_RESET_LOW_MS = 180u;
constexpr uint32_t FT02_LORA_RESET_SETTLE_MS = 20u;

uint32_t g_lastReleaseMs = 0;
uint32_t g_resetCount = 0;
bool g_started = false;
}

void FT02_LoRaCoprocessorBegin()
{
    if(g_started) return;
    g_started = true;

    // Open-drain is intentional: FT-02 may only pull CHIP_PU low. Releasing
    // the line lets the radio board's own pull-up restore the high level.
    pinMode(FT02_LORA_RESET_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(FT02_LORA_RESET_PIN, HIGH);
    delay(FT02_LORA_RESET_SETTLE_MS);

    Serial.printf(
        "[LORA-RST] control ready GPIO%d open-drain (LOW=reset, HIGH=release)\n",
        FT02_LORA_RESET_PIN
    );
}

void FT02_LoRaCoprocessorReset(const char* reason)
{
    FT02_LoRaCoprocessorBegin();

    digitalWrite(FT02_LORA_RESET_PIN, LOW);
    delay(FT02_LORA_RESET_LOW_MS);
    digitalWrite(FT02_LORA_RESET_PIN, HIGH);

    g_lastReleaseMs = millis();
    ++g_resetCount;

    Serial.printf(
        "[LORA-RST] pulse #%lu reason=%s low_ms=%lu released\n",
        static_cast<unsigned long>(g_resetCount),
        reason != nullptr ? reason : "unspecified",
        static_cast<unsigned long>(FT02_LORA_RESET_LOW_MS)
    );
}

uint32_t FT02_LoRaCoprocessorLastReleaseMs()
{
    return g_lastReleaseMs;
}

uint32_t FT02_LoRaCoprocessorResetCount()
{
    return g_resetCount;
}
