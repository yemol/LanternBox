#include "FT02_PbfMapUI.h"

#include "FT02_PbfMapRuntime.h"
#include "FT02_RoadNameFont.h"
#include "FT02_FooterFont20.h"
#include "FT02_StatusBar.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const int MAP_TOP = 76;
static const int MAP_HEIGHT = 364;
static const int MAP_BOTTOM = 440;
static const int MAP_WIDTH = 800;

struct FT02LabelRect
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

static void FT02_MapText(
    FT02Display& display,
    const char* text,
    int x,
    int y,
    uint8_t size
)
{
    display.setFont(nullptr);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(size);
    display.setCursor(x, y);
    display.print(text);
}

static int FT02_MapTextWidth(const char* text, uint8_t size)
{
    return text == nullptr ? 0 : (int)strlen(text) * 6 * size;
}

static void FT02_MapCentered(
    FT02Display& display,
    const char* text,
    int x,
    int y,
    int width,
    uint8_t size
)
{
    int textX = x + (width - FT02_MapTextWidth(text, size)) / 2;
    if(textX < x + 4) textX = x + 4;
    FT02_MapText(display, text, textX, y, size);
}

static uint8_t FT02_LineCode(int x, int y)
{
    uint8_t code = 0;
    if(x < 0) code |= 1;
    else if(x >= MAP_WIDTH) code |= 2;
    if(y < 0) code |= 4;
    else if(y >= MAP_HEIGHT) code |= 8;
    return code;
}

static bool FT02_ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    uint8_t c1 = FT02_LineCode(x1, y1);
    uint8_t c2 = FT02_LineCode(x2, y2);
    while(true)
    {
        if((c1 | c2) == 0) return true;
        if((c1 & c2) != 0) return false;
        uint8_t out = c1 ? c1 : c2;
        int x = 0;
        int y = 0;
        if(out & 8)
        {
            if(y2 == y1) return false;
            y = MAP_HEIGHT - 1;
            x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        }
        else if(out & 4)
        {
            if(y2 == y1) return false;
            y = 0;
            x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        }
        else if(out & 2)
        {
            if(x2 == x1) return false;
            x = MAP_WIDTH - 1;
            y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
        }
        else
        {
            if(x2 == x1) return false;
            x = 0;
            y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
        }
        if(out == c1)
        {
            x1 = x; y1 = y; c1 = FT02_LineCode(x1, y1);
        }
        else
        {
            x2 = x; y2 = y; c2 = FT02_LineCode(x2, y2);
        }
    }
}

static void FT02_DrawLocalLine(
    FT02Display& display,
    int x1,
    int y1,
    int x2,
    int y2
)
{
    if(!FT02_ClipLine(x1, y1, x2, y2)) return;
    display.drawLine(x1, y1 + MAP_TOP, x2, y2 + MAP_TOP, GxEPD_BLACK);
}

static void FT02_DrawThickLine(
    FT02Display& display,
    int x1,
    int y1,
    int x2,
    int y2,
    int width
)
{
    FT02_DrawLocalLine(display, x1, y1, x2, y2);
    if(width <= 1) return;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    bool mostlyHorizontal = dx >= dy;
    if(mostlyHorizontal)
    {
        FT02_DrawLocalLine(display, x1, y1 - 1, x2, y2 - 1);
        if(width >= 3) FT02_DrawLocalLine(display, x1, y1 + 1, x2, y2 + 1);
    }
    else
    {
        FT02_DrawLocalLine(display, x1 - 1, y1, x2 - 1, y2);
        if(width >= 3) FT02_DrawLocalLine(display, x1 + 1, y1, x2 + 1, y2);
    }
}

static void FT02_DrawPathLine(
    FT02Display& display,
    int x1,
    int y1,
    int x2,
    int y2
)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if(steps <= 0) return;
    for(int i = 0; i <= steps; i += 5)
    {
        int end = i + 2;
        if(end > steps) end = steps;
        int ax = x1 + dx * i / steps;
        int ay = y1 + dy * i / steps;
        int bx = x1 + dx * end / steps;
        int by = y1 + dy * end / steps;
        FT02_DrawLocalLine(display, ax, ay, bx, by);
    }
}

static void FT02_DrawMapGeometry(FT02Display& display)
{
    const FT02PbfMapSegment* segments = FT02_PbfMapSegments();
    size_t count = FT02_PbfMapSegmentCount();

    for(uint8_t pass = FT02_PBF_MAP_STYLE_BUILDING; pass >= FT02_PBF_MAP_STYLE_MINOR; pass--)
    {
        for(size_t i = 0; i < count; i++)
        {
            const FT02PbfMapSegment& segment = segments[i];
            if(segment.style != pass) continue;
            if(segment.style == FT02_PBF_MAP_STYLE_PATH)
                FT02_DrawPathLine(display, segment.x1, segment.y1, segment.x2, segment.y2);
            else if(segment.style == FT02_PBF_MAP_STYLE_MAJOR)
                FT02_DrawThickLine(display, segment.x1, segment.y1, segment.x2, segment.y2, 3);
            else if(segment.style == FT02_PBF_MAP_STYLE_MEDIUM)
                FT02_DrawThickLine(display, segment.x1, segment.y1, segment.x2, segment.y2, 2);
            else
                FT02_DrawThickLine(display, segment.x1, segment.y1, segment.x2, segment.y2, 1);
        }
        if(pass == FT02_PBF_MAP_STYLE_MINOR) break;
    }

    for(size_t i = 0; i < count; i++)
    {
        const FT02PbfMapSegment& segment = segments[i];
        if(segment.style != FT02_PBF_MAP_STYLE_RAIL && segment.style != FT02_PBF_MAP_STYLE_WATER) continue;
        FT02_DrawThickLine(display, segment.x1, segment.y1, segment.x2, segment.y2, 1);
    }
}

static bool FT02_Utf8Next(const char*& cursor, uint32_t& codepoint)
{
    const uint8_t* s = (const uint8_t*)cursor;
    if(*s == 0) return false;
    if(s[0] < 0x80)
    {
        codepoint = s[0]; cursor += 1; return true;
    }
    if((s[0] & 0xE0) == 0xC0 && s[1])
    {
        codepoint = ((uint32_t)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        cursor += 2; return true;
    }
    if((s[0] & 0xF0) == 0xE0 && s[1] && s[2])
    {
        codepoint = ((uint32_t)(s[0] & 0x0F) << 12)
            | ((uint32_t)(s[1] & 0x3F) << 6)
            | (s[2] & 0x3F);
        cursor += 3; return true;
    }
    if((s[0] & 0xF8) == 0xF0 && s[1] && s[2] && s[3])
    {
        codepoint = ((uint32_t)(s[0] & 0x07) << 18)
            | ((uint32_t)(s[1] & 0x3F) << 12)
            | ((uint32_t)(s[2] & 0x3F) << 6)
            | (s[3] & 0x3F);
        cursor += 4; return true;
    }
    codepoint = '?'; cursor += 1; return true;
}

static int FT02_RoadGlyphIndex(uint32_t codepoint)
{
    int low = 0;
    int high = (int)FT02_ROAD_GLYPH_COUNT - 1;
    while(low <= high)
    {
        int mid = (low + high) / 2;
        uint32_t value = pgm_read_dword(&FT02_ROAD_GLYPHS[mid].codepoint);
        if(value == codepoint) return mid;
        if(value < codepoint) low = mid + 1;
        else high = mid - 1;
    }
    if(codepoint != '?') return FT02_RoadGlyphIndex('?');
    return -1;
}

static int FT02_RoadTextGlyphCount(const char* text, int maxGlyphs)
{
    int count = 0;
    const char* cursor = text;
    uint32_t cp = 0;
    while(count < maxGlyphs && FT02_Utf8Next(cursor, cp)) count++;
    return count;
}

static void FT02_DrawRoadGlyph(FT02Display& display, uint32_t codepoint, int x, int y)
{
    int index = FT02_RoadGlyphIndex(codepoint);
    if(index < 0) return;
    for(int row = 0; row < 16; row++)
    {
        uint16_t bits = ((uint16_t)pgm_read_byte(&FT02_ROAD_GLYPHS[index].rows[row * 2]) << 8)
            | pgm_read_byte(&FT02_ROAD_GLYPHS[index].rows[row * 2 + 1]);
        for(int col = 0; col < 16; col++)
            if(bits & (1U << (15 - col))) display.drawPixel(x + col, y + row, GxEPD_BLACK);
    }
}

static bool FT02_RectsOverlap(const FT02LabelRect& a, const FT02LabelRect& b)
{
    return !(a.x + a.w < b.x || b.x + b.w < a.x || a.y + a.h < b.y || b.y + b.h < a.y);
}

static void FT02_DrawRoadLabels(FT02Display& display)
{
    const FT02PbfMapLabel* labels = FT02_PbfMapLabels();
    size_t count = FT02_PbfMapLabelCount();
    FT02LabelRect used[24];
    size_t usedCount = 0;

    for(size_t i = 0; i < count && usedCount < 24; i++)
    {
        const FT02PbfMapLabel& label = labels[i];
        int glyphs = FT02_RoadTextGlyphCount(label.text, 8);
        if(glyphs <= 0) continue;
        FT02LabelRect box;
        box.w = glyphs * 16 + 6;
        box.h = 20;
        box.x = label.x - box.w / 2;
        box.y = label.y - box.h / 2;
        if(box.x < 2 || box.y < 2 || box.x + box.w >= MAP_WIDTH - 2 || box.y + box.h >= MAP_HEIGHT - 2) continue;
        bool collision = false;
        for(size_t j = 0; j < usedCount; j++)
        {
            FT02LabelRect padded = used[j];
            padded.x -= 4; padded.y -= 3; padded.w += 8; padded.h += 6;
            if(FT02_RectsOverlap(box, padded)) { collision = true; break; }
        }
        if(collision) continue;

        display.fillRect(box.x, box.y + MAP_TOP, box.w, box.h, GxEPD_WHITE);
        int gx = box.x + 3;
        const char* cursor = label.text;
        uint32_t cp = 0;
        for(int n = 0; n < glyphs && FT02_Utf8Next(cursor, cp); n++)
        {
            FT02_DrawRoadGlyph(display, cp, gx, box.y + MAP_TOP + 2);
            gx += 16;
        }
        used[usedCount++] = box;
    }
}

static void FT02_DrawZoomSymbol(
    FT02Display& display,
    int x,
    int y,
    bool plus
)
{
    const int centerX = x + 28;
    const int centerY = y + 28;
    const int symbolLength = 26;
    const int symbolThickness = 4;
    const int halfLength = symbolLength / 2;
    const int halfThickness = symbolThickness / 2;

    display.fillRect(
        centerX - halfLength,
        centerY - halfThickness,
        symbolLength,
        symbolThickness,
        GxEPD_BLACK
    );

    if(plus)
    {
        display.fillRect(
            centerX - halfThickness,
            centerY - halfLength,
            symbolThickness,
            symbolLength,
            GxEPD_BLACK
        );
    }
}

static void FT02_DrawZoomControl(
    FT02Display& display,
    int x,
    int y,
    bool plus
)
{
    const int width = 56;
    const int height = 56;
    display.fillRect(x, y, width, height, GxEPD_WHITE);
    display.drawRect(x, y, width, height, GxEPD_BLACK);
    display.drawRect(x + 1, y + 1, width - 2, height - 2, GxEPD_BLACK);
    display.drawRect(x + 2, y + 2, width - 4, height - 4, GxEPD_BLACK);
    FT02_DrawZoomSymbol(display, x, y, plus);
}

static void FT02_DrawMapOverlay(FT02Display& display)
{
    // Frozen absolute positions from the approved map layout.
    FT02_DrawZoomControl(display, 18, 196, true);
    FT02_DrawZoomControl(display, 18, 264, false);
    const int cx = 400;
    const int cy = MAP_TOP + MAP_HEIGHT / 2;
    display.drawCircle(cx, cy, 7, GxEPD_BLACK);
    display.drawLine(cx - 12, cy, cx + 12, cy, GxEPD_BLACK);
    display.drawLine(cx, cy - 12, cx, cy + 12, GxEPD_BLACK);
}

static int FT02_MapScaleMeters(int zoom)
{
    if(zoom >= 20) return 20;
    if(zoom == 19) return 50;
    if(zoom == 18) return 100;
    if(zoom == 17) return 200;
    return 500;
}

// The map footer uses a dedicated 20 px bitmap font. It is intentionally
// between the old 16 px road font and the oversized 32 px scaled footer.
// Chinese glyphs use a 22 px advance; ASCII uses a compact 12 px advance.
static const FT02FooterGlyph20* FT02_FindFooterGlyph20(uint32_t codepoint)
{
    for(size_t i = 0; i < FT02_FOOTER_GLYPH_COUNT_20; i++)
    {
        if(pgm_read_dword(&FT02_FOOTER_GLYPHS_20[i].codepoint) == codepoint)
            return &FT02_FOOTER_GLYPHS_20[i];
    }
    return nullptr;
}

static int FT02_FooterAdvance20(uint32_t codepoint)
{
    if(codepoint == 0x20U) return 8;
    const FT02FooterGlyph20* glyph = FT02_FindFooterGlyph20(codepoint);
    return glyph == nullptr ? 0 : (int)pgm_read_byte(&glyph->advance);
}

static void FT02_DrawFooterGlyph20(
    FT02Display& display,
    const FT02FooterGlyph20* glyph,
    int x,
    int y
)
{
    if(glyph == nullptr) return;
    const int width = (int)pgm_read_byte(&glyph->width);
    for(int row = 0; row < 20; row++)
    {
        const uint32_t bits = pgm_read_dword(&glyph->rows[row]);
        for(int col = 0; col < width; col++)
        {
            if(bits & (1UL << (19 - col)))
                display.drawPixel(x + col, y + row, GxEPD_BLACK);
        }
    }
}

static int FT02_FooterTextWidth20(const char* text)
{
    int width = 0;
    const char* cursor = text;
    uint32_t cp = 0;
    while(FT02_Utf8Next(cursor, cp)) width += FT02_FooterAdvance20(cp);
    return width;
}

static void FT02_DrawFooterText20(
    FT02Display& display,
    const char* text,
    int x
)
{
    const char* cursor = text;
    uint32_t cp = 0;
    while(FT02_Utf8Next(cursor, cp))
    {
        const int advance = FT02_FooterAdvance20(cp);
        if(cp != 0x20U)
            FT02_DrawFooterGlyph20(display, FT02_FindFooterGlyph20(cp), x, 451);
        x += advance;
    }
}

static void FT02_DrawMapStatusCellChinese(
    FT02Display& display,
    int cellIndex,
    const char* text
)
{
    const int cellWidth = MAP_WIDTH / 5;
    const int cellX = cellIndex * cellWidth;
    const int textWidth = FT02_FooterTextWidth20(text);
    int textX = cellX + (cellWidth - textWidth) / 2;
    if(textX < cellX + 4) textX = cellX + 4;
    FT02_DrawFooterText20(display, text, textX);
}

static void FT02_DrawReadyBottom(FT02Display& display)
{
    const FT02PbfMapReport& report = FT02_PbfMapReportCurrent();
    display.fillRect(0, MAP_BOTTOM, MAP_WIDTH, 40, GxEPD_WHITE);
    display.fillRect(0, MAP_BOTTOM, MAP_WIDTH, 4, GxEPD_BLACK);
    display.drawLine(0, 479, MAP_WIDTH - 1, 479, GxEPD_BLACK);

    for(int i = 1; i < 5; i++)
    {
        const int x = i * (MAP_WIDTH / 5);
        display.drawLine(x, MAP_BOTTOM + 4, x, 478, GxEPD_BLACK);
    }

    char scaleText[32];
    snprintf(
        scaleText,
        sizeof(scaleText),
        "Z%d %d米",
        report.zoom,
        FT02_MapScaleMeters(report.zoom)
    );

    FT02_DrawMapStatusCellChinese(display, 0, "速度 --");
    FT02_DrawMapStatusCellChinese(display, 1, "方向 --");
    FT02_DrawMapStatusCellChinese(display, 2, "海拔 --");
    FT02_DrawMapStatusCellChinese(display, 3, "距离 --");
    FT02_DrawMapStatusCellChinese(display, 4, scaleText);
}

static void FT02_DrawLoading(FT02Display& display)
{
    const FT02PbfMapReport& report = FT02_PbfMapReportCurrent();
    display.fillRect(0, MAP_TOP, 800, MAP_HEIGHT, GxEPD_WHITE);
    display.drawRect(70, 145, 660, 220, GxEPD_BLACK);
    display.drawRect(74, 149, 652, 212, GxEPD_BLACK);
    FT02_MapCentered(display, "DIRECT PBF MAP A3.13", 80, 184, 640, 3);
    FT02_MapCentered(display, "LOADING REGIONAL CACHE OR BUILDING IT FROM RAW PBF", 80, 238, 640, 1);
    FT02_MapCentered(display, "FIRST REGION IS SLOW; LATER PAN AND ZOOM USE CACHE", 80, 272, 640, 1);

    char position[96];
    snprintf(position, sizeof(position), "CENTER %.5f %.5f   Z%d", report.centerLon, report.centerLat, report.zoom);
    FT02_MapCentered(display, position, 80, 310, 640, 1);

    display.fillRect(0, MAP_BOTTOM, 800, 40, GxEPD_WHITE);
    display.fillRect(0, MAP_BOTTOM, 800, 3, GxEPD_BLACK);
    FT02_MapCentered(display, "SERIAL MONITOR SHOWS LIVE CACHE PROGRESS", 0, 455, 800, 1);
}

static void FT02_DrawError(FT02Display& display)
{
    const FT02PbfMapReport& report = FT02_PbfMapReportCurrent();
    display.fillRect(0, MAP_TOP, 800, MAP_HEIGHT, GxEPD_WHITE);
    display.drawRect(70, 150, 660, 190, GxEPD_BLACK);
    FT02_MapCentered(display, "DIRECT PBF MAP A3.13 FAILED", 80, 190, 640, 2);
    FT02_MapCentered(display, FT02_PbfMapErrorText(), 80, 240, 640, 2);

    char detail[128];
    snprintf(detail, sizeof(detail), "NODES %lu  WAYS %lu  CACHE SEG %lu  FREE PSRAM %.1fM",
        (unsigned long)report.nodesKept,
        (unsigned long)report.waysScanned,
        (unsigned long)report.cachedSegments,
        report.freePsramAfter / (1024.0 * 1024.0));
    FT02_MapCentered(display, detail, 80, 290, 640, 1);

    display.fillRect(0, MAP_BOTTOM, 800, 40, GxEPD_WHITE);
    display.fillRect(0, MAP_BOTTOM, 800, 3, GxEPD_BLACK);
    FT02_MapCentered(display, "ENTER RETRY    R RECENTER    B BACK", 0, 455, 800, 1);
}

void FT02_DrawPbfMapScreen(FT02Display& display)
{
    const FT02PbfMapReport& report = FT02_PbfMapReportCurrent();

    Serial.printf(
        "[MAP-A3.13] render begin state=%d zoom=%d center=%.7f,%.7f\n",
        (int)report.state,
        report.zoom,
        report.centerLon,
        report.centerLat
    );

    if(report.state == FT02_PBF_MAP_READY)
    {
        // The home/help footer is reliable because those pages use a full
        // display update. The map footer is now put on the same proven path:
        // redraw the status bar, map and footer in one full-window transaction.
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);
            FT02_DrawStatusBar(display);
            FT02_DrawMapGeometry(display);
            FT02_DrawRoadLabels(display);
            FT02_DrawMapOverlay(display);
            FT02_DrawReadyBottom(display);
        }
        while(display.nextPage());

        Serial.printf(
            "[MAP-A3.13] FULL refresh complete; balanced 20px Chinese footer drawn zoom=%d scale=%dm\n",
            report.zoom,
            FT02_MapScaleMeters(report.zoom)
        );
        return;
    }

    // Loading and error states keep the existing below-header partial update.
    display.setPartialWindow(0, MAP_TOP, MAP_WIDTH, 480 - MAP_TOP);
    display.firstPage();
    do
    {
        display.fillRect(0, MAP_TOP, MAP_WIDTH, 480 - MAP_TOP, GxEPD_WHITE);
        if(report.state == FT02_PBF_MAP_ERROR)
            FT02_DrawError(display);
        else
            FT02_DrawLoading(display);
    }
    while(display.nextPage());
    display.setFullWindow();
}
