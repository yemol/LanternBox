#include "UiLoRaProbe.h"
#include "FtUiCommon.h"
#include "FtConfig.h"
#include "FtGnssContext.h"

namespace {
  const int MESSAGE_MAX_BYTES = (int)FT_MESH_USER_TEXT_MAX_BYTES;
  const uint32_t NODEINFO_RETRY_MS = 120000UL;
  const uint32_t NODEINFO_PENDING_RETRY_MS = 30000UL;
  const size_t NODE_LIST_ROWS = 5;

  size_t utf8CharBytes(uint8_t first) {
    if ((first & 0x80) == 0) return 1;
    if ((first & 0xE0) == 0xC0) return 2;
    if ((first & 0xF0) == 0xE0) return 3;
    if ((first & 0xF8) == 0xF0) return 4;
    return 1;
  }
}

UiLoRaProbe::UiLoRaProbe()
  : canvas(nullptr),
    lora(nullptr),
    exitRequested(false),
    composing(false),
    diagnosticMode(false),
    directoryMode(false),
    composeReturnToDirectory(false),
    lastNodeInfoRequestMs(0),
    lastObservedRxCount(0),
    lastObservedMessageRevision(0),
    selectedFrameIndex(0),
    selectedMessageIndex(0),
    selectedNodeIndex(0),
    nodeScrollOffset(0),
    composeTargetNode(0),
    composeText("") {
}

void UiLoRaProbe::begin(M5Canvas* targetCanvas, LoRaManager* targetLoRa) {
  canvas = targetCanvas;
  lora = targetLoRa;
}

void UiLoRaProbe::drawRow(int y, const String& label, const String& value, uint16_t color) {
  if (!canvas) return;
  canvas->setFont(&fonts::Font0);
  canvas->setTextSize(1);
  canvas->setTextDatum(top_left);
  canvas->setTextColor(DARKGREY, BLACK);
  canvas->setCursor(8, y);
  canvas->print(label);
  canvas->setTextColor(color, BLACK);
  canvas->setCursor(72, y);
  canvas->print(value);
}

void UiLoRaProbe::drawHeader(const char* title) {
  canvas->setFont(&fonts::efontCN_16);
  canvas->setTextSize(1);
  canvas->setTextDatum(top_left);
  canvas->setTextColor(GREEN, BLACK);
  canvas->setCursor(8, 4);
  canvas->print(title);

  canvas->setFont(&fonts::Font0);
  canvas->setTextColor(lora && lora->pkiReady() ? GREEN : ORANGE, BLACK);
  canvas->setCursor(150, 10);
  if (lora && lora->pkiReady()) {
    canvas->print(String("LOCK PKI:") + String(lora->pkiPeerCount()));
  } else {
    canvas->print("PKI ERROR");
  }
  canvas->drawLine(0, 24, canvas->width(), 24, DARKGREY);
}

String UiLoRaProbe::ageText(uint32_t ageMs) const {
  if (ageMs == 0xFFFFFFFF) return "--";
  if (ageMs < 1000) return "now";
  if (ageMs < 60000) return String(ageMs / 1000) + "s";
  if (ageMs < 3600000) return String(ageMs / 60000) + "m";
  return String(ageMs / 3600000) + "h";
}

String UiLoRaProbe::frameIndexText() const {
  if (!lora || lora->inboxCount() == 0) return "0/0";
  return String((int)selectedFrameIndex + 1) + "/" + String((int)lora->inboxCount());
}

String UiLoRaProbe::messageIndexText() const {
  if (!lora || lora->messageCount() == 0) return "0/0";
  return String((int)selectedMessageIndex + 1) + "/" + String((int)lora->messageCount());
}

String UiLoRaProbe::messageTypeText(const String& kind) const {
  if (kind == "MESH_PKI_TEXT") return "PKI 私信";
  if (kind == "MESH_TEXT") return "AES 频道";
  if (kind == "MESH_PKI_WAIT") return "等待密钥";
  if (kind == "MESH_PKI_FAIL") return "解密失败";
  return "消息";
}

uint16_t UiLoRaProbe::messageTypeColor(const String& kind) const {
  if (kind == "MESH_PKI_TEXT" || kind == "MESH_TEXT") return GREEN;
  if (kind == "MESH_PKI_WAIT") return ORANGE;
  if (kind == "MESH_PKI_FAIL") return RED;
  return WHITE;
}

void UiLoRaProbe::drawFooter(const char* text) {
  canvas->drawLine(0, canvas->height() - 12, canvas->width(), canvas->height() - 12, DARKGREY);
  canvas->setFont(&fonts::Font0);
  canvas->setTextColor(WHITE, BLACK);
  canvas->setCursor(5, canvas->height() - 10);
  canvas->print(text);
}

void UiLoRaProbe::drawWrappedMessage(const String& text, int x, int y, int maxUnits, int maxLines) {
  canvas->setFont(&fonts::efontCN_12);
  canvas->setTextColor(WHITE, BLACK);

  String line;
  int units = 0;
  int lineNo = 0;
  int pos = 0;
  while (pos < text.length() && lineNo < maxLines) {
    uint8_t first = (uint8_t)text[pos];
    size_t charBytes = utf8CharBytes(first);
    if (pos + (int)charBytes > text.length()) charBytes = 1;
    int charUnits = first < 0x80 ? 1 : 2;

    if (units + charUnits > maxUnits && line.length() > 0) {
      canvas->setCursor(x, y + lineNo * 14);
      canvas->print(line);
      line = "";
      units = 0;
      lineNo++;
      if (lineNo >= maxLines) break;
    }

    line += text.substring(pos, pos + (int)charBytes);
    units += charUnits;
    pos += (int)charBytes;
  }

  if (lineNo < maxLines && line.length() > 0) {
    if (pos < text.length() && units <= maxUnits - 2) line += "..";
    canvas->setCursor(x, y + lineNo * 14);
    canvas->print(line);
  }
}

void UiLoRaProbe::drawInbox() {
  drawHeader("LoRa 消息");

  size_t count = lora->messageCount();
  if (selectedMessageIndex >= count && count > 0) selectedMessageIndex = count - 1;

  LoRaFrameView frame;
  if (!lora->getMessageFrame(selectedMessageIndex, frame)) {
    canvas->setFont(&fonts::efontCN_16);
    canvas->setTextColor(WHITE, BLACK);
    canvas->setCursor(76, 47);
    canvas->print("暂无消息");

    canvas->setFont(&fonts::efontCN_12);
    canvas->setTextColor(lora->pkiPeerCount() == 0 ? ORANGE : DARKGREY, BLACK);
    canvas->setCursor(39, 76);
    if (lora->pkiPeerCount() == 0) {
      canvas->print("正在获取其他节点公钥");
    } else {
      canvas->print("正在监听加密通信");
    }

    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(DARKGREY, BLACK);
    canvas->setCursor(68, 99);
    canvas->print(String("RX:") + String(lora->rxCount()) + "  KEY:" + String(lora->pkiPeerCount()));
    drawFooter("T Cast  M Nodes  B Keys  ` Back");
    return;
  }

  canvas->setFont(&fonts::efontCN_12);
  canvas->setTextColor(WHITE, BLACK);
  canvas->setCursor(8, 29);
  String senderName = frame.lbxDevice.length() ? frame.lbxDevice : lora->nodeDisplayName(frame.from);
  canvas->print(String("来自 ") + senderName);

  canvas->setTextColor(messageTypeColor(frame.kind), BLACK);
  canvas->setCursor(166, 29);
  canvas->print(messageTypeText(frame.kind));
  canvas->drawLine(8, 43, canvas->width() - 8, 43, DARKGREY);

  String body = frame.text.length() ? frame.text : "--";
  drawWrappedMessage(body, 9, 49, 36, 4);

  canvas->setFont(&fonts::Font0);
  canvas->setTextColor(DARKGREY, BLACK);
  canvas->setCursor(8, 108);
  canvas->print(messageIndexText());
  canvas->setCursor(46, 108);
  canvas->print(ageText(frame.ageMs));
  canvas->setCursor(92, 108);
  canvas->print(String(frame.rssi, 0) + "dBm");
  canvas->setCursor(157, 108);
  canvas->print(String(frame.snr, 1) + "dB");

  drawFooter("T Cast M Nodes N/P Loop D Diag");
}

void UiLoRaProbe::drawDiagnostic() {
  drawHeader("LoRa 诊断");

  if (selectedFrameIndex >= lora->inboxCount() && lora->inboxCount() > 0) selectedFrameIndex = lora->inboxCount() - 1;
  LoRaFrameView frame;
  bool hasFrame = lora->getInboxFrame(selectedFrameIndex, frame);

  drawRow(29, "Radio", String(lora->currentProfileName()) + " " + String(lora->currentFreqMhz(), 3));
  drawRow(42, "PKI", String("Peers:") + String(lora->pkiPeerCount()) + " Wait:" + String(lora->pkiPendingCount()), lora->pkiPeerCount() ? GREEN : ORANGE);
  drawRow(55, "Crypto", String("OK:") + String(lora->pkiDecryptOkCount()) + " Ack:" + String(lora->ackSentCount()) + " Dup:" + String(lora->duplicateDropCount()));

  if (!hasFrame) {
    drawRow(70, "Frame", "--", ORANGE);
    drawRow(83, "Status", lora->lastStatus(), lora->isReady() ? GREEN : ORANGE);
  } else {
    drawRow(68, "Frame", frameIndexText() + " " + frame.kind, frame.kind == "MESH_PKI_FAIL" ? RED : WHITE);
    String senderName = frame.lbxDevice.length() ? frame.lbxDevice : lora->nodeDisplayName(frame.from);
    drawRow(81, "From", senderName);
    drawRow(94, "Packet", lora->formatMeshHex32(frame.id));
    drawRow(107, "Link", String(frame.rssi, 1) + "dBm " + String(frame.snr, 1) + "dB");
  }

  drawFooter("D Inbox B Node N/P Frame C Clear");
}

void UiLoRaProbe::drawNodeDirectory() {
  drawHeader("网络终端");

  const size_t count = lora->networkNodeCount();
  if (count == 0) {
    selectedNodeIndex = 0;
    nodeScrollOffset = 0;
    canvas->setFont(&fonts::efontCN_16);
    canvas->setTextColor(WHITE, BLACK);
    canvas->setCursor(68, 49);
    canvas->print("暂无终端");

    canvas->setFont(&fonts::efontCN_12);
    canvas->setTextColor(ORANGE, BLACK);
    canvas->setCursor(42, 78);
    canvas->print("按 B 请求节点信息");
    drawFooter("B Refresh  ` Back");
    return;
  }

  if (selectedNodeIndex >= count) selectedNodeIndex = count - 1;
  if (nodeScrollOffset > selectedNodeIndex) nodeScrollOffset = selectedNodeIndex;
  if (selectedNodeIndex >= nodeScrollOffset + NODE_LIST_ROWS) {
    nodeScrollOffset = selectedNodeIndex - NODE_LIST_ROWS + 1;
  }
  size_t maxOffset = count > NODE_LIST_ROWS ? count - NODE_LIST_ROWS : 0;
  if (nodeScrollOffset > maxOffset) nodeScrollOffset = maxOffset;

  for (size_t row = 0; row < NODE_LIST_ROWS; row++) {
    size_t index = nodeScrollOffset + row;
    if (index >= count) break;

    LoRaNodeView node;
    if (!lora->getNetworkNode(index, node)) continue;
    const int y = 28 + (int)row * 16;
    const bool selected = index == selectedNodeIndex;
    uint16_t background = selected ? DARKGREY : BLACK;
    canvas->fillRect(4, y - 1, canvas->width() - 8, 15, background);

    canvas->setFont(&fonts::efontCN_12);
    canvas->setTextColor(selected ? WHITE : LIGHTGREY, background);
    canvas->setCursor(8, y);
    canvas->print(selected ? ">" : " ");
    canvas->setCursor(20, y);
    canvas->print(node.displayName);

    // Repaint the capability cell after the name so a long device name cannot
    // cover the lock state.
    canvas->fillRect(194, y - 1, 40, 15, background);
    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(node.pkiAvailable ? GREEN : ORANGE, background);
    canvas->setCursor(200, y + 2);
    canvas->print(node.pkiAvailable ? "PKI" : "---");
  }

  LoRaNodeView selectedNode;
  bool hasSelected = lora->getNetworkNode(selectedNodeIndex, selectedNode);
  canvas->setFont(&fonts::Font0);
  canvas->setTextColor(DARKGREY, BLACK);
  canvas->setCursor(8, 109);
  canvas->print(String(selectedNodeIndex + 1) + "/" + String(count));
  if (hasSelected) {
    canvas->setCursor(48, 109);
    canvas->print(ageText(selectedNode.ageMs));
    canvas->setTextColor(selectedNode.pkiAvailable ? GREEN : ORANGE, BLACK);
    canvas->setCursor(112, 109);
    canvas->print(selectedNode.pkiAvailable ? "Enter: private" : "No public key");
  }

  drawFooter("Up/Dn Scroll Enter DM B Refresh");
}

void UiLoRaProbe::drawCompose() {
  const bool privateMessage = composeTargetNode != 0;
  drawHeader(privateMessage ? "发送私信" : "发送广播");

  FtGnssSnapshot gnssSnapshot;
  const bool attachLocation = ftGetFreshGnssSnapshot(gnssSnapshot);

  if (privateMessage) {
    canvas->setFont(&fonts::efontCN_12);
    canvas->setTextColor(GREEN, BLACK);
    canvas->setCursor(8, 29);
    canvas->print(String("至 ") + lora->nodeDisplayName(composeTargetNode));
    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(DARKGREY, BLACK);
    canvas->setCursor(8, 43);
    canvas->print(attachLocation ? "PKI + ACK  GPS:ON" : "PKI + ACK  GPS:--");
  } else {
    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(GREEN, BLACK);
    canvas->setCursor(8, 32);
    canvas->print(attachLocation ? "AES-256 broadcast  GPS:ON" : "AES-256 broadcast  GPS:--");
  }

  const int boxY = privateMessage ? 55 : 45;
  const int boxH = privateMessage ? 49 : 59;
  canvas->drawRect(7, boxY, canvas->width() - 14, boxH, DARKGREY);
  String shown = composeText + "|";
  int maxLines = privateMessage ? 4 : 5;
  for (int line = 0; line < maxLines; line++) {
    int start = line * 36;
    if (start >= shown.length()) break;
    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(WHITE, BLACK);
    canvas->setCursor(11, boxY + 5 + line * 11);
    canvas->print(shown.substring(start, start + 36));
  }

  canvas->setFont(&fonts::Font0);
  canvas->setTextColor(DARKGREY, BLACK);
  canvas->setCursor(170, 108);
  canvas->print(String(composeText.length()) + "/" + String(MESSAGE_MAX_BYTES));
  drawFooter("Enter Send  Del Delete  ` Cancel");
}

void UiLoRaProbe::beginCompose(uint32_t targetNode, bool returnToDirectory) {
  composing = true;
  composeTargetNode = targetNode;
  composeReturnToDirectory = returnToDirectory;
  composeText = "";
}

void UiLoRaProbe::endCompose() {
  composing = false;
  directoryMode = composeReturnToDirectory;
  composeReturnToDirectory = false;
  composeTargetNode = 0;
  composeText = "";
}

void UiLoRaProbe::openNodeDirectory() {
  diagnosticMode = false;
  directoryMode = true;
  size_t count = lora ? lora->networkNodeCount() : 0;
  if (count == 0) {
    selectedNodeIndex = 0;
    nodeScrollOffset = 0;
    requestNodeInfo();
  } else if (selectedNodeIndex >= count) {
    selectedNodeIndex = count - 1;
  }
}

void UiLoRaProbe::moveNodeSelection(int delta) {
  if (!lora) return;
  size_t count = lora->networkNodeCount();
  if (count == 0 || delta == 0) return;

  if (delta < 0) {
    if (selectedNodeIndex > 0) selectedNodeIndex--;
  } else if (selectedNodeIndex + 1 < count) {
    selectedNodeIndex++;
  }

  if (selectedNodeIndex < nodeScrollOffset) nodeScrollOffset = selectedNodeIndex;
  if (selectedNodeIndex >= nodeScrollOffset + NODE_LIST_ROWS) {
    nodeScrollOffset = selectedNodeIndex - NODE_LIST_ROWS + 1;
  }
}

void UiLoRaProbe::moveMessageSelection(int delta) {
  if (!lora) return;
  const size_t count = lora->messageCount();
  if (count == 0 || delta == 0) {
    selectedMessageIndex = 0;
    return;
  }

  // Message index 0 is the newest entry. N moves toward newer messages and
  // wraps from the newest entry to the oldest. P moves toward older messages
  // and wraps from the oldest entry back to the newest.
  if (selectedMessageIndex >= count) selectedMessageIndex = 0;
  if (delta < 0) {
    selectedMessageIndex = selectedMessageIndex == 0 ? count - 1 : selectedMessageIndex - 1;
  } else {
    selectedMessageIndex = (selectedMessageIndex + 1) % count;
  }
}

void UiLoRaProbe::requestNodeInfo() {
  if (!lora) return;
  lora->sendMeshtasticNodeInfo(true);
  lora->startListening();
  lastNodeInfoRequestMs = millis();
}

void UiLoRaProbe::activate() {
  exitRequested = false;
  composing = false;
  diagnosticMode = false;
  directoryMode = false;
  composeReturnToDirectory = false;
  composeTargetNode = 0;
  composeText = "";
  selectedFrameIndex = 0;
  selectedMessageIndex = 0;
  selectedNodeIndex = 0;
  nodeScrollOffset = 0;
  lastNodeInfoRequestMs = 0;
  if (lora) {
    if (!lora->isReady()) lora->begin();
    lora->startListening();
    lora->markMessagesRead();
    lastObservedRxCount = lora->rxCount();
    lastObservedMessageRevision = lora->messageRevision();
    if (lora->pkiReady()) requestNodeInfo();
  }
  draw();
}

void UiLoRaProbe::draw() {
  if (!canvas || !lora) return;
  canvas->fillSprite(BLACK);

  if (composing) {
    drawCompose();
  } else if (diagnosticMode) {
    drawDiagnostic();
  } else if (directoryMode) {
    drawNodeDirectory();
  } else {
    drawInbox();
  }
  canvas->pushSprite(0, 0);
}

void UiLoRaProbe::handleKey(const String& key) {
  if (!lora) return;

  if (composing) {
    if (key.indexOf("[DEL]") >= 0) {
      if (composeText.length() > 0) composeText.remove(composeText.length() - 1);
    } else if (FtKey::isEnter(key)) {
      bool sent = false;
      if (composeText.length() > 0) {
        bool locationAttached = false;
        const String outgoingText = ftBuildMessageWithGnss(composeText, locationAttached);
        sent = composeTargetNode != 0
                 ? lora->sendMeshtasticPrivateText(composeTargetNode, outgoingText)
                 : lora->sendMeshtasticText(outgoingText);
        Serial.print("MESH_TX_LOCATION attached=");
        Serial.print(locationAttached ? "true" : "false");
        if (locationAttached) {
          FtGnssSnapshot snapshot;
          if (ftGetFreshGnssSnapshot(snapshot)) {
            Serial.print(" age_ms=");
            Serial.print(snapshot.ageMs);
            Serial.print(" sats=");
            Serial.print(snapshot.satellites);
          }
        }
        Serial.println();
      }
      if (sent) endCompose();
    } else if (FtKey::isEsc(key)) {
      endCompose();
    } else {
      if (key.indexOf("[SPACE]") >= 0 && composeText.length() < MESSAGE_MAX_BYTES) composeText += ' ';
      int token = key.indexOf('[');
      String typed = token >= 0 ? key.substring(0, token) : key;
      for (int i = 0; i < typed.length() && composeText.length() < MESSAGE_MAX_BYTES; i++) {
        uint8_t c = (uint8_t)typed[i];
        if (c >= 0x20 && c <= 0x7E) composeText += (char)c;
      }
    }
    draw();
    return;
  }

  if (directoryMode) {
    if (FtKey::isEsc(key) || key.indexOf("[DEL]") >= 0) {
      directoryMode = false;
    } else if (FtKey::hasLetter(key, 'b', 'B')) {
      requestNodeInfo();
    } else if (FtKey::hasLetter(key, 't', 'T')) {
      beginCompose(0, true);
    } else if (FtKey::isEnter(key)) {
      LoRaNodeView node;
      if (lora->getNetworkNode(selectedNodeIndex, node)) {
        if (node.pkiAvailable) {
          beginCompose(node.node, true);
        } else {
          requestNodeInfo();
        }
      }
    } else if (FtKey::isUp(key) || FtKey::isLeft(key) || FtKey::hasLetter(key, 'p', 'P')) {
      moveNodeSelection(-1);
    } else if (FtKey::isDown(key) || FtKey::isRight(key) || FtKey::hasLetter(key, 'n', 'N')) {
      moveNodeSelection(1);
    }
    draw();
    return;
  }

  if (FtKey::isEsc(key) || key.indexOf("[DEL]") >= 0) {
    exitRequested = true;
    return;
  }

  if (FtKey::hasLetter(key, 'm', 'M') || FtKey::hasLetter(key, 'o', 'O')) {
    openNodeDirectory();
  } else if (FtKey::hasLetter(key, 'd', 'D')) {
    diagnosticMode = !diagnosticMode;
  } else if (FtKey::hasLetter(key, 't', 'T') || FtKey::isEnter(key)) {
    beginCompose(0, false);
  } else if (FtKey::hasLetter(key, 'b', 'B')) {
    requestNodeInfo();
  } else if (FtKey::hasLetter(key, 'n', 'N') || key.indexOf('.') >= 0) {
    if (diagnosticMode) {
      if (lora->inboxCount() > 0 && selectedFrameIndex > 0) selectedFrameIndex--;
    } else {
      moveMessageSelection(-1);
    }
  } else if (FtKey::hasLetter(key, 'p', 'P') || key.indexOf('/') >= 0) {
    if (diagnosticMode) {
      if (selectedFrameIndex + 1 < lora->inboxCount()) selectedFrameIndex++;
    } else {
      moveMessageSelection(1);
    }
  } else if (diagnosticMode && FtKey::hasLetter(key, 'c', 'C')) {
    lora->resetStats();
    selectedFrameIndex = 0;
    selectedMessageIndex = 0;
  } else if (diagnosticMode && FtKey::hasLetter(key, 'r', 'R')) {
    lora->startListening();
  }
  draw();
}

void UiLoRaProbe::tick() {
  if (!lora) return;
  const unsigned long now = millis();
  bool changed = false;

  // Radio reception is serviced globally from the main application loop.
  // The communication page only observes state changes and redraws itself.
  const uint32_t rxNow = lora->rxCount();
  if (rxNow != lastObservedRxCount) {
    lastObservedRxCount = rxNow;
    selectedFrameIndex = 0;
    changed = true;
  }

  const uint32_t revisionNow = lora->messageRevision();
  if (revisionNow != lastObservedMessageRevision) {
    lastObservedMessageRevision = revisionNow;
    selectedMessageIndex = 0;
    lora->markMessagesRead();
    changed = true;
  }

  if (!lora->isListening()) lora->startListening();

  const uint32_t nodeInfoRetry = lora->pkiPendingCount() > 0
                                   ? NODEINFO_PENDING_RETRY_MS
                                   : NODEINFO_RETRY_MS;
  if (!composing && (lora->pkiPeerCount() == 0 || lora->pkiPendingCount() > 0) &&
      now - lastNodeInfoRequestMs >= nodeInfoRetry) {
    requestNodeInfo();
    changed = true;
  }

  if (changed) draw();
}

bool UiLoRaProbe::wantsExit() const {
  return exitRequested;
}

void UiLoRaProbe::clearExit() {
  exitRequested = false;
}
