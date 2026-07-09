#pragma once

#include <M5Cardputer.h>
#include <SD.h>

// Navigation / track viewer module.
// Provides:
// - track file reading from /lanternbox/tracks/path_points.jsonl
// - overview of recorded path points
// - relative position plot
// - north-fixed compass navigation to BASE or selected path point

void navReloadTrackData();
void drawNavScreen();
void handleNavKey(const String& key);
