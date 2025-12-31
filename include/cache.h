#ifndef CACHE_H_
#define CACHE_H_
#include <filesystem>
#include <string>
#include <fstream>
#include "logger.h"
#include "exceptions.h"
#include "helpers.h"
#include "containers.h"
namespace fs = std::filesystem;

void cleanTemp(const fs::path &tempPath) {
  try {
    if (fs::exists(tempPath)) {
      fs::remove(tempPath);
    }
  } catch (...) {
    Logger::warningLog("clean up error while removing: \"" + readable_path(tempPath) + "\"");
  }
}

void save_cache(const fs::path &cachePath) {
  fs::path tempPath = cachePath;
  tempPath += ".tmp";
  std::ofstream out(tempPath);
  ENABLE_EXCEPTIONS(out);

  try {
    for (auto &[_, s] : sources)
      out << normalize_path(s.path) << " " << s.hash << "\n";

    for (auto &[_, h] : headers)
      out << normalize_path(h.path) << " " << h.hash << "\n";

    out.flush();
    out.close();

    if (!out.good()) {
      throw std::ios_base::failure("Failed to write all data");
    }

    fs::rename(tempPath, cachePath);

  } catch (const std::ios_base::failure &e) {
    Logger::failLog("save_cache(): failed to write \"" + readable_path(cachePath) + "\"", e.what());
    cleanTemp(tempPath);
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("save_cache(): failed to rename temporary file", e.what());
    cleanTemp(tempPath);
  }

  for (const auto& inc : headers) {
    Logger::debug("cached header: " + readable_path(inc.second.path) + " | hash: " + std::to_string(inc.second.hash));
  }
}


void load_cache(const fs::path &cachePath) {
  if (!fs::exists(cachePath) || fs::is_empty(cachePath)) {
    Logger::warningLog("cache missing or empty, full rebuild required");
    return;
  }

  std::ifstream in(cachePath); 
  ENABLE_EXCEPTIONS(in);
  std::string path;
  uint64_t h;

  try {
    while (in >> path >> h) {
      old_hashes[path] = h;
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("load_cache(): failed to read cache at \"" + readable_path(cachePath) + "\"", e.what());
  }
}

#endif
