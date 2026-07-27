#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"

enum FT02MapTileLoadResult
{
    FT02_MAP_TILE_OK = 0,
    FT02_MAP_TILE_MISSING,
    FT02_MAP_TILE_OPEN_FAILED,
    FT02_MAP_TILE_BAD_HEADER,
    FT02_MAP_TILE_READ_FAILED
};

FT02MapTileLoadResult FT02_DrawMapTile(
    FT02Display& display,
    const char* path,
    int screenX,
    int screenY
);

const char* FT02_MapTileLoadResultText(
    FT02MapTileLoadResult result
);
