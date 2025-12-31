#ifndef HELPERS_H_
#define HELPERS_H_ 
#include <filesystem>
#include <fstream>
#include <string>
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
#endif
