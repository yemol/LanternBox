#include "FT02_FontPackRenderer.h"

namespace
{

static const int FT02_MISSING_GLYPH_ADVANCE = 18;
static const int FT02_GLYPH_TRACKING = 1;

static const uint8_t* FT02_FindGlyphRecord(
    const FT02FontPack& font,
    uint32_t codepoint
)
{
    int low = 0;
    int high = (int)font.count - 1;

    while(low <= high)
    {
        const int mid = (low + high) / 2;
        const uint32_t cp = font.index[mid].codepoint;

        if(cp == codepoint)
        {
            const uint32_t offset = font.index[mid].offset;
            return offset < font.dataLength ? font.data + offset : nullptr;
        }

        if(cp < codepoint) low = mid + 1;
        else high = mid - 1;
    }

    return nullptr;
}

static void FT02_DrawGlyphRecordColor(
    FT02Display& display,
    const uint8_t* rec,
    int cursorX,
    int baselineY,
    uint16_t ink,
    bool syntheticBold
)
{
    const uint8_t width = rec[0];
    const uint8_t height = rec[1];
    const uint8_t bytesPerRow = rec[2];
    const int8_t xOffset = (int8_t)rec[3];
    const int8_t yOffset = (int8_t)rec[4];
    const uint8_t* bitmap = rec + 8;

    const int drawX = cursorX + xOffset;
    const int drawY = baselineY + yOffset;

    for(int row = 0; row < height; row++)
    {
        for(int col = 0; col < width; col++)
        {
            const int byteIndex = row * bytesPerRow + col / 8;
            const uint8_t mask = 0x80 >> (col % 8);
            if(bitmap[byteIndex] & mask)
            {
                display.drawPixel(drawX + col, drawY + row, ink);
                if(syntheticBold)
                {
                    display.drawPixel(drawX + col + 1, drawY + row, ink);
                }
            }
        }
    }
}

static void FT02_DrawMissingGlyph(
    FT02Display& display,
    int cursorX,
    int baselineY,
    uint16_t ink,
    bool syntheticBold
)
{
    const int x = cursorX + 1;
    const int y = baselineY - 18;
    display.drawRect(x, y, 15, 18, ink);
    display.drawLine(x + 3, y + 3, x + 11, y + 14, ink);
    display.drawLine(x + 11, y + 3, x + 3, y + 14, ink);
    if(syntheticBold)
    {
        display.drawLine(x + 1, y, x + 1, y + 17, ink);
    }
}

static void FT02_DrawTextPackInternal(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    uint16_t ink,
    bool syntheticBold
)
{
    int cursorX = x;

    while(*text)
    {
        int used = 0;
        const uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);

        if(rec)
        {
            FT02_DrawGlyphRecordColor(
                display,
                rec,
                cursorX,
                baselineY,
                ink,
                syntheticBold
            );
            cursorX += rec[5] + FT02_GLYPH_TRACKING;
        }
        else
        {
            FT02_DrawMissingGlyph(
                display,
                cursorX,
                baselineY,
                ink,
                syntheticBold
            );
            cursorX += FT02_MISSING_GLYPH_ADVANCE;
        }

        text += used;
    }
}


static bool FT02_PointInsideClip(
    int x,
    int y,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    return x >= clipX &&
           y >= clipY &&
           x < clipX + clipW &&
           y < clipY + clipH;
}

static void FT02_DrawGlyphRecordClipped(
    FT02Display& display,
    const uint8_t* rec,
    int cursorX,
    int baselineY,
    uint16_t ink,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    const uint8_t width = rec[0];
    const uint8_t height = rec[1];
    const uint8_t bytesPerRow = rec[2];
    const int8_t xOffset = (int8_t)rec[3];
    const int8_t yOffset = (int8_t)rec[4];
    const uint8_t* bitmap = rec + 8;

    const int drawX = cursorX + xOffset;
    const int drawY = baselineY + yOffset;

    for(int row = 0; row < height; row++)
    {
        const int py = drawY + row;
        if(py < clipY || py >= clipY + clipH) continue;

        for(int col = 0; col < width; col++)
        {
            const int px = drawX + col;
            if(px < clipX || px >= clipX + clipW) continue;

            const int byteIndex = row * bytesPerRow + col / 8;
            const uint8_t mask = 0x80 >> (col % 8);
            if(bitmap[byteIndex] & mask)
            {
                display.drawPixel(px, py, ink);
            }
        }
    }
}

static void FT02_DrawMissingGlyphClipped(
    FT02Display& display,
    int cursorX,
    int baselineY,
    uint16_t ink,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    const int x = cursorX + 1;
    const int y = baselineY - 18;

    for(int px = x; px <= x + 15; px++)
    {
        if(FT02_PointInsideClip(px, y, clipX, clipY, clipW, clipH))
            display.drawPixel(px, y, ink);
        if(FT02_PointInsideClip(px, y + 18, clipX, clipY, clipW, clipH))
            display.drawPixel(px, y + 18, ink);
    }
    for(int py = y; py <= y + 18; py++)
    {
        if(FT02_PointInsideClip(x, py, clipX, clipY, clipW, clipH))
            display.drawPixel(x, py, ink);
        if(FT02_PointInsideClip(x + 15, py, clipX, clipY, clipW, clipH))
            display.drawPixel(x + 15, py, ink);
    }
    for(int i = 0; i <= 12; i++)
    {
        const int px1 = x + 2 + i;
        const int py1 = y + 3 + i;
        const int px2 = x + 13 - i;
        if(FT02_PointInsideClip(px1, py1, clipX, clipY, clipW, clipH))
            display.drawPixel(px1, py1, ink);
        if(FT02_PointInsideClip(px2, py1, clipX, clipY, clipW, clipH))
            display.drawPixel(px2, py1, ink);
    }
}

static void FT02_DrawTextPackClippedInternal(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    uint16_t ink,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    int cursorX = x;

    while(*text)
    {
        int used = 0;
        const uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);

        if(rec)
        {
            FT02_DrawGlyphRecordClipped(
                display,
                rec,
                cursorX,
                baselineY,
                ink,
                clipX,
                clipY,
                clipW,
                clipH
            );
            cursorX += rec[5] + FT02_GLYPH_TRACKING;
        }
        else
        {
            FT02_DrawMissingGlyphClipped(
                display,
                cursorX,
                baselineY,
                ink,
                clipX,
                clipY,
                clipW,
                clipH
            );
            cursorX += FT02_MISSING_GLYPH_ADVANCE;
        }

        text += used;
        if(cursorX >= clipX + clipW + FT02_MISSING_GLYPH_ADVANCE) break;
    }
}

} // namespace

uint32_t FT02_ReadCodepoint(
    const char* text,
    int* usedBytes
)
{
    const uint8_t c0 = (uint8_t)text[0];

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

bool FT02_HasGlyphPack(
    const FT02FontPack& font,
    uint32_t codepoint
)
{
    return FT02_FindGlyphRecord(font, codepoint) != nullptr;
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
        const uint32_t cp = FT02_ReadCodepoint(text, &used);
        const uint8_t* rec = FT02_FindGlyphRecord(font, cp);
        width += rec ? rec[5] + FT02_GLYPH_TRACKING : FT02_MISSING_GLYPH_ADVANCE;
        text += used;
    }

    return width;
}

int FT02_CodepointAdvancePack(
    const FT02FontPack& font,
    uint32_t codepoint
)
{
    const uint8_t* rec = FT02_FindGlyphRecord(font, codepoint);
    return rec ? rec[5] + FT02_GLYPH_TRACKING : FT02_MISSING_GLYPH_ADVANCE;
}

void FT02_DrawTextPackClipped(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    FT02_DrawTextPackClippedInternal(
        display,
        font,
        text,
        x,
        baselineY,
        GxEPD_BLACK,
        clipX,
        clipY,
        clipW,
        clipH
    );
}

void FT02_DrawTextPackInvertClipped(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    int clipX,
    int clipY,
    int clipW,
    int clipH
)
{
    FT02_DrawTextPackClippedInternal(
        display,
        font,
        text,
        x,
        baselineY,
        GxEPD_WHITE,
        clipX,
        clipY,
        clipW,
        clipH
    );
}

void FT02_DrawTextPack(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    FT02_DrawTextPackInternal(display, font, text, x, baselineY, GxEPD_BLACK, false);
}

void FT02_DrawTextPackBold(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    // Deliberately avoid synthetic one-pixel emboldening on CJK glyphs.
    // At 1-bit resolution it produces uneven, blocky strokes. Emphasis is
    // provided by card inversion, spacing, rules, and hierarchy instead.
    FT02_DrawTextPackInternal(display, font, text, x, baselineY, GxEPD_BLACK, false);
}

void FT02_DrawTextPackInvert(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    FT02_DrawTextPackInternal(display, font, text, x, baselineY, GxEPD_WHITE, false);
}

void FT02_DrawTextPackInvertBold(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
)
{
    FT02_DrawTextPackInternal(display, font, text, x, baselineY, GxEPD_WHITE, false);
}
