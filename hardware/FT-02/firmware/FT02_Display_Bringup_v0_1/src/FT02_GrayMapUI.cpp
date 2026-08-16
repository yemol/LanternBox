#include "FT02_GrayMapUI.h"

#include "FT02_Gray4Panel.h"
#include "FT02_Gnss.h"
#include "FT02_PbfMapRuntime.h"
#include "FT02_GrayMapFontData.h"
#include "FT02_StatusBar.h"
#include "FT02_IconData.h"
#include "FT02_BatteryIcon.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
constexpr int MAP_X = 0;
constexpr int MAP_Y = 76;
constexpr int MAP_W = 800;
constexpr int MAP_H = 364;
constexpr int FOOTER_Y = 440;
constexpr int FOOTER_H = 40;

class GrayCanvas
{
public:
    explicit GrayCanvas(uint8_t* frame) : frame_(frame) {}

    bool valid() const { return frame_ != nullptr; }

    void clear(uint8_t level)
    {
        if(frame_ == nullptr) return;
        const uint8_t value = static_cast<uint8_t>((level & 0x03U) * 0x55U);
        memset(frame_, value, FT02_GRAY4_FRAME_PACKED_BYTES);
    }

    void pixel(int x, int y, uint8_t level)
    {
        if(frame_ == nullptr || x < 0 || y < 0 || x >= FT02_GRAY4_WIDTH || y >= FT02_GRAY4_HEIGHT)
            return;
        // The normal FT-02 UI uses display.setRotation(2). The raw SSD1677
        // four-gray path bypasses Adafruit_GFX, so apply the same 180-degree
        // mounting transform once at the framebuffer boundary. All map and
        // UI drawing code remains in the normal logical 800x480 coordinates.
        const int physicalX = FT02_GRAY4_WIDTH - 1 - x;
        const int physicalY = FT02_GRAY4_HEIGHT - 1 - y;
        const size_t pixelIndex =
            static_cast<size_t>(physicalY) * FT02_GRAY4_WIDTH +
            static_cast<size_t>(physicalX);
        const size_t byteIndex = pixelIndex >> 2U;
        const uint8_t shift = static_cast<uint8_t>(6U - ((pixelIndex & 0x03U) * 2U));
        const uint8_t mask = static_cast<uint8_t>(0x03U << shift);
        frame_[byteIndex] = static_cast<uint8_t>(
            (frame_[byteIndex] & static_cast<uint8_t>(~mask)) |
            static_cast<uint8_t>((level & 0x03U) << shift)
        );
    }

    void fillRect(int x, int y, int w, int h, uint8_t level)
    {
        if(w <= 0 || h <= 0) return;
        int x0 = x < 0 ? 0 : x;
        int y0 = y < 0 ? 0 : y;
        int x1 = x + w;
        int y1 = y + h;
        if(x1 > FT02_GRAY4_WIDTH) x1 = FT02_GRAY4_WIDTH;
        if(y1 > FT02_GRAY4_HEIGHT) y1 = FT02_GRAY4_HEIGHT;
        for(int py = y0; py < y1; ++py)
            for(int px = x0; px < x1; ++px)
                pixel(px, py, level);
    }

    void line(int x0, int y0, int x1, int y1, uint8_t level)
    {
        int dx = abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;
        for(;;)
        {
            pixel(x0, y0, level);
            if(x0 == x1 && y0 == y1) break;
            const int e2 = error * 2;
            if(e2 >= dy) { error += dy; x0 += sx; }
            if(e2 <= dx) { error += dx; y0 += sy; }
        }
    }

    void rect(int x, int y, int w, int h, uint8_t level)
    {
        if(w <= 0 || h <= 0) return;
        line(x, y, x + w - 1, y, level);
        line(x, y + h - 1, x + w - 1, y + h - 1, level);
        line(x, y, x, y + h - 1, level);
        line(x + w - 1, y, x + w - 1, y + h - 1, level);
    }

    void circle(int cx, int cy, int radius, uint8_t level)
    {
        if(radius <= 0) return;
        int x = radius;
        int y = 0;
        int error = 1 - x;
        while(x >= y)
        {
            pixel(cx + x, cy + y, level); pixel(cx + y, cy + x, level);
            pixel(cx - y, cy + x, level); pixel(cx - x, cy + y, level);
            pixel(cx - x, cy - y, level); pixel(cx - y, cy - x, level);
            pixel(cx + y, cy - x, level); pixel(cx + x, cy - y, level);
            ++y;
            if(error < 0) error += 2 * y + 1;
            else { --x; error += 2 * (y - x) + 1; }
        }
    }

    void fillCircle(int cx, int cy, int radius, uint8_t level)
    {
        for(int y = -radius; y <= radius; ++y)
        {
            const int span = static_cast<int>(sqrt(static_cast<double>(radius * radius - y * y)));
            line(cx - span, cy + y, cx + span, cy + y, level);
        }
    }

private:
    uint8_t* frame_;
};

struct LabelRect
{
    int x;
    int y;
    int w;
    int h;
};

static uint8_t lineCode(int x, int y)
{
    uint8_t code = 0;
    if(x < 0) code |= 1U;
    else if(x >= MAP_W) code |= 2U;
    if(y < 0) code |= 4U;
    else if(y >= MAP_H) code |= 8U;
    return code;
}

static bool clipLocalLine(int& x1, int& y1, int& x2, int& y2)
{
    uint8_t c1 = lineCode(x1, y1);
    uint8_t c2 = lineCode(x2, y2);
    while(true)
    {
        if((c1 | c2) == 0U) return true;
        if((c1 & c2) != 0U) return false;
        const uint8_t outside = c1 != 0U ? c1 : c2;
        int x = 0;
        int y = 0;
        if((outside & 8U) != 0U)
        {
            if(y2 == y1) return false;
            y = MAP_H - 1;
            x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        }
        else if((outside & 4U) != 0U)
        {
            if(y2 == y1) return false;
            y = 0;
            x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        }
        else if((outside & 2U) != 0U)
        {
            if(x2 == x1) return false;
            x = MAP_W - 1;
            y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
        }
        else
        {
            if(x2 == x1) return false;
            x = 0;
            y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
        }

        if(outside == c1)
        {
            x1 = x; y1 = y; c1 = lineCode(x1, y1);
        }
        else
        {
            x2 = x; y2 = y; c2 = lineCode(x2, y2);
        }
    }
}

static void drawLocalLine(
    GrayCanvas& canvas,
    int x1,
    int y1,
    int x2,
    int y2,
    uint8_t level
)
{
    if(!clipLocalLine(x1, y1, x2, y2)) return;
    canvas.line(x1 + MAP_X, y1 + MAP_Y, x2 + MAP_X, y2 + MAP_Y, level);
}

static void drawThickLocalLine(
    GrayCanvas& canvas,
    int x1,
    int y1,
    int x2,
    int y2,
    int width,
    uint8_t level
)
{
    drawLocalLine(canvas, x1, y1, x2, y2, level);
    if(width <= 1) return;
    const bool horizontal = abs(x2 - x1) >= abs(y2 - y1);
    for(int offset = 1; offset <= width / 2; ++offset)
    {
        if(horizontal)
        {
            drawLocalLine(canvas, x1, y1 - offset, x2, y2 - offset, level);
            drawLocalLine(canvas, x1, y1 + offset, x2, y2 + offset, level);
        }
        else
        {
            drawLocalLine(canvas, x1 - offset, y1, x2 - offset, y2, level);
            drawLocalLine(canvas, x1 + offset, y1, x2 + offset, y2, level);
        }
    }
}

static void drawDashedLocalLine(
    GrayCanvas& canvas,
    int x1,
    int y1,
    int x2,
    int y2,
    uint8_t level,
    int dash,
    int gap
)
{
    const int dx = x2 - x1;
    const int dy = y2 - y1;
    const int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if(steps <= 0) return;
    const int stride = dash + gap;
    for(int start = 0; start <= steps; start += stride)
    {
        int end = start + dash;
        if(end > steps) end = steps;
        drawLocalLine(
            canvas,
            x1 + dx * start / steps,
            y1 + dy * start / steps,
            x1 + dx * end / steps,
            y1 + dy * end / steps,
            level
        );
    }
}

static bool utf8Next(const char*& cursor, uint32_t& codepoint)
{
    const uint8_t* s = reinterpret_cast<const uint8_t*>(cursor);
    if(*s == 0U) return false;
    if(s[0] < 0x80U)
    {
        codepoint = s[0]; cursor += 1; return true;
    }
    if((s[0] & 0xE0U) == 0xC0U && s[1] != 0U)
    {
        codepoint = (static_cast<uint32_t>(s[0] & 0x1FU) << 6U) | (s[1] & 0x3FU);
        cursor += 2; return true;
    }
    if((s[0] & 0xF0U) == 0xE0U && s[1] != 0U && s[2] != 0U)
    {
        codepoint = (static_cast<uint32_t>(s[0] & 0x0FU) << 12U) |
            (static_cast<uint32_t>(s[1] & 0x3FU) << 6U) |
            (s[2] & 0x3FU);
        cursor += 3; return true;
    }
    if((s[0] & 0xF8U) == 0xF0U && s[1] != 0U && s[2] != 0U && s[3] != 0U)
    {
        codepoint = (static_cast<uint32_t>(s[0] & 0x07U) << 18U) |
            (static_cast<uint32_t>(s[1] & 0x3FU) << 12U) |
            (static_cast<uint32_t>(s[2] & 0x3FU) << 6U) |
            (s[3] & 0x3FU);
        cursor += 4; return true;
    }
    codepoint = '?';
    cursor += 1;
    return true;
}

static const uint8_t* findGrayGlyphRecord(
    const FT02GrayFontPack& font,
    uint32_t codepoint
)
{
    int low = 0;
    int high = static_cast<int>(font.count) - 1;
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
    if(codepoint != static_cast<uint32_t>('?'))
        return findGrayGlyphRecord(font, static_cast<uint32_t>('?'));
    return nullptr;
}

static int grayTextWidth(
    const FT02GrayFontPack& font,
    const char* text,
    int maxGlyphs = 1000
)
{
    if(text == nullptr) return 0;
    const char* cursor = text;
    uint32_t cp = 0;
    int count = 0;
    int width = 0;
    while(count < maxGlyphs && utf8Next(cursor, cp))
    {
        const uint8_t* rec = findGrayGlyphRecord(font, cp);
        width += rec != nullptr ? rec[5] + 1 : 12;
        ++count;
    }
    return width > 0 ? width - 1 : 0;
}

static void drawGrayGlyph(
    GrayCanvas& canvas,
    const FT02GrayFontPack& font,
    uint32_t codepoint,
    int cursorX,
    int baselineY
)
{
    const uint8_t* rec = findGrayGlyphRecord(font, codepoint);
    if(rec == nullptr)
    {
        canvas.rect(cursorX + 1, baselineY - 16, 11, 16, FT02_GRAY4_BLACK);
        return;
    }

    const int width = rec[0];
    const int height = rec[1];
    const int bytesPerRow = rec[2];
    const int xOffset = static_cast<int8_t>(rec[3]);
    const int yOffset = static_cast<int8_t>(rec[4]);
    const uint8_t* bitmap = rec + 8;
    const int drawX = cursorX + xOffset;
    const int drawY = baselineY + yOffset;

    for(int row = 0; row < height; ++row)
    {
        for(int col = 0; col < width; ++col)
        {
            const int byteIndex = row * bytesPerRow + col / 4;
            const int shift = 6 - (col & 3) * 2;
            const uint8_t coverage = static_cast<uint8_t>((bitmap[byteIndex] >> shift) & 0x03U);
            if(coverage == 0U) continue;
            const uint8_t level = coverage == 1U
                ? FT02_GRAY4_LIGHT
                : (coverage == 2U ? FT02_GRAY4_DARK : FT02_GRAY4_BLACK);
            canvas.pixel(drawX + col, drawY + row, level);
        }
    }
}

static void drawGrayText(
    GrayCanvas& canvas,
    const FT02GrayFontPack& font,
    const char* text,
    int x,
    int baselineY,
    int maxGlyphs = 1000
)
{
    if(text == nullptr) return;
    const char* cursor = text;
    uint32_t cp = 0;
    int count = 0;
    while(count < maxGlyphs && utf8Next(cursor, cp))
    {
        const uint8_t* rec = findGrayGlyphRecord(font, cp);
        drawGrayGlyph(canvas, font, cp, x, baselineY);
        x += rec != nullptr ? rec[5] + 1 : 12;
        ++count;
    }
}

static void drawGrayTextCentered(
    GrayCanvas& canvas,
    const FT02GrayFontPack& font,
    const char* text,
    int x,
    int width,
    int baselineY
)
{
    int textX = x + (width - grayTextWidth(font, text)) / 2;
    if(textX < x + 3) textX = x + 3;
    drawGrayText(canvas, font, text, textX, baselineY);
}

static bool rectanglesOverlap(const LabelRect& a, const LabelRect& b)
{
    return !(a.x + a.w < b.x || b.x + b.w < a.x || a.y + a.h < b.y || b.y + b.h < a.y);
}

// v2.64: building polygons are intentionally not filled. Real PBF building
// geometry can be fragmented, clipped, or assembled from relations, and
// treating every visible segment run as one closed polygon can paint incorrect
// gray blocks. Keep buildings as subdued outlines so roads, labels, position,
// and tracks remain reliable and readable.
static size_t drawBuildingOutlines(GrayCanvas& canvas, bool fourGray)
{
    const FT02PbfMapSegment* segments = FT02_PbfMapSegments();
    const size_t count = FT02_PbfMapSegmentCount();
    if(segments == nullptr) return 0U;

    size_t outlinedSegments = 0U;
    const uint8_t level = fourGray ? FT02_GRAY4_LIGHT : FT02_GRAY4_BLACK;
    for(size_t i = 0; i < count; ++i)
    {
        if((i & 0xFFU) == 0U) delay(1);
        const FT02PbfMapSegment& segment = segments[i];
        if(segment.style != FT02_PBF_MAP_STYLE_BUILDING) continue;
        drawThickLocalLine(
            canvas,
            segment.x1,
            segment.y1,
            segment.x2,
            segment.y2,
            1,
            level
        );
        ++outlinedSegments;
    }
    return outlinedSegments;
}

static size_t drawGeometry(
    GrayCanvas& canvas,
    bool fourGray,
    size_t& clippedFilledCount
)
{
    const FT02PbfMapSegment* segments = FT02_PbfMapSegments();
    const size_t count = FT02_PbfMapSegmentCount();
    if(segments == nullptr) return 0U;

    // Buildings are deliberately outline-only. Draw them first as a subdued
    // background layer, then place roads and navigation information above.
    clippedFilledCount = 0U;
    const size_t outlinedBuildings = drawBuildingOutlines(canvas, fourGray);

    for(size_t i = 0; i < count; ++i)
    {
        if((i & 0xFFU) == 0U) delay(1);
        const FT02PbfMapSegment& s = segments[i];
        switch(s.style)
        {
            case FT02_PBF_MAP_STYLE_MAJOR:
                if(fourGray)
                {
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 5, FT02_GRAY4_LIGHT);
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 2, FT02_GRAY4_BLACK);
                }
                else
                {
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 3, FT02_GRAY4_BLACK);
                }
                break;
            case FT02_PBF_MAP_STYLE_MEDIUM:
                if(fourGray)
                {
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 3, FT02_GRAY4_LIGHT);
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 1, FT02_GRAY4_DARK);
                }
                else
                {
                    drawThickLocalLine(canvas, s.x1, s.y1, s.x2, s.y2, 2, FT02_GRAY4_BLACK);
                }
                break;
            case FT02_PBF_MAP_STYLE_MINOR:
                drawThickLocalLine(
                    canvas, s.x1, s.y1, s.x2, s.y2, 1,
                    fourGray ? FT02_GRAY4_LIGHT : FT02_GRAY4_BLACK
                );
                break;
            case FT02_PBF_MAP_STYLE_PATH:
                drawDashedLocalLine(
                    canvas, s.x1, s.y1, s.x2, s.y2,
                    fourGray ? FT02_GRAY4_DARK : FT02_GRAY4_BLACK,
                    3, 3
                );
                break;
            case FT02_PBF_MAP_STYLE_RAIL:
                drawDashedLocalLine(
                    canvas, s.x1, s.y1, s.x2, s.y2,
                    fourGray ? FT02_GRAY4_DARK : FT02_GRAY4_BLACK,
                    5, 3
                );
                break;
            case FT02_PBF_MAP_STYLE_WATER:
                drawThickLocalLine(
                    canvas, s.x1, s.y1, s.x2, s.y2, 1,
                    fourGray ? FT02_GRAY4_DARK : FT02_GRAY4_BLACK
                );
                break;
            case FT02_PBF_MAP_STYLE_BUILDING:
            default:
                break;
        }
    }
    return outlinedBuildings;
}

static void drawRoadLabels(GrayCanvas& canvas)
{
    const FT02PbfMapLabel* labels = FT02_PbfMapLabels();
    const size_t count = FT02_PbfMapLabelCount();
    if(labels == nullptr) return;

    LabelRect used[28];
    size_t usedCount = 0;
    for(size_t i = 0; i < count && usedCount < 28U; ++i)
    {
        const FT02PbfMapLabel& label = labels[i];
        const int textWidth = grayTextWidth(ft02_gray_road_18r, label.text, 8);
        if(textWidth <= 0) continue;
        LabelRect box;
        box.w = textWidth + 8;
        box.h = 24;
        box.x = label.x - box.w / 2;
        box.y = label.y - box.h / 2;
        if(box.x < 2 || box.y < 2 || box.x + box.w >= MAP_W - 2 || box.y + box.h >= MAP_H - 2)
            continue;

        bool collision = false;
        for(size_t j = 0; j < usedCount; ++j)
        {
            LabelRect padded = used[j];
            padded.x -= 4; padded.y -= 3; padded.w += 8; padded.h += 6;
            if(rectanglesOverlap(box, padded)) { collision = true; break; }
        }
        if(collision) continue;

        canvas.fillRect(box.x, box.y + MAP_Y, box.w, box.h, FT02_GRAY4_WHITE);
        drawGrayText(
            canvas,
            ft02_gray_road_18r,
            label.text,
            box.x + 4,
            box.y + MAP_Y + 20,
            8
        );
        used[usedCount++] = box;
    }
}

static void drawIconSize(
    GrayCanvas& canvas,
    const FT02Icon& icon,
    int x,
    int y,
    int targetSize
)
{
    for(int py = 0; py < targetSize; ++py)
    {
        for(int px = 0; px < targetSize; ++px)
        {
            const int sx = px * icon.width / targetSize;
            const int sy = py * icon.height / targetSize;
            const int byteIndex = sy * icon.bytesPerRow + sx / 8;
            const uint8_t mask = static_cast<uint8_t>(0x80U >> (sx & 7));
            if((icon.bitmap[byteIndex] & mask) != 0U)
                canvas.pixel(x + px, y + py, FT02_GRAY4_BLACK);
        }
    }
}

static void drawZoomControl(GrayCanvas& canvas, int x, int y, bool plus)
{
    constexpr int width = 56;
    constexpr int height = 56;
    canvas.fillRect(x, y, width, height, FT02_GRAY4_WHITE);
    canvas.rect(x, y, width, height, FT02_GRAY4_BLACK);
    canvas.rect(x + 2, y + 2, width - 4, height - 4, FT02_GRAY4_DARK);
    const int cx = x + width / 2;
    const int cy = y + height / 2;
    canvas.fillRect(cx - 13, cy - 2, 26, 4, FT02_GRAY4_BLACK);
    if(plus) canvas.fillRect(cx - 2, cy - 13, 4, 26, FT02_GRAY4_BLACK);
}

static void drawPositionMarkers(GrayCanvas& canvas)
{
    drawZoomControl(canvas, 18, 196, true);
    drawZoomControl(canvas, 18, 264, false);

    const int centerX = MAP_X + MAP_W / 2;
    const int centerY = MAP_Y + MAP_H / 2;
    canvas.circle(centerX, centerY, 7, FT02_GRAY4_BLACK);
    canvas.line(centerX - 12, centerY, centerX + 12, centerY, FT02_GRAY4_BLACK);
    canvas.line(centerX, centerY - 12, centerX, centerY + 12, FT02_GRAY4_BLACK);

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(!gnss.hasPosition) return;
    int16_t localX = 0;
    int16_t localY = 0;
    if(!FT02_PbfMapProjectCoordinate(gnss.longitude, gnss.latitude, localX, localY)) return;
    if(localX < 10 || localX >= MAP_W - 10 || localY < 10 || localY >= MAP_H - 10) return;

    const int x = localX + MAP_X;
    const int y = localY + MAP_Y;
    canvas.fillCircle(x, y, 3, FT02_GRAY4_BLACK);
    canvas.circle(x, y, 8, FT02_GRAY4_BLACK);
    if(gnss.fixValid)
    {
        canvas.circle(x, y, 10, FT02_GRAY4_BLACK);
        if(gnss.courseValid)
        {
            const double radians = gnss.courseDegrees * M_PI / 180.0;
            const int hx = x + static_cast<int>(lround(sin(radians) * 15.0));
            const int hy = y - static_cast<int>(lround(cos(radians) * 15.0));
            canvas.line(x, y, hx, hy, FT02_GRAY4_BLACK);
        }
    }
    else
    {
        canvas.line(x - 6, y - 6, x + 6, y + 6, FT02_GRAY4_BLACK);
        canvas.line(x + 6, y - 6, x - 6, y + 6, FT02_GRAY4_BLACK);
    }
}

static int mapScaleMeters(int zoom)
{
    if(zoom >= 20) return 20;
    if(zoom == 19) return 50;
    if(zoom == 18) return 100;
    if(zoom == 17) return 200;
    return 500;
}

static void drawStatusBlock(
    GrayCanvas& canvas,
    const FT02Icon& icon,
    int blockStart,
    const char* line1,
    const char* line2
)
{
    drawIconSize(canvas, icon, blockStart + 7, 16, 32);
    drawGrayText(canvas, ft02_gray_ui_20r, line1, blockStart + 52, 34);
    drawGrayText(canvas, ft02_gray_ui_20r, line2, blockStart + 52, 58);
}

static void drawProductionHeader(GrayCanvas& canvas)
{
    FT02StatusBarSnapshot status = {};
    FT02_GetStatusBarSnapshot(status);

    canvas.fillRect(0, 0, FT02_GRAY4_WIDTH, MAP_Y, FT02_GRAY4_WHITE);
    canvas.fillRect(0, 73, FT02_GRAY4_WIDTH, 3, FT02_GRAY4_BLACK);

    drawGrayText(canvas, ft02_gray_ui_30b, status.clockHHMM, 27, 45);
    drawGrayText(canvas, ft02_gray_ui_20r, status.clockMMDD, 132, 44);

    constexpr int blockStartX = 225;
    constexpr int blockWidth = 150;
    for(int i = 0; i < 4; ++i)
    {
        const int x = blockStartX + i * blockWidth - 10;
        canvas.line(x, 6, x, 66, FT02_GRAY4_BLACK);
    }

    drawStatusBlock(canvas, ICON_STATUS_WIRELESS, blockStartX, "LoRa", "已连接");
    drawStatusBlock(canvas, ICON_STATUS_GPS, blockStartX + blockWidth, status.gnssLine1, status.gnssLine2);
    drawStatusBlock(canvas, ICON_STATUS_SD, blockStartX + blockWidth * 2, status.storageLine1, status.storageLine2);
    drawStatusBlock(canvas, ICON_STATUS_BATTERY, blockStartX + blockWidth * 3, "86%", "电量");
}

static void drawFooterCell(
    GrayCanvas& canvas,
    int cellIndex,
    const char* text
)
{
    constexpr int cellWidth = FT02_GRAY4_WIDTH / 5;
    drawGrayTextCentered(
        canvas,
        ft02_gray_ui_20r,
        text,
        cellIndex * cellWidth,
        cellWidth,
        FOOTER_Y + 29
    );
}

static void drawProductionFooter(GrayCanvas& canvas, const FT02PbfMapReport& mapReport)
{
    canvas.fillRect(0, FOOTER_Y, FT02_GRAY4_WIDTH, FOOTER_H, FT02_GRAY4_WHITE);
    canvas.fillRect(0, FOOTER_Y, FT02_GRAY4_WIDTH, 4, FT02_GRAY4_BLACK);
    canvas.line(0, FT02_GRAY4_HEIGHT - 1, FT02_GRAY4_WIDTH - 1, FT02_GRAY4_HEIGHT - 1, FT02_GRAY4_BLACK);
    for(int i = 1; i < 5; ++i)
    {
        const int x = i * (FT02_GRAY4_WIDTH / 5);
        canvas.line(x, FOOTER_Y + 4, x, FT02_GRAY4_HEIGHT - 2, FT02_GRAY4_DARK);
    }

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    char speedText[32];
    char courseText[32];
    char altitudeText[32];
    char scaleText[32];

    if(gnss.fixValid && gnss.speedValid)
        snprintf(speedText, sizeof(speedText), "速度 %d", static_cast<int>(lroundf(gnss.speedKmh)));
    else
        snprintf(speedText, sizeof(speedText), "速度 --");

    if(gnss.fixValid && gnss.courseValid)
        snprintf(courseText, sizeof(courseText), "方向 %d", static_cast<int>(lroundf(gnss.courseDegrees)));
    else
        snprintf(courseText, sizeof(courseText), "方向 --");

    if(gnss.fixValid && gnss.altitudeValid)
        snprintf(altitudeText, sizeof(altitudeText), "海拔 %d", static_cast<int>(lroundf(gnss.altitudeMeters)));
    else
        snprintf(altitudeText, sizeof(altitudeText), "海拔 --");

    snprintf(scaleText, sizeof(scaleText), "Z%d %d米", mapReport.zoom, mapScaleMeters(mapReport.zoom));

    drawFooterCell(canvas, 0, speedText);
    drawFooterCell(canvas, 1, courseText);
    drawFooterCell(canvas, 2, altitudeText);
    drawFooterCell(canvas, 3, "距离 --");
    drawFooterCell(canvas, 4, scaleText);
}

static bool renderRealMap(
    uint8_t* frame,
    uint32_t& renderMs,
    size_t& outlinedBuildingSegments
)
{
    if(frame == nullptr) return false;
    const uint32_t started = millis();
    GrayCanvas canvas(frame);
    canvas.clear(FT02_GRAY4_WHITE);

    size_t unusedClippedCount = 0U;
    outlinedBuildingSegments = drawGeometry(canvas, true, unusedClippedCount);
    drawRoadLabels(canvas);
    drawPositionMarkers(canvas);
    canvas.rect(MAP_X, MAP_Y, MAP_W, MAP_H, FT02_GRAY4_BLACK);
    drawProductionHeader(canvas);
    drawProductionFooter(canvas, FT02_PbfMapReportCurrent());

    renderMs = millis() - started;
    return true;
}

}

FT02GrayMapReport FT02_DrawGrayMapScreen(
    SPIClass& spi,
    uint32_t spiHz,
    int pwrPin,
    int busyPin,
    int rstPin,
    int dcPin,
    int csPin
)
{
    FT02GrayMapReport report = {};
    report.message = "not-started";
    const uint32_t started = millis();

    const FT02PbfMapReport& mapReport = FT02_PbfMapReportCurrent();
    report.mapReady = mapReport.state == FT02_PBF_MAP_READY;
    report.zoom = mapReport.zoom;
    report.segmentCount = FT02_PbfMapSegmentCount();
    report.labelCount = FT02_PbfMapLabelCount();
    if(!report.mapReady)
    {
        report.message = "real-map-not-ready";
        report.elapsedMs = millis() - started;
        return report;
    }

    uint8_t* frame = FT02_AllocateGray4Framebuffer();
    report.frameAllocated = frame != nullptr;
    Serial.printf(
        "[GRAY4-MAP] render begin frame=%s building_fill=disabled heap=%u psram=%u\n",
        frame != nullptr ? "yes" : "no",
        static_cast<unsigned int>(ESP.getFreeHeap()),
        static_cast<unsigned int>(ESP.getFreePsram())
    );
    if(frame == nullptr)
    {
        report.message = "framebuffer-allocation-failed";
        report.elapsedMs = millis() - started;
        return report;
    }

    report.frameRendered = renderRealMap(
        frame,
        report.renderMs,
        report.outlinedBuildingSegments
    );
    if(!report.frameRendered)
    {
        report.message = "real-map-render-failed";
        FT02_FreeGray4Framebuffer(frame);
        report.elapsedMs = millis() - started;
        return report;
    }

    const FT02Gray4PanelReport panelReport = FT02_DisplayGray4Framebuffer(
        spi,
        spiHz,
        pwrPin,
        busyPin,
        rstPin,
        dcPin,
        csPin,
        frame,
        FT02_GRAY4_FRAME_PACKED_BYTES
    );
    FT02_FreeGray4Framebuffer(frame);

    report.panelInitialized = panelReport.panelInitialized;
    report.refreshCompleted = panelReport.refreshCompleted;
    report.panelQuiesced = panelReport.panelQuiesced;
    report.success = panelReport.success;
    report.message = report.success ? "ok" : panelReport.message;
    report.elapsedMs = millis() - started;

    Serial.printf(
        "[GRAY4-MAP] production success=%s sleep=%s building_fill=off aa_font=yes zoom=%d segments=%u labels=%u building_segments=%u render=%lums total=%lums message=%s\n",
        report.success ? "yes" : "no",
        report.panelQuiesced ? "yes" : "no",
        report.zoom,
        static_cast<unsigned int>(report.segmentCount),
        static_cast<unsigned int>(report.labelCount),
        static_cast<unsigned int>(report.outlinedBuildingSegments),
        static_cast<unsigned long>(report.renderMs),
        static_cast<unsigned long>(report.elapsedMs),
        report.message != nullptr ? report.message : "--"
    );

    return report;
}
