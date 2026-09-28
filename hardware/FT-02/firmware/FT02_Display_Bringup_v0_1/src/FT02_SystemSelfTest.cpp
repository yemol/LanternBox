#include "FT02_SystemSelfTest.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Arduino.h>
#include <esp_system.h>

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
#include "FT02_GnssPowerControl.h"
#include "FT02_InputManager.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LR01HostRuntime.h"
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

struct FT02CommErrorBaseline
{
    bool valid = false;
    uint32_t rxErrors = 0;
    uint32_t uartErrors = 0;
    uint32_t radioResets = 0;
};

FT02CommErrorBaseline g_commBaseline = {};

static constexpr uint8_t FT02_EXPECTED_LR01_PROTOCOL = 2;
static constexpr uint8_t FT02_SELFTEST_PAGE_COUNT = 3;
static constexpr uint8_t FT02_SELFTEST_ITEMS_PER_PAGE = 7;

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
            "严重低电量 %.0f%% %.3fV %s %+.1f%%/h",
            static_cast<double>(battery.percent),
            static_cast<double>(battery.voltage),
            flow,
            static_cast<double>(battery.ratePercentPerHour)
        );
        addItem("电池", FT02_SELFTEST_WARN, detail);
        return;
    }

    if(band == FT02_BATTERY_LOW)
    {
        snprintf(
            detail,
            sizeof(detail),
            "低电量 %.0f%% %.3fV %s %+.1f%%/h",
            static_cast<double>(battery.percent),
            static_cast<double>(battery.voltage),
            flow,
            static_cast<double>(battery.ratePercentPerHour)
        );
        addItem("电池", FT02_SELFTEST_WARN, detail);
        return;
    }

    snprintf(
        detail,
        sizeof(detail),
        "%.0f%% %.3fV %s %+.1f%%/h",
        static_cast<double>(battery.percent),
        static_cast<double>(battery.voltage),
        flow,
        static_cast<double>(battery.ratePercentPerHour)
    );
    addItem("电池", FT02_SELFTEST_PASS, detail);
}

void runGnss()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    const FT02GnssSnapshot nav = FT02_GnssSnapshotCurrent();
    const bool userEnabled = FT02_GnssPowerDesiredEnabled();

    if(!FT02_LR01HostOnline())
    {
        addItem("通讯模块", FT02_SELFTEST_FAIL, "Host UART无有效状态");
    }
    else if(lr01.protocolVersion == 0u)
    {
        addItem("通讯模块", FT02_SELFTEST_PASS, "Host UART在线·启动协议帧未捕获");
    }
    else if(lr01.protocolVersion == FT02_EXPECTED_LR01_PROTOCOL)
    {
        addItem("通讯模块", FT02_SELFTEST_PASS, "Host UART协议A2在线");
    }
    else
    {
        char text[96];
        snprintf(text, sizeof(text), "Host UART协议A%u，期望A2", static_cast<unsigned>(lr01.protocolVersion));
        addItem("通讯模块", FT02_SELFTEST_WARN, text);
    }

    if(!userEnabled)
    {
        if(FT02_ModeAwakeMode() == FT02_MODE_INDOOR)
            addItem("GNSS", FT02_SELFTEST_NA, "室内模式·GNSS关闭");
        else
            addItem("GNSS", FT02_SELFTEST_NA, "模式策略·GNSS关闭");
        addItem("GNSS定位", FT02_SELFTEST_NA, "GNSS关闭，未进行定位");
    }
    else if(FT02_GnssPowerActualKnown() && !FT02_GnssPowerActualEnabled())
    {
        addItem("GNSS", FT02_SELFTEST_WARN, "已请求开启，正在唤醒");
        addItem("GNSS定位", FT02_SELFTEST_WARN, "等待GNSS恢复数据流");
    }
    else
    {
        if(lr01.gnssReady)
            addItem("GNSS", FT02_SELFTEST_PASS, "已开启·GNSS数据流正常");
        else
            addItem("GNSS", FT02_SELFTEST_WARN, "已开启·等待GNSS数据流");

        if(nav.fixValid)
        {
            char text[96];
            snprintf(text,sizeof(text),"%u/%u星 %s HDOP %.2f",(unsigned)nav.satellites,(unsigned)nav.satellitesVisible,nav.fixType>=3?"3D定位":"2D定位",nav.hdop);
            addItem("GNSS定位",FT02_SELFTEST_PASS,text);
        }
        else if(lr01.gnssReady)
            addItem("GNSS定位",FT02_SELFTEST_WARN,"数据流正常，当前未定位");
        else
            addItem("GNSS定位",FT02_SELFTEST_WARN,"等待导航模块GNSS恢复");
    }

    // The QMC compass is intentionally independent from GNSS software standby.
    if(lr01.compassReady && lr01.compassValid)
    {
        char text[96];
        if(lr01.compassQuality >= 3)
            snprintf(text,sizeof(text),"航向 %.1f° 已校准·高质量",(double)lr01.headingX10/10.0);
        else if(lr01.compassQuality == 2)
            snprintf(text,sizeof(text),"航向 %.1f° 已校准·可用",(double)lr01.headingX10/10.0);
        else
            snprintf(text,sizeof(text),"航向 %.1f° 建议校准",(double)lr01.headingX10/10.0);
        addItem("罗盘",lr01.compassQuality>=2?FT02_SELFTEST_PASS:FT02_SELFTEST_WARN,text);
    }
    else
        addItem("罗盘",FT02_SELFTEST_FAIL,"导航模块罗盘不可用");
}

void runLoRa()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    if(FT02_LR01HostOnline() && lr01.loraReady)
    {
        char text[96];
        snprintf(text,sizeof(text),"%s %.3fMHz RX%lu",lr01.radioProfile,(double)lr01.frequencyMHz,(unsigned long)lr01.rxCount);
        addItem("LoRa通讯",FT02_SELFTEST_PASS,text);
    }
    else if(FT02_LR01HostOnline())
        addItem("LoRa通讯",FT02_SELFTEST_WARN,"通讯模块在线，Radio未就绪");
    else
        addItem("LoRa通讯",FT02_SELFTEST_FAIL,"通讯模块Host UART离线");

    if(lr01.loraReady && lr01.nodeCount>0)
    {
        char text[96];
        snprintf(text,sizeof(text),"发现%u节点 PKI%u",(unsigned)lr01.nodeCount,(unsigned)lr01.pkiPeerCount);
        addItem("LoRa网络",FT02_SELFTEST_PASS,text);
    }
    else if(lr01.loraReady)
        addItem("LoRa网络",FT02_SELFTEST_WARN,"尚未发现其他节点");
    else
        addItem("LoRa网络",FT02_SELFTEST_WARN,"等待通讯模块Radio就绪");
}

void runAudio()
{
    const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
    if(audio.codecReady && audio.i2sReady)
        addItem("音频", FT02_SELFTEST_PASS, "ES8311与I2S正常");
    else if(!audio.i2sReady)
        addItem("音频", FT02_SELFTEST_FAIL, "I2S初始化失败");
    else
        addItem("音频", FT02_SELFTEST_FAIL, "ES8311无响应");
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
    const FT02LR01State& lr01 = FT02_LR01HostState();

    if(!gnss.timeValid || !lr01.timeValid || lr01.unixTime == 0u)
    {
        if(!FT02_GnssPowerDesiredEnabled())
            addItem("系统时间", FT02_SELFTEST_NA, "GNSS已关闭，未执行卫星校时");
        else
            addItem("系统时间", FT02_SELFTEST_WARN, "尚未获得有效GNSS时间");
        return;
    }

    const time_t utc = static_cast<time_t>(lr01.unixTime);
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

void runHostUart()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    if(!FT02_LR01HostOnline())
    {
        addItem("Host UART", FT02_SELFTEST_FAIL, "通讯模块离线");
        return;
    }

    char text[96];
    if(lr01.protocolVersion == 0u)
    {
        snprintf(text, sizeof(text), "在线 RX%lu·协议启动帧未捕获",
                 static_cast<unsigned long>(FT02_LR01HostRxLineCount()));
        addItem("Host UART", FT02_SELFTEST_PASS, text);
        return;
    }

    snprintf(text, sizeof(text), "A%u RX%lu 当前链路正常",
             static_cast<unsigned>(lr01.protocolVersion),
             static_cast<unsigned long>(FT02_LR01HostRxLineCount()));

    addItem("Host UART",
            lr01.protocolVersion == FT02_EXPECTED_LR01_PROTOCOL
                ? FT02_SELFTEST_PASS
                : FT02_SELFTEST_WARN,
            text);
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

void runCommunicationErrors()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();

    const auto deltaCounter = [](uint32_t current, uint32_t previous) -> uint32_t
    {
        // LR01 counters reset when LR01 reboots. Treat the post-reboot value as
        // the new delta instead of allowing unsigned wrap to create a false alarm.
        return current >= previous ? current - previous : current;
    };

    const uint32_t deltaRx = g_commBaseline.valid
        ? deltaCounter(lr01.rxErrors, g_commBaseline.rxErrors)
        : 0u;
    const uint32_t deltaUart = g_commBaseline.valid
        ? deltaCounter(lr01.uartErrors, g_commBaseline.uartErrors)
        : 0u;
    const uint32_t deltaResets = g_commBaseline.valid
        ? deltaCounter(lr01.radioResets, g_commBaseline.radioResets)
        : 0u;

    const bool newErrors = deltaRx > 0u || deltaUart > 0u || deltaResets > 0u;

    char text[96];
    FT02SelfTestState state = FT02_SELFTEST_PASS;

    if(newErrors)
    {
        snprintf(text, sizeof(text), "本次新增 RX+%lu UART+%lu 复位+%lu",
                 static_cast<unsigned long>(deltaRx),
                 static_cast<unsigned long>(deltaUart),
                 static_cast<unsigned long>(deltaResets));
        state = FT02_SELFTEST_WARN;
    }
    else if(lr01.rxErrors > 0u || lr01.uartErrors > 0u || lr01.radioResets > 0u)
    {
        // LR01 counters are lifetime counters for the current LR01 boot. A
        // historical total must not keep Device Status permanently in WARN.
        // Only counters that increased since the previous self-test are a
        // current warning; stable totals remain visible as diagnostics.
        snprintf(text, sizeof(text), "当前正常·累计 RX%lu UART%lu 复位%lu",
                 static_cast<unsigned long>(lr01.rxErrors),
                 static_cast<unsigned long>(lr01.uartErrors),
                 static_cast<unsigned long>(lr01.radioResets));
    }
    else
    {
        snprintf(text, sizeof(text), "当前正常·无通讯错误");
    }

    g_commBaseline.valid = true;
    g_commBaseline.rxErrors = lr01.rxErrors;
    g_commBaseline.uartErrors = lr01.uartErrors;
    g_commBaseline.radioResets = lr01.radioResets;

    addItem("通讯错误", state, text);
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
}

void FT02_SystemSelfTestRun()
{
    const uint32_t started = millis();
    const uint32_t nextGeneration = g_report.generation + 1U;
    memset(&g_report, 0, sizeof(g_report));
    g_report.generation = nextGeneration;
    g_pageIndex = 0;

    // Page 1/3: Core hardware
    runMainControl();
    runPsram();
    runStorage();
    runKeyboard();
    runAudio();
    runTime();
    runBattery();

    // Page 2/3: Navigation and communications
    runGnss();       // 通讯模块 / GNSS / GNSS定位 / 罗盘
    runLoRa();       // LoRa通讯 / LoRa网络
    runHostUart();   // Host UART

    // Page 3/3: Data and system health
    runMap();
    runManual();
    runPinyin();
    runStorageSpace();
    runMemoryHealth();
    runCommunicationErrors();
    runStartupState();

    g_report.elapsedMs = millis() - started;
    saveReportLog();

    for(uint8_t i = 0; i < g_report.count; ++i)
    {
        const FT02SelfTestItem& item = g_report.items[i];

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

        const uint8_t startIndex =
            static_cast<uint8_t>(g_pageIndex * FT02_SELFTEST_ITEMS_PER_PAGE);
        const uint8_t pageEnd =
            static_cast<uint8_t>(startIndex + FT02_SELFTEST_ITEMS_PER_PAGE);
        const uint8_t endIndex =
            report.count < pageEnd ? report.count : pageEnd;

        uint8_t row = 0;
        for(uint8_t i = startIndex; i < endIndex; ++i, ++row)
        {
            const int baselineY = 184 + static_cast<int>(row) * 41;
            drawItem(display, report.items[i], 48, baselineY);
        }

        FT02_DrawBottomBarWithFont(display, FT02_SELFTEST_BOTTOM_DEFAULT, ft02_cjk_20r);
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display);
}
