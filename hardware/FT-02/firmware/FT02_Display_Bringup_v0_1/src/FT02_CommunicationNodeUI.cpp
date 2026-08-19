#include "FT02_CommunicationNodeUI.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_Gnss.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_LR01HostRuntime.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_PinyinIme.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_TextInputMode.h"

namespace
{
enum CommMode : uint8_t
{
    COMM_INBOX = 0,
    COMM_NODES,
    COMM_COMPOSE,
    COMM_OUTBOX,
    COMM_OUTBOX_CANCEL_CONFIRM,
    COMM_OUTBOX_CLEAR_CONFIRM,
    COMM_DIAG
};

constexpr size_t VISIBLE_NODE_ROWS = 4;
constexpr uint32_t COMPOSE_REDRAW_IDLE_MS = 650u;
constexpr size_t PINYIN_VISIBLE_CANDIDATES = FT02_PINYIN_PAGE_SIZE;

CommMode g_mode = COMM_INBOX;

// Unit CardKB2 (U215) I2C factory firmware consumes Fn+D/Z/X/C internally
// and does not emit those arrow combinations to the I2C host. Use translated
// Sym-layer angle brackets instead; they arrive as ordinary ASCII bytes and
// provide an unambiguous left/right compose shortcut.
static constexpr uint8_t CARDKB_MODE_LEFT  = static_cast<uint8_t>('<');
static constexpr uint8_t CARDKB_MODE_RIGHT = static_cast<uint8_t>('>');
CommMode g_composeReturnMode = COMM_INBOX;
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

static const FT02BottomBarItem BOTTOM_INBOX[3] = {
    {nullptr, "确认回复 / T广播"}, {nullptr, "M节点 / O发件"}, {nullptr, "S诊断 / B返回"}
};
static const FT02BottomBarItem BOTTOM_NODES[3] = {
    {nullptr, "方向键选择"}, {nullptr, "确认 私信"}, {nullptr, "T广播 / B收件箱"}
};
static const FT02BottomBarItem BOTTOM_COMPOSE_CN[3] = {
    {nullptr, "空格首选 / 1-5选词"}, {nullptr, "Sym+V/B 模式·候选"}, {nullptr, "` 中英 / ENTER发送"}
};
static const FT02BottomBarItem BOTTOM_COMPOSE_EN[3] = {
    {nullptr, "英文直接输入"}, {nullptr, "Sym+V/B模式 / ` 中英"}, {nullptr, "DEL删除 / ENTER发送"}
};
static const FT02BottomBarItem BOTTOM_OUTBOX[3] = {
    {nullptr, "方向键 浏览"}, {nullptr, "DEL 撤销 / K清空"}, {nullptr, "B 收件箱"}
};
static const FT02BottomBarItem BOTTOM_DIAG[3] = {
    {nullptr, "R 重同步"}, {nullptr, "M 节点"}, {nullptr, "B 收件箱"}
};

void setNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
    g_syncNoticeActive = false;
}

void setSyncNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
    g_syncNoticeActive = g_notice[0] != '\0';
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
    // Sym+W is a backtick (`) in CardKB2 I2C symbol mode. It is reserved as
    // the compose-mode switch so normal digits remain available for messages.
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

void drawInbox(FT02Display& display)
{
    drawHeader(display, "收件箱");
    const size_t count = FT02_LoRaCommunicationMessageCount();
    char summary[160];
    snprintf(
        summary, sizeof(summary),
        "消息：%u   未读：%u   节点：%u/%lu   %s",
        static_cast<unsigned>(count),
        static_cast<unsigned>(FT02_LoRaCommunicationUnreadCount()),
        static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
        static_cast<unsigned long>(FT02_LoRaNodeRuntimeExpectedNodeCount()),
        FT02_LoRaNodeRuntimeReady() ? "在线" : "同步中"
    );
    FT02_DrawTextPack(display, ft02_cjk_20r, summary, 32, 156);

    if(g_notice[0])
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 430, 156);
    }

    if(count == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "暂无收到的消息", 278, 280);
        FT02_DrawTextPack(display, ft02_cjk_20r, "按 T 编写广播，按 M 查看网络终端", 226, 318);
    }
    else
    {
        if(g_messageIndex >= count) g_messageIndex = count - 1;
        FT02LoRaMessageView msg;
        if(FT02_LoRaCommunicationGetMessageNewest(g_messageIndex, msg))
        {
            char fallback[20];
            const char* sender = nodeName(msg.from, fallback, sizeof(fallback));
            char meta[220];
            char msgTime[24];
            formatMessageTime(msg, msgTime, sizeof(msgTime));
            snprintf(
                meta, sizeof(meta),
                "%s  %s%s  %s   %u/%u",
                sender,
                msg.broadcast ? "广播" : "私信",
                msg.pkiEncrypted ? "·PKI" : "",
                msgTime,
                static_cast<unsigned>(g_messageIndex + 1),
                static_cast<unsigned>(count)
            );
            FT02_DrawTextPack(display, ft02_cjk_24b, meta, 32, 196);

            display.drawRect(32, 210, 736, 142, GxEPD_BLACK);
            drawUtf8Lines(display, msg.text, 48, 240, 54, 27, 4);

            char radio[220];
            char hops[24];
            if(msg.hasHops) snprintf(hops, sizeof(hops), "%u跳", static_cast<unsigned>(msg.hops));
            else snprintf(hops, sizeof(hops), "路由--");
            snprintf(
                radio, sizeof(radio),
                "ID 0x%08lX   %s   RSSI %s%d   SNR %s%.1f dB",
                static_cast<unsigned long>(msg.packetId),
                hops,
                msg.hasRssi ? "" : "--/",
                msg.hasRssi ? static_cast<int>(msg.rssi) : 0,
                msg.hasSnr ? "" : "--/",
                static_cast<double>(msg.hasSnr ? msg.snr : 0.0f)
            );
            FT02_DrawTextPack(display, ft02_cjk_20r, radio, 32, 386);
            FT02_DrawTextPack(display, ft02_cjk_20r, "←/→ 浏览消息   ENTER 直接回复", 32, 420);
        }
    }
    FT02_DrawBottomBarWithFont(display, BOTTOM_INBOX, ft02_cjk_20r);
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
    drawHeader(display, "网络终端");
    const size_t count = FT02_LoRaNodeRuntimeNodeCount();
    const FT02LR01State& lr01 = FT02_LR01HostState();
    char summary[180];
    snprintf(summary, sizeof(summary), "通讯模块：%s   节点缓存：%u/%u   R=刷新节点",
             FT02_LR01HostOnline() ? "在线" : "离线",
             static_cast<unsigned>(count),
             static_cast<unsigned>(lr01.listedNodeCount));
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
    FT02_DrawBottomBarWithFont(display, BOTTOM_NODES, ft02_cjk_20r);
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
            FT02_DrawTextPack(display, ft02_cjk_20r, "Sym标点自动中文化；`切英文", 48, 338);
        }
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "[EN] 英文输入：字母 / 数字 / 符号直接上屏", 48, 312);
        FT02_DrawTextPack(display, ft02_cjk_20r, "`切中文；标点保持英文", 48, 338);
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
    const FT02LR01State& lr01 = FT02_LR01HostState();
    snprintf(line, sizeof(line), "通讯模块：%s   协议：A%u   节点：%u/%u",
             FT02_LR01HostOnline() ? "在线" : "离线",
             static_cast<unsigned>(lr01.protocolVersion),
             static_cast<unsigned>(FT02_LoRaNodeRuntimeNodeCount()),
             static_cast<unsigned>(lr01.listedNodeCount));
    FT02_DrawTextPack(display, ft02_cjk_20r, line, 32, 158);

    snprintf(line, sizeof(line), "Host RX行：%lu   LR01 Radio复位：%lu",
             static_cast<unsigned long>(FT02_LR01HostRxLineCount()),
             static_cast<unsigned long>(lr01.radioResets));
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

    // CardKB2 I2C symbol map: Sym+W emits '`'. Reserve it as a mode switch.
    // This avoids consuming any number key, which remains important for IDs,
    // coordinates and field messages.
    if(raw == static_cast<uint8_t>('`'))
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
        Serial.printf("[COMM] delivery mode key raw='%c' mode=%s\n",
                      static_cast<char>(raw),
                      deliveryModeDetail(g_composeDeliveryMode));
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

    // Chinese mode keeps the A3 candidate workflow. 1..5 select visible
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
    g_messageIndex = 0;
    g_outboxIndex = 0;
    setNotice("");
    FT02_LoRaCommunicationMarkAllRead();
}

void FT02_DrawCommunicationNodeScreen(FT02Display& display)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        if(g_mode == COMM_INBOX) drawInbox(display);
        else if(g_mode == COMM_NODES) drawNodes(display);
        else if(g_mode == COMM_COMPOSE) drawCompose(display);
        else if(g_mode == COMM_OUTBOX) drawOutbox(display);
        else if(g_mode == COMM_OUTBOX_CANCEL_CONFIRM) drawOutboxCancelConfirm(display);
        else if(g_mode == COMM_OUTBOX_CLEAR_CONFIRM) drawOutboxClearConfirm(display);
        else drawDiag(display);
    }
    while(display.nextPage());
    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display, "communication-full");
    g_composeDirty = false;
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

    if(event.key == FT02_KEY_HELP) return FT02_COMM_INPUT_OPEN_HELP;
    if(event.key == FT02_KEY_BACK)
    {
        if(g_mode == COMM_INBOX) return FT02_COMM_INPUT_EXIT_HOME;
        g_mode = COMM_INBOX;
        g_messageIndex = 0;
        FT02_LoRaCommunicationMarkAllRead();
        setNotice("");
        return FT02_COMM_INPUT_REDRAW;
    }

    const char cmd = event.command;
    if(cmd == 't')
    {
        beginCompose(false, 0, g_mode);
        return FT02_COMM_INPUT_REDRAW;
    }
    if(cmd == 'm')
    {
        g_mode = COMM_NODES;
        g_nodeSelection = 0;
        if(FT02_LR01HostRequestNodes()) setNotice("已请求通讯模块节点列表");
        else setNotice("通讯模块节点请求发送失败");
        return FT02_COMM_INPUT_REDRAW;
    }
    if(cmd == 'o')
    {
        g_mode = COMM_OUTBOX;
        g_outboxIndex = 0;
        setNotice("");
        return FT02_COMM_INPUT_REDRAW;
    }
    if(cmd == 's')
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
        const size_t count = FT02_LoRaCommunicationMessageCount();
        if(count > 0 && event.key == FT02_KEY_SELECT)
        {
            if(g_messageIndex >= count) g_messageIndex = count - 1;
            FT02LoRaMessageView msg;
            if(!FT02_LoRaCommunicationGetMessageNewest(g_messageIndex, msg))
            {
                setNotice("无法读取当前消息");
                return FT02_COMM_INPUT_REDRAW;
            }
            if(msg.from == 0 || msg.from == FT02_LoRaNodeRuntimeLocalNode())
            {
                setNotice("当前消息没有可回复的发送者");
                return FT02_COMM_INPUT_REDRAW;
            }
            // A2: Core no longer gates replies on its legacy NodeDB. LR01 owns
            // peer discovery and PKI state and returns node_not_found / pki_not_ready
            // if this target is not currently eligible for a private send.
            beginCompose(true, msg.from, COMM_INBOX, true, msg.packetId);
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && cmd == 'p')
        {
            g_messageIndex = (g_messageIndex + 1) % count;
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && cmd == 'n')
        {
            g_messageIndex = g_messageIndex == 0 ? count - 1 : g_messageIndex - 1;
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && (event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP))
        {
            g_messageIndex = (g_messageIndex + 1) % count;
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && (event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN))
        {
            g_messageIndex = g_messageIndex == 0 ? count - 1 : g_messageIndex - 1;
            return FT02_COMM_INPUT_REDRAW;
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
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && (event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN))
        {
            g_outboxIndex = g_outboxIndex == 0 ? count - 1u : g_outboxIndex - 1u;
            return FT02_COMM_INPUT_REDRAW;
        }
    }
    else if(g_mode == COMM_NODES)
    {
        const size_t count = FT02_LoRaNodeRuntimeNodeCount();
        if(count > 0 && event.key == FT02_KEY_UP)
        {
            g_nodeSelection = g_nodeSelection == 0 ? count - 1 : g_nodeSelection - 1;
            return FT02_COMM_INPUT_REDRAW;
        }
        if(count > 0 && event.key == FT02_KEY_DOWN)
        {
            g_nodeSelection = (g_nodeSelection + 1) % count;
            return FT02_COMM_INPUT_REDRAW;
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
            // PKI ownership moved to LR01. Let LR01 validate whether this peer
            // currently has a learned public key.
            beginCompose(true, node.node, COMM_NODES);
            return FT02_COMM_INPUT_REDRAW;
        }
        if(cmd == 'r')
        {
            setSyncNotice("正在刷新通讯模块节点列表...");
            (void)FT02_LR01HostRequestStatus();
            (void)FT02_LR01HostRequestNodes();
            return FT02_COMM_INPUT_REDRAW;
        }
    }
    else if(g_mode == COMM_DIAG && cmd == 'r')
    {
        setSyncNotice("正在请求通讯模块刷新状态...");
        (void)FT02_LR01HostRequestStatus();
        (void)FT02_LR01HostRequestNodeInfo();
        (void)FT02_LR01HostRequestNodes();
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

bool FT02_CommunicationUIIsNodes()
{
    return g_mode == COMM_NODES;
}

void FT02_CommunicationUIOnNodeListUpdated(uint16_t count)
{
    if(g_mode != COMM_NODES) return;
    if(count == 0) setNotice("通讯模块当前未发现其他节点");
    else
    {
        char notice[64];
        snprintf(notice, sizeof(notice), "通讯模块节点列表已更新：%u", static_cast<unsigned>(count));
        setNotice(notice);
    }
}

void FT02_CommunicationUIOnSyncStarted(const char* noticeText)
{
    setSyncNotice(
        (noticeText != nullptr && noticeText[0] != '\0')
            ? noticeText
            : "通讯模块正在同步状态..."
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
