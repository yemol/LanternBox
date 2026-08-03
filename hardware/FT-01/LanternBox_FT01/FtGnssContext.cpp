#include "FtGnssContext.h"
#include "FtConfig.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// GNSS state is owned by LanternBox_FT01.ino. This module provides the single
// read-only boundary used by communication features.
extern bool gnssFix;
extern int gnssSatellites;
extern double gnssLat;
extern double gnssLon;
extern unsigned long gnssLastFixMillis;

namespace {
  bool coordinatesValid(double latitude, double longitude) {
    return isfinite(latitude) && isfinite(longitude) &&
           latitude >= -90.0 && latitude <= 90.0 &&
           longitude >= -180.0 && longitude <= 180.0 &&
           !(latitude == 0.0 && longitude == 0.0);
  }
}

bool ftGetFreshGnssSnapshot(FtGnssSnapshot& out) {
  out.valid = false;
  out.latitude = 0.0;
  out.longitude = 0.0;
  out.satellites = -1;
  out.ageMs = 0;

  if (!gnssFix || gnssLastFixMillis == 0) return false;

  const uint32_t ageMs = (uint32_t)(millis() - gnssLastFixMillis);
  if (ageMs > FT_GNSS_MESSAGE_MAX_AGE_MS) return false;
  if (!coordinatesValid(gnssLat, gnssLon)) return false;

  out.valid = true;
  out.latitude = gnssLat;
  out.longitude = gnssLon;
  out.satellites = gnssSatellites;
  out.ageMs = ageMs;
  return true;
}

String ftBuildMessageWithGnss(const String& userText, bool& attached) {
  attached = false;

  FtGnssSnapshot snapshot;
  if (!ftGetFreshGnssSnapshot(snapshot)) return userText;

  char footer[48];
  const int written = snprintf(footer,
                               sizeof(footer),
                               " [GPS:%.6f,%.6f]",
                               snapshot.latitude,
                               snapshot.longitude);
  if (written <= 0 || (size_t)written >= sizeof(footer)) return userText;

  const size_t footerLen = (size_t)written;
  String result = userText;
  if ((size_t)result.length() + footerLen > FT_MESH_TEXT_PAYLOAD_MAX_BYTES) {
    const size_t keep = FT_MESH_TEXT_PAYLOAD_MAX_BYTES > footerLen
                          ? FT_MESH_TEXT_PAYLOAD_MAX_BYTES - footerLen
                          : 0;
    result = result.substring(0, (int)keep);
  }

  result += footer;
  attached = true;
  return result;
}
