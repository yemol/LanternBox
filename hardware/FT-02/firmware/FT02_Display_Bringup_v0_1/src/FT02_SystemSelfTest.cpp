#include "FT02_SystemSelfTest.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Arduino.h>
#include <esp_system.h>
#include <esp_sleep.h>

#include "FT02_AudioLog.h"
#include "FT02_BatteryMonitor.h"
#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_FieldManual.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_Gnss.h"
#include "FT02_CompassCore.h"
#include "FT02_GnssPowerControl.h"
#include "FT02_InputManager.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_MeshService.h"
#include "FT02_ModeManager.h"
#include "FT02_PbfIndex.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_SleepDiagnostics.h"
#include "FT02_Storage.h"

namespace
{
FT02SelfTestReport g_report = {};
uint8_t g_pageIndex = 0;

static constexpr uint8_t FT02_SELFTEST_PAGE_COUNT = 4;
static constexpr uint8_t FT02_SELFTEST_PAGE_STARTS[FT02_SELFTEST_PAGE_COUNT] = {0, 6, 12, 19};
static constexpr uint8_t FT02_SELFTEST_PAGE_LENGTHS[FT02_SELFTEST_PAGE_COUNT] = {6, 6, 7, 3};
static constexpr uint8_t FT02_SELFTEST_POWER_PAGE = 3;

static const FT02BottomBarItem FT02_SELFTEST_BOTTOM_DEFAULT[3] = {
    {nullptr, "←/→ 翻页"},
    {nullptr, "ENTER 重新自检"},
    {nullptr, "B 返回"}
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

void runStoragePassive()
{
    if(!FT02_StorageIsReady())
    {
        const FT02StorageError error = FT02_StorageLastError();
        addItem("SD卡", FT02_SELFTEST_FAIL,
                error == FT02_STORAGE_ERROR_CARD_NONE ? "未检测到SD卡" : "当前未就绪");
        return;
    }

    char detail[96];
    snprintf(detail, sizeof(detail), "已挂载 空闲%luMB · ENTER主动读写自检",
             FT02_StorageFreeMB());
    addItem("SD卡", FT02_SELFTEST_PASS, detail);
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

void runBatteryCached()
{
    const FT02BatterySnapshot battery = FT02_BatterySnapshotCurrent();

    if(!battery.monitorPresent || !battery.valid)
    {
        addItem("电池", FT02_SELFTEST_WARN, "SOC估算暂无有效缓存 · ENTER主动自检");
        return;
    }

    char detail[96];
    const char* flow = FT02_BatteryIsCharging() ? "充电" : "供电";
    snprintf(
        detail,
        sizeof(detail),
        "SOC估算 %.0f%% | %.3fV | %s",
        static_cast<double>(battery.percent),
        static_cast<double>(battery.voltage),
        flow
    );

    const FT02BatteryBand band = FT02_BatteryBandCurrent();
    addItem("电池",
            (band == FT02_BATTERY_CRITICAL || band == FT02_BATTERY_LOW)
                ? FT02_SELFTEST_WARN : FT02_SELFTEST_PASS,
            detail);
}

void runBattery()
{
    FT02_BatteryMonitorRefreshNow();
    const FT02BatterySnapshot battery = FT02_BatterySnapshotCurrent();

    if(!battery.monitorPresent || !battery.valid)
    {
        addItem("电池", FT02_SELFTEST_FAIL, "MAX17048 0x36未检测/读取失败");
        return;
    }

    char detail[96];
    const char* flow = FT02_BatteryIsCharging() ? "充电" : "供电";
    const FT02BatteryBand band = FT02_BatteryBandCurrent();

    if(band == FT02_BATTERY_CRITICAL)
    {
        snprintf(
            detail,
            sizeof(detail),
            "严重低电量 SOC估算 %.0f%% %.3fV %s",
            static_cast<double>(battery.percent),
            static_cast<double>(battery.voltage),
            flow
        );
        addItem("电池", FT02_SELFTEST_WARN, detail);
        return;
    }

    if(band == FT02_BATTERY_LOW)
    {
        snprintf(
            detail,
            sizeof(detail),
            "低电量 SOC估算 %.0f%% %.3fV %s",
            static_cast<double>(battery.percent),
            static_cast<double>(battery.voltage),
            flow
        );
        addItem("电池", FT02_SELFTEST_WARN, detail);
        return;
    }

    snprintf(
        detail,
        sizeof(detail),
        "SOC估算 %.0f%% %.3fV %s",
        static_cast<double>(battery.percent),
        static_cast<double>(battery.voltage),
        flow
    );
    addItem("电池", FT02_SELFTEST_PASS, detail);
}

void runGnss()
{
    const FT02GnssSnapshot nav = FT02_GnssSnapshotCurrent();
    const bool userEnabled = FT02_GnssPowerDesiredEnabled();

    if(!userEnabled)
    {
        if(FT02_ModeAwakeMode() == FT02_MODE_INDOOR)
            addItem("GNSS", FT02_SELFTEST_NA, "室内模式·GNSS关闭");
        else
            addItem("GNSS", FT02_SELFTEST_NA, "模式策略·GNSS关闭");
        addItem("GNSS定位", FT02_SELFTEST_NA, "GNSS关闭，未进行定位");
    }
    else
    {
        if(FT02_GnssHardwareReady())
            addItem("GNSS", nav.communicationActive ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN,
                    nav.communicationActive ? "Core UART5/6 115200·数据流正常" : "Core UART5/6 115200·等待NMEA");
        else if(FT02_GnssHardwareKnown())
            addItem("GNSS", nav.startAgeMs < 60000u ? FT02_SELFTEST_WARN : FT02_SELFTEST_FAIL,
                    nav.startAgeMs < 60000u ? "Core UART5/6 115200·接收启动中" : "Core UART5/6 115200无有效NMEA");
        else
            addItem("GNSS", FT02_SELFTEST_WARN, "Core UART尚未探测");

        if(nav.fixValid)
        {
            char text[96];
            snprintf(text,sizeof(text),"%u星 %s PDOP %.2f",(unsigned)nav.satellites,nav.fixType>=3?"3D定位":"2D定位",nav.hdop);
            addItem("GNSS定位",FT02_SELFTEST_PASS,text);
        }
        else if(nav.communicationActive)
            addItem("GNSS定位",FT02_SELFTEST_WARN,"NMEA正常，当前未定位");
        else
            addItem("GNSS定位",FT02_SELFTEST_WARN,"等待Core UART GNSS数据");
    }

    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    if(compass.ready && compass.valid)
    {
        char text[96];
        if(compass.quality >= 3)
            snprintf(text,sizeof(text),"Core I2C 0x0D 航向 %.1f° 已校准·高质量",(double)compass.headingX10/10.0);
        else if(compass.quality == 2)
            snprintf(text,sizeof(text),"Core I2C 0x0D 航向 %.1f° 已校准·可用",(double)compass.headingX10/10.0);
        else
            snprintf(text,sizeof(text),"Core I2C 0x0D 航向 %.1f° 建议校准",(double)compass.headingX10/10.0);
        addItem("罗盘",compass.quality>=2?FT02_SELFTEST_PASS:FT02_SELFTEST_WARN,text);
    }
    else if(compass.hardwareKnown)
        addItem("罗盘",FT02_SELFTEST_FAIL,"Core I2C 0x0D QMC5883L无有效数据");
    else
        addItem("罗盘",FT02_SELFTEST_WARN,"Core I2C罗盘尚未初始化");
}

void runMeshRadio()
{
    const bool meshReady = FT02_MeshServiceReady();
    addItem("Mesh无线",
            meshReady ? FT02_SELFTEST_PASS : FT02_SELFTEST_FAIL,
            meshReady ? "SX1262 · HW-SPI · LongFast" : "SX1262 MeshService未就绪");

    char networkText[96];
    snprintf(networkText, sizeof(networkText), "Meshtastic · 节点%u · PKI%u",
             static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
             static_cast<unsigned>(FT02_MeshServicePkiPeerCount()));
    addItem("Mesh网络",
            FT02_LoRaNodeRuntimeReady() ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN,
            FT02_LoRaNodeRuntimeReady() ? networkText : "Meshtastic节点表初始化中");
}

void runAudio()
{
    const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
    if(!FT02_AudioLogHardwareActive())
    {
        addItem("音频", FT02_SELFTEST_PASS, "休眠中 · ENTER主动自检");
        return;
    }
    if(audio.codecReady && audio.i2sReady)
        addItem("音频", FT02_SELFTEST_PASS, "ES8311与I2S正常");
    else if(!audio.i2sReady)
        addItem("音频", FT02_SELFTEST_FAIL, "I2S初始化失败");
    else
        addItem("音频", FT02_SELFTEST_FAIL, "ES8311无响应");
}

void runMapPassive()
{
    if(!FT02_StorageIsReady())
    {
        addItem("地图数据", FT02_SELFTEST_FAIL, "SD未就绪");
        return;
    }
    // Do not touch map files from the observational Device Status page.
    addItem("地图数据", FT02_SELFTEST_NA, "未主动访问 · ENTER重新自检");
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

    if(!gnss.timeValid || gnss.unixTime == 0u)
    {
        if(!FT02_GnssPowerDesiredEnabled())
            addItem("系统时间", FT02_SELFTEST_NA, "GNSS已关闭，未执行卫星校时");
        else
            addItem("系统时间", FT02_SELFTEST_WARN, "尚未获得有效GNSS时间");
        return;
    }

    const time_t utc = static_cast<time_t>(gnss.unixTime);
    struct tm tmUtc = {};
    if(gmtime_r(&utc, &tmUtc) == nullptr)
    {
        addItem("系统时间", FT02_SELFTEST_FAIL, "时间转换失败");
        return;
    }

    const int year = tmUtc.tm_year + 1900;
    if(year < 2025 || year > 2099)
    {
        char text[96];
        snprintf(text, sizeof(text), "时间越界 年份%d", year);
        addItem("系统时间", FT02_SELFTEST_FAIL, text);
        return;
    }

    char text[96];
    if(gnss.localDate[0] != '\0' && gnss.localTime[0] != '\0')
        snprintf(text, sizeof(text), "%s %s %s", gnss.localDate, gnss.localTime,
                 gnss.systemTimeSynchronized ? "已同步" : "待同步");
    else
        snprintf(text, sizeof(text), "UTC %04d-%02d-%02d %02d:%02d %s",
                 year, tmUtc.tm_mon + 1, tmUtc.tm_mday, tmUtc.tm_hour, tmUtc.tm_min,
                 gnss.systemTimeSynchronized ? "已同步" : "待同步");

    addItem("系统时间",
            gnss.systemTimeSynchronized ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN,
            text);
}

void runMeshPki()
{
    const bool ready = FT02_MeshServicePkiReady();
    char text[96];
    snprintf(text, sizeof(text), "本机密钥 %s · 已学习对端 %u",
             ready ? "正常" : "未就绪",
             static_cast<unsigned>(FT02_MeshServicePkiPeerCount()));
    addItem("Mesh PKI", ready ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN, text);
}

void runStorageSpace()
{
    if(!FT02_StorageIsReady())
    {
        addItem("存储空间", FT02_SELFTEST_FAIL, "SD未就绪");
        return;
    }

    const unsigned long freeMb = FT02_StorageFreeMB();
    char text[96];
    snprintf(text, sizeof(text), "SD剩余 %luMB", freeMb);

    addItem("存储空间",
            freeMb < 500UL ? FT02_SELFTEST_WARN : FT02_SELFTEST_PASS,
            text);
}

void runMemoryHealth()
{
    const uint32_t freeHeap = ESP.getFreeHeap();
    const uint32_t totalPsram = ESP.getPsramSize();
    const uint32_t freePsram = ESP.getFreePsram();

    char text[96];
    snprintf(text, sizeof(text), "Heap %luKB PSRAM %lu/%luKB",
             static_cast<unsigned long>(freeHeap / 1024U),
             static_cast<unsigned long>(freePsram / 1024U),
             static_cast<unsigned long>(totalPsram / 1024U));

    FT02SelfTestState state = FT02_SELFTEST_PASS;
    if(freeHeap < 32U * 1024U || (totalPsram > 0u && freePsram < 256U * 1024U))
        state = FT02_SELFTEST_FAIL;
    else if(freeHeap < 64U * 1024U || (totalPsram > 0u && freePsram < 512U * 1024U))
        state = FT02_SELFTEST_WARN;

    addItem("内存状态", state, text);
}

void runMeshCounters()
{
    char text[96];
    snprintf(text, sizeof(text), "RF %lu · 解码 %lu",
             static_cast<unsigned long>(FT02_MeshServiceRawFrameCount()),
             static_cast<unsigned long>(FT02_MeshServiceDecodedFrameCount()));
    addItem("Mesh接收", FT02_MeshServiceReady() ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN, text);
}

const char* resetReasonText(esp_reset_reason_t reason)
{
    switch(reason)
    {
        case ESP_RST_POWERON: return "正常上电";
        case ESP_RST_SW: return "软件复位";
        case ESP_RST_PANIC: return "异常崩溃复位";
        case ESP_RST_INT_WDT: return "中断看门狗";
        case ESP_RST_TASK_WDT: return "任务看门狗";
        case ESP_RST_WDT: return "其他看门狗";
        case ESP_RST_BROWNOUT: return "欠压复位";
        case ESP_RST_DEEPSLEEP: return "深睡眠唤醒";
        case ESP_RST_EXT: return "外部复位";
        default: return "其他复位";
    }
}

void runSleepBattery()
{
    const FT02SleepTruthSnapshot truth = FT02_SleepTruthSnapshotCurrent();
    if(!truth.valid || truth.elapsedUs == 0u)
    {
        addItem("休眠耗电", FT02_SELFTEST_NA, "暂无完整休眠结果");
        return;
    }

    if(!truth.batteryStartSampleValid)
    {
        addItem("休眠耗电", FT02_SELFTEST_NA, "起点无效：睡前电量缓存过旧");
        return;
    }

    if(truth.batteryWakeSettling)
    {
        char pending[64];
        snprintf(
            pending,
            sizeof(pending),
            "终点稳定采样 %u/3",
            static_cast<unsigned>(truth.batteryWakeSampleCount)
        );
        addItem("休眠耗电", FT02_SELFTEST_NA, pending);
        return;
    }

    if(!truth.batterySampleValid)
    {
        addItem("休眠耗电", FT02_SELFTEST_NA, "上次终点样本未完成");
        return;
    }

    const double hours = static_cast<double>(truth.elapsedUs) / 3600000000.0;
    const double drop =
        static_cast<double>(truth.batteryStartPercent - truth.batteryWakePercent);
    const double rate = hours > 0.0 ? drop / hours : 0.0;
    const long deltaMv = lround(
        static_cast<double>(truth.batteryWakeVoltage - truth.batteryStartVoltage) * 1000.0);

    char detail[128];
    if(hours < 2.0)
    {
        snprintf(
            detail,
            sizeof(detail),
            "%.3f→%.3fV Δ%+ldmV | SOC估算 %.0f→%.0f%% | %.2fh 短时参考",
            static_cast<double>(truth.batteryStartVoltage),
            static_cast<double>(truth.batteryWakeVoltage),
            deltaMv,
            static_cast<double>(truth.batteryStartPercent),
            static_cast<double>(truth.batteryWakePercent),
            hours
        );
    }
    else
    {
        snprintf(
            detail,
            sizeof(detail),
            "%.3f→%.3fV Δ%+ldmV | SOC估算 %.0f→%.0f%% | %.2fh %.2f%%/h",
            static_cast<double>(truth.batteryStartVoltage),
            static_cast<double>(truth.batteryWakeVoltage),
            deltaMv,
            static_cast<double>(truth.batteryStartPercent),
            static_cast<double>(truth.batteryWakePercent),
            hours,
            rate
        );
    }

    // This row is measurement evidence, not a system-health verdict. In the
    // current prototype power architecture, SOC-derived drain must not turn
    // the overall Device Status into WARN/FAIL by itself.
    addItem("休眠耗电", FT02_SELFTEST_NA, detail);
}

void runSleepTruth()
{
    const FT02SleepTruthSnapshot truth = FT02_SleepTruthSnapshotCurrent();
    if(!truth.valid)
    {
        addItem("休眠统计", FT02_SELFTEST_NA, "暂无完整休眠结果");
        return;
    }

    const double elapsedHours = static_cast<double>(truth.elapsedUs) / 3600000000.0;
    const double sleepHours = static_cast<double>(truth.lightSleepUs) / 3600000000.0;
    const double awakeMinutes = static_cast<double>(truth.awakeUs) / 60000000.0;
    const double sleepRatio = truth.elapsedUs > 0u
        ? (100.0 * static_cast<double>(truth.lightSleepUs) / static_cast<double>(truth.elapsedUs))
        : 0.0;

    char detail[96];
    snprintf(
        detail,
        sizeof(detail),
        "窗%.2fh 睡%.2fh %.1f%% 醒%.1fmin 会话%lu LS%lu 异常%lu",
        elapsedHours,
        sleepHours,
        sleepRatio,
        awakeMinutes,
        static_cast<unsigned long>(truth.sessionCount),
        static_cast<unsigned long>(truth.lightSleepReturnCount),
        static_cast<unsigned long>(truth.unexpectedReturnCount)
    );

    FT02SelfTestState state = FT02_SELFTEST_PASS;
    if(truth.resetDuringWindowCount > 0u || truth.lastSleepErr != ESP_OK)
        state = FT02_SELFTEST_FAIL;
    else if(truth.unexpectedReturnCount > 0u ||
            truth.lightSleepEntryCount != truth.lightSleepReturnCount ||
            truth.lastWakeCause != static_cast<int32_t>(ESP_SLEEP_WAKEUP_EXT1) ||
            sleepRatio < 95.0)
        state = FT02_SELFTEST_WARN;

    addItem("休眠统计", state, detail);
}

void runStartupState()
{
    const esp_reset_reason_t reason = esp_reset_reason();
    const char* resetDetail = resetReasonText(reason);
    const FT02SleepDiagnosticsSnapshot sleepDiag =
        FT02_SleepDiagnosticsSnapshotCurrent();

    char detail[96];
    if(sleepDiag.reason == FT02_SLEEP_EXIT_KEYBOARD)
    {
        snprintf(
            detail,
            sizeof(detail),
            "%s | 休眠键0x%02X #%lu",
            resetDetail,
            static_cast<unsigned>(sleepDiag.lastRaw),
            static_cast<unsigned long>(sleepDiag.keyboardWakeCount)
        );
    }
    else if(sleepDiag.reason == FT02_SLEEP_EXIT_RESET)
    {
        snprintf(
            detail,
            sizeof(detail),
            "%s | 休眠中复位:%s",
            resetDetail,
            resetReasonText(static_cast<esp_reset_reason_t>(sleepDiag.lastResetReason))
        );
    }
    else
    {
        snprintf(detail, sizeof(detail), "%s | 无休眠退出记录", resetDetail);
    }

    FT02SelfTestState state = FT02_SELFTEST_PASS;
    if(reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
       reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT ||
       reason == ESP_RST_BROWNOUT ||
       sleepDiag.reason == FT02_SLEEP_EXIT_RESET)
        state = FT02_SELFTEST_WARN;

    addItem("启动状态", state, detail);
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
        "[%s] FT02 %s SELFTEST-A2 pass=%u warn=%u fail=%u skip=%u elapsed=%lums\n",
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
    // Single-line diagnostic row: item, state and concise explanation.
    FT02_DrawTextPack(display, ft02_cjk_20r, item.name, x, baselineY);
    FT02_DrawTextPack(display, ft02_cjk_20r, stateText(item.state), x + 150, baselineY);
    FT02_DrawTextPack(display, ft02_cjk_20r, item.detail, x + 258, baselineY);
}


void drawPowerSleepPage(FT02Display& display)
{
    const FT02BatterySnapshot battery = FT02_BatterySnapshotCurrent();
    const FT02SleepTruthSnapshot truth = FT02_SleepTruthSnapshotCurrent();

    const int left = 48;
    const int right = 752;

    // Section 1: current battery. This page is observational and uses only
    // already-published BatteryMonitor cache; drawing it never forces I2C.
    FT02_DrawTextPack(display, ft02_cjk_24b, "当前电池", left, 187);
    char line[128];
    if(battery.monitorPresent && battery.valid)
    {
        snprintf(line, sizeof(line), "SOC估算 %.0f%%    电压 %.3fV    %s",
                 static_cast<double>(battery.percent),
                 static_cast<double>(battery.voltage),
                 FT02_BatteryIsCharging() ? "充电中" : "供电中");
    }
    else
    {
        snprintf(line, sizeof(line), "SOC估算暂无有效缓存    ENTER可重新自检");
    }
    FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 216);
    display.drawLine(left, 229, right, 229, GxEPD_BLACK);

    // Section 2: last independent sleep energy session. Split the former
    // single long row into multiple lines so voltage/SOC/time evidence is never
    // clipped off-screen.
    FT02_DrawTextPack(display, ft02_cjk_24b, "上次休眠耗电", left, 259);
    if(!truth.valid || truth.elapsedUs == 0u)
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "暂无完整休眠结果", left, 288);
    }
    else if(!truth.batteryStartSampleValid)
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "起点无效：睡前电量缓存过旧", left, 288);
    }
    else if(truth.batteryWakeSettling)
    {
        snprintf(line, sizeof(line), "终点稳定采样 %u/3",
                 static_cast<unsigned>(truth.batteryWakeSampleCount));
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 288);
    }
    else if(!truth.batterySampleValid)
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "上次终点样本未完成", left, 288);
    }
    else
    {
        const double hours = static_cast<double>(truth.elapsedUs) / 3600000000.0;
        const double drop = static_cast<double>(truth.batteryStartPercent - truth.batteryWakePercent);
        const double rate = hours > 0.0 ? drop / hours : 0.0;
        const long deltaMv = lround(
            static_cast<double>(truth.batteryWakeVoltage - truth.batteryStartVoltage) * 1000.0);

        snprintf(line, sizeof(line), "电压 %.3f → %.3fV    Δ%+ldmV",
                 static_cast<double>(truth.batteryStartVoltage),
                 static_cast<double>(truth.batteryWakeVoltage),
                 deltaMv);
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 288);

        snprintf(line, sizeof(line), "SOC估算 %.0f → %.0f%%    时长 %.2fh",
                 static_cast<double>(truth.batteryStartPercent),
                 static_cast<double>(truth.batteryWakePercent),
                 hours);
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 315);

        if(hours < 2.0)
            snprintf(line, sizeof(line), "趋势：短时参考，不外推 %%/h");
        else
            snprintf(line, sizeof(line), "SOC长时趋势 %.2f%%/h（分析参考）", rate);
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 342);
    }
    display.drawLine(left, 353, right, 353, GxEPD_BLACK);

    // Section 3: Light-Sleep truth. Keep each diagnostic dimension visible on
    // its own line instead of packing the whole session into a clipped row.
    FT02_DrawTextPack(display, ft02_cjk_24b, "休眠统计", left, 377);
    if(!truth.valid)
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "暂无完整休眠结果", left, 405);
    }
    else
    {
        const double elapsedHours = static_cast<double>(truth.elapsedUs) / 3600000000.0;
        const double sleepHours = static_cast<double>(truth.lightSleepUs) / 3600000000.0;
        const double awakeMinutes = static_cast<double>(truth.awakeUs) / 60000000.0;
        const double sleepRatio = truth.elapsedUs > 0u
            ? (100.0 * static_cast<double>(truth.lightSleepUs) / static_cast<double>(truth.elapsedUs))
            : 0.0;

        snprintf(line, sizeof(line), "窗口 %.2fh    实睡 %.2fh    占比 %.1f%%",
                 elapsedHours, sleepHours, sleepRatio);
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 405);

        snprintf(line, sizeof(line), "清醒 %.1fmin    会话%lu    LS%lu    异常%lu",
                 awakeMinutes,
                 static_cast<unsigned long>(truth.sessionCount),
                 static_cast<unsigned long>(truth.lightSleepReturnCount),
                 static_cast<unsigned long>(truth.unexpectedReturnCount));
        FT02_DrawTextPack(display, ft02_cjk_20r, line, left, 430);
    }
}
}

void FT02_SystemSelfTestRefreshPassive()
{
    const uint32_t started = millis();
    const uint32_t nextGeneration = g_report.generation + 1U;
    memset(&g_report, 0, sizeof(g_report));
    g_report.generation = nextGeneration;
    g_pageIndex = 0;

    // Observational Device Status: cached/in-memory state only. No SD writes,
    // no file probes, no forced MAX17048 read, no audio acquisition.
    // Page 1/4: core hardware (6 items)
    runMainControl();
    runPsram();
    runStoragePassive();
    runKeyboard();
    runAudio();
    runTime();

    // Page 2/4: navigation and communications (6 items)
    runGnss();
    runMeshRadio();
    runMeshPki();

    // Page 3/4: data and system health (7 items)
    runMapPassive();
    runManual();
    runPinyin();
    runStorageSpace();
    runMemoryHealth();
    runMeshCounters();
    runStartupState();

    // Page 4/4: power and sleep diagnostics (3 items)
    runBatteryCached();
    runSleepBattery();
    runSleepTruth();

    g_report.elapsedMs = millis() - started;
    // Deliberately no saveReportLog(): merely opening Device Status must not
    // mutate SD state or change the next sleep's hardware starting condition.
}

void FT02_SystemSelfTestRun()
{
    const uint32_t started = millis();
    const uint32_t nextGeneration = g_report.generation + 1U;
    memset(&g_report, 0, sizeof(g_report));
    g_report.generation = nextGeneration;
    g_pageIndex = 0;

    // Page 1/4: Core hardware
    runMainControl();
    runPsram();
    runStorage();
    runKeyboard();
    runAudio();
    runTime();

    // Page 2/4: Navigation and communications
    runGnss();       // GNSS / GNSS定位 / 罗盘
    runMeshRadio();  // Mesh无线 / Mesh网络
    runMeshPki();    // Mesh PKI

    // Page 3/4: Data and system health
    runMap();
    runManual();
    runPinyin();
    runStorageSpace();
    runMemoryHealth();
    runMeshCounters();
    runStartupState();

    // Page 4/4: Power and sleep diagnostics
    runBattery();
    runSleepBattery();
    runSleepTruth();

    g_report.elapsedMs = millis() - started;
    saveReportLog();
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
        FT02_DrawTextPack(
            display,
            ft02_cjk_24b,
            g_pageIndex == FT02_SELFTEST_POWER_PAGE ? "设备状态  电源与休眠" : "设备状态  系统自检",
            32,
            116
        );

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

        if(g_pageIndex == FT02_SELFTEST_POWER_PAGE)
        {
            drawPowerSleepPage(display);
        }
        else
        {
            const uint8_t startIndex = FT02_SELFTEST_PAGE_STARTS[g_pageIndex];
            const uint8_t requestedEnd = static_cast<uint8_t>(
                startIndex + FT02_SELFTEST_PAGE_LENGTHS[g_pageIndex]);
            const uint8_t endIndex = report.count < requestedEnd ? report.count : requestedEnd;

            uint8_t row = 0;
            for(uint8_t i = startIndex; i < endIndex; ++i, ++row)
            {
                const int baselineY = 184 + static_cast<int>(row) * 41;
                drawItem(display, report.items[i], 48, baselineY);
            }
        }

        FT02_DrawBottomBarWithFont(display, FT02_SELFTEST_BOTTOM_DEFAULT, ft02_cjk_20r);
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display);
}
