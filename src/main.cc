#include "../include/build_procedure.h"
#include "../include/file_watch.h"
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

  Logger::set_log_verbosity(config.log_verbosity);
  Logger::set_log_immediacy(config.log_immediately);

  // TODO: load config file
  if (!config.config_file.empty()) {
    // load and merge config from file
  }



  if (config.watch_mode) {
    Logger::warningLog("Watching for changes on root directory: \"" + config.root_dir + "\"");
    while (true) {
      if (file_watcher(config)) {
        try {
          build_procedure(config);
        } catch (...) {
          std::cout << std::endl;
          return 1;
        }
      }
    }
  }

  try {
    build_procedure(config);
  } catch (...) {
    std::cout << std::endl;
    return 1;
  }

  std::cout << std::endl;
  return 0;
}
