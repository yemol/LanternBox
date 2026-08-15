#pragma once

#include "FT02_LocationLog.h"
#include "FT02_StatusBar.h"

// Loads all route points for the selected session, selects the highest useful
// detail zoom in Z16-Z18, centers the route bounds, and prepares the mini map.
// Z18 is the maximum allowed magnification; wider routes may step down to Z17/Z16.
// Routes wider than the Z16 viewport remain truthfully aligned and are clipped.
bool FT02_PrepareLocationLogDetailMap(uint16_t selectedNewestIndex);
void FT02_ReleaseLocationLogDetailMap();

void FT02_DrawLocationLogListScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
);
void FT02_DrawLocationLogListBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
);
void FT02_DrawLocationLogDetailScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
);
void FT02_DrawLocationLogDetailBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
);
void FT02_DrawLocationLogDeleteConfirmScreen(
    FT02Display& display,
    const FT02LocationLogEntry& entry
);
