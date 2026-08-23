#pragma once

// Persistent FieldManual favorites/bookmarks.
// Favorite membership is stored by stable card ID so knowledge-pack reordering
// does not silently point a bookmark at a different action card.

constexpr const char* FT02_FIELD_MANUAL_FAVORITES_PATH =
    "/lanternbox/field_manual/favorites_v1.tsv";

void FT02_FieldManualFavoritesBegin();

int FT02_FieldManualFavoritesCount();
int FT02_FieldManualFavoritesGlobalIndexAt(int favoriteIndex);
bool FT02_FieldManualFavoritesIsFavorite(int globalCardIndex);

// Persist immediately using an atomic temp/backup snapshot. `favorite` is the
// complete requested state. Existing favorites are kept in newest-first order;
// re-adding an already-favorited card is a no-op.
bool FT02_FieldManualFavoritesSet(int globalCardIndex, bool favorite);

// Convenience toggle. `outFavorite` receives the persisted state on success.
bool FT02_FieldManualFavoritesToggle(int globalCardIndex, bool* outFavorite = nullptr);
