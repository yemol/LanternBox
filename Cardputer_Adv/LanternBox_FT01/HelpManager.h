#pragma once
#include <Arduino.h>


enum HelpType {
  HELP_HOME,
  HELP_RECORDER,
  HELP_NAVIGATION,
  HELP_NAV_MAP,
  HELP_NAV_COMPASS,
  HELP_DEVICE,
  HELP_TASK,
  HELP_KNOWLEDGE,
  HELP_COMMUNICATION
};

void showHelp(HelpType type);
void handleHelpManagerKey(const String& key);

void drawHelpManager();
