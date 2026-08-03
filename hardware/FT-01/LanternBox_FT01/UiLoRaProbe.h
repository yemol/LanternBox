#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>
#include "LoRaManager.h"

class UiLoRaProbe {
public:
  UiLoRaProbe();
  void begin(M5Canvas* targetCanvas, LoRaManager* targetLoRa);
  void activate();
  void draw();
  void handleKey(const String& key);
  void tick();
  bool wantsExit() const;
  void clearExit();

private:
  void drawRow(int y, const String& label, const String& value, uint16_t color = WHITE);
  void drawHeader(const char* title);
  void drawFooter(const char* text);
  void drawInbox();
  void drawDiagnostic();
  void drawNodeDirectory();
  void drawCompose();
  void drawWrappedMessage(const String& text, int x, int y, int maxUnits, int maxLines);
  String frameIndexText() const;
  String messageIndexText() const;
  String ageText(uint32_t ageMs) const;
  String messageTypeText(const String& kind) const;
  uint16_t messageTypeColor(const String& kind) const;
  void beginCompose(uint32_t targetNode = 0, bool returnToDirectory = false);
  void endCompose();
  void openNodeDirectory();
  void moveNodeSelection(int delta);
  void moveMessageSelection(int delta);
  void requestNodeInfo();

  M5Canvas* canvas;
  LoRaManager* lora;
  bool exitRequested;
  bool composing;
  bool diagnosticMode;
  bool directoryMode;
  bool composeReturnToDirectory;
  unsigned long lastNodeInfoRequestMs;
  uint32_t lastObservedRxCount;
  uint32_t lastObservedMessageRevision;
  size_t selectedFrameIndex;
  size_t selectedMessageIndex;
  size_t selectedNodeIndex;
  size_t nodeScrollOffset;
  uint32_t composeTargetNode;
  String composeText;
};
