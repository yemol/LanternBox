#pragma once

#include "FT02_StatusBar.h"

enum FT02MapLoadingReason : uint8_t
{
    FT02_MAP_LOADING_GENERIC = 0,
    FT02_MAP_LOADING_SEARCH,
    FT02_MAP_LOADING_PAN,
    FT02_MAP_LOADING_ZOOM,
    FT02_MAP_LOADING_NAVIGATION
};

void FT02_PbfMapSetLoadingReason(FT02MapLoadingReason reason);
void FT02_PbfMapSetFollowGnss(bool enabled);
void FT02_DrawPbfMapScreen(FT02Display& display);
bool FT02_RefreshPbfMapNavigationPartial(
    FT02Display& display,
    double oldLon,
    double oldLat,
    double newLon,
    double newLat
);
