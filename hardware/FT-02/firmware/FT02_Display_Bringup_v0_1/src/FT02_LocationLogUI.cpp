#include "FT02_LocationLogUI.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_PbfMapRuntime.h"
#include "FT02_StatusBar.h"

namespace
{
constexpr int FT02_LOG_BODY_X = 20;
constexpr int FT02_LOG_BODY_Y = 78;
constexpr int FT02_LOG_BODY_W = 760;
constexpr int FT02_LOG_BODY_H = 354;
constexpr int FT02_LOG_LIST_ROWS = 5;
constexpr int FT02_LOG_ROW_X = 32;
constexpr int FT02_LOG_ROW_Y = 132;
constexpr int FT02_LOG_ROW_W = 736;
constexpr int FT02_LOG_ROW_H = 54;
constexpr int FT02_LOG_ROW_GAP = 4;
constexpr uint16_t FT02_LOG_PARTIAL_LIMIT = 60;
constexpr int FT02_LOG_DETAIL_MAP_X = 410;
constexpr int FT02_LOG_DETAIL_MAP_Y = 184;
constexpr int FT02_LOG_DETAIL_MAP_W = 346;
constexpr int FT02_LOG_DETAIL_MAP_H = 194;
constexpr int FT02_LOG_DETAIL_MAP_PADDING = 18;
constexpr int FT02_LOG_DETAIL_MAP_MIN_ZOOM = 16;
constexpr int FT02_LOG_DETAIL_MAP_MAX_ZOOM = 18;
constexpr int FT02_PBF_SCREEN_CENTER_X = 400;
constexpr int FT02_PBF_SCREEN_CENTER_Y = 182;

uint16_t g_ft02LocationLogPartialCount = 0;
bool g_ft02LocationDetailMapProjectionFits = false;
bool g_ft02LocationDetailMapUsePbfProjection = false;
bool g_ft02LocationDetailMapBaseReady = false;

static const FT02BottomBarItem FT02_LOG_LIST_BOTTOM_ITEMS[3] = {
    {nullptr, "方向键 循环选择"},
    {nullptr, "ENTER 查看"},
    {nullptr, "T删除 R刷新 B返回"}
};

static const FT02BottomBarItem FT02_LOG_DETAIL_BOTTOM_ITEMS[3] = {
    {nullptr, "方向键 循环切换"},
    {nullptr, "T 删除"},
    {nullptr, "B 返回"}
};

static const FT02BottomBarItem FT02_LOG_DELETE_BOTTOM_ITEMS[3] = {
    {nullptr, "ENTER 确认"},
    {nullptr, "删除后不可恢复"},
    {nullptr, "B 取消"}
};

static void FT02_LocationLogFormatDuration(
    const FT02LocationLogEntry& entry,
    char* output,
    size_t outputSize
)
{
    if(!entry.durationValid)
    {
        snprintf(output, outputSize, "--:--:--");
        return;
    }

    const uint32_t hours = entry.durationSeconds / 3600UL;
    const uint32_t minutes = (entry.durationSeconds / 60UL) % 60UL;
    const uint32_t seconds = entry.durationSeconds % 60UL;
    snprintf(
        output,
        outputSize,
        "%02lu:%02lu:%02lu",
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds)
    );
}

static const char* FT02_LocationLogStateText(const FT02LocationLogEntry& entry)
{
    if(entry.active) return "记录中";
    if(entry.completed) return "已完成";
    return "未正常结束";
}

static const char* FT02_LocationLogModeText(const FT02LocationLogEntry& entry)
{
    if(entry.autoTrackUsed && entry.manualPointCount > 0) return "自动+手动";
    if(entry.autoTrackUsed) return "自动轨迹";
    return "手动记录";
}

static uint16_t FT02_LocationLogPageStart(uint16_t selectedNewestIndex)
{
    return static_cast<uint16_t>(
        (selectedNewestIndex / FT02_LOG_LIST_ROWS) * FT02_LOG_LIST_ROWS
    );
}

static void FT02_DrawLocationLogHeader(
    FT02Display& display,
    const char* title,
    const char* rightText
)
{
    FT02_DrawTextPack(display, ft02_cjk_24b, title, 32, 115);
    if(rightText != nullptr)
    {
        const int width = FT02_TextWidthPack(ft02_cjk_20r, rightText);
        FT02_DrawTextPack(display, ft02_cjk_20r, rightText, display.width() - 32 - width, 113);
    }
}

static void FT02_DrawLocationLogListContent(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();
    const uint16_t pageStart = FT02_LocationLogPageStart(selectedNewestIndex);
    const uint16_t pageNumber = status.count == 0
        ? 0
        : static_cast<uint16_t>(pageStart / FT02_LOG_LIST_ROWS + 1);
    const uint16_t pageCount = status.count == 0
        ? 0
        : static_cast<uint16_t>((status.count + FT02_LOG_LIST_ROWS - 1) / FT02_LOG_LIST_ROWS);

    char rightText[80];
    if(status.count > 0)
    {
        snprintf(
            rightText,
            sizeof(rightText),
            "共%u条  %u/%u页",
            static_cast<unsigned int>(status.count),
            static_cast<unsigned int>(pageNumber),
            static_cast<unsigned int>(pageCount)
        );
    }
    else
    {
        snprintf(rightText, sizeof(rightText), "%s", status.message);
    }
    FT02_DrawLocationLogHeader(display, "定位记录列表", rightText);

    if(status.count == 0)
    {
        display.drawRect(32, 146, 736, 220, GxEPD_BLACK);
        const char* emptyText = status.loading
            ? "正在加载"
            : (status.storageReady ? "暂无定位记录" : "SD未就绪");
        const int emptyWidth = FT02_TextWidthPack(ft02_cjk_24b, emptyText);
        FT02_DrawTextPack(
            display,
            ft02_cjk_24b,
            emptyText,
            (display.width() - emptyWidth) / 2,
            240
        );
        const int messageWidth = FT02_TextWidthPack(ft02_cjk_20r, status.message);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            status.message,
            (display.width() - messageWidth) / 2,
            282
        );
        return;
    }

    for(int row = 0; row < FT02_LOG_LIST_ROWS; row++)
    {
        const uint16_t newestIndex = static_cast<uint16_t>(pageStart + row);
        if(newestIndex >= status.count) break;

        FT02LocationLogEntry entry;
        if(!FT02_LocationLogGetNewest(newestIndex, entry)) continue;

        const int y = FT02_LOG_ROW_Y + row * (FT02_LOG_ROW_H + FT02_LOG_ROW_GAP);
        const bool selected = newestIndex == selectedNewestIndex;
        display.fillRect(
            FT02_LOG_ROW_X,
            y,
            FT02_LOG_ROW_W,
            FT02_LOG_ROW_H,
            selected ? GxEPD_BLACK : GxEPD_WHITE
        );
        display.drawRect(FT02_LOG_ROW_X, y, FT02_LOG_ROW_W, FT02_LOG_ROW_H, GxEPD_BLACK);

        char firstLine[96];
        snprintf(
            firstLine,
            sizeof(firstLine),
            "%s  %s",
            entry.startDate[0] != '\0' ? entry.startDate : "日期未知",
            entry.startTime[0] != '\0' ? entry.startTime : "--:--:--"
        );

        char durationText[20];
        FT02_LocationLogFormatDuration(entry, durationText, sizeof(durationText));
        char secondLine[128];
        snprintf(
            secondLine,
            sizeof(secondLine),
            "点数%lu  时长%s  %s",
            static_cast<unsigned long>(entry.pointCount),
            durationText,
            FT02_LocationLogModeText(entry)
        );

        const char* stateText = FT02_LocationLogStateText(entry);
        const int stateWidth = FT02_TextWidthPack(ft02_cjk_20r, stateText);

        if(selected)
        {
            FT02_DrawTextPackInvertClipped(
                display,
                ft02_cjk_20r,
                firstLine,
                46,
                y + 23,
                42,
                y + 2,
                560,
                FT02_LOG_ROW_H - 4
            );
            FT02_DrawTextPackInvertClipped(
                display,
                ft02_cjk_20r,
                stateText,
                FT02_LOG_ROW_X + FT02_LOG_ROW_W - 18 - stateWidth,
                y + 23,
                FT02_LOG_ROW_X,
                y,
                FT02_LOG_ROW_W,
                FT02_LOG_ROW_H
            );
            FT02_DrawTextPackInvertClipped(
                display,
                ft02_cjk_20r,
                secondLine,
                46,
                y + 47,
                42,
                y + 24,
                FT02_LOG_ROW_W - 24,
                28
            );
        }
        else
        {
            FT02_DrawTextPackClipped(
                display,
                ft02_cjk_20r,
                firstLine,
                46,
                y + 23,
                42,
                y + 2,
                560,
                FT02_LOG_ROW_H - 4
            );
            FT02_DrawTextPack(
                display,
                ft02_cjk_20r,
                stateText,
                FT02_LOG_ROW_X + FT02_LOG_ROW_W - 18 - stateWidth,
                y + 23
            );
            FT02_DrawTextPackClipped(
                display,
                ft02_cjk_20r,
                secondLine,
                46,
                y + 47,
                42,
                y + 24,
                FT02_LOG_ROW_W - 24,
                28
            );
        }
    }
}

static void FT02_LocationLogDrawLabelValue(
    FT02Display& display,
    const char* label,
    const char* value,
    int x,
    int baselineY,
    int valueX,
    int clipWidth
)
{
    FT02_DrawTextPack(display, ft02_cjk_20r, label, x, baselineY);
    FT02_DrawTextPackClipped(
        display,
        ft02_cjk_20r,
        value,
        valueX,
        baselineY,
        valueX,
        baselineY - 24,
        clipWidth,
        29
    );
}


static uint8_t FT02_LocationDetailLineCode(int x, int y)
{
    uint8_t code = 0;
    if(x < FT02_LOG_DETAIL_MAP_X) code |= 1U;
    else if(x >= FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W) code |= 2U;
    if(y < FT02_LOG_DETAIL_MAP_Y) code |= 4U;
    else if(y >= FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H) code |= 8U;
    return code;
}

static bool FT02_LocationDetailClipLine(int& x1, int& y1, int& x2, int& y2)
{
    uint8_t c1 = FT02_LocationDetailLineCode(x1, y1);
    uint8_t c2 = FT02_LocationDetailLineCode(x2, y2);
    while(true)
    {
        if((c1 | c2) == 0) return true;
        if((c1 & c2) != 0) return false;

        const uint8_t outside = c1 != 0 ? c1 : c2;
        int x = 0;
        int y = 0;
        if((outside & 8U) != 0)
        {
            if(y2 == y1) return false;
            y = FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H - 1;
            x = x1 + static_cast<int>(
                static_cast<int64_t>(x2 - x1) * static_cast<int64_t>(y - y1) /
                static_cast<int64_t>(y2 - y1)
            );
        }
        else if((outside & 4U) != 0)
        {
            if(y2 == y1) return false;
            y = FT02_LOG_DETAIL_MAP_Y;
            x = x1 + static_cast<int>(
                static_cast<int64_t>(x2 - x1) * static_cast<int64_t>(y - y1) /
                static_cast<int64_t>(y2 - y1)
            );
        }
        else if((outside & 2U) != 0)
        {
            if(x2 == x1) return false;
            x = FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 1;
            y = y1 + static_cast<int>(
                static_cast<int64_t>(y2 - y1) * static_cast<int64_t>(x - x1) /
                static_cast<int64_t>(x2 - x1)
            );
        }
        else
        {
            if(x2 == x1) return false;
            x = FT02_LOG_DETAIL_MAP_X;
            y = y1 + static_cast<int>(
                static_cast<int64_t>(y2 - y1) * static_cast<int64_t>(x - x1) /
                static_cast<int64_t>(x2 - x1)
            );
        }

        if(outside == c1)
        {
            x1 = x;
            y1 = y;
            c1 = FT02_LocationDetailLineCode(x1, y1);
        }
        else
        {
            x2 = x;
            y2 = y;
            c2 = FT02_LocationDetailLineCode(x2, y2);
        }
    }
}

static void FT02_LocationDetailDrawClippedLine(
    FT02Display& display,
    int x1,
    int y1,
    int x2,
    int y2
)
{
    if(FT02_LocationDetailClipLine(x1, y1, x2, y2))
    {
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
    }
}

static void FT02_LocationDetailProjectPbf(
    double longitude,
    double latitude,
    int& x,
    int& y
)
{
    int32_t screenX = 0;
    int32_t screenY = 0;
    if(!FT02_PbfMapProjectCoordinateWide(longitude, latitude, screenX, screenY))
    {
        x = FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W / 2;
        y = FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H / 2;
        return;
    }
    x = FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W / 2 +
        static_cast<int>(screenX) - FT02_PBF_SCREEN_CENTER_X;
    y = FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H / 2 +
        static_cast<int>(screenY) - FT02_PBF_SCREEN_CENTER_Y;
}

static double FT02_LocationDetailMercatorY(double latitude)
{
    const double limited = latitude > 85.0 ? 85.0 : (latitude < -85.0 ? -85.0 : latitude);
    const double radians = limited * 3.14159265358979323846 / 180.0;
    return log(tan(3.14159265358979323846 * 0.25 + radians * 0.5));
}

static void FT02_LocationDetailProjectOverview(
    const FT02LocationRouteStatus& route,
    double longitude,
    double latitude,
    int& x,
    int& y
)
{
    const double minX = route.minLongitude;
    const double maxX = route.maxLongitude;
    const double minY = FT02_LocationDetailMercatorY(route.minLatitude);
    const double maxY = FT02_LocationDetailMercatorY(route.maxLatitude);
    const double spanX = fabs(maxX - minX);
    const double spanY = fabs(maxY - minY);
    const double usableW = FT02_LOG_DETAIL_MAP_W - FT02_LOG_DETAIL_MAP_PADDING * 2;
    const double usableH = FT02_LOG_DETAIL_MAP_H - FT02_LOG_DETAIL_MAP_PADDING * 2;

    double scaleX = spanX > 1e-12 ? usableW / spanX : 1e12;
    double scaleY = spanY > 1e-12 ? usableH / spanY : 1e12;
    const double scale = scaleX < scaleY ? scaleX : scaleY;
    const double centerX = (minX + maxX) * 0.5;
    const double centerY = (minY + maxY) * 0.5;
    const double pointY = FT02_LocationDetailMercatorY(latitude);

    x = static_cast<int>(lround(
        FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W * 0.5 +
        (longitude - centerX) * scale
    ));
    y = static_cast<int>(lround(
        FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H * 0.5 -
        (pointY - centerY) * scale
    ));
}

static void FT02_LocationDetailProjectRoutePoint(
    const FT02LocationRouteStatus& route,
    const FT02LocationRoutePoint& point,
    int& x,
    int& y
)
{
    if(g_ft02LocationDetailMapUsePbfProjection)
    {
        FT02_LocationDetailProjectPbf(point.longitude, point.latitude, x, y);
    }
    else
    {
        FT02_LocationDetailProjectOverview(route, point.longitude, point.latitude, x, y);
    }
}

static void FT02_DrawLocationDetailBaseMap(FT02Display& display)
{
    if(!g_ft02LocationDetailMapBaseReady) return;

    const FT02PbfMapSegment* segments = FT02_PbfMapSegments();
    const size_t count = FT02_PbfMapSegmentCount();
    for(size_t i = 0; i < count; i++)
    {
        const FT02PbfMapSegment& segment = segments[i];
        if(segment.style == FT02_PBF_MAP_STYLE_BUILDING &&
           FT02_PbfMapReportCurrent().zoom < 19)
        {
            continue;
        }

        int x1 = FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W / 2 +
            static_cast<int>(segment.x1) - FT02_PBF_SCREEN_CENTER_X;
        int y1 = FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H / 2 +
            static_cast<int>(segment.y1) - FT02_PBF_SCREEN_CENTER_Y;
        int x2 = FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W / 2 +
            static_cast<int>(segment.x2) - FT02_PBF_SCREEN_CENTER_X;
        int y2 = FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H / 2 +
            static_cast<int>(segment.y2) - FT02_PBF_SCREEN_CENTER_Y;

        if(segment.style == FT02_PBF_MAP_STYLE_PATH)
        {
            const int dx = x2 - x1;
            const int dy = y2 - y1;
            const int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            if(steps <= 0) continue;
            for(int step = 0; step <= steps; step += 6)
            {
                const int end = step + 2 < steps ? step + 2 : steps;
                FT02_LocationDetailDrawClippedLine(
                    display,
                    x1 + dx * step / steps,
                    y1 + dy * step / steps,
                    x1 + dx * end / steps,
                    y1 + dy * end / steps
                );
            }
        }
        else
        {
            FT02_LocationDetailDrawClippedLine(display, x1, y1, x2, y2);
            if(segment.style == FT02_PBF_MAP_STYLE_MAJOR)
            {
                FT02_LocationDetailDrawClippedLine(display, x1, y1 + 1, x2, y2 + 1);
            }
        }
    }
}

static void FT02_DrawLocationDetailRoute(FT02Display& display)
{
    const FT02LocationRouteStatus route = FT02_LocationLogRouteStatusCurrent();
    const FT02LocationRoutePoint* points = FT02_LocationLogRoutePoints();
    if(!route.loaded || !route.hasBounds || route.count == 0 || points == nullptr)
    {
        const char* message = route.message[0] != '\0' ? route.message : "没有有效轨迹点";
        const int width = FT02_TextWidthPack(ft02_cjk_20r, message);
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_20r,
            message,
            FT02_LOG_DETAIL_MAP_X + (FT02_LOG_DETAIL_MAP_W - width) / 2,
            FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H / 2,
            FT02_LOG_DETAIL_MAP_X + 6,
            FT02_LOG_DETAIL_MAP_Y + 6,
            FT02_LOG_DETAIL_MAP_W - 12,
            FT02_LOG_DETAIL_MAP_H - 12
        );
        return;
    }

    int previousX = 0;
    int previousY = 0;
    bool hasPrevious = false;
    for(uint32_t i = 0; i < route.count; i++)
    {
        int x = 0;
        int y = 0;
        FT02_LocationDetailProjectRoutePoint(route, points[i], x, y);
        if(hasPrevious)
        {
            FT02_LocationDetailDrawClippedLine(display, previousX, previousY, x, y);
            FT02_LocationDetailDrawClippedLine(display, previousX, previousY + 1, x, y + 1);
        }
        if(x >= FT02_LOG_DETAIL_MAP_X &&
           x < FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W &&
           y >= FT02_LOG_DETAIL_MAP_Y &&
           y < FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H)
        {
            display.fillRect(x - 1, y - 1, 3, 3, GxEPD_BLACK);
        }
        previousX = x;
        previousY = y;
        hasPrevious = true;
    }

    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
    FT02_LocationDetailProjectRoutePoint(route, points[0], startX, startY);
    FT02_LocationDetailProjectRoutePoint(route, points[route.count - 1U], endX, endY);
    if(startX >= FT02_LOG_DETAIL_MAP_X + 5 &&
       startX < FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 5 &&
       startY >= FT02_LOG_DETAIL_MAP_Y + 5 &&
       startY < FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H - 5)
    {
        display.fillRect(startX - 3, startY - 3, 7, 7, GxEPD_BLACK);
        display.drawRect(startX - 5, startY - 5, 11, 11, GxEPD_BLACK);
    }
    if(endX >= FT02_LOG_DETAIL_MAP_X + 5 &&
       endX < FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 5 &&
       endY >= FT02_LOG_DETAIL_MAP_Y + 5 &&
       endY < FT02_LOG_DETAIL_MAP_Y + FT02_LOG_DETAIL_MAP_H - 5)
    {
        display.drawLine(endX - 5, endY - 5, endX + 5, endY + 5, GxEPD_BLACK);
        display.drawLine(endX - 5, endY + 5, endX + 5, endY - 5, GxEPD_BLACK);
    }
}

static void FT02_DrawLocationDetailMapPanel(FT02Display& display)
{
    const FT02LocationRouteStatus route = FT02_LocationLogRouteStatusCurrent();
    display.drawRect(398, 136, 370, 276, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "轨迹地图", 418, 172);

    char mapMeta[64];
    if(route.loaded && route.count > 0)
    {
        if(g_ft02LocationDetailMapProjectionFits)
        {
            snprintf(
                mapMeta,
                sizeof(mapMeta),
                "%lu点 Z%d",
                static_cast<unsigned long>(route.count),
                FT02_PbfMapReportCurrent().zoom
            );
        }
        else
        {
            snprintf(
                mapMeta,
                sizeof(mapMeta),
                "%lu点 Z%d限幅",
                static_cast<unsigned long>(route.count),
                FT02_PbfMapReportCurrent().zoom
            );
        }
    }
    else
    {
        snprintf(mapMeta, sizeof(mapMeta), "无有效点");
    }
    const int metaWidth = FT02_TextWidthPack(ft02_cjk_20r, mapMeta);
    FT02_DrawTextPack(display, ft02_cjk_20r, mapMeta, 748 - metaWidth, 171);

    display.fillRect(
        FT02_LOG_DETAIL_MAP_X,
        FT02_LOG_DETAIL_MAP_Y,
        FT02_LOG_DETAIL_MAP_W,
        FT02_LOG_DETAIL_MAP_H,
        GxEPD_WHITE
    );
    display.drawRect(
        FT02_LOG_DETAIL_MAP_X,
        FT02_LOG_DETAIL_MAP_Y,
        FT02_LOG_DETAIL_MAP_W,
        FT02_LOG_DETAIL_MAP_H,
        GxEPD_BLACK
    );

    FT02_DrawLocationDetailBaseMap(display);
    FT02_DrawLocationDetailRoute(display);

    display.drawLine(FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 18,
                     FT02_LOG_DETAIL_MAP_Y + 28,
                     FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 18,
                     FT02_LOG_DETAIL_MAP_Y + 10,
                     GxEPD_BLACK);
    display.drawLine(FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 18,
                     FT02_LOG_DETAIL_MAP_Y + 10,
                     FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 22,
                     FT02_LOG_DETAIL_MAP_Y + 16,
                     GxEPD_BLACK);
    display.drawLine(FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 18,
                     FT02_LOG_DETAIL_MAP_Y + 10,
                     FT02_LOG_DETAIL_MAP_X + FT02_LOG_DETAIL_MAP_W - 14,
                     FT02_LOG_DETAIL_MAP_Y + 16,
                     GxEPD_BLACK);

    const char* footer = "无有效轨迹点";
    if(route.loaded && route.count > 0)
    {
        footer = g_ft02LocationDetailMapProjectionFits
            ? (g_ft02LocationDetailMapBaseReady
                ? "■起点  ×终点  全部点已适配"
                : "■起点  ×终点  底图不可用")
            : "最高Z18，最低Z16；超出范围会裁切";
    }
    FT02_DrawTextPackClipped(
        display,
        ft02_cjk_20r,
        footer,
        418,
        404,
        410,
        382,
        346,
        28
    );
}

static void FT02_DrawLocationLogDetailContent(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    const FT02LocationLogStatus status = FT02_LocationLogStatusCurrent();
    FT02LocationLogEntry entry;
    if(!FT02_LocationLogGetNewest(selectedNewestIndex, entry))
    {
        FT02_DrawLocationLogHeader(display, "定位记录详情", "记录不可用");
        return;
    }

    char rightText[40];
    snprintf(
        rightText,
        sizeof(rightText),
        "%u / %u  循环",
        static_cast<unsigned int>(selectedNewestIndex + 1),
        static_cast<unsigned int>(status.count)
    );
    FT02_DrawLocationLogHeader(display, "定位记录详情", rightText);

    display.drawRect(32, 136, 350, 276, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "行程概要", 52, 172);

    char value[96];
    FT02_LocationLogDrawLabelValue(
        display,
        "状态",
        FT02_LocationLogStateText(entry),
        52,
        206,
        124,
        236
    );

    snprintf(value, sizeof(value), "%s %s", entry.startDate, entry.startTime);
    FT02_LocationLogDrawLabelValue(display, "开始", value, 52, 236, 124, 236);
    snprintf(value, sizeof(value), "%s %s", entry.endDate, entry.endTime);
    FT02_LocationLogDrawLabelValue(display, "结束", value, 52, 266, 124, 236);

    FT02_LocationLogFormatDuration(entry, value, sizeof(value));
    FT02_LocationLogDrawLabelValue(display, "时长", value, 52, 296, 124, 236);

    snprintf(
        value,
        sizeof(value),
        "%lu  失败%lu",
        static_cast<unsigned long>(entry.pointCount),
        static_cast<unsigned long>(entry.failedPointCount)
    );
    FT02_LocationLogDrawLabelValue(display, "点数", value, 52, 326, 124, 236);
    FT02_LocationLogDrawLabelValue(
        display,
        "方式",
        FT02_LocationLogModeText(entry),
        52,
        356,
        124,
        236
    );
    FT02_LocationLogDrawLabelValue(display, "编号", entry.sessionId, 52, 388, 124, 236);

    FT02_DrawLocationDetailMapPanel(display);
}

static void FT02_LocationLogCommitBodyPartial(
    FT02Display& display,
    void (*drawContent)(FT02Display&, uint16_t),
    uint16_t selectedNewestIndex,
    const char* tag
)
{
    if(g_ft02LocationLogPartialCount >= FT02_LOG_PARTIAL_LIMIT)
    {
        if(strcmp(tag, "location-log-list-partial") == 0)
            FT02_DrawLocationLogListScreen(display, selectedNewestIndex);
        else
            FT02_DrawLocationLogDetailScreen(display, selectedNewestIndex);
        return;
    }

    display.setPartialWindow(FT02_LOG_BODY_X, FT02_LOG_BODY_Y, FT02_LOG_BODY_W, FT02_LOG_BODY_H);
    display.firstPage();
    do
    {
        display.fillRect(FT02_LOG_BODY_X, FT02_LOG_BODY_Y, FT02_LOG_BODY_W, FT02_LOG_BODY_H, GxEPD_WHITE);
        drawContent(display, selectedNewestIndex);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02LocationLogPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, tag);
}
}

bool FT02_PrepareLocationLogDetailMap(uint16_t selectedNewestIndex)
{
    FT02LocationLogEntry entry;
    if(!FT02_LocationLogGetNewest(selectedNewestIndex, entry))
    {
        FT02_ReleaseLocationLogDetailMap();
        return false;
    }

    FT02_PbfMapUnload();
    g_ft02LocationDetailMapProjectionFits = false;
    g_ft02LocationDetailMapUsePbfProjection = false;
    g_ft02LocationDetailMapBaseReady = false;

    const bool routeLoaded = FT02_LocationLogLoadRoute(entry.sessionId);
    const FT02LocationRouteStatus route = FT02_LocationLogRouteStatusCurrent();
    if(!routeLoaded || !route.loaded || !route.hasBounds || route.count == 0)
    {
        return routeLoaded;
    }

    g_ft02LocationDetailMapProjectionFits = FT02_PbfMapFitBoundsLimited(
        route.minLongitude,
        route.minLatitude,
        route.maxLongitude,
        route.maxLatitude,
        FT02_LOG_DETAIL_MAP_W,
        FT02_LOG_DETAIL_MAP_H,
        FT02_LOG_DETAIL_MAP_PADDING,
        FT02_LOG_DETAIL_MAP_MIN_ZOOM,
        FT02_LOG_DETAIL_MAP_MAX_ZOOM
    );
    g_ft02LocationDetailMapUsePbfProjection = true;
    FT02_PbfMapPrepare();
    g_ft02LocationDetailMapBaseReady = FT02_PbfMapBuild();

    Serial.printf(
        "[LOCATION-LOG] detail map session=%s points=%lu fit=%d zoom=%d base=%d\n",
        entry.sessionId,
        static_cast<unsigned long>(route.count),
        g_ft02LocationDetailMapProjectionFits ? 1 : 0,
        FT02_PbfMapReportCurrent().zoom,
        g_ft02LocationDetailMapBaseReady ? 1 : 0
    );
    return true;
}

void FT02_ReleaseLocationLogDetailMap()
{
    FT02_PbfMapUnload();
    FT02_LocationLogReleaseRoute();
    g_ft02LocationDetailMapProjectionFits = false;
    g_ft02LocationDetailMapUsePbfProjection = false;
    g_ft02LocationDetailMapBaseReady = false;
}

void FT02_DrawLocationLogListScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    g_ft02LocationLogPartialCount = 0;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawLocationLogListContent(display, selectedNewestIndex);
        FT02_DrawBottomBarWithFont(display, FT02_LOG_LIST_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "location-log-list-full");
}

void FT02_DrawLocationLogListBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    FT02_LocationLogCommitBodyPartial(
        display,
        FT02_DrawLocationLogListContent,
        selectedNewestIndex,
        "location-log-list-partial"
    );
}

void FT02_DrawLocationLogDetailScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    g_ft02LocationLogPartialCount = 0;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawLocationLogDetailContent(display, selectedNewestIndex);
        FT02_DrawBottomBarWithFont(display, FT02_LOG_DETAIL_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "location-log-detail-full");
}

void FT02_DrawLocationLogDetailBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    FT02_LocationLogCommitBodyPartial(
        display,
        FT02_DrawLocationLogDetailContent,
        selectedNewestIndex,
        "location-log-detail-partial"
    );
}

void FT02_DrawLocationLogDeleteConfirmScreen(
    FT02Display& display,
    const FT02LocationLogEntry& entry
)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawLocationLogHeader(display, "删除定位记录", "需要确认");

        display.drawRect(82, 150, 636, 238, GxEPD_BLACK);
        display.drawRect(86, 154, 628, 230, GxEPD_BLACK);

        const char* warning = "将永久删除本次行程的全部定位记录";
        const int warningWidth = FT02_TextWidthPack(ft02_cjk_24b, warning);
        FT02_DrawTextPack(
            display,
            ft02_cjk_24b,
            warning,
            (display.width() - warningWidth) / 2,
            206
        );

        char dateLine[96];
        snprintf(dateLine, sizeof(dateLine), "%s  %s", entry.startDate, entry.startTime);
        const int dateWidth = FT02_TextWidthPack(ft02_cjk_20r, dateLine);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            dateLine,
            (display.width() - dateWidth) / 2,
            252
        );

        const int idWidth = FT02_TextWidthPack(ft02_cjk_20r, entry.sessionId);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            entry.sessionId,
            (display.width() - idWidth) / 2,
            286
        );

        const char* action = "按 ENTER 确认删除，按 B 取消";
        const int actionWidth = FT02_TextWidthPack(ft02_cjk_20r, action);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            action,
            (display.width() - actionWidth) / 2,
            342
        );

        FT02_DrawBottomBarWithFont(display, FT02_LOG_DELETE_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "location-log-delete-confirm");
}
