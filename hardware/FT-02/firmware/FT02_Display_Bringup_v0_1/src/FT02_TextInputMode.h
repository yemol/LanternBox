#pragma once

#include <Arduino.h>

// Shared FT-02 text input mode.
// All text-entry surfaces use the same Chinese/English state and the same
// CardKB2 Sym+W (`) toggle. This prevents each page from inventing its own
// mode state or resetting the user's choice when moving between pages.
enum FT02TextInputMode : uint8_t
{
    FT02_TEXT_INPUT_CN = 0,
    FT02_TEXT_INPUT_EN
};

FT02TextInputMode FT02_TextInputModeCurrent();
bool FT02_TextInputModeIsChinese();
FT02TextInputMode FT02_TextInputModeToggle();
void FT02_TextInputModeSet(FT02TextInputMode mode);
