#pragma once

#include <Arduino.h>

// Shared FT-02 text input mode.
// All text-entry surfaces use the same Chinese/English state and the same
// CardKB2 long-SYM IME direct toggle (0xF1). This prevents each page from inventing its own
// mode state or resetting the user's choice when moving between pages.
enum FT02TextInputMode : uint8_t
{
    FT02_TEXT_INPUT_CN = 0,
    FT02_TEXT_INPUT_EN
};

bool FT02_TextInputModeIsChinese();
FT02TextInputMode FT02_TextInputModeToggle();
