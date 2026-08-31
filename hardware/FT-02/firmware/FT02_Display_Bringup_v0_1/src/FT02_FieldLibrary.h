#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

enum FT02FieldLibraryAction
{
    FT02_LIBRARY_ACTION_NONE = 0,
    FT02_LIBRARY_ACTION_EXIT_HOME
};

// FT-02 Field Library v0.8.2 Clean BW Mainline / Reading Comfort / Fit-Page Figures / Persistent Resume
// - P0-first offline Chinese library
// - native import workflow for EPUB/TXT/text-PDF/scanned-PDF
// - unified 1-bit Media Reader for EPUB illustrations and scanned PDF pages
// - EPUB is normalized by tools/build_ft02_field_library.py into chapter cache
// - table of contents, reading progress and simple full-library query
// Data root on SD: /library
// Catalog v0.2:
// id<TAB>title<TAB>author<TAB>category<TAB>priority<TAB>format<TAB>root<TAB>toc
void FT02_FieldLibraryBegin(bool forceReload = false);
void FT02_FieldLibraryReset();
void FT02_DrawFieldLibraryScreen(FT02Display& display);
FT02FieldLibraryAction FT02_HandleFieldLibraryInput(
    FT02Display& display,
    const FT02InputEvent& event
);

bool FT02_FieldLibraryReady();
int FT02_FieldLibraryBookCount();
