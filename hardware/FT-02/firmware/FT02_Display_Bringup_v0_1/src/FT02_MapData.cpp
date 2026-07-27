#include "FT02_MapData.h"

#include "FT02_Storage.h"

#include <stdlib.h>
#include <string.h>

static const char* FT02_MAP_CONFIG_PATH = "/maps/current/map.cfg";

static FT02MapState g_mapState = FT02_MAP_STATE_NOT_STARTED;
static FT02MapConfig g_mapConfig;

static void FT02_ResetMapConfig()
{
    memset(
        &g_mapConfig,
        0,
        sizeof(g_mapConfig)
    );

    snprintf(
        g_mapConfig.name,
        sizeof(g_mapConfig.name),
        "FT-02 Offline Map"
    );

    snprintf(
        g_mapConfig.attribution,
        sizeof(g_mapConfig.attribution),
        "(c) OpenStreetMap contributors"
    );

    g_mapConfig.zoom = 0;
    g_mapConfig.minX = 0;
    g_mapConfig.maxX = 0;
    g_mapConfig.minY = 0;
    g_mapConfig.maxY = 0;
    g_mapConfig.startX = 0;
    g_mapConfig.startY = 0;
}

static char* FT02_Trim(char* text)
{
    if(text == nullptr)
    {
        return text;
    }

    while(*text == ' ' || *text == '\t')
    {
        text++;
    }

    size_t length = strlen(text);

    while(length > 0)
    {
        char tail = text[length - 1];

        if(
            tail == ' '
            || tail == '\t'
            || tail == '\r'
            || tail == '\n'
        )
        {
            text[length - 1] = 0;
            length--;
        }
        else
        {
            break;
        }
    }

    return text;
}

static bool FT02_ParseInteger(
    const char* value,
    int& output
)
{
    if(value == nullptr || *value == 0)
    {
        return false;
    }

    char* end = nullptr;
    long parsed = strtol(
        value,
        &end,
        10
    );

    if(end == value || *FT02_Trim(end) != 0)
    {
        return false;
    }

    output = (int)parsed;
    return true;
}

static void FT02_CopyText(
    char* destination,
    size_t destinationSize,
    const char* source
)
{
    if(
        destination == nullptr
        || destinationSize == 0
    )
    {
        return;
    }

    if(source == nullptr)
    {
        destination[0] = 0;
        return;
    }

    strncpy(
        destination,
        source,
        destinationSize - 1
    );

    destination[destinationSize - 1] = 0;
}

static bool FT02_ParseMapConfigLine(
    char* line
)
{
    char* trimmed = FT02_Trim(line);

    if(*trimmed == 0 || *trimmed == '#')
    {
        return true;
    }

    if(strcmp(trimmed, "FTMAP1") == 0)
    {
        return true;
    }

    char* equal = strchr(
        trimmed,
        '='
    );

    if(equal == nullptr)
    {
        return false;
    }

    *equal = 0;

    char* key = FT02_Trim(trimmed);
    char* value = FT02_Trim(equal + 1);

    if(strcmp(key, "name") == 0)
    {
        FT02_CopyText(
            g_mapConfig.name,
            sizeof(g_mapConfig.name),
            value
        );

        return true;
    }

    if(strcmp(key, "attribution") == 0)
    {
        FT02_CopyText(
            g_mapConfig.attribution,
            sizeof(g_mapConfig.attribution),
            value
        );

        return true;
    }

    int parsed = 0;

    if(!FT02_ParseInteger(value, parsed))
    {
        return false;
    }

    if(strcmp(key, "zoom") == 0)
    {
        g_mapConfig.zoom = parsed;
    }
    else if(strcmp(key, "min_x") == 0)
    {
        g_mapConfig.minX = parsed;
    }
    else if(strcmp(key, "max_x") == 0)
    {
        g_mapConfig.maxX = parsed;
    }
    else if(strcmp(key, "min_y") == 0)
    {
        g_mapConfig.minY = parsed;
    }
    else if(strcmp(key, "max_y") == 0)
    {
        g_mapConfig.maxY = parsed;
    }
    else if(strcmp(key, "start_x") == 0)
    {
        g_mapConfig.startX = parsed;
    }
    else if(strcmp(key, "start_y") == 0)
    {
        g_mapConfig.startY = parsed;
    }
    else
    {
        // Unknown keys are ignored so the manifest can grow later.
    }

    return true;
}

static bool FT02_ValidateMapConfig()
{
    if(g_mapConfig.zoom < 0 || g_mapConfig.zoom > 30)
    {
        return false;
    }

    if(
        g_mapConfig.minX > g_mapConfig.maxX
        || g_mapConfig.minY > g_mapConfig.maxY
    )
    {
        return false;
    }

    if(
        g_mapConfig.startX < g_mapConfig.minX
        || g_mapConfig.startX > g_mapConfig.maxX
        || g_mapConfig.startY < g_mapConfig.minY
        || g_mapConfig.startY > g_mapConfig.maxY
    )
    {
        return false;
    }

    return true;
}

void FT02_MapDataBegin()
{
    FT02_MapDataReload();
}

bool FT02_MapDataReload()
{
    FT02_ResetMapConfig();

    if(!FT02_StorageIsReady())
    {
        g_mapState = FT02_MAP_STATE_SD_NOT_READY;
        Serial.println("FT02 Map: SD is not ready");
        return false;
    }

    FILE* configFile = FT02_StorageOpenReadFile(
        FT02_MAP_CONFIG_PATH
    );

    if(configFile == nullptr)
    {
        g_mapState = FT02_MAP_STATE_CONFIG_MISSING;
        Serial.print("FT02 Map: config missing: ");
        Serial.println(FT02_MAP_CONFIG_PATH);
        return false;
    }

    bool firstLineChecked = false;
    bool signatureOk = false;
    bool parseOk = true;

    char line[160];
    size_t lineLength = 0;

    while(true)
    {
        int raw = fgetc(configFile);

        if(raw == EOF)
        {
            break;
        }

        char c = (char)raw;

        if(c == '\n' || lineLength >= sizeof(line) - 1)
        {
            line[lineLength] = 0;

            char* trimmed = FT02_Trim(line);

            if(!firstLineChecked && *trimmed != 0)
            {
                firstLineChecked = true;
                signatureOk = strcmp(trimmed, "FTMAP1") == 0;
            }

            if(!FT02_ParseMapConfigLine(line))
            {
                parseOk = false;
                break;
            }

            lineLength = 0;
            continue;
        }

        line[lineLength++] = c;
    }

    if(parseOk && lineLength > 0)
    {
        line[lineLength] = 0;

        char* trimmed = FT02_Trim(line);

        if(!firstLineChecked && *trimmed != 0)
        {
            firstLineChecked = true;
            signatureOk = strcmp(trimmed, "FTMAP1") == 0;
        }

        parseOk = FT02_ParseMapConfigLine(line);
    }

    fclose(configFile);

    if(
        !firstLineChecked
        || !signatureOk
        || !parseOk
        || !FT02_ValidateMapConfig()
    )
    {
        g_mapState = FT02_MAP_STATE_CONFIG_INVALID;
        Serial.println("FT02 Map: invalid map.cfg");
        return false;
    }

    g_mapState = FT02_MAP_STATE_READY;

    Serial.print("FT02 Map READY: name=");
    Serial.print(g_mapConfig.name);
    Serial.print(" z=");
    Serial.print(g_mapConfig.zoom);
    Serial.print(" bounds=");
    Serial.print(g_mapConfig.minX);
    Serial.print(",");
    Serial.print(g_mapConfig.minY);
    Serial.print("..");
    Serial.print(g_mapConfig.maxX);
    Serial.print(",");
    Serial.println(g_mapConfig.maxY);

    return true;
}

FT02MapState FT02_MapStateCurrent()
{
    return g_mapState;
}

const FT02MapConfig& FT02_MapConfigCurrent()
{
    return g_mapConfig;
}

const char* FT02_MapStateText()
{
    switch(g_mapState)
    {
        case FT02_MAP_STATE_NOT_STARTED:
            return "NOT_STARTED";

        case FT02_MAP_STATE_READY:
            return "READY";

        case FT02_MAP_STATE_SD_NOT_READY:
            return "SD_NOT_READY";

        case FT02_MAP_STATE_CONFIG_MISSING:
            return "CONFIG_MISSING";

        case FT02_MAP_STATE_CONFIG_INVALID:
            return "CONFIG_INVALID";

        default:
            return "UNKNOWN";
    }
}

bool FT02_MapBuildTilePath(
    int zoom,
    int tileX,
    int tileY,
    char* output,
    size_t outputSize
)
{
    if(output == nullptr || outputSize == 0)
    {
        return false;
    }

    int written = snprintf(
        output,
        outputSize,
        "/maps/current/%d/%d/%d.ftm",
        zoom,
        tileX,
        tileY
    );

    return written > 0
        && (size_t)written < outputSize;
}
