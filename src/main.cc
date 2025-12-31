#include "../include/scan.h"
#include "../include/cache.h" 
#include "../include/compiler.h"
#include "../include/cli.h"


int main(int argc, char *argv[]) {
  Config config;

  try {
    config = parse_cli_args(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    printHelp();
    return 1;
  }
  
  if (config.show_help) {
    printHelp();
    return 0;
  }

  Logger::setVerbosity(config.log_verbosity);

  // TODO: load config file if specified
  if (!config.config_file.empty()) {
    // load and merge config from file
  }
  
  // TODO: watch mode
  if (config.watch_mode) {
    // rebuild on changes to watch_files
  }

  // TODO: extract this into the init function.
  fs::create_directories("build");
  std::ofstream("build/log.out", std::ios::trunc).close();
  /////////////////////////////////////////

  try {
    scan(config.root_dir);
  } catch (const char *msg) {
    Logger::debug("failed at stage: " + std::string(msg));
    std::cout << std::endl;
    return 1;
  }

  load_cache("build/.cache");

  for (auto &[_, src] : sources) {
    try {
      generate_deps(config.compiler, config.include_dirs, src);
      load_compiler_deps(src);
    } catch (const char *msg) {
      Logger::printLogfile("build/log.out");
      Logger::debug("failed at stage: " + std::string(msg));
      std::cout << std::endl;
      return 1;
    }
  }

  mark_modified(config);

  try {
    compile_and_link(config);
  } catch (const char *msg) {
    Logger::printLogfile("build/log.out");
    Logger::debug("failed at stage: " + std::string(msg));
    std::cout << std::endl;
    return 1;
  }

  save_cache("build/.cache");

  std::cout << std::endl;
  return 0;
}


// TODO: create a cachefile on mkc init command
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

