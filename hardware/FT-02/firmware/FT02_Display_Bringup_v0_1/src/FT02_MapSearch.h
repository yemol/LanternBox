#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

// FT-02 Map Search A4 - Disk Index
//
// Search data is split into a tiny bucket directory (.search.idx) and a
// fixed-record data file (.search.dat). The directory stays in normal RAM;
// query execution seeks only the relevant bucket in the data file and streams
// records through a small bounded buffer. No full search index is loaded into
// PSRAM, so map regional-cache memory remains available.

static constexpr const char* FT02_MAP_SEARCH_INDEX_PATH =
    "/maps/raw/shanghai-260726.osm.search.idx";
static constexpr const char* FT02_MAP_SEARCH_DATA_PATH =
    "/maps/raw/shanghai-260726.osm.search.dat";

static constexpr size_t FT02_MAP_SEARCH_QUERY_BYTES = 72;
static constexpr size_t FT02_MAP_SEARCH_RESULT_COUNT = 8;

struct FT02MapSearchResult
{
    char name[72];
    char detail[72];
    double latitude;
    double longitude;
};

enum FT02MapSearchAction : uint8_t
{
    FT02_MAP_SEARCH_ACTION_NONE = 0,
    FT02_MAP_SEARCH_ACTION_REDRAW,
    FT02_MAP_SEARCH_ACTION_EXIT_MAP,
    FT02_MAP_SEARCH_ACTION_JUMP
};

void FT02_MapSearchOpen();
void FT02_DrawMapSearchScreen(FT02Display& display);
FT02MapSearchAction FT02_MapSearchHandleInput(const FT02InputEvent& event);
bool FT02_MapSearchTakeDeferredRedraw(uint32_t nowMs);
bool FT02_MapSearchTakeJump(double& longitude, double& latitude, char* name, size_t nameBytes);
