#include "FT02_FontPackRenderer.h"

uint32_t FT02_ReadCodepoint(
    const char* text,
    int* usedBytes
)
{
    uint8_t c0 = (uint8_t)text[0];

    if(c0 < 0x80)
    {
        *usedBytes = 1;
        return c0;
    }

    if((c0 & 0xE0) == 0xC0)
    {
        *usedBytes = 2;
        return ((uint32_t)(c0 & 0x1F) << 6) |
               ((uint8_t)text[1] & 0x3F);
    }

    if((c0 & 0xF0) == 0xE0)
    {
        *usedBytes = 3;
        return ((uint32_t)(c0 & 0x0F) << 12) |
               (((uint8_t)text[1] & 0x3F) << 6) |
               ((uint8_t)text[2] & 0x3F);
    }

    if((c0 & 0xF8) == 0xF0)
    {
        *usedBytes = 4;
        return ((uint32_t)(c0 & 0x07) << 18) |
               (((uint8_t)text[1] & 0x3F) << 12) |
               (((uint8_t)text[2] & 0x3F) << 6) |
               ((uint8_t)text[3] & 0x3F);
    }

    *usedBytes = 1;
    return c0;
}

static const uint8_t* FT02_FindGlyphRecord(
    const FT02FontPack& font,
    uint32_t codepoint
)
{
    int low = 0;
    int high = (int)font.count - 1;

    while(low <= high)
    {
        int mid = (low + high) / 2;
        uint32_t cp = font.index[mid].codepoint;

        if(cp == codepoint)
        {
            uint32_t offset = font.index[mid].offset;

            if(offset < font.dataLength)
            {
                return font.data + offset;
            }

            return nullptr;
        }

        if(cp < codepoint)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    return nullptr;
}

static void FT02_DrawGlyphRecord(
    FT02Display& display,
    const uint8_t* rec,
    int cursorX,
    int baselineY
)
{
    uint8_t width = rec[0];
    uint8_t height = rec[1];
    uint8_t bytesPerRow = rec[2];
    int8_t xOffset = (int8_t)rec[3];
    int8_t yOffset = (int8_t)rec[4];

    const uint8_t* bitmap = rec + 8;

    int drawX = cursorX + xOffset;
    int drawY = baselineY + yOffset;

    for(int row = 0; row < height; row++)
    {
        for(int col = 0; col < width; col++)
        {
            int byteIndex = row * bytesPerRow + col / 8;
            uint8_t data = bitmap[byteIndex];
            uint8_t mask = 0x80 >> (col % 8);

            if(data & mask)
            {
                display.drawPixel(
                    drawX + col,
                    drawY + row,
                    GxEPD_BLACK
                );
            }
        }
    }
}

int FT02_TextWidthPack(
    const FT02FontPack& font,
    const char* text
)
{
    int width = 0;

    while(*text)
    {
        int used = 0;
        uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);

        if(rec)
        {
            width += rec[5] + 2;
        }
        else
        {
            width += 12;
        }

        text += used;
    }

    return width;
}

void FT02_DrawTextPack(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    int cursorX = x;

    while(*text)
    {
        int used = 0;
        uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);

        if(rec)
        {
            FT02_DrawGlyphRecord(
                display,
                rec,
                cursorX,
                baselineY
            );

            cursorX += rec[5] + 2;
        }
        else
        {
            cursorX += 12;
        }

        text += used;
    }
}


void FT02_DrawTextPackInvert(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    int cursorX = x;

    while(*text)
    {
        int used = 0;
        uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);

        if(rec)
        {
            uint8_t width = rec[0];
            uint8_t height = rec[1];
            uint8_t bytesPerRow = rec[2];
            int8_t xOffset = (int8_t)rec[3];
            int8_t yOffset = (int8_t)rec[4];
            const uint8_t* bitmap = rec + 8;

            for(int row = 0; row < height; row++)
            {
                for(int col = 0; col < width; col++)
                {
                    int index = row * bytesPerRow + col / 8;
                    uint8_t mask = 0x80 >> (col % 8);

                    if(bitmap[index] & mask)
                    {
                        display.drawPixel(
                            cursorX + xOffset + col,
                            baselineY + yOffset + row,
                            GxEPD_WHITE
                        );
                    }
                }
            }

            cursorX += rec[5] + 2;
        }

        text += used;
    }
}
