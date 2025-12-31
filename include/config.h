#ifndef CONFIG_H_
#define CONFIG_H_
#include <string>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

enum class Verbosity {
  silent,
  normal,
  verbose,
  debug
};

enum class BuildMode {
  release,
  debug
};

struct Config {
  bool rebuild_all = false;
  bool track_external_headers = false;
  bool show_help = false;
  bool watch_mode = false;
  Verbosity log_verbosity = Verbosity::normal;
  BuildMode build_mode = BuildMode::release;
  std::string executable_name = "app";
  std::string root_dir = ".";
  std::string compiler = "g++";
  std::string config_file = "";
  int parallel_jobs = 1;
  std::vector<fs::path> include_dirs = {
    fs::path("include"), 
  };
  std::vector<std::string> compile_flags;
  std::vector<std::string> link_flags;
  std::vector<fs::path> watch_files;
};
#endif
