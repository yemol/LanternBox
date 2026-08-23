#pragma once

#include <stdint.h>

// Persistent FieldManual reading history.
// IDs, rather than array positions, are stored on SD so a rebuilt knowledge
// pack may reorder cards without making the recent list point at wrong cards.
constexpr int FT02_FIELD_MANUAL_RECENT_MAX = 16;
constexpr const char* FT02_FIELD_MANUAL_RECENT_PATH = "/lanternbox/field_manual/recent_v1.tsv";

void FT02_FieldManualRecentBegin();
void FT02_FieldManualRecentPoll(uint32_t nowMs);

int FT02_FieldManualRecentCount();
int FT02_FieldManualRecentGlobalIndexAt(int recentIndex);
int FT02_FieldManualRecentPageAt(int recentIndex);

// Record/open a card. Existing entries move to the front. Page is zero-based.
void FT02_FieldManualRecentRecord(int globalCardIndex, int page);

// Update the saved page without changing the card identity. The entry is kept
// at the front because page changes can only happen in the currently open card.
void FT02_FieldManualRecentUpdatePage(int globalCardIndex, int page);

bool FT02_FieldManualRecentFlush();
