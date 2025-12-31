#include <filesystem>
#include <string>
#include "../include/cache.h" 
#include "../include/compiler.h"
#include "../include/scan.h"

void printHelp() {
  std::cerr << R"(
Usage: mkc [option]

Options:
  verbose          Enable verbose logging
  clean            Rebuild all files
  track-external   Rebuild files affected by external headers outside the working dir
)" << std::endl;
}


int main(int argc, char *argv[]) {
  Config config;
  if (argc == 2) {
    if (std::string(argv[1]) == "verbose") {
      config.log_verbosity = Verbosity::verbose;
    } else if (std::string(argv[1]) == "clean") {
      config.rebuild_all = true;
    } else if (std::string(argv[1]) == "track-external") {
      config.rebuild_all = true;
    } else {
      printHelp();
      return 1;
    }
  } else if (argc > 2) {
    printHelp();
    return 1;
  }
  Logger::setVerbosity(config.log_verbosity);

  // TODO: extract this into the init function.
  fs::create_directories("build");
  std::ofstream("build/log.out", std::ios::trunc).close();
  /////////////////////////////////////////



  scan(".");
  load_cache("build/.cache");

  for (auto &[_, src] : sources) {
    generate_deps(config.compiler, config.include_dirs, src);
    load_compiler_deps(src);
  }

  mark_modified(config);
  if (compile(config) != 0) {
    Logger::printLogfile("build/log.out");
    return 1;
  }
    
  save_cache("build/.cache");

  std::cout << std::endl;
  return 0;
}


// leverage this to create a cachefile on mkc init command
  // // Create parent directory if it doesn't exist
  // fs::path parentDir = cachePath.parent_path();
  // if (!parentDir.empty() && !fs::exists(parentDir)) {
  //   try {
  //     fs::create_directories(parentDir);
  //   } catch (const fs::filesystem_error &e) {
  //     Logger::failLog("save_cache(): failed to create directory \"" + readable_path(parentDir) + "\"", e.what());
  //     return;
  //   }
  // }

