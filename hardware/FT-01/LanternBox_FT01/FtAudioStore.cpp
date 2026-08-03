#include "FtAudioStore.h"

#include <SD.h>
#include "FtHardware.h"
#include "FtTextUtil.h"

namespace {
  const char* AUDIO_DIR = "/lanternbox/audio";
  const char* AUDIO_INDEX_FILE = "/lanternbox/audio/index.jsonl";
  const char* AUDIO_INDEX_TMP_FILE = "/lanternbox/audio/index.tmp";

  bool ensureDir(const char* path) {
    return SD.exists(path) || SD.mkdir(path);
  }

  String indexFilename(const String& line) {
    String name = ftJsonString(line, "filename");
    if (name.length() == 0) name = ftBaseName(ftJsonString(line, "path"));
    if (name.length() == 0) name = ftBaseName(ftJsonString(line, "file"));
    return name;
  }
}

bool ftIsAudioWavFilename(const String& filename) {
  if (filename.length() == 0 || filename.startsWith(".") || filename.startsWith("._")) return false;
  if (filename.indexOf("/.") >= 0) return false;

  String lower = filename;
  lower.toLowerCase();
  return lower.startsWith("audio_") && lower.endsWith(".wav");
}

bool ftIsSafeAudioDeleteFilename(const String& filename) {
  return ftIsAudioWavFilename(filename) &&
         filename.indexOf('/') < 0 &&
         filename.indexOf('\\') < 0 &&
         filename.indexOf("..") < 0 &&
         filename.length() <= 64;
}

bool ftRewriteAudioIndexWithoutFile(const String& filename, int& removedOut, String& errorOut) {
  removedOut = 0;
  errorOut = "";
  FtHardware::prepareSharedSpiIdle(20000UL);

  if (!ensureDir("/lanternbox")) {
    errorOut = "mkdir_lanternbox_failed";
    return false;
  }
  if (!ensureDir(AUDIO_DIR)) {
    errorOut = "mkdir_audio_failed";
    return false;
  }

  if (!SD.exists(AUDIO_INDEX_FILE)) {
    File empty = SD.open(AUDIO_INDEX_FILE, FILE_WRITE);
    if (!empty) {
      errorOut = "open_index_failed";
      return false;
    }
    empty.flush();
    empty.close();
    return true;
  }

  File in = SD.open(AUDIO_INDEX_FILE, FILE_READ);
  if (!in) {
    errorOut = "open_index_failed";
    return false;
  }

  if (SD.exists(AUDIO_INDEX_TMP_FILE)) SD.remove(AUDIO_INDEX_TMP_FILE);
  File out = SD.open(AUDIO_INDEX_TMP_FILE, FILE_WRITE);
  if (!out) {
    in.close();
    errorOut = "open_temp_failed";
    return false;
  }

  while (in.available()) {
    String line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    String lineFilename = indexFilename(line);
    if (lineFilename == filename) {
      removedOut++;
      continue;
    }

    // Keep malformed/legacy metadata, but prune valid entries whose WAV is gone.
    if (lineFilename.length() > 0) {
      String fullPath = ftNormalizePath(lineFilename, AUDIO_DIR);
      if (!SD.exists(fullPath)) continue;
    }

    out.println(line);
    delay(1);
  }

  in.close();
  out.flush();
  out.close();

  if (SD.exists(AUDIO_INDEX_FILE) && !SD.remove(AUDIO_INDEX_FILE)) {
    SD.remove(AUDIO_INDEX_TMP_FILE);
    errorOut = "remove_index_failed";
    return false;
  }
  if (!SD.rename(AUDIO_INDEX_TMP_FILE, AUDIO_INDEX_FILE)) {
    errorOut = "rename_temp_failed";
    return false;
  }
  return true;
}

bool ftDeleteAudioAndIndex(const String& filenameInput, FtAudioDeleteResult& result) {
  result = FtAudioDeleteResult();
  String filename = filenameInput;
  filename.trim();

  if (!ftIsSafeAudioDeleteFilename(filename)) {
    result.error = "bad_filename";
    return false;
  }

  FtHardware::prepareSharedSpiIdle(20000UL);
  if (!ensureDir("/lanternbox") || !ensureDir(AUDIO_DIR)) {
    result.error = "mkdir_failed";
    return false;
  }

  String wavPath = String(AUDIO_DIR) + "/" + filename;
  if (SD.exists(wavPath)) {
    result.wavDeleted = SD.remove(wavPath);
    if (!result.wavDeleted) {
      result.error = "wav_remove_failed";
      return false;
    }
  } else {
    result.wavMissing = true;
  }

  result.indexOk = ftRewriteAudioIndexWithoutFile(filename, result.indexRemoved, result.error);
  return (result.wavDeleted || result.wavMissing) && result.indexOk;
}
