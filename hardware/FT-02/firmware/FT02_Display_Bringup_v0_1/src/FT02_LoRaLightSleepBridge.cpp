#include "FT02_LoRaLightSleepBridge.h"
#include "FT02_Log.h"

#ifndef FT02_LORA_LIGHT_SLEEP_TRIAL
#define FT02_LORA_LIGHT_SLEEP_TRIAL 0
#endif
#if FT02_LORA_LIGHT_SLEEP_TRIAL
#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include "FT02_MeshService.h"
#include "FT02_LoRaHardwareSpi.h"

namespace {
constexpr gpio_num_t LORA_DIO1 = static_cast<gpio_num_t>(FT02_LoRaHardwareSpi::DIO1);
bool g_active = false;
bool g_gpioArmed = false;
uint32_t g_gpioWakeCount = 0;
uint32_t g_armErrors = 0;
uint32_t g_highStallCount = 0;
void unarm() {
    if (!g_gpioArmed) return;
    (void)gpio_wakeup_disable(LORA_DIO1);
    (void)esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    g_gpioArmed = false;
}
}

void FT02_LoRaLightSleepEnter() {
    // Do not change radio/CPU policy if the receive probe has not initialized.
    g_active = FT02_MeshServiceSleepAvailable();
    g_gpioArmed = false;
    g_gpioWakeCount = g_armErrors = g_highStallCount = 0;
    if (g_active) FT02_MeshServiceSleepEnter();
}
void FT02_LoRaLightSleepBeforeSlice() {
    if (!g_active) return;
    FT02_MeshServiceSleepService();
    // A stuck high DIO would instantly wake on every light-sleep entry.
    // Degrade to the existing 1-second timer slice, not a busy wake loop.
    if (digitalRead(FT02_LoRaHardwareSpi::DIO1) == HIGH) {
        ++g_highStallCount;
        unarm();
        return;
    }
    if (!g_gpioArmed) {
        const esp_err_t pin = gpio_wakeup_enable(LORA_DIO1, GPIO_INTR_HIGH_LEVEL);
        if (pin != ESP_OK) { ++g_armErrors; return; }
        const esp_err_t source = esp_sleep_enable_gpio_wakeup();
        if (source != ESP_OK) {
            ++g_armErrors;
            (void)gpio_wakeup_disable(LORA_DIO1);
            return;
        }
        g_gpioArmed = true;
    }
}
void FT02_LoRaLightSleepAfterSlice(bool enteredSleep) {
    if (!g_active) return;
    if (enteredSleep && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) ++g_gpioWakeCount;
    // Run the FIFO/IRQ service silently, before keyboard polling or next slice.
    // This is still the original calling thread: no additional SPI bus owner.
    FT02_MeshServiceSleepService();
}
void FT02_LoRaLightSleepExit() {
    unarm();
    if (!g_active) return;
    g_active = false;
    // Called only AFTER USB Serial has been restored by the existing wake path.
    FT02_LOGD("[LORA-SLEEP] WAKE gpio=%lu arm_err=%lu dio_high_fallback=%lu\n",
                  static_cast<unsigned long>(g_gpioWakeCount),
                  static_cast<unsigned long>(g_armErrors),
                  static_cast<unsigned long>(g_highStallCount));
    FT02_MeshServiceSleepExit();
}
#else
void FT02_LoRaLightSleepEnter() {}
void FT02_LoRaLightSleepBeforeSlice() {}
void FT02_LoRaLightSleepAfterSlice(bool) {}
void FT02_LoRaLightSleepExit() {}
#endif
