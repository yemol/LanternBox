#pragma once

// FieldManual full-text search over the already-loaded card index plus card
// section text streamed from cards.dat. Results are kept in a small bounded
// RAM array and refer to existing global card indices.
constexpr int FT02_FIELD_MANUAL_SEARCH_MAX_RESULTS = 64;
constexpr int FT02_FIELD_MANUAL_SEARCH_MAX_QUERY_BYTES = 96;

bool FT02_FieldManualSearchExecute(const char* queryUtf8);
int FT02_FieldManualSearchResultCount();
int FT02_FieldManualSearchGlobalIndexAt(int resultIndex);
void FT02_FieldManualSearchClear();
