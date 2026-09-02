#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "FT02_CoreLink.h"
#include "FT02_CorePortalModel.h"

void FT02_CorePortalClientBegin();
void FT02_CorePortalClientReset();
void FT02_CorePortalClientOpenHome();
void FT02_CorePortalClientRefreshFromCoreLink();
bool FT02_CorePortalClientMove(int8_t delta);
bool FT02_CorePortalClientMoveGrid(int8_t deltaRow, int8_t deltaCol);
bool FT02_CorePortalClientSelectCurrent();
const FT02CorePortalPage& FT02_CorePortalClientPage();
const char* FT02_CorePortalClientMessage();
