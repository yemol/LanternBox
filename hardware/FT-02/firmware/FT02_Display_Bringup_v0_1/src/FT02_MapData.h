#pragma once

#include <Arduino.h>

enum FT02MapState
{
    FT02_MAP_STATE_NOT_STARTED = 0,
    FT02_MAP_STATE_READY,
    FT02_MAP_STATE_SD_NOT_READY,
    FT02_MAP_STATE_CONFIG_MISSING,
    FT02_MAP_STATE_CONFIG_INVALID
};

struct FT02MapConfig
{
    char name[48];
    char attribution[64];

    int zoom;
    int minX;
    int maxX;
    int minY;
    int maxY;
    int startX;
    int startY;
};

void FT02_MapDataBegin();

bool FT02_MapDataReload();

FT02MapState FT02_MapStateCurrent();

const FT02MapConfig& FT02_MapConfigCurrent();

const char* FT02_MapStateText();

bool FT02_MapBuildTilePath(
    int zoom,
    int tileX,
    int tileY,
    char* output,
    size_t outputSize
);
