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

struct PkgDependency {
  std::string name;
};


struct StaticLib {
  fs::path name;
  fs::path archive;
  std::vector<fs::path> sources;
  std::vector<fs::path> include_dirs;
};

struct Config {
  int parallel_jobs           = 1;
  bool rebuild_all            = false;
  bool track_external_headers = false;
  bool show_help              = false;
  bool watch_mode             = false;
  bool run_mode               = false;
  bool make_shared            = false;
  bool log_immediately        = false;
  bool error_nums             = false;
  bool benchmark              = false;
  bool        unity_b         = false;
  fs::path    unity_src_name  = ""   ;
  fs::path    unity_obj              ;
  std::string benchmark_msg          ;
  std::string executable_name = "app";
  std::string compiler        = "g++";
  std::string root_dir        = "."  ;
  std::string config_file     = "mkc_config.toml";
  Verbosity log_verbosity     = Verbosity::normal;
  BuildMode build_mode        = BuildMode::release;
  // default tracked extension: .cpp, .c, .cc, .h, .hpp;
  std::vector<std::string> exclude_exts;
  std::vector<std::string> compile_flags;
  std::vector<std::string> link_flags;
  std::vector<fs::path> include_dirs;
  std::vector<PkgDependency> pkg_deps;
  std::vector<StaticLib> static_libs;
  std::vector<fs::path> explicit_sources;
  std::vector<fs::path> exclude_dirs = {
    fs::weakly_canonical("build")
  };
};
#endif
