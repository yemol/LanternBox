#include "FT02_MapTile.h"

#include "FT02_Storage.h"

#include <string.h>

static const int FT02_MAP_TILE_WIDTH = 256;
static const int FT02_MAP_TILE_HEIGHT = 256;
static const int FT02_MAP_TILE_STRIDE = 32;
static const size_t FT02_MAP_TILE_PAYLOAD_SIZE =
    FT02_MAP_TILE_STRIDE * FT02_MAP_TILE_HEIGHT;

static uint8_t g_mapTileBuffer[
    FT02_MAP_TILE_PAYLOAD_SIZE
];

static uint16_t FT02_ReadLe16(
    const uint8_t* data
)
{
    return (uint16_t)data[0]
        | ((uint16_t)data[1] << 8);
}

static uint32_t FT02_ReadLe32(
    const uint8_t* data
)
{
    return (uint32_t)data[0]
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24);
}

static FT02MapTileLoadResult FT02_LoadMapTileData(
    const char* path,
    size_t* payloadBytesRead
)
{
    if(payloadBytesRead != nullptr)
    {
        *payloadBytesRead = 0;
    }

    if(!FT02_StorageFileExists(path))
    {
        return FT02_MAP_TILE_MISSING;
    }

    FILE* tileFile = FT02_StorageOpenReadFile(path);

    if(tileFile == nullptr)
    {
        return FT02_MAP_TILE_OPEN_FAILED;
    }

    uint8_t header[16];

    size_t headerRead = fread(
        header,
        1,
        sizeof(header),
        tileFile
    );

    if(
        headerRead != sizeof(header)
        || memcmp(header, "FTM1", 4) != 0
        || FT02_ReadLe16(header + 4) != FT02_MAP_TILE_WIDTH
        || FT02_ReadLe16(header + 6) != FT02_MAP_TILE_HEIGHT
        || FT02_ReadLe16(header + 8) != FT02_MAP_TILE_STRIDE
        || header[10] != 1
        || FT02_ReadLe32(header + 12) != FT02_MAP_TILE_PAYLOAD_SIZE
    )
    {
        fclose(tileFile);
        return FT02_MAP_TILE_BAD_HEADER;
    }

    size_t payloadRead = fread(
        g_mapTileBuffer,
        1,
        FT02_MAP_TILE_PAYLOAD_SIZE,
        tileFile
    );

    fclose(tileFile);

    if(payloadBytesRead != nullptr)
    {
        *payloadBytesRead = payloadRead;
    }

    if(payloadRead != FT02_MAP_TILE_PAYLOAD_SIZE)
    {
        return FT02_MAP_TILE_READ_FAILED;
    }

    return FT02_MAP_TILE_OK;
}

static void FT02_DrawMissingTile(
    FT02Display& display,
    int x,
    int y
)
{
    display.fillRect(
        x,
        y,
        FT02_MAP_TILE_WIDTH,
        FT02_MAP_TILE_HEIGHT,
        GxEPD_WHITE
    );

    display.drawRect(
        x,
        y,
        FT02_MAP_TILE_WIDTH,
        FT02_MAP_TILE_HEIGHT,
        GxEPD_BLACK
    );

    for(int offset = 0; offset < FT02_MAP_TILE_WIDTH; offset += 32)
    {
        display.drawLine(
            x + offset,
            y,
            x + offset,
            y + FT02_MAP_TILE_HEIGHT - 1,
            GxEPD_BLACK
        );

        display.drawLine(
            x,
            y + offset,
            x + FT02_MAP_TILE_WIDTH - 1,
            y + offset,
            GxEPD_BLACK
        );
    }

    display.drawLine(
        x,
        y,
        x + FT02_MAP_TILE_WIDTH - 1,
        y + FT02_MAP_TILE_HEIGHT - 1,
        GxEPD_BLACK
    );

    display.drawLine(
        x + FT02_MAP_TILE_WIDTH - 1,
        y,
        x,
        y + FT02_MAP_TILE_HEIGHT - 1,
        GxEPD_BLACK
    );
}

FT02MapTileLoadResult FT02_DrawMapTile(
    FT02Display& display,
    const char* path,
    int screenX,
    int screenY
)
{
    FT02MapTileLoadResult result =
        FT02_LoadMapTileData(
            path,
            nullptr
        );

    if(result != FT02_MAP_TILE_OK)
    {
        FT02_DrawMissingTile(
            display,
            screenX,
            screenY
        );

        return result;
    }

    display.fillRect(
        screenX,
        screenY,
        FT02_MAP_TILE_WIDTH,
        FT02_MAP_TILE_HEIGHT,
        GxEPD_WHITE
    );

    display.drawBitmap(
        screenX,
        screenY,
        g_mapTileBuffer,
        FT02_MAP_TILE_WIDTH,
        FT02_MAP_TILE_HEIGHT,
        GxEPD_BLACK
    );

    return FT02_MAP_TILE_OK;
}

const char* FT02_MapTileLoadResultText(
    FT02MapTileLoadResult result
)
{
    switch(result)
    {
        case FT02_MAP_TILE_OK:
            return "OK";

        case FT02_MAP_TILE_MISSING:
            return "MISSING";

        case FT02_MAP_TILE_OPEN_FAILED:
            return "OPEN_FAILED";

        case FT02_MAP_TILE_BAD_HEADER:
            return "BAD_HEADER";

        case FT02_MAP_TILE_READ_FAILED:
            return "READ_FAILED";

        default:
            return "UNKNOWN";
    }
}
