#pragma once

#include <M5Cardputer.h>
#include <SD.h>

// Recorder page UI module.
// This module only draws the LOG / 路径记录 screen.
// It does not own SD, GNSS, session, or input logic.

void drawRecorderScreen();
