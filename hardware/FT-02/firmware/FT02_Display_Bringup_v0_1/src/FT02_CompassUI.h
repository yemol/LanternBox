#pragma once

#include "FT02_FontPackRenderer.h"

void FT02_CompassUIOpen();
void FT02_DrawCompassScreen(FT02Display& display);
void FT02_CompassUIService(FT02Display& display, uint32_t nowMs);
