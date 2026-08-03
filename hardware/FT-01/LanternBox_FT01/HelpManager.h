#pragma once
#include <Arduino.h>


enum HelpType {
  HELP_HOME,
  HELP_RECORDER,
  HELP_NAVIGATION,
  HELP_NAV_MAP,
  HELP_NAV_COMPASS,
  HELP_DEVICE,
  HELP_SYNC,
  HELP_TASK,
  HELP_KNOWLEDGE,
  HELP_COMMUNICATION,
  HELP_AUDIO
};

void showHelp(HelpType type);
void drawHelpManager();
