#pragma once

#include "FT02_AudioLog.h"
#include "FT02_StatusBar.h"

void FT02_DrawAudioLogListScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
);

void FT02_DrawAudioLogListBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
);

void FT02_DrawAudioLogRecordingBodyPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
);

void FT02_DrawAudioLogRecordingTimerPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
);

void FT02_DrawAudioLogPlayingBodyPartial(
    FT02Display& display,
    const FT02AudioLogEntry& entry
);

void FT02_DrawAudioLogPlayingStatusPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
);

void FT02_DrawAudioLogDeleteConfirmScreen(
    FT02Display& display,
    const FT02AudioLogEntry& entry
);
