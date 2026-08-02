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

FT02KnowledgeAction FT02_HandleKnowledgeInput(
    FT02Display& display,
    const FT02InputEvent& event
);
