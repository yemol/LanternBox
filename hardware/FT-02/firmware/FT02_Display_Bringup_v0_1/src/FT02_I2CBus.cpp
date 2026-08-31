#include "FT02_I2CBus.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace
{
StaticSemaphore_t g_ft02I2CBusMutexStorage;
SemaphoreHandle_t g_ft02I2CBusMutex = nullptr;
}

void FT02_I2CBusInit()
{
    if(g_ft02I2CBusMutex == nullptr)
    {
        g_ft02I2CBusMutex = xSemaphoreCreateMutexStatic(&g_ft02I2CBusMutexStorage);
    }
}

bool FT02_I2CBusLock(uint32_t timeoutMs)
{
    if(g_ft02I2CBusMutex == nullptr) FT02_I2CBusInit();
    if(g_ft02I2CBusMutex == nullptr) return false;
    return xSemaphoreTake(
        g_ft02I2CBusMutex,
        pdMS_TO_TICKS(timeoutMs)
    ) == pdTRUE;
}

void FT02_I2CBusUnlock()
{
    if(g_ft02I2CBusMutex != nullptr)
    {
        xSemaphoreGive(g_ft02I2CBusMutex);
    }
}
