#ifndef CONFIG_H_
#define CONFIG_H_
#include <string>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

enum class Verbosity {
  silent,
  normal,
  verbose
};

struct Config {
  bool rebuild_all = false;
  bool track_external_headers = false;
  Verbosity log_verbosity = Verbosity::normal;
  std::string compiler = "g++";
  std::vector<fs::path> include_dirs= {
    fs::path("."), 
    fs::path("src")
  };

};

#endif
