#include "../include/build_procedure.h"
#include "../include/parse_config.h"
#include "../include/file_watch.h"
#include "../include/pkg_config.h"
#include "../include/benchmark.h"
#include "../include/cli.h"

int main(int argc, char *argv[]) {
  Config config;
  try {
    config = parse_cli_args(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    printHelp();
    return 1;
  } catch (const int& x) {
    // NOTE: here we just init the working dir and leave
    build_procedure(config, true);
    generate_example_config(config.config_file);
    return 1;
  }

  if (config.show_help) {
    printHelp();
    return 0;
  }

  if (!config.config_file.empty()) {
    try {
      load_toml_config(config.config_file, config);
    } catch (const std::exception &e) {
      Logger::failLog("No config: proceeding with defaults.", e.what());
    } catch (...) {
      std::cout << std::endl;
      return 1;
    }
  };

  Logger::set_log_verbosity(config.log_verbosity);
  Logger::set_log_immediacy(config.log_immediately);

  for (auto& p : config.include_dirs) normalize_fs_path(p);
  for (auto& p : config.exclude_dirs) normalize_fs_path(p);
  if (config.unity_b) config.exclude_dirs.push_back(fs::weakly_canonical("build"));
  

  if (config.watch_mode) {
    Logger::warningLog("Watching for changes on root directory: \"" + config.root_dir + "\"");
    while (true) {
      if (file_watcher(config)) {
        try {
          std::unique_ptr<BuildTimer> timer;
          if (config.benchmark) timer = std::make_unique<BuildTimer>("finished in: ", config.benchmark_msg.c_str());
          resolve_pkg_config(config);
          build_procedure(config);
        } catch (...) {
          std::cout << std::endl;
          return 1;
        }
      }
    }
  }

  try {
    std::unique_ptr<BuildTimer> timer;
    if (config.benchmark) timer = std::make_unique<BuildTimer>("finished in: ", config.benchmark_msg.c_str());
    resolve_pkg_config(config);
    build_procedure(config);
  } catch (...) {
    std::cout << std::endl;
    return 1;
  }

  std::cout << std::endl;
  return 0;
}
