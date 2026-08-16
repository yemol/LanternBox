#include "FT02_SystemSelfTest.h"

#include <stdio.h>
#include <string.h>

#include <Arduino.h>

#include "FT02_AudioLog.h"
#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_FieldManual.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_Gnss.h"
#include "FT02_InputManager.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LoRaTransport.h"
#include "FT02_PbfIndex.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_Storage.h"

namespace
{
FT02SelfTestReport g_report = {};
uint8_t g_pageIndex = 0;

static constexpr uint8_t FT02_SELFTEST_PAGE_COUNT = 2;
static constexpr uint8_t FT02_SELFTEST_PAGE1_ITEMS = 7;

static const FT02BottomBarItem FT02_SELFTEST_BOTTOM_ITEMS[3] = {
    {nullptr, "←/→ 翻页"},
    {nullptr, "ENTER 重新自检"},
    {nullptr, "H帮助 B返回"}
};

const char* stateText(FT02SelfTestState state)
{
    switch(state)
    {
        case FT02_SELFTEST_PASS: return "PASS";
        case FT02_SELFTEST_WARN: return "WARN";
        case FT02_SELFTEST_FAIL: return "FAIL";
        default: return "--";
    }
}

void addItem(const char* name, FT02SelfTestState state, const char* detail)
{
    if(g_report.count >= FT02SelfTestReport::MAX_ITEMS) return;
    FT02SelfTestItem& item = g_report.items[g_report.count++];
    item.name = name;
    item.state = state;
    snprintf(item.detail, sizeof(item.detail), "%s", detail != nullptr ? detail : "");

    if(state == FT02_SELFTEST_PASS) g_report.passed++;
    else if(state == FT02_SELFTEST_WARN) g_report.warned++;
    else if(state == FT02_SELFTEST_FAIL) g_report.failed++;
    else g_report.skipped++;
}

bool storageWriteReadProbe(char* detail, size_t detailSize)
{
    const char* path = "/lanternbox/.selftest.tmp";
    const char* token = "FT02_SELFTEST_A1";

    FILE* out = FT02_StorageOpenWriteFile(path, true);
    if(out == nullptr)
    {
        snprintf(detail, detailSize, "写入测试失败");
        return false;
    }

    const size_t expected = strlen(token);
    const size_t written = fwrite(token, 1, expected, out);
    const bool synced = FT02_StorageSyncFile(out);
    fclose(out);

    if(written != expected || !synced)
    {
        FT02_StorageDeleteFile(path);
        snprintf(detail, detailSize, "写入测试失败");
        return false;
    }

    FILE* in = FT02_StorageOpenReadFile(path);
    if(in == nullptr)
    {
        FT02_StorageDeleteFile(path);
        snprintf(detail, detailSize, "读取测试失败");
        return false;
    }

    char buffer[24] = {};
    const size_t read = fread(buffer, 1, expected, in);
    fclose(in);
    FT02_StorageDeleteFile(path);

    if(read != expected || memcmp(buffer, token, expected) != 0)
    {
        snprintf(detail, detailSize, "读取校验失败");
        return false;
    }

    snprintf(detail, detailSize, "读写正常 空闲%luMB", FT02_StorageFreeMB());
    return true;
}

void runMainControl()
{
    const uint32_t freeHeap = ESP.getFreeHeap();
    if(freeHeap < 16U * 1024U)
    {
        char text[96];
        snprintf(text, sizeof(text), "堆内存偏低 %luKB", (unsigned long)(freeHeap / 1024U));
        addItem("主控", FT02_SELFTEST_WARN, text);
    }
    else
    {
        char text[96];
        snprintf(text, sizeof(text), "运行正常 空闲%luKB", (unsigned long)(freeHeap / 1024U));
        addItem("主控", FT02_SELFTEST_PASS, text);
    }
}

void runPsram()
{
    const uint32_t bytes = ESP.getPsramSize();
    if(bytes == 0)
    {
        addItem("PSRAM", FT02_SELFTEST_FAIL, "未检测到PSRAM");
        return;
    }

    char text[96];
    snprintf(text, sizeof(text), "%luMB可用", (unsigned long)(bytes / (1024U * 1024U)));
    if(bytes < 8U * 1024U * 1024U)
        addItem("PSRAM", FT02_SELFTEST_WARN, text);
    else
        addItem("PSRAM", FT02_SELFTEST_PASS, text);
}

void runStorage()
{
    if(!FT02_StorageIsReady())
    {
        const FT02StorageError error = FT02_StorageLastError();
        if(error == FT02_STORAGE_ERROR_CARD_NONE)
            addItem("SD卡", FT02_SELFTEST_FAIL, "未检测到SD卡");
        else
            addItem("SD卡", FT02_SELFTEST_FAIL, "挂载失败");
        return;
    }

    if(FT02_AudioLogIsBusy())
    {
        addItem("SD卡", FT02_SELFTEST_WARN, "音频忙，跳过写测");
        return;
    }

    char detail[96];
    if(storageWriteReadProbe(detail, sizeof(detail)))
    {
        const unsigned long freeMb = FT02_StorageFreeMB();
        if(freeMb < 500UL)
            addItem("SD卡", FT02_SELFTEST_WARN, "读写正常，空间不足");
        else
            addItem("SD卡", FT02_SELFTEST_PASS, detail);
    }
    else
    {
        addItem("SD卡", FT02_SELFTEST_FAIL, detail);
    }
}

void runKeyboard()
{
    if(FT02_InputCardKbAvailable())
        addItem("键盘", FT02_SELFTEST_PASS, "CardKB2 0x5F正常");
    else
        addItem("键盘", FT02_SELFTEST_FAIL, "I2C设备无响应");
}

void runGnss()
{
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();

    if(gnss.communicationActive)
    {
        addItem("GNSS通讯", FT02_SELFTEST_PASS, "NMEA数据正常");
    }
    else if(gnss.nmeaSeen)
    {
        addItem("GNSS通讯", FT02_SELFTEST_WARN, "近期NMEA通讯超时");
    }
    else if(gnss.serialDataSeen)
    {
        addItem("GNSS通讯", FT02_SELFTEST_FAIL, "串口有数据但NMEA异常");
    }
    else
    {
        addItem("GNSS通讯", FT02_SELFTEST_FAIL, "未收到GNSS串口数据");
    }

    if(gnss.fixValid)
    {
        char text[96];
        snprintf(
            text,
            sizeof(text),
            "%u/%u星 %s",
            (unsigned)gnss.satellites,
            (unsigned)gnss.satellitesVisible,
            gnss.fixType >= 3 ? "3D定位" : "已定位"
        );
        addItem("GNSS定位", FT02_SELFTEST_PASS, text);
    }
    else if(gnss.communicationActive)
    {
        char text[96];
        if(gnss.gsvSeen && gnss.satellitesVisible > 0)
            snprintf(text, sizeof(text), "已见%u星，尚未定位", (unsigned)gnss.satellitesVisible);
        else if(gnss.gsvSeen)
            snprintf(text, sizeof(text), "未发现可见卫星");
        else
            snprintf(text, sizeof(text), "等待卫星数据");
        addItem("GNSS定位", FT02_SELFTEST_WARN, text);
    }
    else
    {
        addItem("GNSS定位", FT02_SELFTEST_WARN, "等待GNSS通讯恢复");
    }
}

void runLoRa()
{
    const bool link = FT02_LoRaTransportLinkUp();
    const bool ready = FT02_LoRaNodeRuntimeReady();
    const uint32_t frames = FT02_LoRaTransportFrameCount();

    if(link && ready)
    {
        char text[96];
        snprintf(text, sizeof(text), "模块通讯正常 帧%lu", (unsigned long)frames);
        addItem("LoRa通讯", FT02_SELFTEST_PASS, text);
    }
    else if(link)
    {
        addItem("LoRa通讯", FT02_SELFTEST_WARN, "模块在线，节点同步中");
    }
    else
    {
        addItem("LoRa通讯", FT02_SELFTEST_FAIL, "无线模块无有效响应");
    }

    const size_t nodes = FT02_LoRaNodeRuntimeNodeCount();
    if(ready && nodes > 1)
    {
        char text[96];
        snprintf(text, sizeof(text), "已发现%u个节点", (unsigned)nodes);
        addItem("LoRa网络", FT02_SELFTEST_PASS, text);
    }
    else if(ready)
    {
        addItem("LoRa网络", FT02_SELFTEST_WARN, "未发现其他节点");
    }
    else
    {
        addItem("LoRa网络", FT02_SELFTEST_WARN, "等待节点数据库就绪");
    }
}

void runAudio()
{
    const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
    if(audio.codecReady && audio.i2sReady)
        addItem("音频", FT02_SELFTEST_PASS, "WM8960与I2S正常");
    else if(!audio.codecReady)
        addItem("音频", FT02_SELFTEST_FAIL, "WM8960无响应");
    else
        addItem("音频", FT02_SELFTEST_FAIL, "I2S初始化失败");
}

void runMap()
{
    if(!FT02_StorageIsReady())
    {
        addItem("地图数据", FT02_SELFTEST_FAIL, "SD未就绪");
        return;
    }

    const bool source = FT02_StorageFileExists(FT02_PBF_SOURCE_PATH);
    const bool index = FT02_StorageFileExists(FT02_PBF_INDEX_PATH);
    if(source && index)
        addItem("地图数据", FT02_SELFTEST_PASS, "地图与索引文件存在");
    else if(source)
        addItem("地图数据", FT02_SELFTEST_WARN, "地图存在，索引缺失");
    else
        addItem("地图数据", FT02_SELFTEST_FAIL, "地图源文件缺失");
}

void runManual()
{
    const FT02FieldManualState state = FT02_FieldManualStateCurrent();
    if(state == FT02_FIELD_MANUAL_READY)
    {
        char text[96];
        snprintf(
            text,
            sizeof(text),
            "%d类 %d卡",
            FT02_FieldManualCategoryCount(),
            FT02_FieldManualCardCount()
        );
        addItem("应急手册", FT02_SELFTEST_PASS, text);
    }
    else if(state == FT02_FIELD_MANUAL_PACK_NOT_FOUND)
        addItem("应急手册", FT02_SELFTEST_FAIL, "手册数据包缺失");
    else if(state == FT02_FIELD_MANUAL_CHECKSUM_FAILED)
        addItem("应急手册", FT02_SELFTEST_FAIL, "数据校验失败");
    else if(state == FT02_FIELD_MANUAL_STORAGE_NOT_READY)
        addItem("应急手册", FT02_SELFTEST_FAIL, "SD未就绪");
    else
        addItem("应急手册", FT02_SELFTEST_FAIL, "手册数据读取异常");
}

void runPinyin()
{
    const size_t entries = FT02_PinyinLearningEntryCount();
    if(FT02_StorageIsReady())
    {
        char text[96];
        snprintf(text, sizeof(text), "基础词库正常 学习%u条", (unsigned)entries);
        addItem("拼音输入", FT02_SELFTEST_PASS, text);
    }
    else
    {
        addItem("拼音输入", FT02_SELFTEST_WARN, "可输入，学习暂不持久化");
    }
}

void runTime()
{
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(gnss.systemTimeSynchronized)
        addItem("系统时间", FT02_SELFTEST_PASS, "GNSS校时完成");
    else if(gnss.timeValid)
        addItem("系统时间", FT02_SELFTEST_WARN, "GNSS时间可用，尚未同步");
    else
        addItem("系统时间", FT02_SELFTEST_WARN, "尚未完成GNSS校时");
}

void saveReportLog()
{
    if(!FT02_StorageIsReady() || FT02_AudioLogIsBusy()) return;

    FILE* file = FT02_StorageOpenWriteFile("/lanternbox/logs/selftest.log", false);
    if(file == nullptr) return;

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    char stamp[40];
    if(gnss.timeValid && gnss.localDate[0] != '\0' && gnss.localTime[0] != '\0')
        snprintf(stamp, sizeof(stamp), "%s %s", gnss.localDate, gnss.localTime);
    else
        snprintf(stamp, sizeof(stamp), "UNSYNC uptime=%lus", (unsigned long)(millis() / 1000UL));

    fprintf(
        file,
        "[%s] FT02 %s SELFTEST-A1 pass=%u warn=%u fail=%u skip=%u elapsed=%lums\n",
        stamp,
        FT02_FIRMWARE_VERSION,
        (unsigned)g_report.passed,
        (unsigned)g_report.warned,
        (unsigned)g_report.failed,
        (unsigned)g_report.skipped,
        (unsigned long)g_report.elapsedMs
    );
    for(uint8_t i = 0; i < g_report.count; ++i)
    {
        const FT02SelfTestItem& item = g_report.items[i];
        fprintf(file, "  %s %s %s\n", item.name, stateText(item.state), item.detail);
    }
    fprintf(file, "\n");
    (void)FT02_StorageSyncFile(file);
    fclose(file);
}

void drawItem(
    FT02Display& display,
    const FT02SelfTestItem& item,
    int x,
    int baselineY
)
{
    // Single-line diagnostic row: item + state + concise explanation.
    // The second detail line used by A1/A1-layout is intentionally removed so
    // the freed vertical space can become larger, calmer spacing between rows.
    FT02_DrawTextPack(display, ft02_cjk_20r, item.name, x, baselineY);
    FT02_DrawTextPack(display, ft02_cjk_20r, stateText(item.state), x + 150, baselineY);
    FT02_DrawTextPack(display, ft02_cjk_20r, item.detail, x + 258, baselineY);
}
}

void FT02_SystemSelfTestRun()
{
    const uint32_t started = millis();
    const uint32_t nextGeneration = g_report.generation + 1U;
    memset(&g_report, 0, sizeof(g_report));
    g_report.generation = nextGeneration;
    g_pageIndex = 0;

    runMainControl();
    runPsram();
    runStorage();
    runKeyboard();
    runGnss();
    runLoRa();
    runAudio();
    runMap();
    runManual();
    runPinyin();
    runTime();
    addItem("电池", FT02_SELFTEST_NA, "尚未接入检测");

    g_report.elapsedMs = millis() - started;
    saveReportLog();

    Serial.printf(
        "[SELFTEST-A1] complete pass=%u warn=%u fail=%u skip=%u elapsed=%lums\n",
        (unsigned)g_report.passed,
        (unsigned)g_report.warned,
        (unsigned)g_report.failed,
        (unsigned)g_report.skipped,
        (unsigned long)g_report.elapsedMs
    );
    for(uint8_t i = 0; i < g_report.count; ++i)
    {
        const FT02SelfTestItem& item = g_report.items[i];
        Serial.printf(
            "[SELFTEST-A1] %-12s %-4s %s\n",
            item.name,
            stateText(item.state),
            item.detail
        );
    }
}

const FT02SelfTestReport& FT02_SystemSelfTestReportCurrent()
{
    return g_report;
}

void FT02_SystemSelfTestSetPage(uint8_t pageIndex)
{
    g_pageIndex = pageIndex < FT02_SELFTEST_PAGE_COUNT ? pageIndex : (FT02_SELFTEST_PAGE_COUNT - 1U);
}

uint8_t FT02_SystemSelfTestPage()
{
    return g_pageIndex;
}

uint8_t FT02_SystemSelfTestPageCount()
{
    return FT02_SELFTEST_PAGE_COUNT;
}

void FT02_DrawSystemSelfTestScreen(FT02Display& display)
{
    const FT02SelfTestReport& report = FT02_SystemSelfTestReportCurrent();

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawTextPack(display, ft02_cjk_24b, "设备状态  系统自检", 32, 116);

        char pageText[24];
        snprintf(pageText, sizeof(pageText), "%u/%u", (unsigned)(g_pageIndex + 1U), (unsigned)FT02_SELFTEST_PAGE_COUNT);
        const int pageWidth = FT02_TextWidthPack(ft02_cjk_20r, pageText);
        FT02_DrawTextPack(display, ft02_cjk_20r, pageText, display.width() - 32 - pageWidth, 116);

        char summary[96];
        snprintf(
            summary,
            sizeof(summary),
            "通过%u  警告%u  失败%u  未检测%u",
            (unsigned)report.passed,
            (unsigned)report.warned,
            (unsigned)report.failed,
            (unsigned)report.skipped
        );
        FT02_DrawTextPack(display, ft02_cjk_20r, summary, 32, 145);

        const char* resultText = report.failed > 0
            ? "系统部分功能不可用"
            : (report.warned > 0 ? "系统可用，存在注意项" : "系统状态正常");
        const int resultWidth = FT02_TextWidthPack(ft02_cjk_20r, resultText);
        FT02_DrawTextPack(display, ft02_cjk_20r, resultText, display.width() - 32 - resultWidth, 145);

        display.drawLine(32, 157, 768, 157, GxEPD_BLACK);

        const uint8_t startIndex = g_pageIndex == 0 ? 0 : FT02_SELFTEST_PAGE1_ITEMS;
        const uint8_t endIndex = g_pageIndex == 0
            ? (report.count < FT02_SELFTEST_PAGE1_ITEMS ? report.count : FT02_SELFTEST_PAGE1_ITEMS)
            : report.count;

        uint8_t row = 0;
        for(uint8_t i = startIndex; i < endIndex; ++i, ++row)
        {
            const int baselineY = 184 + static_cast<int>(row) * 41;
            drawItem(display, report.items[i], 48, baselineY);
        }

        FT02_DrawBottomBarWithFont(display, FT02_SELFTEST_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display, "system-selftest-full");
}
