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

// Quiesce the e-paper drive after a short idle period following fast partial
// FieldManual navigation. Call from the main loop while Knowledge is active.
FT02KnowledgeAction FT02_KnowledgeService(
    FT02Display& display,
    uint32_t nowMs
);

// True while FieldManual owns a free-form text entry surface. Global
// single-letter shortcuts such as H=Help must not pre-empt raw letters here.
bool FT02_KnowledgeTextInputActive();

FT02KnowledgeAction FT02_HandleKnowledgeInput(
    FT02Display& display,
    const FT02InputEvent& event
);
