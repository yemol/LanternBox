#include "FT02_MapUI.h"

#include "FT02_BottomBar.h"
#include "FT02_FontData.h"
#include "FT02_MapData.h"
#include "FT02_MapTile.h"
#include "FT02_StatusBar.h"

#include <stdio.h>

static const int FT02_MAP_VIEWPORT_X = 0;
static const int FT02_MAP_VIEWPORT_Y = 76;
static const int FT02_MAP_VIEWPORT_W = 800;
static const int FT02_MAP_VIEWPORT_H = 364;

static const int FT02_MAP_TILE_SIZE = 256;
static const int FT02_MAP_TILE_COLUMNS = 4;
static const int FT02_MAP_TILE_ROWS = 2;

static int g_mapTopLeftX = 0;
static int g_mapTopLeftY = 0;
static bool g_mapPositionInitialized = false;

static const FT02BottomBarItem FT02_MAP_BOTTOM_ITEMS[3] = {
    {
        nullptr,
        "方向键"
    },
    {
        nullptr,
        "ENTER LOAD"
    },
    {
        nullptr,
        "B/ESC 返回"
    }
};

static int FT02_Clamp(
    int value,
    int minimum,
    int maximum
)
{
    if(value < minimum)
    {
        return minimum;
    }

    if(value > maximum)
    {
        return maximum;
    }

    return value;
}

static void FT02_ClampMapPosition()
{
    const FT02MapConfig& config =
        FT02_MapConfigCurrent();

    int maxTopLeftX =
        config.maxX - FT02_MAP_TILE_COLUMNS + 1;

    int maxTopLeftY =
        config.maxY - FT02_MAP_TILE_ROWS + 1;

    if(maxTopLeftX < config.minX)
    {
        maxTopLeftX = config.minX;
    }

    if(maxTopLeftY < config.minY)
    {
        maxTopLeftY = config.minY;
    }

    g_mapTopLeftX = FT02_Clamp(
        g_mapTopLeftX,
        config.minX,
        maxTopLeftX
    );

    g_mapTopLeftY = FT02_Clamp(
        g_mapTopLeftY,
        config.minY,
        maxTopLeftY
    );
}

static void FT02_InitializeMapPosition()
{
    if(
        g_mapPositionInitialized
        || FT02_MapStateCurrent() != FT02_MAP_STATE_READY
    )
    {
        return;
    }

    const FT02MapConfig& config =
        FT02_MapConfigCurrent();

    g_mapTopLeftX = config.startX;
    g_mapTopLeftY = config.startY;

    FT02_ClampMapPosition();

    g_mapPositionInitialized = true;
}

void FT02_MapUIOpen()
{
    if(FT02_MapStateCurrent() != FT02_MAP_STATE_READY)
    {
        FT02_MapDataReload();
    }

    FT02_InitializeMapPosition();
}

void FT02_MapUIReload()
{
    g_mapPositionInitialized = false;

    FT02_MapDataReload();

    FT02_InitializeMapPosition();
}

bool FT02_MapUIMove(
    int deltaX,
    int deltaY
)
{
    if(FT02_MapStateCurrent() != FT02_MAP_STATE_READY)
    {
        return false;
    }

    FT02_InitializeMapPosition();

    int oldX = g_mapTopLeftX;
    int oldY = g_mapTopLeftY;

    g_mapTopLeftX += deltaX;
    g_mapTopLeftY += deltaY;

    FT02_ClampMapPosition();

    return oldX != g_mapTopLeftX
        || oldY != g_mapTopLeftY;
}

static void FT02_DrawAsciiText(
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
    display.setCursor(
        x,
        y
    );
    display.print(text);
}

static void FT02_DrawMapUnavailable(
    FT02Display& display
)
{
    display.fillRect(
        FT02_MAP_VIEWPORT_X,
        FT02_MAP_VIEWPORT_Y,
        FT02_MAP_VIEWPORT_W,
        FT02_MAP_VIEWPORT_H,
        GxEPD_WHITE
    );

    display.drawRect(
        80,
        150,
        640,
        190,
        GxEPD_BLACK
    );

    display.drawRect(
        84,
        154,
        632,
        182,
        GxEPD_BLACK
    );

    FT02_DrawAsciiText(
        display,
        "OFFLINE MAP NOT READY",
        210,
        220,
        2
    );

    FT02_DrawAsciiText(
        display,
        FT02_MapStateText(),
        300,
        260,
        1
    );

    FT02_DrawAsciiText(
        display,
        "COPY /maps/current TO SD, THEN PRESS ENTER",
        210,
        300,
        1
    );
}

static void FT02_DrawMapOverlay(
    FT02Display& display,
    int loadedTiles,
    int missingTiles
)
{
    const FT02MapConfig& config =
        FT02_MapConfigCurrent();

    display.fillRect(
        10,
        84,
        780,
        36,
        GxEPD_WHITE
    );

    display.drawRect(
        10,
        84,
        780,
        36,
        GxEPD_BLACK
    );

    char leftText[64];
    char rightText[80];

    snprintf(
        leftText,
        sizeof(leftText),
        "%s",
        config.name
    );

    snprintf(
        rightText,
        sizeof(rightText),
        "Z%d X%d Y%d  T%d M%d",
        config.zoom,
        g_mapTopLeftX,
        g_mapTopLeftY,
        loadedTiles,
        missingTiles
    );

    FT02_DrawAsciiText(
        display,
        leftText,
        18,
        96,
        1
    );

    FT02_DrawAsciiText(
        display,
        rightText,
        560,
        96,
        1
    );

    // Center marker. GNSS will replace the fixed center marker later.
    int centerX = FT02_MAP_VIEWPORT_W / 2;
    int centerY = FT02_MAP_VIEWPORT_Y
        + FT02_MAP_VIEWPORT_H / 2;

    display.drawCircle(
        centerX,
        centerY,
        9,
        GxEPD_BLACK
    );

    display.drawLine(
        centerX - 15,
        centerY,
        centerX + 15,
        centerY,
        GxEPD_BLACK
    );

    display.drawLine(
        centerX,
        centerY - 15,
        centerX,
        centerY + 15,
        GxEPD_BLACK
    );

    display.fillRect(
        582,
        420,
        208,
        16,
        GxEPD_WHITE
    );

    FT02_DrawAsciiText(
        display,
        config.attribution,
        588,
        422,
        1
    );
}

static void FT02_DrawReadyMap(
    FT02Display& display
)
{
    FT02_InitializeMapPosition();

    const FT02MapConfig& config =
        FT02_MapConfigCurrent();

    display.fillRect(
        FT02_MAP_VIEWPORT_X,
        FT02_MAP_VIEWPORT_Y,
        FT02_MAP_VIEWPORT_W,
        FT02_MAP_VIEWPORT_H,
        GxEPD_WHITE
    );

    int loadedTiles = 0;
    int missingTiles = 0;

    for(int row = 0; row < FT02_MAP_TILE_ROWS; row++)
    {
        for(int column = 0; column < FT02_MAP_TILE_COLUMNS; column++)
        {
            int tileX = g_mapTopLeftX + column;
            int tileY = g_mapTopLeftY + row;

            char tilePath[96];

            if(
                tileX < config.minX
                || tileX > config.maxX
                || tileY < config.minY
                || tileY > config.maxY
                || !FT02_MapBuildTilePath(
                    config.zoom,
                    tileX,
                    tileY,
                    tilePath,
                    sizeof(tilePath)
                )
            )
            {
                missingTiles++;
                continue;
            }

            FT02MapTileLoadResult result =
                FT02_DrawMapTile(
                    display,
                    tilePath,
                    column * FT02_MAP_TILE_SIZE,
                    FT02_MAP_VIEWPORT_Y
                        + row * FT02_MAP_TILE_SIZE
                );

            if(result == FT02_MAP_TILE_OK)
            {
                loadedTiles++;
            }
            else
            {
                missingTiles++;
            }
        }
    }

    FT02_DrawMapOverlay(
        display,
        loadedTiles,
        missingTiles
    );
}

void FT02_DrawMapScreen(
    FT02Display& display
)
{
    display.setFullWindow();

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        FT02_DrawStatusBar(display);

        if(FT02_MapStateCurrent() == FT02_MAP_STATE_READY)
        {
            FT02_DrawReadyMap(display);
        }
        else
        {
            FT02_DrawMapUnavailable(display);
        }

        FT02_DrawBottomBar(
            display,
            FT02_MAP_BOTTOM_ITEMS
        );
    }
    while(display.nextPage());
}
