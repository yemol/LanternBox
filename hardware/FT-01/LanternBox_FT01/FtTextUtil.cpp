#include "FtTextUtil.h"

namespace {
  int valueStart(const String& line, const String& key) {
    String needle = "\"" + key + "\"";
    int keyPos = line.indexOf(needle);
    if (keyPos < 0) return -1;

    int colon = line.indexOf(':', keyPos + needle.length());
    if (colon < 0) return -1;

    int start = colon + 1;
    while (start < line.length() && (line[start] == ' ' || line[start] == '\t')) start++;
    return start;
  }
}

String ftJsonEscape(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (int i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else out += c;
  }
  return out;
}

String ftJsonString(const String& line, const String& key) {
  int start = valueStart(line, key);
  if (start < 0 || start >= line.length() || line[start] != '"') return "";
  start++;

  String out;
  bool escaped = false;
  for (int i = start; i < line.length(); i++) {
    char c = line[i];
    if (escaped) {
      if (c == 'n') out += '\n';
      else if (c == 'r') out += '\r';
      else out += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      return out;
    } else {
      out += c;
    }
  }
  return "";
}

int ftJsonInt(const String& line, const String& key, int fallback) {
  int start = valueStart(line, key);
  if (start < 0) return fallback;

  int end = start;
  if (end < line.length() && line[end] == '-') end++;
  while (end < line.length() && line[end] >= '0' && line[end] <= '9') end++;
  if (end <= start || (end == start + 1 && line[start] == '-')) return fallback;
  return line.substring(start, end).toInt();
}

double ftJsonDouble(const String& line, const String& key, double fallback) {
  int start = valueStart(line, key);
  if (start < 0) return fallback;

  int end = start;
  while (end < line.length()) {
    char c = line[end];
    if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') end++;
    else break;
  }
  if (end <= start) return fallback;
  return line.substring(start, end).toDouble();
}

String ftBaseName(const String& path) {
  int slash = path.lastIndexOf('/');
  return slash < 0 ? path : path.substring(slash + 1);
}

String ftNormalizePath(const String& path, const String& directory) {
  if (path.startsWith("/")) return path;
  return directory + "/" + path;
}

String ftFormatBytes(uint64_t bytes, unsigned int decimals) {
  static const char* suffixes[] = {"B", "KB", "MB", "GB"};
  double value = (double)bytes;
  int suffix = 0;
  while (value >= 1024.0 && suffix < 3) {
    value /= 1024.0;
    suffix++;
  }
  if (suffix == 0) return String((uint32_t)bytes) + "B";
  return String(value, decimals) + suffixes[suffix];
}
