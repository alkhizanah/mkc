#include "../include/scan.h"
#include "../include/cache.h" 
#include "../include/compiler.h"
#include "../include/cli.h"

#define CACHE_PATH config.root_dir + "/build/.cache"
#define LOG_PATH config.root_dir + "/build/log.out"

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

  // TODO: load config file
  if (!config.config_file.empty()) {
    // load and merge config from file
  }
  
  // TODO: watch mode
  if (config.watch_mode) {
    // rebuild on changes to watch_files
  }

  try {
    init_working_dir(config.root_dir);
    scan(config.root_dir);
  } catch (const char *msg) {
    Logger::debug("failed at stage: " + std::string(msg));
    std::cout << std::endl;
    return 1;
  }

  load_cache(CACHE_PATH);

  for (auto &[_, src] : sources) {
    try {
      generate_deps(LOG_PATH, config, src);
      load_compiler_deps(src);
    } catch (const char *msg) {
      Logger::printLogfile(LOG_PATH);
      Logger::debug("failed at stage: " + std::string(msg));
      std::cout << std::endl;
      return 1;
    }
  }

  mark_modified(config);

  try {
    compile_and_link(config);
  } catch (const char *msg) {
    Logger::printLogfile(LOG_PATH);
    Logger::debug("failed at stage: " + std::string(msg));
    std::cout << std::endl;
    return 1;
  }

  save_cache(CACHE_PATH);

  std::cout << std::endl;
  return 0;
}

