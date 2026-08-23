#include "FT02_TextInputMode.h"

namespace
{
FT02TextInputMode g_mode = FT02_TEXT_INPUT_CN;
}


bool FT02_TextInputModeIsChinese()
{
    return g_mode == FT02_TEXT_INPUT_CN;
}

FT02TextInputMode FT02_TextInputModeToggle()
{
    g_mode = (g_mode == FT02_TEXT_INPUT_CN) ? FT02_TEXT_INPUT_EN : FT02_TEXT_INPUT_CN;
    return g_mode;
}

