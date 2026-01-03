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
  int parallel_jobs = 1;
  bool rebuild_all            = false;
  bool track_external_headers = false;
  bool show_help              = false;
  bool watch_mode             = false;
  bool run_mode               = false;
  bool make_shared            = false;
  bool      log_immediately   = false;
  Verbosity log_verbosity     = Verbosity::normal;
  BuildMode build_mode        = BuildMode::release;
  std::string executable_name = "app";
  std::string compiler        = "g++";
  std::string root_dir        = ".";
  std::string config_file     = "";
  std::vector<fs::path>    exclude_dirs;
  // default tracked extension: .cpp, .c, .cc, .h, .hpp;
  std::vector<std::string> exclude_exts;
  std::vector<fs::path> include_dirs = {
    // fs::path("/usr/include/libdrm"),
    // fs::path("/usr/local/include/hyprland/protocols"),
    // fs::path("/usr/local/include/hyprland"),
    // fs::path("/usr/include/pango-1.0"),
    // fs::path("/usr/include/cairo"),
    // fs::path("/usr/include/pixman-1"),
    // fs::path("/usr/include/libmount"),
    // fs::path("/usr/include/blkid"),
    // fs::path("/usr/include/fribidi"),
    // fs::path("/usr/include/libxml2"),
    // fs::path("/usr/include/harfbuzz"),
    // fs::path("/usr/include/freetype2"),
    // fs::path("/usr/include/libpng16"),
    // fs::path("/usr/include/glib-2.0"),
    // fs::path("/usr/lib64/glib-2.0/include"),
    // fs::path("/usr/include/sysprof-6"),
  };
  std::vector<std::string> compile_flags;
  std::vector<std::string> link_flags;
};
#endif
