#ifndef HELPERS_H_
#define HELPERS_H_ 
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "config.h"
namespace fs = std::filesystem;

// normalizes paths from "./logger.h" into "logger.h"
std::string normalize_path(const fs::path& p) {
  try {
    return fs::weakly_canonical(p).string();
  } catch (...) {
    return p.string();
  }
}

// relative instead of absolute for readability in the logs
std::string readable_path(const fs::path& p) {
  try {
    return fs::relative(p, fs::current_path()).string();
  } catch (...) {
    return p.string();
  }
}

bool is_under(const fs::path &p, const fs::path &dir) {
  auto canon_p = fs::weakly_canonical(p);
  auto canon_d = fs::weakly_canonical(dir);
  return std::mismatch(canon_d.begin(), canon_d.end(), canon_p.begin()).first == canon_d.end();
}

// TODO: apply watcher exclusion as well
bool is_excluded(const Config &conf, const fs::path &path, const std::string &ext) {
  if (conf.exclude_dirs.size() != 0) {
    bool excluded = false;
    for (const auto &excl : conf.exclude_dirs) {
      fs::path excl_path = fs::path(excl);
      if (path.filename() == excl_path.filename()) {
        excluded = true;
        break;
      }
      if (fs::is_directory(excl_path) && is_under(path, fs::path(conf.root_dir) / excl_path)) {
        excluded = true;
        break;
      }
    }
    if (excluded)
      return true;
  }
  if (conf.exclude_exts.size() != 0) {
    if (std::find(conf.exclude_exts.begin(), conf.exclude_exts.end(), ext) !=
        conf.exclude_exts.end())
      return true;
  }

  return false;
}

#endif
