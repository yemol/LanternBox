#include "FT02_AudioLogUI.h"

#include <stdio.h>
#include <string.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"

namespace
{
constexpr int FT02_AUDIO_BODY_X = 20;
constexpr int FT02_AUDIO_BODY_Y = 78;
constexpr int FT02_AUDIO_BODY_W = 760;
constexpr int FT02_AUDIO_OPERATION_H = 402;
constexpr int FT02_AUDIO_LIST_ROWS = 5;
constexpr int FT02_AUDIO_ROW_X = 32;
constexpr int FT02_AUDIO_ROW_Y = 132;
constexpr int FT02_AUDIO_ROW_W = 736;
constexpr int FT02_AUDIO_ROW_H = 54;
constexpr int FT02_AUDIO_ROW_GAP = 4;
constexpr int FT02_AUDIO_ROW_BASELINE_OFFSET = 36;
constexpr uint16_t FT02_AUDIO_PARTIAL_LIMIT = 60;

constexpr int FT02_AUDIO_REC_TIMER_X = 496;
constexpr int FT02_AUDIO_REC_TIMER_Y = 178;
constexpr int FT02_AUDIO_REC_TIMER_W = 216;
constexpr int FT02_AUDIO_REC_TIMER_H = 92;

constexpr int FT02_AUDIO_PLAY_STATUS_X = 208;
constexpr int FT02_AUDIO_PLAY_STATUS_Y = 270;
constexpr int FT02_AUDIO_PLAY_STATUS_W = 512;
constexpr int FT02_AUDIO_PLAY_STATUS_H = 88;

uint16_t g_ft02AudioPartialCount = 0;

static const FT02BottomBarItem FT02_AUDIO_LIST_BOTTOM_ITEMS[3] = {
    {nullptr, "方向键 循环选择"},
    {nullptr, "R录音 ENTER/P播放"},
    {nullptr, "T删除 B返回"}
};

static const FT02BottomBarItem FT02_AUDIO_RECORD_BOTTOM_ITEMS[3] = {
    {nullptr, "正在录音并计时"},
    {nullptr, "再次按R结束"},
    {nullptr, "计时区小范围刷新"}
};

static const FT02BottomBarItem FT02_AUDIO_PLAY_BOTTOM_ITEMS[3] = {
    {nullptr, "上下键 调整音量"},
    {nullptr, "B/ENTER/P 停止"},
    {nullptr, "音量自动保存"}
};

static const FT02BottomBarItem FT02_AUDIO_DELETE_BOTTOM_ITEMS[3] = {
    {nullptr, "ENTER 确认"},
    {nullptr, "删除后不可恢复"},
    {nullptr, "B 取消"}
};

static uint16_t FT02_AudioPageStart(uint16_t selectedNewestIndex)
{
    return static_cast<uint16_t>(
        (selectedNewestIndex / FT02_AUDIO_LIST_ROWS) * FT02_AUDIO_LIST_ROWS
    );
}

static void FT02_AudioFormatDuration(
    uint32_t durationMs,
    char* output,
    size_t outputSize
)
{
    const uint32_t totalSeconds = durationMs / 1000u;
    const uint32_t minutes = totalSeconds / 60u;
    const uint32_t seconds = totalSeconds % 60u;
    snprintf(
        output,
        outputSize,
        "%02lu:%02lu",
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds)
    );
}

static void FT02_AudioFormatSize(
    uint64_t bytes,
    char* output,
    size_t outputSize
)
{
    if(bytes >= 1024u * 1024u)
    {
        const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        snprintf(output, outputSize, "%.1fMB", mb);
    }
    else
    {
        const unsigned long kb = static_cast<unsigned long>((bytes + 1023u) / 1024u);
        snprintf(output, outputSize, "%luKB", kb);
    }
}

static bool FT02_AudioEntryHasValidDate(const FT02AudioLogEntry& entry)
{
    return strlen(entry.deviceDate) >= 10 &&
           strcmp(entry.deviceDate, "0000-00-00") != 0;
}

static void FT02_AudioFormatEntryTitle(
    const FT02AudioLogEntry& entry,
    char* output,
    size_t outputSize
)
{
    if(entry.sequence == 0)
    {
        snprintf(output, outputSize, "旧记录  %.38s", entry.audioId);
        return;
    }

    if(FT02_AudioEntryHasValidDate(entry) && strlen(entry.deviceTime) >= 5)
    {
        snprintf(
            output,
            outputSize,
            "#%06lu  %c%c-%c%c %c%c:%c%c",
            static_cast<unsigned long>(entry.sequence),
            entry.deviceDate[5],
            entry.deviceDate[6],
            entry.deviceDate[8],
            entry.deviceDate[9],
            entry.deviceTime[0],
            entry.deviceTime[1],
            entry.deviceTime[3],
            entry.deviceTime[4]
        );
        return;
    }

    snprintf(
        output,
        outputSize,
        "#%06lu  日期待同步",
        static_cast<unsigned long>(entry.sequence)
    );
}

static void FT02_AudioFormatEntryDetail(
    const FT02AudioLogEntry& entry,
    char* output,
    size_t outputSize
)
{
    if(FT02_AudioEntryHasValidDate(entry) && strlen(entry.deviceTime) >= 8)
    {
        snprintf(
            output,
            outputSize,
            "编号 #%06lu  %s %s",
            static_cast<unsigned long>(entry.sequence),
            entry.deviceDate,
            entry.deviceTime
        );
    }
    else
    {
        snprintf(
            output,
            outputSize,
            "编号 #%06lu  日期待同步",
            static_cast<unsigned long>(entry.sequence)
        );
    }
}

static void FT02_DrawAudioHeader(
    FT02Display& display,
    const char* title,
    const char* rightText
)
{
    FT02_DrawTextPack(display, ft02_cjk_24b, title, 32, 115);
    if(rightText != nullptr)
    {
        const int width = FT02_TextWidthPack(ft02_cjk_20r, rightText);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            rightText,
            display.width() - 32 - width,
            113
        );
    }
}

static void FT02_DrawAudioListContent(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    const FT02AudioLogStatus status = FT02_AudioLogStatusCurrent();
    const uint16_t pageStart = FT02_AudioPageStart(selectedNewestIndex);
    const uint16_t pageNumber = status.count == 0
        ? 0
        : static_cast<uint16_t>(pageStart / FT02_AUDIO_LIST_ROWS + 1u);
    const uint16_t pageCount = status.count == 0
        ? 0
        : static_cast<uint16_t>(
            (status.count + FT02_AUDIO_LIST_ROWS - 1u) / FT02_AUDIO_LIST_ROWS
        );

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
    FT02_DrawAudioHeader(display, "语音日志", rightText);

    if(status.count == 0)
    {
        display.drawRect(32, 146, 736, 220, GxEPD_BLACK);
        const char* emptyText = status.storageReady ? "暂无语音日志" : "SD未就绪";
        const int emptyWidth = FT02_TextWidthPack(ft02_cjk_24b, emptyText);
        FT02_DrawTextPack(
            display,
            ft02_cjk_24b,
            emptyText,
            (display.width() - emptyWidth) / 2,
            238
        );
        const char* hint = status.codecReady && status.i2sReady
            ? "按 R 开始第一条录音"
            : "音频模块未就绪，请检查串口日志";
        const int hintWidth = FT02_TextWidthPack(ft02_cjk_20r, hint);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            hint,
            (display.width() - hintWidth) / 2,
            284
        );
        return;
    }

    for(int row = 0; row < FT02_AUDIO_LIST_ROWS; row++)
    {
        const uint16_t newestIndex = static_cast<uint16_t>(pageStart + row);
        if(newestIndex >= status.count) break;

        FT02AudioLogEntry entry;
        if(!FT02_AudioLogGetNewest(newestIndex, entry)) continue;

        const int y = FT02_AUDIO_ROW_Y + row * (FT02_AUDIO_ROW_H + FT02_AUDIO_ROW_GAP);
        const int baselineY = y + FT02_AUDIO_ROW_BASELINE_OFFSET;
        const bool selected = newestIndex == selectedNewestIndex;
        display.fillRect(
            FT02_AUDIO_ROW_X,
            y,
            FT02_AUDIO_ROW_W,
            FT02_AUDIO_ROW_H,
            selected ? GxEPD_BLACK : GxEPD_WHITE
        );
        display.drawRect(
            FT02_AUDIO_ROW_X,
            y,
            FT02_AUDIO_ROW_W,
            FT02_AUDIO_ROW_H,
            GxEPD_BLACK
        );

        char dateLine[96];
        FT02_AudioFormatEntryTitle(entry, dateLine, sizeof(dateLine));

        char duration[20];
        char size[20];
        FT02_AudioFormatDuration(entry.durationMs, duration, sizeof(duration));
        FT02_AudioFormatSize(entry.sizeBytes, size, sizeof(size));
        char metaLine[112];
        snprintf(
            metaLine,
            sizeof(metaLine),
            "时长%s  %s  %s",
            duration,
            size,
            entry.fileExists ? "文件正常" : "文件缺失"
        );

        if(selected)
        {
            FT02_DrawTextPackInvertClipped(
                display,
                ft02_cjk_20r,
                dateLine,
                FT02_AUDIO_ROW_X + 16,
                baselineY,
                FT02_AUDIO_ROW_X,
                y,
                FT02_AUDIO_ROW_W,
                FT02_AUDIO_ROW_H
            );
            FT02_DrawTextPackInvertClipped(
                display,
                ft02_cjk_20r,
                metaLine,
                FT02_AUDIO_ROW_X + 350,
                baselineY,
                FT02_AUDIO_ROW_X,
                y,
                FT02_AUDIO_ROW_W,
                FT02_AUDIO_ROW_H
            );
        }
        else
        {
            FT02_DrawTextPackClipped(
                display,
                ft02_cjk_20r,
                dateLine,
                FT02_AUDIO_ROW_X + 16,
                baselineY,
                FT02_AUDIO_ROW_X,
                y,
                FT02_AUDIO_ROW_W,
                FT02_AUDIO_ROW_H
            );
            FT02_DrawTextPackClipped(
                display,
                ft02_cjk_20r,
                metaLine,
                FT02_AUDIO_ROW_X + 350,
                baselineY,
                FT02_AUDIO_ROW_X,
                y,
                FT02_AUDIO_ROW_W,
                FT02_AUDIO_ROW_H
            );
        }
    }
}

static void FT02_DrawAudioRecordingTimerContent(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    const char* label = status.stopRequested ? "正在保存" : "已录制";
    FT02_DrawTextPack(display, ft02_cjk_20r, label, 530, 205);

    char elapsed[20];
    FT02_AudioFormatDuration(status.recordingElapsedMs, elapsed, sizeof(elapsed));
    FT02_DrawTextPack(display, ft02_cjk_24b, elapsed, 530, 246);
}

static void FT02_DrawAudioRecordingContent(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    FT02_DrawAudioHeader(display, "语音日志", "录制中");
    display.drawRect(72, 150, 656, 224, GxEPD_BLACK);
    display.drawRect(78, 156, 644, 212, GxEPD_BLACK);

    display.fillCircle(150, 244, 34, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "正在录音", 214, 218);
    FT02_DrawTextPack(display, ft02_cjk_20r, "麦克风已开启，请开始说话", 214, 258);
    FT02_DrawTextPack(display, ft02_cjk_20r, "再次按 R 结束并保存", 214, 298);
    FT02_DrawAudioRecordingTimerContent(display, status);

    char idLine[96];
    if(status.activeSequence > 0)
    {
        snprintf(
            idLine,
            sizeof(idLine),
            "编号 #%06lu",
            static_cast<unsigned long>(status.activeSequence)
        );
    }
    else
    {
        snprintf(idLine, sizeof(idLine), "编号准备中");
    }

    FT02_DrawTextPackClipped(
        display,
        ft02_cjk_20r,
        idLine,
        106,
        346,
        92,
        316,
        616,
        40
    );
}

static void FT02_DrawAudioPlayingStatusContent(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    char elapsed[20];
    char duration[20];
    FT02_AudioFormatDuration(status.playbackElapsedMs, elapsed, sizeof(elapsed));
    FT02_AudioFormatDuration(status.playbackDurationMs, duration, sizeof(duration));

    char progressLine[72];
    snprintf(
        progressLine,
        sizeof(progressLine),
        "%s  %s / %s",
        status.playbackStopRequested ? "停止中" : "播放",
        elapsed,
        duration
    );
    FT02_DrawTextPack(display, ft02_cjk_24b, progressLine, 226, 302);

    char volumeLine[96];
    snprintf(
        volumeLine,
        sizeof(volumeLine),
        "音量 %u/10  上下键调整  B/ENTER/P停止",
        static_cast<unsigned int>(status.playbackVolumeLevel)
    );
    FT02_DrawTextPack(display, ft02_cjk_20r, volumeLine, 226, 342);
}

static void FT02_DrawAudioPlayingContent(
    FT02Display& display,
    const FT02AudioLogEntry& entry
)
{
    const FT02AudioLogStatus status = FT02_AudioLogStatusCurrent();
    FT02_DrawAudioHeader(display, "语音日志", "播放中");
    display.drawRect(72, 150, 656, 224, GxEPD_BLACK);
    display.drawRect(78, 156, 644, 212, GxEPD_BLACK);

    display.fillTriangle(122, 198, 122, 278, 190, 238, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "正在播放录音", 226, 214);

    char dateLine[112];
    FT02_AudioFormatEntryDetail(entry, dateLine, sizeof(dateLine));
    FT02_DrawTextPack(display, ft02_cjk_20r, dateLine, 226, 254);
    FT02_DrawAudioPlayingStatusContent(display, status);
}

static void FT02_AudioCommitBodyPartial(
    FT02Display& display,
    void (*drawContent)(FT02Display&, uint16_t),
    uint16_t selectedNewestIndex,
    const char* tag
)
{
    if(g_ft02AudioPartialCount >= FT02_AUDIO_PARTIAL_LIMIT)
    {
        FT02_DrawAudioLogListScreen(display, selectedNewestIndex);
        return;
    }

    display.setPartialWindow(
        FT02_AUDIO_BODY_X,
        FT02_AUDIO_BODY_Y,
        FT02_AUDIO_BODY_W,
        FT02_AUDIO_OPERATION_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_AUDIO_BODY_X,
            FT02_AUDIO_BODY_Y,
            FT02_AUDIO_BODY_W,
            FT02_AUDIO_OPERATION_H,
            GxEPD_WHITE
        );
        drawContent(display, selectedNewestIndex);
        FT02_DrawBottomBarWithFont(display, FT02_AUDIO_LIST_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02AudioPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, tag);
}
}

void FT02_DrawAudioLogListScreen(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    g_ft02AudioPartialCount = 0;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawAudioListContent(display, selectedNewestIndex);
        FT02_DrawBottomBarWithFont(display, FT02_AUDIO_LIST_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "audio-log-list-full");
}

void FT02_DrawAudioLogListBodyPartial(
    FT02Display& display,
    uint16_t selectedNewestIndex
)
{
    FT02_AudioCommitBodyPartial(
        display,
        FT02_DrawAudioListContent,
        selectedNewestIndex,
        "audio-log-list-partial"
    );
}

void FT02_DrawAudioLogRecordingBodyPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    display.setPartialWindow(
        FT02_AUDIO_BODY_X,
        FT02_AUDIO_BODY_Y,
        FT02_AUDIO_BODY_W,
        FT02_AUDIO_OPERATION_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_AUDIO_BODY_X,
            FT02_AUDIO_BODY_Y,
            FT02_AUDIO_BODY_W,
            FT02_AUDIO_OPERATION_H,
            GxEPD_WHITE
        );
        FT02_DrawAudioRecordingContent(display, status);
        FT02_DrawBottomBarWithFont(display, FT02_AUDIO_RECORD_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02AudioPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, "audio-log-recording-cue");
}

void FT02_DrawAudioLogRecordingTimerPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    display.setPartialWindow(
        FT02_AUDIO_REC_TIMER_X,
        FT02_AUDIO_REC_TIMER_Y,
        FT02_AUDIO_REC_TIMER_W,
        FT02_AUDIO_REC_TIMER_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_AUDIO_REC_TIMER_X,
            FT02_AUDIO_REC_TIMER_Y,
            FT02_AUDIO_REC_TIMER_W,
            FT02_AUDIO_REC_TIMER_H,
            GxEPD_WHITE
        );
        FT02_DrawAudioRecordingTimerContent(display, status);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02AudioPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, "audio-log-recording-timer");
}

void FT02_DrawAudioLogPlayingBodyPartial(
    FT02Display& display,
    const FT02AudioLogEntry& entry
)
{
    display.setPartialWindow(
        FT02_AUDIO_BODY_X,
        FT02_AUDIO_BODY_Y,
        FT02_AUDIO_BODY_W,
        FT02_AUDIO_OPERATION_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_AUDIO_BODY_X,
            FT02_AUDIO_BODY_Y,
            FT02_AUDIO_BODY_W,
            FT02_AUDIO_OPERATION_H,
            GxEPD_WHITE
        );
        FT02_DrawAudioPlayingContent(display, entry);
        FT02_DrawBottomBarWithFont(display, FT02_AUDIO_PLAY_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02AudioPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, "audio-log-playing-cue");
}

void FT02_DrawAudioLogPlayingStatusPartial(
    FT02Display& display,
    const FT02AudioLogStatus& status
)
{
    display.setPartialWindow(
        FT02_AUDIO_PLAY_STATUS_X,
        FT02_AUDIO_PLAY_STATUS_Y,
        FT02_AUDIO_PLAY_STATUS_W,
        FT02_AUDIO_PLAY_STATUS_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_AUDIO_PLAY_STATUS_X,
            FT02_AUDIO_PLAY_STATUS_Y,
            FT02_AUDIO_PLAY_STATUS_W,
            FT02_AUDIO_PLAY_STATUS_H,
            GxEPD_WHITE
        );
        FT02_DrawAudioPlayingStatusContent(display, status);
    }
    while(display.nextPage());
    display.setFullWindow();
    g_ft02AudioPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, "audio-log-playing-status");
}

void FT02_DrawAudioLogDeleteConfirmScreen(
    FT02Display& display,
    const FT02AudioLogEntry& entry
)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawAudioHeader(display, "删除语音日志", "需要确认");

        display.drawRect(82, 150, 636, 238, GxEPD_BLACK);
        display.drawRect(86, 154, 628, 230, GxEPD_BLACK);

        const char* warning = "将永久删除这条录音和索引";
        const int warningWidth = FT02_TextWidthPack(ft02_cjk_24b, warning);
        FT02_DrawTextPack(
            display,
            ft02_cjk_24b,
            warning,
            (display.width() - warningWidth) / 2,
            208
        );

        char dateLine[112];
        FT02_AudioFormatEntryDetail(entry, dateLine, sizeof(dateLine));
        const int dateWidth = FT02_TextWidthPack(ft02_cjk_20r, dateLine);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            dateLine,
            (display.width() - dateWidth) / 2,
            254
        );

        char duration[20];
        FT02_AudioFormatDuration(entry.durationMs, duration, sizeof(duration));
        char metaLine[96];
        snprintf(
            metaLine,
            sizeof(metaLine),
            "时长 %s  文件 %s",
            duration,
            entry.fileExists ? "正常" : "缺失"
        );
        const int metaWidth = FT02_TextWidthPack(ft02_cjk_20r, metaLine);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            metaLine,
            (display.width() - metaWidth) / 2,
            294
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

        FT02_DrawBottomBarWithFont(display, FT02_AUDIO_DELETE_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "audio-log-delete-confirm");
}
