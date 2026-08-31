#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

enum FT02KnowledgeAction
{
    FT02_KNOWLEDGE_ACTION_NONE = 0,
    FT02_KNOWLEDGE_ACTION_EXIT_HOME
};

void FT02_KnowledgeReset();

void FT02_DrawKnowledgeScreen(
    FT02Display& display
);

// Service deferred FieldManual work such as recent-reading persistence and
// coalesced search redraws. Global EPD runtime owns BUSY input and power-off.
FT02KnowledgeAction FT02_KnowledgeService(
    FT02Display& display,
    uint32_t nowMs
);

// Consume the oldest BUSY-captured input before the main loop polls any newer
// live key. Navigation bursts may be coalesced, but text input remains strict
// FIFO. outProcessed is true when one or more queued events were consumed.
FT02KnowledgeAction FT02_KnowledgeProcessQueuedInput(
    FT02Display& display,
    bool* outProcessed
);

// True while FieldManual owns a free-form text entry surface. Global
// single-letter shortcuts such as H=Help must not pre-empt raw letters here.
bool FT02_KnowledgeTextInputActive();

FT02KnowledgeAction FT02_HandleKnowledgeInput(
    FT02Display& display,
    const FT02InputEvent& event
);
