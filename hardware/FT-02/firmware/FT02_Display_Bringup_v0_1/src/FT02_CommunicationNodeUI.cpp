#include "FT02_CommunicationNodeUI.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_Gnss.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_Log.h"
#include "FT02_MeshService.h"
#include "FT02_MeshProtocol.h"
#include "FT02_MeshVoiceAudio.h"
#include "FT02_MeshVoiceProtocol.h"
#include "FT02_MeshVoiceTransport.h"
#include "FT02_PinyinIme.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_TextInputMode.h"

namespace
{
enum CommMode : uint8_t
{
    COMM_INBOX = 0,
    COMM_FILTER,
    COMM_SEND_MENU,
    COMM_NODES,
    COMM_COMPOSE,
    COMM_OUTBOX,
    COMM_OUTBOX_CANCEL_CONFIRM,
    COMM_OUTBOX_CLEAR_CONFIRM,
    COMM_DIAG,
    COMM_VOICE
};

enum NodeAction : uint8_t
{
    NODE_ACTION_BROWSE = 0,
    NODE_ACTION_PRIVATE_TEXT,
    NODE_ACTION_PRIVATE_VOICE
};

enum HistoryFilter : uint8_t
{
    HISTORY_ALL = 0,
    HISTORY_BROADCAST,
    HISTORY_DIRECT
};

constexpr size_t VISIBLE_NODE_ROWS = 4;
constexpr uint32_t COMPOSE_REDRAW_IDLE_MS = 320u;
constexpr uint32_t MESH_SYNC_NOTICE_TIMEOUT_MS = 6000u;
constexpr size_t PINYIN_VISIBLE_CANDIDATES = FT02_PINYIN_PAGE_SIZE;

CommMode g_mode = COMM_INBOX;
HistoryFilter g_historyFilter = HISTORY_ALL;
uint8_t g_filterSelection = 0u;
NodeAction g_nodeAction = NODE_ACTION_BROWSE;
uint8_t g_sendSelection = 0u;

// Unit CardKB2 (U215) I2C factory firmware consumes Fn+D/Z/X/C internally
// and does not emit those arrow combinations to the I2C host. Use translated
// Sym-layer angle brackets instead; they arrive as ordinary ASCII bytes and
// provide an unambiguous left/right compose shortcut.
static constexpr uint8_t CARDKB_MODE_LEFT  = static_cast<uint8_t>('<');
static constexpr uint8_t CARDKB_MODE_RIGHT = static_cast<uint8_t>('>');
CommMode g_composeReturnMode = COMM_INBOX;
CommMode g_nodesReturnMode = COMM_INBOX;
CommMode g_voiceReturnMode = COMM_INBOX;
bool g_voiceStartPending = false;
bool g_voiceCaptureCueReady = false;
uint32_t g_voicePendingDestination = FT02_MESH_BROADCAST_NODE;
bool g_voiceTxObserved = false;
size_t g_messageIndex = 0;
size_t g_nodeSelection = 0;
size_t g_outboxIndex = 0;
uint32_t g_pendingCancelLogicalId = 0;
bool g_composePrivate = false;
bool g_composeReply = false;
uint32_t g_composeTarget = 0;
uint32_t g_replyPacketId = 0;
char g_composeText[FT02_LORA_USER_TEXT_MAX_BYTES + 1] = {};
size_t g_composeLength = 0;
char g_pinyin[FT02_PINYIN_MAX_INPUT + 1] = {};
size_t g_pinyinLength = 0;
size_t g_pinyinPage = 0;
FT02MessageDeliveryMode g_composeDeliveryMode = FT02_MESSAGE_IMMEDIATE;
bool g_composeDirty = false;
uint32_t g_composeLastEditMs = 0;
char g_notice[96] = {};
bool g_syncNoticeActive = false;
uint32_t g_syncNoticeStartedMs = 0u;
uint32_t g_syncNoticeNodeInfoRxBaseline = 0u;

static const FT02BottomBarItem BOTTOM_INBOX[3] = {
    {nullptr, "↑↓浏览 · F筛选"}, {nullptr, "ENTER回复/播放 · T新建"}, {nullptr, "S诊断"}
};
static const FT02BottomBarItem BOTTOM_FILTER[3] = {
    {nullptr, "↑↓选择"}, {nullptr, "ENTER应用筛选"}, {nullptr, "B返回消息"}
};
static const FT02BottomBarItem BOTTOM_SEND_MENU[3] = {
    {nullptr, "↑↓选择"}, {nullptr, "ENTER确认"}, {nullptr, "B返回收件箱"}
};
static const FT02BottomBarItem BOTTOM_NODES[3] = {
    {nullptr, "↑↓浏览节点"}, {nullptr, "R刷新身份"}, {nullptr, "B返回诊断"}
};
static const FT02BottomBarItem BOTTOM_NODES_PICK_TEXT[3] = {
    {nullptr, "↑↓选择收件人"}, {nullptr, "ENTER确认私聊文字"}, {nullptr, "B返回新建"}
};
static const FT02BottomBarItem BOTTOM_NODES_PICK_VOICE[3] = {
    {nullptr, "↑↓选择收件人"}, {nullptr, "ENTER确认私聊语音"}, {nullptr, "B返回新建"}
};
static const FT02BottomBarItem BOTTOM_VOICE_RECORD[3] = {
    {nullptr, "最长15秒"}, {nullptr, "ENTER停止并发送"}, {nullptr, "B取消"}
};
static const FT02BottomBarItem BOTTOM_VOICE_PREP[3] = {
    {nullptr, "准备录音"}, {nullptr, "看到提示后说话"}, {nullptr, "B取消"}
};
static const FT02BottomBarItem BOTTOM_VOICE_PROCESS[3] = {
    {nullptr, "处理中"}, {nullptr, "正在保存/压缩语音"}, {nullptr, "请稍候"}
};
static const FT02BottomBarItem BOTTOM_VOICE_TX[3] = {
    {nullptr, "发送中"}, {nullptr, "等待发送完成"}, {nullptr, "B取消"}
};
static const FT02BottomBarItem BOTTOM_VOICE_IDLE[3] = {
    {nullptr, "语音已结束"}, {nullptr, ""}, {nullptr, "B返回"}
};
static const FT02BottomBarItem BOTTOM_COMPOSE_CN[3] = {
    {nullptr, "空格首选 / 1-5选词"}, {nullptr, "Sym+V/B 模式·候选"}, {nullptr, "长按SYM中英 / ENTER发送"}
};
static const FT02BottomBarItem BOTTOM_COMPOSE_EN[3] = {
    {nullptr, "英文直接输入"}, {nullptr, "Sym+V/B模式 / 长按SYM中英"}, {nullptr, "DEL删除 / ENTER发送"}
};
static const FT02BottomBarItem BOTTOM_OUTBOX[3] = {
    {nullptr, "方向键 浏览"}, {nullptr, "DEL 撤销 / K清空"}, {nullptr, "B 收件箱"}
};
static const FT02BottomBarItem BOTTOM_DIAG[3] = {
    {nullptr, "R重同步"}, {nullptr, "M节点列表"}, {nullptr, "B返回消息"}
};

void setNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
    g_syncNoticeActive = false;
    g_syncNoticeStartedMs = 0u;
}

void setSyncNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
    g_syncNoticeActive = g_notice[0] != '\0';
    g_syncNoticeStartedMs = g_syncNoticeActive ? millis() : 0u;
    g_syncNoticeNodeInfoRxBaseline = FT02_MeshServiceNodeInfoRxCount();
}

bool isUtf8Continuation(uint8_t c)
{
    return (c & 0xC0u) == 0x80u;
}

void utf8Backspace()
{
    if(g_composeLength == 0) return;
    size_t n = g_composeLength - 1;
    while(n > 0 && isUtf8Continuation(static_cast<uint8_t>(g_composeText[n]))) --n;
    g_composeLength = n;
    g_composeText[n] = '\0';
}

void appendRaw(char raw)
{
    const uint8_t b = static_cast<uint8_t>(raw);
    if(b < 0x20u || b == 0x7Fu) return;
    if(g_composeLength >= FT02_LORA_USER_TEXT_MAX_BYTES) return;
    g_composeText[g_composeLength++] = raw;
    g_composeText[g_composeLength] = '\0';
}

void resetPinyin()
{
    g_pinyinLength = 0;
    g_pinyin[0] = '\0';
    g_pinyinPage = 0;
}

bool appendUtf8ToCompose(const char* utf8)
{
    if(utf8 == nullptr || utf8[0] == '\0') return false;
    const size_t n = strlen(utf8);
    if(g_composeLength + n > FT02_LORA_USER_TEXT_MAX_BYTES)
    {
        setNotice("消息长度已达上限");
        return false;
    }
    memcpy(g_composeText + g_composeLength, utf8, n);
    g_composeLength += n;
    g_composeText[g_composeLength] = '\0';
    return true;
}

bool appendAsciiToCompose(char raw)
{
    const uint8_t b = static_cast<uint8_t>(raw);
    if(b < 0x20u || b == 0x7Fu || b >= 0x80u) return false;
    if(g_composeLength >= FT02_LORA_USER_TEXT_MAX_BYTES)
    {
        setNotice("消息长度已达上限");
        return false;
    }
    g_composeText[g_composeLength++] = raw;
    g_composeText[g_composeLength] = '\0';
    return true;
}

const char* chinesePunctuation(uint8_t raw)
{
    switch(raw)
    {
        case ',': return "，";
        case '.': return "。";
        case '?': return "？";
        case '!': return "！";
        case ':': return "：";
        case ';': return "；";
        default: return nullptr;
    }
}

bool appendComposePunctuationOrAscii(uint8_t raw)
{
    if(FT02_TextInputModeIsChinese())
    {
        if(const char* mapped = chinesePunctuation(raw))
            return appendUtf8ToCompose(mapped);
    }
    return appendAsciiToCompose(static_cast<char>(raw));
}

void markComposeEdited()
{
    g_composeDirty = true;
    g_composeLastEditMs = millis();
}

size_t currentPinyinCandidateCount()
{
    return FT02_PinyinImeCandidateCount(g_pinyin);
}

void clampPinyinPage()
{
    const size_t count = currentPinyinCandidateCount();
    if(count == 0)
    {
        g_pinyinPage = 0;
        return;
    }
    const size_t pages = (count + PINYIN_VISIBLE_CANDIDATES - 1u) / PINYIN_VISIBLE_CANDIDATES;
    if(g_pinyinPage >= pages) g_pinyinPage = pages - 1u;
}

bool commitPinyinCandidate(size_t slot)
{
    const size_t count = currentPinyinCandidateCount();
    if(count == 0) return false;
    const size_t index = g_pinyinPage * PINYIN_VISIBLE_CANDIDATES + slot;
    if(index >= count) return false;

    char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
    size_t consumed = 0;
    if(!FT02_PinyinImeCandidate(g_pinyin, index, candidate, sizeof(candidate), &consumed)) return false;
    if(consumed == 0 || consumed > g_pinyinLength) return false;
    if(!appendUtf8ToCompose(candidate)) return false;

    // Learn only the Pinyin segment that this committed candidate consumed.
    // For exact phrases this is the full composition; for segmented fallback
    // it is only the first syllable. The message body itself is never stored.
    char learningKey[FT02_PINYIN_MAX_INPUT + 1] = {};
    memcpy(learningKey, g_pinyin, consumed);
    learningKey[consumed] = '\0';
    FT02_PinyinLearningRecord(learningKey, candidate);

    if(consumed >= g_pinyinLength)
    {
        resetPinyin();
    }
    else
    {
        const size_t remain = g_pinyinLength - consumed;
        memmove(g_pinyin, g_pinyin + consumed, remain);
        g_pinyinLength = remain;
        g_pinyin[remain] = '\0';
        g_pinyinPage = 0;
    }
    setNotice("");
    markComposeEdited();
    return true;
}

bool commitRawPinyin()
{
    if(g_pinyinLength == 0) return false;
    if(g_composeLength + g_pinyinLength > FT02_LORA_USER_TEXT_MAX_BYTES)
    {
        setNotice("消息长度已达上限");
        return false;
    }
    memcpy(g_composeText + g_composeLength, g_pinyin, g_pinyinLength);
    g_composeLength += g_pinyinLength;
    g_composeText[g_composeLength] = '\0';
    resetPinyin();
    setNotice("");
    markComposeEdited();
    return true;
}

bool commitPinyinDefault()
{
    if(g_pinyinLength == 0) return false;
    clampPinyinPage();
    if(currentPinyinCandidateCount() > 0) return commitPinyinCandidate(0);
    return commitRawPinyin();
}

void appendPinyinLetter(char raw)
{
    if(g_pinyinLength >= FT02_PINYIN_MAX_INPUT)
    {
        setNotice("拼音过长，请先选字");
        return;
    }
    if(raw >= 'A' && raw <= 'Z') raw = static_cast<char>(raw - 'A' + 'a');
    g_pinyin[g_pinyinLength++] = raw;
    g_pinyin[g_pinyinLength] = '\0';
    g_pinyinPage = 0;
    setNotice("");
    markComposeEdited();
}

void pinyinBackspace()
{
    if(g_pinyinLength == 0) return;
    --g_pinyinLength;
    g_pinyin[g_pinyinLength] = '\0';
    g_pinyinPage = 0;
    setNotice("");
    markComposeEdited();
}

void movePinyinPage(int delta)
{
    const size_t count = currentPinyinCandidateCount();
    if(count == 0) return;
    const size_t pages = (count + PINYIN_VISIBLE_CANDIDATES - 1u) / PINYIN_VISIBLE_CANDIDATES;
    if(pages <= 1u) return;
    int next = static_cast<int>(g_pinyinPage) + delta;
    if(next < 0) next = static_cast<int>(pages) - 1;
    if(next >= static_cast<int>(pages)) next = 0;
    g_pinyinPage = static_cast<size_t>(next);
    markComposeEdited();
}

bool toggleComposeInputMode()
{
    // CardKB2 IME Direct Toggle A1 uses long-SYM -> 0xF1 for mode switching.
    // Printable punctuation, including backtick, remains available to messages.
    // If a Chinese composition is active, commit its default candidate first
    // instead of silently discarding what the user already typed.
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
    {
        if(!commitPinyinDefault())
        {
            setNotice("当前拼音无法上屏，先用0转英文或DEL修改");
            return false;
        }
    }

    const FT02TextInputMode mode = FT02_TextInputModeToggle();
    setNotice(mode == FT02_TEXT_INPUT_CN ? "已切换中文输入" : "已切换英文输入");
    markComposeEdited();
    return true;
}

const char* deliveryModeDetail(FT02MessageDeliveryMode mode)
{
    switch(mode)
    {
        case FT02_MESSAGE_IMMEDIATE: return "即时·10分钟";
        case FT02_MESSAGE_RELIABLE: return "可靠·2小时";
        case FT02_MESSAGE_PERSISTENT: return "持久·至送达/撤销";
        default: return "--";
    }
}

void cycleComposeDeliveryMode(int delta)
{
    if(!g_composePrivate)
    {
        setNotice("广播在A1中固定为即时消息");
        markComposeEdited();
        return;
    }
    int mode = static_cast<int>(g_composeDeliveryMode);
    mode = (mode + delta + 3) % 3;
    g_composeDeliveryMode = static_cast<FT02MessageDeliveryMode>(mode);
    char notice[64];
    snprintf(notice, sizeof(notice), "消息模式：%s", deliveryModeDetail(g_composeDeliveryMode));
    setNotice(notice);
    markComposeEdited();
}

void beginCompose(bool isPrivate, uint32_t target, CommMode returnMode, bool isReply = false, uint32_t replyPacketId = 0)
{
    g_mode = COMM_COMPOSE;
    g_composeReturnMode = returnMode;
    g_composePrivate = isPrivate;
    g_composeReply = isReply;
    g_composeTarget = target;
    g_replyPacketId = isReply ? replyPacketId : 0;
    g_composeDeliveryMode = FT02_MESSAGE_IMMEDIATE;
    g_composeLength = 0;
    g_composeText[0] = '\0';
    resetPinyin();
    g_composeDirty = false;
    setNotice("");
}

FT02CommunicationInputResult beginVoice(uint32_t destination, CommMode returnMode)
{
    if(FT02_MeshVoiceTransportBusy())
    {
        setNotice("上一条语音仍在发送，请先等待或取消");
        return FT02_COMM_INPUT_REDRAW;
    }
    if(destination != FT02_MESH_BROADCAST_NODE && !FT02_MeshServiceCanSendPrivate(destination))
    {
        setNotice("该节点尚未建立PKI，暂不能发送私聊语音");
        return FT02_COMM_INPUT_REDRAW;
    }

    g_voiceReturnMode = returnMode;
    g_voicePendingDestination = destination;
    g_voiceStartPending = true;
    g_voiceCaptureCueReady = false;
    g_voiceTxObserved = false;
    g_mode = COMM_VOICE;
    setNotice("");
    return FT02_COMM_INPUT_REDRAW_THEN_START_VOICE;
}

const char* nodeName(uint32_t node, char* out, size_t outSize)
{
    FT02LoRaNodeView view;
    if(FT02_LoRaNodeRuntimeFindNode(node, view))
    {
        if(view.longName[0])
        {
            snprintf(out, outSize, "%s", view.longName);
            return out;
        }
        if(view.shortName[0])
        {
            snprintf(out, outSize, "%s", view.shortName);
            return out;
        }
    }
    snprintf(out, outSize, "!%08lX", static_cast<unsigned long>(node));
    return out;
}

void drawUtf8Lines(FT02Display& display, const char* text, int x, int firstBaseline, size_t maxBytesPerLine, int lineHeight, size_t maxLines)
{
    if(text == nullptr || !text[0]) return;
    const size_t len = strlen(text);
    size_t offset = 0;
    for(size_t line = 0; line < maxLines && offset < len; ++line)
    {
        size_t count = (len - offset) < maxBytesPerLine ? (len - offset) : maxBytesPerLine;
        if(offset + count < len)
        {
            while(count > 0 && isUtf8Continuation(static_cast<uint8_t>(text[offset + count]))) --count;
        }
        if(count == 0) break;
        char buf[96];
        if(count >= sizeof(buf)) count = sizeof(buf) - 1;
        memcpy(buf, text + offset, count);
        buf[count] = '\0';
        FT02_DrawTextPack(display, ft02_cjk_20r, buf, x, firstBaseline + static_cast<int>(line) * lineHeight);
        offset += count;
    }
}

void drawHeader(FT02Display& display, const char* section)
{
    FT02_DrawStatusBar(display);
    FT02_DrawTextPack(display, ft02_cjk_24b, "内部通讯", 32, 116);
    FT02_DrawTextPack(display, ft02_cjk_20r, section, 166, 116);
    display.drawLine(32, 128, 768, 128, GxEPD_BLACK);
}

void drawCenteredText(FT02Display& display, const FT02FontPack& font, const char* text, int centerX, int baselineY)
{
    const int w = FT02_TextWidthPack(font, text != nullptr ? text : "");
    FT02_DrawTextPack(display, font, text != nullptr ? text : "", centerX - w / 2, baselineY);
}


void drawCenteredTextScaled(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int centerX,
    int baselineY,
    uint8_t scaleNum,
    uint8_t scaleDen
)
{
    const char* safe = text != nullptr ? text : "";
    const int w = FT02_TextWidthPackScaled(font, safe, scaleNum, scaleDen);
    FT02_DrawTextPackScaled(display, font, safe, centerX - w / 2, baselineY, scaleNum, scaleDen);
}


void formatMessageTime(const FT02LoRaMessageView& msg, char* out, size_t outSize)
{
    if(outSize == 0) return;
    if(!msg.hasRxTime || msg.rxTimeEpoch < 1600000000u)
    {
        snprintf(out, outSize, "时间--");
        return;
    }
    const time_t epoch = static_cast<time_t>(msg.rxTimeEpoch);
    struct tm localTm;
    if(localtime_r(&epoch, &localTm) == nullptr)
    {
        snprintf(out, outSize, "时间--");
        return;
    }
    snprintf(out, outSize, "%02d:%02d", localTm.tm_hour, localTm.tm_min);
}

void formatNodeAge(uint32_t epoch, char* out, size_t outSize)
{
    if(outSize == 0) return;
    const time_t now = time(nullptr);
    if(epoch < 1600000000u || now < static_cast<time_t>(epoch))
    {
        snprintf(out, outSize, "最近--");
        return;
    }
    const uint32_t age = static_cast<uint32_t>(now - static_cast<time_t>(epoch));
    if(age < 60u) snprintf(out, outSize, "%us前", static_cast<unsigned>(age));
    else if(age < 3600u) snprintf(out, outSize, "%um前", static_cast<unsigned>(age / 60u));
    else if(age < 86400u) snprintf(out, outSize, "%uh前", static_cast<unsigned>(age / 3600u));
    else snprintf(out, outSize, "%ud前", static_cast<unsigned>(age / 86400u));
}

const char* historyFilterText()
{
    if(g_historyFilter == HISTORY_BROADCAST) return "广播";
    if(g_historyFilter == HISTORY_DIRECT) return "私聊";
    return "全部";
}

bool historyMessageMatches(const FT02LoRaMessageView& msg)
{
    if(g_historyFilter == HISTORY_BROADCAST) return msg.broadcast;
    if(g_historyFilter == HISTORY_DIRECT) return !msg.broadcast;
    return true;
}

size_t historyFilteredCount()
{
    const size_t total = FT02_LoRaCommunicationMessageCount();
    size_t matched = 0u;
    FT02LoRaMessageView msg{};
    for(size_t i = 0; i < total; ++i)
        if(FT02_LoRaCommunicationGetMessageNewest(i, msg) && historyMessageMatches(msg)) ++matched;
    return matched;
}

bool historyGetFiltered(size_t filteredIndex, FT02LoRaMessageView& out)
{
    const size_t total = FT02_LoRaCommunicationMessageCount();
    size_t matched = 0u;
    for(size_t i = 0; i < total; ++i)
    {
        FT02LoRaMessageView msg{};
        if(!FT02_LoRaCommunicationGetMessageNewest(i, msg) || !historyMessageMatches(msg)) continue;
        if(matched++ == filteredIndex)
        {
            out = msg;
            return true;
        }
    }
    memset(&out, 0, sizeof(out));
    return false;
}

void formatHistoryPeer(const FT02LoRaMessageView& msg, char* out, size_t outSize)
{
    if(!out || outSize == 0u) return;
    char name[40] = {};
    if(msg.outgoing)
    {
        if(msg.broadcast) snprintf(out, outSize, "我 → 广播");
        else snprintf(out, outSize, "我 → %s", nodeName(msg.to, name, sizeof(name)));
    }
    else
    {
        if(msg.broadcast) snprintf(out, outSize, "广播 · %s", nodeName(msg.from, name, sizeof(name)));
        else snprintf(out, outSize, "%s → 我", nodeName(msg.from, name, sizeof(name)));
    }
}

void drawDashedDivider(FT02Display& display, int y)
{
    for(int x = 42; x < 758; x += 18)
    {
        display.drawFastHLine(x, y, 10, GxEPD_BLACK);
    }
}

void drawHistoryRow(FT02Display& display, const FT02LoRaMessageView& msg, int y, bool selected)
{
    // Keep the timeline visually quiet: selection is a narrow rail, while
    // sender/time/state live in stable columns instead of one variable-width string.
    if(selected) display.fillRect(30, y + 8, 4, 54, GxEPD_BLACK);

    char peer[84] = {};
    formatHistoryPeer(msg, peer, sizeof(peer));
    char when[20] = {};
    formatMessageTime(msg, when, sizeof(when));
    const char* state = msg.outgoing
        ? FT02_MeshMessageStatusText(msg.historyStatus)
        : (msg.unread ? "新消息" : "");

    FT02_DrawTextPack(display, ft02_cjk_20r, peer, 44, y + 22);
    FT02_DrawTextPack(display, ft02_cjk_20r, when, 500, y + 22);
    if(state[0]) FT02_DrawTextPack(display, ft02_cjk_20r, state, 636, y + 22);

    if(msg.messageType == FT02_MESH_MESSAGE_VOICE)
    {
        const uint32_t seconds = (msg.voiceDurationMs + 500u) / 1000u;
        char voice[96] = {};
        snprintf(voice, sizeof(voice), "%s  语音  %lus%s",
                 FT02_MeshVoiceAudioIsPlaying() && selected ? "■" : "▶",
                 static_cast<unsigned long>(seconds),
                 msg.historyStatus == FT02_MESH_STATUS_INCOMPLETE ? "  · 不完整" : "");
        FT02_DrawTextPack(display, ft02_cjk_20r, voice, 58, y + 52);
    }
    else
    {
        drawUtf8Lines(display, msg.text, 58, y + 50, 64, 22, 1);
    }

    drawDashedDivider(display, y + 72);
}

void drawInbox(FT02Display& display)
{
    drawHeader(display, "消息");
    const size_t total = FT02_LoRaCommunicationMessageCount();
    const size_t count = historyFilteredCount();
    if(count == 0u) g_messageIndex = 0u;
    else if(g_messageIndex >= count) g_messageIndex = count - 1u;

    char summary[180];
    snprintf(summary, sizeof(summary),
             "历史 %u/%u   未读 %u   筛选 %s   Mesh %s",
             static_cast<unsigned>(total),
             static_cast<unsigned>(FT02_MESH_HISTORY_LIMIT),
             static_cast<unsigned>(FT02_LoRaCommunicationUnreadCount()),
             historyFilterText(),
             FT02_MeshServiceReady() ? "在线" : "离线");
    FT02_DrawTextPack(display, ft02_cjk_20r, summary, 32, 156);
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 442, 156);

    if(count == 0u)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, total == 0u ? "暂无消息记录" : "当前筛选没有消息", 286, 278);
        FT02_DrawTextPack(display, ft02_cjk_20r, "T 新建消息   ·   S 通讯诊断", 206, 320);
    }
    else
    {
        constexpr size_t VISIBLE_ROWS = 3u;
        size_t pageStart = (g_messageIndex / VISIBLE_ROWS) * VISIBLE_ROWS;
        for(size_t row = 0; row < VISIBLE_ROWS; ++row)
        {
            const size_t filteredIndex = pageStart + row;
            if(filteredIndex >= count) break;
            FT02LoRaMessageView msg{};
            if(!historyGetFiltered(filteredIndex, msg)) continue;
            drawHistoryRow(display, msg, 176 + static_cast<int>(row) * 80, filteredIndex == g_messageIndex);
        }
        char pos[80] = {};
        snprintf(pos, sizeof(pos), "%u/%u", static_cast<unsigned>(g_messageIndex + 1u), static_cast<unsigned>(count));
        FT02_DrawTextPack(display, ft02_cjk_20r, pos, 704, 424);
    }
    // Inbox has a longer action group in the center; use 25/50/25 so
    // message actions stay together without crowding browse/diagnostics.
    FT02_DrawBottomBarWithFontSplits(display, BOTTOM_INBOX, ft02_cjk_20r, 25, 75);
}

void drawFilter(FT02Display& display)
{
    drawHeader(display, "消息筛选");
    FT02_DrawTextPack(display, ft02_cjk_20r, "选择消息列表中要显示的类型", 48, 166);

    static const char* ITEMS[3] = {"全部消息", "广播消息", "私聊消息"};
    static const char* DETAILS[3] = {
        "显示广播与私聊消息",
        "只显示公共频道广播",
        "只显示点对点私聊"
    };

    for(uint8_t i = 0u; i < 3u; ++i)
    {
        const int y = 196 + static_cast<int>(i) * 68;
        if(i == g_filterSelection)
        {
            display.fillRect(48, y + 8, 5, 42, GxEPD_BLACK);
        }
        FT02_DrawTextPack(display, i == g_filterSelection ? ft02_cjk_24b : ft02_cjk_24r, ITEMS[i], 76, y + 30);
        FT02_DrawTextPack(display, ft02_cjk_20r, DETAILS[i], 276, y + 30);
        if(i < 2u) drawDashedDivider(display, y + 58);
    }

    char current[80] = {};
    snprintf(current, sizeof(current), "当前：%s", historyFilterText());
    FT02_DrawTextPack(display, ft02_cjk_20r, current, 48, 414);
    FT02_DrawBottomBarWithFont(display, BOTTOM_FILTER, ft02_cjk_20r);
}

void drawSendMenu(FT02Display& display)
{
    drawHeader(display, "新建消息");
    FT02_DrawTextPack(display, ft02_cjk_20r,
                      "新建发送与历史回复已分离，请明确选择广播或私聊。", 32, 156);

    static const char* ITEMS[4] = {
        "广播文字",
        "广播语音",
        "私聊文字",
        "私聊语音"
    };
    static const char* DETAILS[4] = {
        "向 Primary 全网发送文字",
        "直接开始一条新的广播语音",
        "先选择节点，再编写 PKI 私信",
        "先选择节点，再录制 PKI 私聊语音"
    };

    for(uint8_t i = 0u; i < 4u; ++i)
    {
        const int y = 190 + static_cast<int>(i) * 58;
        display.drawRect(48, y, 704, 50, GxEPD_BLACK);
        if(i == g_sendSelection)
        {
            display.drawRect(50, y + 2, 700, 46, GxEPD_BLACK);
            display.fillRect(58, y + 19, 8, 12, GxEPD_BLACK);
        }
        FT02_DrawTextPack(display, ft02_cjk_24b, ITEMS[i], 82, y + 28);
        FT02_DrawTextPack(display, ft02_cjk_20r, DETAILS[i], 280, y + 28);
    }

    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 32, 430);
    FT02_DrawBottomBarWithFont(display, BOTTOM_SEND_MENU, ft02_cjk_20r);
}

void drawVoice(FT02Display& display)
{
    drawHeader(display, "Mesh语音");
    const FT02MeshVoiceAudioStatus voice = FT02_MeshVoiceAudioStatusCurrent();
    const uint32_t shownDestination = g_voiceStartPending ? g_voicePendingDestination : voice.destination;

    char targetValue[72] = {};
    if(shownDestination == FT02_MESH_BROADCAST_NODE)
    {
        snprintf(targetValue, sizeof(targetValue), "广播");
    }
    else
    {
        char name[40] = {};
        snprintf(targetValue, sizeof(targetValue), "%s", nodeName(shownDestination, name, sizeof(name)));
    }

    // One quiet metadata row. Keep the target left-aligned and codec details on
    // the right so the central status/progress area remains visually stable.
    FT02_DrawTextPack(display, ft02_cjk_24r, "发送对象", 48, 176);
    FT02_DrawTextPack(display, ft02_cjk_24b, targetValue, 166, 176);
    const char* codecMeta = "Opus 6k · 最长15秒";
    const int codecW = FT02_TextWidthPack(ft02_cjk_24r, codecMeta);
    FT02_DrawTextPack(display, ft02_cjk_24r, codecMeta, 752 - codecW, 176);
    display.drawFastHLine(48, 196, 704, GxEPD_BLACK);

    const bool txBusy = FT02_MeshVoiceTransportBusy();
    const uint8_t txPct = txBusy ? FT02_MeshVoiceTransportProgressPercent() : 0u;
    const uint32_t shownMs = voice.elapsedMs > FT02_VOICE_TX_MAX_DURATION_MS
        ? FT02_VOICE_TX_MAX_DURATION_MS : voice.elapsedMs;

    const char* state = g_voiceStartPending ? "准备录音" : "待机";
    if(!g_voiceStartPending && voice.state == FT02_MESH_VOICE_AUDIO_RECORDING)
        state = g_voiceCaptureCueReady ? "● 开始说话" : "麦克风稳定中";
    else if(voice.state == FT02_MESH_VOICE_AUDIO_SAVING) state = "正在保存语音";
    else if(voice.state == FT02_MESH_VOICE_AUDIO_ENCODING) state = "正在压缩语音";
    else if(voice.state == FT02_MESH_VOICE_AUDIO_ERROR) state = "语音处理失败";
    else if(txBusy) state = "正在发送语音";
    drawCenteredTextScaled(display, ft02_cjk_24b, state, 400, 250, 4, 3);

    // Progress is the visual anchor of this page. Its geometry never changes,
    // which avoids the floating/misaligned appearance of the older layout.
    constexpr int BAR_X = 80;
    constexpr int BAR_Y = 276;
    constexpr int BAR_W = 640;
    constexpr int BAR_H = 30;
    display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, GxEPD_BLACK);
    const int innerW = BAR_W - 4;
    const int fill = txBusy
        ? static_cast<int>((static_cast<uint32_t>(txPct) * static_cast<uint32_t>(innerW)) / 100u)
        : static_cast<int>((static_cast<uint64_t>(shownMs) * static_cast<uint64_t>(innerW)) / FT02_VOICE_TX_MAX_DURATION_MS);
    if(fill > 0) display.fillRect(BAR_X + 2, BAR_Y + 2, fill > innerW ? innerW : fill, BAR_H - 4, GxEPD_BLACK);

    if(txBusy)
    {
        char chunks[64] = {};
        snprintf(chunks, sizeof(chunks), "分片 %03u / %03u",
                 static_cast<unsigned>(FT02_MeshVoiceTransportChunksSent()),
                 static_cast<unsigned>(FT02_MeshVoiceTransportTotalChunks()));
        FT02_DrawTextPack(display, ft02_cjk_24b, chunks, BAR_X, 346);

        char pct[16] = {};
        snprintf(pct, sizeof(pct), "%03u%%", static_cast<unsigned>(txPct));
        const int pctW = FT02_TextWidthPack(ft02_cjk_24b, pct);
        FT02_DrawTextPack(display, ft02_cjk_24b, pct, BAR_X + BAR_W - pctW, 346);
        drawCenteredText(display, ft02_cjk_24r, "请保持设备开启，发送完成后自动返回", 400, 394);
    }
    else
    {
        char timer[48] = {};
        snprintf(timer, sizeof(timer), "%02lu:%02lu / 00:15",
                 static_cast<unsigned long>((shownMs / 1000u) / 60u),
                 static_cast<unsigned long>((shownMs / 1000u) % 60u));
        drawCenteredText(display, ft02_title_32b, timer, 400, 346);

        const char* detail = nullptr;
        if(g_voiceStartPending) detail = "请等待“开始说话”提示";
        else if(voice.state == FT02_MESH_VOICE_AUDIO_RECORDING && !g_voiceCaptureCueReady)
            detail = "正在建立录音前缓冲…";
        else if(voice.state == FT02_MESH_VOICE_AUDIO_RECORDING && g_voiceCaptureCueReady)
            detail = "录音已启动，按 ENTER 停止并发送";
        else if(voice.state == FT02_MESH_VOICE_AUDIO_SAVING || voice.state == FT02_MESH_VOICE_AUDIO_ENCODING)
            detail = "正在处理，请稍候";
        else if(voice.message[0]) detail = voice.message;
        if(detail != nullptr && detail[0]) drawCenteredText(display, ft02_cjk_24r, detail, 400, 394);
    }

    const FT02BottomBarItem* bottom = BOTTOM_VOICE_IDLE;
    if(g_voiceStartPending) bottom = BOTTOM_VOICE_PREP;
    else if(voice.state == FT02_MESH_VOICE_AUDIO_RECORDING) bottom = BOTTOM_VOICE_RECORD;
    else if(voice.state == FT02_MESH_VOICE_AUDIO_SAVING ||
            voice.state == FT02_MESH_VOICE_AUDIO_ENCODING) bottom = BOTTOM_VOICE_PROCESS;
    else if(txBusy) bottom = BOTTOM_VOICE_TX;
    FT02_DrawBottomBarWithFont(display, bottom, ft02_cjk_20r);
}

void drawOutbox(FT02Display& display)
{
    drawHeader(display, "发件箱");
    const size_t count = FT02_LoRaMessageOutboxCount();
    char summary[160];
    snprintf(summary, sizeof(summary), "本地投递记录：%u   即时10分钟 / 可靠2小时 / 持久至送达或撤销",
             static_cast<unsigned>(count));
    FT02_DrawTextPack(display, ft02_cjk_20r, summary, 32, 156);
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 32, 181);

    if(count == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "暂无发件记录", 304, 280);
        FT02_DrawTextPack(display, ft02_cjk_20r, "私信编写时用 Sym+V/B 切换投递模式", 214, 320);
    }
    else
    {
        if(g_outboxIndex >= count) g_outboxIndex = count - 1;
        FT02LoRaOutboxView item;
        if(FT02_LoRaMessageGetOutboxNewest(g_outboxIndex, item))
        {
            char targetName[40] = {};
            if(item.broadcast) snprintf(targetName, sizeof(targetName), "Primary广播");
            else nodeName(item.destination, targetName, sizeof(targetName));

            char meta[220];
            snprintf(meta, sizeof(meta), "%s  %s  → %s   %u/%u",
                     FT02_LoRaMessageDeliveryModeText(item.mode),
                     FT02_LoRaMessageDeliveryStateText(item.state),
                     targetName,
                     static_cast<unsigned>(g_outboxIndex + 1u),
                     static_cast<unsigned>(count));
            FT02_DrawTextPack(display, ft02_cjk_24b, meta, 32, 214);

            display.drawRect(32, 230, 736, 122, GxEPD_BLACK);
            drawUtf8Lines(display, item.text, 48, 260, 58, 28, 3);

            char details[220];
            snprintf(details, sizeof(details), "逻辑ID 0x%08lX   尝试 %u   最后包 0x%08lX",
                     static_cast<unsigned long>(item.logicalId),
                     static_cast<unsigned>(item.attemptCount),
                     static_cast<unsigned long>(item.lastPacketId));
            FT02_DrawTextPack(display, ft02_cjk_20r, details, 32, 386);

            const bool active = item.state == FT02_DELIVERY_QUEUED || item.state == FT02_DELIVERY_WAITING_ACK;
            FT02_DrawTextPack(display, ft02_cjk_20r,
                              active ? "DEL 可撤销当前待投递消息" : "该消息已结束投递流程",
                              32, 420);
        }
    }
    FT02_DrawBottomBarWithFont(display, BOTTOM_OUTBOX, ft02_cjk_20r);
}

void drawOutboxCancelConfirm(FT02Display& display)
{
    drawHeader(display, "撤销投递");

    FT02_DrawTextPack(display, ft02_cjk_24r, "确认撤销当前待投递消息？", 214, 220);
    FT02_DrawTextPack(display, ft02_cjk_20r, "撤销后 Core 将停止继续投递该消息。", 200, 276);
    FT02_DrawTextPack(display, ft02_cjk_20r, "确认键 确认撤销    B / ESC 取消", 220, 360);
}

void drawOutboxClearConfirm(FT02Display& display)
{
    drawHeader(display, "清空发件箱");
    const size_t count = FT02_LoRaMessageOutboxCount();

    FT02_DrawTextPack(display, ft02_cjk_24r, "确认清空全部发件记录？", 236, 220);

    char detail[160];
    snprintf(detail, sizeof(detail), "当前共有 %u 条记录。此操作不可恢复。", static_cast<unsigned>(count));
    FT02_DrawTextPack(display, ft02_cjk_20r, detail, 210, 270);

    FT02_DrawTextPack(display, ft02_cjk_20r, "若仍有待确认/待投递消息，清空后 Core 将停止跟踪。", 128, 312);
    FT02_DrawTextPack(display, ft02_cjk_20r, "确认键 确认清空    B / ESC 取消", 220, 370);
}

void drawNodeRow(FT02Display& display, size_t nodeIndex, int y, bool selected)
{
    FT02LoRaNodeView node;
    if(!FT02_LoRaNodeRuntimeGetNode(nodeIndex, node)) return;
    const bool local = node.node == FT02_LoRaNodeRuntimeLocalNode();
    display.drawRect(32, y, 736, 58, GxEPD_BLACK);
    if(selected)
    {
        display.drawRect(34, y + 2, 732, 54, GxEPD_BLACK);
        display.fillRect(39, y + 24, 7, 12, GxEPD_BLACK);
    }
    char line1[140];
    snprintf(
        line1, sizeof(line1), "%s [%s]%s",
        node.longName[0] ? node.longName : "未知节点",
        node.shortName[0] ? node.shortName : "--",
        local ? " 本机" : ""
    );
    FT02_DrawTextPack(display, ft02_cjk_20r, line1, 56, y + 23);

    char route[24];
    if(local) snprintf(route, sizeof(route), "本地");
    else if(node.hasHops && node.hops == 0) snprintf(route, sizeof(route), "直连");
    else if(node.hasHops) snprintf(route, sizeof(route), "%u跳", static_cast<unsigned>(node.hops));
    else snprintf(route, sizeof(route), "路由--");
    char age[24];
    formatNodeAge(node.lastHeardEpoch, age, sizeof(age));
    char line2[210];
    snprintf(
        line2, sizeof(line2), "!%08lX  %s  SNR %.1f  %s  %s",
        static_cast<unsigned long>(node.node), route,
        static_cast<double>(node.snr), age, node.publicKeyValid ? "PKI可私信" : "无PKI"
    );
    FT02_DrawTextPack(display, ft02_cjk_20r, line2, 56, y + 49);
}

void drawNodes(FT02Display& display)
{
    const char* header = "Mesh节点诊断";
    if(g_nodeAction == NODE_ACTION_PRIVATE_TEXT) header = "选择私聊文字收件人";
    else if(g_nodeAction == NODE_ACTION_PRIVATE_VOICE) header = "选择私聊语音收件人";
    drawHeader(display, header);
    const size_t count = FT02_LoRaNodeRuntimeNodeCount();
    char summary[180];
    snprintf(summary, sizeof(summary), "Mesh：%s   节点：%u/%lu   PKI：%u",
             FT02_MeshServiceReady() ? "在线" : "离线",
             static_cast<unsigned>(count),
             static_cast<unsigned long>(FT02_LoRaNodeRuntimeExpectedNodeCount()),
             static_cast<unsigned>(FT02_MeshServicePkiPeerCount()));
    FT02_DrawTextPack(display, ft02_cjk_20r, summary, 32, 156);
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 32, 181);

    if(count == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "正在读取通讯模块节点...", 270, 292);
    }
    else
    {
        if(g_nodeSelection >= count) g_nodeSelection = count - 1;
        const size_t pageStart = (g_nodeSelection / VISIBLE_NODE_ROWS) * VISIBLE_NODE_ROWS;
        const size_t visible = (count - pageStart) < VISIBLE_NODE_ROWS ? (count - pageStart) : VISIBLE_NODE_ROWS;
        for(size_t row = 0; row < visible; ++row)
        {
            const size_t idx = pageStart + row;
            drawNodeRow(display, idx, 194 + static_cast<int>(row) * 60, idx == g_nodeSelection);
        }
    }
    const FT02BottomBarItem* bottom = BOTTOM_NODES;
    if(g_nodeAction == NODE_ACTION_PRIVATE_TEXT) bottom = BOTTOM_NODES_PICK_TEXT;
    else if(g_nodeAction == NODE_ACTION_PRIVATE_VOICE) bottom = BOTTOM_NODES_PICK_VOICE;
    FT02_DrawBottomBarWithFont(display, bottom, ft02_cjk_20r);
}

void drawCompose(FT02Display& display)
{
    drawHeader(display, g_composeReply ? "回复消息" : (g_composePrivate ? "编写私信" : "编写广播"));
    char target[180];
    if(g_composePrivate)
    {
        char fallback[20];
        const char* name = nodeName(g_composeTarget, fallback, sizeof(fallback));
        if(g_composeReply)
        {
            snprintf(target, sizeof(target), "回复：%s  原消息 0x%08lX  PKI + ACK",
                     name, static_cast<unsigned long>(g_replyPacketId));
        }
        else
        {
            snprintf(target, sizeof(target), "收件人：%s  !%08lX  PKI加密 + ACK", name, static_cast<unsigned long>(g_composeTarget));
        }
    }
    else
    {
        snprintf(target, sizeof(target), "收件人：Primary 全网广播");
    }
    FT02_DrawTextPack(display, ft02_cjk_20r, target, 32, 158);

    display.drawRect(32, 178, 736, 170, GxEPD_BLACK);
    if(g_composeLength == 0)
        FT02_DrawTextPack(display, ft02_cjk_20r, "请输入消息...", 48, 210);
    else
        drawUtf8Lines(display, g_composeText, 48, 210, 60, 30, 3);

    // Compose input status occupies the lower part of the editor so mode,
    // Pinyin candidates and punctuation behavior are always visible.
    display.drawFastHLine(40, 286, 720, GxEPD_BLACK);
    if(FT02_TextInputModeIsChinese())
    {
        if(g_pinyinLength > 0)
        {
            char pyDisplay[FT02_PINYIN_MAX_INPUT * 2 + 1] = {};
            FT02_PinyinImeSegmentDisplay(g_pinyin, pyDisplay, sizeof(pyDisplay));
            char pyLine[88];
            snprintf(pyLine, sizeof(pyLine), "[中] 拼音：%s", pyDisplay[0] ? pyDisplay : g_pinyin);
            FT02_DrawTextPack(display, ft02_cjk_20r, pyLine, 48, 312);

            const size_t count = currentPinyinCandidateCount();
            char candLine[220] = {};
            size_t used = 0;
            for(size_t slot = 0; slot < PINYIN_VISIBLE_CANDIDATES; ++slot)
            {
                char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
                const size_t idx = g_pinyinPage * PINYIN_VISIBLE_CANDIDATES + slot;
                if(idx >= count || !FT02_PinyinImeCandidate(g_pinyin, idx, candidate, sizeof(candidate))) break;
                const int wrote = snprintf(candLine + used, sizeof(candLine) - used, "%u%s  ",
                                           static_cast<unsigned>(slot + 1u), candidate);
                if(wrote <= 0 || static_cast<size_t>(wrote) >= sizeof(candLine) - used) break;
                used += static_cast<size_t>(wrote);
            }
            if(count == 0)
            {
                snprintf(candLine, sizeof(candLine), "继续输入拼音；0按原英文上屏");
            }
            else
            {
                const size_t pages = (count + PINYIN_VISIBLE_CANDIDATES - 1u) / PINYIN_VISIBLE_CANDIDATES;
                if(pages > 1u && used < sizeof(candLine) - 24u)
                {
                    snprintf(candLine + used, sizeof(candLine) - used, " %u/%u",
                             static_cast<unsigned>(g_pinyinPage + 1u),
                             static_cast<unsigned>(pages));
                }
            }
            FT02_DrawTextPack(display, ft02_cjk_20r, candLine, 48, 338);
        }
        else
        {
            FT02_DrawTextPack(display, ft02_cjk_20r, "[中] 拼音输入：连续拼音 / 词组 / 词频学习", 48, 312);
            FT02_DrawTextPack(display, ft02_cjk_20r, "Sym标点自动中文化；长按SYM切英文", 48, 338);
        }
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "[EN] 英文输入：字母 / 数字 / 符号直接上屏", 48, 312);
        FT02_DrawTextPack(display, ft02_cjk_20r, "长按SYM切中文；标点保持英文", 48, 338);
    }

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    const bool gpsAttach = gnss.fixValid && gnss.hasPosition && gnss.lastFixAgeMs <= 15000u;
    char foot[220];
    snprintf(foot, sizeof(foot), "模式：%s   字节：%u/%u   GPS：%s   DEL删除   ESC取消",
             g_composePrivate ? deliveryModeDetail(g_composeDeliveryMode) : "即时·广播",
             static_cast<unsigned>(g_composeLength),
             static_cast<unsigned>(FT02_LORA_USER_TEXT_MAX_BYTES),
             gpsAttach ? "已定位" : "无定位");
    FT02_DrawTextPack(display, ft02_cjk_20r, foot, 32, 382);
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 32, 414);
    FT02_DrawBottomBarWithFont(
        display,
        FT02_TextInputModeIsChinese() ? BOTTOM_COMPOSE_CN : BOTTOM_COMPOSE_EN,
        ft02_cjk_20r
    );
}

void drawDiag(FT02Display& display)
{
    drawHeader(display, "通讯诊断");
    char line[220];
    snprintf(line, sizeof(line), "Mesh：%s   PKI：%s   节点：%u   公钥：%u",
             FT02_MeshServiceReady() ? "在线" : "离线",
             FT02_MeshServicePkiReady() ? "就绪" : "异常",
             static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
             static_cast<unsigned>(FT02_MeshServicePkiPeerCount()));
    FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 158);

    snprintf(line, sizeof(line), "Mesh Raw：%lu   已解码：%lu   本机：!%08lX",
             static_cast<unsigned long>(FT02_MeshServiceRawFrameCount()),
             static_cast<unsigned long>(FT02_MeshServiceDecodedFrameCount()),
             static_cast<unsigned long>(FT02_LoRaNodeRuntimeLocalNode()));
    FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 196);

    snprintf(line, sizeof(line), "消息 RX：%lu   TX：%lu   重复丢弃：%lu   未读：%u",
             static_cast<unsigned long>(FT02_LoRaCommunicationRxTextCount()),
             static_cast<unsigned long>(FT02_LoRaCommunicationTxCount()),
             static_cast<unsigned long>(FT02_LoRaCommunicationDuplicateCount()),
             static_cast<unsigned>(FT02_LoRaCommunicationUnreadCount()));
    FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 234);

    snprintf(line, sizeof(line), "ACK：%lu   NAK：%lu   最后RX ID：0x%08lX",
             static_cast<unsigned long>(FT02_LoRaCommunicationAckCount()),
             static_cast<unsigned long>(FT02_LoRaCommunicationNakCount()),
             static_cast<unsigned long>(FT02_LoRaCommunicationLastRxPacketId()));
    FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 272);

    FT02LoRaTxStatusView tx;
    if(FT02_LoRaCommunicationGetLastTx(tx))
    {
        const char* txState = (tx.broadcast && tx.state == FT02_LORA_TX_SENT)
            ? "已提交"
            : FT02_LoRaCommunicationTxStateText(tx.state);
        snprintf(line, sizeof(line), "最后TX：0x%08lX  %s  %s",
                 static_cast<unsigned long>(tx.packetId),
                 tx.broadcast ? "广播" : "私信",
                 txState);
        FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 310);
        if(tx.routingError != 0)
        {
            snprintf(line, sizeof(line), "Routing：%lu / %s",
                     static_cast<unsigned long>(tx.routingError),
                     FT02_LoRaCommunicationRoutingErrorText(tx.routingError));
            FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 348);
        }
        drawUtf8Lines(display, tx.preview, 32, 386, 64, 26, 1);
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "最后TX：--", 32, 310);
    }
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 470, 414);
    FT02_DrawBottomBarWithFont(display, BOTTOM_DIAG, ft02_cjk_20r);
}

bool handleComposeRaw(const FT02InputEvent& event)
{
    const uint8_t raw = static_cast<uint8_t>(event.raw);

    // CardKB2 IME Direct Toggle A1: long-press SYM emits semantic 0xF1.
    // Backtick is therefore a normal printable character again.
    if(event.key == FT02_KEY_IME_TOGGLE)
    {
        (void)toggleComposeInputMode();
        return true;
    }

    // CardKB2 I2C does not forward Fn+D/Z/X/C to the host. The factory
    // firmware does forward Sym-layer '<' and '>' as ASCII, so use those for
    // delivery-mode cycling. While Pinyin is active the same pair pages the
    // candidate list instead of changing delivery policy.
    if(g_pinyinLength == 0 && (raw == CARDKB_MODE_LEFT || raw == CARDKB_MODE_RIGHT))
    {
        cycleComposeDeliveryMode(raw == CARDKB_MODE_LEFT ? -1 : +1);

        return true;
    }

    // Candidate paging remains available through the same Sym-layer pair,
    // with 6/7 retained as the proven deterministic fallback during Pinyin.
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && (raw == CARDKB_MODE_LEFT || raw == '6'))
    {
        movePinyinPage(-1);
        return true;
    }
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && (raw == CARDKB_MODE_RIGHT || raw == '7'))
    {
        movePinyinPage(+1);
        return true;
    }

    if(raw == '\r' || raw == '\n')
    {
        // ENTER commits active Pinyin first. A second ENTER sends, which
        // prevents accidental transmission while the user is choosing a word.
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
        {
            commitPinyinDefault();
            return false;
        }

        if(g_composeLength == 0)
        {
            setNotice("消息不能为空");
            return true;
        }
        bool ok = false;
        if(g_composePrivate)
            ok = FT02_LoRaMessageQueuePrivate(g_composeTarget, g_composeText, true, g_composeDeliveryMode);
        else
            ok = FT02_LoRaMessageQueueBroadcast(g_composeText, true);

        if(ok)
        {
            // Queue acceptance is the durability boundary for the outgoing
            // message. Delivery may happen immediately or later according to
            // the selected policy.
            (void)FT02_PinyinLearningFlush();
            if(!g_composePrivate) setNotice("即时广播已加入发送队列");
            else if(g_composeDeliveryMode == FT02_MESSAGE_IMMEDIATE) setNotice("即时消息已加入发送队列");
            else if(g_composeDeliveryMode == FT02_MESSAGE_RELIABLE) setNotice("可靠消息已保存，等待目标 ACK");
            else setNotice("持久通知已保存，等待送达或撤销");
            g_mode = COMM_INBOX;
            if(!g_composeReply) g_messageIndex = 0;
            g_composeReply = false;
            g_replyPacketId = 0;
            resetPinyin();
        }
        else
        {
            setNotice(g_composePrivate ? "无法加入发件队列：目标或队列不可用" : "无法加入广播队列");
        }
        return true;
    }

    if(raw == 0x1Bu)
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
        {
            resetPinyin();
            setNotice("已取消当前拼音");
            markComposeEdited();
            return false;
        }
        g_mode = g_composeReturnMode;
        g_composeReply = false;
        g_replyPacketId = 0;
        resetPinyin();
        setNotice("已取消编写");
        return true;
    }

    if(raw == 0x08u || raw == 0x7Fu)
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0) pinyinBackspace();
        else
        {
            utf8Backspace();
            markComposeEdited();
        }
        return false;
    }

    if(raw == ' ')
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
        {
            if(!commitPinyinDefault()) setNotice("无法上屏当前拼音");
        }
        else
        {
            appendAsciiToCompose(' ');
            markComposeEdited();
        }
        return false;
    }

    // Chinese mode keeps the candidate workflow. 1..5 select visible
    // candidates, 6/7 are paging shortcuts above, and 0 commits active Pinyin
    // as literal English/ASCII without changing the persistent input mode.
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && raw >= '1' && raw <= '5')
    {
        if(!commitPinyinCandidate(static_cast<size_t>(raw - '1')))
            setNotice("该候选不存在");
        return false;
    }
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && raw == '0')
    {
        commitRawPinyin();
        return false;
    }

    if((raw >= 'a' && raw <= 'z') || (raw >= 'A' && raw <= 'Z'))
    {
        if(FT02_TextInputModeIsChinese())
            appendPinyinLetter(static_cast<char>(raw));
        else
        {
            appendAsciiToCompose(static_cast<char>(raw));
            markComposeEdited();
        }
        return false;
    }

    // In Chinese mode, committing punctuation first commits any active Pinyin,
    // then maps the six most useful field-message marks to full-width Chinese
    // forms: ，。？！：；. English mode preserves the exact ASCII symbol.
    if(raw >= 0x20u && raw < 0x7Fu)
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && !commitPinyinDefault())
            return false;
        if(!appendComposePunctuationOrAscii(raw))
            return false;
        markComposeEdited();
        return false;
    }

    // Ignore non-character Fn codes that are not handled above; never append
    // them as malformed UTF-8 bytes to the message body.
    return false;
}
}

void FT02_CommunicationUIOpen()
{
    g_mode = COMM_INBOX;
    g_historyFilter = HISTORY_ALL;
    g_filterSelection = static_cast<uint8_t>(g_historyFilter);
    g_nodeAction = NODE_ACTION_BROWSE;
    g_sendSelection = 0u;
    g_messageIndex = 0;
    g_outboxIndex = 0;
    g_voiceStartPending = false;
    g_voiceCaptureCueReady = false;
    g_voicePendingDestination = FT02_MESH_BROADCAST_NODE;
    g_voiceTxObserved = false;
    setNotice("");
    FT02_LoRaCommunicationMarkAllRead();
}

void FT02_DrawCommunicationNodeScreen(FT02Display& display)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        if(g_mode == COMM_INBOX) drawInbox(display);
        else if(g_mode == COMM_FILTER) drawFilter(display);
        else if(g_mode == COMM_SEND_MENU) drawSendMenu(display);
        else if(g_mode == COMM_NODES) drawNodes(display);
        else if(g_mode == COMM_COMPOSE) drawCompose(display);
        else if(g_mode == COMM_OUTBOX) drawOutbox(display);
        else if(g_mode == COMM_OUTBOX_CANCEL_CONFIRM) drawOutboxCancelConfirm(display);
        else if(g_mode == COMM_OUTBOX_CLEAR_CONFIRM) drawOutboxClearConfirm(display);
        else if(g_mode == COMM_VOICE) drawVoice(display);
        else drawDiag(display);
    }
    while(display.nextPage());
    display.setFullWindow();
    FT02_EpdInteractivePowerOffNow(display);
    FT02_EpdInteractiveEndBusyCapture(display);
    g_composeDirty = false;
}

static void FT02_CommunicationCommitPartial(
    FT02Display& display,
    int x,
    int y,
    int w,
    int h
)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        if(g_mode == COMM_INBOX) drawInbox(display);
        else if(g_mode == COMM_FILTER) drawFilter(display);
        else if(g_mode == COMM_SEND_MENU) drawSendMenu(display);
        else if(g_mode == COMM_NODES) drawNodes(display);
        else if(g_mode == COMM_COMPOSE) drawCompose(display);
        else if(g_mode == COMM_OUTBOX) drawOutbox(display);
        else if(g_mode == COMM_OUTBOX_CANCEL_CONFIRM) drawOutboxCancelConfirm(display);
        else if(g_mode == COMM_OUTBOX_CLEAR_CONFIRM) drawOutboxClearConfirm(display);
        else if(g_mode == COMM_VOICE) drawVoice(display);
        else drawDiag(display);
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display);
}

void FT02_DrawCommunicationBrowsePartial(FT02Display& display)
{
    // Inbox/outbox/node browsing only changes the content below the page header.
    FT02_CommunicationCommitPartial(display, 16, 174, 768, 266);
}

void FT02_DrawCommunicationComposePartial(FT02Display& display)
{
    // Text editor, Pinyin candidates, mode and notice. Keep status/header/footer.
    FT02_CommunicationCommitPartial(display, 24, 140, 752, 300);
    g_composeDirty = false;
}

void FT02_DrawCommunicationVoiceTimerPartial(FT02Display& display)
{
    if(g_mode != COMM_VOICE) return;
    // Keep e-paper activity small while the microphone/I2S path is active.
    FT02_CommunicationCommitPartial(display, 56, 204, 688, 164);
}

void FT02_DrawCommunicationVoiceStartCuePartial(FT02Display& display)
{
    if(g_mode != COMM_VOICE) return;
    // Capture is already running. Refresh only the state/message strip so the
    // user gets an unambiguous go cue without a full EPD transaction.
    FT02_CommunicationCommitPartial(display, 56, 204, 688, 208);
}

void FT02_DrawCommunicationVoiceTxPartial(FT02Display& display)
{
    if(g_mode != COMM_VOICE) return;
    // Only refresh state/progress while RF chunks advance. Full-screen refreshes
    // would needlessly stall the mesh between every packet.
    FT02_CommunicationCommitPartial(display, 56, 204, 688, 208);
}

FT02CommunicationInputResult FT02_CommunicationUIHandleInput(const FT02InputEvent& event)
{
    if(g_mode == COMM_COMPOSE)
    {
        return handleComposeRaw(event) ? FT02_COMM_INPUT_REDRAW : FT02_COMM_INPUT_NONE;
    }

    // CardKB2 DEL emits 0x08, which InputManager normally maps to BACK. In the
    // outbox it is intentionally reserved for canceling an active delivery.
    const uint8_t raw = static_cast<uint8_t>(event.raw);
    if(g_mode == COMM_OUTBOX && (raw == 0x08u || raw == 0x7Fu))
    {
        const size_t count = FT02_LoRaMessageOutboxCount();
        if(count == 0) return FT02_COMM_INPUT_NONE;
        if(g_outboxIndex >= count) g_outboxIndex = count - 1;
        FT02LoRaOutboxView item;
        if(!FT02_LoRaMessageGetOutboxNewest(g_outboxIndex, item)) return FT02_COMM_INPUT_NONE;
        const bool active = item.state == FT02_DELIVERY_QUEUED || item.state == FT02_DELIVERY_WAITING_ACK;
        if(active)
        {
            g_pendingCancelLogicalId = item.logicalId;
            g_mode = COMM_OUTBOX_CANCEL_CONFIRM;
            setNotice("");
        }
        else if(item.state == FT02_DELIVERY_DELIVERED)
        {
            setNotice("消息已经投递，无法撤销");
        }
        else
        {
            setNotice("该消息已结束，无法撤销");
        }
        return FT02_COMM_INPUT_REDRAW;
    }

    if(g_mode == COMM_OUTBOX_CLEAR_CONFIRM)
    {
        if(event.key == FT02_KEY_SELECT)
        {
            const bool ok = FT02_LoRaMessageClearOutbox();
            g_mode = COMM_OUTBOX;
            g_outboxIndex = 0;
            setNotice(ok ? "发件箱已清空" : "清空发件箱失败");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(event.key == FT02_KEY_BACK || event.command == 'b')
        {
            g_mode = COMM_OUTBOX;
            setNotice("已取消清空");
            return FT02_COMM_INPUT_REDRAW;
        }
        return FT02_COMM_INPUT_NONE;
    }

    if(event.key == FT02_KEY_HELP)
    {
        if(FT02_MeshVoiceAudioBusy())
        {
            setNotice("请先结束当前语音录制或播放");
            return FT02_COMM_INPUT_REDRAW;
        }
        return FT02_COMM_INPUT_OPEN_HELP;
    }
    if(event.key == FT02_KEY_BACK)
    {
        if(FT02_MeshVoiceAudioIsPlaying())
        {
            (void)FT02_MeshVoiceAudioStopPlayback();
            setNotice("正在停止语音播放");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(g_mode == COMM_VOICE)
        {
            if(g_voiceStartPending)
            {
                g_voiceStartPending = false;
                g_voiceCaptureCueReady = false;
                g_mode = g_voiceReturnMode;
                setNotice("已取消语音录制");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(FT02_MeshVoiceAudioIsRecording())
            {
                // Never energize the e-paper while capture/post-roll is active.
                // Audio completion will publish a revision after the owner is released.
                (void)FT02_MeshVoiceAudioCancelRecording();
                return FT02_COMM_INPUT_NONE;
            }
            if(FT02_MeshVoiceAudioBusy())
            {
                // Saving/encoding owns the audio/SD pipeline. Do not let the
                // page disappear underneath it; the status revision will
                // redraw automatically when the operation finishes.
                return FT02_COMM_INPUT_NONE;
            }
            if(FT02_MeshVoiceTransportBusy())
            {
                FT02_MeshVoiceTransportCancel();
                g_mode = g_voiceReturnMode;
                g_voiceTxObserved = false;
                g_voiceCaptureCueReady = false;
                setNotice("已取消语音发送");
                return FT02_COMM_INPUT_REDRAW;
            }
            g_mode = g_voiceReturnMode;
            setNotice(FT02_MeshVoiceAudioStatusCurrent().message);
            return FT02_COMM_INPUT_REDRAW;
        }
        if(g_mode == COMM_FILTER)
        {
            g_mode = COMM_INBOX;
            setNotice("");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(g_mode == COMM_SEND_MENU)
        {
            g_mode = COMM_INBOX;
            setNotice("");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(g_mode == COMM_NODES)
        {
            if(g_nodeAction == NODE_ACTION_BROWSE)
            {
                g_mode = g_nodesReturnMode;
            }
            else
            {
                g_mode = COMM_SEND_MENU;
                g_nodeAction = NODE_ACTION_BROWSE;
            }
            setNotice("");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(g_mode == COMM_INBOX) return FT02_COMM_INPUT_EXIT_HOME;
        g_mode = COMM_INBOX;
        g_messageIndex = 0;
        FT02_LoRaCommunicationMarkAllRead();
        setNotice("");
        return FT02_COMM_INPUT_REDRAW;
    }

    const char cmd = event.command;
    if(g_mode == COMM_VOICE)
    {
        if(g_voiceStartPending) return FT02_COMM_INPUT_NONE;
        if((event.key == FT02_KEY_SELECT || cmd == 'v') && FT02_MeshVoiceAudioIsRecording())
        {
            // Match the proven AudioLog UX: request stop, but do not refresh
            // e-paper until post-roll/save has released the audio owner.
            (void)FT02_MeshVoiceAudioStopRecording();
            return FT02_COMM_INPUT_NONE;
        }
        // While a capture is being saved/encoded, keep the UI locked to this
        // page so navigation cannot orphan the audio owner or temp file.
        return FT02_COMM_INPUT_NONE;
    }

    const bool filterShortcut = cmd == 'f' || raw == static_cast<uint8_t>('f') ||
        raw == static_cast<uint8_t>('F') || raw == static_cast<uint8_t>('[');
    if(g_mode == COMM_INBOX && filterShortcut)
    {
        g_filterSelection = static_cast<uint8_t>(g_historyFilter);
        g_mode = COMM_FILTER;
        setNotice("");
        FT02_LOGI("[COMM-FILTER] open current=%u raw=0x%02X cmd=%c\n",
                  static_cast<unsigned>(g_filterSelection),
                  static_cast<unsigned>(raw), cmd ? cmd : '-');
        return FT02_COMM_INPUT_REDRAW;
    }

    if(g_mode == COMM_FILTER)
    {
        if(event.key == FT02_KEY_UP || event.key == FT02_KEY_LEFT)
        {
            g_filterSelection = g_filterSelection == 0u ? 2u : static_cast<uint8_t>(g_filterSelection - 1u);
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(event.key == FT02_KEY_DOWN || event.key == FT02_KEY_RIGHT)
        {
            g_filterSelection = static_cast<uint8_t>((g_filterSelection + 1u) % 3u);
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(event.key == FT02_KEY_SELECT)
        {
            g_historyFilter = static_cast<HistoryFilter>(g_filterSelection);
            g_messageIndex = 0u;
            g_mode = COMM_INBOX;
            setNotice("");
            FT02_LOGI("[COMM-FILTER] applied=%s\n", historyFilterText());
            return FT02_COMM_INPUT_REDRAW;
        }
        return FT02_COMM_INPUT_NONE;
    }

    if(g_mode == COMM_SEND_MENU)
    {
        if(event.key == FT02_KEY_UP || event.key == FT02_KEY_LEFT)
        {
            g_sendSelection = g_sendSelection == 0u ? 3u : static_cast<uint8_t>(g_sendSelection - 1u);
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(event.key == FT02_KEY_DOWN || event.key == FT02_KEY_RIGHT)
        {
            g_sendSelection = static_cast<uint8_t>((g_sendSelection + 1u) % 4u);
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(event.key == FT02_KEY_SELECT)
        {
            switch(g_sendSelection)
            {
                case 0u:
                    beginCompose(false, 0u, COMM_INBOX);
                    return FT02_COMM_INPUT_REDRAW;
                case 1u:
                    return beginVoice(FT02_MESH_BROADCAST_NODE, COMM_INBOX);
                case 2u:
                    g_nodeAction = NODE_ACTION_PRIVATE_TEXT;
                    g_nodesReturnMode = COMM_SEND_MENU;
                    g_mode = COMM_NODES;
                    g_nodeSelection = 0u;
                    setNotice(FT02_MeshServiceRequestNodeInfo() ? "请选择私聊文字收件人" : "节点刷新失败，使用现有列表");
                    return FT02_COMM_INPUT_REDRAW;
                case 3u:
                    g_nodeAction = NODE_ACTION_PRIVATE_VOICE;
                    g_nodesReturnMode = COMM_SEND_MENU;
                    g_mode = COMM_NODES;
                    g_nodeSelection = 0u;
                    setNotice(FT02_MeshServiceRequestNodeInfo() ? "请选择私聊语音收件人" : "节点刷新失败，使用现有列表");
                    return FT02_COMM_INPUT_REDRAW;
                default:
                    break;
            }
        }
        return FT02_COMM_INPUT_NONE;
    }

    if(g_mode == COMM_INBOX && cmd == 't')
    {
        g_mode = COMM_SEND_MENU;
        g_sendSelection = 0u;
        setNotice("");
        return FT02_COMM_INPUT_REDRAW;
    }
    if(g_mode == COMM_INBOX && cmd == 's')
    {
        g_mode = COMM_DIAG;
        setNotice("");
        return FT02_COMM_INPUT_REDRAW;
    }
    if(cmd == 'i')
    {
        g_mode = COMM_INBOX;
        g_messageIndex = 0;
        FT02_LoRaCommunicationMarkAllRead();
        return FT02_COMM_INPUT_REDRAW;
    }

    if(g_mode == COMM_INBOX)
    {
        const size_t count = historyFilteredCount();
        if(count > 0 && event.key == FT02_KEY_SELECT)
        {
            if(g_messageIndex >= count) g_messageIndex = count - 1u;
            FT02LoRaMessageView msg{};
            if(!historyGetFiltered(g_messageIndex, msg))
            {
                setNotice("无法读取当前消息");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(msg.messageType == FT02_MESH_MESSAGE_VOICE)
            {
                if(msg.historyStatus == FT02_MESH_STATUS_INCOMPLETE || msg.voicePath[0] == '\0')
                {
                    setNotice("这条语音不完整，无法播放");
                    return FT02_COMM_INPUT_REDRAW;
                }
                if(FT02_MeshVoiceAudioIsPlaying())
                {
                    (void)FT02_MeshVoiceAudioStopPlayback();
                    setNotice("正在停止语音播放");
                    return FT02_COMM_INPUT_REDRAW;
                }
                if(FT02_MeshVoiceAudioStartPlayback(msg.voicePath))
                    setNotice("语音播放中 · ENTER停止");
                else
                    setNotice(FT02_MeshVoiceAudioStatusCurrent().message);
                return FT02_COMM_INPUT_REDRAW;
            }
            const uint32_t replyNode = msg.outgoing ? msg.to : msg.from;
            if(msg.broadcast && msg.outgoing)
            {
                setNotice("自己发送的广播无需回复");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(replyNode == 0 || replyNode == FT02_LoRaNodeRuntimeLocalNode() || replyNode == 0xFFFFFFFFu)
            {
                setNotice("当前消息没有可回复的目标");
                return FT02_COMM_INPUT_REDRAW;
            }
            beginCompose(true, replyNode, COMM_INBOX, true, msg.packetId);
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && (cmd == 'p' || event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP))
        {
            g_messageIndex = (g_messageIndex + 1u) % count;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(count > 0 && (cmd == 'n' || event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN))
        {
            g_messageIndex = g_messageIndex == 0u ? count - 1u : g_messageIndex - 1u;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
    }
    else if(g_mode == COMM_OUTBOX)
    {
        const size_t count = FT02_LoRaMessageOutboxCount();
        if(cmd == 'k')
        {
            if(count == 0)
            {
                setNotice("发件箱已经为空");
                return FT02_COMM_INPUT_REDRAW;
            }
            g_mode = COMM_OUTBOX_CLEAR_CONFIRM;
            setNotice("");
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && (event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP))
        {
            g_outboxIndex = (g_outboxIndex + 1u) % count;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(count > 0 && (event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN))
        {
            g_outboxIndex = g_outboxIndex == 0 ? count - 1u : g_outboxIndex - 1u;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
    }
    else if(g_mode == COMM_NODES)
    {
        const size_t count = FT02_LoRaNodeRuntimeNodeCount();
        if(count > 0 && event.key == FT02_KEY_UP)
        {
            g_nodeSelection = g_nodeSelection == 0 ? count - 1 : g_nodeSelection - 1;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(count > 0 && event.key == FT02_KEY_DOWN)
        {
            g_nodeSelection = (g_nodeSelection + 1) % count;
            return FT02_COMM_INPUT_BROWSE_PARTIAL;
        }
        if(count > 0 && event.key == FT02_KEY_SELECT)
        {
            FT02LoRaNodeView node;
            if(!FT02_LoRaNodeRuntimeGetNode(g_nodeSelection, node)) return FT02_COMM_INPUT_NONE;
            if(node.node == FT02_LoRaNodeRuntimeLocalNode())
            {
                setNotice("本机节点不能作为私信目标");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(!node.publicKeyValid || !FT02_MeshServiceCanSendPrivate(node.node))
            {
                setNotice("该节点尚未学习PKI公钥，暂不能私聊");
                return FT02_COMM_INPUT_REDRAW;
            }

            if(g_nodeAction == NODE_ACTION_PRIVATE_VOICE)
            {
                g_nodeAction = NODE_ACTION_BROWSE;
                return beginVoice(node.node, COMM_INBOX);
            }

            const CommMode returnMode = g_nodeAction == NODE_ACTION_PRIVATE_TEXT
                ? COMM_SEND_MENU : COMM_NODES;
            g_nodeAction = NODE_ACTION_BROWSE;
            beginCompose(true, node.node, returnMode);
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && cmd == 'v' && g_nodeAction == NODE_ACTION_BROWSE)
        {
            FT02LoRaNodeView node{};
            if(!FT02_LoRaNodeRuntimeGetNode(g_nodeSelection, node)) return FT02_COMM_INPUT_NONE;
            if(node.node == FT02_LoRaNodeRuntimeLocalNode())
            {
                setNotice("本机节点不能作为私聊语音目标");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(!node.publicKeyValid || !FT02_MeshServiceCanSendPrivate(node.node))
            {
                setNotice("该节点尚未学习PKI公钥，暂不能私聊语音");
                return FT02_COMM_INPUT_REDRAW;
            }
            return beginVoice(node.node, COMM_INBOX);
        }
        if(cmd == 'r')
        {
            setSyncNotice("正在刷新Mesh节点身份...");
            if(!FT02_MeshServiceRequestNodeInfo())
                setNotice("Mesh刷新失败，网络未就绪");
            return FT02_COMM_INPUT_REDRAW;
        }
    }
    else if(g_mode == COMM_DIAG && cmd == 'm')
    {
        g_mode = COMM_NODES;
        g_nodeAction = NODE_ACTION_BROWSE;
        g_nodesReturnMode = COMM_DIAG;
        g_nodeSelection = 0u;
        setSyncNotice("正在读取Mesh节点列表...");
        if(!FT02_MeshServiceRequestNodeInfo())
            setNotice("节点刷新失败，显示现有列表");
        return FT02_COMM_INPUT_REDRAW;
    }
    else if(g_mode == COMM_DIAG && cmd == 'r')
    {
        setSyncNotice("正在请求Mesh NodeInfo刷新...");
        if(!FT02_MeshServiceRequestNodeInfo())
            setNotice("Mesh刷新失败，网络未就绪");
        return FT02_COMM_INPUT_REDRAW;
    }

    return FT02_COMM_INPUT_NONE;
}

bool FT02_CommunicationUITakeDeferredRedraw(uint32_t nowMs)
{
    if(g_mode != COMM_COMPOSE || !g_composeDirty) return false;
    if(nowMs - g_composeLastEditMs < COMPOSE_REDRAW_IDLE_MS) return false;
    g_composeDirty = false;
    return true;
}

bool FT02_CommunicationUIIsInbox()
{
    return g_mode == COMM_INBOX;
}

bool FT02_CommunicationUIIsCompose()
{
    return g_mode == COMM_COMPOSE;
}

bool FT02_CommunicationUIIsVoice()
{
    return g_mode == COMM_VOICE;
}

bool FT02_CommunicationUIIsNodes()
{
    return g_mode == COMM_NODES;
}

bool FT02_CommunicationUIVoiceStartPending()
{
    return g_mode == COMM_VOICE && g_voiceStartPending;
}

bool FT02_CommunicationUIStartPendingVoiceCapture()
{
    if(g_mode != COMM_VOICE || !g_voiceStartPending) return false;
    const uint32_t destination = g_voicePendingDestination;
    g_voiceStartPending = false;
    g_voiceCaptureCueReady = false;
    const bool started = FT02_MeshVoiceAudioStartRecording(destination);
    if(!started) setNotice(FT02_MeshVoiceAudioStatusCurrent().message);
    return started;
}

void FT02_CommunicationUIVoiceCaptureCueReady()
{
    if(g_mode != COMM_VOICE || !FT02_MeshVoiceAudioIsRecording()) return;
    g_voiceCaptureCueReady = true;
}

bool FT02_CommunicationUIServiceVoiceTransport()
{
    if(g_mode != COMM_VOICE || g_voiceStartPending || FT02_MeshVoiceAudioBusy()) return false;

    const bool busy = FT02_MeshVoiceTransportBusy();
    if(busy)
    {
        g_voiceTxObserved = true;
        // Direct voice may remain in the transport ACK hold after every RF frame
        // has already left the radio. Do not strand the user on the voice page
        // for that hold window; delivery status remains visible in message history.
        if(g_voicePendingDestination != FT02_MESH_BROADCAST_NODE &&
           FT02_MeshVoiceTransportAwaitingAck())
        {
            g_mode = g_voiceReturnMode;
            g_messageIndex = 0u;
            setNotice("私聊语音已发出，等待对端确认");
            g_voiceTxObserved = false;
            g_voiceCaptureCueReady = false;
            return true;
        }
        return false;
    }

    // Broadcast transport clears its TX session only after BEGIN/CHUNK/END have
    // all returned success from the blocking RadioLib transmit path. Observe the
    // busy phase first so an idle voice page cannot be mistaken for completion.
    if(g_voiceTxObserved)
    {
        const bool broadcast = g_voicePendingDestination == FT02_MESH_BROADCAST_NODE;
        const FT02MeshVoiceTxOutcome outcome = FT02_MeshVoiceTransportLastOutcome();
        g_mode = g_voiceReturnMode;
        g_messageIndex = 0u;
        if(outcome == FT02_MESH_VOICE_TX_FAILED)
            setNotice(broadcast ? "广播语音发送失败，已停止重试" : "私聊语音发送失败，已停止重试");
        else if(outcome == FT02_MESH_VOICE_TX_CANCELED)
            setNotice("语音发送已取消");
        else
            setNotice(broadcast ? "广播语音已发送" : "私聊语音发送完成");
        g_voiceTxObserved = false;
        g_voiceCaptureCueReady = false;
        return true;
    }
    return false;
}

void FT02_CommunicationUIOnNodeListUpdated(uint16_t count)
{
    if(g_mode != COMM_NODES) return;
    if(count == 0) setNotice("当前尚未学习到其他Mesh节点");
    else if(g_nodeAction == NODE_ACTION_PRIVATE_TEXT) setNotice("请选择私聊文字收件人");
    else if(g_nodeAction == NODE_ACTION_PRIVATE_VOICE) setNotice("请选择私聊语音收件人");
    else
    {
        char notice[64];
        snprintf(notice, sizeof(notice), "Mesh节点列表已更新：%u", static_cast<unsigned>(count));
        setNotice(notice);
    }
}

void FT02_CommunicationUIOnSyncStarted(const char* noticeText)
{
    setSyncNotice(
        (noticeText != nullptr && noticeText[0] != '\0')
            ? noticeText
            : "Mesh正在同步状态..."
    );
}

void FT02_CommunicationUIOnSyncReady()
{
    if(g_syncNoticeActive)
    {
        g_notice[0] = '\0';
        g_syncNoticeActive = false;
    }
}

bool FT02_CommunicationUIServiceSyncNotice(uint32_t nowMs)
{
    if(!g_syncNoticeActive) return false;

    // Manual R refresh keeps MeshService READY the whole time, so the legacy
    // false->true readiness transition cannot close this notice. A real
    // NODEINFO_APP response is the authoritative success signal.
    if(FT02_MeshServiceNodeInfoRxCount() != g_syncNoticeNodeInfoRxBaseline)
    {
        FT02_LOGI_PRINTLN("[COMM-SYNC] NodeInfo refresh response received");
        FT02_CommunicationUIOnSyncReady();
        return true;
    }

    if(g_syncNoticeStartedMs != 0u &&
       static_cast<uint32_t>(nowMs - g_syncNoticeStartedMs) >= MESH_SYNC_NOTICE_TIMEOUT_MS)
    {
        if(FT02_MeshServiceReady())
            setNotice("Mesh刷新完成，暂无新身份响应");
        else
            setNotice("Mesh同步超时，请稍后重试");
        FT02_LOGW_PRINTLN("[COMM-SYNC] refresh timeout closeout");
        return true;
    }

    return false;
}
